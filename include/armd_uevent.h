#ifndef ARMD_UEVENT_H
#define ARMD_UEVENT_H

#include <stdint.h>

int armd_uevent_create(void);
void armd_uevent_cb(int fd, uint32_t event, void *arg);

#endif
