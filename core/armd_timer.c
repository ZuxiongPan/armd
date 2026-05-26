#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <sys/timerfd.h>
#include <sys/sysinfo.h>

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

    // 获取内存信息
    struct sysinfo info;
    if (sysinfo(&info) == 0)
    {
        unsigned long total_mem = info.totalram * info.mem_unit;
        unsigned long free_mem = info.freeram * info.mem_unit;
        unsigned long avail_mem = free_mem + info.bufferram * info.mem_unit;
        armd_log("TotalMem: %luMB, FreeMem: %luMB, AvailMem: %luMB\n", \
            total_mem / 1024 / 1024, free_mem / 1024 / 1024, avail_mem / 1024 / 1024);
    }
    else
    {
        armd_log("get meminfo failed\n");
    }

    static unsigned long long last_total = 0, last_idle = 0;
    FILE *fp = fopen("/proc/stat", "r");
    if(NULL != fp)
    {
        char buf[256];
        if(fgets(buf, sizeof(buf), fp))
        {
            unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
            int n = sscanf(buf, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
                &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
            if(n >= 4)
            {
                unsigned long long idle_all = idle + iowait;
                unsigned long long non_idle = user + nice + system + irq + softirq + steal;
                unsigned long long total = idle_all + non_idle;
                if (last_total != 0)
                {
                    unsigned long long totald = total - last_total;
                    unsigned long long idled = idle_all - last_idle;
                    armd_log("CPU Usage: %llu.%llu%%\n", (totald - idled) * 100 / totald, (totald - idled) * 100 % totald);
                }
                last_total = total;
                last_idle = idle_all;
            }
        }
        fclose(fp);
    }
    else
    {
        armd_log("open /proc/stat failed\n");
    }

    return ;
}