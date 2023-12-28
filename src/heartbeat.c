#include <stddef.h>
#include "scheduler.h"

#include "rtc_rp2040.h"
#include "heartbeat.h"
#include "utils.h"
#include "taskdef.h"
#include "gpio_def.h"

#define HEARTBEAT_ON_TIME                20
#define HEARBEAT_OFF_TIME               980

void HeartbeatTask(PTASKPARM p) {
    static uint8_t on = 0;

    if (isDebugActive()) {
        if (on) {
            turnOff(ONBAORD_LED_PIN);
            on = 0;

            scheduleTask(
                    TASK_HEARTBEAT, 
                    rtc_val_ms(HEARBEAT_OFF_TIME), 
                    false,
                    NULL);
        }
        else {
            turnOn(ONBAORD_LED_PIN);
            on = 1;

            scheduleTask(
                    TASK_HEARTBEAT, 
                    rtc_val_ms(HEARTBEAT_ON_TIME), 
                    false,
                    NULL);
        }
    }
    else {
        scheduleTask(
                TASK_HEARTBEAT, 
                rtc_val_sec(1), 
                false,
                NULL);
    }
}
