#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "scheduler.h"
#include "taskdef.h"
#include "i2ctask.h"
#include "i2c_rp2040.h"
#include "rtc_rp2040.h"

static char                szTemp[64];

// bool reserved_addr(uint8_t addr) {
//     return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
// }

// void TaskTriggerTemp(PTASKPARM p) {
//     uart_puts(uart0, "I2C Bus Scan\n");
//     uart_puts(uart0, "   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

//     for (int addr = 0; addr < (1 << 7); ++addr) {
//         if (addr % 16 == 0) {
//             sprintf(szTemp, "%02x ", addr);
//             uart_puts(uart0, szTemp);
//         }

//         // Perform a 1-byte dummy read from the probe address. If a slave
//         // acknowledges this address, the function returns the number of bytes
//         // transferred. If the address byte is ignored, the function returns
//         // -1.

//         // Skip over any reserved addresses.
//         int ret;
//         uint8_t rxdata;
//         if (reserved_addr(addr))
//             ret = PICO_ERROR_GENERIC;
//         else
//             ret = i2c_read_blocking(i2c_default, addr, &rxdata, 1, false);

//         uart_puts(uart0, ret < 0 ? "." : "@");
//         uart_puts(uart0, addr % 16 == 15 ? "\n" : "  ");
//     }
//     uart_puts(uart0, "Done.\n");
// }

void TaskTriggerTemp(PTASKPARM p) {
    int                 error = 0;
    uint8_t             configData[2];
    uint8_t             deviceIDValue[2];
    uint16_t            deviceID;

    uart_puts(uart0, "TRd:");

    // error = i2cReadRegister(i2c0, TMP117_ADDRESS, TMP117_REG_DEVICE_ID, deviceIDValue, 2);

    // if (error == PICO_ERROR_GENERIC) {
    //     uart_puts(uart0, "ERR_GEN\n");
    // }
    // else if (error == PICO_ERROR_TIMEOUT) {
    //     uart_puts(uart0, "ERR_TM\n");
    // }
    // else {
    //     deviceID = ((uint16_t)deviceIDValue[0]) << 8 | (uint16_t)deviceIDValue[1];
    //     sprintf(szTemp, "OK!: %04X\n", deviceID);
    //     uart_puts(uart0, szTemp);
    // }

    /*
    ** One-shot conversion, 8 sample average...
    */
    configData[0] = 0x0C;
    configData[1] = 0x20;
    error = i2cWriteRegister(i2c0, TMP117_ADDRESS, TMP117_REG_CONFIG, configData, 2);

    if (error == PICO_ERROR_GENERIC) {
        uart_puts(uart0, "ERR_GEN\n");
    }
    else if (error == PICO_ERROR_TIMEOUT) {
        uart_puts(uart0, "ERR_TM\n");
    }
    else {
        uart_puts(uart0, "OK!\n");
    }

    scheduleTaskOnce(TASK_TEMP_READ, rtc_val_ms(150), NULL);
}

void TaskReadTemp(PTASKPARM p) {
    uint8_t             tempRegister[2];
    int16_t             tempInt;
    double              temp;
    uint32_t            startTime;

    startTime = timer_hw->timerawl;

    i2cReadRegister(i2c0, TMP117_ADDRESS, TMP117_REG_TEMP, tempRegister, 2);

    tempInt = (int16_t)((((int16_t)tempRegister[0]) << 8) | (int16_t)tempRegister[1]);
    temp = (double)tempInt * 0.0078125;

    sprintf(szTemp, "Got temp in %u us: %.2f\n", (timer_hw->timerawl - startTime), temp);
    uart_puts(uart0, szTemp);

    scheduleTaskOnce(TASK_TEMP_TRIGGER, rtc_val_sec(5), NULL);
}
