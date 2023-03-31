#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/resets.h"
#include "logger.h"
#include "scheduler.h"
#include "taskdef.h"
#include "rtc_rp2040.h"
#include "i2c_rp2040.h"

#define I2C_OK                                  0x0000
#define I2C_ERROR_DEVICE_NOT_CONNECTED          0x0100
#define I2C_ERROR_GENERIC                       0x0200

#define I2C_READ_STATE_BEGIN                    0x0010
#define I2C_READ_STATE_WRITE_CMD                0x0020
#define I2C_READ_STATE_READ_BYTE                0x0040
#define I2C_READ_STATE_CALLBACK                 0x0080

#define I2C_WRITE_STATE_BEGIN                   0x0100
#define I2C_WRITE_STATE_TX_BYTE                 0x0200
#define I2C_WRITE_STATE_TX_WAIT                 0x0201
#define I2C_WRITE_STATE_TX_CHECK                0x0202
#define I2C_WRITE_STATE_TX_FINALISE             0x0400
#define I2C_WRITE_STATE_CALLBACK                0x0800

typedef struct {
    i2c_inst_t *        i2c;

    uint16_t            txCallbackTask;
    uint16_t            rxCallbackTask;

    uint16_t            errorCode;
    uint16_t            count_TX_EMPTY;
    uint16_t            count_RX_FULL;
    uint16_t            count_TX_ABRT;
    uint16_t            count_STOP_DET;

    int                 ix;
    int                 txLen;
    int                 rxLen;
    uint8_t *           buffer;

    uint8_t             addr;
    bool                txNoStop;
    bool                rxNoStop;

    rtc_t               readDelay;
}
i2c_read_write_t;

static i2c_read_write_t         i2cReadWrite;

int getTxEmptyIntCount() {
    return (int)i2cReadWrite.count_TX_EMPTY;
}

int getTxAbrtIntCount() {
    return (int)i2cReadWrite.count_TX_ABRT;
}

int getRxFullIntCount() {
    return (int)i2cReadWrite.count_RX_FULL;
}

int getStopDetIntCount() {
    return (int)i2cReadWrite.count_STOP_DET;
}

void irqI2CTxAbortHandler(i2c_read_write_t * i2c_rw) {
    uint32_t            abrtReason;

    abrtReason = i2c_rw->i2c->hw->tx_abrt_source;

    if (abrtReason & I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS) {
        i2c_rw->errorCode = I2C_ERROR_DEVICE_NOT_CONNECTED;
    }
    else if (abrtReason & I2C_IC_TX_ABRT_SOURCE_BITS) {
        i2c_rw->errorCode = I2C_ERROR_GENERIC;
    }
    else {
        i2c_rw->errorCode = I2C_OK;
    }
}

void irqI2CTxEmptyHandler(i2c_read_write_t * i2c_rw) {
    bool            isFirst = false;
    bool            isLast = false;

    isFirst = (i2c_rw->ix == 0);
    isLast = (i2c_rw->ix == i2c_rw->txLen - 1);

    if (i2c_rw->ix < i2c_rw->txLen) {
        i2c_rw->i2c->hw->data_cmd =
                bool_to_bit(isFirst && i2c_rw->i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
                bool_to_bit(isLast && !i2c_rw->txNoStop) << I2C_IC_DATA_CMD_STOP_LSB |
                i2c_rw->buffer[i2c_rw->ix++];
    }
    else {
        i2c_rw->i2c->restart_on_next = i2c_rw->txNoStop;
        i2c_rw->i2c->hw->intr_mask &= ~(I2C_IC_INTR_MASK_M_TX_EMPTY_BITS & 0x00001FFF);

        scheduleTaskOnce(i2c_rw->txCallbackTask, i2c_rw->readDelay, NULL);
    }
}

void irqI2CStopDetectHandler(i2c_read_write_t * i2c_rw) {
    i2c_rw->i2c->hw->clr_stop_det;
}

void irqI2CRxFullHandler(i2c_read_write_t * i2c_rw) {
    bool            isLast = false;
    bool            isFirst = false;

    isFirst = (i2c_rw->ix == 0);
    isLast = (i2c_rw->ix == i2c_rw->rxLen - 1);

    i2c_rw->buffer[i2c_rw->ix++] = (uint8_t)(i2c_rw->i2c->hw->data_cmd & 0x000000FF);

    if (!isLast) {
        i2c_rw->i2c->hw->data_cmd =
                bool_to_bit(isFirst && i2c_rw->i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
                bool_to_bit(isLast && !i2c_rw->rxNoStop) << I2C_IC_DATA_CMD_STOP_LSB |
                I2C_IC_DATA_CMD_CMD_BITS; // -> 1 for read
    }
    else {
        i2c_rw->i2c->restart_on_next = i2c_rw->rxNoStop;
        i2c_rw->i2c->hw->intr_mask &= ~(I2C_IC_INTR_MASK_M_RX_FULL_BITS & 0x00001FFF);

        scheduleTaskOnce(i2c_rw->rxCallbackTask, rtc_val_ms(2), NULL);
    }
}

void irqI2CHandler() {
    uint32_t            intrStat;

    intrStat = i2cReadWrite.i2c->hw->raw_intr_stat;

    switch (intrStat & I2C_IC_RAW_INTR_STAT_BITS) {
        case I2C_IC_RAW_INTR_STAT_STOP_DET_BITS:
            irqI2CStopDetectHandler(&i2cReadWrite);
            i2cReadWrite.count_STOP_DET++;
            break;

        case I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS:
            irqI2CTxEmptyHandler(&i2cReadWrite);
            i2cReadWrite.count_TX_EMPTY++;
            break;

        case I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS:
            irqI2CTxAbortHandler(&i2cReadWrite);
            i2cReadWrite.count_TX_ABRT++;
            break;

        case I2C_IC_RAW_INTR_STAT_RX_FULL_BITS:
            irqI2CRxFullHandler(&i2cReadWrite);
            i2cReadWrite.count_RX_FULL++;
            break;
    }
}

void taskI2CRead(PTASKPARM p) {
    bool                    isFirst = false;
    bool                    isLast = false;
    i2c_read_write_t *      i2c_rw;

    i2c_rw = (i2c_read_write_t *)p;

    if (!i2c_get_write_available(i2c_rw->i2c)) {
        scheduleTaskOnce(TASK_I2C_READ, rtc_val_ms(1), p);
    }

    isFirst = (i2c_rw->ix == 0);
    isLast = (i2c_rw->ix == (i2c_rw->rxLen - 1));

    i2c_rw->i2c->hw->data_cmd =
            bool_to_bit(isFirst && i2c_rw->i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
            bool_to_bit(isLast && !i2c_rw->rxNoStop) << I2C_IC_DATA_CMD_STOP_LSB |
            I2C_IC_DATA_CMD_CMD_BITS; // -> 1 for read
}

static uint32_t i2cSetBaudrate(i2c_inst_t *i2c, uint32_t baudrate) {
    invalid_params_if(I2C, baudrate == 0);
    // I2C is synchronous design that runs from clk_sys
    uint freq_in = clock_get_hz(clk_sys);

    // TODO there are some subtleties to I2C timing which we are completely ignoring here
    uint period = (freq_in + baudrate / 2) / baudrate;
    uint lcnt = period * 3 / 5; // oof this one hurts
    uint hcnt = period - lcnt;
    // Check for out-of-range divisors:
    invalid_params_if(I2C, hcnt > I2C_IC_FS_SCL_HCNT_IC_FS_SCL_HCNT_BITS);
    invalid_params_if(I2C, lcnt > I2C_IC_FS_SCL_LCNT_IC_FS_SCL_LCNT_BITS);
    invalid_params_if(I2C, hcnt < 8);
    invalid_params_if(I2C, lcnt < 8);

    // Per I2C-bus specification a device in standard or fast mode must
    // internally provide a hold time of at least 300ns for the SDA signal to
    // bridge the undefined region of the falling edge of SCL. A smaller hold
    // time of 120ns is used for fast mode plus.
    uint sda_tx_hold_count;
    if (baudrate < 1000000) {
        // sda_tx_hold_count = freq_in [cycles/s] * 300ns * (1s / 1e9ns)
        // Reduce 300/1e9 to 3/1e7 to avoid numbers that don't fit in uint.
        // Add 1 to avoid division truncation.
        sda_tx_hold_count = ((freq_in * 3) / 10000000) + 1;
    } else {
        // sda_tx_hold_count = freq_in [cycles/s] * 120ns * (1s / 1e9ns)
        // Reduce 120/1e9 to 3/25e6 to avoid numbers that don't fit in uint.
        // Add 1 to avoid division truncation.
        sda_tx_hold_count = ((freq_in * 3) / 25000000) + 1;
    }
    assert(sda_tx_hold_count <= lcnt - 2);

    i2c->hw->enable = 0;
    // Always use "fast" mode (<= 400 kHz, works fine for standard mode too)
    hw_write_masked(&i2c->hw->con,
                   I2C_IC_CON_SPEED_VALUE_FAST << I2C_IC_CON_SPEED_LSB,
                   I2C_IC_CON_SPEED_BITS
    );
    i2c->hw->fs_scl_hcnt = hcnt;
    i2c->hw->fs_scl_lcnt = lcnt;
    i2c->hw->fs_spklen = lcnt < 16 ? 1 : lcnt / 16;
    hw_write_masked(&i2c->hw->sda_hold,
                    sda_tx_hold_count << I2C_IC_SDA_HOLD_IC_SDA_TX_HOLD_LSB,
                    I2C_IC_SDA_HOLD_IC_SDA_TX_HOLD_BITS);

    i2c->hw->enable = 1;
    return freq_in / period;
}

uint32_t i2cInit(i2c_inst_t *i2c, uint32_t baudrate) {
    reset_block(i2c == i2c0 ? RESETS_RESET_I2C0_BITS : RESETS_RESET_I2C1_BITS);
    unreset_block_wait(i2c == i2c0 ? RESETS_RESET_I2C0_BITS : RESETS_RESET_I2C1_BITS);

    i2c->restart_on_next = false;
    i2c->hw->enable = 0;

    // Configure as a fast-mode master with RepStart support, 7-bit addresses
    i2c->hw->con =
            I2C_IC_CON_SPEED_VALUE_FAST << I2C_IC_CON_SPEED_LSB |
            I2C_IC_CON_MASTER_MODE_BITS |
            I2C_IC_CON_IC_SLAVE_DISABLE_BITS |
            I2C_IC_CON_IC_RESTART_EN_BITS |
            I2C_IC_CON_TX_EMPTY_CTRL_BITS;

    // Set FIFO watermarks to 1 to make things simpler. This is encoded by a register value of 0.
    i2c->hw->tx_tl = 0;
    i2c->hw->rx_tl = 0;

    // Always enable the DREQ signalling -- harmless if DMA isn't listening
    i2c->hw->dma_cr = I2C_IC_DMA_CR_TDMAE_BITS | I2C_IC_DMA_CR_RDMAE_BITS;

    memset(&i2cReadWrite, 0, sizeof(i2c_read_write_t));

    i2c->hw->intr_mask = 
        I2C_IC_INTR_MASK_M_TX_ABRT_BITS | 
        I2C_IC_INTR_MASK_M_TX_EMPTY_BITS | 
        I2C_IC_INTR_MASK_M_STOP_DET_BITS | 
        I2C_IC_INTR_MASK_M_RX_FULL_BITS;

    irq_set_exclusive_handler(I2C0_IRQ, irqI2CHandler);
    irq_set_enabled(I2C0_IRQ, true);

    // Re-sets i2c->hw->enable upon returning:
    return i2c_set_baudrate(i2c, baudrate);
}

void i2cTriggerRead(i2c_inst_t * i2c, uint16_t callbackTask, uint8_t addr, uint8_t * dst, size_t len, bool nostop) {
    i2cReadWrite.addr = addr;
    i2cReadWrite.buffer = dst;
    i2cReadWrite.rxLen = len;
    i2cReadWrite.ix = 0;
    i2cReadWrite.rxNoStop = nostop;

    i2cReadWrite.i2c = i2c;
    i2cReadWrite.rxCallbackTask = callbackTask;

    i2cReadWrite.i2c->hw->enable = 0;
    i2cReadWrite.i2c->hw->tar = i2cReadWrite.addr;
    i2cReadWrite.i2c->hw->enable = 1;

    i2cReadWrite.count_RX_FULL = 0;
    i2cReadWrite.count_STOP_DET = 0;
    i2cReadWrite.count_TX_ABRT = 0;
    i2cReadWrite.count_TX_EMPTY = 0;

    i2c->hw->intr_mask = 
        I2C_IC_INTR_MASK_M_TX_ABRT_BITS | 
        I2C_IC_INTR_MASK_M_RX_FULL_BITS | 
        I2C_IC_INTR_MASK_M_STOP_DET_BITS;

    scheduleTaskOnce(TASK_I2C_READ, rtc_val_ms(2), &i2cReadWrite);
}

void i2cTriggerWrite(i2c_inst_t * i2c, uint16_t callbackTask, uint8_t addr, uint8_t * src, size_t len, bool nostop) {
    bool        isFirst;
    bool        isLast;

    i2cReadWrite.addr = addr;
    i2cReadWrite.buffer = src;
    i2cReadWrite.txLen = len;
    i2cReadWrite.ix = 0;
    i2cReadWrite.txNoStop = nostop;

    i2cReadWrite.i2c = i2c;
    i2cReadWrite.txCallbackTask = callbackTask;

    i2cReadWrite.i2c->hw->enable = 0;
    i2cReadWrite.i2c->hw->tar = i2cReadWrite.addr;
    i2cReadWrite.i2c->hw->enable = 1;

    i2cReadWrite.count_RX_FULL = 0;
    i2cReadWrite.count_STOP_DET = 0;
    i2cReadWrite.count_TX_ABRT = 0;
    i2cReadWrite.count_TX_EMPTY = 0;

    i2c->hw->intr_mask = 
        I2C_IC_INTR_MASK_M_TX_ABRT_BITS | 
        I2C_IC_INTR_MASK_M_TX_EMPTY_BITS | 
        I2C_IC_INTR_MASK_M_STOP_DET_BITS;

    isFirst = (i2cReadWrite.ix == 0);
    isLast = (i2cReadWrite.ix == (i2cReadWrite.txLen - 1));

    i2cReadWrite.i2c->hw->data_cmd =
            bool_to_bit(isFirst && i2cReadWrite.i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
            bool_to_bit(isLast && !i2cReadWrite.txNoStop) << I2C_IC_DATA_CMD_STOP_LSB |
            i2cReadWrite.buffer[i2cReadWrite.ix++];
}

void i2cTriggerReadRegister(
                i2c_inst_t * i2c, 
                uint16_t callbackTask, 
                rtc_t writeReadDelay, 
                uint8_t addr, 
                uint8_t * src,
                size_t srcLen,
                uint8_t * dst, 
                size_t dstLen, 
                bool nostopWrite,
                bool nostopRead)
{
    bool        isFirst;
    bool        isLast;

    i2cReadWrite.txCallbackTask = TASK_I2C_READ;
    i2cReadWrite.rxCallbackTask = callbackTask;
    i2cReadWrite.addr = addr;
    i2cReadWrite.buffer = src;
    i2cReadWrite.i2c = i2c;
    i2cReadWrite.txLen = srcLen;
    i2cReadWrite.rxLen = dstLen;
    i2cReadWrite.ix = 0;
    i2cReadWrite.txNoStop = nostopWrite;
    i2cReadWrite.rxNoStop = nostopRead;
    i2cReadWrite.readDelay = writeReadDelay;

    i2cReadWrite.i2c->hw->enable = 0;
    i2cReadWrite.i2c->hw->tar = i2cReadWrite.addr;
    i2cReadWrite.i2c->hw->enable = 1;

    i2cReadWrite.count_RX_FULL = 0;
    i2cReadWrite.count_STOP_DET = 0;
    i2cReadWrite.count_TX_ABRT = 0;
    i2cReadWrite.count_TX_EMPTY = 0;

    i2c->hw->intr_mask = 
        I2C_IC_INTR_MASK_M_TX_ABRT_BITS | 
        I2C_IC_INTR_MASK_M_TX_EMPTY_BITS | 
        I2C_IC_INTR_MASK_M_RX_FULL_BITS | 
        I2C_IC_INTR_MASK_M_STOP_DET_BITS;

    isFirst = (i2cReadWrite.ix == 0);
    isLast = (i2cReadWrite.ix == (i2cReadWrite.txLen - 1));

    i2cReadWrite.i2c->hw->data_cmd =
            bool_to_bit(isFirst && i2cReadWrite.i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
            bool_to_bit(isLast && !i2cReadWrite.txNoStop) << I2C_IC_DATA_CMD_STOP_LSB |
            i2cReadWrite.buffer[i2cReadWrite.ix++];
}

int i2cReadBlocking(i2c_inst_t * i2c, uint8_t addr, uint8_t * dst, size_t len, bool nostop) {
    bool            abort = false;
    uint32_t        abort_reason;
    uint8_t         b;
    int             byte_ctr;
    int             ilen = (int)len;

    i2c->hw->enable = 0;
    i2c->hw->tar = addr;
    i2c->hw->enable = 1;

    lgLogDebug("I2C_read: Ad:0x%02X, Ln:%d", addr, (int)len);

    for (byte_ctr = 0; byte_ctr < ilen; ++byte_ctr) {
        bool first = byte_ctr == 0;
        bool last = byte_ctr == ilen - 1;
        
        // lgLogDebug("I2C_read: check write");
        // while (!i2c_get_write_available(i2c)) {
        //     tight_loop_contents();
        // }

        i2c->hw->data_cmd =
                bool_to_bit(first && i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
                bool_to_bit(last && !nostop) << I2C_IC_DATA_CMD_STOP_LSB |
                I2C_IC_DATA_CMD_CMD_BITS; // -> 1 for read

        lgLogDebug("I2C_read: set I2C_cmd");
        do {
            abort_reason = i2c->hw->tx_abrt_source;
            abort = (bool) i2c->hw->clr_tx_abrt;
        }
        while (!abort && !i2c_get_read_available(i2c));

        if (abort) {
            lgLogError("Abort: Rn:0x%08X\n", abort_reason);
            break;
        }

        b = (uint8_t)i2c->hw->data_cmd;

        lgLogDebug("Read: 0x%02X", b);

        *dst++ = b;
    }

    int rval;

    if (abort) {
        if (!abort_reason || abort_reason & I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS) {
            // No reported errors - seems to happen if there is nothing connected to the bus.
            // Address byte not acknowledged
            rval = PICO_ERROR_GENERIC;
        }
        else {
            rval = PICO_ERROR_GENERIC;
        }
    }
    else {
        rval = byte_ctr;
    }

    i2c->restart_on_next = nostop;
    
    return rval;
}

int i2cWriteBlocking(i2c_inst_t * i2c, uint8_t addr, const uint8_t * src, size_t len, bool nostop) {
    bool            abort = false;
    uint32_t        abort_reason = 0;
    int             byte_ctr;
    int             ilen = (int)len;

    i2c->hw->enable = 0;
    i2c->hw->tar = addr;
    i2c->hw->enable = 1;

    for (byte_ctr = 0; byte_ctr < ilen; ++byte_ctr) {
        bool first = byte_ctr == 0;
        bool last = byte_ctr == ilen - 1;

        i2c->hw->data_cmd =
                bool_to_bit(first && i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
                bool_to_bit(last && !nostop) << I2C_IC_DATA_CMD_STOP_LSB |
                *src++;

        // Wait until the transmission of the address/data from the internal
        // shift register has completed. For this to function correctly, the
        // TX_EMPTY_CTRL flag in IC_CON must be set. The TX_EMPTY_CTRL flag
        // was set in i2c_init.
        while (i2c->hw->txflr) {
            tight_loop_contents();
        }

        abort_reason = i2c->hw->tx_abrt_source;

        if (abort_reason) {
            // Note clearing the abort flag also clears the reason, and
            // this instance of flag is clear-on-read! Note also the
            // IC_CLR_TX_ABRT register always reads as 0.
            i2c->hw->clr_tx_abrt;
            abort = true;
        }

        if (abort || (last && !nostop)) {
            // If the transaction was aborted or if it completed
            // successfully wait until the STOP condition has occured.

            // TODO Could there be an abort while waiting for the STOP
            // condition here? If so, additional code would be needed here
            // to take care of the abort.
            do {
                tight_loop_contents();
            }
            while (!(i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS));

            i2c->hw->clr_stop_det;
        }

        // Note the hardware issues a STOP automatically on an abort condition.
        // Note also the hardware clears RX FIFO as well as TX on abort,
        // because we set hwparam IC_AVOID_RX_FIFO_FLUSH_ON_TX_ABRT to 0.
        if (abort) {
            break;
        }
    }

    int rval;

    // A lot of things could have just happened due to the ingenious and
    // creative design of I2C. Try to figure things out.
    if (abort) {
        if (!abort_reason || abort_reason & I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS) {
            // No reported errors - seems to happen if there is nothing connected to the bus.
            // Address byte not acknowledged
            rval = PICO_ERROR_GENERIC;
        }
        else if (abort_reason & I2C_IC_TX_ABRT_SOURCE_ABRT_TXDATA_NOACK_BITS) {
            // Address acknowledged, some data not acknowledged
            rval = byte_ctr;
        }
        else {
            rval = PICO_ERROR_GENERIC;
        }
    }
    else {
        rval = byte_ctr;
    }

    // nostop means we are now at the end of a *message* but not the end of a *transfer*
    i2c->restart_on_next = nostop;

    return rval;
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

    bytesWritten = i2cWriteBlocking(i2c, addr, msg, (length + 1), false);

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
    i2cWriteBlocking(i2c, addr, &reg, 1, true);
    bytesRead = i2cReadBlocking(i2c, addr, data, length, false);

    return bytesRead;
}
