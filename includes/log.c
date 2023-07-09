#include "log.h"

#define LOG_BUFFER_SIZE 256
#define MAX_ARGS 30

static char logfile[27] = "logs/lol";
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// Initializes the log file in the format yyyy-dd-MM-hhmmss
void log_init()
{
    time_t t = time(NULL);
    struct tm now = *localtime(&t);
    // yyyy-dd-MM-hhmmss.log
    mkdir("logs", 0777);
    snprintf(logfile, 27, "logs/%04d-%02d-%02d-%02d%02d%02d.log",
             now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
}

void log_log(FILE *stream, char *sig, char *message)
{
    time_t t = time(NULL);
    struct tm now = *localtime(&t);
    char msg[LOG_BUFFER_SIZE];

    // va_list fmtlist;
    // va_start(fmtlist, message);

    snprintf(msg, LOG_BUFFER_SIZE, "[%02d:%02d:%02d] %5s: %s\n",
             now.tm_hour, now.tm_min, now.tm_sec, sig, message);

    // va_end(fmtlist);

    fprintf(stream, "%s", msg);

    if (!strcmp(logfile, "logs/lol"))
    {
        pthread_mutex_lock(&mtx);
        FILE *fd = fopen(logfile, "a");
        fprintf(fd, msg);
        fclose(fd);
        pthread_mutex_unlock(&mtx);
    }
}