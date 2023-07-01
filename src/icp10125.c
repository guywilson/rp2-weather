#include <stdint.h>

#include "hardware/i2c.h"
#include "i2c_rp2040.h"
#include "utils.h"
#include "logger.h"
#include "icp10125.h"

uint16_t            otpValues[4];

int icp10125_setup(i2c_inst_t * i2c) {
    int                 error;
    int                 i;
    uint8_t             buffer[8];
    uint16_t            chipID;

    /*
    ** Issue soft reset...
    */
    buffer[0] = 0x80;
    buffer[1] = 0x5D;

    error = i2cWriteTimeoutProtected(i2c, ICP10125_ADDRESS, buffer, 2, false);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }

    sleep_ms(10U);

    /*
    ** Read chip ID...
    */
    buffer[0] = 0xEF;
    buffer[1] = 0xC8;

    i2cWriteTimeoutProtected(i2c, ICP10125_ADDRESS, buffer, 2, true);
    error = i2cReadTimeoutProtected(i2c, ICP10125_ADDRESS, buffer, 3, false);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }
    
    chipID = copyI2CReg_uint16(buffer);

    if ((chipID & 0x3F) != 0x08) {
        lgLogError("Read incorrect icp10125 chip ID: 0x%02X", (chipID & 0x3F));
        return -1;
    }

    buffer[0] = 0xC5;
    buffer[1] = 0x95;
    buffer[2] = 0x00;
    buffer[3] = 0x66;
    buffer[4] = 0x9C;

    error = i2cWriteTimeoutProtected(i2c0, ICP10125_ADDRESS, buffer, 5, false);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }
    
    for (i = 0; i < 4; i++) {
        buffer[0] = 0xC7;
        buffer[1] = 0xF7;

        i2cWriteTimeoutProtected(i2c0, ICP10125_ADDRESS, buffer, 2, false);
        i2cReadTimeoutProtected(i2c0, ICP10125_ADDRESS, buffer, 3, false);

        otpValues[i] = (uint16_t)((uint16_t)buffer[0] << 8 | (uint16_t)buffer[1]);
    }

    return 0;
}

uint16_t * getOTPValues() {
    return &otpValues[0];
}
