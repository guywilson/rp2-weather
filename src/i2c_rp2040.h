#include <stdint.h>
#include <stdbool.h>

#include "hardware/i2c.h"
#include "scheduler.h"

#ifndef __INCL_I2C_RP2040
#define __INCL_I2C_RP2040

#define I2C_SDA_HOLD                38

typedef struct {
    uint                    address;

    rtc_t                   lastStateTime;
    bool                    isActive;

    int (* setup)(i2c_inst_t *);
}
i2c_device_t;

bool    i2cGetDeviceState(i2c_inst_t * i2c, uint address);
int     i2c_bus_open(i2c_inst_t * i2c, int numDevices);
int     i2c_bus_close(i2c_inst_t * i2c);
int     i2c_bus_register_device(
            i2c_inst_t * i2c, 
            uint address, 
            int (* setup)(i2c_inst_t *));
int     i2c_bus_setup(i2c_inst_t * i2c);
int     i2cReadTimeoutProtected(
            i2c_inst_t * i2c, 
            const uint address, 
            uint8_t * dst, 
            size_t len, 
            bool nostop);
int     i2cWriteTimeoutProtected(
            i2c_inst_t * i2c, 
            const uint address, 
            uint8_t * src, 
            size_t len, 
            bool nostop);
int     i2cWriteRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length);
int     i2cReadRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length);

#endif
