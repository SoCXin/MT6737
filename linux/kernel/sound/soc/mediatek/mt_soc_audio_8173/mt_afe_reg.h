/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MT_AFE_REG_H__
#define __MT_AFE_REG_H__

#include "mt_afe_def.h"
#include <linux/types.h>

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/

/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/

/*****************************************************************************
 *                          C O N S T A N T S
 *****************************************************************************/
#define AUDIO_HW_PHYSICAL_BASE  (0x11220000)
#define AUDIO_HW_VIRTUAL_BASE   (0xF2220000)
#ifdef AUDIO_MEM_IOREMAP
#define AFE_BASE                (0)
#else
#define AFE_BASE                (AUDIO_HW_VIRTUAL_BASE)
#endif

/* Internal sram: 0x11221000~0x1122A000 (36K) */
#define AFE_INTERNAL_SRAM_PHY_BASE  (0x11221000)
#define AFE_INTERNAL_SRAM_VIR_BASE  (AUDIO_HW_VIRTUAL_BASE - 0x70000 + 0x8000)
#define AFE_INTERNAL_SRAM_SIZE  (0x9000)

/* need enable this SPM register before access all AFE registers, set 0x1000629c = 0xd */
#define SPM_BASE (0x10006000)
#define SCP_AUDIO_PWR_CON (0x29C)

#define CKSYS_TOP       (0x10000000)

#define AUDIO_DCM_CFG      (0x0004)
#define AUDIO_CLK_CFG_4    (0x0080)
#define AUDIO_CLK_CFG_6    (0x00A0)
#define AUDIO_CLK_CFG_7    (0x00B0)
#define AUDIO_CLK_AUDDIV_0 (0x0120)
#define AUDIO_CLK_AUDDIV_1 (0x0124)
#define AUDIO_CLK_AUDDIV_2 (0x0128)
#define AUDIO_CLK_AUDDIV_3 (0x012C)
#define AUDIO_CLK_AUDDIV_4 (0x0134)

/*****************************************************************************
 *                         M A C R O
 *****************************************************************************/

/*****************************************************************************
 *                  R E G I S T E R       D E F I N I T I O N
 *****************************************************************************/
/* #define APMIXEDSYS_BASE   (0xF0209000) */
#define APMIXEDSYS_BASE       (0x10209000)
#define AP_PLL_CON5           (0x0014)
#define AUDIO_APLL1_CON0      (0x02A0)
#define AUDIO_APLL1_CON1      (0x02A4)
#define AUDIO_APLL1_CON2      (0x02A8)
#define AUDIO_APLL1_PWR_CON0  (0x02B0)
#define AUDIO_APLL2_CON0      (0x02B4)
#define AUDIO_APLL2_CON1      (0x02B8)
#define AUDIO_APLL2_CON2      (0x02C0)
#define AUDIO_APLL2_PWR_CON0  (0x02C4)


/* Fix build warning */
#undef AUDIO_TOP_CON0
/* End */
#define AUDIO_TOP_CON0          (AFE_BASE + 0x0000)
#define AUDIO_TOP_CON1          (AFE_BASE + 0x0004)
#define AUDIO_TOP_CON2          (AFE_BASE + 0x0008)
#define AUDIO_TOP_CON3          (AFE_BASE + 0x000C)
#define AFE_DAC_CON0            (AFE_BASE + 0x0010)
#define AFE_DAC_CON1            (AFE_BASE + 0x0014)
#define AFE_I2S_CON             (AFE_BASE + 0x0018)
#define AFE_DAIBT_CON0          (AFE_BASE + 0x001c)

#define AFE_CONN0       (AFE_BASE + 0x0020)
#define AFE_CONN1       (AFE_BASE + 0x0024)
#define AFE_CONN2       (AFE_BASE + 0x0028)
#define AFE_CONN3       (AFE_BASE + 0x002C)
#define AFE_CONN4       (AFE_BASE + 0x0030)
#define AFE_CONN5       (AFE_BASE + 0x005C)
#define AFE_CONN6       (AFE_BASE + 0x00BC)
#define AFE_CONN7       (AFE_BASE + 0x0460)
#define AFE_CONN8       (AFE_BASE + 0x0464)
#define AFE_CONN9       (AFE_BASE + 0x0468)

#define AFE_I2S_CON1    (AFE_BASE + 0x0034)
#define AFE_I2S_CON2    (AFE_BASE + 0x0038)
#define AFE_MRGIF_CON           (AFE_BASE + 0x003C)

/* Memory interface */
#define AFE_DL1_BASE      (AFE_BASE + 0x0040)
#define AFE_DL1_CUR       (AFE_BASE + 0x0044)
#define AFE_DL1_END       (AFE_BASE + 0x0048)
#define AFE_DL1_D2_BASE   (AFE_BASE + 0x0340)
#define AFE_DL1_D2_CUR    (AFE_BASE + 0x0344)
#define AFE_DL1_D2_END    (AFE_BASE + 0x0348)
#define AFE_VUL_D2_BASE   (AFE_BASE + 0x0350)
#define AFE_VUL_D2_END    (AFE_BASE + 0x0358)
#define AFE_VUL_D2_CUR    (AFE_BASE + 0x035C)

#define AFE_I2S_CON3      (AFE_BASE + 0x004C)
#define AFE_DL2_BASE      (AFE_BASE + 0x0050)
#define AFE_DL2_CUR       (AFE_BASE + 0x0054)
#define AFE_DL2_END       (AFE_BASE + 0x0058)
#define AFE_CONN_24BIT    (AFE_BASE + 0x006C)
#define AFE_AWB_BASE      (AFE_BASE + 0x0070)
#define AFE_AWB_END       (AFE_BASE + 0x0078)
#define AFE_AWB_CUR       (AFE_BASE + 0x007C)
#define AFE_VUL_BASE      (AFE_BASE + 0x0080)
#define AFE_VUL_END       (AFE_BASE + 0x0088)
#define AFE_VUL_CUR       (AFE_BASE + 0x008C)
#define AFE_DAI_BASE      (AFE_BASE + 0x0090)
#define AFE_DAI_END       (AFE_BASE + 0x0098)
#define AFE_DAI_CUR       (AFE_BASE + 0x009C)

/* Memory interface monitor */
#define AFE_MEMIF_MSB           (AFE_BASE + 0x00CC)
#define AFE_MEMIF_MON0          (AFE_BASE + 0x00D0)
#define AFE_MEMIF_MON1          (AFE_BASE + 0x00D4)
#define AFE_MEMIF_MON2          (AFE_BASE + 0x00D8)
#define AFE_MEMIF_MON4          (AFE_BASE + 0x00E0)

#define AFE_ADDA_DL_SRC2_CON0   (AFE_BASE + 0x0108)
#define AFE_ADDA_DL_SRC2_CON1   (AFE_BASE + 0x010C)
#define AFE_ADDA_UL_SRC_CON0    (AFE_BASE + 0x0114)
#define AFE_ADDA_UL_SRC_CON1    (AFE_BASE + 0x0118)
#define AFE_ADDA_TOP_CON0       (AFE_BASE + 0x0120)
#define AFE_ADDA_UL_DL_CON0     (AFE_BASE + 0x0124)
#define AFE_ADDA_SRC_DEBUG      (AFE_BASE + 0x012C)
#define AFE_ADDA_SRC_DEBUG_MON0 (AFE_BASE + 0x0130)
#define AFE_ADDA_SRC_DEBUG_MON1 (AFE_BASE + 0x0134)
#define AFE_ADDA_NEWIF_CFG0     (AFE_BASE + 0x0138)
#define AFE_ADDA_NEWIF_CFG1     (AFE_BASE + 0x013C)

#define AFE_SIDETONE_DEBUG  (AFE_BASE + 0x01D0)
#define AFE_SIDETONE_MON    (AFE_BASE + 0x01D4)
#define AFE_SIDETONE_CON0   (AFE_BASE + 0x01E0)
#define AFE_SIDETONE_COEFF  (AFE_BASE + 0x01E4)
#define AFE_SIDETONE_CON1   (AFE_BASE + 0x01E8)
#define AFE_SIDETONE_GAIN   (AFE_BASE + 0x01EC)
#define AFE_SGEN_CON0       (AFE_BASE + 0x01F0)
#define AFE_SGEN_CON1       (AFE_BASE + 0x01F4)
#define AFE_TOP_CON0        (AFE_BASE + 0x0200)

#define AFE_ADDA_PREDIS_CON0    (AFE_BASE + 0x0260)
#define AFE_ADDA_PREDIS_CON1    (AFE_BASE + 0x0264)

#define AFE_MRGIF_MON0          (AFE_BASE + 0x0270)
#define AFE_MRGIF_MON1          (AFE_BASE + 0x0274)
#define AFE_MRGIF_MON2          (AFE_BASE + 0x0278)

#define AFE_MOD_DAI_BASE (AFE_BASE + 0x0330)
#define AFE_MOD_DAI_END  (AFE_BASE + 0x0338)
#define AFE_MOD_DAI_CUR  (AFE_BASE + 0x033C)

#define AFE_MOD_PCM_BASE (AFE_BASE + 0x0330)
#define AFE_MOD_PCM_END  (AFE_BASE + 0x0338)
#define AFE_MOD_PCM_CUR  (AFE_BASE + 0x033C)

#define AFE_SPDIF2_OUT_CON0     (AFE_BASE + 0x0360)
#define AFE_SPDIF2_BASE         (AFE_BASE + 0x0364)
#define AFE_SPDIF2_CUR          (AFE_BASE + 0x0368)
#define AFE_SPDIF2_END          (AFE_BASE + 0x036C)
#define AFE_HDMI_OUT_CON0       (AFE_BASE + 0x0370)
#define AFE_HDMI_OUT_BASE       (AFE_BASE + 0x0374)
#define AFE_HDMI_OUT_CUR        (AFE_BASE + 0x0378)
#define AFE_HDMI_OUT_END        (AFE_BASE + 0x037C)
#define AFE_SPDIF_OUT_CON0      (AFE_BASE + 0x0380)
#define AFE_SPDIF_BASE          (AFE_BASE + 0x0384)
#define AFE_SPDIF_CUR           (AFE_BASE + 0x0388)
#define AFE_SPDIF_END           (AFE_BASE + 0x038C)
#define AFE_HDMI_CONN0          (AFE_BASE + 0x0390)
#define AFE_HDMI_CONN1          (AFE_BASE + 0x0398)

#define AFE_IRQ_MCU_CON     (AFE_BASE + 0x03A0)
#define AFE_IRQ_MCU_STATUS  (AFE_BASE + 0x03A4)
#define AFE_IRQ_MCU_CLR     (AFE_BASE + 0x03A8)
#define AFE_IRQ_MCU_CNT1    (AFE_BASE + 0x03AC)
#define AFE_IRQ_MCU_CNT2    (AFE_BASE + 0x03B0)
#define AFE_IRQ_MCU_EN      (AFE_BASE + 0x03B4)
#define AFE_IRQ_MCU_MON2    (AFE_BASE + 0x03B8)
#define AFE_IRQ_MCU_CNT5    (AFE_BASE + 0x03BC)

#define AFE_IRQ1_MCU_CNT_MON    (AFE_BASE + 0x03C0)
#define AFE_IRQ2_MCU_CNT_MON    (AFE_BASE + 0x03C4)
#define AFE_IRQ1_MCU_EN_CNT_MON (AFE_BASE + 0x03C8)
#define AFE_IRQ5_MCU_CNT_MON    (AFE_BASE + 0x03CC)
#define AFE_MEMIF_MAXLEN        (AFE_BASE + 0x03D4)
#define AFE_MEMIF_PBUF_SIZE     (AFE_BASE + 0x03D8)
#define AFE_IRQ_MCU_CNT7        (AFE_BASE + 0x03DC)
#define AFE_MEMIF_PBUF2_SIZE    (AFE_BASE + 0x03EC)
#define AFE_APLL1_TUNER_CFG     (AFE_BASE + 0x03f0)
#define AFE_APLL2_TUNER_CFG     (AFE_BASE + 0x03f4)


/* AFE GAIN CONTROL REGISTER */
#define AFE_GAIN1_CON0         (AFE_BASE + 0x0410)
#define AFE_GAIN1_CON1         (AFE_BASE + 0x0414)
#define AFE_GAIN1_CON2         (AFE_BASE + 0x0418)
#define AFE_GAIN1_CON3         (AFE_BASE + 0x041C)
#define AFE_GAIN1_CONN         (AFE_BASE + 0x0420)
#define AFE_GAIN1_CUR          (AFE_BASE + 0x0424)
#define AFE_GAIN2_CON0         (AFE_BASE + 0x0428)
#define AFE_GAIN2_CON1         (AFE_BASE + 0x042C)
#define AFE_GAIN2_CON2         (AFE_BASE + 0x0430)
#define AFE_GAIN2_CON3         (AFE_BASE + 0x0434)
#define AFE_GAIN2_CONN         (AFE_BASE + 0x0438)
#define AFE_GAIN2_CUR          (AFE_BASE + 0x043C)
#define AFE_GAIN2_CONN2        (AFE_BASE + 0x0440)
#define AFE_GAIN2_CONN3        (AFE_BASE + 0x0444)
#define AFE_GAIN1_CONN2        (AFE_BASE + 0x0448)
#define AFE_GAIN1_CONN3        (AFE_BASE + 0x044C)

#define AFE_IEC_CFG             (AFE_BASE + 0x0480)
#define AFE_IEC_NSNUM           (AFE_BASE + 0x0484)
#define AFE_IEC_BURST_INFO      (AFE_BASE + 0x0488)
#define AFE_IEC_BURST_LEN       (AFE_BASE + 0x048C)
#define AFE_IEC_NSADR           (AFE_BASE + 0x0490)
#define AFE_IEC_CHL_STAT0       (AFE_BASE + 0x04A0)
#define AFE_IEC_CHL_STAT1       (AFE_BASE + 0x04A4)
#define AFE_IEC_CHR_STAT0       (AFE_BASE + 0x04A8)
#define AFE_IEC_CHR_STAT1       (AFE_BASE + 0x04AC)

#define AFE_IEC2_CFG            (AFE_BASE + 0x04B0)
#define AFE_IEC2_NSNUM          (AFE_BASE + 0x04B4)
#define AFE_IEC2_BURST_INFO     (AFE_BASE + 0x04B8)
#define AFE_IEC2_BURST_LEN      (AFE_BASE + 0x04BC)
#define AFE_IEC2_NSADR          (AFE_BASE + 0x04C0)
#define AFE_IEC2_CHL_STAT0      (AFE_BASE + 0x04D0)
#define AFE_IEC2_CHL_STAT1      (AFE_BASE + 0x04D4)
#define AFE_IEC2_CHR_STAT0      (AFE_BASE + 0x04D8)
#define AFE_IEC2_CHR_STAT1      (AFE_BASE + 0x04DC)


/* here is only fpga needed */
#define FPGA_CFG2           (AFE_BASE + 0x04B8)
#define FPGA_CFG3           (AFE_BASE + 0x04BC)
#define FPGA_CFG0           (AFE_BASE + 0x04C0)
#define FPGA_CFG1           (AFE_BASE + 0x04C4)
#define FPGA_VERSION        (AFE_BASE + 0x04C8)
#define FPGA_STC            (AFE_BASE + 0x04CC)


#define AFE_ASRC_CON0           (AFE_BASE + 0x0500)
#define AFE_ASRC_CON1           (AFE_BASE + 0x0504)
#define AFE_ASRC_CON2           (AFE_BASE + 0x0508)
#define AFE_ASRC_CON3           (AFE_BASE + 0x050C)
#define AFE_ASRC_CON4           (AFE_BASE + 0x0510)
#define AFE_ASRC_CON5           (AFE_BASE + 0x0514)
#define AFE_ASRC_CON6           (AFE_BASE + 0x0518)
#define AFE_ASRC_CON7           (AFE_BASE + 0x051C)
#define AFE_ASRC_CON8           (AFE_BASE + 0x0520)
#define AFE_ASRC_CON9           (AFE_BASE + 0x0524)
#define AFE_ASRC_CON10          (AFE_BASE + 0x0528)
#define AFE_ASRC_CON11          (AFE_BASE + 0x052C)

#define PCM_INTF_CON1           (AFE_BASE + 0x0530)
#define PCM_INTF_CON2           (AFE_BASE + 0x0538)
#define PCM2_INTF_CON           (AFE_BASE + 0x053C)

#define AFE_TDM_CON1    (AFE_BASE + 0x548)
#define AFE_TDM_CON2    (AFE_BASE + 0x54C)

#define AFE_ASRC_CON13  (AFE_BASE+0x00550)
#define AFE_ASRC_CON14  (AFE_BASE+0x00554)
#define AFE_ASRC_CON15  (AFE_BASE+0x00558)
#define AFE_ASRC_CON16  (AFE_BASE+0x0055C)
#define AFE_ASRC_CON17  (AFE_BASE+0x00560)
#define AFE_ASRC_CON18  (AFE_BASE+0x00564)
#define AFE_ASRC_CON19  (AFE_BASE+0x00568)
#define AFE_ASRC_CON20  (AFE_BASE+0x0056C)
#define AFE_ASRC_CON21  (AFE_BASE+0x00570)

#define AFE_ADDA2_TOP_CON0    (AFE_BASE + 0x0600)


/**********************************
 *  Detailed Definitions
 **********************************/
#define IRQ_STATUS_BIT  0xFF

int mt_afe_reg_remap(void *dev);
void mt_afe_reg_unmap(void);

void mt_afe_set_reg(uint32_t offset, uint32_t value, uint32_t mask);
uint32_t mt_afe_get_reg(uint32_t offset);

uint32_t mt_afe_topck_get_reg(uint32_t offset);
void mt_afe_topck_set_reg(uint32_t offset, uint32_t value, uint32_t mask);

uint32_t mt_afe_pll_get_reg(uint32_t offset);
void mt_afe_pll_set_reg(uint32_t offset, uint32_t value, uint32_t mask);

uint32_t mt_afe_spm_get_reg(uint32_t offset);
void mt_afe_spm_set_reg(uint32_t offset, uint32_t value, uint32_t mask);

void *mt_afe_get_sram_base_ptr(void);
phys_addr_t mt_afe_get_sram_phy_addr(void);

void mt_afe_log_print(void);

#endif
