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

#ifndef __RAM_CONSOLE_H__
#define __RAM_CONSOLE_H__
#include <typedefs.h>

#define RC_CPU_COUNT 8
#define TASK_COMM_LEN 16

#ifdef CFG_NON_SECURE_SRAM_ADDR
#define RAM_CONSOLE_SRAM_ADDR CFG_NON_SECURE_SRAM_ADDR
#else
#define RAM_CONSOLE_SRAM_ADDR (0x0010DC00)
#endif

#ifdef CFG_NON_SECURE_SRAM_SIZE
#define RAM_CONSOLE_SRAM_SIZE CFG_NON_SECURE_SRAM_SIZE
#else
#define RAM_CONSOLE_SRAM_SIZE (0x2400)
#endif

#define RAM_CONSOLE_DRAM_ADDR (0x43F00000)
#define RAM_CONSOLE_DRAM_SIZE (0x10000)

#define RAM_CONSOLE_PL_SIZE	6

struct ram_console_buffer {
	u32 sig;
	/* for size comptible */
	u32 off_pl;
	u32 off_lpl; /* last preloader */
	u32 sz_pl;
	u32 off_lk;
	u32 off_llk; /* last lk */
	u32 sz_lk;
	u32 padding[9]; /* size = 4 * 16 = 64 byte */
};

struct reboot_reason_pl {
	u32 wdt_status;
	u32 last_func[RAM_CONSOLE_PL_SIZE];
};

#if CFG_RAM_CONSOLE
extern void ram_console_init(void);
extern void ram_console_reboot_reason_save(u8 rgu_status);
extern void ram_console_pl_save(unsigned int val, int index);
#else
void ram_console_init(void)
{
    print("ram_console disabled\n");
}

void ram_console_reboot_reason_save(u8 rgu_status)
{
}

void ram_console_pl_save(unsigned int val, int index)
{
}
#endif

#endif // #ifndef __RAM_CONSOLE_H__
