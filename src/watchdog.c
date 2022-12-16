#include <stddef.h>
#include "scheduler.h"

#include "hardware/watchdog.h"
#include "rtc_rp2040.h"
#include "watchdog.h"
#include "taskdef.h"

void WatchdogTask(PTASKPARM p)
{
    watchdog_update();
}
