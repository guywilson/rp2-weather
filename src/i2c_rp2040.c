#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "scheduler.h"
#include "taskdef.h"
#include "i2c_rp2040.h"

#define IRQ_REASON_I2C0_TX_EMPTY            0x01
#define IRQ_REASON_I2C0_RX_FULL             0x02
#define IRQ_REASON_I2C1_TX_EMPTY            0x04
#define IRQ_REASON_I2C1_RX_FULL             0x08

I2C_device              i2cDevices[2];
I2C_RX                  rxStruct;

void irqI2C0Handler() {
    if (i2c0->hw->raw_intr_stat & I2C_IC_INTR_STAT_R_TX_EMPTY_BITS) {
        /*
        ** TX complete, send the next byte...
        */
        i2c0->hw->data_cmd = i2cDevices[0].txBuffer[i2cDevices[0].txIndex++];

        if (i2cDevices[0].txIndex == i2cDevices[0].txBufferLen) {
            /*
            ** Mask the interrupt...
            */
            i2c0->hw->intr_mask &= !I2C_IC_INTR_MASK_M_TX_EMPTY_BITS;
        }
    }
    else if (i2c0->hw->raw_intr_stat & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        if (i2c0->hw->rxflr) {
            i2cDevices[0].rxBuffer[i2cDevices[0].rxIndex++] = i2c0->hw->data_cmd;
        }
        else {
            /*
            ** Mask the interrupt...
            */
            i2c0->hw->intr_mask &= !I2C_IC_INTR_MASK_M_RX_FULL_BITS;

            rxStruct.buffer = i2cDevices[0].rxBuffer;
            rxStruct.bufferLen = i2cDevices[0].rxIndex;

            scheduleTaskOnce(TASK_TEMP_RESULT, RUN_NOW, &rxStruct);
        }
    }
}

void irqI2C1Handler() {
    if (i2c1->hw->raw_intr_stat & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        /*
        ** TX complete, send the next byte...
        */
        i2c1->hw->data_cmd = i2cDevices[1].txBuffer[i2cDevices[1].txIndex++];

        if (i2cDevices[1].txIndex == i2cDevices[1].txBufferLen) {
            /*
            ** Mask the interrupt...
            */
            i2c1->hw->intr_mask &= !I2C_IC_INTR_MASK_M_TX_EMPTY_BITS;
        }
    }
    else if (i2c1->hw->raw_intr_stat & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        if (i2c1->hw->rxflr) {
            i2cDevices[1].rxBuffer[i2cDevices[1].rxIndex++] = i2c1->hw->data_cmd;
        }
        else {
            /*
            ** Mask the interrupt...
            */
            i2c1->hw->intr_mask &= !I2C_IC_INTR_MASK_M_RX_FULL_BITS;

            rxStruct.buffer = i2cDevices[1].rxBuffer;
            rxStruct.bufferLen = i2cDevices[1].rxIndex;

            scheduleTaskOnce(TASK_TEMP_RESULT, RUN_NOW, &rxStruct);
        }
    }
}

int i2cWriteRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length)
{
    int             bytesWritten = 0;
    uint8_t         msg[length + 1];

    msg[0] = reg;
    memcpy(&msg[1], data, length);

    bytesWritten = i2c_write_blocking(i2c, addr, msg, (length + 1), false);

    return bytesWritten;
}

int i2cReadRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length)
{
   int      bytesRead = 0;

    // Read data from register(s) over I2C
    i2c_write_blocking(i2c, addr, &reg, 1, true);
    bytesRead = i2c_read_blocking(i2c, addr, data, length, false);

    return bytesRead;
}

void setupI2C(i2c_inst_t * i2c, i2c_baud baud) {
    uint16_t        lcnt = (uint16_t)(baud & 0x0000FFFF);
    uint16_t        hcnt = (uint16_t)((baud >> 16) & 0x0000FFFF);

    i2c->hw->enable = 0;

    i2c->hw->con =
            I2C_IC_CON_SPEED_VALUE_FAST << I2C_IC_CON_SPEED_LSB |
            I2C_IC_CON_MASTER_MODE_BITS |
            I2C_IC_CON_IC_SLAVE_DISABLE_BITS |
            I2C_IC_CON_IC_RESTART_EN_BITS |
            I2C_IC_CON_TX_EMPTY_CTRL_BITS;

    hw_write_masked(&i2c->hw->con,
                   I2C_IC_CON_SPEED_VALUE_FAST << I2C_IC_CON_SPEED_LSB,
                   I2C_IC_CON_SPEED_BITS);

    i2c->hw->tx_tl = 0;
    i2c->hw->rx_tl = 0;

    i2c->hw->fs_scl_hcnt = hcnt;
    i2c->hw->fs_scl_lcnt = lcnt;
    i2c->hw->fs_spklen = lcnt < 16 ? 1 : lcnt / 16;

    hw_write_masked(&i2c->hw->sda_hold,
                    I2C_SDA_HOLD << I2C_IC_SDA_HOLD_IC_SDA_TX_HOLD_LSB,
                    I2C_IC_SDA_HOLD_IC_SDA_TX_HOLD_BITS);

    // Always enable the DREQ signalling -- harmless if DMA isn't listening
    i2c->hw->dma_cr = I2C_IC_DMA_CR_TDMAE_BITS | I2C_IC_DMA_CR_RDMAE_BITS;

    i2c->hw->enable = 1;

    irq_set_exclusive_handler(I2C0_IRQ, irqI2C0Handler);
    irq_set_exclusive_handler(I2C1_IRQ, irqI2C1Handler);

    /*
    ** Unmask the TX_EMPTY & RX_FULL interrupts...
    */
    i2c->hw->intr_mask |= 
        (I2C_IC_INTR_MASK_M_TX_EMPTY_BITS | 
        I2C_IC_INTR_MASK_M_RX_FULL_BITS);
}

void i2cWrite(i2c_inst_t * i2c, uint8_t addr, uint8_t * data, int length) {
    i2c->hw->enable = 0;
    i2c->hw->tar = addr;
    i2c->hw->enable = 1;

    memcpy(i2cDevices[0].txBuffer, data, length);

    i2cDevices[0].txIndex = 0;
    i2cDevices[0].txBufferLen = length;
    i2c->hw->data_cmd = i2cDevices[0].txBuffer[i2cDevices[0].txIndex++];

    /*
    ** Unmask the TX_EMPTY interrupt...
    */
    i2c->hw->intr_mask |= I2C_IC_INTR_MASK_M_TX_EMPTY_BITS;
}

void i2cWriteByte(i2c_inst_t * i2c, uint8_t addr, uint8_t data) {
    i2c->hw->enable = 0;
    i2c->hw->tar = addr;
    i2c->hw->enable = 1;

    i2c->hw->data_cmd = data;
}

void i2cRead(i2c_inst_t * i2c, uint8_t addr) {
    i2c->hw->enable = 0;
    i2c->hw->tar = addr;
    i2c->hw->enable = 1;

    /*
    ** Let the I2C know we're reading...
    */
    i2c->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS;

    /*
    ** Unmask the RX_FULL interrupt...
    */
    i2c->hw->intr_mask |= I2C_IC_INTR_MASK_M_RX_FULL_BITS;
}
