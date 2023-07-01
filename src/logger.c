#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "hardware/uart.h"
#include "scheduler.h"
#include "logger.h"

#define LOG_BUFFER_LENGTH                   128


struct _log_handle_t {
    uart_inst_t *   uart;

    int             logLevel;
    bool            isInstantiated;
};

static log_handle_t         _log;
static char                 _logBuffer[LOG_BUFFER_LENGTH];
static char                 _logBufferOut[LOG_BUFFER_LENGTH];

static log_handle_t * lgGetHandle() {
    static log_handle_t *       pLog = NULL;

    if (pLog == NULL) {
        pLog = &_log;
        pLog->isInstantiated = false;
    }

    return pLog;
}

static int _log_message(log_handle_t * hlog, int logLevel, bool addCR, const char * fmt, va_list args) {
    int         bytesWritten = 0;

    if (hlog->logLevel & logLevel) {
        if (strlen(fmt) > MAX_LOG_LENGTH) {
            return -1;
        }

        if (addCR) {
            sprintf(_logBuffer, "[%010d] ", getRTCClock());

            switch (logLevel) {
                case LOG_LEVEL_DEBUG:
                    strncat(_logBuffer, "[DBG]", 6);
                    break;

                case LOG_LEVEL_STATUS:
                    strncat(_logBuffer, "[STA]", 6);
                    break;

                case LOG_LEVEL_INFO:
                    strncat(_logBuffer, "[INF]", 6);
                    break;

                case LOG_LEVEL_ERROR:
                    strncat(_logBuffer, "[ERR]", 6);
                    break;

                case LOG_LEVEL_FATAL:
                    strncat(_logBuffer, "[FTL]", 6);
                    break;
            }

            strncat(_logBuffer, fmt, (LOG_BUFFER_LENGTH >> 1));
            strncat(_logBuffer, "\n", 2);
        }
        else {
            strncpy(_logBuffer, fmt, (LOG_BUFFER_LENGTH >> 1));
        }

        bytesWritten = vsprintf(_logBufferOut, _logBuffer, args);
        uart_puts(hlog->uart, _logBufferOut);

        _logBuffer[0] = 0;
    }

    return bytesWritten;
}

int lgOpen(uart_inst_t * u, int logFlags) {
    log_handle_t * pLog = lgGetHandle();

    if (!pLog->isInstantiated) {
        pLog->uart = u;

        pLog->logLevel = logFlags;

        pLog->isInstantiated = true;
    }
    else {
        uart_puts(u, "Logger already initialised. You should only call lgOpen() once\n");
        return 0;
    }

    return 0;
}

void lgSetLogLevel(int logLevel) {
    log_handle_t * hlog = lgGetHandle();
    hlog->logLevel = logLevel;
}

int lgGetLogLevel() {
    log_handle_t * hlog = lgGetHandle();
    return hlog->logLevel;
}

bool lgCheckLogLevel(int logLevel) {
    log_handle_t * hlog = lgGetHandle();
    return ((hlog->logLevel & logLevel) == logLevel ? true : false);
}

void lgNewline() {
    log_handle_t * hlog = lgGetHandle();
    uart_puts(hlog->uart, "\n");
}

int lgLogInfo(const char * fmt, ...) {
    va_list     args;
    int         bytesWritten;
    
    log_handle_t * hlog = lgGetHandle();

    va_start (args, fmt);
    
    bytesWritten = _log_message(hlog, LOG_LEVEL_INFO, true, fmt, args);
    
    va_end(args);
    
    return bytesWritten;
}

int lgLogStatus(const char * fmt, ...) {
    va_list     args;
    int         bytesWritten;
    
    log_handle_t * hlog = lgGetHandle();

    va_start (args, fmt);
    
    bytesWritten = _log_message(hlog, LOG_LEVEL_STATUS, true, fmt, args);
    
    va_end(args);
    
    return bytesWritten;
}

int lgLogDebug(const char * fmt, ...) {
    va_list     args;
    int         bytesWritten;
    
    log_handle_t * hlog = lgGetHandle();

    va_start (args, fmt);
    
    bytesWritten = _log_message(hlog, LOG_LEVEL_DEBUG, true, fmt, args);
    
    va_end(args);
    
    return bytesWritten;
}

int lgLogDebugNoCR(const char * fmt, ...) {
    va_list     args;
    int         bytesWritten;
    
    log_handle_t * hlog = lgGetHandle();

    va_start (args, fmt);
    
    bytesWritten = _log_message(hlog, LOG_LEVEL_DEBUG, false, fmt, args);
    
    va_end(args);
    
    return bytesWritten;
}

int lgLogError(const char * fmt, ...) {
    va_list     args;
    int         bytesWritten;
    
    log_handle_t * hlog = lgGetHandle();

    va_start (args, fmt);
    
    bytesWritten = _log_message(hlog, LOG_LEVEL_ERROR, true, fmt, args);
    
    va_end(args);
    
    return bytesWritten;
}

int lgLogFatal(const char * fmt, ...) {
    va_list     args;
    int         bytesWritten;
    
    log_handle_t * hlog = lgGetHandle();

    va_start (args, fmt);
    
    bytesWritten = _log_message(hlog, LOG_LEVEL_FATAL, true, fmt, args);
    
    va_end(args);
    
    return bytesWritten;
}
