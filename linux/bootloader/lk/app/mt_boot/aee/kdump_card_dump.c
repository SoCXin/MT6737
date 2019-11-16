#include "aee.h"
#include "kdump.h"

part_t *card_dump_init(int dev, const char *part_name)
{
    part_t *part = mt_part_get_partition((char *)part_name);
    if (part == NULL) {
        return NULL;
    }
#ifdef MTK_GPT_SCHEME_SUPPORT
    voprintf_info("%s offset: %lu, size: %lu Mb\n", part->info->name, part->start_sect, (part->nr_sects * BLK_SIZE) / 0x100000UL);
#else
    voprintf_info("%s offset: %lu, size: %lu Mb\n", part->name, part->startblk, (part->blknum * BLK_SIZE) / 0x100000UL);
#endif

    return part;
}

int card_dump_read(part_t *part, unsigned char* buf, uint64_t offset, uint32_t len)
{
#ifdef MTK_GPT_SCHEME_SUPPORT
    if ((offset / BLK_SIZE + len / BLK_SIZE) >= part->nr_sects) {
      voprintf_error("Read %s partition overflow, size%lu, block %lu\n", part->info->name, offset / BLK_SIZE, part->nr_sects);
        return 0;
    }
#else
    if ((offset / BLK_SIZE + len / BLK_SIZE) >= part->blknum) {
      voprintf_error("Read %s partition overflow, size%lu, block %lu\n", part->name, offset / BLK_SIZE, part->blknum);
        return 0;
    }
#endif
    if (len % BLK_SIZE != 0) {
#ifdef MTK_GPT_SCHEME_SUPPORT
        voprintf_error("Read partition size/offset not align, start %ld offset %lld elen %lu\n", part->start_sect, offset, len);
#else
        voprintf_error("Read partition size/offset not align, start %ld offset %lld elen %lu\n", part->startblk, offset, len);
#endif
        return 0;
    }
#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
#ifdef MTK_GPT_SCHEME_SUPPORT
    return emmc_read(part->part_id, ((uint64_t)part->start_sect * BLK_SIZE) + offset, buf, len) == len;
#else
    return emmc_read(part->part_id, ((uint64_t)part->startblk * BLK_SIZE) + offset, buf, len) == len;
#endif
#else
#ifdef MTK_GPT_SCHEME_SUPPORT
    return emmc_read(((uint64_t)part->start_sect * BLK_SIZE) + offset, buf, len) == len;
#else
    return emmc_read(((uint64_t)part->startblk * BLK_SIZE) + offset, buf, len) == len;
#endif
#endif
#else
    return 0;
#endif
}

bool card_dump_write(const part_t *part, const void *buf, uint64_t offset, uint32_t len)
{
#ifdef MTK_GPT_SCHEME_SUPPORT
    if ((offset / BLK_SIZE + len / BLK_SIZE) >= part->nr_sects) {
        voprintf_error("Write to %s partition overflow, size %lu,  block %lu\n", part->info->name, offset / BLK_SIZE, part->nr_sects);
        return 0;
    }
#else
    if ((offset / BLK_SIZE + len / BLK_SIZE) >= part->blknum) {
        voprintf_error("Write to %s partition overflow, size %lu,  block %lu\n", part->name, offset / BLK_SIZE, part->blknum);
        return 0;
    }
#endif
    if (len % BLK_SIZE != 0) {
        voprintf_error("Write to partition size/offset not align, offset %lld elen %lu\n", offset, len);
        return 0;
    }
#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
#ifdef MTK_GPT_SCHEME_SUPPORT
    bool retval = emmc_write(part->part_id, ((uint64_t)part->start_sect * BLK_SIZE) + offset, (unsigned char *)buf, len) == len;
#else
    bool retval = emmc_write(part->part_id, ((uint64_t)part->startblk * BLK_SIZE) + offset, (unsigned char *)buf, len) == len;
#endif
#else
#ifdef MTK_GPT_SCHEME_SUPPORT
    bool retval = emmc_write(((uint64_t)part->start_sect * BLK_SIZE) + offset, (unsigned char *)buf, len) == len;
#else
    bool retval = emmc_write(((uint64_t)part->startblk * BLK_SIZE) + offset, (unsigned char *)buf, len) == len;
#endif
#endif
#else
    bool retval = FALSE;
#endif
    if (!retval) {
        voprintf_error("EMMC write failed %d\n", len);
    }
    return retval;
}

// for ext4
int ext4_dump_read(part_t *part, unsigned char* buf, uint64_t offset, uint32_t len)
{
#ifdef MTK_GPT_SCHEME_SUPPORT
    if ((offset / BLK_SIZE + len / BLK_SIZE) >= part->nr_sects) {
      voprintf_error("GPT: Read %s partition overflow, size%lu, block %lu\n", part->info->name, offset / BLK_SIZE, part->nr_sects);
        return 0;
    }
#else
    if ((offset / BLK_SIZE + len / BLK_SIZE) >= part->blknum) {
      voprintf_error("Read %s partition overflow, size%lu, block %lu\n", part->name, offset / BLK_SIZE, part->blknum);
        return 0;
    }
#endif

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
#ifdef MTK_GPT_SCHEME_SUPPORT
    return emmc_read(part->part_id, ((uint64_t)part->start_sect * BLK_SIZE) + offset, buf, len) == len;
#else
    return emmc_read(part->part_id, ((uint64_t)part->startblk * BLK_SIZE) + offset, buf, len) == len;
#endif
#else
#ifdef MTK_GPT_SCHEME_SUPPORT
    return emmc_read(((uint64_t)part->start_sect * BLK_SIZE) + offset, buf, len) == len;
#else
    return emmc_read(((uint64_t)part->startblk * BLK_SIZE) + offset, buf, len) == len;
#endif
#endif
#else
    return 0;
#endif
}

bool ext4_dump_write(const part_t *part, const void *buf, uint64_t offset, uint32_t len)
{
#ifdef MTK_GPT_SCHEME_SUPPORT
    if ((offset / BLK_SIZE + len / BLK_SIZE) >= part->nr_sects) {
        voprintf_error("GPT: Write to %s partition overflow, size %lu,  block %lu\n", part->info->name, offset / BLK_SIZE, part->nr_sects);
        return 0;
    }
#else
    if ((offset / BLK_SIZE + len / BLK_SIZE) >= part->blknum) {
        voprintf_error("Write to %s partition overflow, size %lu,  block %lu\n", part->name, offset / BLK_SIZE, part->blknum);
        return 0;
    }
#endif

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
#ifdef MTK_GPT_SCHEME_SUPPORT
    bool retval = emmc_write(part->part_id, ((uint64_t)part->start_sect * BLK_SIZE) + offset, (unsigned char *)buf, len) == len;
#else
    bool retval = emmc_write(part->part_id, ((uint64_t)part->startblk * BLK_SIZE) + offset, (unsigned char *)buf, len) == len;
#endif
#else
#ifdef MTK_GPT_SCHEME_SUPPORT
    bool retval = emmc_write(((uint64_t)part->start_sect * BLK_SIZE) + offset, (unsigned char *)buf, len) == len;
#else
    bool retval = emmc_write(((uint64_t)part->startblk * BLK_SIZE) + offset, (unsigned char *)buf, len) == len;
#endif
#endif
#else
    bool retval = FALSE;
#endif

    if (!retval) {
        voprintf_error("EMMC write failed %d\n", len);
    }
    return retval;
}
