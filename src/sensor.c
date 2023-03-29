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
#include "logger.h"
#include "i2c_rp2040.h"
#include "rtc_rp2040.h"
#include "sensor.h"
#include "TMP117.h"
#include "SHT4x.h"
#include "icp10125.h"
#include "veml7700.h"
#include "nRF24L01.h"
#include "utils.h"

#define STATE_READ_TEMP_1           0x0100
#define STATE_READ_TEMP_2           0x0101
#define STATE_READ_HUMIDITY_1       0x0200
#define STATE_READ_HUMIDITY_2       0x0201
#define STATE_READ_PRESSURE_1       0x0300
#define STATE_READ_PRESSURE_2       0x0301
#define STATE_READ_LUX_1            0x0400
#define STATE_READ_LUX_2            0x0401

weather_packet_t            weather;
static uint8_t              buffer[32];

weather_packet_t * getWeatherPacket() {
    return &weather;
}

int initSensors(i2c_inst_t * i2c) {
    weather.chipID = * ((io_ro_32 *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET));

    return tmp117_setup(i2c) | sht4x_setup(i2c) | icp10125_setup(i2c) | veml7700_setup(i2c);
}

void taskI2CSensor(PTASKPARM p) {
    static int          state = STATE_READ_TEMP_1;
    rtc_t               delay = rtc_val_ms(4000);
    uint8_t             reg;

    switch (state) {
        case STATE_READ_TEMP_1:
            lgLogDebug("Rd T1");
            reg = TMP117_REG_TEMP;
            i2cTriggerReadRegister(
                            i2c0, 
                            TASK_I2C_SENSOR, 
                            rtc_val_ms(5), 
                            TMP117_ADDRESS, 
                            &reg, 
                            1, 
                            buffer, 
                            2, 
                            true, 
                            false);

            state = STATE_READ_TEMP_2;
            return;

        case STATE_READ_TEMP_2:
            lgLogDebug("Rd T2");
            weather.rawTemperature = copyI2CReg_int16(buffer);

            delay = rtc_val_ms(2000);

            state = STATE_READ_HUMIDITY_1;
            break;

        case STATE_READ_HUMIDITY_1:
            lgLogDebug("Rd H1");
            reg = SHT4X_CMD_MEASURE_HI_PRN;
            i2cTriggerReadRegister(
                            i2c0, 
                            TASK_I2C_SENSOR, 
                            rtc_val_ms(10), 
                            SHT4X_ADDRESS, 
                            &reg, 
                            1, 
                            buffer, 
                            6, 
                            true, 
                            false);

            state = STATE_READ_HUMIDITY_2;
            return;

        case STATE_READ_HUMIDITY_2:
            lgLogDebug("Rd H2");

            weather.rawHumidity = copyI2CReg_uint16(&buffer[3]);

            delay = rtc_val_ms(1990);

            state = STATE_READ_PRESSURE_1;
            break;

        case STATE_READ_PRESSURE_1:
            lgLogDebug("Rd P1");
            buffer[0] = 0x70;
            buffer[1] = 0xDF;

            i2cTriggerReadRegister(
                            i2c0, 
                            TASK_I2C_SENSOR, 
                            rtc_val_ms(24), 
                            ICP10125_ADDRESS, 
                            buffer, 
                            2, 
                            buffer, 
                            9, 
                            false,
                            false);

            state = STATE_READ_PRESSURE_2;
            return;

        case STATE_READ_PRESSURE_2:
            lgLogDebug("Rd P2");

            weather.rawICPTemperature = copyI2CReg_uint16(&buffer[0]);

            weather.rawICPPressure = 
                (uint32_t)(((uint32_t)buffer[3] << 16) | 
                            ((uint32_t)buffer[4] << 8) | 
                            (uint32_t)buffer[6]);

            delay = rtc_val_ms(1975);

            state = STATE_READ_LUX_1;
            break;

        case STATE_READ_LUX_1:
            lgLogDebug("Rd L1");
            buffer[0] = VEML7700_REG_ALS;

            i2cTriggerReadRegister(
                            i2c0, 
                            TASK_I2C_SENSOR, 
                            rtc_val_ms(1), 
                            VEML7700_ADDRESS, 
                            buffer, 
                            1, 
                            buffer, 
                            2, 
                            true,
                            false);

            state = STATE_READ_LUX_2;
            return;

        case STATE_READ_LUX_2:
            /*
            ** The veml7700 seems to have the opposite response structure
            ** to other sensors, i.e. LSB then MSB...
            */
            weather.rawLux = ((uint16_t)((((uint16_t)buffer[1]) << 8) | (uint16_t)buffer[0]));

            memcpy(buffer, &weather, sizeof(weather_packet_t));
            nRF24L01_transmit_buffer(spi0, buffer, sizeof(weather_packet_t), false);

            delay = rtc_val_ms(4000);

            state = STATE_READ_TEMP_1;
            break;
    }

    scheduleTaskOnce(TASK_I2C_SENSOR, delay, NULL);
}
