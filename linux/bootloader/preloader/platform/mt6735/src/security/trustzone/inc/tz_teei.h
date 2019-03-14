/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2011
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE. 
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#ifndef TZ_TEEI_H
#define TZ_TEEI_H

#include "typedefs.h"

/* Tbase Magic For Interface */
#define TEEI_BOOTCFG_MAGIC (0x434d4254) // String TBMC in little-endian

/* TEE version */
#define TEE_ARGUMENT_VERSION            (0x00010000U)
#define KEY_LEN 32
#define MTK_ID "mtkInc"
#define TEE01_ID "tee_01"
#define TEE02_ID "tee_02"
#define TEE03_ID "tee_03"
#define TEE04_ID "tee_04"
#define TEE05_ID "tee_05"
#define TEE06_ID "tee_06"
#define TEE07_ID "tee_07"
#define TEE08_ID "tee_08"
#define TEE09_ID "tee_09"
#define TEE0A_ID "tee_0A"

/*#begin, [microtrust] [hanzefeng] [add] [For init value ][2015.07.15]*/
#define GIC_BASE 0x10221000
#define GIC_CPU 0x10222000
#define GIC_VERSION  0x02

#define MT_UART0_IRQ (91+32)
#define MT_UART1_IRQ (92+32)
#define MT_UART2_IRQ (93+32)
#define MT_UART3_IRQ (94+32)

#define MT_UART0_DAPC (46)
#define MT_UART1_DAPC (47)
#define MT_UART2_DAPC (48)
#define MT_UART3_DAPC (49)

#define MT_SEC_GPT_BASE 0x10008000
#define MT_SEC_GPT_IRQ (173+32)
#define MT_SEC_GPT_DAPC (8)

#define MT_SEC_WDT_BASE 0x10008000
#define MT_SEC_WDT_IRQ (174+32)
#define MT_SEC_WDT_DAPC (8)

//#begin, [microtrust] [hanzefeng] [add] [For init value ][2015.07.15]
enum device_type{
	MT_UNUSED = 0,
	MT_UART16550 =1,		//secure uart
	MT_SEC_GPT,		//secure gp timer
	MT_SEC_WDT		//secure watch dog
} ;
/*
 * because different 32/64 system or different align rules ,
 * we use attribute packed  ,only init once so not speed probelm,
 *  */
typedef struct{
	u32  dev_type;		//secure device type ,enum device_type
	u64  base_addr;		//secure deivice base address
	u32  intr_num;		//irq number for device
	u32 apc_num;		//secure  device apc (secure attribute)
	u32 param[3];		//others paramenter ,baudrate,speed,etc				
}  __attribute__((packed)) tee_dev_t,*tee_dev_t_ptr;		

typedef struct {
    u32 magic;	// magic value from information 
    u32 length;	// size of struct in bytes.
    u64 version;	// Version of structure
    u64 dRamBase; 	// NonSecure DRAM start address
    u64 dRamSize;		// NonSecure DRAM size
    u64 secDRamBase;	// Secure DRAM start address
    u64 secDRamSize;	// Secure DRAM size
    u64 secIRamBase;	// Secure IRAM base
    u64 secIRamSize;		// Secure IRam size
    u64 gic_distributor_base;		// gic_distributor_base
    u64 gic_cpuinterface_base;		//gic_cpuinterface_base
    u32 gic_version;			//gic version ,now is 2 ,later will be 3
    u32 total_number_spi;		//spi numbers
    u32 ssiq_number[5];		//ssiq numners 
    tee_dev_t tee_dev[5];	//secure device info
    u64 flags;
} __attribute__((packed)) tee_arg_t, *tee_arg_t_ptr;
/*#end, [microtrust] [hanzefeng] [add] [For init value ][2015.07.15]	*/

typedef struct
{
	u32 magic;
	u32 version; 
	u8 rpmb_key[KEY_LEN];     //rpmb key
	u8 huk_master[KEY_LEN];   //master huk key
	u8 huk_01[KEY_LEN];		  //tee_01 huk
	u8 huk_02[KEY_LEN];		  //tee_02 huk
	u8 huk_03[KEY_LEN];		  //tee_03 huk
	u8 huk_04[KEY_LEN];		  //tee_04 huk
	u8 huk_05[KEY_LEN];		  //tee_05 huk
	u8 huk_06[KEY_LEN];		  //tee_06 huk
	u8 huk_07[KEY_LEN];		  //tee_07 huk
	u8 huk_08[KEY_LEN];		  //tee_08 huk
	u8 hw_id[KEY_LEN];			  //add by libaojun20150806
} tee_keys_t,*tee_keys_t_ptr;

/**************************************************************************
 * EXPORTED FUNCTIONS
 **************************************************************************/
void teei_boot_param_prepare(u32 param_addr, u32 tee_entry, u64 teei_sec_dram_size, u64 dram_base, u64 dram_size);
void teei_key_param_prepare(u32 param_addr,u8 * hwuid,u8 * rpmb_key);

#endif /* TZ_TEEI_H */

