#ifndef LOG_H
#define LOG_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

static inline char *timenow();

#define _FILE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__

#define LOG_ARGS(LOG_TAG)   timenow(), LOG_TAG, _FILE, __LINE__

#define LOG_FMT  "[%s] [%s] [%s:%d] "

#define NEWLINE     "\n"

#ifdef __OBJC__
#define PRINTFUNCTION(msg, ...)      objc_print(@msg, __VA_ARGS__)
#else
#define PRINTFUNCTION(msg, ...)      fprintf(stderr, msg, __VA_ARGS__)

#endif

#define ERROR_TAG "ERROR"
#define INFO_TAG "INFO"

#define INFO_LOG(msg, args...) PRINTFUNCTION(GRN LOG_FMT RESET msg NEWLINE, LOG_ARGS(INFO_TAG), ##args)

#define ERROR_LOG(msg, args...) PRINTFUNCTION(RED LOG_FMT RESET msg NEWLINE, LOG_ARGS(ERROR_TAG), ##args)


static inline char *timenow() {
    static char buffer[64];
    time_t rawtime;
    struct tm *timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", timeinfo);
    
    return buffer;
}


#endif