#include "log.h"

#define LOG_BUFFER_SIZE 256
#define MAX_ARGS 30

static char logfile[27];
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
// 0 Print everything, 1 Print everything but debug info, 2 Print nothing (Everything is logs regardless of the mode)
static int printMode;

// Initializes the log file in the format yyyy-dd-MM-hhmmss
int log_init(int mode)
{
    if (mode < 0 || mode > 2)
    {
        return -1;
    }
    printMode = mode;
    time_t t = time(NULL);
    struct tm now = *localtime(&t);
    // yyyy-dd-MM-hhmmss.log
    mkdir("logs", 0777);
    snprintf(logfile, 27, "logs/%04d-%02d-%02d-%02d%02d%02d.log",
             now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
}

int log_log(enum logSigs sig, const char *fmt, ...)
{
    char *err_type;
    switch (sig)
    {
    case LOG_ERROR:
        err_type = "Error";
        break;
    case LOG_WARN:
        err_type = "Warn";
        break;
    case LOG_INFO:
        err_type = "Info";
        break;
    case LOG_DEBUG:
        err_type = "Debug";
        break;
    default:
        log_log(LOG_ERROR, "Not a handled signal");
        return -1;
    }

    /*va_list fmtList;
    va_start(fmtList, 20);


    va_end(fmtList);*/

    time_t t = time(NULL);
    struct tm now = *localtime(&t);
    char msg[256];

    // TODO Get the thread name instead of the ptid
    snprintf(msg, 256, "[%02d:%02d:%02d] %5s: (ptid:%li) %s\n",
             now.tm_hour, now.tm_min, now.tm_sec, err_type, pthread_self(), fmt);
    // EHHHH
    pthread_mutex_lock(&mtx);
    FILE *fd = fopen(logfile, "a");
    fprintf(fd, msg);
    fclose(fd);
    pthread_mutex_unlock(&mtx);

    switch (printMode)
    {
    case 0:
        printf("%s", msg);
        break;
    case 1:
        if (*err_type != LOG_DEBUG)
        {
            printf("%s", msg);
        }
        break;
    default:
        //Do nothing
        break;
    }

    return 0;
}