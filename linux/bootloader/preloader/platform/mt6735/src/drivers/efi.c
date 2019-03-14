#include "typedefs.h"
#include "print.h"
#include "blkdev.h"
#include "dram_buffer.h"
#include "partition.h"

typedef struct {
    u8 b[16];
} __attribute__((packed)) efi_guid_t;


typedef struct {
    u64 signature;
    u32 revision;
    u32 header_size;
    u32 header_crc32;
    u32 reserved;
    u64 my_lba;
    u64 alternate_lba;
    u64 first_usable_lba;
    u64 last_usable_lba;
    efi_guid_t disk_guid;
    u64 partition_entry_lba;
    u32 num_partition_entries;
    u32 sizeof_partition_entry;
    u32 partition_entry_array_crc32;
} __attribute__((packed)) gpt_header;

#define GPT_ENTRY_NAME_LEN  (72 / sizeof(u16))

typedef struct {
    efi_guid_t partition_type_guid;
    efi_guid_t unique_partition_guid;
    u64 starting_lba;
    u64 ending_lba;
    u64 attributes;
    u16 partition_name[GPT_ENTRY_NAME_LEN];
} __attribute__((packed))gpt_entry;

static part_t *part_ptr = NULL;

/* 
 ********** Definition of Debug Macro **********
 */
#define TAG "[GPT_PL]"

#define LEVEL_ERR   (0x0001)
#define LEVEL_INFO  (0x0004)

#define DEBUG_LEVEL (LEVEL_ERR | LEVEL_INFO)   

#define efi_err(fmt, args...)   \
do {    \
    if (DEBUG_LEVEL & LEVEL_ERR) {  \
        printf(fmt, ##args); \
    }   \
} while (0)

#define efi_info(fmt, args...)  \
do {    \
    if (DEBUG_LEVEL & LEVEL_INFO) {  \
        printf(fmt, ##args);    \
    }   \
} while (0)


/* 
 ********** Definition of GPT buffer **********
 */
#define pgpt_header g_dram_buf->pgpt_header_buf
#define pgpt_entries g_dram_buf->pgpt_entries_buf

#define sgpt_header g_dram_buf->sgpt_header_buf
#define sgpt_entries g_dram_buf->sgpt_entries_buf

/* 
 ********** Definition of CRC32 Calculation **********
 */
static int crc32_table_init = 0;
#define crc32_table g_dram_buf->crc32_table

static void init_crc32_table(void)
{
    int i, j;
    u32 crc;

    if (crc32_table_init) {
        return;
    } 

    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_init = 1;
}

static u32 crc32(u32 crc, u8 *p, u32 len)
{
    init_crc32_table();
    
    while (len--) {
        crc ^= *p++;
        crc = (crc >> 8) ^ crc32_table[crc & 255];
    }

    return crc;
} 

static u32 efi_crc32(u8 *p, u32 len)
{
    return (crc32(~0L, p, len) ^ ~0L);
}


static void w2s(u8 *dst, int dst_max, u16 *src, int src_max)
{
    int i = 0;
    int len = min(src_max, dst_max - 1);

    while (i < len) {
        if (!src[i]) {
            break;
        }
        dst[i] = src[i] & 0xFF;
        i++;
    }

    dst[i] = 0;

    return;
}

extern u64 g_emmc_user_size;

static u64 last_lba(u32 part_id)
{
    /* Only support USER region now */
    return g_emmc_user_size / 512 - 1;
}

static int read_data(u8 *buf, u32 part_id, u64 lba, u64 size)
{
    int err;
    blkdev_t *dev;

    dev = blkdev_get(BOOTDEV_SDMMC);
    if (!dev) {
        efi_err("%sread data, err(no dev)\n", TAG);
        return 1;
    }

    err = dev->bread(dev, (u32)lba, (u32)(size / 512), buf, part_id);
    if (err) {
        efi_err("%sread data, err(%d)\n", TAG, err);
        return err;
    }

    return 0;
}


#define GPT_HEADER_SIGNATURE    0x5452415020494645ULL


static int parse_gpt_header(u32 part_id, u64 header_lba, u8 *header_buf, u8 *entries_buf)
{
    int i;

    int err;
    u32 calc_crc, orig_header_crc;
    u64 entries_real_size, entries_read_size;

    gpt_header *header = (gpt_header *)header_buf;
    gpt_entry *entries = (gpt_entry *)entries_buf;

    err = read_data(header_buf, part_id, header_lba, 512);
    if (err) {
        efi_err("%sread header(part_id=%d,lba=%llx), err(%d)\n", 
                TAG, part_id, header_lba, err);
        return err;
    }

    if (header->signature != GPT_HEADER_SIGNATURE) {
        efi_err("%scheck header, err(signature 0x%llx!=0x%llx)\n",
                TAG, header->signature, GPT_HEADER_SIGNATURE);
        return 1;
    }

    orig_header_crc = header->header_crc32;
    header->header_crc32 = 0;
    calc_crc = efi_crc32((u8 *)header, header->header_size);

    if (orig_header_crc != calc_crc) {
        efi_err("%scheck header, err(crc 0x%x!=0x%x(calc))\n",
                TAG, orig_header_crc, calc_crc);
        return 1;
    }
    
    header->header_crc32 = orig_header_crc;
    
    if (header->my_lba != header_lba) {
        efi_err("%scheck header, err(my_lba 0x%llx!=0x%llx)\n",
                TAG, header->my_lba, header_lba);
        return 1;
    }

    entries_real_size = (u64)header->num_partition_entries * header->sizeof_partition_entry;
    entries_read_size = (u64)((header->num_partition_entries + 3) / 4) * 512; 

    err = read_data(entries_buf, part_id, header->partition_entry_lba, entries_read_size);
    if (err) {
        efi_err("%sread entries(part_id=%d,lba=%llx), err(%d)\n", 
                TAG, part_id, header->partition_entry_lba, err);
        return err;
    }

    calc_crc = efi_crc32((u8 *)entries, (u32)entries_real_size);

    if (header->partition_entry_array_crc32 != calc_crc) {
        efi_err("%scheck header, err(entries crc 0x%x!=0x%x(calc))\n",
                TAG, header->partition_entry_array_crc32, calc_crc);
        return 1;
    }

    for (i = 0; i < header->num_partition_entries; i++) {
        part_ptr[i].info = &g_dram_buf->meta_info[i];
        if ((entries[i].partition_name[0] & 0xFF00) == 0) {
            w2s(part_ptr[i].info->name, PART_META_INFO_NAMELEN, entries[i].partition_name, GPT_ENTRY_NAME_LEN);
        } else {
            memcpy(part_ptr[i].info->name, entries[i].partition_name, 64);
        }
        part_ptr[i].start_sect = (unsigned long)entries[i].starting_lba;
        part_ptr[i].nr_sects = (unsigned long)(entries[i].ending_lba - entries[i].starting_lba + 1);
        part_ptr[i].part_id = EMMC_PART_USER;
        efi_info("%s[%d]name=%s, part_id=%d, start_sect=0x%x, nr_sects=0x%x\n", TAG, i, part_ptr[i].info->name,
                part_ptr[i].part_id, part_ptr[i].start_sect, part_ptr[i].nr_sects);
    }

    return 0;
}


int read_gpt(part_t *part)
{
    int err;
    u64 lba;
    u32 part_id = EMMC_PART_USER;

    part_ptr = part;
    
    efi_info("%sParsing Primary GPT now...\n", TAG);
    err = parse_gpt_header(part_id, 1, pgpt_header, pgpt_entries);
    if (!err) {
        goto find;
    }

    efi_info("%sParsing Secondary GPT now...\n", TAG);
    lba = last_lba(part_id);
    err = parse_gpt_header(part_id, lba, sgpt_header, sgpt_entries);
    if (!err) {
        goto find;
    }
    
    efi_err("%sFailure to find valid GPT.\n", TAG);
    return err;

find:
    efi_info("%sSuccess to find valid GPT.\n", TAG);
    return 0;
}
