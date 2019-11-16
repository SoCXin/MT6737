/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/
/* MTK Proprietary Wrapper File */

#include <platform/mt_typedefs.h>
#include <platform/sec_status.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <platform/errno.h>
#include <debug.h>
#include <platform/boot_mode.h>

extern BOOTMODE g_boot_mode;

#ifndef MTK_EMMC_SUPPORT
extern u32 mtk_nand_erasesize(void);
extern int nand_erase(u64 offset, u64 size);
#endif

typedef struct {
	char official_name[32];
	char alt_name1[32];
	char alt_name2[32];
	char alt_name3[32];
} SEC_PART_TRANS_TBL_ENTRY;

SEC_PART_TRANS_TBL_ENTRY g_part_name_trans_tbl[] = {
	{"preloader", "PRELOADER", "NULL",    "NULL"      },
	{"seccfg",    "SECCFG",    "SECCNFG", "NULL"      },
	{"uboot",     "UBOOT",     "lk",      "LK"        },
	{"boot",      "BOOT",      "bootimg", "BOOTIMG"   },
	{"recovery",  "RECOVERY",  "NULL",    "NULL"      },
	{"secro",     "SECRO",     "sec_ro",   "SEC_RO"   },
	{"logo",      "LOGO",      "NULL",    "NULL"      },
	{"system",    "SYSTEM",    "android", "ANDROID"   },
	{"userdata",  "USERDATA",  "usrdata", "USRDATA"   },
	{"frp",       "FRP",       "NULL",    "NULL"      },
	{"scp1",      "SCP1",      "NULL",    "NULL"      },
	{"scp2",      "SCP2",      "NULL",    "NULL"      },
	{"NULL",      "NULL",      "NULL",    "NULL"      },
};

/* name: should be translated to official name first in security library */
static int sec_partition_get_index(char *name)
{
	int index = -1;
	int found = 0;
	SEC_PART_TRANS_TBL_ENTRY *entry = g_part_name_trans_tbl;

	/* if name matches one of the official names, use translation table */
	for (; 0 != memcmp(entry->official_name, "NULL", strlen("NULL")); entry++) {
		if (0 == memcmp(name, entry->official_name, strlen(entry->official_name))) {
			found = 1;
			break;
		}
	}

	if (found) {
		index = partition_get_index(entry->official_name);
		if (index != -1)
			return index;

		/* try alt_name1 */
		index = partition_get_index(entry->alt_name1);
		if (index != -1) {
			return index;
		}

		/* try alt_name2 */
		index = partition_get_index(entry->alt_name2);
		if (index != -1) {
			return index;
		}

		/* try alt_name3 */
		index = partition_get_index(entry->alt_name3);
		if (index != -1) {
			return index;
		}
	} else {
		index = partition_get_index(name);
		return index;
	}

	return index;
}

char* sec_dev_get_part_r_name(char *name)
{
	int index;
	int err;
	index = sec_partition_get_index(name);
	if (index == -1) {
		err = PART_GET_INDEX_FAIL;
		goto _error;
	}

	return g_part_name_map[index].r_name;
_error:
	dprintf(CRITICAL,"[sec_dev_get_part_r_name] error, name: %s, err: 0x%x\n", name, err);
	ASSERT(0);
}

/* please give fb_name here */
u64 sec_dev_part_get_offset_wrapper(char *name)
{
	u64 offset;
	int index;
	int err;

	index = sec_partition_get_index(name);
	if (index == -1) {
		err = PART_GET_INDEX_FAIL;
		goto _error;
	}

	offset = partition_get_offset(index);
	if (offset == -1) {
		err = PART_GET_INDEX_FAIL;
		goto _error;
	}

	return offset;

_error:
	dprintf(CRITICAL,"[sec_dev_part_get_offset_wrapper] error, name: %s, err: 0x%x\n", name, err);
	ASSERT(0);
}

/* please give fb_name here */
u64 sec_dev_part_get_size_wrapper(char *name)
{
	u64 size;
	int index;
	int err;

	index = sec_partition_get_index(name);
	if (index == -1) {
		err = PART_GET_INDEX_FAIL;
		goto _error;
	}

	size = partition_get_size(index);
	if (size == -1) {
		err = PART_GET_INDEX_FAIL;
		goto _error;
	}

	return size;

_error:
	dprintf(CRITICAL,"[sec_dev_part_get_size_wrapper] error, name: %s, err: 0x%x\n", name, err);
	ASSERT(0);
}

/* dummy parameter is for prameter alignment */
int sec_dev_read_wrapper(char *part_name, u64 offset, u8* data, u32 size)
{
	part_dev_t *dev;
	long len;
	u64 part_addr, read_addr;

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	int index;
	index = sec_partition_get_index(part_name);
	if (index == -1) {
		dprintf(CRITICAL,"[read_wrapper] sec_partition_get_index error, name: %s\n", part_name);
		ASSERT(0);
	}
#endif
#endif

	/* does not check part_offset here since if it's wrong, assertion fails */
	part_addr = sec_dev_part_get_offset_wrapper(part_name);
	read_addr = part_addr + offset;

	dev = mt_part_get_device();
	if (!dev) {
		return PART_GET_DEV_FAIL;
	}

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, read_addr, (uchar *) data, size, partition_get_region(index));
#else
	len = dev->read(dev, read_addr, (uchar *) data, size);
#endif
#else
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, read_addr, (uchar *) data, size, NAND_PART_USER);
#else
	len = dev->read(dev, read_addr, (uchar *) data, size,0);
#endif
#endif
	if (len != (int)size) {
		return PART_READ_FAIL;
	}

	return B_OK;
}

/* dummy parameter is for prameter alignment */
int sec_dev_write_wrapper(char *part_name, u64 offset, u8* data, u32 size)
{
	part_dev_t *dev;
	long len;
	u64 part_addr, write_addr;
#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	int index;
#endif
#endif

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	index = sec_partition_get_index(part_name);
	if (index == -1) {
		dprintf(CRITICAL,"[write_wrapper] sec_partition_get_index error, name: %s\n", part_name);
		ASSERT(0);
	}
#endif
#endif

	/* does not check part_offset here since if it's wrong, assertion fails */
	part_addr = sec_dev_part_get_offset_wrapper(part_name);
	write_addr = part_addr + offset;

	dev = mt_part_get_device();
	if (!dev)
		return PART_GET_DEV_FAIL;


#ifndef MTK_EMMC_SUPPORT
	if (nand_erase(write_addr,(u64)size)!=0) {
		return PART_ERASE_FAIL;
	}
#endif

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->write(dev, (uchar *) data, write_addr, size, partition_get_region(index));
#else
	len = dev->write(dev, (uchar *) data, write_addr, size);
#endif
#else
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->write(dev, (uchar *) data, write_addr, size, NAND_PART_USER);
#else
	len = dev->write(dev, (uchar *) data, write_addr, size, 0);
#endif
#endif
	if (len != (int)size) {
		return PART_WRITE_FAIL;
	}

	return B_OK;
}

unsigned int sec_dev_nand_erase_size(void)
{

#ifdef MTK_EMMC_SUPPORT
	return 0;
#else
	return mtk_nand_erasesize();
#endif

}

u64 sec_dev_nand_address_translate(u64 offset)
{
#ifdef MTK_EMMC_SUPPORT
	return offset;
#else
#if defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT)
	int idx, part_addr;
	part_addr = part_get_startaddress(offset, &idx);
	if (raw_partition(idx)) {
		return part_addr + (offset-part_addr)/2;
	}
	return offset;
#else
	return offset;
#endif
#endif
}

unsigned int sec_dev_nand_block_size(void)
{
#ifdef MTK_EMMC_SUPPORT
	return 0;
#else
#if defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT)
	extern unsigned int BLOCK_SIZE;
	return BLOCK_SIZE;
#else
	return mtk_nand_erasesize();
#endif
#endif
}

BOOL is_recovery_mode(void)
{
	if (g_boot_mode == RECOVERY_BOOT) {
		return TRUE;
	} else {
		return FALSE;
	}
}

