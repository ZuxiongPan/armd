#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <sys/timerfd.h>

#include "armd_timer.h"
#include "armd_log.h"

int armd_timer_create(int interval_sec)
{
    int tfd = -1;
    struct itimerspec its;
    memset(&its, 0, sizeof(its));

    tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(tfd < 0)
    {
        armd_log("create timerfd failed, errno: %d\n", errno);
        return -1;
    }

    its.it_interval.tv_sec = interval_sec;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = interval_sec;
    its.it_value.tv_nsec = 0;

    if(timerfd_settime(tfd, TFD_TIMER_ABSTIME, &its, NULL) < 0)
    {
        armd_log("set timerfd time failed, errno: %d\n", errno);
        close(tfd);
        return -1;
    }

    return tfd;
}

void armd_timer_cb(int fd, uint32_t event, void *arg)
{
    uint64_t value = 0;

    ssize_t ret = read(fd, &value, sizeof(value));
    (void)ret;
    //armd_log("timerfd %d expired, value: %lu\n", fd, value);

    return ;
}