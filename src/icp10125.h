#include <stdint.h>

#include "hardware/i2c.h"
#include "i2c_addr.h"

#ifndef __INCL_ICP10125
#define __INCL_ICP10125

#define ICP10125_CMD_MEASURE_LOW_NOISE      0x70DF

int     icp10125_setup(i2c_inst_t * i2c);
int     icp10125_read_otp(void);
void    icp10125_process_data(
                const int p_LSB, 
                const int T_LSB, 
                int *pressure);
#endif
