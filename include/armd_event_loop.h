#ifndef ARMD_EVENT_LOOP_H
#define ARMD_EVENT_LOOP_H

#include <stdint.h>

#define EPOLL_EVENTS_MAXNUM 512

typedef void(*armd_event_cb)(int fd, uint32_t event, void *arg);

int armd_event_loop_init(void);
int armd_event_loop_add(int fd, uint32_t event, armd_event_cb cb, void *arg);
void armd_event_loop_run(void);
void armd_event_loop_exit(void);

#endif