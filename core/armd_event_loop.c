#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/epoll.h>

#include "armd_event_loop.h"
#include "armd_log.h"
#include "armd_comm.h"

typedef struct armd_event_src {
    int fd;
    armd_event_cb cb;
    void *arg;
} armd_event_src_t;

static int g_epfd = -1;

int armd_event_loop_init(void)
{
    g_epfd = epoll_create1(EPOLL_CLOEXEC);
    if(g_epfd < 0)
    {
        armd_log("create epoll failed, errno: %d\n", errno);
        return -1;
    }

    armd_log("event loop init success\n");
    return 0;
}

int armd_event_loop_add(int fd, uint32_t event, armd_event_cb cb, void *arg)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));

    armd_event_src_t *src = (armd_event_src_t *)malloc(sizeof(armd_event_src_t));
    if(NULL == src)
    {
        armd_log("malloc failed, errno: %d\n", errno);
        return -1;
    }

    src->fd = fd;
    src->cb = cb;
    src->arg = arg;
    ev.data.ptr = src;
    ev.events = event;
    if(epoll_ctl(g_epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        armd_log("epoll_ctl failed, errno: %d\n", errno);
        free(src);
        return -1;
    }

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

        for(int i = 0; i < nfds; i++)
        {
            armd_event_src_t *src = (armd_event_src_t *)events[i].data.ptr;
            if(NULL == src || NULL == src->cb)
            {
                armd_log("src is null\n");
                continue;
            }
            src->cb(src->fd, events[i].events, src->arg);
        }
    }

    return ;
}

void armd_event_loop_exit(void)
{
    if(g_epfd >= 0)
    {
        close(g_epfd);
    }

    armd_log("event loop exit success\n");
    return ;
}