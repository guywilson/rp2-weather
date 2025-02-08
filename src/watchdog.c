#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "scheduler.h"

#include "hardware/watchdog.h"
#include "hardware/spi.h"
#include "rtc_rp2040.h"
#include "taskdef.h"
#include "packet.h"
#include "sensor.h"
#include "nRF24L01.h"
#include "gpio_cntrl.h"
#include "watchdog.h"

#define STATE_START                         0x0001
#define STATE_RADIO_TX_POWERUP              0x0100
#define STATE_RADIO_TX_SEND                 0x0200
#define STATE_RADIO_TX_FINISH               0x0300
#define STATE_END                           0xFF00

static bool         doUpdate = true;

void triggerWatchdogReset(void) {
    doUpdate = false;
}

void taskWatchdog(PTASKPARM p) {
    if (doUpdate) {
        watchdog_update();
    }
}

void taskWatchdogWakeUp(PTASKPARM p) {
    static int          state = STATE_START;
    rtc_t               delay;
    watchdog_packet_t * pWatchdog;
    uint8_t             buffer[32];

    pWatchdog = getWatchdogPacket();

    switch (state) {
        case STATE_START:
            initGPIOs();
            spi_init(spi0, 5000000);

            state = STATE_RADIO_TX_POWERUP;
            delay = rtc_val_sec(1);
            break;

        case STATE_RADIO_TX_POWERUP:
            nRF24L01_powerUpTx(spi0);

            state = STATE_RADIO_TX_SEND;
            delay = rtc_val_ms(200);
            break;

        case STATE_RADIO_TX_SEND:
            memcpy(buffer, pWatchdog, sizeof(watchdog_packet_t));
            nRF24L01_transmit_buffer(spi0, buffer, sizeof(watchdog_packet_t), false);
            
            state = STATE_RADIO_TX_FINISH;
            delay = rtc_val_ms(200);
            break;

        case STATE_RADIO_TX_FINISH:
            nRF24L01_powerDown(spi0);
            
            state = STATE_END;
            delay = rtc_val_ms(200);
            break;

        case STATE_END:
            spi_deinit(spi0);
            deInitGPIOs();
            
            state = STATE_START;
            return;
    }

    scheduleTask(TASK_WATCHDOG_WAKEUP, delay, false, NULL);
}
