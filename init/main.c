#include <errno.h>
#include <unistd.h>

#include <sys/epoll.h>

#include "armd_event_loop.h"
#include "armd_log.h"
#include "armd_timer.h"
#include "armd_unix_socket.h"
#include "armd_comm.h"
#include "armd_uevent.h"

static int timerfd = -1;
static int sockfd = -1;
static int ueventfd = -1;

int main(int argc, const char *argv[])
{
    armd_log("event loop start ...\n");

    armd_event_loop_init();

    sockfd = armd_unix_socket_create(ARMD_UNIX_SOCKET_PATH);
    if(sockfd < 0)
    {
        armd_log("create unix socket failed, errno: %d\n", errno);
        return -1;
    }
    if(armd_event_loop_add(sockfd, EPOLLIN, armd_unix_socket_cb, NULL) < 0)
    {
        armd_log("add unix socket to event loop failed, errno: %d\n", errno);
        close(sockfd);
        return -1;
    }

    timerfd = armd_timer_create(5);
    if(timerfd < 0)
    {
        armd_log("create timerfd failed, errno: %d\n", errno);
        close(sockfd);
        return -1;
    }
    if(armd_event_loop_add(timerfd, EPOLLIN, armd_timer_cb, NULL) < 0)
    {
        armd_log("add timerfd to event loop failed, errno: %d\n", errno);
        close(sockfd);
        close(timerfd);
        return -1;
    }

    ueventfd = armd_uevent_create();
    if(ueventfd < 0)
    {
        armd_log("create uevent socket failed, errno: %d\n", errno);
    }
    else if(armd_event_loop_add(ueventfd, EPOLLIN, armd_uevent_cb, NULL) < 0)
    {
        armd_log("add uevent socket to event loop failed, errno: %d\n", errno);
        close(ueventfd);
        ueventfd = -1;
    }

    armd_event_loop_run();

    armd_event_loop_exit();

    close(sockfd);
    if(ueventfd >= 0) close(ueventfd);
    close(timerfd);
    return 0;
}