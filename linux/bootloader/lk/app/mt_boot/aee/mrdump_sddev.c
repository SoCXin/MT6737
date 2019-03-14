#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#if 0
#include <platform/mmc_common_inter.h>
#endif

#include "aee.h"
#include "kdump.h"
#include "kdump_sdhc.h"

#define DEVICE_SECTOR_BYTES 512

static bool mrdump_dev_emmc_read(struct mrdump_dev *dev, uint32_t sector_addr, uint8_t *pdBuf, int32_t blockLen)
{
    part_t *fatpart = dev->handle;
    return card_dump_read(fatpart, pdBuf, (uint64_t)sector_addr * DEVICE_SECTOR_BYTES, blockLen * DEVICE_SECTOR_BYTES);
}

static bool mrdump_dev_emmc_write(struct mrdump_dev *dev, uint32_t sector_addr, uint8_t *pdBuf, int32_t blockLen)
{
    part_t *fatpart = dev->handle;
    return card_dump_write(fatpart, pdBuf, (uint64_t)sector_addr * DEVICE_SECTOR_BYTES, blockLen * DEVICE_SECTOR_BYTES);
}

struct mrdump_dev *mrdump_dev_emmc_vfat(void)
{
    struct mrdump_dev *dev = malloc(sizeof(struct mrdump_dev));

    part_t *fatpart = card_dump_init(0, "intsd");
    if (fatpart == NULL) {
        voprintf_error("No VFAT partition found!\n");
        return NULL;
    }
    dev->name = "emmc";
    dev->handle = fatpart;
    dev->read = mrdump_dev_emmc_read;
    dev->write = mrdump_dev_emmc_write;
    return dev;
}

#define EXT4_BLK_SIZE 4096

static bool mrdump_dev_ext4_read(struct mrdump_dev *dev, uint32_t lba, uint8_t *pdBuf, int32_t dataLen)
{
    part_t *ext4part = dev->handle;
    return ext4_dump_read(ext4part, pdBuf, (uint64_t)lba * EXT4_BLK_SIZE, dataLen);
}

static bool mrdump_dev_ext4_write(struct mrdump_dev *dev, uint32_t lba, uint8_t *pdBuf, int32_t dataLen)
{
    part_t *ext4part = dev->handle;
    return ext4_dump_write(ext4part, pdBuf, (uint64_t)lba * EXT4_BLK_SIZE, dataLen);
}

static part_t *mrdump_get_ext4_partition(void)
{
    part_t *ext4part;

    ext4part = card_dump_init(0, "userdata");   //mt6735, mt6752, mt6795
    if (ext4part != NULL)
        return ext4part;

    ext4part = card_dump_init(0, "USRDATA");    //mt6582, mt6592, mt8127
    if (ext4part != NULL)
        return ext4part;

    ext4part = card_dump_init(0, "PART_USER");  //mt6572
    if (ext4part != NULL)
        return ext4part;

    return NULL;
}

struct mrdump_dev *mrdump_dev_emmc_ext4(void)
{
    struct mrdump_dev *dev = malloc(sizeof(struct mrdump_dev));

    part_t *ext4part = mrdump_get_ext4_partition();
    if (ext4part == NULL) {
        voprintf_error("No EXT4 partition found!\n");
        return NULL;
    }
    dev->name = "emmc";
    dev->handle = ext4part;
    dev->read = mrdump_dev_ext4_read;
    dev->write = mrdump_dev_ext4_write;
    return dev;
}


#if 0
static bool mrdump_dev_sdcard_read(struct mrdump_dev *dev, uint32_t sector_addr, uint8_t *pdBuf, int32_t blockLen)
{
    return mmc_wrap_bread(1, sector_addr, blockLen, pdBuf) == 1;
}

static bool mrdump_dev_sdcard_write(struct mrdump_dev *dev, uint32_t sector_addr, uint8_t *pdBuf, int32_t blockLen)
{
    return mmc_wrap_bwrite(1, sector_addr, blockLen, pdBuf) == 1;
}

struct mrdump_dev *mrdump_dev_sdcard(void)
{
    struct mrdump_dev *dev = malloc(sizeof(struct mrdump_dev));
    dev->name = "sdcard";
    dev->handle = NULL;
    dev->read = mrdump_dev_sdcard_read;
    dev->write = mrdump_dev_sdcard_write;

    mmc_legacy_init(2);
    return dev;
}
#endif
