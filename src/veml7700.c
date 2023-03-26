#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "hardware/i2c.h"
#include "i2c_rp2040.h"
#include "veml7700.h"

int veml7700_setup(i2c_inst_t * i2c) {
    uint8_t         buffer[2];
    uint16_t        als_conf;
    uint16_t        als_psm;

    als_conf = VEML7700_ALS_CONF_ALS_SD_ON | VEML7700_ALS_CONF_ALS_IT_400 | VEML7700_ALS_CONF_GAIN_TWO;

    buffer[0] = als_conf & 0x00FF;
    buffer[1] = (als_conf >> 8) & 0x00FF;

    i2cWriteRegister(i2c, VEML7700_ADDRESS, VEML7700_REG_CMD_ALS_CONF, buffer, 2);

    als_psm = VEML7700_ALS_POWER_SAVING_PSM_4 | VEML7700_ALS_POWER_SAVING_PSM_EN;

    buffer[0] = als_psm & 0x00FF;

    i2cWriteRegister(i2c, VEML7700_ADDRESS, VEML7700_REG_CMD_POWER_SAVING, buffer, 1);

    return 0;
}
