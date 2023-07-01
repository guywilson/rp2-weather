#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "hardware/i2c.h"
#include "scheduler.h"
#include "taskdef.h"
#include "i2c_rp2040.h"
#include "rtc_rp2040.h"
#include "SHT4X.h"

int sht4x_setup(i2c_inst_t * i2c) {
    int         error;
    uint8_t     reg;

    reg = SHT4X_CMD_SOFT_RESET;

    error = i2cWriteTimeoutProtected(i2c0, SHT4X_ADDRESS, &reg, 1, false);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }
    else {
        return error;
    }
}
