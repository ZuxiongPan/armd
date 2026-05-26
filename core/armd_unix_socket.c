#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "armd_unix_socket.h"
#include "armd_comm.h"
#include "armd_log.h"

int armd_unix_socket_create(const char *path)
{
    int sockfd = -1;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        armd_log("create socket failed, errno: %d\n", errno);
        return -1;
    }

    unlink(path);
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);
    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        armd_log("bind socket failed, errno: %d\n", errno);
        close(sockfd);
        return -1;
    }

    return sockfd;
}

void armd_unix_socket_cb(int fd, uint32_t event, void *arg)
{
    char buffer[ARMD_JSON_MSG_MAX_SIZE];
    memset(buffer, 0, sizeof(buffer));

    ssize_t len = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
    if(len < 0)
    {
        armd_log("recvfrom failed, errno: %d\n", errno);
        return ;
    }

    buffer[len] = '\0';
    armd_log("receive message %s\n", buffer);

    return ;
}