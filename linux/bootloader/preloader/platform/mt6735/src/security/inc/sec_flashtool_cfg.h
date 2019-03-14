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

#ifndef SEC_FLASHTOOL_CFG_H
#define SEC_FLASHTOOL_CFG_H

/**************************************************************************
 * [FLASH TOOL ID]
 **************************************************************************/
#define FLASHTOOL_CFG_MAGIC             (0x544F4F4C)
#define FLASHTOOL_CFG_MAGIC_64          (0x544F4F5C)


/**************************************************************************
 * [FLASH TOOL SECURE CONFIG]
 **************************************************************************/
typedef struct {
    unsigned char                       m_img_name [16];
    unsigned int                        m_img_offset;    
    unsigned int                        m_img_length;
} BYPASS_CHECK_IMAGE_T;

typedef struct {
    unsigned char                       m_img_name [16];
    unsigned int                        m_img_offset_high;    
    unsigned int                        m_img_offset_low;
} BYPASS_CHECK_IMAGE_T_64;

/**************************************************************************
 * [FLASH TOOL SECURE CONFIG]
 **************************************************************************/
#define FLASHTOOL_CFG_SIZE              (76)
typedef struct {
    unsigned int                        m_magic_num;
#if defined(FLASHTOOL_SEC_CFG_64)        
    BYPASS_CHECK_IMAGE_T_64             m_bypass_check_img [3];
#else
    BYPASS_CHECK_IMAGE_T                m_bypass_check_img [3];
#endif
} FLASHTOOL_SECCFG_T;


/**************************************************************************
 * [FLASH TOOL FORBID DOWNLOAD ID]
 **************************************************************************/
#define FLASHTOOL_NON_SLA_FORBID_MAGIC             (0x544F4F4D)
#define FLASHTOOL_NON_SLA_FORBID_MAGIC_64          (0x544F4F5D)


/**************************************************************************
 * [FLASH TOOL FORBID DOWNLOAD CONFIG]
 **************************************************************************/
typedef struct {
    unsigned char                       m_img_name [16];
    unsigned int                        m_img_offset;    
    unsigned int                        m_img_length;
} FORBID_DOWNLOAD_IMAGE_T;

typedef struct {
    unsigned char                       m_img_name [16];
    unsigned int                        m_img_offset_high;    
    unsigned int                        m_img_offset_low;
} FORBID_DOWNLOAD_IMAGE_T_64;

/**************************************************************************
 * [FLASH TOOL FORBID DOWNLOAD CONFIG]
 **************************************************************************/
#define FLASHTOOL_NON_SLA_FORBID_CFG_SIZE              (52)
typedef struct {
    unsigned int                        m_forbid_magic_num;

#if defined(FLASHTOOL_FORBID_DL_NSLA_CFG_64)
    FORBID_DOWNLOAD_IMAGE_T_64          m_forbid_dl_nsla_img [2];
#else
    FORBID_DOWNLOAD_IMAGE_T             m_forbid_dl_nsla_img [2];
#endif
} FLASHTOOL_FORBID_DOWNLOAD_NSLA_T;

#endif /* SEC_FLASHTOOL_CFG_H */

