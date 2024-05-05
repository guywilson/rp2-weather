#include "i2c_addr.h"

#ifndef __INCL_MAX17048
#define __INCL_MAX17048

#define MAX17048_REG_VCELL          0x02
#define MAX17048_REG_SOC            0x04
#define MAX17048_REG_MODE           0x06
#define MAX17048_REG_VERSION        0x08
#define MAX17048_REG_HIBRT          0x0A
#define MAX17048_REG_CONFIG         0x0C
#define MAX17048_REG_VALERT         0x14
#define MAX17048_REG_CRATE          0x16
#define MAX17048_REG_VRESET         0x18
#define MAX17048_REG_CHIPID         0x19
#define MAX17048_REG_STATUS         0x1A
#define MAX17048_REG_CMD            0xFE

#define MAX17048_CONFIG_SLEEP       0x0080

int max17048_setup(i2c_inst_t * i2c);

#endif
