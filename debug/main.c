#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "armd_comm.h"

int main(int argc, const char *argv[])
{
    int fd;

    struct sockaddr_un addr;

    if(argc != 3)
    {
        printf("Usage:\n");
        printf("sendcmd [process] [message]\n");
        return -1;
    }

    const char *process = argv[1];
    const char *message = argv[2];

    if(strcmp(process, "armd") != 0)
    {
        printf("unknown process: %s\n", process);
        return -1;
    }

    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        perror("socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", ARMD_UNIX_SOCKET_PATH);

    if(sendto(fd, message, strlen(message), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("sendto");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}