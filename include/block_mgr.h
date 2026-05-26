#ifndef BLOCK_MGR_H
#define BLOCK_MGR_H

#define FS_UNKNOWN 0x0u
#define FS_EXT 0x01u
#define FS_NTFS 0x02u
#define FS_VFAT 0x04u

#define FS_SUPPORTED (FS_EXT)

#define MAGIC_EXT_INDEX 0x438  // 0x53 0xef
#define MAGIC_NTFS_INDEX 0x3   // "NTFS" 0x4e 0x54 0x46 0x53
#define MAGIC_FAT32_INDEX 0x52 // "FAT32" 0x46 0x41 0x54

#include "armd_uevent.h"

int block_device_handler(const uevent_info_t *uevent_info);

#endif