#include "scheduler.h"

#ifndef __INCL_WATCHDOG
#define __INCL_WATCHDOG

void watchdog_disable(void);
void WatchdogTask(PTASKPARM p);

#endif
