#ifndef ARMD_UEVENT_H
#define ARMD_UEVENT_H

#include <stdint.h>

/**
 * uevent message format:
    add@/devices/platform/4010000000.pcie/pci0000:00/0000:00:01.0/usb2/2-1/2-1:1.0/host0/target0:0:0/0:0:0:0/block/sda
    ACTION=add
    DEVPATH=/devices/platform/4010000000.pcie/pci0000:00/0000:00:01.0/usb2/2-1/2-1:1.0/host0/target0:0:0/0:0:0:0/block/sda
    SUBSYSTEM=block
    MAJOR=8
    MINOR=0
    DEVNAME=sda
    DEVTYPE=disk
    DISKSEQ=1
    SEQNUM=400
 */
typedef struct uevent_info {
    char action[16];
    char subsystem[32];
    char devname[16];
    char devtype[32];
} uevent_info_t;

int armd_uevent_create(void);
void armd_uevent_cb(int fd, uint32_t event, void *arg);

#endif
