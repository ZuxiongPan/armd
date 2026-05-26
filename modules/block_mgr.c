#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/mount.h>

#include "armd_log.h"
#include "block_mgr.h"

#define MAGIC_BUFFER_SIZE 4096

const static char *ext_mounted_name = "ext4";
const static char *ntfs_mounted_name = "ntfs";
const static char *vfat_mounted_name = "vfat";
const static char *invalid_mounted_name = "unknown";

uint32_t get_filesystem_type(const char *devpath)
{
    if(NULL == devpath)
    {
        armd_log("invalid device\n");
        return FS_UNKNOWN;
    }

    uint32_t ret = FS_UNKNOWN;
    uint8_t magic_buf[MAGIC_BUFFER_SIZE] = { 0 };
    int fd = open(devpath, O_RDONLY | O_CLOEXEC);
    if(fd < 0)
    {
        armd_log("open %s failed, errno %d\n", devpath, errno);
        return ret;
    }

    lseek(fd, 0, SEEK_SET);
    ssize_t rd_num = read(fd, magic_buf, MAGIC_BUFFER_SIZE);
    close(fd);
    if(rd_num < 0)
    {
        armd_log("read superblock failed\n");
        return ret;
    }

    if((0x53 == magic_buf[MAGIC_EXT_INDEX]) && (0xef == magic_buf[MAGIC_EXT_INDEX + 1]))
    {
        armd_log("this is an ext filesystem\n");
        ret = FS_EXT;
    }
    else if((0x4e == magic_buf[MAGIC_NTFS_INDEX]) && (0x54 == magic_buf[MAGIC_NTFS_INDEX + 1])
        && (0x46 == magic_buf[MAGIC_NTFS_INDEX + 2]) && (0x53 == magic_buf[MAGIC_NTFS_INDEX +3]))
    {
        armd_log("this is an ntfs filesystem\n");
        ret = FS_NTFS;
    }
    else if((0x46 == magic_buf[MAGIC_FAT32_INDEX]) && (0x41 == magic_buf[MAGIC_FAT32_INDEX + 1])
        && (0x54 == magic_buf[MAGIC_FAT32_INDEX + 2]))
    {
        armd_log("this is an vfat filesystem\n");
        ret = FS_VFAT;
    }

    return (ret & FS_SUPPORTED ? ret : FS_UNKNOWN);
}

const char* get_mount_filesystem_string(uint32_t fs_type)
{
    const char *ret = invalid_mounted_name;

    switch(fs_type)
    {
        case FS_EXT:
            ret = ext_mounted_name;
            break;
        case FS_NTFS:
            ret = ntfs_mounted_name;
            break;
        case FS_VFAT:
            ret = vfat_mounted_name;
            break;
        default:
            ret = invalid_mounted_name;
            break;
    }

    return ret;
}

int block_device_handler(const uevent_info_t *uevent_info)
{
    if(NULL == uevent_info)
    {
        armd_log("invalid uevent_info\n");
        return -1;
    }

    if(strncmp(uevent_info->action, "add", 3) == 0)
    {
        char devpath[64] = { 0 };
        snprintf(devpath, sizeof(devpath), "/dev/%s", uevent_info->devname);
        uint32_t fs_type = get_filesystem_type(devpath);
        if(FS_UNKNOWN == fs_type)
        {
            armd_log("get_filesystem_type failed, fs_type: %d\n", fs_type);
            return -1;
        }
        const char *fs_name = get_mount_filesystem_string(fs_type);
        
        return mount(devpath, "/mnt", fs_name, 0, NULL);
    }
    else if(strncmp(uevent_info->action, "remove", 6) == 0)
    {
        return umount("/mnt");
    }

    return 0;
}