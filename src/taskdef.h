#ifndef _INCL_TASKDEF
#define _INCL_TASKDEF

#define TASK_HEARTBEAT			0x0100
#define TASK_WATCHDOG           0x0200
#define TASK_WATCHDOG_WAKEUP    0x0201
#define TASK_DEBUG              0x0300
#define TASK_I2C_SENSOR         0x0400
#define TASK_ADC                0x0700
#define TASK_ANEMOMETER         0x0800
#define TASK_RAIN_GAUGE         0x0900
#define TASK_BATTERY_MONITOR    0x0A00
#define TASK_DEBUG_CHECK        0x0B00

#define TASK_PWM_ANEMOMETER     0xFF00
#define TASK_PWM_RAIN_GAUGE     0xFF10

#endif
