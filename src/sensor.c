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
#include "lc709203.h"
#include "nRF24L01.h"
#include "utils.h"

#define STATE_READ_TEMP             0x0100
#define STATE_READ_HUMIDITY         0x0200
#define STATE_READ_PRESSURE         0x0300
#define STATE_READ_LUX              0x0400
#define STATE_READ_BATTERY_VOLTS    0x0500
#define STATE_READ_BATTERY_PERCENT  0x0501
#define STATE_READ_BATTERY_TEMP     0x0502
#define STATE_SEND_BEGIN            0x0700
#define STATE_SEND_FINISH           0x0701
#define STATE_SEND_PACKET           0x07FF

static uint8_t              buffer[32];

weather_packet_t * getWeatherPacket() {
    static weather_packet_t     weather;

    weather.packetID[0] = 'W';
    weather.packetID[1] = 'P';
    
    return &weather;
}

int initSensors(i2c_inst_t * i2c) {
    weather_packet_t *      pWeather = getWeatherPacket();

    pWeather->chipID = * ((io_ro_32 *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET));

    return tmp117_setup(i2c) | sht4x_setup(i2c) | icp10125_setup(i2c) | veml7700_setup(i2c) | lc709203_setup(i2c);
}

void taskI2CSensor(PTASKPARM p) {
    static int          state = STATE_READ_TEMP;
    weather_packet_t *  pWeather = getWeatherPacket();
    rtc_t               delay;

    switch (state) {
        case STATE_READ_TEMP:
            lgLogDebug("Rd T");

            i2cReadRegister(i2c0, TMP117_ADDRESS, TMP117_REG_TEMP, buffer, 2);

            pWeather->rawTemperature = copyI2CReg_int16(buffer);

            state = STATE_READ_HUMIDITY;
            delay = rtc_val_ms(1000);
            break;

        case STATE_READ_HUMIDITY:
            lgLogDebug("Rd H");

            i2cReadRegister(i2c0, SHT4X_ADDRESS, SHT4X_CMD_MEASURE_HI_PRN, buffer, 6);

            pWeather->rawHumidity = copyI2CReg_uint16(&buffer[3]);

            state = STATE_READ_PRESSURE;
            delay = rtc_val_ms(1000);
            break;

        case STATE_READ_PRESSURE:
            lgLogDebug("Rd P");
            buffer[0] = 0x70;
            buffer[1] = 0xDF;

            i2c_write_blocking(i2c0, ICP10125_ADDRESS, buffer, 2, false);
            i2c_read_blocking(i2c0, ICP10125_ADDRESS, buffer, 9, false);

            pWeather->rawICPTemperature = copyI2CReg_uint16(&buffer[0]);

            pWeather->rawICPPressure = 
                (uint32_t)(((uint32_t)buffer[3] << 16) | 
                            ((uint32_t)buffer[4] << 8) | 
                            (uint32_t)buffer[6]);

            state = STATE_READ_LUX;
            delay = rtc_val_ms(1000);
            break;

        case STATE_READ_LUX:
            lgLogDebug("Rd L");
            buffer[0] = VEML7700_REG_ALS;

            i2cReadRegister(i2c0, VEML7700_ADDRESS, VEML7700_REG_ALS, buffer, 2);

            /*
            ** The veml7700 seems to have the opposite response structure
            ** to other sensors, i.e. LSB then MSB...
            */
            pWeather->rawLux = ((uint16_t)((((uint16_t)buffer[1]) << 8) | (uint16_t)buffer[0]));

            delay = rtc_val_ms(1000);
            state = STATE_READ_BATTERY_VOLTS;
            break;

        case STATE_READ_BATTERY_VOLTS:
            lgLogDebug("Rd BV");
            if (lc709203_read_register(i2c0, LC709203_CMD_CELL_VOLTAGE, &pWeather->rawBatteryVolts)) {
                lgLogError("LC CRC fail");
            }

            delay = rtc_val_ms(1000);
            state = STATE_READ_BATTERY_PERCENT;
            break;

        case STATE_READ_BATTERY_PERCENT:
            lgLogDebug("Rd BP");
            if (lc709203_read_register(i2c0, LC709203_CMD_ITE, &pWeather->rawBatteryPercentage)) {
                lgLogError("LC CRC fail");
            }

            delay = rtc_val_ms(1000);
            state = STATE_READ_BATTERY_TEMP;
            break;

        case STATE_READ_BATTERY_TEMP:
            lgLogDebug("Rd BT");
            if (lc709203_read_register(i2c0, LC709203_CMD_CELL_TEMERATURE, &pWeather->rawBatteryTemperature)) {
                lgLogError("LC CRC fail");
            }

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
