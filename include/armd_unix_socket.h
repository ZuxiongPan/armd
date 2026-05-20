#ifndef ARMD_UNIX_SOCKET_H
#define ARMD_UNIX_SOCKET_H

#include <stdint.h>

#define ARMD_UNIX_SOCKET_BUF_SIZE 1024

int armd_unix_socket_create(const char *path);
void armd_unix_socket_cb(int fd, uint32_t event, void *arg);

#endif