#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "scheduler.h"
#include "taskdef.h"
#include "i2c_rp2040.h"
#include "rtc_rp2040.h"
#include "sensor.h"
#include "TMP117.h"
#include "SHT4x.h"
#include "nRF24L01.h"

weather_packet_t            weather;

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

int initSensors(i2c_inst_t * i2c) {
    int         rtn;

    rtn = tmp117_setup(i2c);

    if (rtn) {
        return rtn;
    }

    return 0;
}

void taskReadTemp(PTASKPARM p) {
    uint8_t             tempRegister[2];
    int16_t             tempInt;
    // sensor_chain_t *    sensor = (sensor_chain_t *)p;

    i2cReadRegister(i2c0, TMP117_ADDRESS, TMP117_REG_TEMP, tempRegister, 2);

    tempInt = (int16_t)((((int16_t)tempRegister[0]) << 8) | (int16_t)tempRegister[1]);
    weather.temperature = (float)tempInt * 0.0078125;

    // memcpy(buffer, &weather, sizeof(weather_packet_t));
    // nRF24L01_transmit_buffer(spi0, buffer, sizeof(weather_packet_t), false);

    // sensor = (sensor_chain_t *)sensor->next;

    scheduleTaskOnce(TASK_READ_HUMIDITY, rtc_val_ms(4000), NULL);
}

void taskReadHumidity(PTASKPARM p) {
    uint8_t             regBuffer[6];
    uint16_t            humidityResp;
    // sensor_chain_t *    sensor = (sensor_chain_t *)p;

    i2cReadRegister(i2c0, SHT4X_ADDRESS, SHT4X_CMD_MEASURE_HI_PRN, regBuffer, 6);

    humidityResp = (uint16_t)regBuffer[3];

    weather.humidity = -6.0f + ((float)humidityResp * 0.0019074);

    if (weather.humidity < 0.0) {
        weather.humidity = 0.0;
    }
    else if (weather.humidity > 100.0) {
        weather.humidity = 100.0;
    }

    // sensor = (sensor_chain_t *)sensor->next;

    scheduleTaskOnce(TASK_READ_PRESSURE, rtc_val_ms(4000), NULL);
}

void taskReadPressure(PTASKPARM p) {
    static uint8_t      buffer[sizeof(weather_packet_t)];
    // sensor_chain_t *    sensor = (sensor_chain_t *)p;

    weather.pressure = 1021.41;

    memcpy(buffer, &weather, sizeof(weather_packet_t));
    nRF24L01_transmit_buffer(spi0, buffer, sizeof(weather_packet_t), false);

    // sensor = (sensor_chain_t *)sensor->next;

    scheduleTaskOnce(TASK_READ_TEMP, rtc_val_ms(4000), NULL);
}
