#include "scheduler.h"

#ifndef _INCL_RTC_RP2040
#define _INCL_RTC_RP2040

/*
 * Set the RTC clock frequency to 10KHz (100us tick). This is a
 * good frequency for the real-time scheduler...
 */
#define RTC_CLOCK_FREQ					10000

/*
** Convenience macros for scheduler time periods.
** The resolution of the scheduler is 100us...
*/
#define rtc_val_100us(time_in_us)			((time_in_us) * (RTC_CLOCK_FREQ / 10000))
#define rtc_val_ms(time_in_ms)				((time_in_ms) * (RTC_CLOCK_FREQ / 1000))

#define rtc_val_sec(time_in_sec)			rtc_val_ms(time_in_sec * 1000)
#define rtc_val_min(time_in_min)			rtc_val_sec(time_in_min * 60)
#define rtc_val_hr(time_in_hr)				rtc_val_min(time_in_hr * 60)

#define RTC_ONE_SECOND						rtc_val_ms(1000)
#define RTC_ONE_MINUTE						rtc_val_ms(60000)
#define RTC_ONE_HOUR						rtc_val_ms(3600000)


void		setupRTC();

#endif
