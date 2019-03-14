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

char* sec_dev_get_part_r_name(u32 index)
{
    return g_part_name_map[index].r_name;
}


int sec_dev_read_wrapper(char *part_name, u64 offset, u8* data, u32 size)
{
    part_dev_t *dev;
    long len;
#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
    part_t *part;
#endif
#endif

    dev = mt_part_get_device();
   	if (!dev){
   	    return PART_GET_DEV_FAIL;
   	}


#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
        part = mt_part_get_partition(part_name);
        if (!part){
            dprintf(CRITICAL,"[read_wrapper] mt_part_get_partition error, name: %s\n", part_name);
            ASSERT(0);
        }
        len = dev->read(dev, offset, (uchar *) data, size, part->part_id);
#else
    len = dev->read(dev, offset, (uchar *) data, size);
#endif
#else
	#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->read(dev, offset, (uchar *) data, size, NAND_PART_USER);
	#else
    len = dev->read(dev, offset, (uchar *) data, size);
	#endif
#endif
    if (len != (int)size)
    {
        return PART_READ_FAIL;
    }

    return B_OK;
}

int sec_dev_write_wrapper(char *part_name, u64 offset, u8* data, u32 size)
{
    part_dev_t *dev;
    long len;
#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
    part_t *part;
#endif
#endif

    dev = mt_part_get_device();
   	if (!dev)
   	    return PART_GET_DEV_FAIL;


#ifndef MTK_EMMC_SUPPORT
    if(nand_erase(offset,(u64)size)!=0){
        return PART_ERASE_FAIL;
    }
#endif

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
   	part = mt_part_get_partition(part_name);
   	if (!part){
   	    dprintf(CRITICAL,"[write_wrapper] mt_part_get_partition error, name: %s\n", part_name);
   	    ASSERT(0);
    }

    len = dev->write(dev, (uchar *) data, offset, size, part->part_id);
#else
    len = dev->write(dev, (uchar *) data, offset, size);
#endif
#else
	#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	len = dev->write(dev, (uchar *) data, offset, size, NAND_PART_USER);
	#else
    len = dev->write(dev, (uchar *) data, offset, size);
	#endif
#endif
    if (len != (int)size)
    {
        return PART_WRITE_FAIL;
    }

    return B_OK;
}

unsigned int sec_dev_nand_erase_size(void){

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
    if (raw_partition(idx))
    {
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
	if(g_boot_mode == RECOVERY_BOOT)
	{
	    return TRUE;
	}
	else
	{
	    return FALSE;
	}
}

