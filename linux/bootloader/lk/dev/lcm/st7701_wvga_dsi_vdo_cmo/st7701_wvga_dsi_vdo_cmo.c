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
#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT 										(800)


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
	
	{0xFF, 5, {0x77,0x01, 0x00, 0x00, 0x11}},
	{0xD1, 2, {0x11, 0x11}},
	{0xFF, 5, {0x77,0x01, 0x00, 0x00, 0x10}},
	{0xC0, 2, {0x63, 0x00}},
	{0xC1, 2, {0x0D, 0x02}},
	{0xC2, 2, {0x31, 0x06}},
	{0xB0, 16,{0x00, 0x03, 0x89, 0x0F, 0x16, 0x0A, 0x06, 0x0B, 0x0A, 0x1B, 0x09, 0x13, 0x12, 0x0C, 0x10, 0x15}},
	{0xB1, 16,{0x00, 0x03, 0x87, 0x0E, 0x14, 0x09, 0x04, 0x09, 0x09, 0x1D, 0x09, 0x18, 0x14, 0x14, 0x1A, 0x15}},
	
	{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x11}},
	{0xB0, 1, {0x4D}},
	{0xB1, 1, {0x64}},//3e
	{0xB2, 1, {0x07}},
	{0xB3, 1, {0x80}},
	{0xB5, 1, {0x47}},
	{0xB7, 1, {0x8A}},
	{0xB8, 1, {0x21}},//21
	{0xC1, 1, {0x78}},
	{0xC2, 1, {0x78}},
	{0xD0, 1, {0x88}},
	
	{REGFLAG_DELAY, 100, {}},
	
	{0xE0, 3, {0x00, 0x00, 0x02}},
	{0xE1, 11,{0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40}},
	{0xE2, 13,{0x33, 0x33, 0x34, 0x34, 0x2C, 0x00, 0x2D, 0x00, 0x2B, 0x00, 0x2E, 0x00, 0x00}},
	{0xE3, 4, {0x00, 0x00, 0x33, 0x33}},
	{0xE4, 2, {0x44, 0x44}},
	{0xE5, 16,{0x04, 0x40, 0xA0, 0xA0, 0x06, 0x40, 0xA0, 0xA0, 0x08, 0x40, 0xA0, 0xA0, 0x0A, 0x40, 0xA0, 0xA0}},
	{0xE6, 4, {0x00, 0x00, 0x33, 0x33}},
	{0xE7, 2, {0x44, 0x44}},
	{0xE8, 16,{0x03, 0x40, 0xA0, 0xA0, 0x05, 0x40, 0xA0, 0xA0, 0x07, 0x40, 0xA0, 0xA0, 0x09, 0x40, 0xA0, 0xA0}},
	{0xEB, 7, {0x02, 0x00, 0x39, 0x39, 0x88, 0x33, 0x10}},
	{0xEC, 2, {0x02, 0x00}},
	{0xED, 16,{0xFF, 0x04, 0x56, 0x7F, 0x89, 0xF2, 0xFF, 0x3F, 0xF3, 0xFF, 0x2F, 0x98, 0xF7, 0x65, 0x40, 0xFF}},
	{0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x00}},
	{REGFLAG_DELAY, 20, {}},

	{0x35, 1, {0x00}},
	{0x11,1,{0x00}},  
	{REGFLAG_DELAY, 120, {}},
	{0x29,1, {0x00}},  
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
    params->dsi.LANE_NUM				= LCM_TWO_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

   // Highly depends on LCD driver capability.
	  params->dsi.packet_size=256;
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
   
	params->dsi.vertical_sync_active				= 8;
	params->dsi.vertical_backporch					= 18;  //8
	params->dsi.vertical_frontporch					= 16;  //8
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active			= 5;//50
	params->dsi.horizontal_backporch				= 65;//60
	params->dsi.horizontal_frontporch				= 67;//100
	params->dsi.horizontal_active_pixel			= FRAME_WIDTH;

	params->dsi.PLL_CLOCK=206;

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
LCM_DRIVER st7701_wvga_dsi_vdo_cmo_lcm_drv = 
{
    .name           = "st7701_wvga_dsi_vdo_cmo",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
   
};
