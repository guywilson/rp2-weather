#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "scheduler.h"
#include "taskdef.h"
#include "i2c_rp2040.h"

static uint8_t          txBuffer[32];
static int              txIndex = 0;
static int              txBufferLen = 0;

static uint8_t          rxBuffer[32];
static int              rxIndex = 0;

I2C_RX                  rxStruct;

void irqI2CHandler() {
    if (i2c0->hw->raw_intr_stat & I2C_IC_INTR_STAT_R_TX_EMPTY_BITS) {
        /*
        ** TX complete, send the next byte...
        */
        i2c0->hw->data_cmd = txBuffer[txIndex++];

        if (txIndex == txBufferLen) {
            /*
            ** Mask the interrupt...
            */
            i2c0->hw->intr_mask &= !I2C_IC_INTR_MASK_M_TX_EMPTY_BITS;
        }
    }
    else if (i2c0->hw->raw_intr_stat & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        if (i2c0->hw->rxflr) {
            rxBuffer[rxIndex++] = i2c0->hw->data_cmd;
        }
        else {
            /*
            ** Mask the interrupt...
            */
            i2c0->hw->intr_mask &= !I2C_IC_INTR_MASK_M_RX_FULL_BITS;

            rxStruct.buffer = rxBuffer;
            rxStruct.bufferLen = rxIndex;

            scheduleTaskOnce(TASK_TEMP_RESULT, RUN_NOW, &rxStruct);
        }
    }
}

void setupI2C(i2c_baud baud) {
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

    irq_set_exclusive_handler(I2C0_IRQ, irqI2CHandler);

    /*
    ** Unmask the TX_EMPTY & RX_FULL interrupts...
    */
    i2c0->hw->intr_mask |= 
        (I2C_IC_INTR_MASK_M_TX_EMPTY_BITS | 
        I2C_IC_INTR_MASK_M_RX_FULL_BITS);
}

void i2cWrite(uint8_t addr, uint8_t * data, int length) {
    i2c0->hw->enable = 0;
    i2c0->hw->tar = addr;
    i2c0->hw->enable = 1;

    memcpy(txBuffer, data, length);

    txIndex = 0;
    txBufferLen = length;
    i2c0->hw->data_cmd = txBuffer[txIndex++];

    /*
    ** Unmask the TX_EMPTY interrupt...
    */
    i2c0->hw->intr_mask |= I2C_IC_INTR_MASK_M_TX_EMPTY_BITS;
}

void i2cRead(uint8_t addr, uint8_t * data) {
    i2c0->hw->enable = 0;
    i2c0->hw->tar = addr;
    i2c0->hw->enable = 1;

    /*
    ** Let the I2C know we're reading...
    */
    i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS;

    /*
    ** Unmask the RX_FULL interrupt...
    */
    i2c0->hw->intr_mask |= I2C_IC_INTR_MASK_M_RX_FULL_BITS;
}
