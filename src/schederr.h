
#ifndef _INCL_ERROR
#define _INCL_ERROR

#define ERROR_SCHED_NOFREETASKS				0x0100
#define ERROR_SCHED_TASKCOUNTOVERFLOW		0x0101
#define ERROR_SCHED_NULLTASK				0x0102
#define ERROR_SCHED_DROPOUT					0x0103
#define ERROR_SCHED_NULLTASKEXEC			0x0104

extern void handleError(unsigned int code);

#endif
