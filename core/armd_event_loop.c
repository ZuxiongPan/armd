#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "armd_event_loop.h"
#include "armd_log.h"
#include "armd_comm.h"

static int g_epfd = -1;
static int g_sockfd = -1;

static int create_unix_socket(void)
{
    struct sockaddr_un addr;
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        armd_log("create unix socket failed, errno: %d\n", errno);
        return -1;
    }

    unlink(ARMD_SOCKET_PATH);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", ARMD_SOCKET_PATH);
    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        armd_log("bind unix socket failed, errno: %d\n", errno);
        close(fd);
        return -1;
    }

    return fd;
}

static void socket_event_handler(void)
{
    char buffer[1024] = { 0 };

    ssize_t len = recvfrom(g_sockfd, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
    if(len < 0)
    {
        armd_log("recv failed, errno: %d\n", errno);
        return ;
    }

    buffer[len] = '\0';
    armd_log("recv: %s\n", buffer);

    return ;
}

int armd_event_loop_init(void)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));

    g_epfd = epoll_create1(EPOLL_CLOEXEC);
    if(g_epfd < 0)
    {
        armd_log("create epoll failed, errno: %d\n", errno);
        return -1;
    }

    g_sockfd = create_unix_socket();
    if(g_sockfd < 0)
    {
        armd_log("create unix socket failed, errno: %d\n", errno);
        return -1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = g_sockfd;
    if(epoll_ctl(g_epfd, EPOLL_CTL_ADD, g_sockfd, &ev) < 0)
    {
        armd_log("epoll_ctl failed, errno: %d\n", errno);
        return -1;
    }

    armd_log("event loop init success\n");

    return 0;
}

void armd_event_loop_run(void)
{
    struct epoll_event events[EPOLL_EVENTS_MAXNUM];
    memset(events, 0, sizeof(events));

    armd_log("event loop is running\n");

    while(true)
    {
        int nfds = epoll_wait(g_epfd, events, EPOLL_EVENTS_MAXNUM, -1);
        if(nfds < 0)
        {
            if(EINTR == errno)
            {
                armd_log("epoll_wait failed, errno: %d\n", errno);
                continue;
            }
            armd_log("epoll_wait failed, errno: %d\n", errno);
            break;
        }
        else
        {
            for(int i = 0; i < nfds; i++)
            {
                if(events[i].data.fd == g_sockfd)
                {
                    socket_event_handler();
                }
            }
        }
    }

    return ;
}

void armd_event_loop_exit(void)
{
    if(g_sockfd >= 0)
    {
        close(g_sockfd);
        unlink(ARMD_SOCKET_PATH);
    }

    if(g_epfd >= 0)
    {
        close(g_epfd);

    }

    armd_log("event loop exit success\n");
    return ;
}