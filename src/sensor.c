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
#include "logger.h"
#include "i2c_rp2040.h"
#include "rtc_rp2040.h"
#include "sensor.h"
#include "TMP117.h"
#include "SHT4x.h"
#include "icp10125.h"
#include "ltr390.h"
#include "lc709203.h"
#include "nRF24L01.h"
#include "utils.h"

#define STATE_READ_TEMP             0x0100
#define STATE_READ_HUMIDITY_1       0x0200
#define STATE_READ_HUMIDITY_2       0x0201
#define STATE_READ_PRESSURE_1       0x0300
#define STATE_READ_PRESSURE_2       0x0301
#define STATE_READ_ALS              0x0400
#define STATE_READ_UVS              0x0410
#define STATE_READ_BATTERY_VOLTS    0x0500
#define STATE_READ_BATTERY_PERCENT  0x0501
#define STATE_READ_BATTERY_TEMP     0x0502
#define STATE_SEND_BEGIN            0x0700
#define STATE_SEND_FINISH           0x0701
#define STATE_SEND_PACKET           0x07FF

static uint8_t              buffer[32];
static char                 szBuffer[128];

int initSensors(i2c_inst_t * i2c) {
    return 
        tmp117_setup(i2c)   | 
        sht4x_setup(i2c)    | 
        icp10125_setup(i2c) | 
        ltr390_setup(i2c)   | 
        lc709203_setup(i2c);
}

void taskI2CSensor(PTASKPARM p) {
    static int          state = STATE_READ_TEMP;
    int                 i;
    int                 count = 0;
    uint8_t             input[2];
    weather_packet_t *  pWeather = getWeatherPacket();
    rtc_t               delay;

    switch (state) {
        case STATE_READ_TEMP:
            lgLogDebug("Rd T");

            i2cReadRegister(i2c0, TMP117_ADDRESS, TMP117_REG_TEMP, buffer, 2);

            pWeather->rawTemperature = copyI2CReg_int16(buffer);

            state = STATE_READ_HUMIDITY_1;
            delay = rtc_val_ms(1000);
            break;

        case STATE_READ_HUMIDITY_1:
            lgLogDebug("Rd H1");

            input[0] = SHT4X_CMD_MEASURE_HI_PRN;

            i2c_write_blocking(i2c0, SHT4X_ADDRESS, input, 1, true);

            state = STATE_READ_HUMIDITY_2;
            delay = rtc_val_ms(12);
            break;

        case STATE_READ_HUMIDITY_2:
            lgLogDebug("Rd H2");

            i2c_read_blocking(i2c0, SHT4X_ADDRESS, buffer, 6, false);

            pWeather->rawHumidity = copyI2CReg_uint16(&buffer[3]);

            state = STATE_READ_PRESSURE_1;
            delay = rtc_val_ms(988);
            break;

        case STATE_READ_PRESSURE_1:
            lgLogDebug("Rd P1");
            
            input[0] = 0x70;
            input[1] = 0xDF;

            i2c_write_blocking(i2c0, ICP10125_ADDRESS, input, 2, false);

            state = STATE_READ_PRESSURE_2;
            delay = rtc_val_ms(25);
            break;

        case STATE_READ_PRESSURE_2:
            lgLogDebug("Rd P2");
            
            i2c_read_blocking(i2c0, ICP10125_ADDRESS, buffer, 9, false);

            pWeather->rawICPTemperature = copyI2CReg_uint16(buffer);

            pWeather->rawICPPressure = 
                (uint32_t)(((uint32_t)buffer[3] << 16) | 
                            ((uint32_t)buffer[4] << 8) | 
                            (uint32_t)buffer[6]);

            state = STATE_READ_ALS;
            delay = rtc_val_ms(975);
            break;

        case STATE_READ_ALS:
            lgLogDebug("Rd ALS");

            i2cReadRegister(i2c0, LTR390_ADDRESS, LTR390_REG_ALS_DATA0, &buffer[0], 1);
            i2cReadRegister(i2c0, LTR390_ADDRESS, LTR390_REG_ALS_DATA1, &buffer[1], 1);
            i2cReadRegister(i2c0, LTR390_ADDRESS, LTR390_REG_ALS_DATA2, &buffer[2], 1);

            memset(&pWeather->rawALS_UV[0], 0, 5);
            memcpy(&pWeather->rawALS_UV[0], buffer, 3);

            input[0] = LTR390_CTRL_SENSOR_ENABLE | LTR390_CTRL_UVS_MODE_UVS;
            i2cWriteRegister(i2c0, LTR390_ADDRESS, LTR390_REG_CTRL, input, 1);

            delay = rtc_val_ms(1000);
            state = STATE_READ_UVS;
            break;

        case STATE_READ_UVS:
            lgLogDebug("Rd UVS");

            i2cReadRegister(i2c0, LTR390_ADDRESS, LTR390_REG_UVS_DATA0, &buffer[0], 1);
            i2cReadRegister(i2c0, LTR390_ADDRESS, LTR390_REG_UVS_DATA1, &buffer[1], 1);
            i2cReadRegister(i2c0, LTR390_ADDRESS, LTR390_REG_UVS_DATA2, &buffer[2], 1);

            pWeather->rawALS_UV[2] |= ((buffer[2] << 4) & 0xF0);
            pWeather->rawALS_UV[3] = buffer[1];
            pWeather->rawALS_UV[4] = buffer[0];

            input[0] = LTR390_CTRL_SENSOR_ENABLE | LTR390_CTRL_UVS_MODE_ALS;
            i2cWriteRegister(i2c0, LTR390_ADDRESS, LTR390_REG_CTRL, input, 1);

            delay = rtc_val_ms(1000);
            state = STATE_READ_BATTERY_VOLTS;
            break;

        case STATE_READ_BATTERY_VOLTS:
            lgLogDebug("Rd BV");
            if (lc709203_read_register(i2c0, LC709203_CMD_CELL_VOLTAGE, &pWeather->rawBatteryVolts)) {
                lgLogError("LC CRC fail");
            }

            lgLogDebug("BV: %.2f", (float)pWeather->rawBatteryVolts / 1000.0);

            delay = rtc_val_ms(1000);
            state = STATE_READ_BATTERY_PERCENT;
            break;

        case STATE_READ_BATTERY_PERCENT:
            lgLogDebug("Rd BP");
            if (lc709203_read_register(i2c0, LC709203_CMD_ITE, &pWeather->rawBatteryPercentage)) {
                lgLogError("LC CRC fail");
            }

            lgLogDebug("BP: %.2f", (float)pWeather->rawBatteryPercentage / 10.0);

            delay = rtc_val_ms(1000);
            state = STATE_READ_BATTERY_TEMP;
            break;

        case STATE_READ_BATTERY_TEMP:
            lgLogDebug("Rd BT");
            if (lc709203_read_register(i2c0, LC709203_CMD_CELL_TEMERATURE, &pWeather->rawBatteryTemperature)) {
                lgLogError("LC CRC fail");
            }

            lgLogDebug("BT: %.2f", ((float)pWeather->rawBatteryTemperature / 10.0) - 273.15);

            delay = rtc_val_sec(20);
            state = STATE_SEND_BEGIN;
            break;

        case STATE_SEND_BEGIN:
            nRF24L01_powerUpTx(spi0);

            delay = rtc_val_ms(150);
            state = STATE_SEND_PACKET;
            break;

        case STATE_SEND_PACKET:
            memcpy(buffer, pWeather, sizeof(weather_packet_t));

            for (i = 0;i < sizeof(weather_packet_t); i++) {
                count += sprintf(&szBuffer[count], "%02X", buffer[i]);
            }
            lgLogDebug("txBuffer: %s", szBuffer);

            nRF24L01_transmit_buffer(spi0, buffer, sizeof(weather_packet_t), false);

            delay = rtc_val_ms(125);
            state = STATE_SEND_FINISH;
            break;

        case STATE_SEND_FINISH:
            nRF24L01_powerDown(spi0);

            delay = rtc_val_ms(125);
            state = STATE_READ_TEMP;
            break;
    }

    scheduleTask(TASK_I2C_SENSOR, delay, false, NULL);
}
