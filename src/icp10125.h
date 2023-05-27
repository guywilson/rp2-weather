#include <stdint.h>

#include "hardware/i2c.h"
#include "i2c_addr.h"

#ifndef __INCL_ICP10125
#define __INCL_ICP10125

#define ICP10125_CMD_MEASURE_LOW_NOISE      0x70DF

int         icp10125_setup(i2c_inst_t * i2c);
uint16_t *  getOTPValues();

#endif
