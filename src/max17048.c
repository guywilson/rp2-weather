#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware/i2c.h"
#include "i2c_rp2040.h"
#include "rtc_rp2040.h"
#include "max17048.h"
#include "utils.h"
#include "logger.h"

int max17048_setup(i2c_inst_t * i2c) {
    uint8_t         data[8];
    uint8_t         configReg[2];

    data[0] = MAX17048_REG_MODE;
    data[1] = 0x00;
    data[2] = 0x00;

    i2cWriteTimeoutProtected(i2c, MAX17048_ADDRESS, data, 3, false);

    i2cReadRegister(i2c, MAX17048_ADDRESS, MAX17048_REG_CONFIG, configReg, 2);

    configReg[1] = (configReg[1] & 0x7F) | 0x80;

    data[0] = MAX17048_REG_CONFIG;
    data[1] = configReg[0];
    data[2] = configReg[1];

    i2cWriteTimeoutProtected(i2c, MAX17048_ADDRESS, data, 3, false);

    // /*
    // ** Always use hibernate mode to save power...
    // */
    // data[0] = MAX17048_REG_HIBRT;
    // data[1] = 0xFF;
    // data[2] = 0xFF;

    // i2cWriteTimeoutProtected(i2c, MAX17048_ADDRESS, data, 3, false);

    return 0;
}
