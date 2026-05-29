#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>

#include <sys/timerfd.h>
#include <sys/sysinfo.h>

#include "armd_timer.h"
#include "armd_log.h"

typedef struct armd_timer_node {
    int timer_id;
    int interval_ms;
    uint64_t expire_ms;
    int next;
    bool alive; // delay delete timer
    armd_timer_callback cb;
    void *cb_arg;
} armd_timer_node_t;

typedef struct armd_timer_list {
    int tfd;
    int used_head;
    int idle_head;
    int size;
    int capacity;
    armd_timer_node_t *slist;  // static list
} armd_timer_list_t;

static armd_timer_list_t g_timer_list;

static inline uint64_t get_monotonic_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void timer_update(void)
{
    uint64_t now_ms = get_monotonic_now_ms();

    if(g_timer_list.used_head == -1)
    {
        struct itimerspec disable_its = {0};
        timerfd_settime(g_timer_list.tfd, 0, &disable_its, NULL);
        return ;
    }

    while(g_timer_list.used_head != -1 && g_timer_list.slist[g_timer_list.used_head].expire_ms <= now_ms)
    {
        int timer_id = g_timer_list.used_head;
        if(g_timer_list.slist[timer_id].alive)
        {
            g_timer_list.slist[timer_id].cb(g_timer_list.slist[timer_id].cb_arg);
        }
        else
        {
            g_timer_list.used_head = g_timer_list.slist[timer_id].next;
            g_timer_list.slist[timer_id].next = g_timer_list.idle_head;
            g_timer_list.idle_head = timer_id;
            g_timer_list.size--;
        }
    }
}

int armd_timer_list_create(void)
{
    memset(&g_timer_list, 0, sizeof(g_timer_list));
    g_timer_list.capacity = ARMD_TIMER_INITIAL_CAPACITY;
    g_timer_list.tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(g_timer_list.tfd < 0)
    {
        armd_log("create timerfd failed, errno: %d\n", errno);
        return -1;
    }

    g_timer_list.slist = (armd_timer_node_t *)malloc(sizeof(armd_timer_node_t) * g_timer_list.capacity);
    if(g_timer_list.slist == NULL)
    {
        armd_log("malloc timer list failed\n");
        close(g_timer_list.tfd);
        g_timer_list.tfd = -1;
        return -1;
    }

    for(int i = 0; i < g_timer_list.capacity; i++)
    {
        g_timer_list.slist[i].next = i + 1;
    }
    g_timer_list.slist[g_timer_list.capacity - 1].next = -1;
    g_timer_list.idle_head = 0;
    g_timer_list.used_head = -1;
    g_timer_list.size = 0;

    return g_timer_list.tfd;
}

int armd_timer_add(uint64_t delay_ms, uint64_t interval_ms, armd_timer_callback cb, void *cb_arg)
{
    if(g_timer_list.size >= g_timer_list.capacity)
    {
        g_timer_list.capacity *= 2;
        g_timer_list.slist = realloc(g_timer_list.slist, sizeof(armd_timer_node_t) * g_timer_list.capacity);
        if(g_timer_list.slist == NULL)
        {
            armd_log("realloc timer list failed, new timer add failed\n");
            return -1;
        }

        for(int i = g_timer_list.size; i < g_timer_list.capacity; i++)
        {
            g_timer_list.slist[i].next = i + 1;
        }
        g_timer_list.slist[g_timer_list.capacity - 1].next = -1;
    }

    int newid = g_timer_list.idle_head;
    g_timer_list.idle_head = g_timer_list.slist[newid].next;
    g_timer_list.slist[newid].timer_id = newid;
    g_timer_list.slist[newid].interval_ms = interval_ms;
    g_timer_list.slist[newid].expire_ms = get_monotonic_now_ms() + delay_ms;
    g_timer_list.slist[newid].next = -1;
    g_timer_list.slist[newid].alive = true;
    g_timer_list.slist[newid].cb = cb;
    g_timer_list.slist[newid].cb_arg = cb_arg;
    
    // insert to static list
    int prev = -1, curr = g_timer_list.used_head;
    while(curr != -1 && g_timer_list.slist[curr].expire_ms <= g_timer_list.slist[newid].expire_ms)
    {
        prev = curr;
        curr = g_timer_list.slist[curr].next;
    }
    if(prev == -1)
    {
        g_timer_list.slist[newid].next = g_timer_list.used_head;
        g_timer_list.used_head = newid;
    }
    else
    {
        g_timer_list.slist[prev].next = newid;
        g_timer_list.slist[newid].next = curr;
    }
    g_timer_list.size++;

    return newid;
}

void armd_timer_delete(int timer_id)
{
    if(timer_id < 0 || timer_id >= g_timer_list.capacity)
    {
        armd_log("invalid timer id: %d\n", timer_id);
        return ;
    }

    g_timer_list.slist[timer_id].alive = false;
    return ;
}

void armd_timer_cb(int fd, uint32_t event, void *arg)
{
    uint64_t value = 0;
    ssize_t ret = read(fd, &value, sizeof(value));
    (void)ret;
}

/*
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