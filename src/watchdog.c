#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "scheduler.h"

#include "hardware/watchdog.h"
#include "taskdef.h"
#include "watchdog.h"

static bool         doUpdate = true;

void triggerWatchdogReset(void) {
    doUpdate = false;
}

void taskWatchdog(PTASKPARM p) {
    if (doUpdate) {
        watchdog_update();
    }
}
