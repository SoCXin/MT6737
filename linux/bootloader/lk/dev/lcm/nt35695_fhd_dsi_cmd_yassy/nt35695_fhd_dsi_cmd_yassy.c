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
#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#endif

#include <cust_gpio_usage.h>
#include <cust_i2c.h>

#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)

static unsigned int nt_cabc = 0x2;//default cabc open
static unsigned int nt_sre = 0x0;//default sre close

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)                                    (lcm_util.set_reset_pin((v)))
#define MDELAY(n)                                           (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmd_by_cmdq(handle,cmd,count,ppara,force_update)    lcm_util.dsi_set_cmdq_V22(handle,cmd,count,ppara,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                       lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


//static unsigned char lcd_id_pins_value = 0xFF;
static const unsigned char LCD_MODULE_ID = 0x01; //  haobing modified 2013.07.11
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE                                    1
#define FRAME_WIDTH                                         (1080)
#define FRAME_HEIGHT                                        (1920)


#define REGFLAG_DELAY                                       0xFC
#define REGFLAG_END_OF_TABLE                                0xFD   // END OF REGISTERS MARKER


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

static LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

//static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
struct LCM_setting_table {
	unsigned char cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_cabc_level_setting[] = {
	{0x55, 1, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_initialization_setting[] = {
	{0xFF,1,{0x24}},
	{0xFB,1,{0x01}},
	{0xC5,1,{0x31}},
	{0xFF,1,{0x20}},
	{0x00,1,{0x01}},
	{0x01,1,{0x55}},
	{0x02,1,{0x45}},
	{0x03,1,{0x55}},
	{0x05,1,{0x50}},    //VGH=2xAVDD, VGL=2xAVEE
	{0x06,1,{0x4A}},
	{0x07,1,{0x24}},
	{0x08,1,{0x0C}},
	{0x0B,1,{0x87}},
	{0x0C,1,{0x87}},
	{0x0E,1,{0xB0}},
	{0x0F,1,{0xB3}},
	{0x11,1,{0x10}},    //VCOM  setting
	{0x12,1,{0x10}},
	{0x13,1,{0x03}},
	{0x14,1,{0x4A}},
	{0x15,1,{0x12}},
	{0x16,1,{0x12}},
	{0x30,1,{0x01}},
	{0x58,1,{0x82}},    //[Added] Enable GCK EQ
	{0x59,1,{0x00}},    //[Added] Enable GCK EQ
	{0x5A,1,{0x02}},    //[Added] GCK EQ Setting
	{0x5B,1,{0x00}},    //[Added] GCK EQ Setting
	{0x5C,1,{0x82}},    //[Added] Enable MUX EQ
	{0x5D,1,{0x82}},    //[Added] Enable MUX EQ
	{0x5E,1,{0x02}},    //[Added] MUX EQ Setting
	{0x5F,1,{0x02}},    //[Added] MUX EQ Setting
	{0x72,1,{0x31}},
	{0xFB,1,{0x01}},
	{0xFF,1,{0x24}},
	{0x00,1,{0x01}},
	{0x01,1,{0x0B}},
	{0x02,1,{0x0C}},
	{0x03,1,{0x09}},
	{0x04,1,{0x0A}},
	{0x05,1,{0x0F}},
	{0x06,1,{0x10}},
	{0x07,1,{0x00}},
	{0x08,1,{0x1C}},
	{0x09,1,{0x00}},
	{0x0A,1,{0x00}},
	{0x0B,1,{0x00}},
	{0x0C,1,{0x00}},
	{0x0D,1,{0x13}},
	{0x0E,1,{0x15}},
	{0x0F,1,{0x17}},
	{0x10,1,{0x01}},
	{0x11,1,{0x0B}},
	{0x12,1,{0x0C}},
	{0x13,1,{0x09}},
	{0x14,1,{0x0A}},
	{0x15,1,{0x0F}},
	{0x16,1,{0x10}},
	{0x17,1,{0x10}},
	{0x18,1,{0x1C}},
	{0x19,1,{0x00}},
	{0x1A,1,{0x00}},
	{0x1B,1,{0x00}},
	{0x1C,1,{0x00}},
	{0x1D,1,{0x13}},
	{0x1E,1,{0x15}},
	{0x1F,1,{0x17}},
	{0x20,1,{0x00}},
	{0x21,1,{0x01}},
	{0x22,1,{0x00}},
	{0x23,1,{0x40}},
	{0x24,1,{0x40}},
	{0x25,1,{0x6D}},
	{0x26,1,{0x40}},
	{0x27,1,{0x40}},
	{0xBD,1,{0x20}},
	{0xB6,1,{0x21}},
	{0xB7,1,{0x22}},
	{0xB8,1,{0x07}},
	{0xB9,1,{0x07}},
	{0xC1,1,{0x6D}},
	{0xBE,1,{0x07}},
	{0xBF,1,{0x07}},
	{0x29,1,{0xD8}},
	{0x2A,1,{0x2A}},
	{0x4B,1,{0x04}},
	{0x4C,1,{0x11}},
	{0x4D,1,{0x00}},
	{0x4E,1,{0x00}},
	{0x4F,1,{0x11}},
	{0x50,1,{0x11}},
	{0x51,1,{0x00}},
	{0x52,1,{0x40}},
	{0x53,1,{0x08}},
	{0x56,1,{0x08}},
	{0x54,1,{0x8C}},
	{0x58,1,{0x8C}},
	{0x55,1,{0x6D}},
	{0x5B,1,{0x43}},
	{0x5C,1,{0x00}},
	{0x5F,1,{0x73}},
	{0x60,1,{0x73}},
	{0x63,1,{0x22}},
	{0x64,1,{0x00}},
	{0x67,1,{0x08}},
	{0x68,1,{0x04}},
	{0x7A,1,{0x80}},
	{0x7B,1,{0x91}},
	{0x7C,1,{0xD8}},
	{0x7D,1,{0x60}},
	{0x74,1,{0x03}},
	{0x7E,1,{0x03}},
	{0x75,1,{0x29}},
	{0x7F,1,{0x29}},
	{0x86,1,{0x1B}},
	{0x87,1,{0x39}},
	{0x88,1,{0x1B}},
	{0x89,1,{0x39}},
	{0x8B,1,{0xF4}},
	{0x8C,1,{0x01}},
	{0x93,1,{0x06}},
	{0x94,1,{0x06}},
	{0xB3,1,{0x00}},
	{0xB4,1,{0x00}},
	{0xB5,1,{0x00}},
	{0x78,1,{0x00}},
	{0x79,1,{0x00}},
	{0x80,1,{0x00}},
	{0x83,1,{0x00}},
	{0x84,1,{0x04}},
	{0x8A,1,{0x33}},
	{0x9B,1,{0x0F}},
	{0xFB,1,{0x01}},
	{0xEC,1,{0x00}},
	{0xFF,1,{0x20}},    //CMD2 Page0
	{0xFB,1,{0x01}},
	{0x75,1,{0x00}},
	{0x76,1,{0x2E}},
	{0x77,1,{0x00}},
	{0x78,1,{0x5F}},
	{0x79,1,{0x00}},
	{0x7A,1,{0x8B}},
	{0x7B,1,{0x00}},
	{0x7C,1,{0xA8}},
	{0x7D,1,{0x00}},
	{0x7E,1,{0xBF}},
	{0x7F,1,{0x00}},
	{0x80,1,{0xD3}},
	{0x81,1,{0x00}},
	{0x82,1,{0xE3}},
	{0x83,1,{0x00}},
	{0x84,1,{0xF2}},
	{0x85,1,{0x01}},
	{0x86,1,{0x00}},
	{0x87,1,{0x01}},
	{0x88,1,{0x2C}},
	{0x89,1,{0x01}},
	{0x8A,1,{0x4F}},
	{0x8B,1,{0x01}},
	{0x8C,1,{0x86}},
	{0x8D,1,{0x01}},
	{0x8E,1,{0xB1}},
	{0x8F,1,{0x01}},
	{0x90,1,{0xF4}},
	{0x91,1,{0x02}},
	{0x92,1,{0x2A}},
	{0x93,1,{0x02}},
	{0x94,1,{0x2B}},
	{0x95,1,{0x02}},
	{0x96,1,{0x5E}},
	{0x97,1,{0x02}},
	{0x98,1,{0x97}},
	{0x99,1,{0x02}},
	{0x9A,1,{0xBC}},
	{0x9B,1,{0x02}},
	{0x9C,1,{0xEE}},
	{0x9D,1,{0x03}},
	{0x9E,1,{0x11}},
	{0x9F,1,{0x03}},
	{0xA0,1,{0x3E}},
	{0xA2,1,{0x03}},
	{0xA3,1,{0x4B}},
	{0xA4,1,{0x03}},
	{0xA5,1,{0x5A}},
	{0xA6,1,{0x03}},
	{0xA7,1,{0x6A}},
	{0xA9,1,{0x03}},
	{0xAA,1,{0x7C}},
	{0xAB,1,{0x03}},
	{0xAC,1,{0x8F}},
	{0xAD,1,{0x03}},
	{0xAE,1,{0xA3}},
	{0xAF,1,{0x03}},
	{0xB0,1,{0xB4}},
	{0xB1,1,{0x03}},
	{0xB2,1,{0xFF}},
	{0xB3,1,{0x00}},
	{0xB4,1,{0x2E}},
	{0xB5,1,{0x00}},
	{0xB6,1,{0x5F}},
	{0xB7,1,{0x00}},
	{0xB8,1,{0x8B}},
	{0xB9,1,{0x00}},
	{0xBA,1,{0xA8}},
	{0xBB,1,{0x00}},
	{0xBC,1,{0xBF}},
	{0xBD,1,{0x00}},
	{0xBE,1,{0xD3}},
	{0xBF,1,{0x00}},
	{0xC0,1,{0xE3}},
	{0xC1,1,{0x00}},
	{0xC2,1,{0xF2}},
	{0xC3,1,{0x01}},
	{0xC4,1,{0x00}},
	{0xC5,1,{0x01}},
	{0xC6,1,{0x2C}},
	{0xC7,1,{0x01}},
	{0xC8,1,{0x4F}},
	{0xC9,1,{0x01}},
	{0xCA,1,{0x86}},
	{0xCB,1,{0x01}},
	{0xCC,1,{0xB1}},
	{0xCD,1,{0x01}},
	{0xCE,1,{0xF4}},
	{0xCF,1,{0x02}},
	{0xD0,1,{0x2A}},
	{0xD1,1,{0x02}},
	{0xD2,1,{0x2B}},
	{0xD3,1,{0x02}},
	{0xD4,1,{0x5E}},
	{0xD5,1,{0x02}},
	{0xD6,1,{0x97}},
	{0xD7,1,{0x02}},
	{0xD8,1,{0xBC}},
	{0xD9,1,{0x02}},
	{0xDA,1,{0xEE}},
	{0xDB,1,{0x03}},
	{0xDC,1,{0x11}},
	{0xDD,1,{0x03}},
	{0xDE,1,{0x3E}},
	{0xDF,1,{0x03}},
	{0xE0,1,{0x4B}},
	{0xE1,1,{0x03}},
	{0xE2,1,{0x5A}},
	{0xE3,1,{0x03}},
	{0xE4,1,{0x6A}},
	{0xE5,1,{0x03}},
	{0xE6,1,{0x7C}},
	{0xE7,1,{0x03}},
	{0xE8,1,{0x8F}},
	{0xE9,1,{0x03}},
	{0xEA,1,{0xA3}},
	{0xEB,1,{0x03}},
	{0xEC,1,{0xB4}},
	{0xED,1,{0x03}},
	{0xEE,1,{0xFF}},
	{0xEF,1,{0x00}},
	{0xF0,1,{0x2E}},
	{0xF1,1,{0x00}},
	{0xF2,1,{0x32}},
	{0xF3,1,{0x00}},
	{0xF4,1,{0x47}},
	{0xF5,1,{0x00}},
	{0xF6,1,{0x59}},
	{0xF7,1,{0x00}},
	{0xF8,1,{0x69}},
	{0xF9,1,{0x00}},
	{0xFA,1,{0x79}},
	{0xFF,1,{0x21}},    //CMD2 Page1
	{0xFB,1,{0x01}},
	{0x00,1,{0x00}},
	{0x01,1,{0x89}},
	{0x02,1,{0x00}},
	{0x03,1,{0x9B}},
	{0x04,1,{0x00}},
	{0x05,1,{0xAD}},
	{0x06,1,{0x00}},
	{0x07,1,{0xDE}},
	{0x08,1,{0x01}},
	{0x09,1,{0x05}},
	{0x0A,1,{0x01}},
	{0x0B,1,{0x49}},
	{0x0C,1,{0x01}},
	{0x0D,1,{0x7F}},
	{0x0E,1,{0x01}},
	{0x0F,1,{0xD4}},
	{0x10,1,{0x02}},
	{0x11,1,{0x16}},
	{0x12,1,{0x02}},
	{0x13,1,{0x17}},
	{0x14,1,{0x02}},
	{0x15,1,{0x4F}},
	{0x16,1,{0x02}},
	{0x17,1,{0x8A}},
	{0x18,1,{0x02}},
	{0x19,1,{0xAF}},
	{0x1A,1,{0x02}},
	{0x1B,1,{0xE1}},
	{0x1C,1,{0x03}},
	{0x1D,1,{0x03}},
	{0x1E,1,{0x03}},
	{0x1F,1,{0x2F}},
	{0x20,1,{0x03}},
	{0x21,1,{0x3D}},
	{0x22,1,{0x03}},
	{0x23,1,{0x4C}},
	{0x24,1,{0x03}},
	{0x25,1,{0x5C}},
	{0x26,1,{0x03}},
	{0x27,1,{0x70}},
	{0x28,1,{0x03}},
	{0x29,1,{0x86}},
	{0x2A,1,{0x03}},
	{0x2B,1,{0xA0}},
	{0x2D,1,{0x03}},
	{0x2F,1,{0xB8}},
	{0x30,1,{0x03}},
	{0x31,1,{0xFF}},
	{0x32,1,{0x00}},
	{0x33,1,{0x2E}},
	{0x34,1,{0x00}},
	{0x35,1,{0x32}},
	{0x36,1,{0x00}},    //
	{0x37,1,{0x47}},
	{0x38,1,{0x00}},
	{0x39,1,{0x59}},
	{0x3A,1,{0x00}},
	{0x3B,1,{0x69}},
	{0x3D,1,{0x00}},
	{0x3F,1,{0x79}},
	{0x40,1,{0x00}},
	{0x41,1,{0x89}},
	{0x42,1,{0x00}},
	{0x43,1,{0x9B}},
	{0x44,1,{0x00}},
	{0x45,1,{0xAD}},
	{0x46,1,{0x00}},
	{0x47,1,{0xDE}},
	{0x48,1,{0x01}},
	{0x49,1,{0x05}},
	{0x4A,1,{0x01}},
	{0x4B,1,{0x49}},
	{0x4C,1,{0x01}},
	{0x4D,1,{0x7F}},
	{0x4E,1,{0x01}},
	{0x4F,1,{0xD4}},
	{0x50,1,{0x02}},
	{0x51,1,{0x16}},
	{0x52,1,{0x02}},
	{0x53,1,{0x17}},
	{0x54,1,{0x02}},
	{0x55,1,{0x4F}},
	{0x56,1,{0x02}},
	{0x58,1,{0x8A}},
	{0x59,1,{0x02}},
	{0x5A,1,{0xAF}},
	{0x5B,1,{0x02}},
	{0x5C,1,{0xE1}},
	{0x5D,1,{0x03}},
	{0x5E,1,{0x03}},
	{0x5F,1,{0x03}},
	{0x60,1,{0x2F}},
	{0x61,1,{0x03}},
	{0x62,1,{0x3D}},
	{0x63,1,{0x03}},
	{0x64,1,{0x4C}},
	{0x65,1,{0x03}},
	{0x66,1,{0x5C}},
	{0x67,1,{0x03}},
	{0x68,1,{0x70}},
	{0x69,1,{0x03}},
	{0x6A,1,{0x86}},
	{0x6B,1,{0x03}},
	{0x6C,1,{0xA0}},
	{0x6D,1,{0x03}},
	{0x6E,1,{0xB8}},
	{0x6F,1,{0x03}},
	{0x70,1,{0xFF}},
	{0x71,1,{0x00}},
	{0x72,1,{0x2E}},
	{0x73,1,{0x00}},
	{0x74,1,{0x41}},
	{0x75,1,{0x00}},
	{0x76,1,{0x63}},
	{0x77,1,{0x00}},
	{0x78,1,{0x7D}},
	{0x79,1,{0x00}},
	{0x7A,1,{0x93}},
	{0x7B,1,{0x00}},
	{0x7C,1,{0xA7}},
	{0x7D,1,{0x00}},
	{0x7E,1,{0xB7}},
	{0x7F,1,{0x00}},
	{0x80,1,{0xC3}},
	{0x81,1,{0x00}},
	{0x82,1,{0xBD}},
	{0x83,1,{0x00}},
	{0x84,1,{0xE8}},
	{0x85,1,{0x01}},
	{0x86,1,{0x16}},
	{0x87,1,{0x01}},
	{0x88,1,{0x5A}},
	{0x89,1,{0x01}},
	{0x8A,1,{0x8D}},
	{0x8B,1,{0x01}},
	{0x8C,1,{0xDB}},
	{0x8D,1,{0x02}},
	{0x8E,1,{0x18}},
	{0x8F,1,{0x02}},
	{0x90,1,{0x19}},
	{0x91,1,{0x02}},
	{0x92,1,{0x4E}},
	{0x93,1,{0x02}},
	{0x94,1,{0x89}},
	{0x95,1,{0x02}},
	{0x96,1,{0xB0}},
	{0x97,1,{0x02}},
	{0x98,1,{0xE4}},
	{0x99,1,{0x03}},
	{0x9A,1,{0x06}},
	{0x9B,1,{0x03}},
	{0x9C,1,{0x36}},
	{0x9D,1,{0x03}},
	{0x9E,1,{0x41}},
	{0x9F,1,{0x03}},
	{0xA0,1,{0x53}},
	{0xA2,1,{0x03}},
	{0xA3,1,{0x62}},
	{0xA4,1,{0x03}},
	{0xA5,1,{0x74}},
	{0xA6,1,{0x03}},
	{0xA7,1,{0x8F}},
	{0xA9,1,{0x03}},
	{0xAA,1,{0x98}},
	{0xAB,1,{0x03}},
	{0xAC,1,{0x9D}},
	{0xAD,1,{0x03}},
	{0xAE,1,{0xFF}},
	{0xAF,1,{0x00}},
	{0xB0,1,{0x2E}},
	{0xB1,1,{0x00}},
	{0xB2,1,{0x41}},
	{0xB3,1,{0x00}},
	{0xB4,1,{0x63}},
	{0xB5,1,{0x00}},
	{0xB6,1,{0x7D}},
	{0xB7,1,{0x00}},
	{0xB8,1,{0x93}},
	{0xB9,1,{0x00}},
	{0xBA,1,{0xA7}},
	{0xBB,1,{0x00}},
	{0xBC,1,{0xB7}},
	{0xBD,1,{0x00}},
	{0xBE,1,{0xC3}},
	{0xBF,1,{0x00}},
	{0xC0,1,{0xBD}},
	{0xC1,1,{0x00}},
	{0xC2,1,{0xE8}},
	{0xC3,1,{0x01}},
	{0xC4,1,{0x16}},
	{0xC5,1,{0x01}},
	{0xC6,1,{0x5A}},
	{0xC7,1,{0x01}},
	{0xC8,1,{0x8D}},
	{0xC9,1,{0x01}},
	{0xCA,1,{0xDB}},
	{0xCB,1,{0x02}},
	{0xCC,1,{0x18}},
	{0xCD,1,{0x02}},
	{0xCE,1,{0x19}},
	{0xCF,1,{0x02}},
	{0xD0,1,{0x4E}},
	{0xD1,1,{0x02}},
	{0xD2,1,{0x89}},
	{0xD3,1,{0x02}},
	{0xD4,1,{0xB0}},
	{0xD5,1,{0x02}},
	{0xD6,1,{0xE4}},
	{0xD7,1,{0x03}},
	{0xD8,1,{0x06}},
	{0xD9,1,{0x03}},
	{0xDA,1,{0x36}},
	{0xDB,1,{0x03}},
	{0xDC,1,{0x41}},
	{0xDD,1,{0x03}},
	{0xDE,1,{0x53}},
	{0xDF,1,{0x03}},
	{0xE0,1,{0x62}},
	{0xE1,1,{0x03}},
	{0xE2,1,{0x74}},
	{0xE3,1,{0x03}},
	{0xE4,1,{0x8F}},
	{0xE5,1,{0x03}},
	{0xE6,1,{0x98}},
	{0xE7,1,{0x03}},
	{0xE8,1,{0x9D}},
	{0xE9,1,{0x03}},
	{0xEA,1,{0xFF}},
	{0xFF,1,{0x24}},
	{0xFB,1,{0x01}},
	{0x9D,1,{0xB0}},
	{0xFF,1,{0x23}},
	{0xFB,1,{0x01}},
	{0x00,1,{0x02}},
	{0x05,1,{0x25}},
	{0x06,1,{0x01}},
	{0x07,1,{0x20}},
	{0x08,1,{0x04}},//change pwm to 20KHz
	{0x46,1,{0x45}},
	{0x17,1,{0xFF}},//change ic default cabc pwm
	{0x18,1,{0xEB}},
	{0x19,1,{0xE1}},
	{0x1A,1,{0xD7}},
	{0x1B,1,{0xC0}},
	{0x1C,1,{0xB5}},
	{0x1D,1,{0xA8}},
	{0x1E,1,{0xA0}},
	{0x1F,1,{0x80}},
	{0x20,1,{0x76}},
	{0x39,1,{0xFA}},

//add SRE code
	{0xFF,1,{0x22}},
	{0xFB,1,{0x01}},
	{0x5E,1,{0x1D}},
	{0x5F,1,{0x15}},
	{0x1A,1,{0x00}},
	{0x53,1,{0x00}},
	{0xFF,1,{0x10}},
	{0xFB,1,{0x01}},
	{0x35,1,{0x00}},    //Tearing Effect Line ON
	{0x44,2,{0x05,0x00}},
//add video mode code
#if (LCM_DSI_CMD_MODE)
	//{0xBB,1,{0x10}},//cmd mode
#else
	{0xBB,1,{0x03}}, //03 video mode,10 video sram
	{0x3B,5,{0x03,0x0A,0x0A,0x0A,0x0A}},
#endif
//enable CABC function.
	{0x53,1,{0x24}},  //Dimming off
	{0x55,1,{0x02}},
	{0x5E,1,{0x29}},

	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 100, {}},
//Display ON
	{0x29, 1, {0x00}},
//{REGFLAG_DELAY, 50, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY,20, {}},

	// Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 100, {}},
	{0x4F, 1, {0x01}},//ENTER DEEP SLEEP MODE
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
	//Sleep Out
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 100, {}},

	// Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table2(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

			case REGFLAG_DELAY :
				if (table[i].count <= 10)
					MDELAY(table[i].count);
				else
					MDELAY(table[i].count);
				break;

			case REGFLAG_END_OF_TABLE :
				break;

			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}


static void push_table(void* handle,struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

			case REGFLAG_DELAY :
				if (table[i].count <= 10)
					MDELAY(table[i].count);
				else
					MDELAY(table[i].count);
				break;

			case REGFLAG_END_OF_TABLE :
				break;

			default:
				dsi_set_cmd_by_cmdq(handle,cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}
// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

	params->dsi.switch_mode_enable = 1;//wangyq13 enable c/v swtich

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM                = LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order     = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq       = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding         = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format              = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	params->dsi.packet_size=256;
	//video mode timing

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active                = 2;
	params->dsi.vertical_backporch                  = 8;
	params->dsi.vertical_frontporch                 = 10;
	params->dsi.vertical_active_line                    = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active              = 10;
	params->dsi.horizontal_backporch                = 20;
	params->dsi.horizontal_frontporch               = 20;
	params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

	//begin:haobing modified
	/*BEGIN PN:DTS2013013101431 modified by s00179437 , 2013-01-31*/
	//improve clk quality
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 440; //this value must be in MTK suggested table
	params->dsi.CLK_HS_POST = 300; //add for screen mess
#else
	params->dsi.PLL_CLOCK = 430; //this value must be in MTK suggested table
#endif
	//params->dsi.pll_div1=0;       // div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
	//params->dsi.pll_div2=1;       // div2=0,1,2,3;div1_real=1,2,4,4
	//params->dsi.fbk_div =21;      // fref=26MHz, fvco=fref*(fbk_div)*2/(div1_real*div2_real)
	/*END PN:DTS2013013101431 modified by s00179437 , 2013-01-31*/
	//end:haobing modified

//add 20150128 enable esd
#if 1
#if (LCM_DSI_CMD_MODE)
	//params->dsi.cont_clock = 0;
	params->dsi.clk_lp_per_line_enable = 1;
#else
	params->dsi.ssc_disable = 1;
	params->dsi.cont_clock = 0;
	params->dsi.clk_lp_per_line_enable = 0;
#endif
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
#endif
}

static void lcm_init(void)
{
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
	MDELAY(2);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(12);

	// when phone initial , config output high, enable backlight drv chip
	push_table2(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	push_table2(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);

	mt_set_gpio_out(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);

	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
	MDELAY(2);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);

	//mt_set_gpio_out(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);
}

static void lcm_resume(void)
{
	//mt_set_gpio_out(GPIO_LCM_BL_EN, GPIO_OUT_ONE);

	/*mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
	MDELAY(6);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
	MDELAY(20);*/

	mt_set_gpio_out(GPIO_LCM_BL_EN, GPIO_OUT_ONE);
	//push_table2(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	/*BEGIN PN:DTS2013013101431 modified by s00179437 , 2013-01-31*/
	//delete high speed packet
	//data_array[0]=0x00290508;
	//dsi_set_cmdq(data_array, 1, 1);
	/*END PN:DTS2013013101431 modified by s00179437 , 2013-01-31*/
	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}

static void lcm_setbacklight(unsigned int level)
{
	unsigned char data_array[16];

	if (level > 255)
		level = 255;
	if (level < 3)
		level = 3;//for nova ic,level 1 and 2,pwm cycle less than 1%

	data_array[0] = level;
	dsi_set_cmdq_V2(0x51, 1, data_array, 1);
}

void* lcm_switch_mode(int mode)
{
	return NULL;
}


static unsigned int lcm_compare_id(void)
{
	unsigned  int ret = 0;
	ret = mt_get_gpio_in(GPIO_DISP_ID0_PIN);
	if (1 == ret)
		dprintf(0, "[LK]yassy_nt35695 found\n");
	return (1 == ret) ? 1: 0;
}

LCM_DRIVER nt35695_fhd_dsi_cmd_yassy_lcm_drv= {
	.name           = "nt35695_fhd_dsi_cmd_yassy",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,/*tianma init fun.*/
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
	.update         = lcm_update,
#endif
	.set_backlight      = lcm_setbacklight,
};
