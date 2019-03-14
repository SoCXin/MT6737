#ifndef BUILD_LK
#include <linux/string.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define LCM_ID       (0x69)
#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0XFD   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

static unsigned int need_set_lcm_addr = 1;
struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_compare_id_setting[] = {
	// Display off sequence
	{0xf0, 5, {0x55, 0xaa, 0x52, 0x08, 0x01}},
	{REGFLAG_DELAY, 10, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_vdo_initialization_setting[] = {

    {0x00,1 ,{0x00}},
    {0xff,3 ,{0x12,0x89,0x01}},
    {0x00,1 ,{0x80}},
    {0xff,2 ,{0x12,0x89}},
    {0x00,1 ,{0x90}},
    {0xff,1 ,{0xa0}},
//-------------------- panel setting --------------------------------//    
    {0x00,1 ,{0x80}},
    {0xc0,8 ,{0x4a,0x00,0x10,0x10,0x96,0x01,0x68,0x40}},
    {0x00,1 ,{0x90}}, 
    {0xc0,3 ,{0x3b,0x01,0x09}},
    {0x00,1 ,{0x8c}}, 
    {0xc0,1 ,{0x00}}, 
    {0x00,1 ,{0x80}}, 
    {0xc1,1 ,{0x33}},    
    //{0x00,1 ,{0x92}},  
    //{0xb3,2 ,{0x01,0xbc}},  
//-------------------- power setting --------------------------------//
    {0x00,1 ,{0x85}},
    {0xc5,3 ,{0x0a,0x0a,0x49}},      
    {0x00,1 ,{0x00}},
    {0xd8,2 ,{0x29,0x29}},    
    {0x00,1 ,{0x00}},    
    {0xd9,2 ,{0x00,0x27}},    
    {0x00,1 ,{0x84}},    
    {0xc4,1 ,{0x02}},       
    {0x00,1 ,{0x93}},    
    {0xc4,1 ,{0x04}},
    {0x00,1 ,{0x96}},    
    {0xf5,1 ,{0xe7}},    
    {0x00,1 ,{0xa0}},    
    {0xf5,1 ,{0x4a}},    
    {0x00,1 ,{0x8a}},    
    {0xc0,1 ,{0x11}},     
    {0x00,1 ,{0x83}},    
    {0xf5,1 ,{0x81}},      
//-------------------- for Power IC ---------------------------------//    
    {0x00,1 ,{0x90}},    
    {0xc4,2 ,{0x96,0x05}},
//-------------------- panel timing state control -------------------//        
    {0x00,1 ,{0x80}},     
    {0xcb,15 ,{0x00,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x00,0x00,0x00,0x00,0x00,0x14,0x14}},    
    {0x00,1 ,{0x90}}, 
    {0xcb,7 ,{0x14,0x00,0x00,0x14,0x14,0x00,0x00}}, 
//-------------------- panel pad mapping control --------------------//
    {0x00,1 ,{0x80}},
    {0xcc,14 ,{0x00,0x05,0x07,0x11,0x15,0x13,0x17,0x0d,0x00,0x00,0x00,0x00,0x09,0x0f}},
    {0x00,1 ,{0x90}},  
    {0xcc,15 ,{0x0b,0x00,0x00,0x01,0x03,0x00,0x00,0x00,0x06,0x08,0x12,0x16,0x14,0x18,0x0e}},    
    {0x00,1 ,{0xa0}},  
    {0xcc,13 ,{0x00,0x00,0x00,0x00,0x0a,0x10,0x0c,0x00,0x00,0x02,0x04,0x00,0x00}}, 
    {0x00,1 ,{0xb0}}, 
    {0xcc,14 ,{0x00,0x04,0x02,0x14,0x18,0x12,0x16,0x0c,0x00,0x00,0x00,0x00,0x10,0x0a}}, 
    {0x00,1 ,{0xc0}}, 
    {0xcc,15 ,{0x0e,0x00,0x00,0x08,0x06,0x00,0x00,0x00,0x03,0x01,0x13,0x17,0x11,0x15,0x0b}}, 
    {0x00,1 ,{0xd0}}, 
    {0xcc,13 ,{0x00,0x00,0x00,0x00,0x0f,0x09,0x0d,0x00,0x00,0x07,0x05,0x00,0x00}}, 
//-------------------- panel timing setting -------------------------//
    {0x00,1 ,{0x80}},
    {0xce,6 ,{0x87,0x03,0x2a,0x86,0x85,0x84}},
    {0x00,1 ,{0x90}},
    {0xce,9 ,{0x34,0xfc,0x2a,0x04,0xfd,0x04,0xfe,0x04,0xff}},
    {0x00,1 ,{0xa0}},
    {0xce,15 ,{0x30,0x87,0x00,0x00,0x2a,0x00,0x86,0x01,0x00,0x85,0x02,0x00,0x84,0x03,0x00}},

    {0x00,1 ,{0xb0}},
    {0xce,15 ,{0x30,0x83,0x04,0x00,0x2a,0x00,0x82,0x05,0x00,0x81,0x06,0x00,0x80,0x07,0x00}},
    {0x00,1 ,{0xc0}},
    {0xce,15 ,{0x30,0x87,0x00,0x00,0x1d,0x2a,0x86,0x01,0x00,0x85,0x02,0x00,0x84,0x03,0x00}},
    {0x00,1 ,{0xd0}},   
    {0xce,15 ,{0x30,0x83,0x04,0x00,0x1d,0x2a,0x82,0x05,0x00,0x81,0x06,0x00,0x80,0x07,0x00}},
    {0x00,1 ,{0xf0}},  
    {0xce,6 ,{0x01,0x20,0x01,0x81,0x00,0x00}}, 
//-------------------- gamma ----------------------------------------//
    {0x00,1 ,{0x00}},
    {0xe1,16 ,{0x04,0x62,0x6c,0x7a,0x81,0x97,0x8f,0xa0,0x5b,0x4c,0x58,0x45,0x34,0x26,0x1b,0x05}},    
    {0x00,1 ,{0x00}},    
    {0xe2,16 ,{0x04,0x62,0x6c,0x7a,0x81,0x97,0x8f,0xa0,0x5b,0x4c,0x58,0x45,0x34,0x26,0x1b,0x05}},    
    {0x00,1 ,{0x00}},      //CMD2 disable 
    {0xff,3 ,{0xff,0xff,0xff}},     
    
               
    {0x11,1, {0x00}},     
    {REGFLAG_DELAY, 150, {}},
    {0x29,1,{0x00}},
    {REGFLAG_DELAY, 50, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 1, {0x00}},
       {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},  //
	{REGFLAG_END_OF_TABLE, 0x00, {}}  
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	 {0x28, 1, {0x00}},
	 {REGFLAG_DELAY, 40, {}},  //20
	 // Sleep Mode On
	 {0x10, 1, {0x00}},
	 {REGFLAG_DELAY, 150, {}},
	 	
	 {REGFLAG_END_OF_TABLE, 0x00, {}}
};




static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
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
    memcpy((void*)&lcm_util, (void*)util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
		memset((void*)params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
#else
		params->dsi.mode   = BURST_VDO_MODE; //SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE
#endif
		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_THREE_LANE;
		//params->dsi.data_format.color_order      = LCM_COLOR_ORDER_RGB;
		//params->dsi.data_format.trans_seq      = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		//params->dsi.data_format.padding      = LCM_DSI_PADDING_ON_LSB;
		
//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch					= 16; //12
		params->dsi.vertical_frontporch					= 16;  //16
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 10;
		params->dsi.horizontal_backporch				= 34;
		params->dsi.horizontal_frontporch				= 34; //34
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.PLL_CLOCK = 286; 

}

static void lcm_init(void)
{
	SET_RESET_PIN(1);
	//MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50); //20
	SET_RESET_PIN(1);
	MDELAY(50);
	push_table(lcm_vdo_initialization_setting, sizeof(lcm_vdo_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	need_set_lcm_addr = 1;
}

static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{

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

	// need update at the first time
	if(need_set_lcm_addr)
	{
		data_array[0]= 0x00053902;
		data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
		data_array[2]= (x1_LSB);
		dsi_set_cmdq(data_array, 3, 1);
		
		data_array[0]= 0x00053902;
		data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
		data_array[2]= (y1_LSB);
		dsi_set_cmdq(data_array, 3, 1);
		
		need_set_lcm_addr = 0;
	}
	
	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

static unsigned int lcm_compare_id(void)											
{											
  unsigned int id0,id1, id2, id3,id4;											
  unsigned char buffer[5];											
  unsigned int array[5];											
  SET_RESET_PIN(1);											
  MDELAY(10);											
  SET_RESET_PIN(0);											
  MDELAY(50);											
  SET_RESET_PIN(1);											
  MDELAY(120);											
											
  // Set Maximum return byte = 1											
  array[0] = 0x00053700;											
  dsi_set_cmdq(array, 1, 1);											
											
  read_reg_v2(0xA1, buffer, 5);											
  id0 = buffer[0];											
  id1 = buffer[1];											
  id2 = buffer[2];											
  id3 = buffer[3];											
  id4 = buffer[4];											
											
#if defined(BUILD_LK)											
  printf("%s, [otm1289a] Module ID = {%x, %x, %x, %x, %x} \n", __func__, id0, id1, id2,id3,id4);											
#else											
  printk("%s, [otm1289a] Module ID = {%x, %x, %x, %x,%x} \n", __func__,  id0,id1, id2, id3,id4);											
#endif											
	return (0x1289==((id2 << 8) | id3) )?1:0;											
}


LCM_DRIVER otm1289a_hd720_dsi_vdo_auo_lcm_drv = 
{
        .name		 = "otm1289a_hd720_dsi_vdo_auo",
	.set_util_funcs  = lcm_set_util_funcs,
	.get_params      = lcm_get_params,
	.init            = lcm_init,
	.suspend         = lcm_suspend,
	.resume          = lcm_resume,
	.compare_id      = lcm_compare_id,
	//.esd_check = lcm_esd_check,
	//.esd_recover = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
};

