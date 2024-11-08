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
#include "watchdog.h"
#include "battery.h"
#include "TMP117.h"
#include "SHT4x.h"
#include "icp10125.h"
#include "ltr390.h"
#include "lc709203.h"
#include "max17048.h"
#include "nRF24L01.h"
#include "gpio_def.h"
#include "utils.h"

#define STATE_I2C_INIT              0x0001
#define STATE_I2C_INIT2             0x0002
#define STATE_START                 0x0003
#define STATE_SETUP_I2C0            0x0010
#define STATE_SETUP_I2C1            0x0020
#define STATE_SETUP_LC709203        0x0030
#define STATE_READ_TEMP             0x0100
#define STATE_READ_HUMIDITY_1       0x0200
#define STATE_READ_HUMIDITY_2       0x0201
#define STATE_READ_PRESSURE_1       0x0300
#define STATE_READ_PRESSURE_2       0x0301
#define STATE_READ_PRESSURE_3       0x0302
#define STATE_LTR390_ENABLE         0x0400
#define STATE_READ_ALS              0x0410
#define STATE_READ_UVS              0x0420
#define STATE_READ_BATTERY_VOLTS    0x0500
#define STATE_READ_BATTERY_PERCENT  0x0501
#define STATE_READ_BATTERY_TEMP     0x0502
#define STATE_READ_BATTERY_CRATE    0x0503
#define STATE_SEND_BEGIN            0x0700
#define STATE_SEND_FINISH           0x0701
#define STATE_CRC_FAILURE_1         0x0900
#define STATE_CRC_FAILURE_2         0x0901
#define STATE_CRC_FAILURE_3         0x0902
#define STATE_SEND_PACKET           0x07FF

#define CRC_FAIL_COUNT_LIMIT        3

static uint8_t              buffer[32];
static char                 szBuffer[128];

int nullSetup(i2c_inst_t * i2c) {
    return 0;
}

static void setPacketNumber(weather_packet_t * p) {
    static uint32_t             packetNum = 0;

    packetNum &= 0x00FFFFFF;

    p->packetNum[0] = (uint8_t)(packetNum & 0x000000FF);
    p->packetNum[1] = (uint8_t)((packetNum >> 8) & 0x000000FF);
    p->packetNum[2] = (uint8_t)((packetNum >> 16) & 0x000000FF);

    packetNum++;
}

static int registerSensorsI2C0(void) {
    int         rtn = 0;

    i2c_bus_open(i2c0, 4);

    i2c_bus_register_device(i2c0, TMP117_ADDRESS, &tmp117_setup);
    i2c_bus_register_device(i2c0, SHT4X_ADDRESS, &sht4x_setup);
    i2c_bus_register_device(i2c0, ICP10125_ADDRESS, &icp10125_setup);
    i2c_bus_register_device(i2c0, MAX17048_ADDRESS, &max17048_setup);
//    i2c_bus_register_device(i2c0, LTR390_ADDRESS, &ltr390_setup);

    return rtn;
}

void taskI2CSensor(PTASKPARM p) {
    static int                  state = STATE_START;
    static int                  crcFailCount = 0;
    static weather_packet_t     lastPacket;
    int                         i;
    int                         count = 0;
    int                         bytesRead = 0;
    int                         p_LSB;
    int                         t_LSB;
    int                         icpPressure;
    uint8_t                     input[2];
    rtc_t                       delay;

    weather_packet_t *          pWeather = getWeatherPacket();

    switch (state) {
        case STATE_START:
            lgLogDebug("I2C Start");

            registerSensorsI2C0();

            memset(pWeather, 0, sizeof(weather_packet_t));

            /*
            ** Deliberately fall through...
            */

        case STATE_I2C_INIT:
            lgLogDebug("I2C Init");

            /*
            ** Deliberately fall through...
            */

        case STATE_I2C_INIT2:
            lgLogDebug("I2C Init2");

            i2c_init(i2c0, 400000);

            delay = rtc_val_sec(1);
            state = STATE_SETUP_I2C0;
            break;

        case STATE_SETUP_I2C0:
            lgLogDebug("I2C0 Setup");

            i2c_bus_setup(i2c0);

            state = STATE_READ_TEMP;
            delay = rtc_val_sec(50);
            break;

        case STATE_READ_TEMP:
            lgLogDebug("Rd T");

            bytesRead = i2cReadRegister(i2c0, TMP117_ADDRESS, TMP117_REG_TEMP, buffer, 2);

            if (bytesRead > 0) {
                pWeather->rawTemperature = copyI2CReg_int16(buffer);
                lastPacket.rawTemperature = pWeather->rawTemperature;
            }
            else {
                pWeather->rawTemperature = lastPacket.rawTemperature;
                pWeather->status |= STATUS_BITS_TMP117_I2C_ERROR;
            }

            state = STATE_READ_HUMIDITY_1;
            delay = rtc_val_ms(1000);
            break;

        case STATE_READ_HUMIDITY_1:
            lgLogDebug("Rd H1");

            input[0] = SHT4X_CMD_MEASURE_HI_PRN;

            i2cWriteTimeoutProtected(i2c0, SHT4X_ADDRESS, input, 1, true);

            state = STATE_READ_HUMIDITY_2;
            delay = rtc_val_ms(20);
            break;

        case STATE_READ_HUMIDITY_2:
            lgLogDebug("Rd H2");

            bytesRead = i2cReadTimeoutProtected(i2c0, SHT4X_ADDRESS, buffer, 6, false);

            if (bytesRead > 0) {
                pWeather->rawHumidity = copyI2CReg_uint16(&buffer[3]);
                lastPacket.rawHumidity = pWeather->rawHumidity;
            }
            else {
                pWeather->rawHumidity = lastPacket.rawHumidity;
                pWeather->status |= STATUS_BITS_SHT4X_I2C_ERROR;
            }

            state = STATE_READ_PRESSURE_1;
            delay = rtc_val_ms(980);
            break;

        case STATE_READ_PRESSURE_1:
            lgLogDebug("Rd P1");

            icp10125_read_otp(i2c0);

            state = STATE_READ_PRESSURE_2;
            delay = rtc_val_ms(50);
            break;

        case STATE_READ_PRESSURE_2:
            lgLogDebug("Rd P2");
            
            input[0] = 0x70;
            input[1] = 0xDF;

            i2cWriteTimeoutProtected(i2c0, ICP10125_ADDRESS, input, 2, false);

            state = STATE_READ_PRESSURE_3;
            delay = rtc_val_ms(30);
            break;

        case STATE_READ_PRESSURE_3:
            lgLogDebug("Rd P3");
            
            bytesRead = i2cReadTimeoutProtected(i2c0, ICP10125_ADDRESS, buffer, 9, false);

            if (bytesRead == 9) {
                t_LSB = copyI2CReg_uint16(buffer);

                p_LSB = (int)(((int)buffer[3] << 16) | 
                                ((int)buffer[4] << 8) | 
                                (int)buffer[6]);

                icp10125_process_data(p_LSB, t_LSB, &icpPressure);

                pWeather->rawICPPressure = (uint32_t)icpPressure;
                lastPacket.rawICPPressure = pWeather->rawICPPressure;
            }
            else {
                pWeather->rawICPPressure = lastPacket.rawICPPressure;
                pWeather->status |= STATUS_BITS_ICP10125_I2C_ERROR;
            }

            state = STATE_READ_BATTERY_VOLTS;
            delay = rtc_val_ms(920);
            break;

        case STATE_READ_BATTERY_VOLTS:
            lgLogDebug("Rd BV");

            bytesRead = i2cReadRegister(i2c0, MAX17048_ADDRESS, MAX17048_REG_VCELL, buffer, 2);            

            lgLogDebug("MAX Bytes read: %d", bytesRead);

            if (bytesRead > 0) {
                pWeather->rawBatteryVolts = copyI2CReg_uint16(buffer);
                lastPacket.rawBatteryVolts = pWeather->rawBatteryVolts;
                lgLogDebug("BV: %.2f", (float)pWeather->rawBatteryVolts * 78.125f / 1000000.0f);
            }
            else {
                pWeather->rawBatteryVolts = lastPacket.rawBatteryVolts;
                pWeather->status |= STATUS_BITS_MAX17048_BV_I2C_ERROR;
            }

            delay = rtc_val_ms(1000);
            state = STATE_READ_BATTERY_PERCENT;
            break;

        case STATE_READ_BATTERY_PERCENT:
            lgLogDebug("Rd BP");

            bytesRead = i2cReadRegister(i2c0, MAX17048_ADDRESS, MAX17048_REG_SOC, buffer, 2);            

            lgLogDebug("MAX Bytes read: %d", bytesRead);

            if (bytesRead > 0) {
                pWeather->rawBatteryPercentage = buffer[0];
                lastPacket.rawBatteryPercentage = pWeather->rawBatteryPercentage;
                lgLogDebug("BP: %d", (int)pWeather->rawBatteryPercentage);
            }
            else {
                pWeather->rawBatteryPercentage = lastPacket.rawBatteryPercentage;
                pWeather->status |= STATUS_BITS_MAX17048_BP_I2C_ERROR;
            }

            delay = rtc_val_ms(1000);
            state = STATE_READ_BATTERY_CRATE;
            break;

        case STATE_READ_BATTERY_CRATE:
            lgLogDebug("Rd BCR");

            bytesRead = i2cReadRegister(i2c0, MAX17048_ADDRESS, MAX17048_REG_CRATE, buffer, 2);            

            lgLogDebug("MAX Bytes read: %d", bytesRead);

            if (bytesRead > 0) {
                pWeather->rawBatteryChargeRate = copyI2CReg_int16(buffer);
                lastPacket.rawBatteryChargeRate = pWeather->rawBatteryChargeRate;
                lgLogDebug("BCR: %.2f", (float)pWeather->rawBatteryChargeRate * 0.208f);
            }
            else {
                pWeather->rawBatteryChargeRate = lastPacket.rawBatteryChargeRate;
                pWeather->status |= STATUS_BITS_MAX17048_BCR_I2C_ERROR;
            }

            if (isDebugActive()) {
                delay = rtc_val_sec(53);
            }
            else {
                if (pWeather->rawBatteryPercentage < BATTERY_PERCENTAGE_MEDIUM) {
                    delay = rtc_val_hr(1);
                }
                else if (pWeather->rawBatteryPercentage < BATTERY_PERCENTAGE_OK) {
                    delay = rtc_val_min(20);
                }
                else {
                    delay = rtc_val_sec(293);
                }
            }

            state = STATE_SEND_BEGIN;
            break;

        case STATE_SEND_BEGIN:
            nRF24L01_powerUpTx(spi0);

            setPacketNumber(pWeather);

            delay = rtc_val_ms(150);
            state = STATE_SEND_PACKET;
            break;

        case STATE_SEND_PACKET:
            memcpy(buffer, pWeather, sizeof(weather_packet_t));

            for (i = 0;i < sizeof(weather_packet_t); i++) {
                count += sprintf(&szBuffer[count], "%02X", buffer[i]);
            }
            lgLogDebug("txBuffer: %s", szBuffer);

            /*
            ** Reset the rainfall count so we start counting again
            ** for the next message...
            */
            pWeather->rawRainfall = 0;

            nRF24L01_transmit_buffer(spi0, buffer, sizeof(weather_packet_t), false);

            delay = rtc_val_ms(130);
            state = STATE_SEND_FINISH;
            break;

        case STATE_SEND_FINISH:
            nRF24L01_powerDown(spi0);

            pWeather->status = 0x0000;

            if (crcFailCount > 9) {
                crcFailCount = 0;

                /*
                ** Attempt to recover...
                */
                state = STATE_CRC_FAILURE_1;
                delay = rtc_val_ms(10);
                break;
            }

            delay = rtc_val_ms(120);
            state = STATE_READ_TEMP;
            break;

        case STATE_CRC_FAILURE_1:
            lc709203_reset(i2c0);

            delay = rtc_val_ms(100);
            state = STATE_CRC_FAILURE_2;
            break;

        case STATE_CRC_FAILURE_2:
            i2c_deinit(i2c0);

            delay = rtc_val_ms(100);
            state = STATE_CRC_FAILURE_3;
            break;

        case STATE_CRC_FAILURE_3:
            /*
            ** Don't update the watchdog timer, this will reset
            ** the device in 100ms...
            */
            triggerWatchdogReset();

            delay = rtc_val_sec(10);
            state = STATE_CRC_FAILURE_3;
            break;
    }

    scheduleTask(TASK_I2C_SENSOR, delay, false, NULL);
}
