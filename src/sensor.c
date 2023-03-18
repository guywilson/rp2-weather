#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "hardware/address_mapped.h"
#include "hardware/regs/tbman.h"
#include "hardware/regs/sysinfo.h"
#include "scheduler.h"
#include "taskdef.h"
#include "i2c_rp2040.h"
#include "rtc_rp2040.h"
#include "sensor.h"
#include "TMP117.h"
#include "SHT4x.h"
#include "icp10125.h"
#include "nRF24L01.h"


weather_packet_t            weather;

inline int16_t copyI2CReg_int16(uint8_t * reg) {
    return ((int16_t)((((int16_t)reg[0]) << 8) | (int16_t)reg[1]));
}

inline uint16_t copyI2CReg_uint16(uint8_t * reg) {
    return ((uint16_t)((((uint16_t)reg[0]) << 8) | (uint16_t)reg[1]));
}

int tmp117_setup(i2c_inst_t * i2c) {
    int                 error = 0;
    uint8_t             deviceIDValue[2];
    uint16_t            deviceID;
    uint8_t             configData[2];

    error = i2cReadRegister(i2c, TMP117_ADDRESS, TMP117_REG_DEVICE_ID, deviceIDValue, 2);

    if (error < 0) {
        return error;
    }
    else {
        deviceID = ((uint16_t)deviceIDValue[0]) << 8 | (uint16_t)deviceIDValue[1];

        if (deviceID != 0x0117) {
            return PICO_ERROR_GENERIC;
        }
    }

    /*
    ** Continuous conversion, 8 sample average, 4 sec cycle time...
    */
    configData[0] = 0x02;
    configData[1] = 0xA0;
    error = i2cWriteRegister(i2c0, TMP117_ADDRESS, TMP117_REG_CONFIG, configData, 2);

    if (error == PICO_ERROR_GENERIC) {
        uart_puts(uart0, "ERR_GEN\n");
    }
    else if (error == PICO_ERROR_TIMEOUT) {
        uart_puts(uart0, "ERR_TM\n");
    }

    return 0;
}

int sht4x_setup(i2c_inst_t * i2c) {
    int         error;
    uint8_t     reg;

    reg = SHT4X_CMD_SOFT_RESET;

    error = i2c_write_blocking(i2c0, SHT4X_ADDRESS, &reg, 1, false);

    if (error == PICO_ERROR_GENERIC) {
        uart_puts(uart0, "ERR_GEN\n");
        return -1;
    }
    else if (error == PICO_ERROR_TIMEOUT) {
        uart_puts(uart0, "ERR_TM\n");
        return -1;
    }

    return 0;
}

int initSensors(i2c_inst_t * i2c) {
    weather.chipID = * ((io_ro_32 *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET));

    return tmp117_setup(i2c) | sht4x_setup(i2c) | icp10125_setup(i2c);
}

void taskReadTemp(PTASKPARM p) {
    uint8_t             tempRegister[2];

    i2cReadRegister(i2c0, TMP117_ADDRESS, TMP117_REG_TEMP, tempRegister, 2);

    weather.rawTemperature = copyI2CReg_int16(tempRegister);

    scheduleTaskOnce(TASK_READ_HUMIDITY_1, rtc_val_ms(4000), NULL);
}

void taskReadHumidity_step1(PTASKPARM p) {
    uint8_t             reg;

    reg = SHT4X_CMD_MEASURE_HI_PRN;

    i2c_write_blocking(i2c0, SHT4X_ADDRESS, &reg, 1, true);

    scheduleTaskOnce(TASK_READ_HUMIDITY_2, rtc_val_ms(15), NULL);
}

void taskReadHumidity_step2(PTASKPARM p) {
    uint8_t             regBuffer[6];

    i2c_read_blocking(i2c0, SHT4X_ADDRESS, regBuffer, 6, false);

    weather.rawHumidity = copyI2CReg_uint16(&regBuffer[3]);

    scheduleTaskOnce(TASK_READ_PRESSURE_1, rtc_val_ms(3985), NULL);
}

void taskReadPressure_step1(PTASKPARM p) {
    uint8_t             regBuffer[2];

    regBuffer[0] = 0x70;
    regBuffer[1] = 0xDF;

    i2c_write_blocking(i2c0, ICP10215_ADDRESS, regBuffer, 2, true);

    scheduleTaskOnce(TASK_READ_PRESSURE_2, rtc_val_ms(25), NULL);
}

void taskReadPressure_step2(PTASKPARM p) {
    uint32_t            rawPressure;
    uint16_t            rawTemperature;
    uint8_t             paBuf[9];
    static uint8_t      buffer[sizeof(weather_packet_t)];

    i2c_read_blocking(i2c0, ICP10215_ADDRESS, paBuf, 9, false);

    rawTemperature = (uint16_t)(((uint16_t)paBuf[0] << 8) | (uint16_t)paBuf[1]);
    rawPressure = (uint32_t)(((uint32_t)paBuf[3] << 16) | ((uint32_t)paBuf[4] << 8) | (uint32_t)paBuf[6]);

    weather.pressurePa = icp10125_get_pressure(rawTemperature, rawPressure);

    memcpy(buffer, &weather, sizeof(weather_packet_t));
    nRF24L01_transmit_buffer(spi0, buffer, sizeof(weather_packet_t), true);

    scheduleTaskOnce(TASK_READ_TEMP, rtc_val_ms(3975), NULL);
}
