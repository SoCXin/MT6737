#if !defined(__KDUMP_H__)
#define __KDUMP_H__

#include <stdint.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <platform/mt_reg_base.h>
#include <dev/mrdump.h>

#define KDUMP_CONFIG_NONE              0
#define KDUMP_CONFIG_USB               1
#define KDUMP_CONFIG_MSDC_FORMAT_WRITE 2
#define KDUMP_CONFIG_MSDC_WRITE        6

#define MRDUMP_DEV_NULL 0
#define MRDUMP_DEV_SDCARD 1
#define MRDUMP_DEV_EMMC 2

// Added for ext4,  2014/09/22
#define MRDUMP_FS_NULL 0
#define MRDUMP_FS_VFAT 1
#define MRDUMP_FS_EXT4 2

#define MRDUMP_GO_DUMP "MRDUMP04"

// for DTS
#ifdef CFG_DTB_EARLY_LOADER_SUPPORT

#include <libfdt.h>
extern void *g_fdt;

struct mrdump_reserve_args {
	uint32_t hi_addr;
	uint32_t lo_addr;
	uint32_t hi_size;
	uint32_t lo_size;
};

#endif

// for ext4, InfoLBA (header), 2014/10/03
typedef enum {
	EXT4_1ST_LBA,
	EXT4_2ND_LBA,
	EXT4_CORE_DUMP_SIZE,
	EXT4_USER_FILESIZE,
	EXT4_INFOBLOCK_CRC,
	EXT4_LBA_INFO_NUM
} MRDUMP_LBA_INFO;

#define KZIP_ENTRY_MAX 8
#define LOCALHEADERMAGIC 0x04034b50UL
#define CENTRALHEADERMAGIC 0x02014b50UL
#define ENDOFCENTRALDIRMAGIC 0x06054b50UL

#define KDUMP_CORE_HEADER_SIZE 2 * 4096

struct kzip_entry {
	char *filename;
	int level;
	uint64_t localheader_offset;
	uint32_t comp_size;
	uint32_t uncomp_size;
	uint32_t crc32;
};

struct kzip_file {
	uint32_t reported_size;
	uint32_t wdk_kick_size;
	uint32_t current_size;

	uint32_t entries_num;
	struct kzip_entry zentries[KZIP_ENTRY_MAX];
	void *handle;

	int (*write_cb)(void *handle, void *buf, int size);
};

struct kzip_memlist {
	void *address;
	uint32_t size;
};

#define MRDUMP_CB_ADDR (DRAM_PHY_ADDR + 0x1F00000)
#define MRDUMP_CB_SIZE 0x1000

#define MRDUMP_CPU_MAX 16

typedef uint32_t arm32_gregset_t[18];
typedef uint64_t arm64_gregset_t[34];

struct mrdump_crash_record {
	int reboot_mode;

	char msg[128];
	char backtrace[512];

	uint32_t fault_cpu;

	union {
		arm32_gregset_t arm32_regs;
		arm64_gregset_t arm64_regs;
	} cpu_regs[MRDUMP_CPU_MAX];
};

struct mrdump_machdesc {
	uint32_t crc;

	uint32_t output_device;

	uint32_t nr_cpus;

	uint64_t page_offset;
	uint64_t high_memory;

	uint64_t vmalloc_start;
	uint64_t vmalloc_end;

	uint64_t modules_start;
	uint64_t modules_end;

	uint64_t phys_offset;
	uint64_t master_page_table;

	// don't affect the original data structure...
	uint32_t output_fstype;
	uint32_t output_lbaooo;
};

struct __attribute__((__packed__)) mrdump_cblock_result {
	char sig[9];
	char status[128];
	char log_buf[2048];
};

struct mrdump_control_block {
	char sig[8];

	struct mrdump_machdesc machdesc;
	struct mrdump_crash_record crash_record;
};

struct kzip_file *kzip_open(void *handle, int (*write_cb)(void *handle, void *p, int size));
bool kzip_add_file(struct kzip_file *zf, const struct kzip_memlist *memlist, const char *zfilename);
bool kzip_close(struct kzip_file *zf);

int kdump_emmc_output(const struct mrdump_control_block *kparams, uint32_t total_dump_size);
int kdump_sdcard_output(const struct mrdump_control_block *kparams, uint32_t total_dump_size);
int kdump_null_output(const struct mrdump_control_block *kparams, uint32_t total_dump_size);

part_t *card_dump_init(int dev, const char *name);
int card_dump_read(part_t *part, unsigned char* buf, uint64_t offset, uint32_t len);
bool card_dump_write(const part_t *part, const void *buf, uint64_t offset, uint32_t len);
int ext4_dump_read(part_t *part, unsigned char* buf, uint64_t offset, uint32_t len);
bool ext4_dump_write(const part_t *part, const void *buf, uint64_t offset, uint32_t len);

void *kdump_core_header_init(const struct mrdump_control_block *kparams, uint64_t kmem_address, uint64_t kmem_size);
void *kdump_core32_header_init(const struct mrdump_control_block *kparams, uint64_t kmem_address, uint64_t kmem_size);
void *kdump_core64_header_init(const struct mrdump_control_block *kparams, uint64_t kmem_address, uint64_t kmem_size);

#endif
