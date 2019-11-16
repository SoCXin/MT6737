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

#define LCM_ID_ILI9881C  0x9881
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
#define changcmd(a,b,c,d)((d<<24)|(c<<16)|(b<<8)|a)

void ILI9881_DCS_write_1A_1P(unsigned char cmd, unsigned char para)
{
    unsigned int data_array[16];

      data_array[0] =(0x00023902);
      data_array[1] =(0x00000000 | (para<<8) | (cmd));
      dsi_set_cmdq(data_array, 2, 1);

}

void ILI9881_DCS_write_1A_0P(unsigned char cmd)
{
    unsigned int data_array[16];

    data_array[0]=(0x00000500 | (cmd<<16)); 
    dsi_set_cmdq(data_array, 1, 1);
}


static void init_lcm_registers(void)  
{
	unsigned int data_array[16];

#if 0
	data_array[0] =(0x00043902);
	data_array[1] =(0x038198FF);
	dsi_set_cmdq(data_array, 2, 1);

	ILI9881_DCS_write_1A_1P(0x01,0x08); 
	ILI9881_DCS_write_1A_1P(0x02,0x00); 
	ILI9881_DCS_write_1A_1P(0x03,0x73); 
	ILI9881_DCS_write_1A_1P(0x04,0x73); 
	ILI9881_DCS_write_1A_1P(0x05,0x14); 
	ILI9881_DCS_write_1A_1P(0x06,0x06); 
	ILI9881_DCS_write_1A_1P(0x07,0x02); 
	ILI9881_DCS_write_1A_1P(0x08,0x05); 
	ILI9881_DCS_write_1A_1P(0x09,0x14); 
	ILI9881_DCS_write_1A_1P(0x0a,0x14); 
	ILI9881_DCS_write_1A_1P(0x0b,0x00); 
	ILI9881_DCS_write_1A_1P(0x0c,0x14); 
	ILI9881_DCS_write_1A_1P(0x0d,0x14); 
	ILI9881_DCS_write_1A_1P(0x0e,0x00); 
	ILI9881_DCS_write_1A_1P(0x0f,0x0C); 
	ILI9881_DCS_write_1A_1P(0x10,0x0C);  
	ILI9881_DCS_write_1A_1P(0x11,0x0C); 
	ILI9881_DCS_write_1A_1P(0x12,0x0C); 
	ILI9881_DCS_write_1A_1P(0x13,0x14); 
	ILI9881_DCS_write_1A_1P(0x14,0x0c); 
	ILI9881_DCS_write_1A_1P(0x15,0x00); 
	ILI9881_DCS_write_1A_1P(0x16,0x00);  
	ILI9881_DCS_write_1A_1P(0x17,0x00); 
	ILI9881_DCS_write_1A_1P(0x18,0x00); 
	ILI9881_DCS_write_1A_1P(0x19,0x00); 
	ILI9881_DCS_write_1A_1P(0x1a,0x00); 
	ILI9881_DCS_write_1A_1P(0x1b,0x00); 
	ILI9881_DCS_write_1A_1P(0x1c,0x00); 
	ILI9881_DCS_write_1A_1P(0x1d,0x00); 
	ILI9881_DCS_write_1A_1P(0x1e,0xc8); 
	ILI9881_DCS_write_1A_1P(0x1f,0x80); 
	ILI9881_DCS_write_1A_1P(0x20,0x02); 
	ILI9881_DCS_write_1A_1P(0x21,0x00); 
	ILI9881_DCS_write_1A_1P(0x22,0x02); 
	ILI9881_DCS_write_1A_1P(0x23,0x00); 
	ILI9881_DCS_write_1A_1P(0x24,0x00); 
	ILI9881_DCS_write_1A_1P(0x25,0x00); 
	ILI9881_DCS_write_1A_1P(0x26,0x00); 
	ILI9881_DCS_write_1A_1P(0x27,0x00); 
	ILI9881_DCS_write_1A_1P(0x28,0xfb); 
	ILI9881_DCS_write_1A_1P(0x29,0x43); 
	ILI9881_DCS_write_1A_1P(0x2a,0x00); 
	ILI9881_DCS_write_1A_1P(0x2b,0x00); 
	ILI9881_DCS_write_1A_1P(0x2c,0x07); 
	ILI9881_DCS_write_1A_1P(0x2d,0x07); 
	ILI9881_DCS_write_1A_1P(0x2e,0xff); 
	ILI9881_DCS_write_1A_1P(0x2f,0xff); 
	ILI9881_DCS_write_1A_1P(0x30,0x11); 
	ILI9881_DCS_write_1A_1P(0x31,0x00); 
	ILI9881_DCS_write_1A_1P(0x32,0x00); 
	ILI9881_DCS_write_1A_1P(0x33,0x00); 
	ILI9881_DCS_write_1A_1P(0x34,0x84); 
	ILI9881_DCS_write_1A_1P(0x35,0x80); 
	ILI9881_DCS_write_1A_1P(0x36,0x07); 
	ILI9881_DCS_write_1A_1P(0x37,0x00); 
	ILI9881_DCS_write_1A_1P(0x38,0x00); 
	ILI9881_DCS_write_1A_1P(0x39,0x00); 
	ILI9881_DCS_write_1A_1P(0x3a,0x00);  
	ILI9881_DCS_write_1A_1P(0x3b,0x00); 
	ILI9881_DCS_write_1A_1P(0x3c,0x00); 
	ILI9881_DCS_write_1A_1P(0x3d,0x00); 
	ILI9881_DCS_write_1A_1P(0x3e,0x00); 
	ILI9881_DCS_write_1A_1P(0x3f,0x00); 
	ILI9881_DCS_write_1A_1P(0x40,0x00); 
	ILI9881_DCS_write_1A_1P(0x41,0x88); 
	ILI9881_DCS_write_1A_1P(0x42,0x00); 
	ILI9881_DCS_write_1A_1P(0x43,0x80); 
	ILI9881_DCS_write_1A_1P(0x44,0x08); 
	ILI9881_DCS_write_1A_1P(0x50,0x01); 
	ILI9881_DCS_write_1A_1P(0x51,0x23); 
	ILI9881_DCS_write_1A_1P(0x52,0x45); 
	ILI9881_DCS_write_1A_1P(0x53,0x67); 
	ILI9881_DCS_write_1A_1P(0x54,0x89); 
	ILI9881_DCS_write_1A_1P(0x55,0xab); 
	ILI9881_DCS_write_1A_1P(0x56,0x01); 
	ILI9881_DCS_write_1A_1P(0x57,0x23); 
	ILI9881_DCS_write_1A_1P(0x58,0x45); 
	ILI9881_DCS_write_1A_1P(0x59,0x67); 
	ILI9881_DCS_write_1A_1P(0x5a,0x89); 
	ILI9881_DCS_write_1A_1P(0x5b,0xab); 
	ILI9881_DCS_write_1A_1P(0x5c,0xcd); 
	ILI9881_DCS_write_1A_1P(0x5d,0xef); 
	ILI9881_DCS_write_1A_1P(0x5e,0x10); 
	ILI9881_DCS_write_1A_1P(0x5f,0x02); 
	ILI9881_DCS_write_1A_1P(0x60,0x08); 
	ILI9881_DCS_write_1A_1P(0x61,0x09); 
	ILI9881_DCS_write_1A_1P(0x62,0x10); 
	ILI9881_DCS_write_1A_1P(0x63,0x12); 
	ILI9881_DCS_write_1A_1P(0x64,0x11); 
	ILI9881_DCS_write_1A_1P(0x65,0x13); 
	ILI9881_DCS_write_1A_1P(0x66,0x0c); 
	ILI9881_DCS_write_1A_1P(0x67,0x02); 
	ILI9881_DCS_write_1A_1P(0x68,0x02); 
	ILI9881_DCS_write_1A_1P(0x69,0x02); 
	ILI9881_DCS_write_1A_1P(0x6a,0x02); 
	ILI9881_DCS_write_1A_1P(0x6b,0x02); 
	ILI9881_DCS_write_1A_1P(0x6c,0x0e); 
	ILI9881_DCS_write_1A_1P(0x6d,0x0d); 
	ILI9881_DCS_write_1A_1P(0x6e,0x0f); 
	ILI9881_DCS_write_1A_1P(0x6f,0x02); 
	ILI9881_DCS_write_1A_1P(0x70,0x02); 
	ILI9881_DCS_write_1A_1P(0x71,0x06); 
	ILI9881_DCS_write_1A_1P(0x72,0x07); 
	ILI9881_DCS_write_1A_1P(0x73,0x02); 
	ILI9881_DCS_write_1A_1P(0x74,0x02); 
	ILI9881_DCS_write_1A_1P(0x75,0x02); 
	ILI9881_DCS_write_1A_1P(0x76,0x07); 
	ILI9881_DCS_write_1A_1P(0x77,0x06); 
	ILI9881_DCS_write_1A_1P(0x78,0x11); 
	ILI9881_DCS_write_1A_1P(0x79,0x13); 
	ILI9881_DCS_write_1A_1P(0x7a,0x10); 
	ILI9881_DCS_write_1A_1P(0x7b,0x12); 
	ILI9881_DCS_write_1A_1P(0x7c,0x0f); 
	ILI9881_DCS_write_1A_1P(0x7d,0x02); 
	ILI9881_DCS_write_1A_1P(0x7e,0x02); 
	ILI9881_DCS_write_1A_1P(0x7f,0x02); 
	ILI9881_DCS_write_1A_1P(0x80,0x02); 
	ILI9881_DCS_write_1A_1P(0x81,0x02); 
	ILI9881_DCS_write_1A_1P(0x82,0x0d); 
	ILI9881_DCS_write_1A_1P(0x83,0x0e); 
	ILI9881_DCS_write_1A_1P(0x84,0x0c); 
	ILI9881_DCS_write_1A_1P(0x85,0x02); 
	ILI9881_DCS_write_1A_1P(0x86,0x02); 
	ILI9881_DCS_write_1A_1P(0x87,0x09); 
	ILI9881_DCS_write_1A_1P(0x88,0x08); 
	ILI9881_DCS_write_1A_1P(0x89,0x02); 
	ILI9881_DCS_write_1A_1P(0x8A,0x02); 

	data_array[0] =(0x00043902);
	data_array[1] =(0x048198FF);
	dsi_set_cmdq(data_array, 2, 1);

	ILI9881_DCS_write_1A_1P(0x6C,0x15); 
	ILI9881_DCS_write_1A_1P(0x6E,0x2A);   
	ILI9881_DCS_write_1A_1P(0x6F,0x35); 

	ILI9881_DCS_write_1A_1P(0x3A,0x24); 
	ILI9881_DCS_write_1A_1P(0x8D,0x1A);              //VGL clamp -12V   1F=>1A
	ILI9881_DCS_write_1A_1P(0x87,0xBA);              //ESD  
	ILI9881_DCS_write_1A_1P(0x26,0x76);           
	ILI9881_DCS_write_1A_1P(0xB2,0xD1); 

	ILI9881_DCS_write_1A_1P(0x35,0x1F); 
	ILI9881_DCS_write_1A_1P(0xB5,0x07);  

	ILI9881_DCS_write_1A_1P(0x69,0x57); 
	ILI9881_DCS_write_1A_1P(0x41,0x88); 
	ILI9881_DCS_write_1A_1P(0x33,0x44);  	

	data_array[0] =(0x00043902);
	data_array[1] =(0x018198FF);
	dsi_set_cmdq(data_array, 2, 1);

	ILI9881_DCS_write_1A_1P(0x22,0x0A);     //38   
	ILI9881_DCS_write_1A_1P(0x31,0x0B); 
	ILI9881_DCS_write_1A_1P(0x53,0x4F); 		//VCOM1
	ILI9881_DCS_write_1A_1P(0x55,0x8F); 		//VCOM2      
	ILI9881_DCS_write_1A_1P(0x50,0xB6); 		//VREG1OUT=4.9V
	ILI9881_DCS_write_1A_1P(0x51,0xB6); 		//VREG2OUT=-4.9V
	ILI9881_DCS_write_1A_1P(0x60,0x09); 		//SDT

	ILI9881_DCS_write_1A_1P(0xA0,0x08); 		//VP255	Gamma P
	ILI9881_DCS_write_1A_1P(0xA1,0x16); 		//VP251
	ILI9881_DCS_write_1A_1P(0xA2,0x21); 		//VP247
	ILI9881_DCS_write_1A_1P(0xA3,0x11); 		//VP243
	ILI9881_DCS_write_1A_1P(0xA4,0x13);                //VP239 
	ILI9881_DCS_write_1A_1P(0xA5,0x25);                //VP231
	ILI9881_DCS_write_1A_1P(0xA6,0x19);                //VP219
	ILI9881_DCS_write_1A_1P(0xA7,0x1C);                //VP203
	ILI9881_DCS_write_1A_1P(0xA8,0x7D);                //VP175
	ILI9881_DCS_write_1A_1P(0xA9,0x1D);                //VP144
	ILI9881_DCS_write_1A_1P(0xAA,0x29);                //VP111
	ILI9881_DCS_write_1A_1P(0xAB,0x79);                //VP80
	ILI9881_DCS_write_1A_1P(0xAC,0x1C);                //VP52
	ILI9881_DCS_write_1A_1P(0xAD,0x1B);                //VP36
	ILI9881_DCS_write_1A_1P(0xAE,0x4E);                //VP24
	ILI9881_DCS_write_1A_1P(0xAF,0x25);                //VP16
	ILI9881_DCS_write_1A_1P(0xB0,0x2A);                //VP12
	ILI9881_DCS_write_1A_1P(0xB1,0x57);                //VP8
	ILI9881_DCS_write_1A_1P(0xB2,0x67);                //VP4
	ILI9881_DCS_write_1A_1P(0xB3,0x39);                //VP0

	ILI9881_DCS_write_1A_1P(0xC0,0x08); 		//VN255 GAMMA N
	ILI9881_DCS_write_1A_1P(0xC1,0x16);                //VN251     
	ILI9881_DCS_write_1A_1P(0xC2,0x21);                //VN247     
	ILI9881_DCS_write_1A_1P(0xC3,0x11);                //VN243     
	ILI9881_DCS_write_1A_1P(0xC4,0x13);                //VN239     
	ILI9881_DCS_write_1A_1P(0xC5,0x25);                //VN231     
	ILI9881_DCS_write_1A_1P(0xC6,0x19);                //VN219     
	ILI9881_DCS_write_1A_1P(0xC7,0x1C);                //VN203     
	ILI9881_DCS_write_1A_1P(0xC8,0x7D);                //VN175     
	ILI9881_DCS_write_1A_1P(0xC9,0x1D);                //VN144     
	ILI9881_DCS_write_1A_1P(0xCA,0x29);                //VN111     
	ILI9881_DCS_write_1A_1P(0xCB,0x79);                //VN80      
	ILI9881_DCS_write_1A_1P(0xCC,0x1C);                //VN52      
	ILI9881_DCS_write_1A_1P(0xCD,0x1B);                //VN36      
	ILI9881_DCS_write_1A_1P(0xCE,0x4E);                //VN24      
	ILI9881_DCS_write_1A_1P(0xCF,0x25);                //VN16      
	ILI9881_DCS_write_1A_1P(0xD0,0x2A);                //VN12      
	ILI9881_DCS_write_1A_1P(0xD1,0x57);                //VN8       
	ILI9881_DCS_write_1A_1P(0xD2,0x67);                //VN4       
	ILI9881_DCS_write_1A_1P(0xD3,0x39);                //VN0 

	data_array[0] =(0x00043902);
	data_array[1] =(0x008198FF);
	dsi_set_cmdq(data_array, 2, 1);

	ILI9881_DCS_write_1A_1P(0x36,0x03);		     
	ILI9881_DCS_write_1A_0P(0x11);
	MDELAY(120);                                                
	ILI9881_DCS_write_1A_0P(0x29);
	MDELAY(20);
#else
	data_array[0] =(0x00043902);
	data_array[1] =(0x038198FF);
	dsi_set_cmdq(data_array, 2, 1);

	ILI9881_DCS_write_1A_1P(0x01,0x00);
	ILI9881_DCS_write_1A_1P(0x02,0x00);
	ILI9881_DCS_write_1A_1P(0x03,0x53);
	ILI9881_DCS_write_1A_1P(0x04,0x13);
	ILI9881_DCS_write_1A_1P(0x05,0x13);
	ILI9881_DCS_write_1A_1P(0x06,0x06);
	ILI9881_DCS_write_1A_1P(0x07,0x00);
	ILI9881_DCS_write_1A_1P(0x08,0x04);
	ILI9881_DCS_write_1A_1P(0x09,0x00);
	ILI9881_DCS_write_1A_1P(0x0a,0x00);
	ILI9881_DCS_write_1A_1P(0x0b,0x00);
	ILI9881_DCS_write_1A_1P(0x0c,0x00);
	ILI9881_DCS_write_1A_1P(0x0d,0x00);
	ILI9881_DCS_write_1A_1P(0x0e,0x00);
	ILI9881_DCS_write_1A_1P(0x0f,0x00);
	ILI9881_DCS_write_1A_1P(0x10,0x00);
	ILI9881_DCS_write_1A_1P(0x11,0x00);
	ILI9881_DCS_write_1A_1P(0x12,0x00);
	ILI9881_DCS_write_1A_1P(0x13,0x00);
	ILI9881_DCS_write_1A_1P(0x14,0x00);
	ILI9881_DCS_write_1A_1P(0x15,0x00);
	ILI9881_DCS_write_1A_1P(0x16,0x00);
	ILI9881_DCS_write_1A_1P(0x17,0x00);
	ILI9881_DCS_write_1A_1P(0x18,0x00);
	ILI9881_DCS_write_1A_1P(0x19,0x00);
	ILI9881_DCS_write_1A_1P(0x1a,0x00);
	ILI9881_DCS_write_1A_1P(0x1b,0x00);
	ILI9881_DCS_write_1A_1P(0x1c,0x00);
	ILI9881_DCS_write_1A_1P(0x1d,0x00);
	ILI9881_DCS_write_1A_1P(0x1e,0xC0);
	ILI9881_DCS_write_1A_1P(0x1f,0x80);
	ILI9881_DCS_write_1A_1P(0x20,0x04);
	ILI9881_DCS_write_1A_1P(0x21,0x0B);
	ILI9881_DCS_write_1A_1P(0x22,0x00);
	ILI9881_DCS_write_1A_1P(0x23,0x00);
	ILI9881_DCS_write_1A_1P(0x24,0x00);
	ILI9881_DCS_write_1A_1P(0x25,0x00);
	ILI9881_DCS_write_1A_1P(0x26,0x00);
	ILI9881_DCS_write_1A_1P(0x27,0x00);
	ILI9881_DCS_write_1A_1P(0x28,0x55);
	ILI9881_DCS_write_1A_1P(0x29,0x03);
	ILI9881_DCS_write_1A_1P(0x2a,0x00);
	ILI9881_DCS_write_1A_1P(0x2b,0x00);
	ILI9881_DCS_write_1A_1P(0x2c,0x00);
	ILI9881_DCS_write_1A_1P(0x2d,0x00);
	ILI9881_DCS_write_1A_1P(0x2e,0x00);
	ILI9881_DCS_write_1A_1P(0x2f,0x00);
	ILI9881_DCS_write_1A_1P(0x30,0x00);
	ILI9881_DCS_write_1A_1P(0x31,0x00);
	ILI9881_DCS_write_1A_1P(0x32,0x00);
	ILI9881_DCS_write_1A_1P(0x33,0x00);
	ILI9881_DCS_write_1A_1P(0x34,0x04);
	ILI9881_DCS_write_1A_1P(0x35,0x05);
	ILI9881_DCS_write_1A_1P(0x36,0x05);
	ILI9881_DCS_write_1A_1P(0x37,0x00);
	ILI9881_DCS_write_1A_1P(0x38,0x3C);
	ILI9881_DCS_write_1A_1P(0x39,0x00);
	ILI9881_DCS_write_1A_1P(0x3a,0x40);
	ILI9881_DCS_write_1A_1P(0x3b,0x40);
	ILI9881_DCS_write_1A_1P(0x3c,0x00);
	ILI9881_DCS_write_1A_1P(0x3d,0x00);
	ILI9881_DCS_write_1A_1P(0x3e,0x00);
	ILI9881_DCS_write_1A_1P(0x3f,0x00);
	ILI9881_DCS_write_1A_1P(0x40,0x00);
	ILI9881_DCS_write_1A_1P(0x41,0x00);
	ILI9881_DCS_write_1A_1P(0x42,0x00);
	ILI9881_DCS_write_1A_1P(0x43,0x00);
	ILI9881_DCS_write_1A_1P(0x44,0x00);
	ILI9881_DCS_write_1A_1P(0x50,0x01);
	ILI9881_DCS_write_1A_1P(0x51,0x23);
	ILI9881_DCS_write_1A_1P(0x52,0x45);
	ILI9881_DCS_write_1A_1P(0x53,0x67);
	ILI9881_DCS_write_1A_1P(0x54,0x89);
	ILI9881_DCS_write_1A_1P(0x55,0xab);
	ILI9881_DCS_write_1A_1P(0x56,0x01);
	ILI9881_DCS_write_1A_1P(0x57,0x23);
	ILI9881_DCS_write_1A_1P(0x58,0x45);
	ILI9881_DCS_write_1A_1P(0x59,0x67);
	ILI9881_DCS_write_1A_1P(0x5a,0x89);
	ILI9881_DCS_write_1A_1P(0x5b,0xab);
	ILI9881_DCS_write_1A_1P(0x5c,0xcd);
	ILI9881_DCS_write_1A_1P(0x5d,0xef);
	ILI9881_DCS_write_1A_1P(0x5e,0x01);
	ILI9881_DCS_write_1A_1P(0x5f,0x14);
	ILI9881_DCS_write_1A_1P(0x60,0x15);
	ILI9881_DCS_write_1A_1P(0x61,0x0C);
	ILI9881_DCS_write_1A_1P(0x62,0x0D);
	ILI9881_DCS_write_1A_1P(0x63,0x0E);
	ILI9881_DCS_write_1A_1P(0x64,0x0F);
	ILI9881_DCS_write_1A_1P(0x65,0x10);
	ILI9881_DCS_write_1A_1P(0x66,0x11);
	ILI9881_DCS_write_1A_1P(0x67,0x08);
	ILI9881_DCS_write_1A_1P(0x68,0x02);
	ILI9881_DCS_write_1A_1P(0x69,0x0A);
	ILI9881_DCS_write_1A_1P(0x6a,0x02);
	ILI9881_DCS_write_1A_1P(0x6b,0x02);
	ILI9881_DCS_write_1A_1P(0x6c,0x02);
	ILI9881_DCS_write_1A_1P(0x6d,0x02);
	ILI9881_DCS_write_1A_1P(0x6e,0x02);
	ILI9881_DCS_write_1A_1P(0x6f,0x02);
	ILI9881_DCS_write_1A_1P(0x70,0x02);
	ILI9881_DCS_write_1A_1P(0x71,0x02);
	ILI9881_DCS_write_1A_1P(0x72,0x06);
	ILI9881_DCS_write_1A_1P(0x73,0x02);
	ILI9881_DCS_write_1A_1P(0x74,0x02);
	ILI9881_DCS_write_1A_1P(0x75,0x14);
	ILI9881_DCS_write_1A_1P(0x76,0x15);
	ILI9881_DCS_write_1A_1P(0x77,0x11);
	ILI9881_DCS_write_1A_1P(0x78,0x10);
	ILI9881_DCS_write_1A_1P(0x79,0x0F);
	ILI9881_DCS_write_1A_1P(0x7a,0x0E);
	ILI9881_DCS_write_1A_1P(0x7b,0x0D);
	ILI9881_DCS_write_1A_1P(0x7c,0x0C);
	ILI9881_DCS_write_1A_1P(0x7d,0x06);
	ILI9881_DCS_write_1A_1P(0x7e,0x02);
	ILI9881_DCS_write_1A_1P(0x7f,0x0A);
	ILI9881_DCS_write_1A_1P(0x80,0x02);
	ILI9881_DCS_write_1A_1P(0x81,0x02);
	ILI9881_DCS_write_1A_1P(0x82,0x02);
	ILI9881_DCS_write_1A_1P(0x83,0x02);
	ILI9881_DCS_write_1A_1P(0x84,0x02);
	ILI9881_DCS_write_1A_1P(0x85,0x02);
	ILI9881_DCS_write_1A_1P(0x86,0x02);
	ILI9881_DCS_write_1A_1P(0x87,0x02);
	ILI9881_DCS_write_1A_1P(0x88,0x08);
	ILI9881_DCS_write_1A_1P(0x89,0x02);
	ILI9881_DCS_write_1A_1P(0x8A,0x02);

	data_array[0] =(0x00043902);
	data_array[1] =(0x048198FF);
	dsi_set_cmdq(data_array, 2, 1);

	ILI9881_DCS_write_1A_1P(0x6C,0x15);
	ILI9881_DCS_write_1A_1P(0x6E,0x3B);
	ILI9881_DCS_write_1A_1P(0x6F,0x53);
	ILI9881_DCS_write_1A_1P(0x3A,0xA4);
	ILI9881_DCS_write_1A_1P(0x8D,0x15);
	ILI9881_DCS_write_1A_1P(0x87,0xBA);
	ILI9881_DCS_write_1A_1P(0x26,0x76);
	ILI9881_DCS_write_1A_1P(0xB2,0xD1);
	ILI9881_DCS_write_1A_1P(0x88,0x0B);
	//ILI9881_DCS_write_1A_1P(0x00,0x00); //00h=3LANE  80h=4LANE(default)

	data_array[0] =(0x00043902);
	data_array[1] =(0x018198FF);
	dsi_set_cmdq(data_array, 2, 1);

	ILI9881_DCS_write_1A_1P(0x22,0x0A);
	ILI9881_DCS_write_1A_1P(0x31,0x00);   //Inversion
	ILI9881_DCS_write_1A_1P(0x53,0x90);   //VCOM
	ILI9881_DCS_write_1A_1P(0x55,0x99);   //VCOM_R
	//ILI9881_DCS_write_1A_1P(0x22,0x09); // ·´É¨£¬ÏÔÊ¾Ðý×ª180
	ILI9881_DCS_write_1A_1P(0x50,0xa6);   //vgmp
	ILI9881_DCS_write_1A_1P(0x51,0xa6);   //vgmn
	ILI9881_DCS_write_1A_1P(0x60,0x14);
	ILI9881_DCS_write_1A_1P(0xA0,0x08);
	ILI9881_DCS_write_1A_1P(0xA1,0x23);
	ILI9881_DCS_write_1A_1P(0xA2,0x31);
	ILI9881_DCS_write_1A_1P(0xA3,0x14);
	ILI9881_DCS_write_1A_1P(0xA4,0x17);
	ILI9881_DCS_write_1A_1P(0xA5,0x2B);
	ILI9881_DCS_write_1A_1P(0xA6,0x1F);
	ILI9881_DCS_write_1A_1P(0xA7,0x20);
	ILI9881_DCS_write_1A_1P(0xA8,0x93);
	ILI9881_DCS_write_1A_1P(0xA9,0x1D);
	ILI9881_DCS_write_1A_1P(0xAA,0x28);
	ILI9881_DCS_write_1A_1P(0xAB,0x78);
	ILI9881_DCS_write_1A_1P(0xAC,0x1A);
	ILI9881_DCS_write_1A_1P(0xAD,0x19);
	ILI9881_DCS_write_1A_1P(0xAE,0x4C);
	ILI9881_DCS_write_1A_1P(0xAF,0x22);
	ILI9881_DCS_write_1A_1P(0xB0,0x27);
	ILI9881_DCS_write_1A_1P(0xB1,0x4A);
	ILI9881_DCS_write_1A_1P(0xB2,0x58);
	ILI9881_DCS_write_1A_1P(0xB3,0x2C);
	ILI9881_DCS_write_1A_1P(0xC0,0x08);
	ILI9881_DCS_write_1A_1P(0xC1,0x21);
	ILI9881_DCS_write_1A_1P(0xC2,0x32);
	ILI9881_DCS_write_1A_1P(0xC3,0x14);
	ILI9881_DCS_write_1A_1P(0xC4,0x17);
	ILI9881_DCS_write_1A_1P(0xC5,0x2B);
	ILI9881_DCS_write_1A_1P(0xC6,0x20);
	ILI9881_DCS_write_1A_1P(0xC7,0x20);
	ILI9881_DCS_write_1A_1P(0xC8,0x93);
	ILI9881_DCS_write_1A_1P(0xC9,0x1D);
	ILI9881_DCS_write_1A_1P(0xCA,0x29);
	ILI9881_DCS_write_1A_1P(0xCB,0x78);
	ILI9881_DCS_write_1A_1P(0xCC,0x19);
	ILI9881_DCS_write_1A_1P(0xCD,0x18);
	ILI9881_DCS_write_1A_1P(0xCE,0x4B);
	ILI9881_DCS_write_1A_1P(0xCF,0x20);
	ILI9881_DCS_write_1A_1P(0xD0,0x27);
	ILI9881_DCS_write_1A_1P(0xD1,0x4A);
	ILI9881_DCS_write_1A_1P(0xD2,0x57);
	ILI9881_DCS_write_1A_1P(0xD3,0x2C);
	
	data_array[0] =(0x00043902);
	data_array[1] =(0x008198FF);
	dsi_set_cmdq(data_array, 2, 1);
	ILI9881_DCS_write_1A_1P(0x35,0x00);

	ILI9881_DCS_write_1A_1P(0x36,0x03);		     
	ILI9881_DCS_write_1A_0P(0x11);
	MDELAY(120);                                                
	ILI9881_DCS_write_1A_0P(0x29);
	MDELAY(20);	
#endif	
};


//update initial param for IC ili9881 0.01
static struct LCM_setting_table ili9881_hd720_dsi_vdo_auo[] = {
	{0xFF,3,{0x98,0x81,0x03}},
	{0x01,1,{0x08}},
	{0x02,1,{0x00}},
	{0x03,1,{0x73}},
	{0x04,1,{0x73}},
	{0x05,1,{0x14}},
	{0x06,1,{0x06}},
	{0x07,1,{0x02}},
	{0x08,1,{0x05}},
	{0x09,1,{0x14}},
	{0x0a,1,{0x14}},
	{0x0b,1,{0x00}},
	{0x0c,1,{0x14}},
	{0x0d,1,{0x14}},
	{0x0e,1,{0x00}},
	{0x0f,1,{0x0C}},
	{0x10,1,{0x0C}}, 
	{0x11,1,{0x0C}},
	{0x12,1,{0x0C}},
	{0x13,1,{0x14}},
	{0x14,1,{0x0c}},
	{0x15,1,{0x00}},
	{0x16,1,{0x00}}, 
	{0x17,1,{0x00}},
	{0x18,1,{0x00}},
	{0x19,1,{0x00}},
	{0x1a,1,{0x00}},
	{0x1b,1,{0x00}},
	{0x1c,1,{0x00}},
	{0x1d,1,{0x00}},
	{0x1e,1,{0xc8}},
	{0x1f,1,{0x80}},
	{0x20,1,{0x02}},
	{0x21,1,{0x00}},
	{0x22,1,{0x02}},
	{0x23,1,{0x00}},
	{0x24,1,{0x00}},
	{0x25,1,{0x00}},
	{0x26,1,{0x00}},
	{0x27,1,{0x00}},
	{0x28,1,{0xfb}},
	{0x29,1,{0x43}},
	{0x2a,1,{0x00}},
	{0x2b,1,{0x00}},
	{0x2c,1,{0x07}},
	{0x2d,1,{0x07}},
	{0x2e,1,{0xff}},
	{0x2f,1,{0xff}},
	{0x30,1,{0x11}},
	{0x31,1,{0x00}},
	{0x32,1,{0x00}},
	{0x33,1,{0x00}},
	{0x34,1,{0x84}},
	{0x35,1,{0x80}},
	{0x36,1,{0x07}},
	{0x37,1,{0x00}},
	{0x38,1,{0x00}},
	{0x39,1,{0x00}},
	{0x3a,1,{0x00}}, 
	{0x3b,1,{0x00}},
	{0x3c,1,{0x00}},
	{0x3d,1,{0x00}},
	{0x3e,1,{0x00}},
	{0x3f,1,{0x00}},
	{0x40,1,{0x00}},
	{0x41,1,{0x88}},
	{0x42,1,{0x00}},
	{0x43,1,{0x80}},
	{0x44,1,{0x08}},
	{0x50,1,{0x01}},
	{0x51,1,{0x23}},
	{0x52,1,{0x45}},
	{0x53,1,{0x67}},
	{0x54,1,{0x89}},
	{0x55,1,{0xab}},
	{0x56,1,{0x01}},
	{0x57,1,{0x23}},
	{0x58,1,{0x45}},
	{0x59,1,{0x67}},
	{0x5a,1,{0x89}},
	{0x5b,1,{0xab}},
	{0x5c,1,{0xcd}},
	{0x5d,1,{0xef}},
	{0x5e,1,{0x10}},
	{0x5f,1,{0x02}},
	{0x60,1,{0x08}},
	{0x61,1,{0x09}},
	{0x62,1,{0x10}},
	{0x63,1,{0x12}},
	{0x64,1,{0x11}},
	{0x65,1,{0x13}},
	{0x66,1,{0x0c}},
	{0x67,1,{0x02}},
	{0x68,1,{0x02}},
	{0x69,1,{0x02}},
	{0x6a,1,{0x02}},
	{0x6b,1,{0x02}},
	{0x6c,1,{0x0e}},
	{0x6d,1,{0x0d}},
	{0x6e,1,{0x0f}},
	{0x6f,1,{0x02}},
	{0x70,1,{0x02}},
	{0x71,1,{0x06}},
	{0x72,1,{0x07}},
	{0x73,1,{0x02}},
	{0x74,1,{0x02}},
	{0x75,1,{0x02}},
	{0x76,1,{0x07}},
	{0x77,1,{0x06}},
	{0x78,1,{0x11}},
	{0x79,1,{0x13}},
	{0x7a,1,{0x10}},
	{0x7b,1,{0x12}},
	{0x7c,1,{0x0f}},
	{0x7d,1,{0x02}},
	{0x7e,1,{0x02}},
	{0x7f,1,{0x02}},
	{0x80,1,{0x02}},
	{0x81,1,{0x02}},
	{0x82,1,{0x0d}},
	{0x83,1,{0x0e}},
	{0x84,1,{0x0c}},
	{0x85,1,{0x02}},
	{0x86,1,{0x02}},
	{0x87,1,{0x09}},
	{0x88,1,{0x08}},
	{0x89,1,{0x02}},
	{0x8A,1,{0x02}},		
	{0xFF,3,{0x98,0x81,0x04}},
	{0x6C,1,{0x15}},
	{0x6E,1,{0x2A}},  
	{0x6F,1,{0x35}},

	{0x3A,1,{0x24}},
	{0x8D,1,{0x1A}},             //VGL clamp -12V   1F=>1A
	{0x87,1,{0xBA}},             //ESD  
	{0x26,1,{0x76}},          
	{0xB2,1,{0xD1}},

	{0x35,1,{0x1F}},
	{0xB5,1,{0x07}}, 

	{0x69,1,{0x57}},
	{0x41,1,{0x88}},
	{0x33,1,{0x44}}, 	

	{0xFF,3,{0x98,0x81,0x01}},
	{0x22,1,{0x0A}},    //38   
	{0x31,1,{0x0B}},
	{0x53,1,{0x4F}},		//VCOM1
	{0x55,1,{0x8F}},		//VCOM2      
	{0x50,1,{0xB6}},		//VREG1OUT=4.9V
	{0x51,1,{0xB6}},		//VREG2OUT=-4.9V
	{0x60,1,{0x09}},		//SDT

	{0xA0,1,{0x08}},		//VP255	Gamma P
	{0xA1,1,{0x16}},		//VP251
	{0xA2,1,{0x21}},		//VP247
	{0xA3,1,{0x11}},		//VP243
	{0xA4,1,{0x13}},               //VP239 
	{0xA5,1,{0x25}},               //VP231
	{0xA6,1,{0x19}},               //VP219
	{0xA7,1,{0x1C}},               //VP203
	{0xA8,1,{0x7D}},               //VP175
	{0xA9,1,{0x1D}},               //VP144
	{0xAA,1,{0x29}},               //VP111
	{0xAB,1,{0x79}},               //VP80
	{0xAC,1,{0x1C}},               //VP52
	{0xAD,1,{0x1B}},               //VP36
	{0xAE,1,{0x4E}},               //VP24
	{0xAF,1,{0x25}},               //VP16
	{0xB0,1,{0x2A}},               //VP12
	{0xB1,1,{0x57}},               //VP8
	{0xB2,1,{0x67}},               //VP4
	{0xB3,1,{0x39}},               //VP0

	{0xC0,1,{0x08}},		//VN255 GAMMA N
	{0xC1,1,{0x16}},               //VN251     
	{0xC2,1,{0x21}},               //VN247     
	{0xC3,1,{0x11}},               //VN243     
	{0xC4,1,{0x13}},               //VN239     
	{0xC5,1,{0x25}},               //VN231     
	{0xC6,1,{0x19}},               //VN219     
	{0xC7,1,{0x1C}},               //VN203     
	{0xC8,1,{0x7D}},               //VN175     
	{0xC9,1,{0x1D}},               //VN144     
	{0xCA,1,{0x29}},               //VN111     
	{0xCB,1,{0x79}},               //VN80      
	{0xCC,1,{0x1C}},               //VN52      
	{0xCD,1,{0x1B}},               //VN36      
	{0xCE,1,{0x4E}},               //VN24      
	{0xCF,1,{0x25}},               //VN16      
	{0xD0,1,{0x2A}},               //VN12      
	{0xD1,1,{0x57}},               //VN8       
	{0xD2,1,{0x67}},               //VN4       
	{0xD3,1,{0x39}},               //VN0 

	{0xFF,3,{0x98,0x81,0x00}},		
	{0x36, 1, {0x03}},       
	{REGFLAG_DELAY, 20, {}},   //200     
	{0x11, 1, {0x00}},       
	{REGFLAG_DELAY, 120, {}},   //150
	// Display ON            
	{0x29, 1, {0x00}},       
	{REGFLAG_DELAY, 20, {}},   //200

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
   
	params->dsi.vertical_sync_active 			   = 8;
	params->dsi.vertical_backporch			   = 16;
	params->dsi.vertical_frontporch			   = 16;
	params->dsi.vertical_active_line 			   = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active		   = 10;//4/
	params->dsi.horizontal_backporch 			   = 80;
	params->dsi.horizontal_frontporch			   = 80;
	params->dsi.horizontal_active_pixel		   = FRAME_WIDTH;

	//improve clk quality
	params->dsi.PLL_CLOCK = 220; //this value 
     //  params->dsi.HS_TRAIL = 120;

    //params->dsi.compatibility_for_nvk = 1;
    params->dsi.ssc_disable = 1;

}

static void lcm_init(void)
{
	LCD_DEBUG("uboot:tm_ili9881_lcm_init\n");

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50);
	SET_RESET_PIN(1);
	MDELAY(120);

	init_lcm_registers();
	
	//push_table(ili9881_hd720_dsi_vdo_auo, sizeof(ili9881_hd720_dsi_vdo_auo) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];

	LCD_DEBUG("uboot:tm_ili9881_lcm_suspend\n");	
	data_array[0]=0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	SET_RESET_PIN(1);
	MDELAY(20);  
	SET_RESET_PIN(0);
	MDELAY(50);	   

	//push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}
static void lcm_resume(void)
{
	LCD_DEBUG("uboot:tm_ili9881_lcm_resume\n");

	lcm_init();
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
	unsigned char buffer[2];
	unsigned int array[16];  

	return 1;
	
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	
	SET_RESET_PIN(1);
	MDELAY(20); 

	array[0] = 0x00023700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);
	
	read_reg_v2(0xF4, buffer, 2);
	id = buffer[0]; //we only need ID
#ifdef BUILD_LK
		printf("%s, LK ILI9881C debug: ILI9881C id = 0x%08x\n", __func__, id);
#else
		printk("%s, kernel ILI9881C horse debug: ILI9881C id = 0x%08x\n", __func__, id);
#endif

    if(id == LCM_ID_ILI9881C)
    	return 1;
    else
        return 0;

}
LCM_DRIVER ili9881_hd720_dsi_vdo_auo_lcm_drv =
{
    .name           = "ili9881_hd720_dsi_vdo_auo",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,/*tianma init fun.*/
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
   
};


