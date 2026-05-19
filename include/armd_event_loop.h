#ifndef ARMD_EVENT_LOOP_H
#define ARMD_EVENT_LOOP_H

#define EPOLL_EVENTS_MAXNUM 512

int armd_event_loop_init(void);
void armd_event_loop_run(void);
void armd_event_loop_exit(void);

#endif