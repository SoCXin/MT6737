/*! @file Mtk_qmu.h
*
*  @par Modification Notice:
*
*  This software is modified by MediaTek Inc. and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2005
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
* $Workfile: Mtk_qmu.h$
*
* @par Project:
*    Mtk usb swip
*
* @par Description:
*    Mtk DMAQ module header file, Define DMAQ Address, Offset, and Descriptor
Format

*
* @par Author_Name:
*    tianhao.fei
*
* @par Last_Changed: add comment for Doxygen
* $Author: jianqiang.tang $
* $Modtime: 2010-5-7$
* $Revision: #4 $
*
*/
#ifndef _MTK_QMU_H_
#define _MTK_QMU_H_

#include "mt_typedefs.h"

#define USB_base     (0x11200000)

#define USB_HW_QMU_BASE 0x86000800
#define USB_HW_QMU_LENGTH  0x500
/// @brief Define DMAQ Register Bass Address
/// @Author_Name:tianhao.fei 4/29/2010
/// @{

#define USB_QMUBASE USB_base + 0x800
#define USB_QISAR       USB_base + 0xc00
#define USB_QIMR        USB_base + 0xc04

#define USB_HW_QMU_OFF  USB_QMUBASE + 0x0000
#define USB_HW_QUCS_OFF USB_QMUBASE + 0x0300
#define USB_HW_QIRQ_OFF USB_QMUBASE + 0x0400
#define USB_HW_QDBG_OFF USB_QMUBASE + 0x04F0

/// @}
//#define GPD_LEN_ALIGNED (32)  /* Note: ARM should align cache line size */
#define GPD_LEN_ALIGNED (64)  /* > sizeof (TGPD) + sizeof (list_head)*/
// KOBE , modify

#define RXQ_NUM 1
#define TXQ_NUM 1


/// @brief  Define DMAQ Register Offset
/// @Author_Name:tianhao.fei 4/29/2010
/// @{
#define MGC_O_QMU_QCR0  0x0000
#ifdef MTK_WDMAQ
/*wimax only*/
#define MGC_O_QMU_QCR1  0x0004
/*end*/
#endif
#define MGC_O_QMU_QCR2  0x0008
#define MGC_O_QMU_QCR3  0x000C

#define MGC_O_QMU_RQCSR0    0x0010
#define MGC_O_QMU_RQSAR0    0x0014
#define MGC_O_QMU_RQCPR0    0x0018
#define MGC_O_QMU_RQCSR(n) (MGC_O_QMU_RQCSR0+0x0010*((n)-1))
#define MGC_O_QMU_RQSAR(n) (MGC_O_QMU_RQSAR0+0x0010*((n)-1))
#define MGC_O_QMU_RQCPR(n) (MGC_O_QMU_RQCPR0+0x0010*((n)-1))


#define MGC_O_QMU_RQTR_BASE 0x0090
#define MGC_O_QMU_RQTR(n)       (MGC_O_QMU_RQTR_BASE+0x4*((n)-1))
#define MGC_O_QMU_RQLDPR0       0x0100
#define MGC_O_QMU_RQLDPR(n)     (MGC_O_QMU_RQLDPR0+0x4*((n)-1))


#define MGC_O_QMU_TQCSR0    0x0200
#define MGC_O_QMU_TQSAR0    0x0204
#define MGC_O_QMU_TQCPR0    0x0208
#define MGC_O_QMU_TQCSR(n) (MGC_O_QMU_TQCSR0+0x0010*((n)-1))
#define MGC_O_QMU_TQSAR(n) (MGC_O_QMU_TQSAR0+0x0010*((n)-1))
#define MGC_O_QMU_TQCPR(n) (MGC_O_QMU_TQCPR0+0x0010*((n)-1))

#define MGC_O_QMU_QAR       0x0300
#define MGC_O_QUCS_USBGCSR  0x0000
#ifdef MTK_WDMAQ
/*wimax only*/
#define MGC_O_QUCS_TQUSBSC_BASE     0x0080
#define MGC_O_QUCS_TQUSBSC(n)           (MGC_O_QUCS_TQUSBSC_BASE+0x4*((n)-1))
#define MGC_O_QUCS_FW1          0x0004
#define MGC_O_QUCS_FW2          0x0008
/*end*/
#endif
#define MGC_O_QIRQ_QISAR        0x0000
#define MGC_O_QIRQ_QIMR     0x0004
#define MGC_O_QIRQ_QIMCR        0x0008
#define MGC_O_QIRQ_QIMSR        0x000C
#ifdef MTK_WDMAQ
/*wimax only*/
#define MGC_O_QIRQ_QSISAR       0x0010
#define MGC_O_QIRQ_DSIMR        0x0014
#define MGC_O_QIRQ_DSIMCR       0x0018
#define MGC_O_QIRQ_DSIMSR       0x001C
#define MGC_O_QIRQ_DSICR        0x0020
/*end*/
#endif
#define MGC_O_QIRQ_IOCDISR    0x0030
#define MGC_O_QIRQ_TEPEMPR  0x0060
#define MGC_O_QIRQ_TEPEMPMR 0x0064
#define MGC_O_QIRQ_TEPEMPMCR    0x0068
#define MGC_O_QIRQ_TEPEMPMSR    0x006C
#define MGC_O_QIRQ_REPEMPR  0x0070
#define MGC_O_QIRQ_REPEMPMR 0x0074
#define MGC_O_QIRQ_REPEMPMCR    0x0078
#define MGC_O_QIRQ_REPEMPMSR    0x007C

#define MGC_O_QIRQ_RQEIR        0x0090
#define MGC_O_QIRQ_RQEIMR       0x0094
#define MGC_O_QIRQ_RQEIMCR  0x0098
#define MGC_O_QIRQ_RQEIMSR  0x009C
#define MGC_O_QIRQ_REPEIR       0x00A0
#define MGC_O_QIRQ_REPEIMR  0x00A4
#define MGC_O_QIRQ_REPEIMCR 0x00A8
#define MGC_O_QIRQ_REPEIMSR 0x00AC
#define MGC_O_QIRQ_TQEIR        0x00B0
#define MGC_O_QIRQ_TQEIMR       0x00B4
#define MGC_O_QIRQ_TQEIMCR  0x00B8
#define MGC_O_QIRQ_TQEIMSR  0x00BC
#define MGC_O_QIRQ_TEPEIR       0x00C0
#define MGC_O_QIRQ_TEPEIMR  0x00C4
#define MGC_O_QIRQ_TEPEIMCR 0x00C8
#define MGC_O_QIRQ_TEPEIMSR 0x00CC


#define MGC_O_QDBG_DFCR 0x0000
#define MGC_O_QDBG_DFMR 0x0004
/// @}

/// @brief Queue Control value Definition
/// @Author_Name:tianhao.fei 4/29/2010
/// @{
#define DQMU_QUE_START  0x00000001
#define DQMU_QUE_RESUME 0x00000002
#define DQMU_QUE_STOP       0x00000004
#define DQMU_QUE_ACTIVE 0x00008000
/// @}

/// @brief USB QMU Special Control USBGCSR value Definition
/// @Author_Name:tianhao.fei 4/29/2010
/// @{
#define USB_QMU_Tx0_EN          0x00000001
#define USB_QMU_Tx_EN(n)            (USB_QMU_Tx0_EN<<((n)-1))
#if 1 //CC_TEST
#define USB_QMU_Rx0_EN          0x00010000
#else
#define USB_QMU_Rx0_EN          0x00000020
#endif
#define USB_QMU_Rx_EN(n)            (USB_QMU_Rx0_EN<<((n)-1))
#define USB_QMU_HIFEVT_EN           0x00000100
#define USB_QMU_HIFCMD_EN           0x01000000
#define DQMU_SW_RESET       0x00010000
#define DQMU_CS16B_EN       0x80000000
#define DQMU_TQ0CS_EN       0x00010000
#define DQMU_TQCS_EN(n) (DQMU_TQ0CS_EN<<((n)-1))
#define DQMU_RQ0CS_EN       0x00000001
#define DQMU_RQCS_EN(n) (DQMU_RQ0CS_EN<<((n)-1))
#define DQMU_TX0_ZLP        0x01000000
#define DQMU_TX_ZLP(n)      (DQMU_TX0_ZLP<<((n)-1))
#define DQMU_TX0_MULTIPLE   0x00010000
#define DQMU_TX_MULTIPLE(n) (DQMU_TX0_MULTIPLE<<((n)-1))
#ifdef MTK_WDMAQ
#define DQMU_TX_PADDING 0x00000002
#define DQMU_TX_SDUHDR  0x00000001
#endif
#define DQMU_RX0_MULTIPLE   0x00010000
#define DQMU_RX_MULTIPLE(n) (DQMU_RX0_MULTIPLE<<((n)-1))
#ifdef MTK_WDMAQ
#define DQMU_RX_SDUHDR  0x00000001
#endif
#define DQMU_RX0_ZLP        0x01000000
#define DQMU_RX_ZLP(n)      (DQMU_RX0_ZLP<<((n)-1))

#define DQMU_M_TXEP_ERR 0x10000000
#define DQMU_M_TXQ_ERR  0x08000000
#define DQMU_M_RXEP_ERR 0x04000000
#define DQMU_M_RXQ_ERR  0x02000000
#define DQMU_M_RQ_EMPTY 0x00020000
#define DQMU_M_TQ_EMPTY 0x00010000
#define DQMU_M_RX0_EMPTY    0x00000001
#define DQMU_M_RX_EMPTY(n)  (DQMU_M_RX0_EMPTY<<((n)-1))
#define DQMU_M_TX0_EMPTY    0x00000001
#define DQMU_M_TX_EMPTY(n)  (DQMU_M_TX0_EMPTY<<((n)-1))
#define DQMU_M_RX0_DONE 0x00000100
#define DQMU_M_RX_DONE(n)   (DQMU_M_RX0_DONE<<((n)-1))
#define DQMU_M_TX0_DONE 0x00000001
#define DQMU_M_TX_DONE(n)   (DQMU_M_TX0_DONE<<((n)-1))

#define DQMU_M_RX0_ZLP_ERR  0x01000000
#define DQMU_M_RX_ZLP_ERR(n)    (DQMU_M_RX0_ZLP_ERR<<((n)-1))
#ifdef MTK_WDMAQ
#define DQMU_M_RX0_HDR_ERR  0x00010000
#define DQMU_M_RX_HDR_ERR(n)    (DQMU_M_RX0_HDR_ERR<<((n)-1))
#endif
#define DQMU_M_RX0_LEN_ERR  0x00000100
#define DQMU_M_RX_LEN_ERR(n)    (DQMU_M_RX0_LEN_ERR<<((n)-1))
#define DQMU_M_RX0_GPDCS_ERR        0x00000001
#define DQMU_M_RX_GPDCS_ERR(n)  (DQMU_M_RX0_GPDCS_ERR<<((n)-1))

#define DQMU_M_TX0_LEN_ERR  0x00010000
#define DQMU_M_TX_LEN_ERR(n)    (DQMU_M_TX0_LEN_ERR<<((n)-1))
#define DQMU_M_TX0_GPDCS_ERR    0x00000100
#define DQMU_M_TX_GPDCS_ERR(n)  (DQMU_M_TX0_GPDCS_ERR<<((n)-1))
#define DQMU_M_TX0_BDCS_ERR     0x00000001
#define DQMU_M_TX_BDCS_ERR(n)   (DQMU_M_TX0_BDCS_ERR<<((n)-1))

#define DQMU_M_TX0_EP_ERR       0x00000001
#define DQMU_M_TX_EP_ERR(n) (DQMU_M_TX0_EP_ERR<<((n)-1))

#define DQMU_M_RX0_EP_ERR       0x00000001
#define DQMU_M_RX_EP_ERR(n) (DQMU_M_RX0_EP_ERR<<((n)-1))
#define DQMU_M_RQ_DIS_IOC(n)   (0x100<<((n)-1))
/// @}

/* Mark for resolving issue reported by Coverity. Redefinition issue
 * Those Macro already defined in mt_typdefs.h
 */
#if 0
#define DRV_WriteReg(addr,data)     ((*(volatile kal_uint16 *)(addr)) = (kal_uint16)data)

#define DRV_Reg(addr)               (*(volatile kal_uint16 *)(addr))
#define DRV_WriteReg32(addr,data)   ((*(volatile kal_uint32 *)(addr)) = (kal_uint32)data)

#define DRV_Reg32(addr)             (*(volatile kal_uint32 *)(addr))
#define DRV_WriteReg8(addr,data)    ((*(volatile kal_uint8 *)(addr)) = (kal_uint8)data)

#define DRV_Reg8(addr)              (*(volatile kal_uint8 *)(addr))
#endif

/**
 * @brief Read a 8-bit register from the core
 * @param _pBase core base address in memory
 * @param _offset offset into the core's register space
 * @return 16-bit datum
 */
#define MGC_ReadQMU8(_offset) \
    DRV_Reg8((USB_HW_QMU_OFF + _offset))

#define MGC_ReadQUCS8(base,_offset) \
    DRV_Reg8((USB_HW_QUCS_OFF + _offset))

#define MGC_ReadQIRQ8(base,_offset) \
    DRV_Reg8((USB_HW_QIRQ_OFF + _offset))

/**
 * @brief Read a 16-bit register from the core
 * @param _pBase core base address in memory
 * @param _offset offset into the core's register space
 * @return 16-bit datum
 */
#define MGC_ReadQMU16(_offset) \
    DRV_Reg((USB_HW_QMU_OFF + _offset))

#define MGC_ReadQUCS16(_offset) \
    DRV_Reg((USB_HW_QUCS_OFF + _offset))

#define MGC_ReadQIRQ16(_offset) \
    DRV_Reg((USB_HW_QIRQ_OFF + _offset))

/**
 * @brief Read a 32-bit register from the core
 * @param _pBase core base address in memory
 * @param _offset offset into the core's register space
 * @return 32-bit datum
 */
#define MGC_ReadQMU32(_offset) \
    DRV_Reg32((USB_HW_QMU_OFF + _offset))

#define MGC_ReadQUCS32(_offset) \
    DRV_Reg32((USB_HW_QUCS_OFF + _offset))

#define MGC_ReadQIRQ32(_offset) \
    DRV_Reg32((USB_HW_QIRQ_OFF + _offset))

/**
 * @briefWrite a 32-bit core register
 * @param _pBase core base address in memory
 * @param _offset offset into the core's register space
 * @param _data 32-bit datum
 */
#define MGC_WriteQMU32(_offset, _data) \
    DRV_WriteReg32((USB_HW_QMU_OFF + _offset), _data)

#define MGC_WriteQUCS32(_offset, _data) \
    DRV_WriteReg32((USB_HW_QUCS_OFF + _offset), _data)

#define MGC_WriteQIRQ32(_offset, _data) \
    DRV_WriteReg32((USB_HW_QIRQ_OFF + _offset), _data)

/**
 * TGPD
 */
typedef struct _TGPD {
	unsigned char   flag;
	unsigned char   chksum;
	unsigned short  DataBufferLen; /*Rx Allow Length*/
	struct _TGPD*   pNext;
	unsigned char*  pBuf;
	unsigned short  bufLen;
	unsigned char   ExtLength;
	unsigned char   ZTepFlag;
} TGPD, *PGPD;

typedef struct {
	TGPD gpd[2];
	kal_uint8 cur_gpd_index;
	TGPD *cur_ptr_gpd;
} gpd_pool;


/**
 * TBD
 */
typedef struct _TBD {
	unsigned char  flag;
	unsigned char  chksum;
	unsigned short  reserved1;
	struct _TBD *pNext;
	unsigned char*  pBuf;
	unsigned short bufLen;
	unsigned char  extLen;
	unsigned char  reserved2;
} TBD, *PBD;
/// @}

#endif


