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

#include <debug.h>
#include <stdlib.h>
#include <string.h>
#include <video.h>
#include <dev/uart.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <arch/ops.h>
#include <target/board.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_disp_drv.h>
#include <platform/disp_drv.h>
#include <platform/boot_mode.h>
#include <platform/mt_logo.h>
#include <platform/partition.h>
#include <platform/env.h>
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#include <platform/mt_pmic_wrap_init.h>
#include <platform/mt_i2c.h>
#include <platform/mtk_key.h>
#include <platform/mt_rtc.h>
#include <platform/mt_leds.h>
#include <platform/upmu_common.h>
#include <platform/mtk_wdt.h>
#include <platform/disp_drv_platform.h>
#include <libfdt.h>

#if defined(MTK_SECURITY_SW_SUPPORT)
#include "oemkey.h"
#endif

#ifdef MTK_SECURITY_SW_SUPPORT
extern u8 g_oemkey[OEM_PUBK_SZ];
#endif

#ifdef LK_DL_CHECK
/*block if check dl fail*/
#undef LK_DL_CHECK_BLOCK_LEVEL
#endif
//#define MT_SRAM_REPAIR_SUPPORT

extern void platform_early_init_timer();
extern void jump_da(u32 addr, u32 arg1, u32 arg2);
extern int i2c_hw_init(void);
extern int mboot_common_load_logo(unsigned long logo_addr, char* filename);
extern int sec_func_init(int dev_type);
extern int sec_usbdl_enabled (void);
extern int sec_usbdl_verify_da(unsigned char*, unsigned int, unsigned char*, unsigned int);
extern void mtk_wdt_disable(void);
extern void platform_deinit_interrupts(void);
extern int mmc_get_dl_info(void);
extern int mmc_legacy_init(int);
#ifdef MT_SRAM_REPAIR_SUPPORT
extern int repair_sram(void);
#endif
extern bool g_boot_menu;

#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
extern kal_bool is_low_battery(kal_int32 val);
extern int hw_charging_get_charger_type(void);
#endif
#ifdef MTK_MT8193_SUPPORT
extern int mt8193_init(void);
#endif
void platform_uninit(void);
void config_shared_SRAM_size(void);
extern int dev_info_nr_cpu(void);

BOOT_ARGUMENT *g_boot_arg;
BOOT_ARGUMENT boot_addr;
int g_nr_bank;
BI_DRAM bi_dram[MAX_NR_BANK];
unsigned int g_fb_base;
unsigned int g_fb_size;

static int g_dram_init_ret;

int dram_init(void)
{
	int i;
	struct boot_tag *tags;

	/* Get parameters from pre-loader. Get as early as possible
	 * The address of BOOT_ARGUMENT_LOCATION will be used by Linux later
	 * So copy the parameters from BOOT_ARGUMENT_LOCATION to LK's memory region
	 */
	g_boot_arg = &boot_addr;

	if (*(unsigned int *)BOOT_ARGUMENT_LOCATION == BOOT_ARGUMENT_MAGIC) {
		memcpy(g_boot_arg, (void*)BOOT_ARGUMENT_LOCATION, sizeof(BOOT_ARGUMENT));
	} else {
		g_boot_arg->maggic_number = BOOT_ARGUMENT_MAGIC;
		for (tags = (void *)BOOT_ARGUMENT_LOCATION; tags->hdr.size; tags = boot_tag_next(tags)) {
			switch (tags->hdr.tag) {
				case BOOT_TAG_BOOT_REASON:
					g_boot_arg->boot_reason = tags->u.boot_reason.boot_reason;
					break;
				case BOOT_TAG_BOOT_MODE:
					g_boot_arg->boot_mode = tags->u.boot_mode.boot_mode;
					break;
				case BOOT_TAG_META_COM:
					g_boot_arg->meta_com_type = tags->u.meta_com.meta_com_type;
					g_boot_arg->meta_com_id = tags->u.meta_com.meta_com_id;
					g_boot_arg->meta_uart_port = tags->u.meta_com.meta_uart_port;
					break;
				case BOOT_TAG_LOG_COM:
					g_boot_arg->log_port = tags->u.log_com.log_port;
					g_boot_arg->log_baudrate = tags->u.log_com.log_baudrate;
					g_boot_arg->log_enable = tags->u.log_com.log_enable;
					break;
				case BOOT_TAG_MEM:
					g_boot_arg->dram_rank_num = tags->u.mem.dram_rank_num;
					for (i = 0; i < tags->u.mem.dram_rank_num; i++) {
						g_boot_arg->dram_rank_size[i] = tags->u.mem.dram_rank_size[i];
					}
					g_boot_arg->mblock_info = tags->u.mem.mblock_info;
					g_boot_arg->orig_dram_info = tags->u.mem.orig_dram_info;
					g_boot_arg->lca_reserved_mem = tags->u.mem.lca_reserved_mem;
					g_boot_arg->tee_reserved_mem = tags->u.mem.tee_reserved_mem;
					break;
				case BOOT_TAG_MD_INFO:
					for (i = 0; i < 4; i++) {
						g_boot_arg->md_type[i] = tags->u.md_info.md_type[i];
					}
					break;
				case BOOT_TAG_BOOT_TIME:
					g_boot_arg->boot_time = tags->u.boot_time.boot_time;
					break;
				case BOOT_TAG_DA_INFO:
					memcpy(&g_boot_arg->da_info, &tags->u.da_info.da_info, sizeof(da_info_t));
					break;
				case BOOT_TAG_SEC_INFO:
					memcpy(&g_boot_arg->sec_limit, &tags->u.sec_info.sec_limit, sizeof(SEC_LIMIT));
					break;
				case BOOT_TAG_PART_NUM:
					g_boot_arg->part_num = tags->u.part_num.part_num;
					break;
				case BOOT_TAG_PART_INFO:
					g_boot_arg->part_info = tags->u.part_info.part_info;  /* only copy the pointer but the contains*/
					break;
				case BOOT_TAG_EFLAG:
					g_boot_arg->e_flag = tags->u.eflag.e_flag;
					break;
				case BOOT_TAG_DDR_RESERVE:
					g_boot_arg->ddr_reserve_enable = tags->u.ddr_reserve.ddr_reserve_enable;
					g_boot_arg->ddr_reserve_success = tags->u.ddr_reserve.ddr_reserve_success;
					break;
				case BOOT_TAG_DRAM_BUF:
					g_boot_arg->dram_buf_size = tags->u.dram_buf.dram_buf_size;
					break;
				case BOOT_TAG_SRAM_INFO:
					g_boot_arg->non_secure_sram_addr = tags->u.sram_info.non_secure_sram_addr;
					g_boot_arg->non_secure_sram_size = tags->u.sram_info.non_secure_sram_size;
					break;
				case BOOT_TAG_PTP:
					memcpy(&g_boot_arg->ptp_volt_info, &tags->u.ptp_volt.ptp_volt_info, sizeof(ptp_info_t));
					break;
				case BOOT_TAG_PL_VERSION:
					memcpy(&g_boot_arg->pl_version, &tags->u.pl_version.pl_version, sizeof(tags->u.pl_version.pl_version));
					break;
				default:
					break;
			}
		}
	}


#ifdef MACH_FPGA
	g_nr_bank = 2;
	bi_dram[0].start = DRAM_PHY_ADDR + RIL_SIZE;
	bi_dram[0].size = (256 * 1024 * 1024) - RIL_SIZE;
	bi_dram[1].start = bi_dram[0].start + bi_dram[0].size;
	bi_dram[1].size = (256 * 1024 * 1024);
#else
	g_nr_bank = g_boot_arg->dram_rank_num;

	if (g_nr_bank == 0 || g_nr_bank > MAX_NR_BANK) {
		g_dram_init_ret = -1;
		//dprintf(CRITICAL, "[LK ERROR] DRAM bank number is not correct!!!");
		//while (1) ;
		return -1;
	}

	/* return the actual DRAM info */
	for (i = 0; i < g_boot_arg->mblock_info.mblock_num; i++) {
		bi_dram[i].start = g_boot_arg->mblock_info.mblock[i].start;
		bi_dram[i].size = g_boot_arg->mblock_info.mblock[i].size;
	}

#endif

	return 0;
}

/*******************************************************
 * Routine: memory_size
 * Description: return DRAM size to LCM driver
 ******************************************************/
u64 physical_memory_size(void)
{
	int i;
	unsigned long long size = 0;

	for (i = 0; i < g_boot_arg->orig_dram_info.rank_num; i++) {
		size += g_boot_arg->orig_dram_info.rank_info[i].size;
	}

	return size;
}

u32 memory_size(void)
{
	unsigned long long size = physical_memory_size();

	while (((unsigned long long)DRAM_PHY_ADDR + size) > 0x100000000ULL) {
		size -= (unsigned long long)(1024*1024*1024);
	}

	return (unsigned int)size;
}

void sw_env()
{
#ifdef LK_DL_CHECK
#ifdef MTK_EMMC_SUPPORT
	int dl_status = 0;
	dl_status = mmc_get_dl_info();
	dprintf(INFO, "mt65xx_sw_env--dl_status: %d\n", dl_status);
	if (dl_status != 0) {
		video_printf("=> TOOL DL image Fail!\n");
		dprintf(CRITICAL, "TOOL DL image Fail\n");
#ifdef LK_DL_CHECK_BLOCK_LEVEL
		dprintf(CRITICAL, "uboot is blocking by dl info\n");
		while (1) ;
#endif
	}
#endif
#endif

#ifndef USER_BUILD
	switch (g_boot_mode) {
		case META_BOOT:
			video_printf(" => META MODE\n");
			break;
		case FACTORY_BOOT:
			video_printf(" => FACTORY MODE\n");
			break;
		case RECOVERY_BOOT:
			video_printf(" => RECOVERY MODE\n");
			break;
		case SW_REBOOT:
			//video_printf(" => SW RESET\n");
			break;
		case NORMAL_BOOT:
			//if(g_boot_arg->boot_reason != BR_RTC && get_env("hibboot") != NULL && atoi(get_env("hibboot")) == 1)
			if (!g_boot_menu && get_env("hibboot") != NULL && atoi(get_env("hibboot")) == 1)
				video_printf(" => HIBERNATION BOOT\n");
			else
				video_printf(" => NORMAL BOOT\n");
			break;
		case ADVMETA_BOOT:
			video_printf(" => ADVANCED META MODE\n");
			break;
		case ATE_FACTORY_BOOT:
			video_printf(" => ATE FACTORY MODE\n");
			break;
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
		case KERNEL_POWER_OFF_CHARGING_BOOT:
			video_printf(" => POWER OFF CHARGING MODE\n");
			break;
		case LOW_POWER_OFF_CHARGING_BOOT:
			video_printf(" => LOW POWER OFF CHARGING MODE\n");
			break;
#endif
		case ALARM_BOOT:
			video_printf(" => ALARM BOOT\n");
			break;
		case FASTBOOT:
			video_printf(" => FASTBOOT mode...\n");
			break;
		default:
			video_printf(" => UNKNOWN BOOT\n");
	}
	return;
#endif

#ifdef USER_BUILD
	if (g_boot_mode == FASTBOOT)
		video_printf(" => FASTBOOT mode...\n");
	return;
#endif
}

void platform_init_mmu_mappings(void)
{
	unsigned int addr;
	unsigned int dram_size = 0;

	dram_init();
	arm_mmu_init();
	dram_size = physical_memory_size();

	/* map dram region */
	for (addr = 0; addr < dram_size; addr += (1024*1024)) {
		/*virtual to physical 1-1 mapping*/
		arm_mmu_map_section(bi_dram[0].start+addr,bi_dram[0].start+addr, MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | MMU_MEMORY_AP_READ_WRITE);
	}

	/* ummap TEE memory regoin */
	if (g_boot_arg->tee_reserved_mem.size != 0) {
		u64 j, k;
		k = g_boot_arg->tee_reserved_mem.start;
		j = ROUNDUP(g_boot_arg->tee_reserved_mem.size, 1024*1024);
		for (; j > 0; j -= (1024*1024)) {
			arm_mmu_map_section((uintptr_t)k , (uintptr_t)k, MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN);
			k += (1024*1024);
		}
	}
	arch_enable_mmu();  //enable mmu after setup page table to avoid cpu prefetch which may bring on emi violation
}

void platform_init_mmu(void)
{
	unsigned long long addr;
	unsigned int vaddr;
	unsigned long long dram_size;

	/* configure available RAM banks */
	dram_init();

	dram_size = physical_memory_size();

	if (((unsigned long long)DRAM_PHY_ADDR + dram_size) <= 0x100000000ULL) {
		arm_mmu_init();
		/* map dram region */
		for (addr = 0; addr < dram_size; addr += (1024*1024)) {
			/*virtual to physical 1-1 mapping*/
			arm_mmu_map_section(bi_dram[0].start+addr, bi_dram[0].start+addr, MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | MMU_MEMORY_AP_READ_WRITE);
		}

		/* ummap TEE memory regoin */
		if (g_boot_arg->tee_reserved_mem.size != 0) {
			u64 j, k;
			k = g_boot_arg->tee_reserved_mem.start;
			j = ROUNDUP(g_boot_arg->tee_reserved_mem.size, 1024*1024);
			for (; j > 0; j -= (1024*1024)) {
				arm_mmu_map_section((uintptr_t)k , (uintptr_t)k, MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN);
				k += (1024*1024);
			}
		}
	} else {
		arm_mmu_lpae_init();
		/* map dram region */
		for (addr = 0; addr < dram_size; addr += (unsigned long long)(1024*1024*1024)) {
			vaddr = (bi_dram[0].start + addr < 0x100000000ULL) ? (unsigned int)(bi_dram[0].start + addr) : (0xC0000000);
			arm_mmu_map_block(bi_dram[0].start+addr, vaddr, LPAE_MMU_MEMORY_TYPE_NORMAL_WRITE_BACK);
		}
		/* ummap TEE memory regoin */
		if (g_boot_arg->tee_reserved_mem.size != 0) {
			u64 j, k;
			k = g_boot_arg->tee_reserved_mem.start;
			j = ROUNDUP(g_boot_arg->tee_reserved_mem.size, 0x40000000ULL);
			for (; j > 0; j -= 0x40000000ULL) {
				arm_mmu_map_block((uintptr_t)k , (uintptr_t)k, LPAE_MMU_MEMORY_TYPE_NORMAL_WRITE_BACK | LPAE_MMU_MEMORY_XN);
				k += 0x40000000ULL;
			}
		}
	}
	arch_enable_mmu();  //enable mmu after setup page table to avoid cpu prefetch which may bring on emi violation
}



void platform_early_init(void)
{
#ifdef MT_SRAM_REPAIR_SUPPORT
	int repair_ret;
#endif
#ifdef LK_PROFILING
#ifdef MT_SRAM_REPAIR_SUPPORT
	unsigned int time_repair_sram;
#endif
	unsigned int time_wdt_early_init;
	unsigned int time_pmic_init;
	unsigned int time_platform_early_init;
#endif

	platform_init_interrupts();

	platform_early_init_timer();

#if defined(MACH_FPGA) || defined(SB_LK_BRINGUP)
	mtk_timer_init();  // GPT4 will be initialized at PL after
#endif

#ifdef LK_PROFILING
	time_platform_early_init = get_timer(0);
#endif

#ifndef MACH_FPGA
#ifndef DUMMY_AP
	mt_gpio_set_default();
#endif
#endif

	/* initialize the uart */
	uart_init_early();

	if (g_dram_init_ret < 0) {
		dprintf(CRITICAL, "[LK ERROR] DRAM bank number is not correct!!!\n");
		while (1) ;
	}

	//i2c_v1_init();

#ifdef LK_PROFILING
	time_wdt_early_init = get_timer(0);
#endif
	mtk_wdt_init();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- WDT Init  takes %d ms -------- \n", (int)get_timer(time_wdt_early_init));
#endif

#ifdef MT_SRAM_REPAIR_SUPPORT
#ifdef LK_PROFILING
	time_repair_sram = get_timer(0);
#endif
	repair_ret = repair_sram();
	if (repair_ret != 0) {
		dprintf(CRITICAL, "Sram repair failed %d\n", repair_ret);
		while (1);
	}
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- Repair SRAM takes %d ms -------- \n", (int)get_timer(time_repair_sram));
#endif
#endif

	//i2c init
	i2c_hw_init();

#if defined(MACH_FPGA) || defined(SB_LK_BRINGUP)
	//mtk_timer_init();  // GPT4 will be initialized at PL after
	mtk_wdt_disable();  // WDT will be triggered when uncompressing linux image on FPGA
	*(volatile unsigned int *)0x10212000 = 0x22000064;
#endif

#if MTK_MT8193_SUPPORT
	mt8193_init();
#endif

	// Workaround by Peng
	//pwrap_init_lk();
	//pwrap_init_for_early_porting();

#ifdef LK_PROFILING
	time_pmic_init = get_timer(0);
#endif
	pmic_init();
	/*
	    // Workaround by Weiqi
	    mt6331_upmu_set_rg_vgp1_en(1);
	    mt6331_upmu_set_rg_vcam_io_en(1);
	*/
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- pmic_init takes %d ms -------- \n", (int)get_timer(time_pmic_init));
#endif

#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- platform_early_init takes %d ms -------- \n", (int)get_timer(time_platform_early_init));
#endif
}

extern void mt65xx_bat_init(void);
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)

int kernel_charging_boot(void)
{
	if ((g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT) && upmu_is_chr_det() == KAL_TRUE) {
		dprintf(INFO,"[%s] Kernel Power Off Charging with Charger/Usb \n", __func__);
		return  1;
	} else if ((g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT) && upmu_is_chr_det() == KAL_FALSE) {
		dprintf(INFO,"[%s] Kernel Power Off Charging without Charger/Usb \n", __func__);
		return -1;
	} else
		return 0;
}
#endif


void platform_init(void)
{
#ifdef LK_PROFILING
	unsigned int time_nand_emmc;
	unsigned int time_led_init;
	unsigned int time_env;
	unsigned int time_disp_init;
	unsigned int time_load_logo;
	unsigned int time_backlight;
	unsigned int time_boot_mode;
#ifdef MTK_SECURITY_SW_SUPPORT
	unsigned int time_security_init;
#endif
	unsigned int time_bat_init;
	unsigned int time_RTC_boot_Check;
	unsigned int time_show_logo;
	unsigned int time_sw_env;
	unsigned int time_platform_init;

	time_platform_init = get_timer(0);
#endif

	dprintf(CRITICAL," ==LK info ==\n");
	dprintf(CRITICAL," Build time:%s, %s\n",  __DATE__,__TIME__);
	dprintf(CRITICAL," chip_code[0x%x]\n",  mt_get_chip_hw_code());
	dprintf(CRITICAL," chip_ver[0x%x]\n",   mt_get_chip_sw_ver());
	dprintf(CRITICAL," ==LK info ==\n");
	dprintf(CRITICAL, "platform_init()\n");

#ifdef DUMMY_AP
	dummy_ap_entry();
#endif

#ifdef LK_PROFILING
	time_nand_emmc = get_timer(0);
#endif
#ifdef MTK_EMMC_SUPPORT
	mmc_legacy_init(1);
#else
#ifndef MACH_FPGA
	nand_init();
	nand_driver_test();
#endif
#endif
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- NAND/EMMC init takes %d ms -------- \n", (int)get_timer(time_nand_emmc));
#endif
#if defined(CFG_DTB_EARLY_LOADER_SUPPORT)
	if (bldr_load_dtb()<0)
		dprintf(CRITICAL, "bldr_load_dtb fail\n");
#endif

#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_led_init = get_timer(0);
#endif
	leds_init();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- led init takes %d ms -------- \n", (int)get_timer(time_led_init));
#endif
#endif

#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if ((g_boot_arg->boot_reason == BR_USB) && (upmu_is_chr_det() == KAL_FALSE)) {
		dprintf(INFO, "[%s] Unplugged Charger/Usb between Pre-loader and Uboot in Kernel Charging Mode, Power Off \n", __func__);
		mt6575_power_off();
	}
#endif

#ifdef LK_PROFILING
	time_env = get_timer(0);
#endif
	env_init();
	print_env();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- ENV init takes %d ms -------- \n", (int)get_timer(time_env));
#endif

#ifdef LK_PROFILING
	time_disp_init = get_timer(0);
#endif

	/* initialize the frame buffet information */
#ifndef MACH_FPGA_NO_DISPLAY
	g_fb_size = mt_disp_get_vram_size();
#else
	g_fb_size = 0x1000000;
#endif
#if 0
	g_fb_base = memory_size() - g_fb_size + DRAM_PHY_ADDR;
#else

#if 0
	if (g_is_64bit_kernel) {
		g_fb_base = mblock_reserve(&g_boot_arg->mblock_info, g_fb_size, 0x200000, 0x100000000, RANKMAX);
		g_fb_base = ALIGN_TO(g_fb_base,0x200000); // size 2MB align
	} else {
		g_fb_base = mblock_reserve(&g_boot_arg->mblock_info, g_fb_size, 0x100000, 0x100000000, RANKMAX);
	}
#else
	g_fb_base = mblock_reserve(&g_boot_arg->mblock_info, g_fb_size, 0x10000, 0x100000000, RANKMAX);
#endif

	if (!g_fb_base) {
		/* ERROR */
	}
#endif

	dprintf(CRITICAL, "FB base = 0x%x, FB size = %d\n", g_fb_base, g_fb_size);

#ifndef MACH_FPGA_NO_DISPLAY
	mt_disp_init((void *)g_fb_base);
	/* show black picture fisrtly in case of  backlight is on before nothing is drawed*/
	mt_disp_fill_rect(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT, 0x0);
	mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- disp init takes %d ms -------- \n", (int)get_timer(time_disp_init));
#endif

#ifdef MTK_SECURITY_SW_SUPPORT
#ifdef LK_PROFILING
	time_security_init = get_timer(0);
#endif
	/* initialize security library */
#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
	sec_func_init(3);
#else
	sec_func_init(1);
#endif
#else
	sec_func_init(0);
#endif
#ifdef LK_PROFILING
	dprintf(INFO,"[PROFILE] ------- Security init takes %d ms -------- \n", (int)get_timer(time_security_init));
#endif

	seclib_set_oemkey(g_oemkey, OEM_PUBK_SZ);
	/*Verify logo before use it*/
	if ( 0 != sec_logo_check() ) {
		dprintf(CRITICAL,"<ASSERT> %s:line %d\n",__FILE__,__LINE__);
		while (1);
	}
#endif

#ifdef LK_PROFILING
	time_load_logo = get_timer(0);
#endif
	drv_video_init();

	mboot_common_load_logo((unsigned long)mt_get_logo_db_addr_pa(), "logo");
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- load_logo takes %d ms -------- \n", (int)get_timer(time_load_logo));
#endif
#endif

	/*for kpd pmic mode setting*/
	set_kpd_pmic_mode();

#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_boot_mode = get_timer(0);
#endif
	boot_mode_select();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- boot mode select takes %d ms -------- \n", (int)get_timer(time_boot_mode));
#endif
#endif


	/*Show download logo & message on screen */
	if (g_boot_arg->boot_mode == DOWNLOAD_BOOT) {
		dprintf(CRITICAL, "[LK] boot mode is DOWNLOAD_BOOT\n");

#ifdef MTK_SECURITY_SW_SUPPORT
		/* verify da before jumping to da*/
		if (sec_usbdl_enabled()) {
			u8  *da_addr = (u8*)g_boot_arg->da_info.addr;
			u32 da_len   = g_boot_arg->da_info.len;
			u32 sig_len  = g_boot_arg->da_info.sig_len;
			u8  *sig_addr = (unsigned char *)da_addr + (da_len - sig_len);

			if (da_len == 0 || sig_len == 0) {
				dprintf(INFO, "[LK] da argument is invalid\n");
				dprintf(INFO, "da_addr = 0x%x\n", (int)da_addr);
				dprintf(INFO, "da_len  = 0x%x\n", da_len);
				dprintf(INFO, "sig_len = 0x%x\n", sig_len);
			}

			if (sec_usbdl_verify_da(da_addr, (da_len - sig_len), sig_addr, sig_len)) {
				/* da verify fail */
				video_printf(" => Not authenticated tool, download stop...\n");
				while (1); /* fix me, should not be infinite loop in lk */
			}
		} else
#endif
		{
			dprintf(INFO, " DA verification disabled...\n");
		}

#ifndef MACH_FPGA_NO_DISPLAY
		mt_disp_show_boot_logo();
#endif
		video_printf(" => Downloading...\n");
		dprintf(CRITICAL, "enable backlight after show bootlogo! \n");
		mt65xx_backlight_on();

		mtk_wdt_disable(); //Disable wdt before jump to DA
		platform_uninit();
#ifdef HAVE_CACHE_PL310
		l2_disable();
#endif
		arch_disable_cache(UCACHE);
		arch_disable_mmu();
#ifdef ENABLE_L2_SHARING
		config_shared_SRAM_size();
#endif

		jump_da(g_boot_arg->da_info.addr, g_boot_arg->da_info.arg1, g_boot_arg->da_info.arg2);
	}

#ifdef LK_PROFILING
	time_bat_init = get_timer(0);
#endif
	mt65xx_bat_init();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- battery init takes %d ms -------- \n", (int)get_timer(time_bat_init));
#endif

#ifndef CFG_POWER_CHARGING
#ifdef LK_PROFILING
	time_RTC_boot_Check = get_timer(0);
#endif
	/* NOTE: if define CFG_POWER_CHARGING, will rtc_boot_check() in mt65xx_bat_init() */
	rtc_boot_check(false);
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- RTC boot check Init  takes %d ms -------- \n", (int)get_timer(time_RTC_boot_Check));
#endif
#endif

#ifdef LK_PROFILING
	time_show_logo = get_timer(0);
#endif
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if (kernel_charging_boot() == 1) {
#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
		CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;
		CHR_Type_num = hw_charging_get_charger_type();
		if ((g_boot_mode != LOW_POWER_OFF_CHARGING_BOOT) ||
		        ((CHR_Type_num != STANDARD_HOST) && (CHR_Type_num != NONSTANDARD_CHARGER))) {
#endif
			mt_disp_power(TRUE);
			mt_disp_show_low_battery();
			mt65xx_leds_brightness_set(6, 110);
#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
		}
#endif
	} else if (g_boot_mode != KERNEL_POWER_OFF_CHARGING_BOOT && g_boot_mode != LOW_POWER_OFF_CHARGING_BOOT) {
		if (g_boot_mode != ALARM_BOOT && (g_boot_mode != FASTBOOT)) {
#ifndef MACH_FPGA_NO_DISPLAY
			mt_disp_show_boot_logo();
#endif
		}
	}
#else
	if (g_boot_mode != ALARM_BOOT && (g_boot_mode != FASTBOOT)) {
#ifndef MACH_FPGA_NO_DISPLAY
		mt_disp_show_boot_logo();
#endif
	}
#endif
#ifdef LK_PROFILING
	time_backlight = get_timer(0);
#endif

#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
	if (!is_low_battery(0)) {
#endif
		mt65xx_backlight_on();
#ifndef MACH_FPGA_NO_DISPLAY
		//pwm need display sof
		mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
#endif
#ifdef MTK_BATLOWV_NO_PANEL_ON_EARLY
	}
#endif

#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- backlight takes %d ms -------- \n", (int)get_timer(time_backlight));
#endif

#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- show logo takes %d ms -------- \n", (int)get_timer(time_show_logo));
#endif

#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_sw_env = get_timer(0);
#endif
	sw_env();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- sw_env takes %d ms -------- \n", (int)get_timer(time_sw_env));
#endif
#endif

#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- platform_init takes %d ms -------- \n", (int)get_timer(time_platform_init));
#endif
}

void platform_uninit(void)
{
#ifndef MACH_FPGA
	leds_deinit();
#endif
	platform_deinit_interrupts();
	return;
}

#ifdef ENABLE_L2_SHARING
#define ADDR_CA7L_CACHE_CONFIG_MP(x) (CA7MCUCFG_BASE + 0x200 * x)
#define L2C_SIZE_CFG_OFFSET  8
#define L2C_SHARE_EN_OFFSET  12
/* 4'b1111: 2048KB(not support)
 * 4'b0111: 1024KB(not support)
 * 4'b0011: 512KB
 * 4'b0001: 256KB
 * 4'b0000: 128KB (not support)
 */

int is_l2_need_config(void)
{
	volatile unsigned int cache_cfg, addr;

	addr = ADDR_CA7L_CACHE_CONFIG_MP(0);
	cache_cfg = DRV_Reg32(addr);
	cache_cfg = cache_cfg >> L2C_SIZE_CFG_OFFSET;

	/* only read 256KB need to be config.*/
	if ((cache_cfg &(0x7)) == 0x1) {
		return 1;
	}
	return 0;
}

void cluster_l2_share_enable(int cluster)
{
	volatile unsigned int cache_cfg, addr;

	addr = ADDR_CA7L_CACHE_CONFIG_MP(cluster);
	/* set L2C size to 256KB */
	cache_cfg = DRV_Reg32(addr);
	cache_cfg &= (~0x7) << L2C_SIZE_CFG_OFFSET;
	cache_cfg |= 0x1 << L2C_SIZE_CFG_OFFSET;

	/* enable L2C_share_en. Sram only for other to use*/
	cache_cfg |= (0x1 << L2C_SHARE_EN_OFFSET);
	DRV_WriteReg32(addr, cache_cfg);
}

void cluster_l2_share_disable(int cluster)
{
	volatile unsigned int cache_cfg, addr;

	addr = ADDR_CA7L_CACHE_CONFIG_MP(cluster);
	/* set L2C size to 512KB */
	cache_cfg = DRV_Reg32(addr);
	cache_cfg &= (~0x7) << L2C_SIZE_CFG_OFFSET;
	cache_cfg |= 0x3 << L2C_SIZE_CFG_OFFSET;
	DRV_WriteReg32(addr, cache_cfg);

	/* disable L2C_share_en. Sram only for cpu to use*/
	cache_cfg &= ~(0x1 << L2C_SHARE_EN_OFFSET);
	DRV_WriteReg32(addr, cache_cfg);
}

/* config L2 cache and sram to its size */
void config_L2_size(void)
{
	int cluster;

	if (is_l2_need_config()) {
		/*
		 * Becuase mcu config is protected.
		 * only can write in secutity mode
		 */

		if (dev_info_nr_cpu() == 6) {
			cluster_l2_share_disable(0);
			cluster_l2_share_enable(1);
		}

		else {
			for (cluster = 0; cluster < 2; cluster++) {
				cluster_l2_share_disable(cluster);
			}
		}
	}
}

/* config SRAM back from L2 cache for DA relocation */
void config_shared_SRAM_size(void)
{
	int cluster;

	if (is_l2_need_config()) {
		/*
		 * Becuase mcu config is protected.
		 * only can write in secutity mode
		 */

		for (cluster = 0; cluster < 2; cluster++) {
			cluster_l2_share_enable(cluster);
		}
	}
}
#endif

/*
 * reserve a memory from mblock
 * @mblock_info: address of mblock_info
 * @reserved_size: size of memory
 * @align: alignment
 * @limit: address limit. Must higher than return address + reserved_size
 * @rank: preferable rank, the returned address is in rank or lower ranks
 * It returns as high rank and high address as possible. (consider rank first)
 */
u64 mblock_reserve(mblock_info_t *mblock_info, u64 reserved_size, u64 align, u64 limit,
                   enum reserve_rank rank)
{
	unsigned int i, max_rank;
	int target = -1;
	u64 start, end, sz, max_addr = 0;
	u64 reserved_addr = 0;
	mblock_t mblock;

	if (mblock_info->mblock_num == 128) {
		/* the mblock[] is full */
		dprintf(CRITICAL,"mblock_reserve error: mblock[] is full\n");
		return 0;
	}

	if (!align)
		align = 0x1000;
	/* must be at least 4k aligned */
	if (align & (0x1000 - 1))
		align &= ~(0x1000 - 1);

	if (rank == RANK0) {
		/* reserve memory from rank 0 */
		max_rank = 0;
	} else {
		/* reserve memory from any possible rank */
		/* mblock_num >= nr_ranks is true */
		max_rank = mblock_info->mblock_num - 1;
	}

	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		end = limit < (start + sz)? limit: (start + sz);
		reserved_addr = (end - reserved_size);
		reserved_addr &= ~(align - 1);
		dprintf(CRITICAL,"mblock[%d].start: 0x%llx, sz: 0x%llx, limit: 0x%llx, "
		        "max_addr: 0x%llx, max_rank: %d, target: %d, "
		        "mblock[].rank: %d, reserved_addr: 0x%llx,"
		        "reserved_size: 0x%llx\n",
		        i, start, sz, limit, max_addr, max_rank,
		        target, mblock_info->mblock[i].rank,
		        reserved_addr, reserved_size);
		dprintf(CRITICAL,"mblock_reserve dbg[%d]: %d, %d, %d, %d, %d\n",
		        i, (reserved_addr + reserved_size < start + sz),
		        (reserved_addr >= start),
		        (mblock_info->mblock[i].rank <= max_rank),
		        (start + sz > max_addr),
		        (reserved_addr + reserved_size <= limit));
		if ((reserved_addr + reserved_size <= start + sz) &&
		        (reserved_addr >= start) &&
		        (mblock_info->mblock[i].rank <= max_rank) &&
		        (start + sz > max_addr) &&
		        (reserved_addr + reserved_size <= limit)) {
			max_addr = start + sz;
			target = i;
		}
	}

	if (target < 0) {
		dprintf(CRITICAL,"mblock_reserve error\n");
		return 0;
	}

	start = mblock_info->mblock[target].start;
	sz = mblock_info->mblock[target].size;
	end = limit < (start + sz)? limit: (start + sz);
	reserved_addr = (end - reserved_size);
	reserved_addr &= ~(align - 1);

	/* split mblock if necessary */
	if (reserved_addr == start) {
		/*
		 * only needs to fixup target mblock
		 * [reserved_addr, reserved_size](reserved) +
		 * [reserved_addr + reserved_size, sz - reserved_size]
		 */
		mblock_info->mblock[target].start = reserved_addr + reserved_size;
		mblock_info->mblock[target].size -= reserved_size;
	} else if ((reserved_addr + reserved_size) == (start + sz)) {
		/*
		 * only needs to fixup target mblock
		 * [start, reserved_addr - start] +
		 * [reserved_addr, reserved_size](reserved)
		 */
		mblock_info->mblock[target].size = reserved_addr - start;
	} else {
		/*
		 * fixup target mblock and create a new mblock
		 * [start, reserved_addr - start] +
		 * [reserved_addr, reserved_size](reserved) +
		 * [reserved_addr + reserved_size, start + sz - reserved_addr - reserved_size]
		 */
		/* fixup original mblock */
		mblock_info->mblock[target].size = reserved_addr - start;

		/* new mblock */
		mblock.rank =  mblock_info->mblock[target].rank;
		mblock.start = reserved_addr + reserved_size;
		mblock.size = start + sz - (reserved_addr + reserved_size);

		/* insert the new node, keep the list sorted */
		memmove(&mblock_info->mblock[target + 2],
		        &mblock_info->mblock[target + 1],
		        sizeof(mblock_t) *
		        (mblock_info->mblock_num - target - 1));
		mblock_info->mblock[target + 1] = mblock;
		mblock_info->mblock_num += 1;
		dprintf(CRITICAL, "mblock[%d]: %llx, %llx from mblock\n"
		        "mblock[%d]: %llx, %llx from mblock\n",
		        target,
		        mblock_info->mblock[target].start,
		        mblock_info->mblock[target].size,
		        target + 1,
		        mblock_info->mblock[target + 1].start,
		        mblock_info->mblock[target + 1].size);
	}

	dprintf(CRITICAL,"mblock_reserve: %llx - %llx from mblock %d\n",
	        reserved_addr, reserved_addr + reserved_size,
	        target);

	/* print debug info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		dprintf(CRITICAL,"mblock-debug[%d].start: 0x%llx, sz: 0x%llx\n",
		        i, start, sz);
	}

	return reserved_addr;
}

/*
 * mblock_resize - resize mblock started at addr from oldsize to newsize,
 * current implementation only consider oldsize >= newsize.
 *
 * @mblock_info: mblock information
 * @orig_dram_info: original dram information
 * @addr: start address of a mblock
 * @oldsize: origianl size of the mblock
 * @newsize: new size of the given block
 * return 0 on success, otherwise 1
 */
int mblock_resize(mblock_info_t *mblock_info, dram_info_t *orig_dram_info,
                  u64 addr, u64 oldsize, u64 newsize)
{
	int err = 1;
	unsigned int i;
	u64 start, sz;
	mblock_t mblock;

	/* check size, oldsize must larger than newsize */
	if (oldsize <= newsize) {
		dprintf(CRITICAL,"mblock_resize error: mblock %llx oldsize(%llx) <= newsize(%llx)",
		        addr, oldsize, newsize);
		goto error;
	}

	/* check alignment, at least 4k aligned */
	if ((oldsize & (0x1000 - 1)) || (newsize & (0x1000 - 1))) {
		dprintf(CRITICAL,"mblock_resize alignment error: oldsize(%llx) or newsize(%llx)\n",
		        oldsize, newsize);
		goto error;
	}

	/* check mblock */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		/* invalid mblock */
		if ((addr >= start) && ((addr + oldsize) <= (start + sz))) {
			dprintf(CRITICAL,"mblock_resize error: mblock %llx, size: %llx is free\n",
			        addr, oldsize);
			goto error;
		}
	}

	/*
	 * ok, the mblock is valid and oldsize > newsize, let's
	 * shrink this mblock
	 */
	/* setup a new mblock */
	mblock.start = addr + newsize;
	mblock.size = oldsize - newsize;
	dprintf(CRITICAL,"mblock_resize putback mblock %llx size: %llx\n",
	        mblock.start, mblock.size);
	/* setup rank */
	for (i = 0; i < orig_dram_info->rank_num; i++) {
		start = orig_dram_info->rank_info[i].start;
		sz = orig_dram_info->rank_info[i].size;
		if ((mblock.start >= start) && ((mblock.start + mblock.size) <= (start + sz))) {
			mblock.rank = i;
			break;
		}
	}
	if (i >= orig_dram_info->rank_num) {
		dprintf(CRITICAL,"mblock_resize error: mblock not in orig_dram_info: %llx, size(%llx)\n",
		        mblock.start, mblock.size);
		goto error;
	}

	/* put the mblock back to mblock_info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		if (mblock.rank == mblock_info->mblock[i].rank) {
			if (mblock.start == start + sz) {
				/*
				 * the new mblock can be merged to this mblock
				 * [start, start + sz] +
				 * [mblock.start, mblock.start + mblock.size](new)
				 */
				mblock_info->mblock[i].size += mblock.size;
				/* destroy block */
				mblock.size = 0;
			} else if (start == mblock.start + mblock.size) {
				/*
				 * the new mblock can be merged to this mblock
				 * [mblock.start, mblock.start + * mblock.size](new) +
				 * [start, start + sz]
				 */
				mblock_info->mblock[i].start = mblock.start;
				mblock_info->mblock[i].size += mblock.size;
				/* destroy block */
				mblock.size = 0;
			}
		}
	}

	/*
	 * mblock cannot be merge info mblock_info, insert it into mblock_info
	 */
	if (mblock.size) {
		for (i = 0; i < mblock_info->mblock_num; i++) {
			if (mblock.start < mblock_info->mblock[i].start)
				break;
		}
		memmove(&mblock_info->mblock[i + 1],
		        &mblock_info->mblock[i],
		        sizeof(mblock_t) *
		        (mblock_info->mblock_num - i));
		mblock_info->mblock[i] = mblock;
		mblock_info->mblock_num += 1;
	}

	/* print debug info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		dprintf(CRITICAL,"mblock-resize-debug[%d].start: 0x%llx, sz: 0x%llx\n",
		        i, start, sz);
	}

	return 0;
error:
	return err;
}

/*
 * mblock_create - create mblock started at addr or merge with existing mblock
 *
 * @mblock_info: mblock information
 * @orig_dram_info: original dram information
 * @addr: start address of a mblock, must be 4k align
 * @size: size of the given block, must be 4K align
 * return 0 on success, otherwise 1
 */
int mblock_create(mblock_info_t *mblock_info, dram_info_t *orig_dram_info
                  , u64 addr, u64 size)
{
	int err = 1;
	unsigned i,valid;
	u64 start, sz;
	mblock_t mblock;
	mblock_t *mblock_candidate_left = NULL, *mblock_candidate_right = NULL;

	/* check size, addr valid and align with 4K*/
	if (!size || size&(0x1000 - 1) || addr&(0x1000 - 1) ) {
		dprintf(CRITICAL,"mblock_create size invalid size=%llx\n",size);
		goto error;
	}
	/* for lca check*/
	if (g_boot_arg->lca_reserved_mem.start && g_boot_arg->lca_reserved_mem.size) {
		if (addr >= g_boot_arg->lca_reserved_mem.start || addr + size > g_boot_arg->lca_reserved_mem.start) {
			dprintf(CRITICAL,"mblock_create ERROR , overlap with LCA addr and size invalid addr = %llx size=%llx\n", addr, size);
			goto error;
		}
	}

	/* for tee check*/
	if (g_boot_arg->tee_reserved_mem.start && g_boot_arg->tee_reserved_mem.size) {
		if (addr >= g_boot_arg->tee_reserved_mem.start || addr + size > g_boot_arg->tee_reserved_mem.start) {
			dprintf(CRITICAL,"mblock_create ERROR , overlap with TEE addr and size invalid addr = %llx size=%llx\n", addr, size);
			goto error;
		}
	}

	/*it's not allow to create mblock which is cross rank
	 * and mblock should not exceed rank size */
	for (i = 0, valid = 0; i < orig_dram_info->rank_num; i++) {
		start = orig_dram_info->rank_info[i].start;
		sz = orig_dram_info->rank_info[i].size;
		if (addr >= start && addr < start + sz && addr + size <= start + sz) {
			valid =1;
			break;
		}
	}
	if (!valid) {
		dprintf(CRITICAL,"mblock_create addr and size invalid addr=%llx size=%llx\n",
		        addr,size);
		goto error;
	}

	/* check every mblock the addr and size should not be within any existing mblock */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		/*addr should start from reserved memory space and addr + size should not overlap with mblock
		 * when addr is smaller than start*/
		if (((addr >= start) && (addr < start + sz)) || (addr < start && addr + size > start)) {
			dprintf(CRITICAL,"mblock_create error: addr %llx overlap with mblock %llx, size: %llx \n",
			        addr, start, sz);
			goto error;
		}
	}

	/*
	 * ok, the mblock is valid let's create the mblock
	 * and try to merge it with the same bank and choose the bigger size one
	 */
	/* setup a new mblock */
	mblock.start = addr;
	mblock.size = size;
	dprintf(CRITICAL,"mblock_create mblock start %llx size: %llx\n",
	        mblock.start, mblock.size);
	/* setup rank */
	for (i = 0; i < orig_dram_info->rank_num; i++) {
		start = orig_dram_info->rank_info[i].start;
		sz = orig_dram_info->rank_info[i].size;
		if ((mblock.start >= start) && ((mblock.start + mblock.size) <= (start + sz))) {
			mblock.rank = i;
			break;
		}
	}
	if (i >= orig_dram_info->rank_num) {
		dprintf(CRITICAL,"mblock_create error: mblock not in orig_dram_info: %llx, size(%llx)\n",
		        mblock.start, mblock.size);
		goto error;
	}

	/* put the mblock back to mblock_info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		if (mblock.rank == mblock_info->mblock[i].rank) {
			if (mblock.start + mblock.size == start) {
				/*
				 * the new mblock could be merged to this mblock
				 */
				mblock_candidate_right = &mblock_info->mblock[i];
			} else if (start + sz == mblock.start) {
				/*
				 * the new mblock can be merged to this mblock
				 */
				mblock_candidate_left =  &mblock_info->mblock[i];
			}
		}
	}
	/*we can merge either left or right , choose the bigger one */
	if (mblock_candidate_right && mblock_candidate_left) {
		if (mblock_candidate_right->size >= mblock_candidate_left->size) {
			dprintf(CRITICAL,"mblock_candidate_right->size = %llx \
				mblock_candidate_left->size = %llx \n",mblock_candidate_right->size, mblock_candidate_left->size);
			mblock_candidate_right->start = mblock.start;
			mblock_candidate_right->size += mblock.size;
		} else { /*left bigger*/
			dprintf(CRITICAL,"mblock_candidate_right->size = %llx \
				mblock_candidate_left->size = %llx \n",mblock_candidate_right->size, mblock_candidate_left->size);
			mblock_candidate_left->size += mblock.size;
		}
		/* destroy block */
		mblock.size = 0;
	} else {
		if (mblock_candidate_right) {
			mblock_candidate_right->start = mblock.start;
			mblock_candidate_right->size += mblock.size;
			/* destroy block */
			mblock.size = 0;
		}

		if (mblock_candidate_left) {
			mblock_candidate_left->size += mblock.size;
			/* destroy block */
			mblock.size = 0;
		}
	}

	/*
	 * mblock cannot be merge into mblock_info, insert it into mblock_info
	 */
	if (mblock.size) {
		for (i = 0; i < mblock_info->mblock_num; i++) {
			if (mblock.start < mblock_info->mblock[i].start)
				break;
		}
		/* insert the new node, keep the list sorted */
		if (i != mblock_info->mblock_num) {
			memmove(&mblock_info->mblock[i + 1],
			        &mblock_info->mblock[i],
			        sizeof(mblock_t) *
			        (mblock_info->mblock_num - i));
		}
		mblock_info->mblock[i] = mblock;
		mblock_info->mblock_num += 1;
		dprintf(CRITICAL, "create mblock[%d]: %llx, %llx \n",
		        i,
		        mblock_info->mblock[i].start,
		        mblock_info->mblock[i].size);
	}

	/* print debug info */
	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		dprintf(CRITICAL,"mblock-create-debug[%d].start: 0x%llx, sz: 0x%llx\n",
		        i, start, sz);
	}

	return 0;
error:
	return err;
}

int fdt_memory_append(void *fdt)
{
	char *ptr;
	int offset;
	int ret = 0;

	offset = fdt_path_offset(fdt, "/memory");

	if (offset < 0 ) {
		dprintf(CRITICAL,"%s:[%d] get fdt_path_offset of memory failed \n",__func__,__LINE__);
		ret = offset;
		goto exit;
	}

	ptr = (char *)&g_boot_arg->orig_dram_info;
	ret = fdt_setprop(fdt, offset, "orig_dram_info", ptr, sizeof(dram_info_t));
	if (ret) goto exit;

	ptr = (char *)&g_boot_arg->mblock_info;
	ret = fdt_setprop(fdt, offset, "mblock_info", ptr, sizeof(mblock_info_t));
	if (ret) goto exit;

	ptr = (char *)&g_boot_arg->lca_reserved_mem;
	ret = fdt_setprop(fdt, offset, "lca_reserved_mem", ptr, sizeof(mem_desc_t));
	if (ret) goto exit;

	ptr = (char *)&g_boot_arg->tee_reserved_mem;
	ret = fdt_setprop(fdt, offset, "tee_reserved_mem", ptr, sizeof(mem_desc_t));
	if (ret) goto exit;

exit:

	if (ret)
		return 1;

	return 0;
}

#if 0
/*
 * reserve a memory from mblock
 * @mblock_info: address of mblock_info
 * @size: size of memory
 * @align: alignment, not implemented
 * @limit: address limit. Must higher than return address + size
 * @rank: preferable rank, the returned address is in rank or lower ranks
 * It returns as high rank and high address as possible. (consider rank first)
 */
u64 mblock_reserve(mblock_info_t *mblock_info, u64 size, u64 align, u64 limit,
                   enum reserve_rank rank)
{
	int i, max_rank, target = -1;
	u64 start, sz, max_addr = 0;

	if (size & (0x200000 - 1)) {
		dprintf(CRITICAL,"warning: size is not 2MB aligned\n");
	}

	if (rank == RANK0) {
		/* reserve memory from rank 0 */
		max_rank = 0;
	} else {
		/* reserve memory from any possible rank */
		/* mblock_num >= nr_ranks is true */
		max_rank = mblock_info->mblock_num - 1;
	}

	for (i = 0; i < mblock_info->mblock_num; i++) {
		start = mblock_info->mblock[i].start;
		sz = mblock_info->mblock[i].size;
		dprintf(CRITICAL,"mblock[%d].start: 0x%llx, sz: 0x%llx, limit: 0x%llx, "
		        "max_addr: 0x%llx, max_rank: %d, target: %d, "
		        "mblock[].rank: %d\n",
		        i, start, sz, limit, max_addr, max_rank,
		        target, mblock_info->mblock[i].rank);
		dprintf(CRITICAL,"mblock_reserve dbg[%d]: %d, %d, %d, %d\n",
		        i, (start + sz <= limit),
		        (mblock_info->mblock[i].rank <= max_rank),
		        (start + sz > max_addr),
		        (sz >= size));
		if ((start + sz <= limit) &&
		        (mblock_info->mblock[i].rank <= max_rank) &&
		        (start + sz > max_addr) &&
		        (sz >= size)) {
			max_addr = start + sz;
			target = i;
		}
	}

	if (target < 0) {
		dprintf(CRITICAL,"mblock_reserve error\n");
		return 0;
	}

	mblock_info->mblock[target].size -= size;

	dprintf(CRITICAL,"mblock_reserve: %llx - %llx from mblock %d\n",
	        (mblock_info->mblock[target].start
	         + mblock_info->mblock[target].size),
	        (mblock_info->mblock[target].start
	         + mblock_info->mblock[target].size + size),
	        target);


	return mblock_info->mblock[target].start +
	       mblock_info->mblock[target].size;
}
#endif //NEW_MEMORY_RESERVED_MODEL

int platform_skip_hibernation(void)
{
	if (g_boot_menu == true)
		return 1;

	switch (g_boot_arg->boot_reason) {
#if 0 // let schedule power on to go hiberantion bootup process
		case BR_RTC:
#endif
		case BR_WDT:
		case BR_WDT_BY_PASS_PWK:
		case BR_WDT_SW:
		case BR_WDT_HW:
			return 1;
	}

	return 0;
}
