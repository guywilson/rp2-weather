#include "scheduler.h"

#ifndef _INCL_RTC_RP2040
#define _INCL_RTC_RP2040

/*
 * Set the RTC clock frequency to 10Hz (100ms tick)...
 */
#define RTC_CLOCK_FREQ					10
#define RTC_DEFAULT_INTERRUPT_CYCLE     100000U

void		setupRTC(void);
void        disableRTC(void);
void        rtcDelay(uint32_t delay_us);
double      getRTCFrequency(void);
void        setRTCFrequency(double frequency);


/*
** Convenience macros for scheduler time periods.
** The resolution of the scheduler is 100ms...
*/
#define rtc_val_ms(time_in_ms)				(rtc_t)((double)(time_in_ms) * ((double)RTC_CLOCK_FREQ / (double)1000))

#define rtc_val_sec(time_in_sec)			rtc_val_ms(time_in_sec * 1000)
#define rtc_val_min(time_in_min)			rtc_val_sec(time_in_min * 60)
#define rtc_val_hr(time_in_hr)				rtc_val_min(time_in_hr * 60)

#define RTC_ONE_SECOND						rtc_val_ms(1000)
#define RTC_ONE_MINUTE						rtc_val_ms(60000)
#define RTC_ONE_HOUR						rtc_val_ms(3600000)

#endif
