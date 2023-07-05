#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <pthread.h>
#include <sys/stat.h> /* mkdir */

enum logSigs
{
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

int log_init(int mode);

// LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG
int log_log(enum logSigs sig, const char *fmt, ...);

#define log_error(message, ...) log_log(LOG_ERROR, message, ##__VA_ARGS__)
#define log_warn(message, ...) log_log(LOG_WARN, message, ##__VA_ARGS__)
#define log_info(message, ...) log_log(LOG_INFO, message, ##__VA_ARGS__)
#define log_debug(message, ...) log_log(LOG_DEBUG, message, ##__VA_ARGS__)

#endif
