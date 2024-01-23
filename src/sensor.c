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
#include "SHT4X.h"
#include "icp10125.h"
#include "ltr390.h"
#include "lc709203.h"
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

    i2c_bus_open(i2c0, 3);

    i2c_bus_register_device(i2c0, TMP117_ADDRESS, &tmp117_setup);
    i2c_bus_register_device(i2c0, SHT4X_ADDRESS, &sht4x_setup);
    i2c_bus_register_device(i2c0, ICP10125_ADDRESS, &icp10125_setup);

    return rtn;
}

static int registerSensorsI2C1(void) {
    int         rtn = 0;

    i2c_bus_open(i2c1, 2);

    i2c_bus_register_device(i2c1, LC709203_ADDRESS, &nullSetup);
    i2c_bus_register_device(i2c1, LTR390_ADDRESS, &ltr390_setup);

    return rtn;
}

void taskI2CSensor(PTASKPARM p) {
    static int                  state = STATE_START;
    static int                  crcFailCount = 0;
    static bool                 isI2C1Initialised = false;
    static weather_packet_t     lastPacket;
    int                         i;
    int                         count = 0;
    int                         bytesRead = 0;
    int                         p_LSB;
    int                         t_LSB;
    int                         icpPressure;
    int                         error;
    uint8_t                     input[2];
    rtc_t                       delay;

    weather_packet_t *          pWeather = getWeatherPacket();

    switch (state) {
        case STATE_START:
            lgLogDebug("I2C Start");

            registerSensorsI2C0();
            registerSensorsI2C1();

            memset(pWeather, 0, sizeof(weather_packet_t));

            /*
            ** Deliberately fall through...
            */

        case STATE_I2C_INIT:
            lgLogDebug("I2C Init");

#ifdef I2C_POWER_SAVE
            /*
            ** Turn on the I2C power...
            */
            gpio_put(I2C0_POWER_PIN, true);

            delay = rtc_val_sec(1);
            state = STATE_I2C_INIT2;
            break;
#endif
            /*
            ** Deliberately fall through...
            */

        case STATE_I2C_INIT2:
            lgLogDebug("I2C Init2");

            i2c_init(i2c0, 400000);

            delay = rtc_val_sec(1);

            if (isI2C1Initialised) {
                state = STATE_SETUP_I2C0;
            }
            else {
                i2c_init(i2c1, 100000);
                state = STATE_SETUP_LC709203;
            }
            break;

        case STATE_SETUP_LC709203:
            lgLogDebug("LC Setup");

            lc709203_setup(i2c1);

            delay = rtc_val_sec(10);
            state = STATE_SETUP_I2C0;
            break;

        case STATE_SETUP_I2C0:
            lgLogDebug("I2C0 Setup");

            i2c_bus_setup(i2c0);

            if (isI2C1Initialised) {
                state = STATE_READ_TEMP;
                delay = rtc_val_sec(10);
            }
            else {
                state = STATE_SETUP_I2C1;
                delay = rtc_val_ms(50);
            }
            break;

        case STATE_SETUP_I2C1:
            lgLogDebug("I2C1 Setup");

            error = i2c_bus_setup(i2c1);

            if (error) {
                lgLogError("Error setting up i2c1");
            }

            isI2C1Initialised = true;

            delay = rtc_val_ms(50);
            state = STATE_READ_TEMP;
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

//            /*
//            ** @todo skip over for oscilloscope debugging...
//            */
//            state = STATE_READ_BATTERY_VOLTS;
            state = STATE_READ_ALS;
            delay = rtc_val_ms(920);
            break;

        case STATE_READ_ALS:
            lgLogDebug("Rd ALS");

            bytesRead = i2cReadRegister(i2c1, LTR390_ADDRESS, LTR390_REG_ALS_DATA0, buffer, 3);

            memset(&pWeather->rawALS_UV[0], 0, 6);

            if (bytesRead > 0) {
                lgLogDebug("Rx: %02X %02X %02X", buffer[0], buffer[1], buffer[2]);
                memcpy(&pWeather->rawALS_UV[0], buffer, 3);

                memcpy(&lastPacket.rawALS_UV[0], &pWeather->rawALS_UV[0], 3);
            }
            else {
                lgLogError("ALS Error");
                memcpy(&pWeather->rawALS_UV[0], &lastPacket.rawALS_UV[0], 3);
                pWeather->status |= STATUS_BITS_LTR390_ALS_I2C_ERROR;
            }

            input[0] = LTR390_CTRL_SENSOR_ENABLE | LTR390_CTRL_UVS_MODE_UVS;
            i2cWriteRegister(i2c1, LTR390_ADDRESS, LTR390_REG_CTRL, input, 1);

            delay = rtc_val_ms(1000);
            state = STATE_READ_UVS;
            break;

        case STATE_READ_UVS:
            lgLogDebug("Rd UVS");

            bytesRead = i2cReadRegister(i2c1, LTR390_ADDRESS, LTR390_REG_UVS_DATA0, buffer, 3);

            if (bytesRead > 0) {
                lgLogDebug("Rx: %02X %02X %02X", buffer[0], buffer[1], buffer[2]);
                memcpy(&pWeather->rawALS_UV[3], buffer, 3);

                memcpy(&lastPacket.rawALS_UV[3], &pWeather->rawALS_UV[3], 3);
            }
            else {
                memcpy(&pWeather->rawALS_UV[3], &lastPacket.rawALS_UV[3], 3);
                pWeather->status |= STATUS_BITS_LTR390_UVI_I2C_ERROR;
            }

            input[0] = LTR390_CTRL_SENSOR_ENABLE | LTR390_CTRL_UVS_MODE_ALS;
            i2cWriteRegister(i2c1, LTR390_ADDRESS, LTR390_REG_CTRL, input, 1);

            delay = rtc_val_ms(1000);
            state = STATE_READ_BATTERY_VOLTS;
            break;

        case STATE_READ_BATTERY_VOLTS:
            lgLogDebug("Rd BV");
            
            bytesRead = lc709203_read_register(i2c1, LC709203_CMD_CELL_VOLTAGE, &pWeather->rawBatteryVolts);

            lgLogDebug("LC Bytes read: %d", bytesRead);

            switch (bytesRead) {
                case LC709203_ERROR_CRC:
                    lgLogError("LC crc_err");
                    pWeather->status |= STATUS_BITS_LC709203_BV_I2C_CRC_ERROR;
                    crcFailCount++;
                    break;

                case PICO_ERROR_TIMEOUT:
                    lgLogError("LC tmo_err");
                    pWeather->status |= STATUS_BITS_LC709203_BV_I2C_TIMEOUT_ERROR;
                    break;

                case PICO_ERROR_GENERIC:
                    lgLogError("LC gen_err");
                    pWeather->status |= STATUS_BITS_LC709203_BV_I2C_GEN_ERROR;
                    break;

                default:
                    lastPacket.rawBatteryVolts = pWeather->rawBatteryVolts;
                    lgLogDebug("BV: %.2f", (float)pWeather->rawBatteryVolts / 1000.0);
                    break;
            }

            if (bytesRead < 0) {
                pWeather->rawBatteryVolts = lastPacket.rawBatteryVolts;
            }

            delay = rtc_val_ms(1000);
            state = STATE_READ_BATTERY_PERCENT;
            break;

        case STATE_READ_BATTERY_PERCENT:
            lgLogDebug("Rd BP");

            bytesRead = lc709203_read_register(i2c1, LC709203_CMD_ITE, &pWeather->rawBatteryPercentage);

            lgLogDebug("LC Bytes read: %d", bytesRead);

            switch (bytesRead) {
                case LC709203_ERROR_CRC:
                    lgLogError("LC crc_err");
                    crcFailCount++;
                    pWeather->status |= STATUS_BITS_LC709203_BP_I2C_CRC_ERROR;
                    break;

                case PICO_ERROR_TIMEOUT:
                    lgLogError("LC tmo_err");
                    pWeather->status |= STATUS_BITS_LC709203_BP_I2C_TIMEOUT_ERROR;
                    break;

                case PICO_ERROR_GENERIC:
                    lgLogError("LC gen_err");
                    pWeather->status |= STATUS_BITS_LC709203_BP_I2C_GEN_ERROR;
                    break;

                default:
                    lastPacket.rawBatteryPercentage = pWeather->rawBatteryPercentage;
                    lgLogDebug("BP: %.2f", (float)pWeather->rawBatteryPercentage / 10.0);
                    break;
            }

            if (bytesRead < 0) {
                pWeather->rawBatteryPercentage = lastPacket.rawBatteryPercentage;
            }

#ifdef I2C_POWER_SAVE
            /*
            ** Turn off the I2C power...
            */
            i2c_deinit(i2c0);
            gpio_put(I2C0_POWER_PIN, false);

            /*
            ** Wait 5 minutes between packets...
            */
            if (isDebugActive()) {
                delay = rtc_val_sec(32);
            }
            else {
                delay = rtc_val_sec(272);
            }
#else
            if (isDebugActive()) {
                delay = rtc_val_sec(53);
            }
            else {
                delay = rtc_val_sec(293);
            }
#endif
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
#ifdef I2C_POWER_SAVE
            state = STATE_I2C_INIT;
#else
            state = STATE_READ_TEMP;
#endif
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
