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
#define FRAME_HEIGHT 										(1440)


#define REGFLAG_DELAY             							0xFFFC
#define REGFLAG_END_OF_TABLE      							0xFFFD   // END OF REGISTERS MARKER


#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

static unsigned int lcm_esd_test = FALSE;      ///only for ESD test

#define LCM_DSI_CMD_MODE									0

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

struct LCM_setting_table {
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[128];
};
//update initial param for IC rm68200 0.01
static struct LCM_setting_table rm68200_hd720_dsi_video_hf[] = {
	
	{0xB9, 3, {0xF1,0x12,0x83}},
	{0xBA, 27,{0x33,0x81,0x05,0xF9,0x0e,
	           0x0e,0x02,0x00,0x00,0x00,
	           0x00,0x00,0x00,0x00,0x44,
	           0x25,0x00,0x91,0x0a,0x00,
	           0x00,0x02,0x4F,0x11,0x00,
	           0x00,0x37}}, 
	{0xB8, 4, {0x25,0x22,0x20,0x03}},
	{0xB3, 10,{0x10,0x10,0x05,0x05,0x03,
	           0xFF,0x00,0x00,0x00,0x00}},	
	{0xC0, 9, {0x70,0x73,0x50,0x50,0x00,
	           0x00,0x08,0x70,0x00}},
	{0xBC, 1, {0x4E}},	           
	{0xCC, 1, {0x0B}},	
	{0xB4, 1, {0x80}},
	{0xB2, 3, {0xF0,0x12,0x30}},
	{0xE3, 14,{0x07,0x07,0x0B,0x0B,0x03,
	           0x0B,0x00,0x00,0x00,0x00,
	           0xFF,0x00,0xC0,0x10}},
	{0xC1, 12,{0x54,0xC0,0x1E,0x1E,0x77,
	           0xE1,0xFF,0xFF,0xCC,0xCC,
	           0x77,0x77}},
 	{0xB5, 2, {0x07,0x07}},
 	{0xB6, 2, {0x30,0x30}},  
 	{0xBF, 3, {0x02,0x11,0x00}}, 	
 	{0xE9, 63,{0x04,0x00,0x10,0x05,0xB4,
 	           0x0A,0xA0,0x12,0x31,0x23,
 	           0x37,0x13,0x40,0xA0,0x27,
 	           0x38,0x0C,0x00,0x03,0x00,
 	           0x00,0x00,0x0C,0x00,0x03,
             0x00,0x00,0x00,0x75,0x75,
             0x31,0x88,0x88,0x88,0x88,
             0x88,0x88,0x13,0x88,0x64,
             0x64,0x20,0x88,0x88,0x88,
             0x88,0x88,0x88,0x02,0x88,
             0x00,0x00,0x00,0x00,0x00,
             0x00,0x00,0x00,0x00,0x00,
             0x00,0x00,0x00}},
 	{0xEA, 61,{0x02,0x21,0x00,0x00,0x00,
 	           0x00,0x00,0x00,0x00,0x00,
 	           0x00,0x00,0x02,0x46,0x02,
 	           0x88,0x88,0x88,0x88,0x88,
 	           0x88,0x64,0x88,0x13,0x57,
 	           0x13,0x88,0x88,0x88,0x88,
 	           0x88,0x88,0x75,0x88,0x23,
 	           0x10,0x00,0x00,0x30,0x00,
 	           0x00,0x00,0x00,0x00,0x00,
 	           0x00,0x00,0x00,0x00,0x00,
 	           0x00,0x00,0x00,0x00,0x03,
 	           0x0A,0xA0,0x00,0x00,0x00,
 	           0x00}},
 	{0xE0, 34,{0x00,0x08,0x13,0x23,0x34,
 	           0x3C,0x46,0x34,0x07,0x0E,
 	           0x0D,0x11,0x12,0x10,0x11,
 	           0x10,0x18,0x00,0x08,0x13,
 	           0x23,0x34,0x3C,0x46,0x34,
 	           0x07,0x0E,0x0D,0x11,0x12,
 	           0x10,0x11,0x10,0x18}},
 	                         
	{0x11, 1, {0x00}},  
	{REGFLAG_DELAY, 150, {}},
	{0x29, 1, {0x00}},  
	{REGFLAG_DELAY, 50, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	
};
							
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 0, {0x00}},
    {REGFLAG_DELAY, 100, {}},
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
    params->type   = LCM_TYPE_DSI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

	  params->dsi.mode   = SYNC_PULSE_VDO_MODE;	//BURST_VDO_MODE;	

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
	params->dsi.vertical_backporch					= 18;  //8
	params->dsi.vertical_frontporch					= 18;  //8
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active			= 4;//50
	params->dsi.horizontal_backporch				= 44;//60
	params->dsi.horizontal_frontporch				= 44;//100
	params->dsi.horizontal_active_pixel			= FRAME_WIDTH;

	params->dsi.PLL_CLOCK=246;

}

static void lcm_init(void)
{
	SET_RESET_PIN(1);    
	MDELAY(10); 
	SET_RESET_PIN(0);
	MDELAY(10); 
	SET_RESET_PIN(1);
	MDELAY(120); 

    push_table(rm68200_hd720_dsi_video_hf, sizeof(rm68200_hd720_dsi_video_hf) / sizeof(struct LCM_setting_table), 1);
    //LCD_DEBUG("uboot:tm_rm68200_lcm_init\n");
}


static void lcm_suspend(void)
{
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
    //LCD_DEBUG("uboot:tm_rm68200_lcm_suspend\n");

}
static void lcm_resume(void)
{
	lcm_init();
    //LCD_DEBUG("uboot:tm_rm68200_lcm_resume\n");

}

static unsigned int lcm_compare_id(void)
{
#if 0
	//unsigned int id=0;
	unsigned char buffer[5];
	unsigned int array[4]; 
	unsigned char id_high=0;
	unsigned char id_low=0;
	unsigned int id1=0;
	unsigned int id2=0;
 

        SET_RESET_PIN(1);
	MDELAY(10); 
	SET_RESET_PIN(0);
	MDELAY(10); 
	SET_RESET_PIN(1);
	MDELAY(10);

	array[0]=0x01FE1500;
	dsi_set_cmdq(array,1, 1);
		
	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

       read_reg_v2(0xde, buffer, 1);//68
       id_high = buffer[0];
       read_reg_v2(0xdf, buffer, 1);//20
       id_low = buffer[0];
       id1 = (id_high<<8) | id_low;
	   
       #if defined(BUILD_LK)
           printf("[U-BOOT] rm68200a %s id1 = 0x%04x, id2 = 0x%04x\n", __func__, id1,id2);
       #else
	   printk("[KERNEL] rm68200a %s id1 = 0x%04x, id2 = 0x%04x\n", __func__, id1,id2);
       #endif
       return (LCM_RM68200_ID == id1)?1:0;

#else
	return 1;
#endif
}
LCM_DRIVER st7703_hd720_dsi_vdo_ivo_lcm_drv = 
{
    .name           = "st7703_hd720_dsi_vdo_ivo",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
   
};
