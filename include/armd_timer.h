#ifndef ARMD_TIMER_H
#define ARMD_TIMER_H

#include <stdint.h>
#include <stdbool.h>

#define ARMD_TIMER_INITIAL_CAPACITY 16

typedef int (*armd_timer_callback)(void *arg);

int armd_timer_list_create(void);
int armd_timer_add(uint64_t delay_ms, uint64_t interval_ms, armd_timer_callback cb, void *cb_arg);
void armd_timer_delete(int timer_id)
void armd_timer_cb(int fd, uint32_t event, void *arg);

#endif