#include <stdint.h>
#include <stdbool.h>

#ifndef __INCL_I2C_RP2040
#define __INCL_I2C_RP2040

#define I2C_SDA_HOLD                38

int _i2c_read_blocking_debug(
            i2c_inst_t * i2c, 
            uint8_t addr, 
            uint8_t * dst, 
            size_t len, 
            bool nostop);
int _i2c_write_blocking_debug(
            i2c_inst_t * i2c, 
            uint8_t addr, 
            const uint8_t * src, 
            size_t len, 
            bool nostop);
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
