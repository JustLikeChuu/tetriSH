#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevelT;

static const char* level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};

/* Comment out LOGGER_UP to make every LOG_X call a no-op. */
// #define LOGGER_UP
#ifdef LOGGER_UP
#define LOG(level, fmt, ...) fprintf(stderr, "[%s] %s [%s]:%d: " fmt "\n", level_str[level], __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define LOG(level, fmt, ...) ((void)0)
#endif

#define LOG_D(fmt, ...) LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_I(fmt, ...) LOG(LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...) LOG(LOG_WARN, fmt, ##__VA_ARGS__)
#define LOG_E(fmt, ...) LOG(LOG_ERROR, fmt, ##__VA_ARGS__)

#endif
