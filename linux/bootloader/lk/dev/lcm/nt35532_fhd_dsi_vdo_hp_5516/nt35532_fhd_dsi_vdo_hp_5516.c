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
#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)

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

void NT35532_DCS_write_1A_1P(unsigned char cmd, unsigned char para)
{
  unsigned int data_array[16];
  //unsigned char buffer;
  data_array[0] =(0x00022902);
  data_array[1] =(0x00000000| (para<<8)|(cmd));
  dsi_set_cmdq(data_array, 2, 1);
  //MDELAY(1);
}

#define NT35532_DCS_write_1A_0P(cmd)							data_array[0]=(0x00000500 | (cmd<<16)); \
                                                 dsi_set_cmdq(data_array, 1, 1);
#define LCM_ID_NT35532 (0x32)

static void init_lcm_registers(void) //nt35532_sharp55_jsj_fhd
{
	unsigned int data_array[16]; 

		NT35532_DCS_write_1A_1P(0xFF,  0x01);
		NT35532_DCS_write_1A_1P(0x6e,  0x80);
		NT35532_DCS_write_1A_1P(0x68,  0x13);
		NT35532_DCS_write_1A_1P(0xFB,  0x01);
		NT35532_DCS_write_1A_1P(0xFF,  0x02);
		NT35532_DCS_write_1A_1P(0xFB,  0x01);

		NT35532_DCS_write_1A_1P(0xFF,  0x05);
		NT35532_DCS_write_1A_1P(0xFB,  0x01);
		NT35532_DCS_write_1A_1P(0xD7,  0x31);
		NT35532_DCS_write_1A_1P(0xD8,  0x7E);



		NT35532_DCS_write_1A_1P(0xFF,  0x00);
		NT35532_DCS_write_1A_1P(0xFB,  0x01);
		NT35532_DCS_write_1A_1P(0xBA,  0x03);
		NT35532_DCS_write_1A_1P(0x36,  0x00);
		NT35532_DCS_write_1A_1P(0xB0,  0x00);
		NT35532_DCS_write_1A_1P(0xD3,  0x08);//porch
		NT35532_DCS_write_1A_1P(0xD4,  0x0E);//porch
		NT35532_DCS_write_1A_1P(0xD5,  0x0F);
		NT35532_DCS_write_1A_1P(0xD6,  0x48);
		NT35532_DCS_write_1A_1P(0xD7,  0x00);
		NT35532_DCS_write_1A_1P(0xD9,  0x00);
		NT35532_DCS_write_1A_1P(0xFB,  0x01);

		NT35532_DCS_write_1A_1P(0xFF,  0xEE);
		NT35532_DCS_write_1A_1P(0x02,  0x00);
		NT35532_DCS_write_1A_1P(0x40,  0x00);
		NT35532_DCS_write_1A_1P(0x02,  0x00);
		NT35532_DCS_write_1A_1P(0x41,  0x00);
		NT35532_DCS_write_1A_1P(0x02,  0x00);
		NT35532_DCS_write_1A_1P(0x42,  0x00);
		NT35532_DCS_write_1A_1P(0xFB,  0x01);

		NT35532_DCS_write_1A_1P(0xFF,  0x01);
		NT35532_DCS_write_1A_1P(0xFB,  0x01);
		//{0x01,  0x55);
		NT35532_DCS_write_1A_1P(0x01,  0x00);
		NT35532_DCS_write_1A_1P(0x04,  0x0C);
		NT35532_DCS_write_1A_1P(0x05,  0x3A);
		NT35532_DCS_write_1A_1P(0x06,  0x50);
		NT35532_DCS_write_1A_1P(0x07,  0xD0);
		NT35532_DCS_write_1A_1P(0x0A,  0x0F);
		NT35532_DCS_write_1A_1P(0x0C,  0x06);
		NT35532_DCS_write_1A_1P(0x0D,  0x7F);
		NT35532_DCS_write_1A_1P(0x0E,  0x7F);
		NT35532_DCS_write_1A_1P(0x0F,  0x70);
		NT35532_DCS_write_1A_1P(0x10,  0x63);
		NT35532_DCS_write_1A_1P(0x11,  0x3C);
		NT35532_DCS_write_1A_1P(0x12,  0x5C);
		NT35532_DCS_write_1A_1P(0x13,  0x4D);    //* Flicker adjustment value	0x4B	 0x4C 0x4D 0x4e
		NT35532_DCS_write_1A_1P(0x14,  0x4D);    //* Flicker adjustment value	0x4B  0x4C 0x4D 0x4e
		NT35532_DCS_write_1A_1P(0x15,  0x60);
		NT35532_DCS_write_1A_1P(0x16,  0x15);
		NT35532_DCS_write_1A_1P(0x17,  0x15);
		//{0x23,  0x20);  //Refer to "10-3. DDB CTRL"
		//{0x24,  0x20);  //Refer to "10-3. DDB CTRL"
		//{0x25,  0x20);  //Refer to "10-3. DDB CTRL"
		//{0x26,  0x20);  //Refer to "10-3. DDB CTRL"
		//{0x27,  0x20);  //Refer to "10-3. DDB CTRL"
		//{0x28,  0x20);  //Refer to "10-3. DDB CTRL"
		//{0x44,  0x20);  //Refer to "10-4. WID CTRL"
		//{0x45,  0x20);  //Refer to "10-4. WID CTRL"
		//{0x46,  0x20);  //Refer to "10-4. WID CTRL"
		NT35532_DCS_write_1A_1P(0x5B,  0xCA);
		NT35532_DCS_write_1A_1P(0x5C,  0x00);
		NT35532_DCS_write_1A_1P(0x5D,  0x00);
		NT35532_DCS_write_1A_1P(0x5E,  0x26);    // value B = value A / 2 		0x25  0x26 0x26
		NT35532_DCS_write_1A_1P(0x5F,  0x1B);
		NT35532_DCS_write_1A_1P(0x60,  0xD5);
		NT35532_DCS_write_1A_1P(0x61,  0xF0);//Sunny 0Xf7
		NT35532_DCS_write_1A_1P(0x6C,  0xAB);
		NT35532_DCS_write_1A_1P(0x6D,  0x44);

		NT35532_DCS_write_1A_1P(0xFF,  0x05);
		NT35532_DCS_write_1A_1P(0xFB,  0x01);
		NT35532_DCS_write_1A_1P(0x00,  0x3F);
		NT35532_DCS_write_1A_1P(0x01,  0x3F);
		NT35532_DCS_write_1A_1P(0x02,  0x3F);
		NT35532_DCS_write_1A_1P(0x03,  0x3F);
		NT35532_DCS_write_1A_1P(0x04,  0x38);
		NT35532_DCS_write_1A_1P(0x05,  0x3F);
		NT35532_DCS_write_1A_1P(0x06,  0x3F);
		NT35532_DCS_write_1A_1P(0x07,  0x19);
		NT35532_DCS_write_1A_1P(0x08,  0x1D);
		NT35532_DCS_write_1A_1P(0x09,  0x3F);
		NT35532_DCS_write_1A_1P(0x0A,  0x3F);
		NT35532_DCS_write_1A_1P(0x0B,  0x1B);
		NT35532_DCS_write_1A_1P(0x0C,  0x17);
		NT35532_DCS_write_1A_1P(0x0D,  0x3F);
		NT35532_DCS_write_1A_1P(0x0E,  0x3F);
		NT35532_DCS_write_1A_1P(0x0F,  0x08);
		NT35532_DCS_write_1A_1P(0x10,  0x3F);
		NT35532_DCS_write_1A_1P(0x11,  0x10);
		NT35532_DCS_write_1A_1P(0x12,  0x3F);
		NT35532_DCS_write_1A_1P(0x13,  0x3F);
		NT35532_DCS_write_1A_1P(0x14,  0x3F);
		NT35532_DCS_write_1A_1P(0x15,  0x3F);
		NT35532_DCS_write_1A_1P(0x16,  0x3F);
		NT35532_DCS_write_1A_1P(0x17,  0x3F);
		NT35532_DCS_write_1A_1P(0x18,  0x38);
		NT35532_DCS_write_1A_1P(0x19,  0x18);
		NT35532_DCS_write_1A_1P(0x1A,  0x1C);
		NT35532_DCS_write_1A_1P(0x1B,  0x3F);
		NT35532_DCS_write_1A_1P(0x1C,  0x3F);
		NT35532_DCS_write_1A_1P(0x1D,  0x1A);
		NT35532_DCS_write_1A_1P(0x1E,  0x16);
		NT35532_DCS_write_1A_1P(0x1F,  0x3F);
		NT35532_DCS_write_1A_1P(0x20,  0x3F);
		NT35532_DCS_write_1A_1P(0x21,  0x3F);
		NT35532_DCS_write_1A_1P(0x22,  0x3F);
		NT35532_DCS_write_1A_1P(0x23,  0x06);
		NT35532_DCS_write_1A_1P(0x24,  0x3F);
		NT35532_DCS_write_1A_1P(0x25,  0x0E);
		NT35532_DCS_write_1A_1P(0x26,  0x3F);
		NT35532_DCS_write_1A_1P(0x27,  0x3F);
		NT35532_DCS_write_1A_1P(0x54,  0x06);
		NT35532_DCS_write_1A_1P(0x55,  0x05);
		NT35532_DCS_write_1A_1P(0x56,  0x04);
		NT35532_DCS_write_1A_1P(0x58,  0x03);
		NT35532_DCS_write_1A_1P(0x59,  0x1B);
		NT35532_DCS_write_1A_1P(0x5A,  0x1B);
		NT35532_DCS_write_1A_1P(0x5B,  0x01);
		NT35532_DCS_write_1A_1P(0x5C,  0x32);
		NT35532_DCS_write_1A_1P(0x5E,  0x18);
		NT35532_DCS_write_1A_1P(0x5F,  0x20);
		NT35532_DCS_write_1A_1P(0x60,  0x2B);
		NT35532_DCS_write_1A_1P(0x61,  0x2C);
		NT35532_DCS_write_1A_1P(0x62,  0x18);
		NT35532_DCS_write_1A_1P(0x63,  0x01);
		NT35532_DCS_write_1A_1P(0x64,  0x32);
		NT35532_DCS_write_1A_1P(0x65,  0x00);
		NT35532_DCS_write_1A_1P(0x66,  0x44);
		NT35532_DCS_write_1A_1P(0x67,  0x11);
		NT35532_DCS_write_1A_1P(0x68,  0x01);
		NT35532_DCS_write_1A_1P(0x69,  0x01);
		NT35532_DCS_write_1A_1P(0x6A,  0x04);
		NT35532_DCS_write_1A_1P(0x6B,  0x2C);
		NT35532_DCS_write_1A_1P(0x6C,  0x08);
		NT35532_DCS_write_1A_1P(0x6D,  0x08);
		NT35532_DCS_write_1A_1P(0x78,  0x00);
		NT35532_DCS_write_1A_1P(0x79,  0x00);
		NT35532_DCS_write_1A_1P(0x7E,  0x00);
		NT35532_DCS_write_1A_1P(0x7F,  0x00);
		NT35532_DCS_write_1A_1P(0x80,  0x00);
		NT35532_DCS_write_1A_1P(0x81,  0x00);
		NT35532_DCS_write_1A_1P(0x8D,  0x00);
		NT35532_DCS_write_1A_1P(0x8E,  0x00);
		NT35532_DCS_write_1A_1P(0x8F,  0xC0);
		NT35532_DCS_write_1A_1P(0x90,  0x73);
		NT35532_DCS_write_1A_1P(0x91,  0x10);
		NT35532_DCS_write_1A_1P(0x92,  0x07);
		NT35532_DCS_write_1A_1P(0x96,  0x11);
		NT35532_DCS_write_1A_1P(0x97,  0x14);
		NT35532_DCS_write_1A_1P(0x98,  0x00);
		NT35532_DCS_write_1A_1P(0x99,  0x00);
		NT35532_DCS_write_1A_1P(0x9A,  0x00);
		NT35532_DCS_write_1A_1P(0x9B,  0x61);
		NT35532_DCS_write_1A_1P(0x9C,  0x15);
		NT35532_DCS_write_1A_1P(0x9D,  0x30);
		NT35532_DCS_write_1A_1P(0x9F,  0x0F);
		NT35532_DCS_write_1A_1P(0xA2,  0xB0);
		NT35532_DCS_write_1A_1P(0xA7,  0x0A);
		NT35532_DCS_write_1A_1P(0xA9,  0x00);
		NT35532_DCS_write_1A_1P(0xAA,  0x70);
		NT35532_DCS_write_1A_1P(0xAB,  0xDA);
		NT35532_DCS_write_1A_1P(0xAC,  0xFF);
		NT35532_DCS_write_1A_1P(0xAE,  0xF4);
		NT35532_DCS_write_1A_1P(0xAF,  0x40);
		NT35532_DCS_write_1A_1P(0xB0,  0x7F);
		NT35532_DCS_write_1A_1P(0xB1,  0x16);
		NT35532_DCS_write_1A_1P(0xB2,  0x53);
		NT35532_DCS_write_1A_1P(0xB3,  0x00);
		NT35532_DCS_write_1A_1P(0xB4,  0x2A);
		NT35532_DCS_write_1A_1P(0xB5,  0x3A);
		NT35532_DCS_write_1A_1P(0xB6,  0xF0);
		NT35532_DCS_write_1A_1P(0xBC,  0x85);
		NT35532_DCS_write_1A_1P(0xBD,  0xF4);
		NT35532_DCS_write_1A_1P(0xBE,  0x33);
		NT35532_DCS_write_1A_1P(0xBF,  0x13);
		NT35532_DCS_write_1A_1P(0xC0,  0x77);
		NT35532_DCS_write_1A_1P(0xC1,  0x77);
		NT35532_DCS_write_1A_1P(0xC2,  0x77);
		NT35532_DCS_write_1A_1P(0xC3,  0x77);
		NT35532_DCS_write_1A_1P(0xC4,  0x77);
		NT35532_DCS_write_1A_1P(0xC5,  0x77);
		NT35532_DCS_write_1A_1P(0xC6,  0x77);
		NT35532_DCS_write_1A_1P(0xC7,  0x77);
		NT35532_DCS_write_1A_1P(0xC8,  0xAA);
		NT35532_DCS_write_1A_1P(0xC9,  0x2A);
		NT35532_DCS_write_1A_1P(0xCA,  0x00);
		NT35532_DCS_write_1A_1P(0xCB,  0xAA);
		NT35532_DCS_write_1A_1P(0xCC,  0x92);
		NT35532_DCS_write_1A_1P(0xCD,  0x00);
		NT35532_DCS_write_1A_1P(0xCE,  0x18);
		NT35532_DCS_write_1A_1P(0xCF,  0x88);
		NT35532_DCS_write_1A_1P(0xD0,  0xAA);
		NT35532_DCS_write_1A_1P(0xD1,  0x00);
		NT35532_DCS_write_1A_1P(0xD2,  0x00);
		NT35532_DCS_write_1A_1P(0xD3,  0x00);
		NT35532_DCS_write_1A_1P(0xD6,  0x02);
		NT35532_DCS_write_1A_1P(0xED,  0x00);
		NT35532_DCS_write_1A_1P(0xEE,  0x00);
		NT35532_DCS_write_1A_1P(0xEF,  0x70);
		NT35532_DCS_write_1A_1P(0xFA,  0x03);
		NT35532_DCS_write_1A_1P(0xFF,  0x00);

		NT35532_DCS_write_1A_1P(0xFF,0x01);	  //CMD2 Page0
		NT35532_DCS_write_1A_1P(0xFB,0x01);	
		//add by sliter
		//{0x13, 0x18);    //* Flicker adjustment value
		//{0x14, 0x18);    //* Flicker adjustment value
		//{0x5E, 0x09);    //* Flicker adjustment value
		//add end
		NT35532_DCS_write_1A_1P(0x75,0x00);	 //Red posi
		NT35532_DCS_write_1A_1P(0x76,0x00);	
		NT35532_DCS_write_1A_1P(0x77,0x00);	
		NT35532_DCS_write_1A_1P(0x78,0x2C);	
		NT35532_DCS_write_1A_1P(0x79,0x00);	
		NT35532_DCS_write_1A_1P(0x7A,0x4F);	
		NT35532_DCS_write_1A_1P(0x7B,0x00);	
		NT35532_DCS_write_1A_1P(0x7C,0x69);	
		NT35532_DCS_write_1A_1P(0x7D,0x00);	
		NT35532_DCS_write_1A_1P(0x7E,0x7F);	
		NT35532_DCS_write_1A_1P(0x7F,0x00);	
		NT35532_DCS_write_1A_1P(0x80,0x92);	
		NT35532_DCS_write_1A_1P(0x81,0x00);	
		NT35532_DCS_write_1A_1P(0x82,0xA3);	
		NT35532_DCS_write_1A_1P(0x83,0x00);	
		NT35532_DCS_write_1A_1P(0x84,0xB3);	
		NT35532_DCS_write_1A_1P(0x85,0x00);	
		NT35532_DCS_write_1A_1P(0x86,0xC1);	
		NT35532_DCS_write_1A_1P(0x87,0x00);	
		NT35532_DCS_write_1A_1P(0x88,0xF3);	
		NT35532_DCS_write_1A_1P(0x89,0x01);	
		NT35532_DCS_write_1A_1P(0x8A,0x1B);	
		NT35532_DCS_write_1A_1P(0x8B,0x01);	
		NT35532_DCS_write_1A_1P(0x8C,0x5A);	
		NT35532_DCS_write_1A_1P(0x8D,0x01);	
		NT35532_DCS_write_1A_1P(0x8E,0x8B);	
		NT35532_DCS_write_1A_1P(0x8F,0x01);	
		NT35532_DCS_write_1A_1P(0x90,0xD9);	
		NT35532_DCS_write_1A_1P(0x91,0x02);	
		NT35532_DCS_write_1A_1P(0x92,0x16);	
		NT35532_DCS_write_1A_1P(0x93,0x02);	
		NT35532_DCS_write_1A_1P(0x94,0x18);	
		NT35532_DCS_write_1A_1P(0x95,0x02);	
		NT35532_DCS_write_1A_1P(0x96,0x4E);	
		NT35532_DCS_write_1A_1P(0x97,0x02);	
		NT35532_DCS_write_1A_1P(0x98,0x88);	
		NT35532_DCS_write_1A_1P(0x99,0x02);	
		NT35532_DCS_write_1A_1P(0x9A,0xAC);	
		NT35532_DCS_write_1A_1P(0x9B,0x02);	
		NT35532_DCS_write_1A_1P(0x9C,0xDD);	
		NT35532_DCS_write_1A_1P(0x9D,0x03);	
		NT35532_DCS_write_1A_1P(0x9E,0x01);	
		NT35532_DCS_write_1A_1P(0x9F,0x03);	
		NT35532_DCS_write_1A_1P(0xA0,0x2E);	
		NT35532_DCS_write_1A_1P(0xA2,0x03);	
		NT35532_DCS_write_1A_1P(0xA3,0x3C);	
		NT35532_DCS_write_1A_1P(0xA4,0x03);	
		NT35532_DCS_write_1A_1P(0xA5,0x4C);	
		NT35532_DCS_write_1A_1P(0xA6,0x03);	
		NT35532_DCS_write_1A_1P(0xA7,0x5D);	
		NT35532_DCS_write_1A_1P(0xA9,0x03);	
		NT35532_DCS_write_1A_1P(0xAA,0x70);	
		NT35532_DCS_write_1A_1P(0xAB,0x03);	
		NT35532_DCS_write_1A_1P(0xAC,0x88);	
		NT35532_DCS_write_1A_1P(0xAD,0x03);	
		NT35532_DCS_write_1A_1P(0xAE,0xA8);	
		NT35532_DCS_write_1A_1P(0xAF,0x03);	
		NT35532_DCS_write_1A_1P(0xB0,0xC8);	
		NT35532_DCS_write_1A_1P(0xB1,0x03);	
		NT35532_DCS_write_1A_1P(0xB2,0xFF);	
		NT35532_DCS_write_1A_1P(0xB3,0x00);	//Red nega
		NT35532_DCS_write_1A_1P(0xB4,0x00);	
		NT35532_DCS_write_1A_1P(0xB5,0x00);	
		NT35532_DCS_write_1A_1P(0xB6,0x2C);	
		NT35532_DCS_write_1A_1P(0xB7,0x00);	
		NT35532_DCS_write_1A_1P(0xB8,0x4F);	
		NT35532_DCS_write_1A_1P(0xB9,0x00);	
		NT35532_DCS_write_1A_1P(0xBA,0x69);	
		NT35532_DCS_write_1A_1P(0xBB,0x00);	
		NT35532_DCS_write_1A_1P(0xBC,0x7F);	
		NT35532_DCS_write_1A_1P(0xBD,0x00);	
		NT35532_DCS_write_1A_1P(0xBE,0x92);	
		NT35532_DCS_write_1A_1P(0xBF,0x00);	
		NT35532_DCS_write_1A_1P(0xC0,0xA3);	
		NT35532_DCS_write_1A_1P(0xC1,0x00);	
		NT35532_DCS_write_1A_1P(0xC2,0xB3);	
		NT35532_DCS_write_1A_1P(0xC3,0x00);	
		NT35532_DCS_write_1A_1P(0xC4,0xC1);	
		NT35532_DCS_write_1A_1P(0xC5,0x00);	
		NT35532_DCS_write_1A_1P(0xC6,0xF3);	
		NT35532_DCS_write_1A_1P(0xC7,0x01);	
		NT35532_DCS_write_1A_1P(0xC8,0x1B);	
		NT35532_DCS_write_1A_1P(0xC9,0x01);	
		NT35532_DCS_write_1A_1P(0xCA,0x5A);	
		NT35532_DCS_write_1A_1P(0xCB,0x01);	
		NT35532_DCS_write_1A_1P(0xCC,0x8B);	
		NT35532_DCS_write_1A_1P(0xCD,0x01);	
		NT35532_DCS_write_1A_1P(0xCE,0xD9);	
		NT35532_DCS_write_1A_1P(0xCF,0x02);	
		NT35532_DCS_write_1A_1P(0xD0,0x16);	
		NT35532_DCS_write_1A_1P(0xD1,0x02);	
		NT35532_DCS_write_1A_1P(0xD2,0x18);	
		NT35532_DCS_write_1A_1P(0xD3,0x02);	
		NT35532_DCS_write_1A_1P(0xD4,0x4E);	
		NT35532_DCS_write_1A_1P(0xD5,0x02);	
		NT35532_DCS_write_1A_1P(0xD6,0x88);	
		NT35532_DCS_write_1A_1P(0xD7,0x02);	
		NT35532_DCS_write_1A_1P(0xD8,0xAC);	
		NT35532_DCS_write_1A_1P(0xD9,0x02);	
		NT35532_DCS_write_1A_1P(0xDA,0xDD);	
		NT35532_DCS_write_1A_1P(0xDB,0x03);	
		NT35532_DCS_write_1A_1P(0xDC,0x01);	
		NT35532_DCS_write_1A_1P(0xDD,0x03);	
		NT35532_DCS_write_1A_1P(0xDE,0x2E);	
		NT35532_DCS_write_1A_1P(0xDF,0x03);	
		NT35532_DCS_write_1A_1P(0xE0,0x3C);	
		NT35532_DCS_write_1A_1P(0xE1,0x03);	
		NT35532_DCS_write_1A_1P(0xE2,0x4C);	
		NT35532_DCS_write_1A_1P(0xE3,0x03);	
		NT35532_DCS_write_1A_1P(0xE4,0x5D);	
		NT35532_DCS_write_1A_1P(0xE5,0x03);	
		NT35532_DCS_write_1A_1P(0xE6,0x70);	
		NT35532_DCS_write_1A_1P(0xE7,0x03);	
		NT35532_DCS_write_1A_1P(0xE8,0x88);	
		NT35532_DCS_write_1A_1P(0xE9,0x03);	
		NT35532_DCS_write_1A_1P(0xEA,0xA8);	
		NT35532_DCS_write_1A_1P(0xEB,0x03);	
		NT35532_DCS_write_1A_1P(0xEC,0xC8);	
		NT35532_DCS_write_1A_1P(0xED,0x03);	
		NT35532_DCS_write_1A_1P(0xEE,0xFF);	
		NT35532_DCS_write_1A_1P(0xEF,0x00);	//Green posi
		NT35532_DCS_write_1A_1P(0xF0,0x00);	
		NT35532_DCS_write_1A_1P(0xF1,0x00);	
		NT35532_DCS_write_1A_1P(0xF2,0x2C);	
		NT35532_DCS_write_1A_1P(0xF3,0x00);	
		NT35532_DCS_write_1A_1P(0xF4,0x4F);	
		NT35532_DCS_write_1A_1P(0xF5,0x00);	
		NT35532_DCS_write_1A_1P(0xF6,0x69);	
		NT35532_DCS_write_1A_1P(0xF7,0x00);	
		NT35532_DCS_write_1A_1P(0xF8,0x7F);	
		NT35532_DCS_write_1A_1P(0xF9,0x00);	
		NT35532_DCS_write_1A_1P(0xFA,0x92);

		NT35532_DCS_write_1A_1P(0xFF,0x02);	 //CMD2 Page1
		NT35532_DCS_write_1A_1P(0xFB,0x01);	
		NT35532_DCS_write_1A_1P(0x0,0x00);	
		NT35532_DCS_write_1A_1P(0x1,0xA3);	
		NT35532_DCS_write_1A_1P(0x2,0x00);	
		NT35532_DCS_write_1A_1P(0x3,0xB3);	
		NT35532_DCS_write_1A_1P(0x4,0x00);	
		NT35532_DCS_write_1A_1P(0x5,0xC1);	
		NT35532_DCS_write_1A_1P(0x6,0x00);	
		NT35532_DCS_write_1A_1P(0x7,0xF3);	
		NT35532_DCS_write_1A_1P(0x8,0x01);	
		NT35532_DCS_write_1A_1P(0x9,0x1B);	
		NT35532_DCS_write_1A_1P(0x0A,0x01);	
		NT35532_DCS_write_1A_1P(0x0B,0x5A);	
		NT35532_DCS_write_1A_1P(0x0C,0x01);	
		NT35532_DCS_write_1A_1P(0x0D,0x8B);	
		NT35532_DCS_write_1A_1P(0x0E,0x01);	
		NT35532_DCS_write_1A_1P(0x0F,0xD9);	
		NT35532_DCS_write_1A_1P(0x10,0x02);	
		NT35532_DCS_write_1A_1P(0x11,0x16);	
		NT35532_DCS_write_1A_1P(0x12,0x02);	
		NT35532_DCS_write_1A_1P(0x13,0x18);	
		NT35532_DCS_write_1A_1P(0x14,0x02);	
		NT35532_DCS_write_1A_1P(0x15,0x4E);	
		NT35532_DCS_write_1A_1P(0x16,0x02);	
		NT35532_DCS_write_1A_1P(0x17,0x88);	
		NT35532_DCS_write_1A_1P(0x18,0x02);	
		NT35532_DCS_write_1A_1P(0x19,0xAC);	
		NT35532_DCS_write_1A_1P(0x1A,0x02);	
		NT35532_DCS_write_1A_1P(0x1B,0xDD);	
		NT35532_DCS_write_1A_1P(0x1C,0x03);	
		NT35532_DCS_write_1A_1P(0x1D,0x01);	
		NT35532_DCS_write_1A_1P(0x1E,0x03);	
		NT35532_DCS_write_1A_1P(0x1F,0x2E);	
		NT35532_DCS_write_1A_1P(0x20,0x03);	
		NT35532_DCS_write_1A_1P(0x21,0x3C);	
		NT35532_DCS_write_1A_1P(0x22,0x03);	
		NT35532_DCS_write_1A_1P(0x23,0x4C);	
		NT35532_DCS_write_1A_1P(0x24,0x03);	
		NT35532_DCS_write_1A_1P(0x25,0x5D);	
		NT35532_DCS_write_1A_1P(0x26,0x03);	
		NT35532_DCS_write_1A_1P(0x27,0x70);	
		NT35532_DCS_write_1A_1P(0x28,0x03);	
		NT35532_DCS_write_1A_1P(0x29,0x88);	
		NT35532_DCS_write_1A_1P(0x2A,0x03);	
		NT35532_DCS_write_1A_1P(0x2B,0xA8);	
		NT35532_DCS_write_1A_1P(0x2D,0x03);	
		NT35532_DCS_write_1A_1P(0x2F,0xC8);	
		NT35532_DCS_write_1A_1P(0x30,0x03);	
		NT35532_DCS_write_1A_1P(0x31,0xFF);	
		NT35532_DCS_write_1A_1P(0x32,0x00);	 //Green nega
		NT35532_DCS_write_1A_1P(0x33,0x00);	
		NT35532_DCS_write_1A_1P(0x34,0x00);	
		NT35532_DCS_write_1A_1P(0x35,0x2C);	
		NT35532_DCS_write_1A_1P(0x36,0x00);	
		NT35532_DCS_write_1A_1P(0x37,0x4F);	
		NT35532_DCS_write_1A_1P(0x38,0x00);	
		NT35532_DCS_write_1A_1P(0x39,0x69);	
		NT35532_DCS_write_1A_1P(0x3A,0x00);	
		NT35532_DCS_write_1A_1P(0x3B,0x7F);	
		NT35532_DCS_write_1A_1P(0x3D,0x00);	
		NT35532_DCS_write_1A_1P(0x3F,0x92);	
		NT35532_DCS_write_1A_1P(0x40,0x00);	
		NT35532_DCS_write_1A_1P(0x41,0xA3);	
		NT35532_DCS_write_1A_1P(0x42,0x00);	
		NT35532_DCS_write_1A_1P(0x43,0xB3);	
		NT35532_DCS_write_1A_1P(0x44,0x00);	
		NT35532_DCS_write_1A_1P(0x45,0xC1);	
		NT35532_DCS_write_1A_1P(0x46,0x00);	
		NT35532_DCS_write_1A_1P(0x47,0xF3);	
		NT35532_DCS_write_1A_1P(0x48,0x01);	
		NT35532_DCS_write_1A_1P(0x49,0x1B);	
		NT35532_DCS_write_1A_1P(0x4A,0x01);	
		NT35532_DCS_write_1A_1P(0x4B,0x5A);	
		NT35532_DCS_write_1A_1P(0x4C,0x01);	
		NT35532_DCS_write_1A_1P(0x4D,0x8B);	
		NT35532_DCS_write_1A_1P(0x4E,0x01);	
		NT35532_DCS_write_1A_1P(0x4F,0xD9);	
		NT35532_DCS_write_1A_1P(0x50,0x02);	
		NT35532_DCS_write_1A_1P(0x51,0x16);	
		NT35532_DCS_write_1A_1P(0x52,0x02);	
		NT35532_DCS_write_1A_1P(0x53,0x18);	
		NT35532_DCS_write_1A_1P(0x54,0x02);	
		NT35532_DCS_write_1A_1P(0x55,0x4E);	
		NT35532_DCS_write_1A_1P(0x56,0x02);	
		NT35532_DCS_write_1A_1P(0x58,0x88);	
		NT35532_DCS_write_1A_1P(0x59,0x02);	
		NT35532_DCS_write_1A_1P(0x5A,0xAC);	
		NT35532_DCS_write_1A_1P(0x5B,0x02);	
		NT35532_DCS_write_1A_1P(0x5C,0xDD);	
		NT35532_DCS_write_1A_1P(0x5D,0x03);	
		NT35532_DCS_write_1A_1P(0x5E,0x01);	
		NT35532_DCS_write_1A_1P(0x5F,0x03);	
		NT35532_DCS_write_1A_1P(0x60,0x2E);	
		NT35532_DCS_write_1A_1P(0x61,0x03);	
		NT35532_DCS_write_1A_1P(0x62,0x3C);	
		NT35532_DCS_write_1A_1P(0x63,0x03);	
		NT35532_DCS_write_1A_1P(0x64,0x4C);	
		NT35532_DCS_write_1A_1P(0x65,0x03);	
		NT35532_DCS_write_1A_1P(0x66,0x5D);	
		NT35532_DCS_write_1A_1P(0x67,0x03);	
		NT35532_DCS_write_1A_1P(0x68,0x70);	
		NT35532_DCS_write_1A_1P(0x69,0x03);	
		NT35532_DCS_write_1A_1P(0x6A,0x88);	
		NT35532_DCS_write_1A_1P(0x6B,0x03);	
		NT35532_DCS_write_1A_1P(0x6C,0xA8);	
		NT35532_DCS_write_1A_1P(0x6D,0x03);	
		NT35532_DCS_write_1A_1P(0x6E,0xC8);	
		NT35532_DCS_write_1A_1P(0x6F,0x03);	
		NT35532_DCS_write_1A_1P(0x70,0xFF);	
		NT35532_DCS_write_1A_1P(0x71,0x00);	 //Blue posi
		NT35532_DCS_write_1A_1P(0x72,0x00);	
		NT35532_DCS_write_1A_1P(0x73,0x00);	
		NT35532_DCS_write_1A_1P(0x74,0x2C);	
		NT35532_DCS_write_1A_1P(0x75,0x00);	
		NT35532_DCS_write_1A_1P(0x76,0x4F);	
		NT35532_DCS_write_1A_1P(0x77,0x00);	
		NT35532_DCS_write_1A_1P(0x78,0x69);	
		NT35532_DCS_write_1A_1P(0x79,0x00);	
		NT35532_DCS_write_1A_1P(0x7A,0x7F);	
		NT35532_DCS_write_1A_1P(0x7B,0x00);	
		NT35532_DCS_write_1A_1P(0x7C,0x92);	
		NT35532_DCS_write_1A_1P(0x7D,0x00);	
		NT35532_DCS_write_1A_1P(0x7E,0xA3);	
		NT35532_DCS_write_1A_1P(0x7F,0x00);	
		NT35532_DCS_write_1A_1P(0x80,0xB3);	
		NT35532_DCS_write_1A_1P(0x81,0x00);	
		NT35532_DCS_write_1A_1P(0x82,0xC1);	
		NT35532_DCS_write_1A_1P(0x83,0x00);	
		NT35532_DCS_write_1A_1P(0x84,0xF3);	
		NT35532_DCS_write_1A_1P(0x85,0x01);	
		NT35532_DCS_write_1A_1P(0x86,0x1B);	
		NT35532_DCS_write_1A_1P(0x87,0x01);	
		NT35532_DCS_write_1A_1P(0x88,0x5A);	
		NT35532_DCS_write_1A_1P(0x89,0x01);	
		NT35532_DCS_write_1A_1P(0x8A,0x8B);	
		NT35532_DCS_write_1A_1P(0x8B,0x01);	
		NT35532_DCS_write_1A_1P(0x8C,0xD9);	
		NT35532_DCS_write_1A_1P(0x8D,0x02);	
		NT35532_DCS_write_1A_1P(0x8E,0x16);	
		NT35532_DCS_write_1A_1P(0x8F,0x02);	
		NT35532_DCS_write_1A_1P(0x90,0x18);	
		NT35532_DCS_write_1A_1P(0x91,0x02);	
		NT35532_DCS_write_1A_1P(0x92,0x4E);	
		NT35532_DCS_write_1A_1P(0x93,0x02);	
		NT35532_DCS_write_1A_1P(0x94,0x88);	
		NT35532_DCS_write_1A_1P(0x95,0x02);	
		NT35532_DCS_write_1A_1P(0x96,0xAC);	
		NT35532_DCS_write_1A_1P(0x97,0x02);	
		NT35532_DCS_write_1A_1P(0x98,0xDD);	
		NT35532_DCS_write_1A_1P(0x99,0x03);	
		NT35532_DCS_write_1A_1P(0x9A,0x01);	
		NT35532_DCS_write_1A_1P(0x9B,0x03);	
		NT35532_DCS_write_1A_1P(0x9C,0x2E);	
		NT35532_DCS_write_1A_1P(0x9D,0x03);	
		NT35532_DCS_write_1A_1P(0x9E,0x3C);	
		NT35532_DCS_write_1A_1P(0x9F,0x03);	
		NT35532_DCS_write_1A_1P(0xA0,0x4C);	
		NT35532_DCS_write_1A_1P(0xA2,0x03);	
		NT35532_DCS_write_1A_1P(0xA3,0x5D);	
		NT35532_DCS_write_1A_1P(0xA4,0x03);	
		NT35532_DCS_write_1A_1P(0xA5,0x70);	
		NT35532_DCS_write_1A_1P(0xA6,0x03);	
		NT35532_DCS_write_1A_1P(0xA7,0x88);	
		NT35532_DCS_write_1A_1P(0xA9,0x03);	
		NT35532_DCS_write_1A_1P(0xAA,0xA8);	
		NT35532_DCS_write_1A_1P(0xAB,0x03);	
		NT35532_DCS_write_1A_1P(0xAC,0xC8);	
		NT35532_DCS_write_1A_1P(0xAD,0x03);	
		NT35532_DCS_write_1A_1P(0xAE,0xFF);	
		NT35532_DCS_write_1A_1P(0xAF,0x00);	//Blue nega
		NT35532_DCS_write_1A_1P(0xB0,0x00);	
		NT35532_DCS_write_1A_1P(0xB1,0x00);	
		NT35532_DCS_write_1A_1P(0xB2,0x2C);	
		NT35532_DCS_write_1A_1P(0xB3,0x00);	
		NT35532_DCS_write_1A_1P(0xB4,0x4F);	
		NT35532_DCS_write_1A_1P(0xB5,0x00);	
		NT35532_DCS_write_1A_1P(0xB6,0x69);	
		NT35532_DCS_write_1A_1P(0xB7,0x00);	
		NT35532_DCS_write_1A_1P(0xB8,0x7F);	
		NT35532_DCS_write_1A_1P(0xB9,0x00);	
		NT35532_DCS_write_1A_1P(0xBA,0x92);	
		NT35532_DCS_write_1A_1P(0xBB,0x00);	
		NT35532_DCS_write_1A_1P(0xBC,0xA3);	
		NT35532_DCS_write_1A_1P(0xBD,0x00);	
		NT35532_DCS_write_1A_1P(0xBE,0xB3);	
		NT35532_DCS_write_1A_1P(0xBF,0x00);	
		NT35532_DCS_write_1A_1P(0xC0,0xC1);	
		NT35532_DCS_write_1A_1P(0xC1,0x00);	
		NT35532_DCS_write_1A_1P(0xC2,0xF3);	
		NT35532_DCS_write_1A_1P(0xC3,0x01);	
		NT35532_DCS_write_1A_1P(0xC4,0x1B);	
		NT35532_DCS_write_1A_1P(0xC5,0x01);	
		NT35532_DCS_write_1A_1P(0xC6,0x5A);	
		NT35532_DCS_write_1A_1P(0xC7,0x01);	
		NT35532_DCS_write_1A_1P(0xC8,0x8B);	
		NT35532_DCS_write_1A_1P(0xC9,0x01);	
		NT35532_DCS_write_1A_1P(0xCA,0xD9);	
		NT35532_DCS_write_1A_1P(0xCB,0x02);	
		NT35532_DCS_write_1A_1P(0xCC,0x16);	
		NT35532_DCS_write_1A_1P(0xCD,0x02);	
		NT35532_DCS_write_1A_1P(0xCE,0x18);	
		NT35532_DCS_write_1A_1P(0xCF,0x02);	
		NT35532_DCS_write_1A_1P(0xD0,0x4E);	
		NT35532_DCS_write_1A_1P(0xD1,0x02);	
		NT35532_DCS_write_1A_1P(0xD2,0x88);	
		NT35532_DCS_write_1A_1P(0xD3,0x02);	
		NT35532_DCS_write_1A_1P(0xD4,0xAC);	
		NT35532_DCS_write_1A_1P(0xD5,0x02);	
		NT35532_DCS_write_1A_1P(0xD6,0xDD);	
		NT35532_DCS_write_1A_1P(0xD7,0x03);	
		NT35532_DCS_write_1A_1P(0xD8,0x01);	
		NT35532_DCS_write_1A_1P(0xD9,0x03);	
		NT35532_DCS_write_1A_1P(0xDA,0x2E);	
		NT35532_DCS_write_1A_1P(0xDB,0x03);	
		NT35532_DCS_write_1A_1P(0xDC,0x3C);	
		NT35532_DCS_write_1A_1P(0xDD,0x03);	
		NT35532_DCS_write_1A_1P(0xDE,0x4C);	
		NT35532_DCS_write_1A_1P(0xDF,0x03);	
		NT35532_DCS_write_1A_1P(0xE0,0x5D);	
		NT35532_DCS_write_1A_1P(0xE1,0x03);	
		NT35532_DCS_write_1A_1P(0xE2,0x70);	
		NT35532_DCS_write_1A_1P(0xE3,0x03);	
		NT35532_DCS_write_1A_1P(0xE4,0x88);	
		NT35532_DCS_write_1A_1P(0xE5,0x03);	
		NT35532_DCS_write_1A_1P(0xE6,0xA8);	
		NT35532_DCS_write_1A_1P(0xE7,0x03);	
		NT35532_DCS_write_1A_1P(0xE8,0xC8);	
		NT35532_DCS_write_1A_1P(0xE9,0x03);	
		NT35532_DCS_write_1A_1P(0xEA,0xFF);	

		NT35532_DCS_write_1A_1P(0xFF,0x00);	
		NT35532_DCS_write_1A_1P(0xFB,0x01);

		NT35532_DCS_write_1A_1P(0x35,0x00);

		NT35532_DCS_write_1A_1P(0xFF,0x00);

		data_array[0]=0x00110500; // Display Off
		dsi_set_cmdq(data_array, 1, 1);
		MDELAY(120);
		data_array[0]=0x00290500; // Display Off
		dsi_set_cmdq(data_array, 1, 1);
		MDELAY(20);
	
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

    init_lcm_registers();
}


static void lcm_suspend(void)
{
		unsigned int data_array[16]; 
		//LCM_LOGI("lcm_suspend enter\n");

		NT35532_DCS_write_1A_1P(0xFF,0x00);
		NT35532_DCS_write_1A_1P(0xFB,0x01);

		data_array[0]=0x00280500; // Display Off
		dsi_set_cmdq(data_array, 1, 1);
		MDELAY(20);
		data_array[0]=0x00100500; // Display Off
		dsi_set_cmdq(data_array, 1, 1);
		MDELAY(120);

		NT35532_DCS_write_1A_1P(0xFF,0x05);
		NT35532_DCS_write_1A_1P(0xFB,0x01);
		NT35532_DCS_write_1A_1P(0xD7,0x30); 
		NT35532_DCS_write_1A_1P(0xD8,0x70); 
		//SET_RESET_PIN(0);
		//MDELAY(60);
}

static void lcm_resume(void)
{
		lcm_init();
}

static unsigned int lcm_compare_id(void)
{
		unsigned int id=0;
		unsigned char buffer[2];
		unsigned int array[16];  

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
		printf("***********%s, LK nt35532 debug: nt35532 id = 0x%08x\n", __func__, id);
    #else
		printk("*****%s, kernel nt35590 horse debug: nt35590 id = 0x%08x\n", __func__, id);
    #endif
	
    if(id == LCM_ID_NT35532)
    	return 1;
    else
        return 0;
	  
}

LCM_DRIVER nt35532_fhd_dsi_vdo_hp_5516_lcm_drv = 
{
    .name           = "nt35532_fhd_dsi_vdo_hp_5516", 
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
   
};
