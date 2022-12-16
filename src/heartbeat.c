#include <stddef.h>
#include "scheduler.h"

#include "rtc_rp2040.h"
#include "heartbeat.h"
#include "led_utils.h"
#include "taskdef.h"

#define HEARTBEAT_ON_TIME                50
#define HEARBEAT_OFF_TIME               450

void HeartbeatTask(PTASKPARM p) {
  static uint8_t on = 0;

  if (on) {
    turnOff(LED_ONBOARD);
    on = 0;

    scheduleTaskOnce(
            TASK_HEARTBEAT, 
            rtc_val_ms(getCoreID() ? HEARBEAT_OFF_TIME : HEARTBEAT_ON_TIME), 
            NULL);
  }
  else {
    turnOn(LED_ONBOARD);
    on = 1;

    scheduleTaskOnce(
            TASK_HEARTBEAT, 
            rtc_val_ms(getCoreID() ? HEARTBEAT_ON_TIME : HEARBEAT_OFF_TIME), 
            NULL);
  }
}
