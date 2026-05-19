#ifndef ARMD_LOG_H
#define ARMD_LOG_H

#include <stdio.h>

#define armd_log(fmt, ...) \
    printf("[%s]-%d# " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#endif