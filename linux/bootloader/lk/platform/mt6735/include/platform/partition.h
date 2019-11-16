#ifndef __PARTITION_H__
#define __PARTITION_H__

#include <platform/mt_typedefs.h>
#include <platform/part.h>
#include <platform/mmc_core.h>

#define BIMG_HEADER_SZ              (0x800)
#define MKIMG_HEADER_SZ             (0x200)

#define BLK_BITS         (9)
#define BLK_SIZE         (1 << BLK_BITS)
#define PART_KERNEL     "KERNEL"
#define PART_ROOTFS     "ROOTFS"

#define PART_MAGIC          0x58881688
#define PART_MAX_COUNT      128
#define GPT_PART_SIZE       512*1024

#define FRP_NAME        "frp"

typedef union {
	struct {
		unsigned int magic;        /* partition magic */
		unsigned int dsize;        /* partition data size */
		char         name[32];     /* partition name */
		unsigned int maddr;        /* partition memory address */
	} info;
	unsigned char data[BLK_SIZE];
} part_hdr_t;


#define PART_META_INFO_NAMELEN  64
#define PART_META_INFO_UUIDLEN  16

struct part_meta_info {
	u8 name[PART_META_INFO_NAMELEN];
	u8 uuid[PART_META_INFO_UUIDLEN];
};

typedef struct {
	unsigned long  start_sect;
	unsigned long  nr_sects;
	unsigned int part_id;
	char *name;
	struct part_meta_info *info;
} part_t;

struct part_name_map {
	char fb_name[32];   /*partition name used by fastboot*/
	char r_name[32];    /*real partition name*/
	char *partition_type;   /*partition_type*/
	int partition_idx;  /*partition index*/
	int is_support_erase;   /*partition support erase in fastboot*/
	int is_support_dl;  /*partition support download in fastboot*/
};

typedef struct part_dev part_dev_t;

struct part_dev {
	int init;
	int id;
	block_dev_desc_t *blkdev;
	int (*init_dev) (int id);
#ifdef MTK_EMMC_SUPPORT
	int (*read)  (part_dev_t *dev, u64 src, uchar *dst, int size, unsigned int part_id);
	int (*write) (part_dev_t *dev, uchar *src, u64 dst, int size, unsigned int part_id);
#else
	int (*read)  (part_dev_t *dev, ulong src, uchar *dst, int size);
	int (*write) (part_dev_t *dev, uchar *src, ulong dst, int size);
#endif
};

enum {
	RAW_DATA_IMG,
	YFFS2_IMG,
	UBIFS_IMG,
	EXT4_IMG,
	FAT_IMG,
	UNKOWN_IMG,
};

extern part_t *get_part(char *name);
extern void put_part(part_t *part);

extern part_t* mt_part_get_partition(char *name);
extern struct part_name_map g_part_name_map[PART_MAX_COUNT];
extern int mt_part_register_device(part_dev_t *dev);
extern part_dev_t* mt_part_get_device(void);
extern void mt_part_init(unsigned long totalblks);
extern void mt_part_dump(void);
extern int partition_get_index(const char * name);
extern u32 partition_get_region(int index);
extern u64 partition_get_offset(int index);
extern u64 partition_get_size(int index);
extern int partition_get_type(int index, char **p_type);
extern int partition_get_name(int index, char **p_name);
extern int is_support_erase(int index);
extern int is_support_flash(int index);
extern u64 emmc_write(u32 region, u64 offset, void *data, u64 size);
extern u64 emmc_read(u32 region, u64 offset, void *data, u64 size);
extern int emmc_erase(u32 region, u64 offset, u64 size);

#endif /* __PARTITION_H__ */

