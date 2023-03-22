#include <stdio.h>
#include <stdlib.h>

#include "scheduler.h"
#include "taskdef.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"

static char                szTemp[16];

bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void TaskI2CDump(PTASKPARM p) {
    uart_puts(uart0, "I2C Bus Scan\n");
    uart_puts(uart0, "   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            sprintf(szTemp, "%02x ", addr);
            uart_puts(uart0, szTemp);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_blocking(i2c_default, addr, &rxdata, 1, false);

        uart_puts(uart0, ret < 0 ? "." : "@");
        uart_puts(uart0, addr % 16 == 15 ? "\n" : "  ");
    }
    uart_puts(uart0, "Done.\n");
}

void TaskDebug(PTASKPARM p) {
    uart_puts(uart0, "Hello World! - Running on core: ");
    uart_putc(uart0, getCoreID() + 0x30);
    uart_putc(uart0, '\n');
}
