#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <platform/partition.h>
#include <platform/mt_typedefs.h>
#include <platform/boot_mode.h>
#include <platform/mt_reg_base.h>
#include <platform/bootimg.h>
#include <platform/errno.h>
#include <printf.h>
#include <string.h>
#include <malloc.h>
#include <platform/mt_gpt.h>
#define MODULE_NAME "LK_BOOT"

// ************************************************************************


//*********
//* Notice : it's kernel start addr (and not include any debug header)
unsigned int g_kmem_off = 0;

//*********
//* Notice : it's rootfs start addr (and not include any debug header)
unsigned int g_rmem_off = 0;


unsigned int g_bimg_sz = 0;
unsigned int g_rcimg_sz = 0;
unsigned int g_fcimg_sz = 0;
int g_kimg_sz = 0;
int g_rimg_sz = 0;

extern boot_img_hdr *g_boot_hdr;


#if 1

static int mboot_common_load_part_info(part_dev_t *dev, char *part_name, part_hdr_t *part_hdr)
{
	long len;
#ifdef MTK_EMMC_SUPPORT
	u64 addr;
#else
	ulong addr;
#endif
	part_t *part;

	part = mt_part_get_partition(part_name);
	if (part == NULL) {
		return -1;
	}
#ifdef MTK_EMMC_SUPPORT
	addr = (u64)part->start_sect * BLK_SIZE;
#else
	addr = part->start_sect * BLK_SIZE;
#endif

	//***************
	//* read partition header
	//*
#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, addr, (uchar*)part_hdr, sizeof(part_hdr_t), part->part_id);
#else
	len = dev->read(dev, addr, (uchar*)part_hdr, sizeof(part_hdr_t));
#endif
#else
	len = dev->read(dev, addr, (uchar*)part_hdr, sizeof(part_hdr_t));
#endif

	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s partition read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	printf("\n=========================================\n");
	dprintf(CRITICAL, "[%s] %s magic number : 0x%x\n",MODULE_NAME,part_name,part_hdr->info.magic);
	part_hdr->info.name[31]='\0'; //append end char
	dprintf(CRITICAL, "[%s] %s name         : %s\n",MODULE_NAME,part_name,part_hdr->info.name);
	dprintf(CRITICAL, "[%s] %s size         : %d\n",MODULE_NAME,part_name,part_hdr->info.dsize);
	printf("=========================================\n");

	//***************
	//* check partition magic
	//*
	if (part_hdr->info.magic != PART_MAGIC) {
		dprintf(CRITICAL, "[%s] %s partition magic error\n", MODULE_NAME, part_name);
		return -1;
	}

	//***************
	//* check partition name
	//*
	if (strncasecmp(part_hdr->info.name, part_name, sizeof(part_hdr->info.name))) {
		dprintf(CRITICAL, "[%s] %s partition name error\n", MODULE_NAME, part_name);
		return -1;
	}

	//***************
	//* check partition data size
	//*
	if (part_hdr->info.dsize > part->nr_sects * BLK_SIZE) {
		dprintf(CRITICAL, "[%s] %s partition size error\n", MODULE_NAME, part_name);
		return -1;
	}

	return 0;
}


/**********************************************************
 * Routine: mboot_common_load_part
 *
 * Description: common function for loading image from nand flash
 *              this function is called by
 *                  (1) 'mboot_common_load_logo' to display logo
 *
 **********************************************************/
int mboot_common_load_part(char *part_name, unsigned long addr)
{
	long len;
#ifdef MTK_EMMC_SUPPORT
	unsigned long long start_addr;
#else
	unsigned long start_addr;
#endif
	part_t *part;
	part_dev_t *dev;
	part_hdr_t *part_hdr;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		return -ENOENT;
	}

#ifdef MTK_EMMC_SUPPORT
	start_addr = (u64)part->start_sect * BLK_SIZE;
#else
	start_addr = part->start_sect * BLK_SIZE;
#endif

	part_hdr = (part_hdr_t*)malloc(sizeof(part_hdr_t));


	if (!part_hdr) {
		return -ENOMEM;
	}

	len = mboot_common_load_part_info(dev, part_name, part_hdr);
	if (len < 0) {
		len = -EINVAL;
		goto exit;
	}


	//****************
	//* read image data
	//*
	printf("read the data of %s\n", part_name);


#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, start_addr + sizeof(part_hdr_t), (uchar*)addr, part_hdr->info.dsize, part->part_id);
#else
	len = dev->read(dev, start_addr + sizeof(part_hdr_t), (uchar*)addr, part_hdr->info.dsize);
#endif
#else
	len = dev->read(dev, start_addr + sizeof(part_hdr_t), (uchar*)addr, part_hdr->info.dsize);
#endif

	if (len < 0) {
		len = -EIO;
		goto exit;
	}


exit:
	if (part_hdr)
		free(part_hdr);

	return len;
}

/**********************************************************
 * Routine: mboot_common_load_logo
 *
 * Description: function to load logo to display
 *
 **********************************************************/
int mboot_common_load_logo(unsigned long logo_addr, char* filename)
{
	int ret;
#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	long len;
#endif

#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	len = file_fat_read(filename, (unsigned char *)logo_addr, 0);

	if (len > 0)
		return (int)len;
#endif

	ret = mboot_common_load_part("logo", logo_addr);

	return ret;
}

/**********************************************************
 * Routine: mboot_android_check_img_info
 *
 * Description: this function is called to
 *              (1) check the header of kernel / rootfs
 *
 * Notice : this function will be called by 'mboot_android_check_bootimg_hdr'
 *
 **********************************************************/
int mboot_android_check_img_info(char *part_name, part_hdr_t *part_hdr)
{
	//***************
	//* check partition magic
	//*
	if (part_hdr->info.magic != PART_MAGIC) {
		dprintf(CRITICAL, "[%s] %s partition magic not match\n", MODULE_NAME, part_name);
		return -1;
	}

	//***************
	//* check partition name
	//*
	if (strncasecmp(part_hdr->info.name, part_name, sizeof(part_hdr->info.name))) {
		dprintf(CRITICAL, "[%s] %s partition name not match\n", MODULE_NAME, part_name);
		return -1;
	}

	printf("\n=========================================\n");
	dprintf(CRITICAL, "[%s] %s magic number : 0x%x\n",MODULE_NAME,part_name,part_hdr->info.magic);
	dprintf(CRITICAL, "[%s] %s size         : 0x%x\n",MODULE_NAME,part_name,part_hdr->info.dsize);
	printf("=========================================\n");

	//***************
	//* return the image size
	//*
	return part_hdr->info.dsize;
}

/**********************************************************
 * Routine: mboot_android_check_bootimg_hdr
 *
 * Description: this function is called to
 *              (1) 'read' the header of boot image from nand flash
 *              (2) 'parse' the header of boot image to obtain
 *                  - (a) kernel image size
 *                  - (b) rootfs image size
 *                  - (c) rootfs offset
 *
 * Notice : this function must be read first when doing nand / msdc boot
 *
 **********************************************************/
static int mboot_android_check_bootimg_hdr(part_dev_t *dev, char *part_name, boot_img_hdr *boot_hdr)
{
	long len;
#ifdef MTK_EMMC_SUPPORT
	u64 addr;
#else
	ulong addr;
#endif
	part_t *part;


	//**********************************
	// TODO : fix pg_sz assignment
	//**********************************
	unsigned int pg_sz = 2*1024 ;

	part = mt_part_get_partition(part_name);
	if (part == NULL) {
		return -1;
	}
#ifdef MTK_EMMC_SUPPORT
	addr = (u64)part->start_sect * BLK_SIZE;
#else
	addr = part->start_sect * BLK_SIZE;
#endif

	//***************
	//* read partition header
	//*

	dprintf(CRITICAL, "part page addr is 0x%llx\n", addr);

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr), part->part_id);
#else
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr));
#endif
#else
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr));
#endif
	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s boot image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	printf("\n============================================================\n");
	boot_hdr->magic[7] = '\0';
	dprintf(CRITICAL, "[%s] Android Partition Name                : %s\n"    , MODULE_NAME, part_name);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Magic          : %s\n"    , MODULE_NAME, boot_hdr->magic);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Kernel Size    : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_size);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Kernel Address : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_addr);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Rootfs Size    : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_size);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Rootfs Address : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Tags Address   : 0x%08X\n", MODULE_NAME, boot_hdr->tags_addr);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Page Size      : 0x%08X\n", MODULE_NAME, boot_hdr->page_size);
	dprintf(CRITICAL, "[%s] Android Boot IMG Hdr - Command Line   : %s\n"    , MODULE_NAME, boot_hdr->cmdline);
	printf("============================================================\n");

	//***************
	//* check partition magic
	//*
	if (strncmp((char const *)boot_hdr->magic,BOOT_MAGIC, sizeof(BOOT_MAGIC))!=0) {
		dprintf(CRITICAL, "[%s] boot image header magic error\n", MODULE_NAME);
		return -1;
	}

	pg_sz = boot_hdr->page_size;

	//***************
	//* follow bootimg.h to calculate the location of rootfs
	//*
	if (len != -1) {
		unsigned int k_pg_cnt = 0;
		unsigned int r_pg_cnt = 0;
		if (g_is_64bit_kernel) {
			g_kmem_off = (unsigned int)target_get_scratch_address();
		} else {
			g_kmem_off = boot_hdr->kernel_addr;
		}
		if (boot_hdr->kernel_size % pg_sz == 0) {
			k_pg_cnt = boot_hdr->kernel_size / pg_sz;
		} else {
			k_pg_cnt = (boot_hdr->kernel_size / pg_sz) + 1;
		}

		if (boot_hdr->ramdisk_size % pg_sz == 0) {
			r_pg_cnt = boot_hdr->ramdisk_size / pg_sz;
		} else {
			r_pg_cnt = (boot_hdr->ramdisk_size / pg_sz) + 1;
		}

		printf(" > page count of kernel image = %d\n",k_pg_cnt);
		g_rmem_off = g_kmem_off + k_pg_cnt * pg_sz;

		printf(" > kernel mem offset = 0x%x\n",g_kmem_off);
		printf(" > rootfs mem offset = 0x%x\n",g_rmem_off);


		//***************
		//* specify boot image size
		//*
		g_bimg_sz = (k_pg_cnt + r_pg_cnt + 1)* pg_sz;

		printf(" > boot image size = 0x%x\n",g_bimg_sz);
	}

	return 0;
}

/**********************************************************
 * Routine: mboot_android_check_recoveryimg_hdr
 *
 * Description: this function is called to
 *              (1) 'read' the header of boot image from nand flash
 *              (2) 'parse' the header of boot image to obtain
 *                  - (a) kernel image size
 *                  - (b) rootfs image size
 *                  - (c) rootfs offset
 *
 * Notice : this function must be read first when doing nand / msdc boot
 *
 **********************************************************/
static int mboot_android_check_recoveryimg_hdr(part_dev_t *dev, char *part_name, boot_img_hdr *boot_hdr)
{
	long len;
#ifdef MTK_EMMC_SUPPORT
	u64 addr;
#else
	ulong addr;
#endif
	part_t *part;

	//**********************************
	// TODO : fix pg_sz assignment
	//**********************************
	unsigned int pg_sz = 2*1024 ;


	part = mt_part_get_partition(part_name);
	if (part == NULL) {
		return -1;
	}
#ifdef MTK_EMMC_SUPPORT
	addr = (u64)part->start_sect * BLK_SIZE;
#else
	addr = part->start_sect * BLK_SIZE;
#endif

	//***************
	//* read partition header
	//*
#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr), part->part_id);
#else
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr));
#endif
#else
	len = dev->read(dev, addr, (uchar*) boot_hdr, sizeof(boot_img_hdr));
#endif
	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s Recovery image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	printf("\n============================================================\n");
	boot_hdr->magic[7] = '\0';
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Magic          : %s\n"    , MODULE_NAME, boot_hdr->magic);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Kernel Size    : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Kernel Address : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Rootfs Size    : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Rootfs Address : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Tags Address   : 0x%08X\n", MODULE_NAME, boot_hdr->tags_addr);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Page Size      : 0x%08X\n", MODULE_NAME, boot_hdr->page_size);
	dprintf(CRITICAL, "[%s] Android Recovery IMG Hdr - Command Line   : %s\n"    , MODULE_NAME, boot_hdr->cmdline);
	printf("============================================================\n");

	//***************
	//* check partition magic
	//*
	if (strncmp((char const *)boot_hdr->magic,BOOT_MAGIC, sizeof(BOOT_MAGIC))!=0) {
		dprintf(CRITICAL, "[%s] Recovery image header magic error\n", MODULE_NAME);
		return -1;
	}

	pg_sz = boot_hdr->page_size;

	//***************
	//* follow bootimg.h to calculate the location of rootfs
	//*
	if (len != -1) {
		unsigned int k_pg_cnt = 0;
		unsigned int r_pg_cnt = 0;

		if (g_is_64bit_kernel) {
			g_kmem_off = (unsigned int)target_get_scratch_address();
		} else {
			g_kmem_off =  boot_hdr->kernel_addr;
		}


		if (boot_hdr->kernel_size % pg_sz == 0) {
			k_pg_cnt = boot_hdr->kernel_size / pg_sz;
		} else {
			k_pg_cnt = (boot_hdr->kernel_size / pg_sz) + 1;
		}

		if (boot_hdr->ramdisk_size % pg_sz == 0) {
			r_pg_cnt = boot_hdr->ramdisk_size / pg_sz;
		} else {
			r_pg_cnt = (boot_hdr->ramdisk_size / pg_sz) + 1;
		}

		printf(" > page count of kernel image = %d\n",k_pg_cnt);
		g_rmem_off = g_kmem_off + k_pg_cnt * pg_sz;

		printf(" > kernel mem offset = 0x%x\n",g_kmem_off);
		printf(" > rootfs mem offset = 0x%x\n",g_rmem_off);


		//***************
		//* specify boot image size
		//*
		//g_rcimg_sz = part->start_sect * BLK_SIZE;
		g_rcimg_sz = (k_pg_cnt + r_pg_cnt + 1)* pg_sz;

		printf(" > Recovery image size = 0x%x\n", g_rcimg_sz);
	}

	return 0;
}


/**********************************************************
 * Routine: mboot_android_check_factoryimg_hdr
 *
 * Description: this function is called to
 *              (1) 'read' the header of boot image from nand flash
 *              (2) 'parse' the header of boot image to obtain
 *                  - (a) kernel image size
 *                  - (b) rootfs image size
 *                  - (c) rootfs offset
 *
 * Notice : this function must be read first when doing nand / msdc boot
 *
 **********************************************************/
static int mboot_android_check_factoryimg_hdr(char *part_name, boot_img_hdr *boot_hdr)
{
	int len=0;
	//   ulong addr;

	//**********************************
	// TODO : fix pg_sz assignment
	//**********************************
	unsigned int pg_sz = 2*1024 ;

	//***************
	//* read partition header
	//*

#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	len = file_fat_read(part_name, (uchar*) boot_hdr, sizeof(boot_img_hdr));

	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s Factory image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}
#endif

	printf("\n============================================================\n");
	boot_hdr->magic[7] = '\0';
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Magic          : %s\n"    , MODULE_NAME, boot_hdr->magic);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Kernel Size    : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_size);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Kernel Address : 0x%08X\n", MODULE_NAME, boot_hdr->kernel_addr);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Rootfs Size    : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_size);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Rootfs Address : 0x%08X\n", MODULE_NAME, boot_hdr->ramdisk_addr);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Tags Address   : 0x%08X\n", MODULE_NAME, boot_hdr->tags_addr);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Page Size      : 0x%08X\n", MODULE_NAME, boot_hdr->page_size);
	dprintf(CRITICAL, "[%s] Android Factory IMG Hdr - Command Line   : %s\n"    , MODULE_NAME, boot_hdr->cmdline);
	printf("============================================================\n");

	//***************
	//* check partition magic
	//*
	if (strncmp((char const *)boot_hdr->magic,BOOT_MAGIC, sizeof(BOOT_MAGIC))!=0) {
		dprintf(CRITICAL, "[%s] Factory image header magic error\n", MODULE_NAME);
		return -1;
	}

	pg_sz = boot_hdr->page_size;

	//***************
	//* follow bootimg.h to calculate the location of rootfs
	//*
	if (len != -1) {
		unsigned int k_pg_cnt = 0;

		if (g_is_64bit_kernel) {
			g_kmem_off = (unsigned int)target_get_scratch_address();
		} else {
			g_kmem_off =  boot_hdr->kernel_addr;
		}


		if (boot_hdr->kernel_size % pg_sz == 0) {
			k_pg_cnt = boot_hdr->kernel_size / pg_sz;
		} else {
			k_pg_cnt = (boot_hdr->kernel_size / pg_sz) + 1;
		}

		printf(" > page count of kernel image = %d\n",k_pg_cnt);
		g_rmem_off = g_kmem_off + k_pg_cnt * pg_sz;

		printf(" > kernel mem offset = 0x%x\n",g_kmem_off);
		printf(" > rootfs mem offset = 0x%x\n",g_rmem_off);


		//***************
		//* specify boot image size
		//*
		//g_fcimg_sz = PART_BLKS_RECOVERY * BLK_SIZE;

		printf(" > Factory image size = 0x%x\n", g_rcimg_sz);
	}

	return 0;
}


/**********************************************************
 * Routine: mboot_android_load_bootimg_hdr
 *
 * Description: this is the entry function to handle boot image header
 *
 **********************************************************/
int mboot_android_load_bootimg_hdr(char *part_name, unsigned long addr)
{
	long len;
//	unsigned long begin;
//	unsigned long start_addr;
	part_t *part;
	part_dev_t *dev;
	boot_img_hdr *boot_hdr;

	dev = mt_part_get_device();
	if (!dev) {
		printf("mboot_android_load_bootimg_hdr, dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		printf("mboot_android_load_bootimg_hdr (%s), part = NULL\n",part_name);
		return -ENOENT;
	}

//    start_addr = part->start_sect * BLK_SIZE;

	boot_hdr = (boot_img_hdr*)malloc(sizeof(boot_img_hdr));
	if (!boot_hdr) {
		printf("mboot_android_load_bootimg_hdr, boot_hdr = NULL\n");
		return -ENOMEM;
	}

	g_boot_hdr = boot_hdr;

	len = mboot_android_check_bootimg_hdr(dev, part_name, boot_hdr);

	return len;
}

/**********************************************************
 * Routine: mboot_android_load_recoveryimg_hdr
 *
 * Description: this is the entry function to handle Recovery image header
 *
 **********************************************************/
int mboot_android_load_recoveryimg_hdr(char *part_name, unsigned long addr)
{
	long len;
//	unsigned long begin;
//	unsigned long start_addr;
	part_t *part;
	part_dev_t *dev;
	boot_img_hdr *boot_hdr;

	dev = mt_part_get_device();
	if (!dev) {
		printf("mboot_android_load_recoveryimg_hdr, dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		printf("mboot_android_load_recoveryimg_hdr (%s), part = NULL\n",part_name);
		return -ENOENT;
	}

//    start_addr = part->start_sect * BLK_SIZE;

	boot_hdr = (boot_img_hdr*)malloc(sizeof(boot_img_hdr));
	if (!boot_hdr) {
		printf("mboot_android_load_bootimg_hdr, boot_hdr = NULL\n");
		return -ENOMEM;
	}

	g_boot_hdr = boot_hdr;

	len = mboot_android_check_recoveryimg_hdr(dev, part_name, boot_hdr);

	return len;
}


/**********************************************************
 * Routine: mboot_android_load_factoryimg_hdr
 *
 * Description: this is the entry function to handle Factory image header
 *
 **********************************************************/
int mboot_android_load_factoryimg_hdr(char *part_name, unsigned long addr)
{
	long len;

	boot_img_hdr *boot_hdr;

	boot_hdr = (boot_img_hdr*)malloc(sizeof(boot_img_hdr));

	if (!boot_hdr) {
		printf("mboot_android_load_factoryimg_hdr, boot_hdr = NULL\n");
		return -ENOMEM;
	}

	g_boot_hdr = boot_hdr;

	len = mboot_android_check_factoryimg_hdr(part_name, boot_hdr);

	return len;
}


/**********************************************************
 * Routine: mboot_android_load_bootimg
 *
 * Description: main function to load Android Boot Image
 *
 **********************************************************/
int mboot_android_load_bootimg(char *part_name, unsigned long addr)
{
	long len;
#ifdef MTK_EMMC_SUPPORT
	unsigned long long start_addr;
#else
	unsigned long start_addr;
#endif
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		printf("mboot_android_load_bootimg , dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		printf("mboot_android_load_bootimg , part = NULL\n");
		return -ENOENT;
	}

	//***************
	//* not to include unused header
	//*
#ifdef MTK_EMMC_SUPPORT
	start_addr =(u64)part->start_sect * BLK_SIZE + g_boot_hdr->page_size;
#else
	start_addr = part->start_sect * BLK_SIZE + g_boot_hdr->page_size;
#endif

	/*
	 * check mkimg header
	 */
	printf("check mkimg header\n");
#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	dev->read(dev, start_addr, (uchar*)addr, MKIMG_HEADER_SZ, part->part_id);
#else
	dev->read(dev, start_addr, (uchar*)addr, MKIMG_HEADER_SZ);
#endif
	// check kernel header
	g_kimg_sz = mboot_android_check_img_info(PART_KERNEL, (part_hdr_t *)addr);
	if (g_kimg_sz == -1) {
		printf("no mkimg header in kernel image\n");
	} else {
		printf("mkimg header exist in kernel image\n");
		addr  = addr - MKIMG_HEADER_SZ;
		g_rmem_off = g_rmem_off - MKIMG_HEADER_SZ;
	}

	//***************
	//* read image data
	//*
	printf("\nread the data of %s (size = 0x%x)\n", part_name, g_bimg_sz);
#ifdef MTK_EMMC_SUPPORT
	printf(" > from - 0x%016llx (skip boot img hdr)\n",start_addr);
#else
	printf(" > from - 0x%x (skip boot img hdr)\n",start_addr);
#endif
	printf(" > to   - 0x%x (starts with kernel img hdr)\n",addr);

#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	len = dev->read(dev, start_addr, (uchar*)addr, g_bimg_sz, part->part_id);
#else
	len = dev->read(dev, start_addr, (uchar*)addr, g_bimg_sz);
#endif

	// check ramdisk/rootfs header
	g_rimg_sz = mboot_android_check_img_info(PART_ROOTFS, (part_hdr_t *)g_rmem_off);
	if (g_rimg_sz == -1) {
		printf("no mkimg header in ramdisk image\n");
		g_rimg_sz = g_boot_hdr->ramdisk_size;
	} else {
		printf("mkimg header exist in ramdisk image\n");
		g_rmem_off = g_rmem_off + MKIMG_HEADER_SZ;
	}

	if (len < 0) {
		len = -EIO;
	}

	return len;
}

/**********************************************************
 * Routine: mboot_android_load_recoveryimg
 *
 * Description: main function to load Android Recovery Image
 *
 **********************************************************/
int mboot_android_load_recoveryimg(char *part_name, unsigned long addr)
{
	long len;
#ifdef MTK_EMMC_SUPPORT
	unsigned long long start_addr;
#else
	unsigned long start_addr;
#endif
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		printf("mboot_android_load_bootimg , dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		printf("mboot_android_load_bootimg , part = NULL\n");
		return -ENOENT;
	}

	//***************
	//* not to include unused header
	//*
#ifdef MTK_EMMC_SUPPORT
	start_addr = (u64)part->start_sect * BLK_SIZE + g_boot_hdr->page_size;
#else
	start_addr = part->start_sect * BLK_SIZE + g_boot_hdr->page_size;
#endif

	/*
	 * check mkimg header
	 */
	printf("check mkimg header\n");
#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	dev->read(dev, start_addr, (uchar*)addr, MKIMG_HEADER_SZ, part->part_id);
#else
	dev->read(dev, start_addr, (uchar*)addr, MKIMG_HEADER_SZ);
#endif
	// check kernel header
	g_kimg_sz = mboot_android_check_img_info(PART_KERNEL, (part_hdr_t *)addr);
	if (g_kimg_sz == -1) {
		printf("no mkimg header in kernel image\n");
	} else {
		printf("mkimg header exist in kernel image\n");
		addr  = addr - MKIMG_HEADER_SZ;
		g_rmem_off = g_rmem_off - MKIMG_HEADER_SZ;
	}

	//***************
	//* read image data
	//*
	printf("\nread the data of %s (size = 0x%x)\n", part_name, g_rcimg_sz);
#ifdef MTK_EMMC_SUPPORT
	printf(" > from - 0x%016llx (skip recovery img hdr)\n",start_addr);
#else
	printf(" > from - 0x%x (skip recovery img hdr)\n",start_addr);
#endif
	printf(" > to   - 0x%x (starts with kernel img hdr)\n",addr);

#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	len = dev->read(dev, start_addr, (uchar*)addr, g_rcimg_sz, part->part_id);
#else
	len = dev->read(dev, start_addr, (uchar*)addr, g_rcimg_sz);
#endif

	// check ramdisk/rootfs header
	g_rimg_sz = mboot_android_check_img_info("recovery", (part_hdr_t *)g_rmem_off);
	if (g_rimg_sz == -1) {
		printf("no mkimg header in recovery image\n");
		g_rimg_sz = g_boot_hdr->ramdisk_size;
	} else {
		printf("mkimg header exist in recovery image\n");
		g_rmem_off = g_rmem_off + MKIMG_HEADER_SZ;
	}

	if (len < 0) {
		len = -EIO;
	}

	return len;
}


/**********************************************************
 * Routine: mboot_android_load_factoryimg
 *
 * Description: main function to load Android Factory Image
 *
 **********************************************************/
int mboot_android_load_factoryimg(char *part_name, unsigned long addr)
{
	int len = 0;

	//***************
	//* not to include unused header
	//*
	addr = addr - g_boot_hdr->page_size;

	/*
	 * check mkimg header
	 */
	printf("check mkimg header\n");
#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	file_fat_read(part_name, (uchar*)addr, MKIMG_HEADER_SZ);
#endif
	// check kernel header
	g_kimg_sz = mboot_android_check_img_info(PART_KERNEL, (part_hdr_t *)addr);
	if (g_kimg_sz == -1) {
		printf("no mkimg header in kernel image\n");
	} else {
		printf("mkimg header exist in kernel image\n");
		addr = addr - MKIMG_HEADER_SZ;
		g_rmem_off = g_rmem_off - MKIMG_HEADER_SZ;
	}

#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	len = file_fat_read(part_name, (uchar*)addr, 0);
	dprintf(CRITICAL, "len = %d, addr = 0x%x\n", len, addr);
	dprintf(CRITICAL, "part name = %s \n", part_name);
#endif

	// check ramdisk/rootfs header
	g_rimg_sz = mboot_android_check_img_info(PART_ROOTFS, (part_hdr_t *)g_rmem_off);
	if (g_rimg_sz == -1) {
		printf("no mkimg header in ramdisk image\n");
		g_rimg_sz = g_boot_hdr->ramdisk_size;
	} else {
		printf("mkimg header exist in ramdisk image\n");
		g_rmem_off = g_rmem_off + MKIMG_HEADER_SZ;
	}
#if (CONFIG_COMMANDS & CFG_CMD_FAT)
	if (len < 0) {
		len = -EIO;
	}
#endif
	return len;
}


/**********************************************************
 * Routine: mboot_recovery_load_raw_part
 *
 * Description: load raw data for recovery mode support
 *
 **********************************************************/
int mboot_recovery_load_raw_part(char *part_name, unsigned long *addr, unsigned int size)
{
	long len;
	unsigned long begin;

#ifdef MTK_EMMC_SUPPORT
	unsigned long long start_addr;
#else
	unsigned long start_addr;
#endif
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		return -ENOENT;
	}
#ifdef MTK_EMMC_SUPPORT
	start_addr = (u64)part->start_sect * BLK_SIZE;
#else
	start_addr = part->startblk * BLK_SIZE;
#endif
	begin = get_timer(0);

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, start_addr,(uchar*)addr, size, part->part_id);
#else
	len = dev->read(dev, start_addr,(uchar*)addr, size);
#endif
#else
	len = dev->read(dev, start_addr,(uchar*)addr, size);
#endif
	if (len < 0) {
		len = -EIO;
		goto exit;
	}

	dprintf(CRITICAL, "[%s] Load '%s' partition to 0x%08lX (%d bytes in %ld ms)\n", MODULE_NAME, part->name, (unsigned long)addr, size, get_timer(begin));

exit:
	return len;
}

/**********************************************************
 * Routine: mboot_recovery_load_raw_part_offset
 *
 * Description: load partition raw data with offset
 *
 * offset and size must page alignemnt
 **********************************************************/
int mboot_recovery_load_raw_part_offset(char *part_name, unsigned long *addr, unsigned long offset, unsigned int size)
{
	long len;
	unsigned long begin;

#ifdef MTK_EMMC_SUPPORT
	unsigned long long start_addr;
#else
	unsigned long start_addr;
#endif
	part_t *part;
	part_dev_t *dev;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		return -ENOENT;
	}
#ifdef MTK_EMMC_SUPPORT
	start_addr = (u64)part->start_sect * BLK_SIZE + ROUNDUP(offset, BLK_SIZE);
#else
	start_addr = part->startblk * BLK_SIZE + ROUNDUP(offset, BLK_SIZE);
#endif
	begin = get_timer(0);

#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)

	len = dev->read(dev, start_addr,(uchar*)addr, ROUNDUP(size, BLK_SIZE), part->part_id);
#else
	len = dev->read(dev, start_addr,(uchar*)addr, ROUNDUP(size, BLK_SIZE));
#endif

	if (len < 0) {
		len = -EIO;
		goto exit;
	}

	dprintf(INFO, "[%s] Load '%s' partition to 0x%08lX (%d bytes in %ld ms)\n",
	        MODULE_NAME, part->name, (unsigned long)addr, size, get_timer(begin));

exit:
	return len;
}


/**********************************************************
 * Routine: mboot_recovery_load_misc
 *
 * Description: load recovery command
 *
 **********************************************************/
int mboot_recovery_load_misc(unsigned long *misc_addr, unsigned int size)
{
	int ret;

	printf("[mboot_recovery_load_misc]: size is %u\n", size);
	printf("[mboot_recovery_load_misc]: misc_addr is 0x%x\n", misc_addr);

	ret = mboot_recovery_load_raw_part("para", misc_addr, size);

	if (ret < 0)
		return ret;

	return ret;
}

/**********************************************************
 * Routine: mboot_get_inhouse_img_size
 *
 * Description: Get img size from mkimage header (LK,Logo)
                The size include both image and header and the size is align to 4k.
 *
 **********************************************************/
unsigned int mboot_get_inhouse_img_size(char *part_name, unsigned int *size)
{
	int ret = 0;
	long len = 0;
#ifdef MTK_EMMC_SUPPORT
	u64 addr;
#else
	ulong addr;
#endif

	part_t *part;
	part_dev_t *dev;
	part_hdr_t mkimage_hdr;
	part_hdr_t *part_hdr;
	unsigned page_size = 0x1000;

	*size = 0;

	printf("Get inhouse img size from mkimage header\n");

	dev = mt_part_get_device();
	if (!dev) {
		printf("mboot_android_load_img_hdr, dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		printf("mboot_android_load_img_hdr (%s), part = NULL\n",part_name);
		return -ENOENT;
	}

#ifdef MTK_EMMC_SUPPORT
	addr = (u64)part->start_sect * BLK_SIZE;
#else
	addr = part->startblk * BLK_SIZE;
#endif

	/*Read mkimage header*/
#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	len = dev->read(dev, addr, (uchar*)&mkimage_hdr, sizeof(part_hdr_t), part->part_id);
#else
	len = dev->read(dev, addr, (uchar*)&mkimage_hdr, sizeof(part_hdr_t), part->part_id);
#endif

	printf("\n============================================================\n");
	printf("[%s] INHOUSE Partition addr             : %llx\n", MODULE_NAME, (u64)addr);
	printf("[%s] INHOUSE Partition Name             : %s\n", MODULE_NAME, part_name);
	printf("[%s] INHOUSE IMG HDR - Magic            : %x\n", MODULE_NAME, mkimage_hdr.info.magic);
	printf("[%s] INHOUSE IMG size                    : %x\n", MODULE_NAME, mkimage_hdr.info.dsize);
	printf("[%s] INHOUSE IMG HDR size                : %x\n", MODULE_NAME, sizeof(part_hdr_t));
	printf("============================================================\n");

	*size =  (((mkimage_hdr.info.dsize + sizeof(part_hdr_t)  + page_size - 1) / page_size) * page_size);
	printf("[%s] INHOUSE IMG size           : %x\n", MODULE_NAME, *size);

	//mboot_common_load_part_info(dev, part_name, part_hdr);

	return ret;

}

unsigned int mboot_get_img_size(char *part_name, unsigned int *size)
{
	int ret = 0;
	long len = 0;
#ifdef MTK_EMMC_SUPPORT
	u64 addr;
#else
	ulong addr;
#endif
	part_t *part;
	part_dev_t *dev;
	boot_img_hdr boot_hdr;
	/* use starting 16 bytes to get boot sig size */
	/* actual boot signature size is much bigger */
	#define BOOT_SIG_HDR_SZ 16
	unsigned char boot_sig_hdr[BOOT_SIG_HDR_SZ] = {0};
	unsigned boot_sig_size = 0;
	unsigned page_size = 0x800; /* used to cache page size in boot image hdr, default 2KB */

	*size = 0;

	dev = mt_part_get_device();
	if (!dev) {
		printf("mboot_android_load_img_hdr, dev = NULL\n");
		return -ENODEV;
	}

	part = mt_part_get_partition(part_name);
	if (!part) {
		printf("mboot_android_load_img_hdr (%s), part = NULL\n",part_name);
		return -ENOENT;
	}
#ifdef MTK_EMMC_SUPPORT
	addr = (u64)part->start_sect * BLK_SIZE;
#else
	addr = part->startblk * BLK_SIZE;
#endif

#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	len = dev->read(dev, addr, (uchar*)&boot_hdr, sizeof(boot_img_hdr), part->part_id);
#else
	len = dev->read(dev, addr, (uchar*)&boot_hdr, sizeof(boot_img_hdr));
#endif
	if (len < 0) {
		printf("[%s] %s boot image header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	printf("\n============================================================\n");
	boot_hdr.magic[7] = '\0';
	printf("[%s] Android Partition Name             : %s\n", MODULE_NAME, part_name);
	printf("[%s] Android IMG Hdr - Magic            : %s\n", MODULE_NAME, boot_hdr.magic);
	printf("[%s] Android IMG Hdr - Kernel Size      : 0x%08X\n", MODULE_NAME, boot_hdr.kernel_size);
	printf("[%s] Android IMG Hdr - Kernel Address   : 0x%08X\n", MODULE_NAME, boot_hdr.kernel_addr);
	printf("[%s] Android IMG Hdr - Rootfs Size      : 0x%08X\n", MODULE_NAME, boot_hdr.ramdisk_size);
	printf("[%s] Android IMG Hdr - Rootfs Address   : 0x%08X\n", MODULE_NAME, boot_hdr.ramdisk_addr);
	printf("[%s] Android IMG Hdr - Tags Address     : 0x%08X\n", MODULE_NAME, boot_hdr.tags_addr);
	printf("[%s] Android IMG Hdr - Page Size        : 0x%08X\n", MODULE_NAME, boot_hdr.page_size);
	printf("[%s] Android IMG Hdr - Command Line     : %s\n"  , MODULE_NAME, boot_hdr.cmdline);
	printf("============================================================\n");

	page_size = boot_hdr.page_size;
	*size +=  page_size; /* boot header size is 1 page*/
	*size +=  (((boot_hdr.kernel_size + page_size - 1) / page_size) * page_size);
	*size +=  (((boot_hdr.ramdisk_size + page_size - 1) / page_size) * page_size);
	*size +=  (((boot_hdr.second_size + page_size - 1) / page_size) * page_size);

	/* try to get boot siganture size if it exists */
#if defined(MTK_EMMC_SUPPORT) && defined(MTK_NEW_COMBO_EMMC_SUPPORT)
	len = dev->read(dev, addr + (u64)(*size), (uchar*)&boot_sig_hdr, BOOT_SIG_HDR_SZ, part->part_id);
#else
	len = dev->read(dev, addr + (ulong)(*size), (uchar*)&boot_sig_hdr, BOOT_SIG_HDR_SZ);
#endif
	if (len < 0) {
		dprintf(CRITICAL, "[%s] %s boot sig header read error. LINE: %d\n", MODULE_NAME, part_name, __LINE__);
		return -1;
	}

	/* in case boot image is signed by boot signer */
	#define ASN_ID_SEQUENCE  0x30
	if (boot_sig_hdr[0] == ASN_ID_SEQUENCE) {
		/* boot signature exists */
		unsigned len = 0;
		unsigned len_size = 0;
		if (boot_sig_hdr[1] & 0x80) {
			/* multi-byte length field */
			unsigned int i = 0;
			len_size = 1 + (boot_sig_hdr[1] & 0x7f);
			for (i = 0; i < len_size - 1; i++) {
				len = (len << 8) | boot_sig_hdr[2 + i];
			}
		}
		else {
			/* single-byte length field */
			len_size = 1;
			len = boot_sig_hdr[1];
		}

		boot_sig_size = 1 + len_size + len;
	}
	*size +=  (((boot_sig_size + page_size - 1) / page_size) * page_size);

	return ret;
}

#endif
