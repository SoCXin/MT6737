/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include "sec_platform.h"
#include "sec_region.h"
#include "sec.h"

/******************************************************************************
 * MODULE
 ******************************************************************************/
#define MOD                         "SEC_REGION"

/******************************************************************************
 * DEBUG
 ******************************************************************************/
#define SMSG                        dbg_print

/******************************************************************************
 *  EXTERNAL VARIABLES
 ******************************************************************************/

/******************************************************************************
 *  SECURITY REGION CHECK
 ******************************************************************************/
typedef struct region {
	unsigned int start;
	unsigned int size;
} REGION;

REGION g_blacklist[] = {
	{MSDC0_BASE, 0x10000},
	{MSDC1_BASE, 0x10000},
	{MSDC2_BASE, 0x10000},
	{MSDC3_BASE, 0x10000},
	{NFI_BASE, 0x1000},
	{NFIECC_BASE, 0x1000},
};

unsigned int is_region_overlap(REGION *region1, REGION *region2)
{
	unsigned int overlap = 0;

	if (region1->start + region1->size <= region2->start)
		overlap = 0;
	else if (region2->start + region2->size <= region1->start)
		overlap = 0;
	else
		overlap = 1;

	return overlap;
}

int blacklist_check(U32 addr, U32 len)
{
	int ret = 0;
	unsigned int i = 0;
	unsigned int blacklist_size = sizeof(g_blacklist) / sizeof(REGION);
	REGION region;
	region.start = (unsigned int)addr;
	region.size = (unsigned int)len;

	for (i = 0; i < blacklist_size; i++) {
		if (is_region_overlap(&region, &(g_blacklist[i]))) {
			ret = -1;
			break;
		}
	}

	return ret;
}

void sec_region_check (U32 addr, U32 len)
{
	U32 ret = SEC_OK;
	U32 tmp = addr + len;

	/* check if it does access AHB/APB register */
	if ((IO_PHYS != (addr & REGION_MASK)) || (IO_PHYS != (tmp & REGION_MASK))) {
		SMSG("[%s] 0x%x Not AHB/APB Address\n", MOD, addr);
		ASSERT(0);
	}

	if (len >= REGION_BANK) {
		SMSG("[%s] Overflow\n",MOD);
		ASSERT(0);
	}

	if (blacklist_check(addr, len)) {
		SMSG("[%s] Not Allowed\n", MOD);
		ASSERT(0);
	}

#ifdef MTK_SECURITY_SW_SUPPORT
	/* check platform security region */
	if (SEC_OK != (ret = seclib_region_check(addr,len))) {
		SMSG("[%s] ERR '0x%x' ADDR: 0x%x, LEN: %d\n", MOD, ret, addr, len);
		ASSERT(0);
	}
#endif
}

/******************************************************************************
 *  DA REGION CHECK
 ******************************************************************************/
U32 da_region_check (U32 addr, U32 len)
{
	U32 ret = SEC_OK;

	if (DA_DOWNLOAD_LOC != addr) {
		ret = ERR_DA_INVALID_LOCATION;
		goto _exit;
	}

	if (DA_DOWNLOAD_MAX_SZ < len) {
		ret = ERR_DA_INVALID_LENGTH;
		goto _exit;
	}

_exit:

	return ret;
}
