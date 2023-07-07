#ifndef LOG_H
#define LOG_H

#ifndef LOG_STREAM
#define LOG_STREAM stdout
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <pthread.h>
#include <sys/stat.h> /* mkdir */

#define LOG_ERROR "ERROR"
#define LOG_DEBUG "Debug"
#define LOG_WARN "Warn"
#define LOG_INFO "Info"

void log_init();

// LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG
void log_log(FILE *stream, char *sig, char *message);

#define LOG(sig, ...)                   \
    do                                  \
    {                                   \
        char mm[100];                   \
        snprintf(mm, 100, __VA_ARGS__); \
        log_log(LOG_STREAM, sig, mm);   \
    } while (0)

#endif