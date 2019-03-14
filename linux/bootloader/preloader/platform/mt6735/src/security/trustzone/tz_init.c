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
#include "platform.h"
#include "dram_buffer.h"
#include "tz_init.h"
#include "tz_sec_reg.h"
//#include "device_apc.h"
#include "tz_mem.h"
#include "sec_devinfo.h"
#include "cust_sec_ctrl.h"
#include "sec.h"
#if CFG_TRUSTONIC_TEE_SUPPORT
#include "tz_tbase.h"
#endif
#if CFG_GOOGLE_TRUSTY_SUPPORT
#include "tz_trusty.h"
#endif
#if CFG_MICROTRUST_TEE_SUPPORT
#include "tz_teei.h"
#include "kdf.h"
#endif
#include "sec_devinfo.h"
#include "tz_emi_mpu.h"

/**************************************************************************
 *  DEBUG FUNCTIONS
 **************************************************************************/
#define MOD "[TZ_INIT]"

#define TEE_DEBUG
#ifdef TEE_DEBUG
#define DBG_MSG(str, ...) do {print(str, ##__VA_ARGS__);} while(0)
#define DBG_INFO(str, ...) do {print(str, ##__VA_ARGS__);} while(0)
#else
#define DBG_MSG(str, ...) do {} while(0)
#define DBG_INFO(str, ...) do {print(str, ##__VA_ARGS__);} while(0)
#endif

/**************************************************************************
 *  MACROS
 **************************************************************************/
#define TEE_MEM_ALIGNMENT (0x1000)  //4K Alignment
#define TEE_ENABLE_VERIFY (1)

#if CFG_BOOT_ARGUMENT
#define bootarg g_dram_buf->bootarg
#endif

/**************************************************************************
 *  EXTERNAL FUNCTIONS
 **************************************************************************/
extern void tz_sram_sec_init(u32 start);
extern void tz_sec_mem_init(u32 start, u32 end, u32 mpu_region);
//extern u64 platform_memory_size(void);
extern void tz_dapc_sec_init(void);
extern void tz_dapc_sec_postinit(void);
//extern void tz_set_module_apc(unsigned int module, E_MASK_DOM domain_num , APC_ATTR permission_control);

/**************************************************************************
 *  INTERNAL VARIABLES
 **************************************************************************/
static u32 tee_entry_addr = 0;
static const u8 tee_img_vfy_pubk[MTEE_IMG_VFY_PUBK_SZ] = {MTEE_IMG_VFY_PUBK};
static u8 g_hwuid[16];
//for init.s 
u32 bl31_base_addr = BL31_BASE;
u32 rst_vector_base_addr = RVBADDRESS_CPU0;
static u8 g_hwuid_initialized = 0;

/**************************************************************************
 *  INTERNAL FUNCTIONS
 **************************************************************************/
static u32 trustzone_get_atf_boot_param_addr(void)
{
    return ATF_BOOT_ARG_ADDR;
}

static u32 tee_secmem_size = 0;
static u32 tee_secmem_start = 0;
static u32 atf_log_buf_start = 0;
static u32 tee_extra_mem_size = 0;

void tee_get_secmem_start(u32 *addr)
{
#if CFG_TEE_SUPPORT
    *addr = tee_secmem_start - ATF_LOG_BUFFER_SIZE;
#else
    *addr = atf_log_buf_start;
#endif
}

void tee_get_secmem_size(u32 *size)
{
#if CFG_TEE_SUPPORT
    *size = tee_secmem_size + ATF_LOG_BUFFER_SIZE;
#else
    *size = ATF_LOG_BUFFER_SIZE;
#endif
}

void tee_set_entry(u32 addr)
{    
    tee_entry_addr = addr;
    
    DBG_MSG("%s TEE start entry : 0x%x\n", MOD, tee_entry_addr);
}

void tee_set_hwuid(u8 *id, u32 size)
{    
    atf_arg_t_ptr teearg = (atf_arg_t_ptr)trustzone_get_atf_boot_param_addr();

    memcpy(teearg->hwuid, id, size);
    memcpy(g_hwuid, id, size);
    g_hwuid_initialized = 1;

    //DBG_MSG("%s MEID : 0x%x, 0x%x, 0x%x, 0x%x\n", MOD, id[0], id[1], id[2], id[3]);
    //DBG_MSG("%s MEID : 0x%x, 0x%x, 0x%x, 0x%x\n", MOD, id[4], id[5], id[6], id[7]);
    //DBG_MSG("%s MEID : 0x%x, 0x%x, 0x%x, 0x%x\n", MOD, id[8], id[9], id[10], id[11]);
    //DBG_MSG("%s MEID : 0x%x, 0x%x, 0x%x, 0x%x\n", MOD, id[12], id[13], id[14], id[15]);
}

int tee_get_hwuid(u8 *id, u32 size)
{
    int ret = 0;

    if (!g_hwuid_initialized)
        return -1;

    memcpy(id, g_hwuid, size);

    return ret;
}

int tee_verify_image(u32 *addr, u32 size)
{
    u32 haddr = *addr; /* tee header address */
    int ret;

    ret = 0;
    
#if TEE_ENABLE_VERIFY   
    /* verify tee image and addr is updated to pointer entry point */
    ret = trustonic_tee_verify(addr, size, tee_img_vfy_pubk);
    if (ret)
        goto verify_end;
    
    ret = trustonic_tee_decrypt(haddr, size);
#else
    DBG_MSG("%s tee_verify_image : 0x%x, 0x%x (before)\n", MOD, *addr, size);

    *addr = *addr + 0x240;

    DBG_MSG("%s tee_verify_image : 0x%x, 0x%x (after)\n", MOD, *addr, size);    
#endif

verify_end:
    if(ret){       
        DBG_MSG("%s tee_verify_image : failed (%d)\n", MOD, ret);     
        DBG_MSG("%s DUMP 0x%x: 0x%x, 0x%x, 0x%x, 0x%x\n", MOD, *addr, 
            *(u32*)(*addr), *(u32*)(*addr+4), *(u32*)(*addr+8), *(u32*)(*addr+12)); 
        DBG_MSG("%s DUMP 0x%x: 0x%x, 0x%x, 0x%x, 0x%x\n", MOD, *addr+0x240, 
            *(u32*)(*addr+0x240), *(u32*)(*addr+0x244), *(u32*)(*addr+0x248), 
            *(u32*)(*addr+0x24C));
    }else{
        DBG_MSG("%s tee_verify_image : passed\n", MOD);
    }

    return ret;
}

u32 tee_get_load_addr(u32 maddr)
{
    u32 ret_addr = 0;
    u64 limit_addr = 0xC0000000;
    
#if CFG_TEE_SUPPORT
    if (tee_secmem_start != 0)
        goto allocated;

    tee_extra_mem_size = maddr % TEE_MEM_ALIGNMENT;
    tee_secmem_size = maddr - tee_extra_mem_size;

#if CFG_GOOGLE_TRUSTY_SUPPORT
    tee_secmem_start = TRUSTY_MEM_LOAD_ADDR;
#else
#if CFG_MICROTRUST_TEE_SUPPORT
    //limit_addr = 0xB0000000;
    limit_addr = 0x80000000; // Used the same address under 1GB/2GB DRAM.
#endif
    tee_secmem_start = (u32)mblock_reserve(&bootarg.mblock_info,
        (u64)(tee_secmem_size + ATF_LOG_BUFFER_SIZE), (u64)TEE_MEM_ALIGNMENT, 
        limit_addr, RANKMAX);
#endif // !CFG_GOOGLE_TRUSTY_SUPPORT

    if(!tee_secmem_start){
        printf("%s Fail to allocate secure memory: 0x%x, 0x%x\n", MOD,
            (tee_secmem_size + ATF_LOG_BUFFER_SIZE), TEE_MEM_ALIGNMENT);
        return 0;
    }

    atf_log_buf_start = tee_secmem_start;
    tee_secmem_start = tee_secmem_start + ATF_LOG_BUFFER_SIZE;

allocated:
    ret_addr = tee_secmem_start - tee_extra_mem_size;
#endif /* end of CFG_TEE_SUPPORT */

    return ret_addr;
}

static void tee_sec_config(void)
{
    tz_sram_sec_init(CFG_NON_SECURE_SRAM_ADDR);

#if CFG_TEE_SUPPORT
#if CFG_TEE_SECURE_MEM_PROTECTED
    {
    u32 secmem_end_addr = tee_entry_addr + tee_secmem_size - 1;

#if 1
#if CFG_MICROTRUST_TEE_SUPPORT
    /* temporarily workaround for DRAMC memory test issue in kernel */
    secmem_end_addr = secmem_end_addr - 0x10000;
#endif
#endif

    tz_sec_mem_init(tee_entry_addr, secmem_end_addr, SECURE_OS_MPU_REGION_ID);
    DBG_MSG("%s set secure memory protection : 0x%x, 0x%x (%d)\n", MOD, tee_entry_addr,
        secmem_end_addr, SECURE_OS_MPU_REGION_ID);
    }
#endif
#endif
    {
    u32 atf_entry_addr = BL31_BASE;
    atf_entry_addr = atf_entry_addr & ~(EMI_MPU_ALIGNMENT - 1);

    DBG_MSG("%s ATF entry addr, aligned addr : 0x%x, 0x%x\n", MOD, BL31_BASE, atf_entry_addr);
    
    tz_sec_mem_init(atf_entry_addr, atf_entry_addr + BL31_SIZE - 1, ATF_MPU_REGION_ID);
    DBG_MSG("%s set secure memory protection : 0x%x, 0x%x (%d)\n", MOD, atf_entry_addr,
        atf_entry_addr + BL31_SIZE - 1, ATF_MPU_REGION_ID);
    }
}

void trustzone_pre_init(void)
{
    sec_malloc_buf_reset();
    tz_dapc_sec_init();
    crypto_hw_secure(1);

#if CFG_ATF_LOG_SUPPORT    
#if !CFG_TEE_SUPPORT
    {
        atf_arg_t_ptr teearg = (atf_arg_t_ptr)trustzone_get_atf_boot_param_addr();
        atf_log_buf_start = (u32)mblock_reserve(&bootarg.mblock_info,
            (u64)ATF_LOG_BUFFER_SIZE, (u64)TEE_MEM_ALIGNMENT, 
            0xC0000000, RANKMAX);
        if(!atf_log_buf_start){
            printf("%s Fail to allocate atf log buffer: 0x%x, 0x%x\n", MOD, 
                ATF_LOG_BUFFER_SIZE, TEE_MEM_ALIGNMENT);
            teearg->atf_log_buf_size = 0;
        }
    }
#endif
#endif
}

void trustzone_post_init(void)
{
    atf_arg_t_ptr teearg = (atf_arg_t_ptr)trustzone_get_atf_boot_param_addr();

    teearg->atf_magic = ATF_BOOTCFG_MAGIC;
    teearg->tee_entry = tee_entry_addr;
    teearg->tee_boot_arg_addr = TEE_BOOT_ARG_ADDR;
    teearg->HRID[0] = seclib_get_devinfo_with_index(E_AREA12);
    teearg->HRID[1] = seclib_get_devinfo_with_index(E_AREA13);
    teearg->atf_log_port = CFG_UART_LOG;
    teearg->atf_log_baudrate = CFG_LOG_BAUDRATE;
    teearg->atf_irq_num = (32 + 249); /* reserve SPI ID 249 for ATF log, which is ID 281 */

    //DBG_MSG("%s hwuid[0] : 0x%x\n", MOD, teearg->hwuid[0]);
    //DBG_MSG("%s hwuid[1] : 0x%x\n", MOD, teearg->hwuid[1]);
    //DBG_MSG("%s hwuid[2] : 0x%x\n", MOD, teearg->hwuid[2]);
    //DBG_MSG("%s hwuid[3] : 0x%x\n", MOD, teearg->hwuid[3]);
    //DBG_MSG("%s HRID[0] : 0x%x\n", MOD, teearg->HRID[0]);
    //DBG_MSG("%s HRID[1] : 0x%x\n", MOD, teearg->HRID[1]);
    DBG_MSG("%s atf_log_port : 0x%x\n", MOD, teearg->atf_log_port);
    DBG_MSG("%s atf_log_baudrate : 0x%x\n", MOD, teearg->atf_log_baudrate);
    DBG_MSG("%s atf_irq_num : %d\n", MOD, teearg->atf_irq_num);


#if CFG_TRUSTONIC_TEE_SUPPORT
    tbase_secmem_param_prepare(TEE_PARAMETER_ADDR, tee_entry_addr, CFG_TEE_CORE_SIZE, 
        tee_secmem_size);
    tbase_boot_param_prepare(TEE_BOOT_ARG_ADDR, tee_entry_addr, CFG_TEE_CORE_SIZE, 
        CFG_DRAM_ADDR, platform_memory_size());
    teearg->tee_support = 1;
#elif CFG_GOOGLE_TRUSTY_SUPPORT
    trusty_boot_param_prepare(TEE_BOOT_ARG_ADDR, tee_entry_addr, tee_secmem_size,
        CFG_DRAM_ADDR, platform_memory_size());
    teearg->tee_support = 1;
#elif CFG_MICROTRUST_TEE_SUPPORT
	DBG_MSG("%s come to    teei ***  \n", MOD);
    teei_boot_param_prepare(TEE_BOOT_ARG_ADDR, tee_entry_addr, tee_secmem_size,
        CFG_DRAM_ADDR, platform_memory_size());
    //prepare the parameter for secure driver here
    teearg->tee_support = 1;
#else //CFG_ATF_SUPPORT
    teearg->tee_support = 0;
#endif   
    tz_dapc_sec_postinit(); 

#if CFG_ATF_LOG_SUPPORT
    teearg->atf_log_buf_start = atf_log_buf_start;
    teearg->atf_log_buf_size = ATF_LOG_BUFFER_SIZE;
    teearg->atf_aee_debug_buf_start = (atf_log_buf_start + ATF_LOG_BUFFER_SIZE - ATF_AEE_BUFFER_SIZE);
    teearg->atf_aee_debug_buf_size = ATF_AEE_BUFFER_SIZE;
#else
    teearg->atf_log_buf_start = 0;
    teearg->atf_log_buf_size = 0;    
    teearg->atf_aee_debug_buf_start = 0;
    teearg->atf_aee_debug_buf_size = 0;
#endif
    DBG_MSG("%s ATF log buffer start : 0x%x\n", MOD, teearg->atf_log_buf_start); 
    DBG_MSG("%s ATF log buffer size : 0x%x\n", MOD, teearg->atf_log_buf_size); 
    DBG_MSG("%s ATF aee buffer start : 0x%x\n", MOD, teearg->atf_aee_debug_buf_start); 
    DBG_MSG("%s ATF aee buffer size : 0x%x\n", MOD, teearg->atf_aee_debug_buf_size); 

#if CFG_TEE_SUPPORT
#if CFG_MICROTRUST_TEE_SUPPORT
    u8 rpmb_key[KEY_LEN];
	kdflib_get_msg_auth_key(teearg->hwuid, 16, rpmb_key, KEY_LEN);
    teei_key_param_prepare(TEE_PARAMETER_ADDR,(u8 *)teearg->hwuid,rpmb_key);
#else
    u8 rpmb_key[32];
    seclib_get_msg_auth_key(teearg->hwuid, 16, rpmb_key, 32);
#endif
    mmc_rpmb_set_key(rpmb_key);
    teearg->tee_rpmb_size = mmc_rpmb_get_size();
    DBG_MSG("%s TEE RPMB Size : 0x%x\n", MOD, teearg->tee_rpmb_size); 
#endif

    crypto_hw_secure(0);
}

void trustzone_jump(u32 addr, u32 arg1, u32 arg2)
{
    tee_sec_config();

#if CFG_TEE_SUPPORT
    DBG_MSG("%s Jump to ATF, then 0x%x and 0x%x\n", MOD, tee_entry_addr, addr);    
#else
    DBG_MSG("%s Jump to ATF, then 0x%x\n", MOD, addr);
#endif
    jumparch64(addr, arg1, arg2, trustzone_get_atf_boot_param_addr());
}

