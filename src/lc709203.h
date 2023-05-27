#include "i2c_addr.h"

#ifndef __INCL_LC709203
#define __INCL_LC709203

#define LC709203_CMD_BEFORE_RSOC            0x04
#define LC709203_CMD_THERMISTOR_B           0x06
#define LC709203_CMD_INITIAL_RSOC           0x07
#define LC709203_CMD_CELL_TEMERATURE        0x08
#define LC709203_CMD_CELL_VOLTAGE           0x09
#define LC709203_CMD_CURRENT_DIRECTION      0x0A
#define LC709203_CMD_APA                    0x0B
#define LC709203_CMD_APT                    0x0C
#define LC709203_CMD_RSOC                   0x0D
#define LC709203_CMD_ITE                    0x0F
#define LC709203_CMD_IC_VERSION             0x11
#define LC709203_CMD_COTP                   0x12
#define LC709203_CMD_ALARM_LOW_RSOC         0x13
#define LC709203_CMD_ALARM_LOW_CELL_VOLTAGE 0x14
#define LC709203_CMD_IC_POWER_MODE          0x15
#define LC709203_CMD_STATUS_BIT             0x16
#define LC709203_CMD_NOTP                   0x1A

#define THERMISTOR_B_VALUE                  3977

int     lc709203_write_register(i2c_inst_t * i2c, uint8_t reg, uint16_t data);
int     lc709203_read_register(i2c_inst_t * i2c, uint8_t reg, uint16_t * data);
int     lc709203_setup(i2c_inst_t * i2c);

#endif
