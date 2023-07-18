#include <stddef.h>
#include "scheduler.h"

#include "rtc_rp2040.h"
#include "heartbeat.h"
#include "utils.h"
#include "taskdef.h"
#include "gpio_def.h"

#define HEARTBEAT_ON_TIME                20
#define HEARTBEAT_OFF_TIME               240

#define STATE_LED_ON_1                  0x0010
#define STATE_LED_OFF_1                 0x0020
#define STATE_LED_ON_2                  0x0030
#define STATE_LED_OFF_2                 0x0040

void HeartbeatTask(PTASKPARM p) {
    static uint16_t state = STATE_LED_ON_1;

    switch (state) {
        case STATE_LED_ON_1:
            turnOn(ONBAORD_LED_PIN);
            state = STATE_LED_OFF_1;
            scheduleTask(TASK_HEARTBEAT, rtc_val_ms(HEARTBEAT_ON_TIME), false, NULL);
            break;

        case STATE_LED_OFF_1:
            turnOff(ONBAORD_LED_PIN);
            state = STATE_LED_ON_2;
            scheduleTask(TASK_HEARTBEAT, rtc_val_ms(HEARTBEAT_OFF_TIME), false, NULL);
            break;

        case STATE_LED_ON_2:
            turnOn(ONBAORD_LED_PIN);
            state = STATE_LED_OFF_2;
            scheduleTask(TASK_HEARTBEAT, rtc_val_ms(HEARTBEAT_ON_TIME), false, NULL);
            break;

        case STATE_LED_OFF_2:
            turnOff(ONBAORD_LED_PIN);
            state = STATE_LED_ON_1;
            break;
    }
}
