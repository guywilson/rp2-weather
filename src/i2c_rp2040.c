#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/resets.h"
#include "logger.h"
#include "scheduler.h"
#include "taskdef.h"
#include "rtc_rp2040.h"
#include "i2c_rp2040.h"
#include "gpio_def.h"

void i2cPowerUp(void) {
    gpio_put(I2C0_POWER_PIN, 1);
}

void i2cPowerDown(void) {
    gpio_put(I2C0_POWER_PIN, 0);
}

int i2cWriteRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length)
{
    int             bytesWritten = 0;
    uint8_t         msg[length + 1];

    msg[0] = reg;
    memcpy(&msg[1], data, length);

    bytesWritten = i2c_write_blocking(i2c, addr, msg, (length + 1), false);

    return bytesWritten;
}

int i2cReadRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length)
{
   int      bytesRead = 0;

    // Read data from register(s) over I2C
    i2c_write_blocking(i2c, addr, &reg, 1, true);
    bytesRead = i2c_read_blocking(i2c, addr, data, length, false);

    return bytesRead;
}
