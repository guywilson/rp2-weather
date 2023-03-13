#include <stddef.h>
#include "scheduler.h"

#include "hardware/watchdog.h"
#include "rtc_rp2040.h"
#include "watchdog.h"
#include "taskdef.h"

void watchdog_disable(void) {
	hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
}

void WatchdogTask(PTASKPARM p)
{
    watchdog_update();
}
