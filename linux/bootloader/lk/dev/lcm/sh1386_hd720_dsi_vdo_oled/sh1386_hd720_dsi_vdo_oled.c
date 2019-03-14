#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
    #include <platform/disp_drv_platform.h>
	
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    #include <linux/delay.h>
    #include <mach/mt_gpio.h>
#endif
#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)

#define REGFLAG_DELAY             							0xFC
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

#define LCM_RM68200_ID (0x6820)


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

#define   LCM_DSI_CMD_MODE							(0)

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[128];
};

static struct LCM_setting_table rm68200_hd720_dsi_video_hf[] = {

	{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},
	{0xB0,4,{0x0F,0x0F,0x1E,0x14}},
	{0xB2,1,{0x00}},
	{0xB6,1,{0x02}},//0x03	
	
	{0xC0,20,{0x03,0x00,0x06,0x07,0x08,0x09,0x00,0x00,
	          0x00,0x00,0x02,0x00,0x0A,0x0B,0x0C,0x0D,
	          0x00,0x00,0x00,0x00}},
	{0xC1,16,{0x08,0x24,0x24,0x01,0x18,0x24,0x9f,0x85,0x08,0x24,
				 0x24,0x01,0x18,0x24,0x95,0x85}},
	{0xC2,24,{0x03,0x05,0x1B,0x24,0x13,0x31,0x01,0x05,
				 0x1B,0x24,0x13,0x31,0x03,0x05,0x1B,0x38,
				 0x00,0x11,0x02,0x05,0x1B,0x38,0x00,0x11}}, 
	{0xC3,24,{0x02,0x05,0x1B,0x24,0x13,0x11,0x03,0x05,
				 0x1B,0x24,0x13,0x11,0x03,0x05,0x1B,0x38,
				 0x00,0x11,0x02,0x05,0x1B,0x38,0x00,0x11}},		  
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}}, 
	{0xB5,1,{0x1E}}, 
	{0xB6,1,{0x2D}}, 
	{0xB7,1,{0x04}}, 
	{0xB8,1,{0x05}}, 
	{0xB9,1,{0x04}}, 
	{0xBA,1,{0x14}}, 
	{0xBB,1,{0x2f}}, 
	{0xBE,1,{0x12}}, 
	{0xC2,3,{0x00,0x35,0x0F}}, 
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x02}}, 
	{0xC9,1,{0x13}}, 
	{0xD4,3,{0x02,0x04,0x2C}}, 
	{0xE1,35,{0x00,0x91,0xAE,0xCB,0xe6,0x54,0xff,0x1e,0x33,0x43,
				 0x55,0x4f,0x66,0x78,0x8b,0x55,0x9d,0xac,0xc0,0xcf,
				 0x55,0xe0,0xe8,0xf2,0xfb,0xaa,0x03,0x0d,0x15,0x1f,
				 0xaa,0x27,0x2c,0x31,0x34}}, 
	{0xE2,35,{0x00,0xad,0xc6,0xe4,0xfd,0x55,0x11,0x2a,0x3b,0x49,
				 0x55,0x54,0x6b,0x7c,0x8f,0x55,0xa1,0xaf,0xc3,0xd1,
				 0x55,0xe2,0xea,0xf3,0xfc,0xaa,0x04,0x0e,0x15,0x20,
				 0xaa,0x28,0x2d,0x32,0x35}}, 
	{0xE3,35,{0x55,0x05,0x1E,0x37,0x4b,0x55,0x5a,0x64,0x72,0x7f,
				 0x55,0x8b,0xa3,0xb8,0xd1,0xa5,0xe4,0xf6,0x0e,0x23,
				 0xaa,0x39,0x42,0x4f,0x59,0xaa,0x64,0x70,0x7a,0x86,
				 0xaa,0x90,0x96,0x9c,0x9f}},
	{0x8F,6,{0x5A,0x96,0x3C,0xC3,0xA5,0x69}},
	{0x89,1,{0x00}},
	{0x8C,3,{0x55,0x49,0x53}},
	{0x9A,1,{0x5A}},
	{0xFF,4,{0xA5,0x5A,0x13,0x86}},
	{0xFE,2,{0x01,0x54}},
	{0x35,1,{0x00}},
	
	{0x11,0x01,{0x00}},     
	{REGFLAG_DELAY, 150, {}},
	{0x29,0x01,{0x00}},
	{REGFLAG_DELAY, 50, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    // Display ON
	{0x29, 0, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_backlight_level_setting_C1[]={
	{0xc1,2,{0x00,0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},	
};
	
static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 0, {0x00}},
        {REGFLAG_DELAY, 60, {}},
    // Sleep Mode On
	{0x10, 0, {0x00}},
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
                MDELAY(table[i].count);
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

	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = BURST_VDO_MODE;
#endif

	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 4;
	params->dsi.vertical_backporch = 15;
	params->dsi.vertical_frontporch = 15;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 20;
	params->dsi.horizontal_frontporch = 32;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    
  params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.PLL_CLOCK = 208; 

}

static void lcm_init(void)
{
	  SET_RESET_PIN(1);
	  MDELAY(20);
	  SET_RESET_PIN(0);
	  MDELAY(10);
	  SET_RESET_PIN(1);
	  MDELAY(20); 

    push_table(rm68200_hd720_dsi_video_hf, sizeof(rm68200_hd720_dsi_video_hf) / sizeof(struct LCM_setting_table), 1);
#if defined(BUILD_LK)		
	printf("lcm_init sh1386 lcm init\n");
#endif    
}


static void lcm_suspend(void)
{
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);

}
static void lcm_resume(void)
{
	  lcm_init();
}

static void lcm_setbacklight(unsigned int level)
{
	if(level<50) level=50;
	if(level>255) level=255;
    
	level = level*(1023-50)/255+50;
	lcm_backlight_level_setting_C1[0].para_list[0]=level>>8;
	lcm_backlight_level_setting_C1[0].para_list[1]=level&0xff;
	
	dsi_set_cmdq_V2(0xC1, 2,lcm_backlight_level_setting_C1[0].para_list, 1);// backlight adjust
}

LCM_DRIVER sh1386_hd720_dsi_vdo_oled_lcm_drv = 
{
    .name           = "sh1386_hd720_dsi_vdo_oled",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
	  .set_backlight	= lcm_setbacklight,    
     //.compare_id     = lcm_compare_id,
   
};
