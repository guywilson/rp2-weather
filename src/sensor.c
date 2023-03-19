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
#include "utils.h"

#define STATE_READ_TEMP          0x0010
#define STATE_READ_HUMIDITY_1    0x0020
#define STATE_READ_HUMIDITY_2    0x0030
#define STATE_READ_PRESSURE_1    0x0040
#define STATE_READ_PRESSURE_2    0x0050
#define STATE_READ_PRESSURE_0    0x0055
#define STATE_READ_LUX           0x0060

weather_packet_t            weather;

char    pszBuffer[256];

int initSensors(i2c_inst_t * i2c) {
    weather.chipID = * ((io_ro_32 *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET));

    return tmp117_setup(i2c) | sht4x_setup(i2c) | icp10125_setup(i2c);
}

void taskI2CSensor(PTASKPARM p) {
    static int          state = STATE_READ_TEMP;
    int                 error;
    rtc_t               delay = rtc_val_ms(4000);
    uint8_t             reg;
    uint8_t             buffer[32];
//    uint16_t            otpValues[4];

    switch (state) {
        case STATE_READ_TEMP:
            uart_puts(uart0, "T.");
            i2cReadRegister(i2c0, TMP117_ADDRESS, TMP117_REG_TEMP, buffer, 2);
            weather.rawTemperature = copyI2CReg_int16(buffer);

            delay = rtc_val_ms(4000);

            state = STATE_READ_HUMIDITY_1;
            break;

        case STATE_READ_HUMIDITY_1:
            uart_puts(uart0, "H1.");
            reg = SHT4X_CMD_MEASURE_HI_PRN;
            i2c_write_blocking(i2c0, SHT4X_ADDRESS, &reg, 1, true);

            delay = rtc_val_ms(10);

            state = STATE_READ_HUMIDITY_2;
            break;

        case STATE_READ_HUMIDITY_2:
            uart_puts(uart0, "H2.");
            i2c_read_blocking(i2c0, SHT4X_ADDRESS, buffer, 6, false);
            weather.rawHumidity = copyI2CReg_uint16(&buffer[3]);

            delay = rtc_val_ms(3990);

            state = STATE_READ_PRESSURE_1;
            break;

        // case STATE_READ_PRESSURE_0:
        //     uart_puts(uart0, "P0.");
        //     buffer[0] = 0xC5;
        //     buffer[1] = 0x95;
        //     buffer[2] = 0x00;
        //     buffer[3] = 0x66;
        //     buffer[4] = 0x9C;

        //     error = i2c_write_blocking(i2c0, ICP10125_ADDRESS, buffer, 5, false);

        //     if (error == PICO_ERROR_GENERIC) {
        //         return -1;
        //     }
        //     else if (error == PICO_ERROR_TIMEOUT) {
        //         return -1;
        //     }
            
        //     buffer[0] = 0xC7;
        //     buffer[1] = 0xF7;

        //     for (i = 0; i < 4; i++) {
        //         i2c_write_blocking(i2c0, ICP10125_ADDRESS, buffer, 2, false);
        //         i2c_read_blocking(i2c0, ICP10125_ADDRESS, buffer, 3, false);

        //         otpValues[i] = (uint16_t)((uint16_t)buffer[0] << 8 | (uint16_t)buffer[1]);

        //         sprintf(pszBuffer, "OTP:0x%04X ");
        //         uart_puts(uart0, pszBuffer);
        //     }

        //     uart_puts(uart0, "\n");

        //     delay = rtc_val_ms(4000);

        //     state = STATE_READ_PRESSURE_1;
        //     break;

        case STATE_READ_PRESSURE_1:
            uart_puts(uart0, "P1.");
            buffer[0] = 0x70;
            buffer[1] = 0xDF;

            error = i2c_write_blocking(i2c0, ICP10125_ADDRESS, buffer, 2, false);

            if (error == PICO_ERROR_GENERIC) {
                uart_puts(uart0, "Failed to address pressure sensor\n");
            }
            else if (error == 0) {
                uart_puts(uart0, "Wrote 0 bytes to pressure sensor\n");
            }
            else {
                uart_puts(uart0, "Successfully wrote bytes to pressure sensor\n");
            }

            delay = rtc_val_ms(25);

            state = STATE_READ_PRESSURE_2;
            break;

        case STATE_READ_PRESSURE_2:
            uart_puts(uart0, "P2.");
            error = i2c_read_blocking(i2c0, ICP10125_ADDRESS, buffer, 9, false);

            if (error == PICO_ERROR_GENERIC) {
                uart_puts(uart0, "Failed to address pressure sensor\n");
            }
            else if (error == 0) {
                uart_puts(uart0, "Read 0 bytes from pressure sensor\n");
            }
            else {
                uart_puts(uart0, "Successfully read bytes from pressure sensor\n");
            }

            weather.rawICPTemperature = copyI2CReg_uint16(&buffer[0]);

            weather.rawICPPressure = 
                (uint32_t)(((uint32_t)buffer[3] << 16) | 
                            ((uint32_t)buffer[4] << 8) | 
                            (uint32_t)buffer[6]);

            sprintf(pszBuffer, "rawT:%d, rawP:%u\n", weather.rawICPTemperature, weather.rawICPPressure);
            uart_puts(uart0, pszBuffer);

            delay = rtc_val_ms(3975);

            state = STATE_READ_LUX;
            break;

        case STATE_READ_LUX:
            uart_puts(uart0, "L\n");
            memcpy(buffer, &weather, sizeof(weather_packet_t));
            nRF24L01_transmit_buffer(spi0, buffer, sizeof(weather_packet_t), false);

            delay = rtc_val_ms(4000);

            state = STATE_READ_TEMP;
            break;
    }

    scheduleTaskOnce(TASK_I2C_SENSOR, delay, NULL);
}
