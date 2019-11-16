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
#define FRAME_WIDTH  										(1080)
#define FRAME_HEIGHT 										(1920)


#define REGFLAG_DELAY             							0xFFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER


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

#define LCM_RM67200_ID (0x6720)


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
//update initial param for IC rm67200 H546DAN09
static struct LCM_setting_table rm67200_fhd_dsi_video_wcl[] = {
		{0xFE, 1,{0x21}},
		{0x04, 1,{0x00}},
		{0x00, 1,{0x64}},
		{0x2A, 1,{0x00}},
		{0x26, 1,{0x64}},
		{0x54, 1,{0x00}},
		{0x50, 1,{0x64}},
		{0x7B, 1,{0x00}},
		{0x77, 1,{0x64}},
		{0xA2, 1,{0x00}},
		{0x9D, 1,{0x64}},
		{0xC9, 1,{0x00}},
		{0xC5, 1,{0x64}},
		{0x01, 1,{0x71}},
		{0x27, 1,{0x71}},
		{0x51, 1,{0x71}},
		{0x78, 1,{0x71}},
		{0x9E, 1,{0x71}},
		{0xC6, 1,{0x71}},
		{0x02, 1,{0x89}},
		{0x28, 1,{0x89}},
		{0x52, 1,{0x89}},
		{0x79, 1,{0x89}},
		{0x9F, 1,{0x89}},
		{0xC7, 1,{0x89}},
		{0x03, 1,{0x9E}},
		{0x29, 1,{0x9E}},
		{0x53, 1,{0x9E}},
		{0x7A, 1,{0x9E}},
		{0xA0, 1,{0x9E}},
		{0xC8, 1,{0x9E}},
		{0x09, 1,{0x00}},
		{0x05, 1,{0xB0}},
		{0x31, 1,{0x00}},
		{0x2B, 1,{0xB0}},
		{0x5A, 1,{0x00}},
		{0x55, 1,{0xB0}},
		{0x80, 1,{0x00}},
		{0x7C, 1,{0xB0}},
		{0xA7, 1,{0x00}},
		{0xA3, 1,{0xB0}},
		{0xCE, 1,{0x00}},
		{0xCA, 1,{0xB0}},
		{0x06, 1,{0xC0}},
		{0x2D, 1,{0xC0}},
		{0x56, 1,{0xC0}},
		{0x7D, 1,{0xC0}},
		{0xA4, 1,{0xC0}},
		{0xCB, 1,{0xC0}},
		{0x07, 1,{0xCF}},
		{0x2F, 1,{0xCF}},
		{0x58, 1,{0xCF}},
		{0x7E, 1,{0xCF}},
		{0xA5, 1,{0xCF}},
		{0xCC, 1,{0xCF}},
		{0x08, 1,{0xDD}},
		{0x30, 1,{0xDD}},
		{0x59, 1,{0xDD}},
		{0x7F, 1,{0xDD}},
		{0xA6, 1,{0xDD}},
		{0xCD, 1,{0xDD}},
		{0x0E, 1,{0x15}},
		{0x0A, 1,{0xE9}},
		{0x36, 1,{0x15}},
		{0x32, 1,{0xE9}},
		{0x5F, 1,{0x15}},
		{0x5B, 1,{0xE9}},
		{0x85, 1,{0x15}},
		{0x81, 1,{0xE9}},
		{0xAD, 1,{0x15}},
		{0xA9, 1,{0xE9}},
		{0xD3, 1,{0x15}},
		{0xCF, 1,{0xE9}},
		{0x0B, 1,{0x14}},
		{0x33, 1,{0x14}},
		{0x5C, 1,{0x14}},
		{0x82, 1,{0x14}},
		{0xAA, 1,{0x14}},
		{0xD0, 1,{0x14}},
		{0x0C, 1,{0x36}},
		{0x34, 1,{0x36}},
		{0x5D, 1,{0x36}},
		{0x83, 1,{0x36}},
		{0xAB, 1,{0x36}},
		{0xD1, 1,{0x36}},
		{0x0D, 1,{0x6B}},
		{0x35, 1,{0x6B}},
		{0x5E, 1,{0x6B}},
		{0x84, 1,{0x6B}},
		{0xAC, 1,{0x6B}},
		{0xD2, 1,{0x6B}},
		{0x13, 1,{0x5A}},
		{0x0F, 1,{0x94}},
		{0x3B, 1,{0x5A}},
		{0x37, 1,{0x94}},
		{0x64, 1,{0x5A}},
		{0x60, 1,{0x94}},
		{0x8A, 1,{0x5A}},
		{0x86, 1,{0x94}},
		{0xB2, 1,{0x5A}},
		{0xAE, 1,{0x94}},
		{0xD8, 1,{0x5A}},
		{0xD4, 1,{0x94}},
		{0x10, 1,{0xD1}},
		{0x38, 1,{0xD1}},
		{0x61, 1,{0xD1}},
		{0x87, 1,{0xD1}},
		{0xAF, 1,{0xD1}},
		{0xD5, 1,{0xD1}},
		{0x11, 1,{0x04}},
		{0x39, 1,{0x04}},
		{0x62, 1,{0x04}},
		{0x88, 1,{0x04}},
		{0xB0, 1,{0x04}},
		{0xD6, 1,{0x04}},
		{0x12, 1,{0x05}},
		{0x3A, 1,{0x05}},
		{0x63, 1,{0x05}},
		{0x89, 1,{0x05}},
		{0xB1, 1,{0x05}},
		{0xD7, 1,{0x05}},
		{0x18, 1,{0xAA}},
		{0x14, 1,{0x36}},
		{0x42, 1,{0xAA}},
		{0x3D, 1,{0x36}},
		{0x69, 1,{0xAA}},
		{0x65, 1,{0x36}},
		{0x8F, 1,{0xAA}},
		{0x8B, 1,{0x36}},
		{0xB7, 1,{0xAA}},
		{0xB3, 1,{0x36}},
		{0xDD, 1,{0xAA}},
		{0xD9, 1,{0x36}},
		{0x15, 1,{0x74}},
		{0x3F, 1,{0x74}},
		{0x66, 1,{0x74}},
		{0x8C, 1,{0x74}},
		{0xB4, 1,{0x74}},
		{0xDA, 1,{0x74}},
		{0x16, 1,{0x9F}},
		{0x40, 1,{0x9F}},
		{0x67, 1,{0x9F}},
		{0x8D, 1,{0x9F}},
		{0xB5, 1,{0x9F}},
		{0xDB, 1,{0x9F}},
		{0x17, 1,{0xDC}},
		{0x41, 1,{0xDC}},
		{0x68, 1,{0xDC}},
		{0x8E, 1,{0xDC}},
		{0xB6, 1,{0xDC}},
		{0xDC, 1,{0xDC}},
		{0x1D, 1,{0xFF}},
		{0x19, 1,{0x03}},
		{0x47, 1,{0xFF}},
		{0x43, 1,{0x03}},
		{0x6E, 1,{0xFF}},
		{0x6A, 1,{0x03}},
		{0x94, 1,{0xFF}},
		{0x90, 1,{0x03}},
		{0xBC, 1,{0xFF}},
		{0xB8, 1,{0x03}},
		{0xE2, 1,{0xFF}},
		{0xDE, 1,{0x03}},
		{0x1A, 1,{0x35}},
		{0x44, 1,{0x35}},
		{0x6B, 1,{0x35}},
		{0x91, 1,{0x35}},
		{0xB9, 1,{0x35}},
		{0xDF, 1,{0x35}},
		{0x1B, 1,{0x45}},
		{0x45, 1,{0x45}},
		{0x6C, 1,{0x45}},
		{0x92, 1,{0x45}},
		{0xBA, 1,{0x45}},
		{0xE0, 1,{0x45}},
		{0x1C, 1,{0x55}},
		{0x46, 1,{0x55}},
		{0x6D, 1,{0x55}},
		{0x93, 1,{0x55}},
		{0xBB, 1,{0x55}},
		{0xE1, 1,{0x55}},
		{0x22, 1,{0xFF}},
		{0x1E, 1,{0x68}},
		{0x4C, 1,{0xFF}},
		{0x48, 1,{0x68}},
		{0x73, 1,{0xFF}},
		{0x6F, 1,{0x68}},
		{0x99, 1,{0xFF}},
		{0x95, 1,{0x68}},
		{0xC1, 1,{0xFF}},
		{0xBD, 1,{0x68}},
		{0xE7, 1,{0xFF}},
		{0xE3, 1,{0x68}},
		{0x1F, 1,{0x7E}},
		{0x49, 1,{0x7E}},
		{0x70, 1,{0x7E}},
		{0x96, 1,{0x7E}},
		{0xBE, 1,{0x7E}},
		{0xE4, 1,{0x7E}},
		{0x20, 1,{0x97}},
		{0x4A, 1,{0x97}},
		{0x71, 1,{0x97}},
		{0x97, 1,{0x97}},
		{0xBF, 1,{0x97}},
		{0xE5, 1,{0x97}},
		{0x21, 1,{0xB5}},
		{0x4B, 1,{0xB5}},
		{0x72, 1,{0xB5}},
		{0x98, 1,{0xB5}},
		{0xC0, 1,{0xB5}},
		{0xE6, 1,{0xB5}},
		{0x25, 1,{0xF0}},
		{0x23, 1,{0xE8}},
		{0x4F, 1,{0xF0}},
		{0x4D, 1,{0xE8}},
		{0x76, 1,{0xF0}},
		{0x74, 1,{0xE8}},
		{0x9C, 1,{0xF0}},
		{0x9A, 1,{0xE8}},
		{0xC4, 1,{0xF0}},
		{0xC2, 1,{0xE8}},
		{0xEA, 1,{0xF0}},
		{0xE8, 1,{0xE8}},
		{0x24, 1,{0xFF}},
		{0x4E, 1,{0xFF}},
		{0x75, 1,{0xFF}},
		{0x9B, 1,{0xFF}},
		{0xC3, 1,{0xFF}},
		{0xE9, 1,{0xFF}},
		{0xFE, 1,{0x3D}},
		{0x00, 1,{0x04}},
		{0xFE, 1,{0x23}},
		{0x08, 1,{0x82}},
		{0x0A, 1,{0x00}},
		{0x0B, 1,{0x00}},
		{0x0C, 1,{0x01}},
		{0x16, 1,{0x00}},
		{0x18, 1,{0x02}},
		{0x1B, 1,{0x04}},
		{0x19, 1,{0x04}},
		{0x1C, 1,{0x81}},
		{0x1F, 1,{0x00}},
		{0x20, 1,{0x03}},
		{0x23, 1,{0x04}},
		{0x21, 1,{0x01}},
		{0x54, 1,{0x63}},
		{0x55, 1,{0x54}},
		{0x6E, 1,{0x45}},
		{0x6D, 1,{0x36}},
		{0xFE, 1,{0x3D}},
		{0x55, 1,{0x78}},
		{0xFE, 1,{0x20}},
		{0x26, 1,{0x30}},
		{0xFE, 1,{0x3D}},
		{0x20, 1,{0x71}},
		{0x50, 1,{0x8F}},
		{0x51, 1,{0x8F}},
		{0xFE, 1,{0x00}},
		{0x35, 1,{0x00}},
		{0x11, 0,{0x00}},
		{REGFLAG_DELAY, 120, {}},
		{0x29, 0,{0x00}},
		{REGFLAG_DELAY, 30, {}},

	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.


	// Setting ending by predefined flag
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
    {0x28, 0, {0x00}},
    {REGFLAG_DELAY, 20, {}},

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
    params->type   = LCM_TYPE_DSI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

		params->dsi.mode   = BURST_VDO_MODE;	//BURST_VDO_MODE;	//SYNC_PULSE_VDO_MODE;

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
   
		params->dsi.vertical_sync_active	= 2;
		params->dsi.vertical_backporch		= 6;
		params->dsi.vertical_frontporch		= 14;
		params->dsi.vertical_active_line	= FRAME_HEIGHT;

		params->dsi.horizontal_sync_active	= 20;
		params->dsi.horizontal_backporch	= 120;
		params->dsi.horizontal_frontporch	= 120;
		params->dsi.horizontal_active_pixel	= FRAME_WIDTH;

		// Bit rate calculation
		params->dsi.PLL_CLOCK=470;

}

static void lcm_init(void)
{
		SET_RESET_PIN(1);    
		MDELAY(10); 
		SET_RESET_PIN(0);
		MDELAY(10); 
		SET_RESET_PIN(1);
		MDELAY(120); 

    push_table(rm67200_fhd_dsi_video_wcl, sizeof(rm67200_fhd_dsi_video_wcl) / sizeof(struct LCM_setting_table), 1);
    //LCD_DEBUG("uboot:tm_rm67200_lcm_init\n");
}


static void lcm_suspend(void)
{
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
    //LCD_DEBUG("uboot:tm_rm67200_lcm_suspend\n");

}
static void lcm_resume(void)
{
		lcm_init();
    //LCD_DEBUG("uboot:tm_rm67200_lcm_resume\n");

}

static unsigned int lcm_compare_id(void)
{
#if 1
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
			printf("[U-BOOT] rm67200a %s id1 = 0x%04x, id2 = 0x%04x\n", __func__, id1,id2);
		#else
			printk("[KERNEL] rm67200a %s id1 = 0x%04x, id2 = 0x%04x\n", __func__, id1,id2);
		#endif
		
		return (LCM_RM67200_ID == id1)?1:0;
#else
	return 1;
#endif
}
LCM_DRIVER rm67200_fhd_dsi_vdo_wcl_lcm_drv = 
{
    .name           = "rm67200_fhd_dsi_vdo_wcl",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
   
};
