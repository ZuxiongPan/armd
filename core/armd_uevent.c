#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <linux/netlink.h>

#include "armd_log.h"
#include "armd_uevent.h"

#define UEVENT_BUFFER_SIZE 16384

static bool is_usb_storage_event(char *buf, ssize_t len, char **out_devname)
{
    char *p = buf;
    char *end = buf + len;
    const char *action = NULL;
    const char *subsystem = NULL;
    const char *devname = NULL;
    const char *id_bus = NULL;

    while(p < end && *p) {
        if(strncmp(p, "ACTION=", 7) == 0) action = p + 7;
        else if(strncmp(p, "SUBSYSTEM=", 10) == 0) subsystem = p + 10;
        else if(strncmp(p, "DEVNAME=", 8) == 0) devname = p + 8;
        else if(strncmp(p, "ID_BUS=", 7) == 0) id_bus = p + 7;
        p += strlen(p) + 1;
    }

    if(!action || strcmp(action, "add") != 0) return false;
    if(!subsystem) return false;

    // Accept block devices where bus is usb or device path contains "usb"
    if(strcmp(subsystem, "block") == 0) {
        if(id_bus && strcmp(id_bus, "usb") == 0) {
            if(devname) {
                *out_devname = strdup(devname);
                return true;
            }
        }
        // fallback: try to find DEVPATH or any usb hint in buffer
        if(strstr(buf, "usb")) {
            if(devname) {
                *out_devname = strdup(devname);
                return true;
            }
        }
    }

    return false;
}

static int ensure_mount_point(char *mp, size_t buflen)
{
    // Try /mnt/usb, /mnt/usb1, ...
    for(int i = 0; i < 8; i++) {
        if(i == 0) snprintf(mp, buflen, "/mnt/usb");
        else snprintf(mp, buflen, "/mnt/usb%d", i);

        struct stat st;
        if(stat(mp, &st) == 0) {
            if(S_ISDIR(st.st_mode)) return 0;
            continue;
        }

        if(mkdir(mp, 0755) == 0) return 0;
    }
    return -1;
}

static bool try_mount_with_fstypes(const char *devpath, const char *mp)
{
    const char *fstypes[] = {"vfat", "exfat", "ntfs", "ext4", "ext3", "ext2", "fuseblk", NULL};
    for(const char **t = fstypes; *t; t++) {
        if(mount(devpath, mp, *t, MS_RDONLY, NULL) == 0) {
            armd_log("mounted %s on %s type=%s\n", devpath, mp, *t);
            return true;
        }
    }
    // Last resort: try autodetect by passing NULL
    if(mount(devpath, mp, NULL, MS_RDONLY, NULL) == 0) {
        armd_log("mounted %s on %s type=auto\n", devpath, mp);
        return true;
    }
    return false;
}

int armd_uevent_create(void)
{
    int sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(sock < 0) {
        armd_log("create netlink socket failed, errno: %d\n", errno);
        return -1;
    }

    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 1; // listen to group 1

    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        armd_log("bind netlink socket failed, errno: %d\n", errno);
        close(sock);
        return -1;
    }

    return sock;
}

void armd_uevent_cb(int fd, uint32_t event, void *arg)
{
    char buf[UEVENT_BUFFER_SIZE];
    ssize_t len = recv(fd, buf, sizeof(buf) - 1, 0);
    if(len <= 0) return;
    buf[len] = '\0';

    char *devname = NULL;
    if(!is_usb_storage_event(buf, len, &devname)) {
        if(devname) free(devname);
        return;
    }

    char devpath[256];
    snprintf(devpath, sizeof(devpath), "/dev/%s", devname);
    free(devname);

    armd_log("usb storage device detected: %s\n", devpath);

    char mp[256];
    if(ensure_mount_point(mp, sizeof(mp)) < 0) {
        armd_log("failed to prepare mount point\n");
        return;
    }

    if(try_mount_with_fstypes(devpath, mp)) {
        armd_log("device %s mounted at %s\n", devpath, mp);
    } else {
        armd_log("failed to mount %s\n", devpath);
    }
}
