#include "scheduler.h"

#ifndef __INCL_BATTERY
#define __INCL_BATTERY

#define ADC_BATTERY_VOLTAGE_CRITICAL        1427

void taskBatteryMonitor(PTASKPARM p);

#endif
