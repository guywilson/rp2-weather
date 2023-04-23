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
#include "gpio_def.h"

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

    uint16_t            callbackTask;

    uint8_t             addr;
    size_t              len;
    uint8_t *           buffer;
    bool                noStop;

    rtc_t               readDelay;
}
i2c_read_write_t;

static i2c_read_write_t         i2cRdStruct;
static i2c_read_write_t         i2cWrStruct;

void taskI2CWrite(PTASKPARM p) {
    static int              state = I2C_WRITE_STATE_BEGIN;
    static int              ix = 0;
    static bool             abort = false;
    rtc_t                   delay;
    bool                    isFirst = false;
    bool                    isLast = false;
    uint32_t                abort_reason;
    uint8_t                 b;
    i2c_read_write_t *      i2cWrite;

    i2cWrite = (i2c_read_write_t *)p;

    switch (state) {
        case I2C_WRITE_STATE_BEGIN:
            lgLogDebug("I2CWr: begin");
            /*
            ** Let's just do the blocking write for now, there is a bug in
            ** the state machine logic below that I haven't managed to identify
            ** yet...
            */
            i2cWriteBlocking(i2cWrite->i2c, i2cWrite->addr, i2cWrite->buffer, i2cWrite->len, i2cWrite->noStop);
            state = I2C_WRITE_STATE_CALLBACK;
            delay = rtc_val_ms(1);
            break;
            // i2cWrite->i2c->hw->enable = 0;
            // i2cWrite->i2c->hw->tar = i2cWrite->addr;
            // i2cWrite->i2c->hw->enable = 1;

            // Deliberately fall through...

        case I2C_WRITE_STATE_TX_BYTE:
            lgLogDebug("I2CWr: tx");
            isFirst = (ix == 0);
            isLast = (ix == (i2cWrite->len - 1));

            b = i2cWrite->buffer[ix++];

            i2cWrite->i2c->hw->data_cmd =
                    bool_to_bit(isFirst && i2cWrite->i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
                    bool_to_bit(isLast && !i2cWrite->noStop) << I2C_IC_DATA_CMD_STOP_LSB |
                    b;

            // Deliberately fall through...

        case I2C_WRITE_STATE_TX_WAIT:
            lgLogDebug("I2CWr: txw");
            if (i2cWrite->i2c->hw->txflr) {
                state = I2C_WRITE_STATE_TX_WAIT;
                delay = rtc_val_ms(1);
                break;
            }

            isFirst = (ix == 0);
            isLast = (ix == (i2cWrite->len - 1));

            abort_reason = i2cWrite->i2c->hw->tx_abrt_source;

            if (abort_reason) {
                // Note clearing the abort flag also clears the reason, and
                // this instance of flag is clear-on-read! Note also the
                // IC_CLR_TX_ABRT register always reads as 0.
                i2cWrite->i2c->hw->clr_tx_abrt;
                abort = true;
            }

            // Deliberately fall through...

        case I2C_WRITE_STATE_TX_CHECK:
            lgLogDebug("I2CWr: txc");
            isLast = (ix == (i2cWrite->len - 1));

            if ((abort || (isLast && !i2cWrite->noStop))) {
                if (!(i2cWrite->i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS)) {
                    state = I2C_WRITE_STATE_TX_CHECK;
                    delay = rtc_val_ms(1);
                    break;
                }
            }

            // Deliberately fall through...

        case I2C_WRITE_STATE_TX_FINALISE:
            lgLogDebug("I2CWr: txf");

            isLast = (ix == (i2cWrite->len - 1));

            if (abort || (isLast && !i2cWrite->noStop)) {
                i2cWrite->i2c->hw->clr_stop_det;
            }

            // Note the hardware issues a STOP automatically on an abort condition.
            // Note also the hardware clears RX FIFO as well as TX on abort,
            // because we set hwparam IC_AVOID_RX_FIFO_FLUSH_ON_TX_ABRT to 0.
            if (!abort && !isLast) {
                state = I2C_WRITE_STATE_TX_BYTE;
                delay = rtc_val_ms(1);
                break;
            }

            i2cWrite->i2c->restart_on_next = i2cWrite->noStop;

            // Deliberately fall through...

        case I2C_WRITE_STATE_CALLBACK:
            lgLogDebug("I2CWr: cb");
            state = I2C_WRITE_STATE_BEGIN;
            abort = false;
            ix = 0;

            scheduleTask(i2cWrite->callbackTask, i2cWrite->readDelay, false, &i2cRdStruct);
            return;
    }

    scheduleTask(TASK_I2C_WRITE, delay, false, p);
}

void taskI2CRead(PTASKPARM p) {
    static int              state = I2C_READ_STATE_BEGIN;
    static int              ix = 0;
    rtc_t                   delay;
    bool                    isFirst = false;
    bool                    isLast = false;
    bool                    abort = false;
    uint32_t                abort_reason;
    uint8_t                 b;
    i2c_read_write_t *      i2cRead;

    i2cRead = (i2c_read_write_t *)p;

    switch (state) {
        case I2C_READ_STATE_BEGIN:
            i2cRead->i2c->hw->enable = 0;
            i2cRead->i2c->hw->tar = i2cRead->addr;
            i2cRead->i2c->hw->enable = 1;

            // Deliberately fall through...

        case I2C_READ_STATE_WRITE_CMD:
            if (!i2c_get_write_available(i2cRead->i2c)) {
                delay = rtc_val_ms(1);
                break;
            }

            isFirst = (ix == 0);
            isLast = (ix == (i2cRead->len - 1));

            i2cRead->i2c->hw->data_cmd =
                    bool_to_bit(isFirst && i2cRead->i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
                    bool_to_bit(isLast && !i2cRead->noStop) << I2C_IC_DATA_CMD_STOP_LSB |
                    I2C_IC_DATA_CMD_CMD_BITS; // -> 1 for read

            // Deliberately fall through...

        case I2C_READ_STATE_READ_BYTE:
            abort_reason = i2cRead->i2c->hw->tx_abrt_source;
            abort = (bool) i2cRead->i2c->hw->clr_tx_abrt;

            if (!abort && !i2c_get_read_available(i2cRead->i2c)) {
                state = I2C_READ_STATE_READ_BYTE;
                delay = rtc_val_ms(1);
                break;
            }
            else {
                if (abort) {
                    if (!abort_reason || abort_reason & I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS) {
                        lgLogError("I2CRdTask: Abort reason 0x%04X", abort_reason);
                    }
                    else {
                        lgLogFatal("I2CRdTask: Panic - abort reason 0x%04X", abort_reason);
                        return;
                    }                
                }
                else {
                    b = (uint8_t)i2cRead->i2c->hw->data_cmd;
                    i2cRead->buffer[ix++] = b;

                    lgLogDebug("Read: 0x%02X", b);

                    /*
                    ** Head back above until we're done, then fall through
                    ** to callback...
                    */
                    if (ix < i2cRead->len) {
                        state = I2C_READ_STATE_WRITE_CMD;
                        delay = rtc_val_ms(1);
                        
                        i2cRead->i2c->restart_on_next = i2cRead->noStop;
                        break;
                    }
                }
            }

            i2cRead->i2c->restart_on_next = i2cRead->noStop;

        case I2C_READ_STATE_CALLBACK:
            state = I2C_READ_STATE_BEGIN;
            ix = 0;

            scheduleTask(i2cRead->callbackTask, rtc_val_ms(5), false, NULL);
            return;
    }

    scheduleTask(TASK_I2C_READ, delay, false, p);
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

void i2cPowerUp(void) {
    gpio_put(I2C0_POWER_PIN, 1);
}

void i2cPowerDown(void) {
    gpio_put(I2C0_POWER_PIN, 0);
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

    gpio_init(I2C0_POWER_PIN);
    gpio_set_dir(I2C0_POWER_PIN, GPIO_OUT);
    gpio_set_drive_strength(I2C0_POWER_PIN, GPIO_DRIVE_STRENGTH_12MA);

    // Re-sets i2c->hw->enable upon returning:
    return i2c_set_baudrate(i2c, baudrate);
}

int i2cTriggerRead(i2c_inst_t * i2c, uint16_t callbackTask, uint8_t addr, uint8_t * dst, size_t len, bool nostop) {
    i2cRdStruct.callbackTask = callbackTask;
    i2cRdStruct.addr = addr;
    i2cRdStruct.buffer = dst;
    i2cRdStruct.i2c = i2c;
    i2cRdStruct.len = len;
    i2cRdStruct.noStop = nostop;

    scheduleTask(TASK_I2C_READ, rtc_val_ms(2), false, &i2cRdStruct);
    
    return 0;
}

int i2cTriggerWrite(i2c_inst_t * i2c, uint16_t callbackTask, uint8_t addr, uint8_t * src, size_t len, bool nostop) {
    i2cWrStruct.callbackTask = callbackTask;
    i2cWrStruct.addr = addr;
    i2cWrStruct.buffer = src;
    i2cWrStruct.i2c = i2c;
    i2cWrStruct.len = len;
    i2cWrStruct.noStop = nostop;

    scheduleTask(TASK_I2C_WRITE, rtc_val_ms(2), false, &i2cWrStruct);
    
    return 0;
}

int i2cTriggerReadRegister(
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
    i2cWrStruct.callbackTask = TASK_I2C_READ;
    i2cWrStruct.addr = addr;
    i2cWrStruct.buffer = src;
    i2cWrStruct.i2c = i2c;
    i2cWrStruct.len = srcLen;
    i2cWrStruct.noStop = nostopWrite;
    i2cWrStruct.readDelay = writeReadDelay;

    i2cRdStruct.callbackTask = callbackTask;
    i2cRdStruct.addr = addr;
    i2cRdStruct.buffer = dst;
    i2cRdStruct.i2c = i2c;
    i2cRdStruct.len = dstLen;
    i2cRdStruct.noStop = nostopRead;

    scheduleTask(TASK_I2C_WRITE, rtc_val_ms(2), false, &i2cWrStruct);
    
    return 0;
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
