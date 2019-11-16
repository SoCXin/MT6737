/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION LCM_WIDTHOR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
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

#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"
#include <cust_gpio_usage.h>
#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <platform/mt_pmic.h> 
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
	#include <mach/mt_pm_ldo.h>
	#include <mach/upmu_common.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)
#define GPIO_LCD_RST_PIN 	(GPIO28 | 0x80000000)   //GPIO28   ->reset
#define GPIO_LCM_VCI_EN_PIN (GPIO145 | 0x80000000)                        //VGP3-> VCI 1.8v      gpio145 ldo->LCD3.3V->vdd3 3.3v

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif


#ifdef BUILD_LK
#define	LCM_DEBUG(format, ...)   printf("lk " format "\n", ## __VA_ARGS__)
#elif defined(BUILD_UBOOT)
#define	LCM_DEBUG(format, ...)   printf("uboot " format "\n", ## __VA_ARGS__)
#else
#define	LCM_DEBUG(format, ...)   printk("kernel " format "\n", ## __VA_ARGS__)
#endif

static unsigned int lcm_esd_test = FALSE;      ///only for ESD test


// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE							1


static void init_lcm_registers(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00110500;		// Sleep Out
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(20);

	data_array[0] = 0x00351500;                          
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0xc0361500;                          
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00033902;
	data_array[1] = 0x00010044;
	dsi_set_cmdq(&data_array, 2, 1);	

	data_array[0] = 0x00511500;
	dsi_set_cmdq(&data_array, 1, 1);
	data_array[0] = 0x20531500;
	dsi_set_cmdq(&data_array, 1, 1);
	data_array[0] = 0x00551500;
	dsi_set_cmdq(&data_array, 1, 1);

	MDELAY(120);

	data_array[0] = 0x00290500;		// Display On
	dsi_set_cmdq(&data_array, 1, 1);

}

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

//		params->dbi.te_mode                 = LCM_DBI_TE_MODE_VSYNC_ONLY; //LCM_DBI_TE_MODE_VSYNC_OR_HSYNC;                                                
		//params->dbi.te_mode                 = LCM_DBI_TE_MODE_DISABLED;
//		params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;

        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   =  CMD_MODE;
        #else
		params->dsi.mode   = BURST_VDO_MODE;//ESYNC_PULSE_VDO_MODE;//; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
		#endif
	
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.packet_size=256;
		params->dsi.word_count=FRAME_WIDTH*3;

		params->dsi.vertical_sync_active				= 2;// 0x05
		params->dsi.vertical_backporch					= 4;//  0x0d
		params->dsi.vertical_frontporch					= 20; // 0x08
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 10;// 0x12
		params->dsi.horizontal_backporch				= 14; //0x5f
		params->dsi.horizontal_frontporch				= 30;//0x5f
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	    //params->dsi.LPX=8; 
		params->dsi.cont_clock                  = 1;

		// Bit rate calculation
		params->dsi.PLL_CLOCK = 450;
		//1 Every lane speed
		//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
		//params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
		//params->dsi.fbk_div =18;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

}

static void lcm_reset(void)
{
		mt_set_gpio_mode(GPIO_LCD_RST_PIN, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO_LCD_RST_PIN, GPIO_DIR_OUT);        
		mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
		MDELAY(20); // 1ms
		
		mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ZERO);
		MDELAY(20); // 1ms

		mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
		MDELAY(150); // 1ms
}


static void lcm_init(void)
{
	//GPIO28   ->reset
      //VGP3-> VCI 1.8v      gpio145 LDO ->LCD3.3V->vdd3 3.3v
	mt_set_gpio_mode(GPIO_LCM_VCI_EN_PIN, 0);
	mt_set_gpio_dir(GPIO_LCM_VCI_EN_PIN, GPIO_DIR_OUT);        
	mt_set_gpio_out(GPIO_LCM_VCI_EN_PIN, GPIO_OUT_ONE);
	MDELAY(2); // 1ms
	
	mt6325_upmu_set_rg_vgp3_vosel(3);   //3 means 1.8V
	mt6325_upmu_set_rg_vgp3_en(1);

    	LCM_DEBUG("------s6e3fa2_fhd1080_dsi_vdo----%s------\n",__func__);
	lcm_reset();
	init_lcm_registers();
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00280500;		// Display On
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(10);
	
	data_array[0] = 0x00100500;		// Display On
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(120);

	mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ZERO);
	MDELAY(10);

	mt_set_gpio_out(GPIO_LCM_VCI_EN_PIN, GPIO_OUT_ZERO);
	mt6325_upmu_set_rg_vgp3_en(0);
}


static void lcm_resume(void)
{
    LCM_DEBUG("------s6e3fa2_fhd1080_dsi_vdo----%s------\n",__func__);
	lcm_init();
}
         
#if (LCM_DSI_CMD_MODE)
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

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

//extern int IMM_GetOneChannelValue(int dwChannel, int deCount);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
#define VOLTAGE_FULL_RANGE_LCD 	1500 // VA voltage
#define ADC_PRECISE 		4096 // 12 bits
static unsigned int lcm_compare_id(void)
{
		return 1;
	int data[4] = {0,0,0,0};
	int res =0;
	int rawdata=0;
	int val=0;

	res =IMM_GetOneChannelValue(0,data,&rawdata);

	if(res<0) {
#ifdef BUILD_LK
		printf("s6e3fa2_fhd1080_dsi_vdo ,adc err= %d\n",res);  
#else
		printk("s6e3fa2_fhd1080_dsi_vdo ,adc err= %d\n",res);  
#endif
		return 0; 
	} else {
		val=rawdata*VOLTAGE_FULL_RANGE_LCD/ADC_PRECISE;
#ifdef BUILD_LK
		printf("s6e3fa2_fhd1080_dsi_vdo  chip_id  read value: %d\n",  val);
#else
		printk("s6e3fa2_fhd1080_dsi_vdo  chip_id  read value: %d\n",  val);
#endif
	}
	if(val < 100)
		return 1;
	else
		return 0;
}

static void lcm_set_backlight(unsigned int level)                                                                             
{
//	unsigned int default_level = 145;
	unsigned char mapped_level = 0;
	unsigned int data_array[16];
	
	//for LGE backlight IC mapping table
	if(level > 255) 
		mapped_level = 255;
	if(level > 0) 
		mapped_level = (unsigned char)level;
	else
		mapped_level=0;

	data_array[0] = (0x00511500 | (mapped_level<<24));		// Display On
	dsi_set_cmdq(&data_array, 1, 1);
}


static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char  buffer[2];
	int   array[4];

	/// please notice: the max return packet size is 1
	/// if you want to change it, you can refer to the following marked code
	/// but read_reg currently only support read no more than 4 bytes....
	/// if you need to read more, please let BinHan knows.
	/*
	   unsigned int data_array[16];
	   unsigned int max_return_size = 1;

	   data_array[0]= 0x00003700 | (max_return_size << 16);    

	   dsi_set_cmdq(&data_array, 1, 1);
	 */
	array[0] = 0x00013700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0A, buffer, 1);
	if(buffer[0]==0x1C)
	{
		return FALSE;
	}
	else
	{            
		return TRUE;
	}                                                                                                                        
#endif

}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();
	lcm_resume();

	return TRUE;
}



LCM_DRIVER s6e3fa2_fhd1080_dsi_vdo_lcm_drv = 
{
	.name			= "s6e3fa2_fhd1080_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.set_backlight	= lcm_set_backlight,
	//.compare_id     = lcm_compare_id,
	//	.esd_check = lcm_esd_check,
	//	.esd_recover = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
	.update         = lcm_update,
#endif
};

