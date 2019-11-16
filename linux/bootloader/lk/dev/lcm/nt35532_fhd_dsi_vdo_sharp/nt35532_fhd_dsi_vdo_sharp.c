#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/upmu_common.h>
	#include <platform/mt_gpio.h>
	#include <platform/mt_i2c.h> 
	#include <platform/mt_pmic.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_pm_ldo.h>
    #include <mach/mt_gpio.h>
#endif
#include <cust_gpio_usage.h>
#include <cust_i2c.h>


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH  										(1080)
#define FRAME_HEIGHT 										(1920)

#define REGFLAG_DELAY             							 0XFFFA
#define REGFLAG_UDELAY             							 0xFFFB
#define REGFLAG_PORT_SWAP									 0xFFFC
#define REGFLAG_END_OF_TABLE      							 0xFFFD   // END OF REGISTERS MARKER



#define LCM_DSI_CMD_MODE									 0
#define LCM_ID_NT35532                                       0x80

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
static LCM_UTIL_FUNCS lcm_util = {0};
#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


//Gate IC Driver


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};

//basic sequence
static struct LCM_setting_table lcm_initialization_setting1[] = {
	{0xFF,1,{0x00}},
	{0xD3,1,{0x08}},
	{0xD4,1,{0x0E}},
	{0x11,0,{}},
	{REGFLAG_DELAY, 100, {}},
	{0x29,0,{}},
	{REGFLAG_DELAY, 40, {}},
};
//video mode input (MIPI)=Normal Video mode input "60Hz<=>1Hz"
static struct LCM_setting_table lcm_initialization_setting2[] = {
	{0xFF,1,{0x05}},
	{0xFB,1,{0x01}},
	{0xAF,1,{0xC0}},
	{0xB0,1,{0x3A}},
	{0xB1,1,{0x2E}},
	{0xB2,1,{0x70}},
	{0xFF,1,{0x00}},
	{0xFB,1,{0x01}},
	{0xB0,1,{0x09}},
	{0xD3,1,{0x05}},
	{0xD4,1,0x0E},
	{0x11,0,{}},
	{REGFLAG_DELAY, 100, {}},
	{0x29,0,{}},
	{REGFLAG_DELAY, 40, {}},
};
//30Hz<=>60Hz Input (MIPI) = 1 Frame(1/60sec)+1/60sec interval
static struct LCM_setting_table lcm_initialization_setting3[] = {
	{0xFF,1,{0x00}},//used to select page
	{0xFB,1,{0x01}},//used to select the control value of CMD2 Page0
	{0xAF,1,{0x40}},//3GAMMA_BLUE_NEGATIVE
	{0xB0,1,{0x3A}},//
	{0xB1,1,{0x16}},
	{0xB2,1,{0x70}},
	{0xFF,1,{0x00}},
	{0xFB,1,{0x01}},
	{0xB0,1,{0x19}},
	{0xD3,1,{0x06}},//0x08 //0x05
	{0xD4,1,{0x0E}},
	{0x11,0,{}},
	{REGFLAG_DELAY, 100, {}},
	{0x29,0,{}},
	{REGFLAG_DELAY, 40, {}},
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28,0,{}},
	{REGFLAG_DELAY, 17, {}},
	{0x10,0,{}},
	{REGFLAG_DELAY, 100, {}},
	{0xFF,1,{0x05}},
	{0xFB,1,{0x01}},
	{0xD7,1,{0x30}},
	{0xD8,1,{0x70}},
};
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++)
	{
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

			case REGFLAG_DELAY :
				if(table[i].count <= 10)
					MDELAY(table[i].count);
				else
					MDELAY(table[i].count);
				break;
				
			case REGFLAG_UDELAY :
				UDELAY(table[i].count);
				break;

			case REGFLAG_END_OF_TABLE :
				break;

			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
	unsigned char buffer[2];
	unsigned int array[16];  


	array[0]=0x00043902;
	array[1]=0x9983FFB9;// page enable
	dsi_set_cmdq(&array, 2, 1);
	MDELAY(10);

	array[0] = 0x00023700;// return byte number
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xDB, buffer, 1);
	id = buffer[0]; 

	return (LCM_ID_NT35532 == id)?1:0;
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
	
		// DSI
		/* Command mode setting */
        params->dsi.LANE_NUM		    = LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

		params->dsi.vertical_sync_active				= 2;//2;//2;
		params->dsi.vertical_backporch					= 6;//6;//5;//5;//5
		params->dsi.vertical_frontporch					= 14;//14;//10;//10;//13
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

        params->dsi.horizontal_sync_active		 	    = 8;//8;//8;//10;//8
		params->dsi.horizontal_backporch				= 16;//16;//16;//20;//16
		params->dsi.horizontal_frontporch				= 72;//72;//40;//40;//72
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		// Bit rate calculation
	    params->dsi.PLL_CLOCK = 430;
		
		params->dsi.lfr_enable = 1;
		params->dsi.lfr_mode = 3; //mode :1--static mode,2---dynamic mode,3----both mode
		params->dsi.lfr_type = 0;
		params->dsi.lfr_skip_num= 1;
	
}

static void lcm_init(void)
{    
	unsigned int array[16]; 
/*
	array[0]=0x00FF1500;
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(1);

	array[0]=0x08D31500;
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(1);

	array[0]=0x0ED41500;
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(1);

	array[0]=0x00110500;
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(100);

	array[0]=0x00290500;
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(40);
*/
    int i=3;
#ifdef BUILD_LK
    dprintf(0, "[LK]lcm_initialization_setting----%d\n",i);    	
#else
	printk("[KERNEL]lcm_initialization_setting----%d\n",i);
#endif
	push_table(lcm_initialization_setting3, sizeof(lcm_initialization_setting3) / sizeof(struct LCM_setting_table), 1);
    	
}


static void lcm_suspend(void)
{	
	unsigned int array[16]; 

    array[0]=0x00280500;
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(40);

	array[0]=0x00100500;
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(100);

	
	array[0]=0x05FF1500;
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(1);
	
	array[0]=0x01FB1500;
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(1);
	
	array[0]=0x30D71500;
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(1);

	array[0]=0x70D81500;
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(1);

    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(50);

}


static void lcm_resume(void)
{
	 lcm_init();
}



static void lcm_init_power(void)
{
}

static void lcm_suspend_power(void)
{
}

static void lcm_resume_power(void)
{
}



LCM_DRIVER nt35532_fhd_dsi_vdo_sharp_lcm_drv = 
{
    .name			= "nt35532_fhd_dsi_vdo_sharp_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.init_power     = lcm_init_power,
	.resume_power   = lcm_resume_power,
	.suspend_power  = lcm_suspend_power,
	.resume         = lcm_resume,
	.suspend        = lcm_suspend,
};

