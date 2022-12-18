#include <stdlib.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "i2c_rp2040.h"

void initI2C(i2c_baud baud) {
    uint16_t        lcnt = (uint16_t)(baud & 0x0000FFFF);
    uint16_t        hcnt = (uint16_t)((baud >> 16) & 0x0000FFFF);

    i2c0->hw->enable = 0;

    i2c0->hw->con =
            I2C_IC_CON_SPEED_VALUE_FAST << I2C_IC_CON_SPEED_LSB |
            I2C_IC_CON_MASTER_MODE_BITS |
            I2C_IC_CON_IC_SLAVE_DISABLE_BITS |
            I2C_IC_CON_IC_RESTART_EN_BITS |
            I2C_IC_CON_TX_EMPTY_CTRL_BITS;

    hw_write_masked(&i2c0->hw->con,
                   I2C_IC_CON_SPEED_VALUE_FAST << I2C_IC_CON_SPEED_LSB,
                   I2C_IC_CON_SPEED_BITS);

    i2c0->hw->tx_tl = 0;
    i2c0->hw->rx_tl = 0;

    i2c0->hw->fs_scl_hcnt = hcnt;
    i2c0->hw->fs_scl_lcnt = lcnt;
    i2c0->hw->fs_spklen = lcnt < 16 ? 1 : lcnt / 16;

    hw_write_masked(&i2c0->hw->sda_hold,
                    I2C_SDA_HOLD << I2C_IC_SDA_HOLD_IC_SDA_TX_HOLD_LSB,
                    I2C_IC_SDA_HOLD_IC_SDA_TX_HOLD_BITS);

    i2c0->hw->enable = 1;
}
