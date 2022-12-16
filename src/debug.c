#include <stdlib.h>

#include "scheduler.h"
#include "taskdef.h"
#include "hardware/uart.h"

void TaskDebug(PTASKPARM p) {
    uart_puts(uart0, "Hello World! - Running on core: ");
    uart_putc(uart0, getCoreID() + 0x30);
    uart_putc(uart0, '\n');
}
