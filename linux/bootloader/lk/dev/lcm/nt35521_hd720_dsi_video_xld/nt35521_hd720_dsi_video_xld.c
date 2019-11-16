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

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)        (lcm_util.set_reset_pin(v))
#define MDELAY(n)               (lcm_util.mdelay(n))

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
//update initial param for IC nt35521 0.01
static struct LCM_setting_table nt35521_hd720_dsi_video_xld[] = {
	

	{0xFF,4,{0xAA,0x55,0x25,0x01}},
{0xFC,1,{0x08}},

{0xFC,1,{0x00}},
      
{0x6F,1,{0x21}},
{0xF7,1,{0x01}},

{0x6F,1,{0x21}},
{0xF7,1,{0x00}},
{0xFF,4,{0xAA,0x55,0x25,0x00}}, 

        
{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},                
{0xC8,1,{0x83}},                
        
{0xB1,2,{0x68,0x27}},               
        
{0xB6,1,{0x08}},                
        
{0x6F,1,{0x02}},                
{0xB8,1,{0x08}},                
        
{0xBB,2,{0x74,0x44}},               
        
{0xBC,2,{0x00,0x00}},               
        
{0xBD,5,{0x02,0xB0,0x1E,0x1E,0x00}},        
{0xC5,2,{0x01,0x07}},       
        
{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},                
        
{0xB0,2,{0x05,0x05}},               
{0xB1,2,{0x05,0x05}},   
{0xB6,2,{0x05,0x05}},           
        
{0xBC,2,{0xA0,0x01}},           
{0xBD,2,{0xA0,0x01}},               
        
{0xCA,1,{0x00}},        
            
{0xC0,1,{0x04}},                
        
{0xBE,1,{0x6d}},
        
{0xB3,2,{0x37,0x37}},               
{0xB4,2,{0x0F,0x0F}},               
        
{0xB9,2,{0x46,0x46}},           
{0xBA,2,{0x25,0x25}},               
        
{0xF0,5,{0x55,0xAA,0x52,0x08,0x02}},                
        
{0xEE,1,{0x03}},            
      
{0xB0,16,{0x00,0xf6,0x01,0x00,0x01,0x0c,0x01,0x19,0x01,0x25,0x01,0x3b,0x01,0x4f,0x01,0x70}},            
{0xB1,16,{0x01,0x8f,0x01,0xBf,0x01,0xe3,0x02,0x22,0x02,0x59,0x02,0x5b,0x02,0x8f,0x02,0xc6}},            
{0xB2,16,{0x02,0xeb,0x03,0x1e,0x03,0x3c,0x03,0x75,0x03,0x92,0x03,0xaf,0x03,0xc6,0x03,0xD8}},            
{0xB3,4,{0x03,0xE6,0x03,0xFF}},         
{0xB4,16,{0x00,0xb0,0x00,0xBa,0x00,0xcd,0x00,0xe0,0x00,0xee,0x01,0x0d,0x01,0x27,0x01,0x4c}},            
{0xB5,16,{0x01,0x6d,0x01,0xa7,0x01,0xd3,0x02,0x15,0x02,0x4e,0x02,0x50,0x02,0x86,0x02,0xbf}},            
{0xB6,16,{0x02,0xe5,0x03,0x1D,0x03,0x3B,0x03,0x70,0x03,0x90,0x03,0xa9,0x03,0xC1,0x03,0xDA}},            
{0xB7,4,{0x03,0xE6,0x03,0xFF}},         
{0xB8,16,{0x00,0x00,0x00,0x24,0x00,0x4c,0x00,0x66,0x00,0x7b,0x00,0xa4,0x00,0xca,0x01,0x0b}},            
{0xB9,16,{0x01,0x36,0x01,0x78,0x01,0xb2,0x02,0x05,0x02,0x3d,0x02,0x3f,0x02,0x77,0x02,0xb2}},            
{0xBA,16,{0x02,0xd8,0x03,0x1b,0x03,0x3d,0x03,0x7a,0x03,0x9a,0x03,0xba,0x03,0xd6,0x03,0xdf}},            
{0xBB,4,{0x03,0xfb,0x03,0xFF}},         
  
        
{0xF0,5,{0x55,0xAA,0x52,0x08,0x06}},                
{0xB0,2,{0x29,0x2A}},               
{0xB1,2,{0x10,0x12}},               
{0xB2,2,{0x14,0x16}},               
{0xB3,2,{0x18,0x1A}},               
{0xB4,2,{0x08,0x0A}},               
{0xB5,2,{0x2E,0x2E}},               
{0xB6,2,{0x2E,0x2E}},               
{0xB7,2,{0x2E,0x2E}},               
{0xB8,2,{0x2E,0x00}},               
{0xB9,2,{0x2E,0x2E}},               
{0xBA,2,{0x2E,0x2E}},               
{0xBB,2,{0x01,0x2E}},               
{0xBC,2,{0x2E,0x2E}},               
{0xBD,2,{0x2E,0x2E}},               
{0xBE,2,{0x2E,0x2E}},               
{0xBF,2,{0x0B,0x09}},               
{0xC0,2,{0x1B,0x19}},               
{0xC1,2,{0x17,0x15}},               
{0xC2,2,{0x13,0x11}},               
{0xC3,2,{0x2A,0x29}},               
{0xE5,2,{0x2E,0x2E}},               
{0xC4,2,{0x29,0x2A}},               
{0xC5,2,{0x1B,0x19}},               
{0xC6,2,{0x17,0x15}},               
{0xC7,2,{0x13,0x11}},               
{0xC8,2,{0x01,0x0B}},               
{0xC9,2,{0x2E,0x2E}},               
{0xCA,2,{0x2E,0x2E}},               
{0xCB,2,{0x2E,0x2E}},               
{0xCC,2,{0x2E,0x09}},               
{0xCD,2,{0x2E,0x2E}},               
{0xCE,2,{0x2E,0x2E}},               
{0xCF,2,{0x08,0x2E}},               
{0xD0,2,{0x2E,0x2E}},               
{0xD1,2,{0x2E,0x2E}},               
{0xD2,2,{0x2E,0x2E}},               
{0xD3,2,{0x0A,0x00}},               
{0xD4,2,{0x10,0x12}},               
{0xD5,2,{0x14,0x16}},               
{0xD6,2,{0x18,0x1A}},               
{0xD7,2,{0x2A,0x29}},               
{0xE6,2,{0x2E,0x2E}},               
{0xD8,5,{0x00,0x00,0x00,0x00,0x00}},                
{0xD9,5,{0x00,0x00,0x00,0x00,0x00}},                
{0xE7,1,{0x00}},                
        
{0xF0,5,{0x55,0xAA,0x52,0x08,0x03}},                
{0xB0,2,{0x00,0x00}},               
{0xB1,2,{0x00,0x00}},               
{0xB2,5,{0x05,0x00,0x00,0x00,0x00}},                
        
{0xB6,5,{0x05,0x00,0x00,0x00,0x00}},                
{0xB7,5,{0x05,0x00,0x00,0x00,0x00}},                
        
{0xBA,5,{0x57,0x00,0x00,0x00,0x00}},            
{0xBB,5,{0x57,0x00,0x00,0x00,0x00}},                
        
{0xC0,4,{0x00,0x00,0x00,0x00}},             
{0xC1,4,{0x00,0x00,0x00,0x00}},             
        
{0xC4,1,{0x60}},                
{0xC5,1,{0x40}},                
        
{0xF0,5,{0x55,0xAA,0x52,0x08,0x05}},                
{0xBD,5,{0x03,0x01,0x03,0x03,0x03}},         
{0xB0,2,{0x17,0x06}},               
{0xB1,2,{0x17,0x06}},               
{0xB2,2,{0x17,0x06}},               
{0xB3,2,{0x17,0x06}},               
{0xB4,2,{0x17,0x06}},               
{0xB5,2,{0x17,0x06}},               
        
{0xB8,1,{0x00}},                
{0xB9,1,{0x00}},                
{0xBA,1,{0x00}},                
{0xBB,1,{0x02}},                
{0xBC,1,{0x00}},                
        
{0xC0,1,{0x07}},                
        
{0xC4,1,{0x80}},                
{0xC5,1,{0xA4}},                
        
{0xC8,2,{0x05,0x30}},               
{0xC9,2,{0x01,0x31}},               
        
{0xCC,3,{0x00,0x00,0x3C}},              
{0xCD,3,{0x00,0x00,0x3C}},              
        
{0xD1,5,{0x00,0x05,0x09,0x07,0x10}},            
{0xD2,5,{0x00,0x05,0x0E,0x07,0x10}},                
        
{0xE5,1,{0x06}},                
{0xE6,1,{0x06}},                
{0xE7,1,{0x06}},                
{0xE8,1,{0x06}},                
{0xE9,1,{0x06}},                
{0xEA,1,{0x06}},                
        
{0xED,1,{0x30}},  					

	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}}, 
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},

	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.


	// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}	
	
};
							
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
    {REGFLAG_DELAY, 20, {}},

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

    params->dsi.mode   = SYNC_PULSE_VDO_MODE;

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM				= LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

   // Highly depends on LCD driver capability.
   //video mode timing

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
   
	params->dsi.vertical_sync_active 			   = 4;
	params->dsi.vertical_backporch			   = 16;
	params->dsi.vertical_frontporch			   = 16;
	params->dsi.vertical_active_line 			   = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active		   = 10;
	params->dsi.horizontal_backporch 			   = 64;
	params->dsi.horizontal_frontporch			   = 64;
	params->dsi.horizontal_active_pixel		   = FRAME_WIDTH;

	//improve clk quality
	params->dsi.PLL_CLOCK = 208; //this value 
     //  params->dsi.HS_TRAIL = 120;

    params->dsi.compatibility_for_nvk = 1;
    params->dsi.ssc_disable = 1;

}

static void lcm_init(void)
{
    	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50);
	SET_RESET_PIN(1);
	MDELAY(120);
    
    push_table(nt35521_hd720_dsi_video_xld, sizeof(nt35521_hd720_dsi_video_xld) / sizeof(struct LCM_setting_table), 1);
    //LCD_DEBUG("uboot:tm_nt35521_lcm_init\n");
}


static void lcm_suspend(void)
{
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
    //LCD_DEBUG("uboot:tm_nt35521_lcm_suspend\n");
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50);
}
static void lcm_resume(void)
{
	lcm_init();
    //LCD_DEBUG("uboot:tm_nt35521_lcm_resume\n");

}

static unsigned int lcm_compare_id(void)
{
#if 0
    unsigned char LCD_ID_value = 0;
    LCD_ID_value = which_lcd_module_triple();
    if(LCD_MODULE_ID == LCD_ID_value)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#else
	return 1;
#endif
}
LCM_DRIVER nt35521_hd720_dsi_video_xld_lcm_drv =
{
    .name           = "nt35521_hd720_dsi_video_xld",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,/*tianma init fun.*/
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
   
};
