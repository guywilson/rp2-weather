#include <stdint.h>
#include <stdbool.h>

#include "scheduler.h"

#ifndef __INCL_I2C_RP2040
#define __INCL_I2C_RP2040

#define I2C_SDA_HOLD                38

void taskI2CRead(PTASKPARM p);

uint32_t i2cInit(i2c_inst_t *i2c, uint32_t baudrate);

void i2cTriggerReadRegister(
                i2c_inst_t * i2c, 
                uint16_t callbackTask, 
                rtc_t writeReadDelay, 
                uint8_t addr, 
                uint8_t * src,
                size_t srcLen,
                uint8_t * dst, 
                size_t dstLen, 
                bool nostopWrite,
                bool nostopRead);
void i2cTriggerRead(
            i2c_inst_t * i2c, 
            uint16_t callbackTask, 
            uint8_t addr, 
            uint8_t * dst, 
            size_t len, 
            bool nostop);
void i2cTriggerWrite(
            i2c_inst_t * i2c, 
            uint16_t callbackTask, 
            uint8_t addr, 
            uint8_t * src, 
            size_t len, 
            bool nostop);
int i2cReadBlocking(
            i2c_inst_t * i2c, 
            uint8_t addr, 
            uint8_t * dst, 
            size_t len, 
            bool nostop);
int i2cWriteBlocking(
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
