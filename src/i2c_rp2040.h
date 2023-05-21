#include <stdint.h>
#include <stdbool.h>

#include "scheduler.h"

#ifndef __INCL_I2C_RP2040
#define __INCL_I2C_RP2040

#define I2C_SDA_HOLD                38

void i2cPowerUp(void);
void i2cPowerDown(void);
int i2cWriteRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length);
int i2cReadRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length);

#endif
