#include <stddef.h>
#include "scheduler.h"

#include "rtc_rp2040.h"
#include "heartbeat.h"
#include "utils.h"
#include "taskdef.h"

#define HEARTBEAT_ON_TIME                50
#define HEARBEAT_OFF_TIME               950

void HeartbeatTask(PTASKPARM p) {
  static uint8_t on = 0;

  if (on) {
    turnOff(LED_ONBOARD);
    on = 0;

    scheduleTask(
            TASK_HEARTBEAT, 
            rtc_val_ms(getCoreID() ? HEARBEAT_OFF_TIME : HEARTBEAT_ON_TIME), 
            false,
            NULL);
  }
  else {
    turnOn(LED_ONBOARD);
    on = 1;

    scheduleTask(
            TASK_HEARTBEAT, 
            rtc_val_ms(getCoreID() ? HEARTBEAT_ON_TIME : HEARBEAT_OFF_TIME), 
            false,
            NULL);
  }
}
