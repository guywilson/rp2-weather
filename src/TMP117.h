#include "scheduler.h"
#include "hardware/i2c.h"

#ifndef __INCL_I2CTASK
#define __INCL_I2CTASK

#define TMP117_ADDRESS                  0x48

#define TMP117_REG_CONFIG               0x01
#define TMP117_REG_TEMP                 0x00
#define TMP117_REG_TEMP_OFFSET          0x07
#define TMP117_REG_DEVICE_ID            0x0F


int tmp117_setup(i2c_inst_t * i2c);
void tmp117_taskReadTemp(PTASKPARM p);

#endif
