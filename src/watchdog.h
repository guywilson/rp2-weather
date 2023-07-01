#include "scheduler.h"

#ifndef __INCL_WATCHDOG
#define __INCL_WATCHDOG

void watchdog_disable(void);
void triggerWatchdogReset(void);
void taskWatchdog(PTASKPARM p);
void taskWatchdogWakeUp(PTASKPARM p);

#endif
