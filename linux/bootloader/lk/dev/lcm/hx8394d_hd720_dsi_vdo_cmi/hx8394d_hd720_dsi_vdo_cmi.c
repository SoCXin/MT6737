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
#ifndef CONFIG_FPGA_EARLY_PORTING
#include <cust_i2c.h>
#endif
#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif

const static unsigned char LCD_MODULE_ID = 0x02;
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)


#define REGFLAG_DELAY             							0xFC
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#define HX8394D_HD720_ID  (0x0d)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)        (lcm_util.set_reset_pin(v))
#define MDELAY(n)               (lcm_util.mdelay(n))
#define UDELAY(n) 											(lcm_util.udelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[128];
};

static struct LCM_setting_table lcm_init_setting[] = {
	
	/*
	Note :

	Data ID will depends on the following rule.
	
		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/
	
	//  B9
	{0xB9,3,{0xFF,0x83,0x94}},
	
	//  B1
	{0xB1,15,{0x6C,0x0D,0x0D,0x12,0x04,0x11,0xF1,0x80,0xED,0xA1,0x23,0x80,0xC0,0xD2,0x58}},
	
	//  BA
	{0xBA,2,{0x33,0x83}},
	
	//  B2
	{0xB2,11,{0x00,0x64,0x0E,0x0D,0x32,0x1C,0x08,0x08,0x1C,0x4D,0x00}},
	
	//  B4
	{0xB4,12,{0x00,0xFF,0x51,0x5A,0x54,0x54,0x03,0x5A,0x01,0x60,0x20,0x60}},
	
	//  BC
	{0xBC,1,{0x07}},
	
	//  BF
	{0xBF,3,{0x41,0x0E,0x01}},
	
	//  D3
	{0xD3,30,{0x00,0x07,0x00,0x40,0x07,0x10,0x00,0x08,0x10,0x08,0x00,0x08,0x54,0x15,0x0E,0x05,0x0E,0x02,0x15,0x06,0x05,0x06,0x47,0x44,0x0A,0x0A,0x4B,0x10,0x07,0x07}},
	
	//  D5
	{0xD5,44,{0x1A,0x1A,0x1B,0x1B,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x24,0x25,0x18,0x18,0x26,0x27,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x20,0x21,0x18,0x18,0x18,0x18}},
	
	//  D6
	{0xD6,44,{0x1A,0x1A,0x1B,0x1B,0x0B,0x0A,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,0x21,0x20,0x58,0x58,0x27,0x26,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x25,0x24,0x18,0x18,0x18,0x18}},
	
	//GAMMA 2.2
	{0xE0,42,{0x00,0x0E,0x15,0x2B,0x2F,0x3f,0x22,0x3D,0x07,0x0B,0x0D,0x18,0x0F,0x13,0x15,0x13,0x14,0x08,0x12,0x14,0x17,0x00,0x0E,0x15,0x2B,0x2F,0x3f,0x22,0x3D,0x07,0x0B,0x0D,0x18,0x0F,0x13,0x15,0x13,0x14,0x08,0x12,0x14,0x17}},
	
	 /*
	 //GAMMAA 2.5
	{0xE0,42,{0x00,0x11,0x17,0x31,0x34,0x3f,0x26,0x43,0x07,0x0B,0x0D,0x18,0x0F,0x12,0x14,0x12,0x13,0x08,0x12,0x15,0x19,0x00,0x11,0x17,0x31,0x34,0x3f,0x26,0x43,0x07,0x0B,0x0D,0x18,0x0F,0x12,0x14,0x12,0x13,0x08,0x12,0x15,0x19}},
	 */
	
	//  CC
	{0xCC,1,{0x0d}},
	
	//  C0
	{0xC0,2,{0x30,0x14}},
	
	//  B6
	{0xB6,2,{0x49,0x49}},
	
	//  C7
	{0xC7,4,{0x00,0xC0,0x40,0xC0}},
	
	// Sleep Out
	{0x11,0,{}},
	{REGFLAG_DELAY, 120, {}},

	// Display ON
	{0x29,0,{}},
	{REGFLAG_DELAY, 20,{}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_init_data_array(void)
{
	unsigned int data_array[16];
	 
	data_array[0] = 0x00043902;
	data_array[1] = 0x9483FFB9;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(1);
		
	data_array[0] = 0x00033902;				
	data_array[1] = 0x008333BA;
	dsi_set_cmdq(&data_array,2, 1);
	MDELAY(1);
	 
	 
	   
	data_array[0] = 0x00103902; 					
	data_array[1] = 0x12126cB1;
	data_array[2] = 0xf1110434;
	data_array[3] = 0x2354FA80;//0x23543A81
	data_array[4] = 0x58D2C080;
	dsi_set_cmdq(&data_array, 5, 1);
	MDELAY(1);
		
		
	data_array[0] = 0x000c3902; 					
	data_array[1] = 0x0e6400B2;
	data_array[2] = 0x081c320d;
	data_array[3] = 0x004d1c08;
	dsi_set_cmdq(&data_array, 4, 1);
	MDELAY(1);
	   
	data_array[0] = 0x000d3902; 					
	data_array[1] = 0x51ff00B4;
	data_array[2] = 0x035a595a;
	data_array[3] = 0x2070015a;
	data_array[4] = 0x00000070;
	dsi_set_cmdq(&data_array, 5, 1);
	MDELAY(1);
		
	data_array[0] = 0x07BC1500;//			  
	dsi_set_cmdq(&data_array, 1, 1);

	data_array[0] = 0x00043902;						     
	data_array[1] = 0x010e41BF;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(1);
	
	
	data_array[0] = 0x001f3902; 					
	data_array[1] = 0x000700D3;
	data_array[2] = 0x00100740; 					
	data_array[3] = 0x00081008;
	data_array[4] = 0x0e155408; 					
	data_array[5] = 0x15020e05;
	data_array[6] = 0x47060506; 					
	data_array[7] = 0x4b0a0a44;
	data_array[8] = 0x00070710; 			   
	dsi_set_cmdq(&data_array, 9, 1);
	MDELAY(1);
	   
	data_array[0] = 0x002d3902; 						
	data_array[1] = 0x1b1a1aD5;
	data_array[2] = 0x0201001b;
	data_array[3] = 0x06050403;
	data_array[4] = 0x0a090807;
	data_array[5] = 0x1825240b;
	data_array[6] = 0x18272618;
	data_array[7] = 0x18181818;
	data_array[8] = 0x18181818;
	data_array[9] = 0x18181818;
	data_array[10] = 0x20181818;
	data_array[11] = 0x18181821;
	data_array[12] = 0x00000018;
	dsi_set_cmdq(&data_array, 13, 1);
	MDELAY(1);
	 
	data_array[0] = 0x002d3902; 						
	data_array[1] = 0x1b1a1aD6;
	data_array[2] = 0x090a0b1b;
	data_array[3] = 0x05060708;
	data_array[4] = 0x01020304;
	data_array[5] = 0x58202100;
	data_array[6] = 0x18262758;
	data_array[7] = 0x18181818;
	data_array[8] = 0x18181818;
	data_array[9] = 0x18181818;
	data_array[10] = 0x25181818;
	data_array[11] = 0x18181824;
	data_array[12] = 0x00000018;
	dsi_set_cmdq(&data_array, 13, 1);
	MDELAY(1);
	
		
	data_array[0] = 0x002b3902; 						
	data_array[1] = 0x130E00E0;
	data_array[2] = 0x203f3732;
	data_array[3] = 0x0D0B0740;
	data_array[4] = 0x14100E17;
	data_array[5] = 0x10061312;
	data_array[6] = 0x0D001710;
	data_array[7] = 0x3f383314;
	data_array[8] = 0x0A063F21;
	data_array[9] = 0x110D170C;
	data_array[10] = 0x07141213;
	data_array[11] = 0x00161211;
	dsi_set_cmdq(&data_array, 12, 1);
	MDELAY(1);
	
	data_array[0] = 0xBDC61500;// 			
	dsi_set_cmdq(&data_array, 1, 1);
 
	data_array[0] = 0x02BD1500;// 			
	dsi_set_cmdq(&data_array, 1, 1);
	
	data_array[0] = 0x000D3902; 						
	data_array[1] = 0xEEFFFFD8;
	data_array[2] = 0xFFA0FBEB;
	data_array[3] = 0xFBEBEEFF;
	data_array[4] = 0x000000A0;
	dsi_set_cmdq(&data_array, 5, 1);
	 
	data_array[0] = 0x05CC1500;// 			
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(1);

	//data_array[0] = 0x82361500;//			  
	//dsi_set_cmdq(&data_array, 1, 1);
	//MDELAY(1);
	
	data_array[0] = 0x00033902; 						
	data_array[1] = 0x001430C0;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(1);
	
		
	data_array[0] = 0x00053902; 						
	data_array[1] = 0x40c000c7;
	data_array[2] = 0x000000c0;
	dsi_set_cmdq(&data_array, 3, 1);
	MDELAY(1);
		
	data_array[0] = 0x00033902; 						
	data_array[1] = 0x004949B6;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(1);
		
				
	data_array[0] = 0x00110500;//			
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(150);
	
	data_array[0] = 0x00290500;   
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(10);
}
							
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    //Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 50, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    
    {REGFLAG_END_OF_TABLE, 0x00, {}}
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
				//MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
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

	params->dsi.mode   = BURST_VDO_MODE;	//BURST_VDO_MODE;	//SYNC_PULSE_VDO_MODE;

	//params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;
	//params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM				= LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

   // Highly depends on LCD driver capability.
	params->dsi.packet_size=256;
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
   
	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 12;
	params->dsi.vertical_frontporch					= 15;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 20;
	params->dsi.horizontal_backporch				= 70;
	params->dsi.horizontal_frontporch				= 70;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	    //params->dsi.LPX=8; 

		// Bit rate calculation
		params->dsi.PLL_CLOCK = 210;
}

static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(180);
    //push_table(lcm_init_setting, sizeof(lcm_init_setting) / sizeof(struct LCM_setting_table), 1);
   lcm_init_data_array();
}


static void lcm_suspend(void)
{
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}
static void lcm_resume(void)
{
	lcm_init(); 
}

static unsigned int lcm_compare_id(void)
{
#if 1
	char  buffer;
	unsigned int data_array[2];

	SET_RESET_PIN(1);
	MDELAY(10); 
	SET_RESET_PIN(0);
	MDELAY(10); 
	SET_RESET_PIN(1);
	MDELAY(10);	

	data_array[0] = 0x00043902;
	data_array[1] = 0x9483FFB9;
	dsi_set_cmdq(&data_array, 2, 1);
	MDELAY(1);
		
	data_array[0] = 0x00023902;				
	data_array[1] = 0x000033BA;
	dsi_set_cmdq(&data_array,2, 1);
	MDELAY(1);	




	data_array[0] = 0x00013700;
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0xdc, &buffer, 1);

	#ifdef BUILD_LK
		printf("%s, LK debug: hx8394d id = 0x%08x\n", __func__, buffer);
    #else
		printk("%s, kernel debug: hx8394d id = 0x%08x\n", __func__, buffer);
    #endif

	return (buffer == HX8394D_HD720_ID ? 1 : 0);
#else
	return 1;
#endif
}


LCM_DRIVER hx8394d_hd720_dsi_vdo_cmi_lcm_drv = 
{
    .name			= "hx8394d_hd720_dsi_vdo_cmi",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
   
};
