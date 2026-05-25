#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/netlink.h>

#include "armd_log.h"
#include "armd_uevent.h"
#include "block_mgr.h"

#define UEVENT_BUFFER_SIZE 4096

static void handle_kernel_uevent(const uevent_info_t *uevent_info)
{
    int inner_ret = 0;

    if(strncmp(uevent_info->subsystem, "block", 5) == 0)
    {
        inner_ret = block_device_handler(uevent_info);
        if(inner_ret < 0)
        {
            armd_log("block_device_handler failed\n");
        }
    }

    return ;
}

int armd_uevent_create(void)
{
    int sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(sock < 0)
    {
        armd_log("create netlink socket failed, errno: %d\n", errno);
        return -1;
    }

    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 1;

    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        armd_log("bind netlink socket failed, errno: %d\n", errno);
        close(sock);
        return -1;
    }

    return sock;
}

// drive_add 0 file=/home/pzx/armd_env/usb.img,if=none,id=ud,format=raw
// device_add usb-storage,bus=xhci.0,drive=ud,id=us
void armd_uevent_cb(int fd, uint32_t event, void *arg)
{
    char buf[UEVENT_BUFFER_SIZE] = { 0 };
    uevent_info_t uevent_info = { 0 };
    memset(&uevent_info, 0, sizeof(uevent_info));

    ssize_t len = recv(fd, buf, sizeof(buf) - 1, 0);
    if(len <= 0)
    {
        armd_log("recv netlink uevent failed, errno: %d\n", errno);
        return ;
    }

    buf[len] = '\0';
    armd_log("recv uevent string size %d\n", (int)len);
    const char *msg_ptr = buf;
    while(msg_ptr < buf + len)
    {
        if(strncmp(msg_ptr, "ACTION=", 7) == 0)
        {
            strncpy(uevent_info.action, msg_ptr + 7, sizeof(uevent_info.action) - 1);
        }
        else if(strncmp(msg_ptr, "SUBSYSTEM=", 10) == 0)
        {
            strncpy(uevent_info.subsystem, msg_ptr + 10, sizeof(uevent_info.subsystem) - 1);
        }
        else if(strncmp(msg_ptr, "DEVNAME=", 8) == 0)
        {
            strncpy(uevent_info.devname, msg_ptr + 8, sizeof(uevent_info.devname) - 1);
        }
        else if(strncmp(msg_ptr, "DEVTYPE=", 8) == 0)
        {
            strncpy(uevent_info.devtype, msg_ptr + 8, sizeof(uevent_info.devtype) - 1);
        }

        msg_ptr += strlen(msg_ptr) + 1;
    }

    armd_log("uevent:action [%s], subsystem [%s], devname [%s], devtype [%s]\n",\
        uevent_info.action, uevent_info.subsystem, uevent_info.devname, uevent_info.devtype);

    handle_kernel_uevent(&uevent_info);

    return ;
}
