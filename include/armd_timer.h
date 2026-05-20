#ifndef ARMD_TIMER_H
#define ARMD_TIMER_H

#include <stdint.h>

int armd_timer_create(int interval_sec);
void armd_timer_cb(int fd, uint32_t event, void *arg);

#endif