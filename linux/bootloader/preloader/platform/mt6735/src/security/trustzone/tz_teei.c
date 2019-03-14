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

/* Include header files */
#include "typedefs.h"
#include "tz_mem.h"
#include "tz_teei.h"
//#begin, [microtrust] [hanzefeng] [add] [For init value ][2015.07.15]
#include "uart.h"
//#end, [microtrust] [hanzefeng] [add] [For init value ][2015.07.15]
#include "dram_buffer.h"
#include "utos_version.h"
#define MOD "[TZ_teei]"

#define TEE_DEBUG
#ifdef TEE_DEBUG
#define DBG_MSG(str, ...) do {print(str, ##__VA_ARGS__);} while(0)
#else
#define DBG_MSG(str, ...) do {} while(0)
#endif

#if CFG_BOOT_ARGUMENT_BY_ATAG
extern unsigned int g_uart;
#elif CFG_BOOT_ARGUMENT && !CFG_BOOT_ARGUMENT_BY_ATAG
#define bootarg g_dram_buf->bootarg
#endif

void teei_boot_param_prepare(u32 param_addr, u32 tee_entry, 
    u64 teei_sec_dram_size, u64 dram_base, u64 dram_size)
{
    tee_arg_t_ptr teearg = (tee_arg_t_ptr)param_addr;
    u32 teei_log_port = 0;

    DBG_MSG("==================================================================\n");
    DBG_MSG("uTos PL VERSION [%s] \n",UTOS_VERSION);
    DBG_MSG("==================================================================\n");
    /*#begin, [micrortrust] [hanzefeng] [add] [For init value ][2015.07.15]*/
	if(teearg == NULL)
	{
		return;
	}
	tee_dev_t_ptr teedev0 = &(teearg->tee_dev[0]);
    tee_dev_t_ptr teedev1 = &(teearg->tee_dev[1]);
	tee_dev_t_ptr teedev2 =& (teearg->tee_dev[2]);
    tee_dev_t_ptr teedev3 =& (teearg->tee_dev[3]);
	tee_dev_t_ptr teedev4 =& (teearg->tee_dev[4]);
	
	 /*#end, [microtrust] [hanzefeng] [add] [For init value ][2015.07.15]*/

    /* Prepare TEE boot parameters */
    teearg->magic = TEEI_BOOTCFG_MAGIC;             /* TEEI magic number */
    teearg->length = sizeof(tee_arg_t);               /* TEEI argument block size */
    teearg->dRamBase = dram_base;                       /* DRAM base address */
    teearg->dRamSize = dram_size;                       /* Full DRAM size */
    teearg->secDRamBase = tee_entry;                       /* Secure DRAM base address */ 
    teearg->secDRamSize = teei_sec_dram_size;             /* Secure DRAM size */
    teearg->secIRamBase = TEE_SECURE_ISRAM_ADDR;           /* Secure SRAM base address */
    teearg->secIRamSize = TEE_SECURE_ISRAM_SIZE;           /* Secure SRAM size */
    
    //#begin, [microtrust] [hanzefeng] [add] [For init value ][2015.07.15]
    	
    /* SSI Reserve */
    teearg->total_number_spi  = 256;                             /* K2 Support total 256 SPIs and 32 PPIs */
    teearg->ssiq_number[0] = (32 + 248);                /* K2 reserve SPI ID 248 for TEEI, which is ID 280 */
    
    /* GIC parameters */
    teearg->gic_distributor_base = GIC_BASE ;
    teearg->gic_cpuinterface_base=GIC_CPU;
    teearg->gic_version=GIC_VERSION ;

#if CFG_BOOT_ARGUMENT_BY_ATAG
    teei_log_port = g_uart;
    DBG_MSG("%s teearg : CFG_BOOT_ARGUMENT_BY_ATAG 0x%x\n", __func__, g_uart);
#elif CFG_BOOT_ARGUMENT && !CFG_BOOT_ARGUMENT_BY_ATAG
    teei_log_port = bootarg.log_port;
    DBG_MSG("%s teearg : CFG_BOOT_ARGUMENT 0x%x\n", __func__, bootarg.log_port);
#else
    teei_log_port = CFG_UART_LOG;
    DBG_MSG("%s teearg : log port by prj cfg 0x%x\n", __func__, CFG_UART_LOG);
#endif

     /* UART parameters */
	switch(teei_log_port)
	{
		case UART0_BASE:
		{
			teedev0->dev_type = MT_UART16550;
			teedev0->base_addr = UART0_BASE;
			teedev0->intr_num = MT_UART0_IRQ;					//LOG DEBUG UART
			teedev0->apc_num = MT_UART0_DAPC;
			teedev0->param[0] = CFG_LOG_BAUDRATE;
			break;
		}			
		case UART1_BASE:
		{
			teedev0->dev_type = MT_UART16550;
			teedev0->base_addr = UART1_BASE;
			teedev0->intr_num = MT_UART1_IRQ;					//LOG DEBUG UART
			teedev0->apc_num = MT_UART1_DAPC;
			teedev0->param[0] = CFG_LOG_BAUDRATE;
			break;
		}			
		case UART2_BASE:
		{
			teedev0->dev_type = MT_UART16550;
			teedev0->base_addr = UART2_BASE;
			teedev0->intr_num = MT_UART2_IRQ;					//LOG DEBUG UART
			teedev0->apc_num = MT_UART2_DAPC;
			teedev0->param[0] = CFG_LOG_BAUDRATE;
			break;
		}			
		case UART3_BASE:
		{
			teedev0->dev_type = MT_UART16550;
			teedev0->base_addr = UART3_BASE;
			teedev0->intr_num = MT_UART3_IRQ;					//LOG DEBUG UART
			teedev0->apc_num = MT_UART3_DAPC;
			teedev0->param[0] = CFG_LOG_BAUDRATE;
			break;
		}		
		default:
		break;
	}	

     /* SEC GPTIMER parameters */
    teedev1->dev_type = MT_SEC_GPT;
    teedev1->base_addr = MT_SEC_GPT_BASE;
    teedev1->intr_num = MT_SEC_GPT_IRQ;
    teedev1->apc_num = MT_SEC_GPT_DAPC ;

    
     /* SEC GPTWDT parameters */
    teedev2->dev_type = MT_SEC_WDT;
    teedev2->base_addr = MT_SEC_WDT_BASE;
    teedev2->intr_num = MT_SEC_WDT_IRQ;
    teedev2->apc_num = MT_SEC_WDT_DAPC ;

	/* If tee dev is NOT used, it should be set the "MT_UNUSED" flag */
	teedev3->dev_type = MT_UNUSED;
	teedev4->dev_type = MT_UNUSED;
 
	/*
   	DBG_MSG("============================DUMP START=============================\n");
	DBG_MSG("%s teearg : 0x%x\n", __func__, teearg);
	DBG_MSG("%s atf_magic : 0x%x\n", __func__, teearg->magic);
	DBG_MSG("%s length : 0x%x\n", __func__, teearg->length);
	DBG_MSG("%s version : 0x%x\n", __func__, teearg->version);
	DBG_MSG("%s dRamBase : 0x%x\n", __func__, teearg->dRamBase);
	DBG_MSG("%s dRamSize : 0x%x\n", __func__, teearg->dRamSize);
	DBG_MSG("%s secDRamBase : 0x%x\n", __func__, teearg->secDRamBase);
	DBG_MSG("%s secIRamBase : 0x%x\n", __func__, teearg->secIRamBase);
	DBG_MSG("%s secIRamSize : 0x%x\n", __func__, teearg->secIRamSize);
	DBG_MSG("%s gic_distributor_base : 0x%x\n", __func__, teearg->gic_distributor_base);
	DBG_MSG("%s gic_cpuinterface_base : 0x%x\n", __func__,teearg->gic_cpuinterface_base);
	DBG_MSG("%s gic_version : 0x%x\n", __func__, teearg->gic_version);
	
	DBG_MSG("%s total_number_spi : 0x%x\n", __func__, teearg->total_number_spi); 
	DBG_MSG("%s teearg->ssiq_number[0] : 0x%x\n", __func__, teearg->ssiq_number[0]); 
	DBG_MSG("%s teearg->ssiq_number[1] : 0x%x\n", __func__, teearg->ssiq_number[1]); 
	DBG_MSG("%s teearg->ssiq_number[2] : 0x%x\n", __func__, teearg->ssiq_number[2]); 
	DBG_MSG("%s teearg->ssiq_number[3] : 0x%x\n", __func__, teearg->ssiq_number[3]); 
	DBG_MSG("%s teearg->ssiq_number[4] : 0x%x\n", __func__, teearg->ssiq_number[4]);

	DBG_MSG("%s teearg->tee_dev[0].dev_type : 0x%x\n", __func__, (teearg->tee_dev[0]).dev_type); 
	DBG_MSG("%s teearg->tee_dev[0].base_addr : 0x%x\n", __func__, (teearg->tee_dev[0]).base_addr); 
	DBG_MSG("%s teearg->tee_dev[0].intr_num : 0x%x\n", __func__, (teearg->tee_dev[0]).intr_num);
	DBG_MSG("%s teearg->tee_dev[0].apc_num : 0x%x\n", __func__, (teearg->tee_dev[0]).apc_num);
	
	DBG_MSG("%s teearg->tee_dev[1].dev_type : 0x%x\n", __func__, teearg->tee_dev[1].dev_type); 
	DBG_MSG("%s teearg->tee_dev[1].base_addr : 0x%x\n", __func__, teearg->tee_dev[1].base_addr); 
	DBG_MSG("%s teearg->tee_dev[1].intr_num : 0x%x\n", __func__, teearg->tee_dev[1].intr_num);
	DBG_MSG("%s teearg->tee_dev[1].apc_num : 0x%x\n", __func__, teearg->tee_dev[1].apc_num);
		
 	DBG_MSG("%s teearg->tee_dev[2].dev_type : 0x%x\n", __func__, teearg->tee_dev[2].dev_type); 
	DBG_MSG("%s teearg->tee_dev[2].base_addr : 0x%x\n", __func__, teearg->tee_dev[2].base_addr); 
	DBG_MSG("%s teearg->tee_dev[2].intr_num : 0x%x\n", __func__, teearg->tee_dev[2].intr_num);
	DBG_MSG("%s teearg->tee_dev[2].apc_num : 0x%x\n", __func__, teearg->tee_dev[2].apc_num);
	*/	
     /*#end, [microtrust] [hanzefeng] [add] [For init value ][2015.07.15]*/

}

void teei_key_param_prepare(u32 param_addr,u8 * hwuid,u8 * rpmb_key)
{
	tee_keys_t_ptr keyarg = (tee_keys_t_ptr)param_addr;
	keyarg->magic = TEEI_BOOTCFG_MAGIC;
	memcpy(keyarg->rpmb_key,rpmb_key,KEY_LEN);
	kdflib_get_huk(hwuid,16,MTK_ID, 6, keyarg->huk_master, KEY_LEN);
	kdflib_get_huk(keyarg->huk_master,32,TEE01_ID, 6, keyarg->huk_01, KEY_LEN);
	kdflib_get_huk(keyarg->huk_master,32,TEE02_ID, 6, keyarg->huk_02, KEY_LEN);
	kdflib_get_huk(keyarg->huk_master,32,TEE03_ID, 6, keyarg->huk_03, KEY_LEN);
	kdflib_get_huk(keyarg->huk_master,32,TEE04_ID, 6, keyarg->huk_04, KEY_LEN);
	kdflib_get_huk(keyarg->huk_master,32,TEE05_ID, 6, keyarg->huk_05, KEY_LEN);
	kdflib_get_huk(keyarg->huk_master,32,TEE06_ID, 6, keyarg->huk_06, KEY_LEN);
	kdflib_get_huk(keyarg->huk_master,32,TEE07_ID, 6, keyarg->huk_07, KEY_LEN);
	kdflib_get_huk(keyarg->huk_master,32,TEE08_ID, 6, keyarg->huk_08, KEY_LEN);
	memcpy(keyarg->hw_id,(hwuid + 16), 8);
	printf(">>>>>>>>>>>>>>>>>>>>>>> rpmb_key>>>>>>>>>>>>>>>>>>>>>:\n");
		printf(">>>>>>>>>>>>>>>>>>>>>>> rpmb_key>>>>>>>>>>>>>>>>>>>>>:\n");
			printf(">>>>>>>>>>>>>>>>>>>>>>> rpmb_key>>>>>>>>>>>>>>>>>>>>>:\n");
	int i;
	DBG_MSG(">>>>>>>>>>>>>>>>>>>>>>> rpmb_key>>>>>>>>>>>>>>>>>>>>>:\n");
	for (i = 0; i < 32; i++) {
		DBG_MSG("%d%s", rpmb_key[i], ((i+1)%16)?(" "):("\n"));
	} 
	DBG_MSG(">>>>>>>>>>>>>>>>>>>>>>> huk_master>>>>>>>>>>>>>>>>>>>>>:\n");
	for (i = 0; i < 32; i++) {
		DBG_MSG("%d%s", keyarg->huk_master[i], ((i+1)%16)?(" "):("\n"));
	}   
	DBG_MSG(">>>>>>>>>>>>>>>>>>>>>>> huk_01>>>>>>>>>>>>>>>>>>>>>:\n");
	for (i = 0; i < 32; i++) {
		DBG_MSG("%d%s", keyarg->huk_01[i], ((i+1)%16)?(" "):("\n"));
	}   
	DBG_MSG(">>>>>>>>>>>>>>>>>>>>>>> huk_08>>>>>>>>>>>>>>>>>>>>>:\n");
	for (i = 0; i < 32; i++) {
		DBG_MSG("%d%s", keyarg->huk_08[i], ((i+1)%16)?(" "):("\n"));
	} 
	//~ DBG_MSG(">>>>>>>>>>>>>>>>>>>>>>> huk_09>>>>>>>>>>>>>>>>>>>>>:\n");
	//~ for (i = 0; i < 32; i++) {
		//~ DBG_MSG("%d%s", keyarg->huk_09[i], ((i+1)%16)?(" "):("\n"));
	//~ } 
	//~ DBG_MSG(">>>>>>>>>>>>>>>>>>>>>>> huk_0A>>>>>>>>>>>>>>>>>>>>>:\n");
	//~ for (i = 0; i < 32; i++) {
		//~ DBG_MSG("%d%s", keyarg->huk_0A[i], ((i+1)%16)?(" "):("\n"));
	//~ } 
	for (i = 0; i < 8; i++) {
		DBG_MSG("HW random id keyarg->HRID[%d] = 0x%x\n", i, keyarg->hw_id[i]);
	} 

}

