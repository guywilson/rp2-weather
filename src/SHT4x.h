#include "i2c_addr.h"

#ifndef __INCL_SHT4X
#define __INCL_SHT4X

#define SHT4X_CMD_MEASURE_HI_PRN                    0xFD
#define SHT4X_CMD_MEASURE_MD_PRN                    0xF6
#define SHT4X_CMD_MEASURE_LO_PRN                    0xE0
#define SHT4X_CMD_MEASURE_HI_HT_1S_HI_PRN           0x39
#define SHT4X_CMD_MEASURE_HI_HT_100MS_HI_PRN        0x32
#define SHT4X_CMD_MEASURE_MD_HT_1S_HI_PRN           0x2F
#define SHT4X_CMD_MEASURE_MD_HT_100MS_HI_PRN        0x24
#define SHT4X_CMD_MEASURE_LO_HT_1S_HI_PRN           0x1E
#define SHT4X_CMD_MEASURE_LO_HT_100MS_HI_PRN        0x15

#define SHT4X_CMD_READ_SERIAL_NO                    0x89
#define SHT4X_CMD_SOFT_RESET                        0x94

int         sht4x_setup(i2c_inst_t * i2c);

#endif
