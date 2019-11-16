#ifndef DRAM_BUFFER_H 
#define DRAM_BUFFER_H

#include "nand_core.h"
#include "platform.h"
#include "partition.h"

#include "msdc.h"	// for 	gpd_t, bd_t structure

/*bmt.c*/
#define BMT_BUFFER_SIZE     0x10000
//#define MAX_MAIN_SIZE                (0x1000) /*nand_core.h*/
//#define MAX_SPAR_SIZE                (0x80) /*nand_core.h*/
//#define BMT_DAT_BUFFER_SIZE         (MAX_MAIN_SIZE + MAX_SPAR_SIZE) /*nand_core.h*/
/*nand.c*/
//#define NFI_BUF_MAX_SIZE             (0x10000)         /*nand_core.h*/
//#define NAND_NFI_BUFFER_SIZE        (NFI_BUF_MAX_SIZE) /*nand_core.h*/


/*download.c*/
#define PART_HDR_BUF_SIZE 512

/*efi.c*/
#define GPT_BUFFER_SIZE                 (0x4000)

/*mmc_common_inter.c*/
#define STORAGE_BUFFER_SIZE 0x10000

/*partition.c*/
#define IMG_HDR_BUF_SIZE 512

/*print.c*/
#define LOG_BUFFER_MAX_SIZE             (0x10000)

/*sec.c*/
#define DRAM_SEC_SECRO_BUFFER_LENGTH     (0x3000)   /*buffer.h*/
#define DRAM_SEC_WORKING_BUFFER_LENGTH   0x2000  
#define DRAM_SEC_UTIL_BUFFER_LENGTH      0x1000   
#define DRAM_SEC_LIB_HEAP_LENGTH         0x4000   
#define DRAM_SEC_IMG_BUFFER_LENGTH       0x3000    
#define DRAM_SEC_CHUNK_BUFFER_LENGTH     0x100000 

#if CFG_BYPASS_EMI
typedef struct{
    /*bmt.c*/
    u8 bmt_buf[0x1000];
    u8 bmt_dat_buf[BMT_DAT_BUFFER_SIZE];
    /*nand.c*/
    u8 nand_nfi_buf[0x1000];

    /*download.c*/
    part_hdr_t part_hdr_buf[1];  
    /*efi.c*/
    u32 crc32_table[256];
    u8 pgpt_header_buf[512];
    u8 sgpt_header_buf[512];
    u8 pgpt_entries_buf[GPT_BUFFER_SIZE];
    u8 sgpt_entries_buf[GPT_BUFFER_SIZE];
    /*mmc_common_inter.c*/
    unsigned char storage_buffer[16];
    /*partition.c*/
    u8 img_hdr_buf[IMG_HDR_BUF_SIZE];
    unsigned int part_num;
    part_hdr_t   part_info[2];
    part_t  partition_info[2];    
    
#ifdef MTK_EMMC_SUPPORT
    struct part_meta_info meta_info[1];
#endif
    /*platform.c*/
    boot_arg_t bootarg; 
    /*print.c*/
    u8 log_dram_buf[0x1000];
    /*sec.c*/
    u8  sec_secro_buf[DRAM_SEC_SECRO_BUFFER_LENGTH];
    u8  sec_working_buf[DRAM_SEC_WORKING_BUFFER_LENGTH];/*This dram Buffer not used for security concern*/
    u8  sec_util_buf[DRAM_SEC_UTIL_BUFFER_LENGTH];
    u8  sec_lib_heap_buf[DRAM_SEC_LIB_HEAP_LENGTH];
    u8  sec_img_buf[DRAM_SEC_IMG_BUFFER_LENGTH];        /*This dram Buffer not used for security concern*/
    u8  sec_chunk_buf[0x4000];
    /*msdc.c*/
    gpd_t msdc_gpd_pool[MSDC_MAX_NUM][MAX_GPD_POOL_SZ] __attribute__ ((aligned(64)));
    bd_t  msdc_bd_pool[MSDC_MAX_NUM][MAX_BD_POOL_SZ] __attribute__ ((aligned(64)));
    boot_tag *boottag; /*boot args pass to LK by ATAG*/
} dram_buf_t;


#else
typedef struct{
    /*bmt.c*/
    u8 bmt_buf[BMT_BUFFER_SIZE];
    u8 bmt_dat_buf[BMT_DAT_BUFFER_SIZE];
    /*nand.c*/
    u8 nand_nfi_buf[NAND_NFI_BUFFER_SIZE];

    /*download.c*/
    part_hdr_t part_hdr_buf[PART_HDR_BUF_SIZE];  
    /*efi.c*/
    u32 crc32_table[256];
    u8 pgpt_header_buf[512];
    u8 sgpt_header_buf[512];
    u8 pgpt_entries_buf[GPT_BUFFER_SIZE];
    u8 sgpt_entries_buf[GPT_BUFFER_SIZE];
    /*mmc_common_inter.c*/
    unsigned char storage_buffer[STORAGE_BUFFER_SIZE];
    /*partition.c*/
    u8 img_hdr_buf[IMG_HDR_BUF_SIZE];
    unsigned int part_num;
    part_hdr_t   part_info[PART_MAX_NUM];
    part_t  partition_info[128];
	
#ifdef MTK_EMMC_SUPPORT
    struct part_meta_info meta_info[128];
#endif
    /*platform.c*/
    boot_arg_t bootarg; 
    /*print.c*/
    u8 log_dram_buf[LOG_BUFFER_MAX_SIZE];
    /*sec.c*/
    u8  sec_secro_buf[DRAM_SEC_SECRO_BUFFER_LENGTH];
    u8  sec_working_buf[DRAM_SEC_WORKING_BUFFER_LENGTH];/*This dram Buffer not used for security concern*/
    u8  sec_util_buf[DRAM_SEC_UTIL_BUFFER_LENGTH];
    u8  sec_lib_heap_buf[DRAM_SEC_LIB_HEAP_LENGTH];
    u8  sec_img_buf[DRAM_SEC_IMG_BUFFER_LENGTH];        /*This dram Buffer not used for security concern*/
    u8  sec_chunk_buf[DRAM_SEC_CHUNK_BUFFER_LENGTH];
    /*msdc.c*/
    gpd_t msdc_gpd_pool[MSDC_MAX_NUM][MAX_GPD_POOL_SZ] __attribute__ ((aligned(64)));
    bd_t  msdc_bd_pool[MSDC_MAX_NUM][MAX_BD_POOL_SZ] __attribute__ ((aligned(64)));
    boot_tag *boottag; /*boot args pass to LK by ATAG*/
} dram_buf_t;
#endif

typedef struct{
    u8  sram_sec_working_buf[DRAM_SEC_WORKING_BUFFER_LENGTH];
    u8  sram_sec_img_buf[DRAM_SEC_IMG_BUFFER_LENGTH];
} sec_buf_t;
void init_dram_buffer();

extern dram_buf_t *g_dram_buf;

extern sec_buf_t  g_sec_buf;
#endif /*DRAM_BUFFER_H*/
