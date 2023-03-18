#include <stdint.h>

#include "hardware/i2c.h"

#ifndef __INCL_ICP10125
#define __INCL_ICP10125

#define ICP10215_ADDRESS                    0x63

#define ICP10125_CMD_MEASURE_LOW_NOISE      0x70DF

int         icp10125_setup(i2c_inst_t * i2c);
uint32_t    icp10125_get_pressure(uint16_t rawTemperature, uint32_t rawPressure);

#endif
