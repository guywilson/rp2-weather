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

static char                 szTemp[64];
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

void taskReadTemp(PTASKPARM p) {
    uint8_t             tempRegister[2];
    int16_t             tempInt;
    uint8_t             buffer[32];

//    sensor_chain_t *    sensor;

//    sensor = (sensor_chain_t *)p;

//    startTime = timer_hw->timerawl;

    i2cReadRegister(i2c0, TMP117_ADDRESS, TMP117_REG_TEMP, tempRegister, 2);

    tempInt = (int16_t)((((int16_t)tempRegister[0]) << 8) | (int16_t)tempRegister[1]);
    weather.temperature = (float)tempInt * 0.0078125;

    memcpy(buffer, &weather, sizeof(weather_packet_t));

    nRF24L01_transmit_buffer(spi0, buffer, sizeof(weather_packet_t), false);

//    sensor = sensor->next;

//    scheduleTaskOnce(sensor->taskID, rtc_val_ms(4000), sensor);
    scheduleTaskOnce(TASK_READ_TEMP, rtc_val_ms(4000), NULL);
}

void taskReadHumidity(PTASKPARM p) {
    uint8_t             regBuffer[6];
    uint16_t            humidityResp;
    sensor_chain_t *    sensor;

    sensor = (sensor_chain_t *)p;

    i2cReadRegister(i2c0, SHT4X_ADDRESS, SHT4X_CMD_MEASURE_HI_PRN, regBuffer, 6);

    humidityResp = (uint16_t)regBuffer[3];

    weather.humidity = -6.0f + (125.0f * ((float)humidityResp / 65535.0f));

    sensor = sensor->next;

    scheduleTaskOnce(sensor->taskID, rtc_val_ms(4000), sensor);
}

void taskReadPressure(PTASKPARM p) {
//    static uint8_t      buffer[sizeof(weather_packet_t)];
//    float               pressure;
    sensor_chain_t *    sensor;

    sensor = (sensor_chain_t *)p;

    sprintf(szTemp, "T:%.2f/H:%.2f/P:%.2f\n", weather.temperature, weather.humidity, weather.pressure);
    nRF24L01_transmit_string(spi0, szTemp, false);

//    memcpy(buffer, &weather, sizeof(weather_packet_t));
//    nRF24L01_transmit_buffer(spi0, buffer, sizeof(weather_packet_t), false);

    sensor = sensor->next;

    scheduleTaskOnce(sensor->taskID, rtc_val_ms(4000), sensor);
}
