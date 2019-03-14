#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"
//#include <debug.h>
#include <platform/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <cust_i2c.h>

// ------------------------------iic device begin----------------------------------
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_i2c.h>   
#include <platform/mt_pmic.h>
//#include <platform/bq24296.h>
#include <printf.h>
#include <platform/mt_pwm.h>

#define LCM_SLAVE_ADDR_WRITE   0x7C //7bit 3E
#define LCM_SLAVE_ADDR_READ    0x7D
#define LCM_I2C_ID	I2C1
        
extern kal_uint32 ili9881_lcm_write_byte(kal_uint8 addr, kal_uint8 value);
extern kal_uint32 ili9881_lcm_read_byte(kal_uint8 addr, kal_uint8 *dataBuffer); 

static struct mt_i2c_t lcm_i2c;
#define GPIO_65132_ENP   (GPIO64 | 0x80000000)
#define GPIO_65132_ENN   (GPIO63 | 0x80000000)

kal_uint32 ili9881_lcm_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    lcm_i2c.id = LCM_I2C_ID;
    /* Since i2c will left shift 1 bit, we need to set BQ24296 I2C address to >>1 */
    lcm_i2c.addr = (LCM_SLAVE_ADDR_WRITE >> 1);
    lcm_i2c.mode = ST_MODE;
    lcm_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&lcm_i2c, write_data, len);

    if(I2C_OK != ret_code)
        dprintf(INFO, "%s: i2c_write: ret_code: %d\n", __func__, ret_code);

    return ret_code;
}

kal_uint32 ili9881_lcm_read_byte(kal_uint8 addr, kal_uint8 *dataBuffer) 
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint16 len;
    *dataBuffer = addr;

    lcm_i2c.id = LCM_I2C_ID;
    /* Since i2c will left shift 1 bit, we need to set BQ24296 I2C address to >>1 */
    lcm_i2c.addr = (LCM_SLAVE_ADDR_READ >> 1);
    lcm_i2c.mode = ST_MODE;
    lcm_i2c.speed = 100;
    len = 1;

    ret_code = i2c_write_read(&lcm_i2c, dataBuffer, len, len);

    if(I2C_OK != ret_code)
        dprintf(INFO, "%s: i2c_read: ret_code: %d\n", __func__, ret_code);
        

    return ret_code;
}
// ------------------------------iic device end----------------------------------


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1280)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))
#define REGFLAG_DELAY                                       0XFD
#define REGFLAG_END_OF_TABLE                                0xFE   // END OF REGISTERS MARKER


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                  lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)              lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

//#define LCM_DSI_CMD_MODE

struct LCM_setting_table {
    unsigned char cmd;
	  unsigned char count;
    unsigned char para_list[128];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
{0xFF,3,{0x98,0x81,0x03}},
//GIP_1
{0x01,1,{0x00}},
{0x02,1,{0x00}},
{0x03,1,{0x53}},
{0x04,1,{0x13}},
{0x05,1,{0x13}},
{0x06,1,{0x06}},  
{0x07,1,{0x00}},
{0x08,1,{0x04}},
{0x09,1,{0x00}},        
{0x0a,1,{0x00}},
{0x0b,1,{0x00}},
{0x0c,1,{0x00}},
{0x0d,1,{0x00}},
{0x0e,1,{0x00}},
{0x0f,1,{0x00}},  
{0x10,1,{0x00}},  
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x13,1,{0x00}},  
{0x14,1,{0x00}},
{0x15,1,{0x00}},
{0x16,1,{0x00}}, 
{0x17,1,{0x00}}, 
{0x18,1,{0x00}},
{0x19,1,{0x00}},
{0x1a,1,{0x00}},
{0x1b,1,{0x00}},
{0x1c,1,{0x00}},
{0x1d,1,{0x00}},   
{0x1e,1,{0xC0}},
{0x1f,1,{0x80}},  
{0x20,1,{0x04}},
{0x21,1,{0x0B}},
{0x22,1,{0x00}},   
{0x23,1,{0x00}},
{0x24,1,{0x00}}, 
{0x25,1,{0x00}}, 
{0x26,1,{0x00}},
{0x27,1,{0x00}},
{0x28,1,{0x55}},  
{0x29,1,{0x03}},
{0x2a,1,{0x00}},
{0x2b,1,{0x00}},
{0x2c,1,{0x00}},
{0x2d,1,{0x00}},
{0x2e,1,{0x00}},
{0x2f,1,{0x00}},
{0x30,1,{0x00}},
{0x31,1,{0x00}},
{0x32,1,{0x00}},
{0x33,1,{0x00}},
{0x34,1,{0x04}},
{0x35,1,{0x05}},
{0x36,1,{0x05}},
{0x37,1,{0x00}},
{0x38,1,{0x3C}},         
{0x39,1,{0x00}},
{0x3a,1,{0x40}}, 
{0x3b,1,{0x40}},
{0x3c,1,{0x00}},
{0x3d,1,{0x00}},
{0x3e,1,{0x00}},
{0x3f,1,{0x00}},
{0x40,1,{0x00}},
{0x41,1,{0x00}},
{0x42,1,{0x00}},
{0x43,1,{0x00}},
{0x44,1,{0x00}},


//GIP_2
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

//GIP_3
{0x5e,1,{0x01}},   
{0x5f,1,{0x14}},
{0x60,1,{0x15}},
{0x61,1,{0x0C}},
{0x62,1,{0x0D}},
{0x63,1,{0x0E}},
{0x64,1,{0x0F}},
{0x65,1,{0x10}},
{0x66,1,{0x11}},
{0x67,1,{0x08}},
{0x68,1,{0x02}},
{0x69,1,{0x0A}},
{0x6a,1,{0x02}},
{0x6b,1,{0x02}},
{0x6c,1,{0x02}},
{0x6d,1,{0x02}},
{0x6e,1,{0x02}},
{0x6f,1,{0x02}},
{0x70,1,{0x02}},
{0x71,1,{0x02}},
{0x72,1,{0x06}},
{0x73,1,{0x02}},
{0x74,1,{0x02}},
{0x75,1,{0x14}},
{0x76,1,{0x15}},
{0x77,1,{0x11}},
{0x78,1,{0x10}},
{0x79,1,{0x0F}},
{0x7a,1,{0x0E}},
{0x7b,1,{0x0D}},
{0x7c,1,{0x0C}},
{0x7d,1,{0x06}},
{0x7e,1,{0x02}},
{0x7f,1,{0x0A}},
{0x80,1,{0x02}},
{0x81,1,{0x02}},
{0x82,1,{0x02}},
{0x83,1,{0x02}},
{0x84,1,{0x02}},
{0x85,1,{0x02}},
{0x86,1,{0x02}},
{0x87,1,{0x02}},
{0x88,1,{0x08}},
{0x89,1,{0x02}},
{0x8A,1,{0x02}},

//CMD_Page 4
{0xFF,3,{0x98,0x81,0x04}},
{0x6C,1,{0x15}},
{0x6E,1,{0x3B}},               //di_pwr_reg=0 VGH clamp 13.6V
{0x6F,1,{0x53}},               // reg vcl + VGH pumping ratio 3x VGL=-2x
{0x3A,1,{0xA4}},               //POWER SAVING
{0x8D,1,{0x15}},               //VGL clamp -8.6V
{0x87,1,{0xBA}},               //ESD
{0x26,1,{0x76}},            
{0xB2,1,{0xD1}},
{0x88,1,{0x0B}},
{0x00,1,{0x80}}, //00h=3LANE  80h=4LANE(default)

//CMD_Page 1
{0xFF,3,{0x98,0x81,0x01}},
{0x22,1,{0x0A}},//0x09	·´É¨£¬ÏÔÊ¾Ðý×ª180	//BGR,SS
{0x53,1,{0x54}},		//VCOM1 60+ 5b+
{0x55,1,{0x54}},		//VCOM2
{0x50,1,{0x75}}, //75    6a   	//VREG1OUT=V
{0x51,1,{0x85}}, //85    7a   	//VREG2OUT=V
{0x31,1,{0x00}},		//00 column inversion; 02 dot inversion
{0x60,1,{0x14}},               //SDT
      
       //gamma 2.5
{0xA0, 1,{0x13}},			//VP255 Gamma P 0x08 //3.79v
{0xA1, 1,{0x29}},			//22 //VP251
{0xA2, 1,{0x36}},			//33 //VP247
{0xA3, 1,{0x0B}},			//12 //VP243
{0xA4, 1,{0x0B}},			//15 //VP239
{0xA5, 1,{0x1F}},			//27 //VP231
{0xA6, 1,{0x17}},			//1C //VP219
{0xA7, 1,{0x1A}},			//1F //VP203
{0xA8, 1,{0x93}},			//97 //VP175
{0xA9, 1,{0x1C}},			//1E //VP144
{0xAA, 1,{0x27}},			//29 //VP111
{0xAB, 1,{0x7B}},			//7C //VP80
{0xAC, 1,{0x1E}},			//1C //VP52
{0xAD, 1,{0x1F}},			//1A //VP36
{0xAE, 1,{0x55}},			//50 //VP24
{0xAF, 1,{0x29}},			//24 //VP16
{0xB0, 1,{0x2D}},			//29 //VP12
{0xB1, 1,{0x51}},			//44 //VP8
{0xB2, 1,{0x5C}},			//55 //VP4
{0xB3, 1,{0x21}},			//VP0
{0xC0, 1,{0x10}},			//VN255 GAMMA N 0x08 //-4.11
{0xC1, 1,{0x20}},			//22 //VN251
{0xC2, 1,{0x30}},			//33 //VN247
{0xC3, 1,{0x1A}},			//12 //VN243
{0xC4, 1,{0x1F}},			//15 //VN239
{0xC5, 1,{0x30}},			//27 //VN231
{0xC6, 1,{0x23}},			//1C //VN219
{0xC7, 1,{0x23}},			//1F //VN203
{0xC8, 1,{0xA5}},			//97 //VN175
{0xC9, 1,{0x1B}},			//1E //VN144
{0xCA, 1,{0x27}},			//29 //VN111
{0xCB, 1,{0x96}},			//7C //VN80
{0xCC, 1,{0x1E}},			//1C //VN52
{0xCD, 1,{0x1E}},			//1A //VN36
{0xCE, 1,{0x57}},			//50 //VN24
{0xCF, 1,{0x2D}},			//25 //VN16
{0xD0, 1,{0x2E}},			//29 //VN12
{0xD1, 1,{0x5A}},			//4C //VN8
{0xD2, 1,{0x5F}},			//55 //VN4
{0xD3, 1,{0x21}},			//VN0

//CMD_Page 0
{0xFF,3,{0x98,0x81,0x00}},
{0x35,1,{0x00}},
{0x11,1,{0x00}},
{REGFLAG_DELAY, 120, {}},
{0x29,1,{0x00}},
{REGFLAG_DELAY, 10, {}},

//Setting ending by predefined flag
{REGFLAG_END_OF_TABLE, 0x00, {}}	
};

void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {

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
   
	params->dsi.vertical_sync_active				= 8;
	params->dsi.vertical_backporch					= 16;
	params->dsi.vertical_frontporch					= 16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 10;
	params->dsi.horizontal_backporch				= 80;
	params->dsi.horizontal_frontporch				= 80;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	//improve clk quality
	params->dsi.PLL_CLOCK = 300; //this value  
	params->dsi.HS_TRAIL = 120;
	//params->dsi.ufoe_enable = 1;
    //params->dsi.ssc_disable = 1;


	//params->dsi.clk_lp_per_line_enable = 0;
	//params->dsi.esd_check_enable = 0;
	//params->dsi.customization_esd_check_enable = 0;
	//params->dsi.lcm_esd_check_table[0].cmd          = 0x53;
	//params->dsi.lcm_esd_check_table[0].count        = 1;
	//params->dsi.lcm_esd_check_table[0].para_list[0] = 0x24;
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[10];
	unsigned int array[16];

	SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50);
	SET_RESET_PIN(1);
	MDELAY(120);
	array[0]=0x00063902;
	array[1]=0x8198ffff;
	array[2]=0x00000104;
	dsi_set_cmdq(array, 3, 1);
	MDELAY(10);
 
	array[0] = 0x00033700;// return byte number
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x00, &buffer[0], 1);
	read_reg_v2(0x01, &buffer[1], 1);
	read_reg_v2(0x02, &buffer[2], 1);
	
	if((0x98==buffer[0]) && (0x81==buffer[1]))
	{
		id = 0x9881;
	}

#ifdef BUILD_LK
	printf("[LK]------ili9881 read id = 0x%x, 0x%x, 0x%x---------\n", buffer[0], buffer[1], buffer[2]);
#else
	printk("[KERNEL]------ili9881 read id = 0x%x, 0x%x, 0x%x---------\n", buffer[0], buffer[1], buffer[2]);
#endif
	return (0x9881 == id) ? 1 : 0;
}
static void lcm_iic_write(void)
{
   int ret =0;
     ret=ili9881_lcm_write_byte(0x00, 0x0A);
	if (ret)
		printf("[LK]otm9608----tps6132----cmd=0x00 write error----\n");
	else
		printf("[LK]otm9608----tps6132----cmd=0x00 write success----\n");
		     
     ret=ili9881_lcm_write_byte(0x01, 0x0A);
	if (ret)
		printf("[LK]otm9608----tps6132----cmd=0x01 write error----\n");
	else
		printf("[LK]otm9608----tps6132----cmd=0x01 write success----\n");
		     
   //  ret=ili9881_lcm_write_byte(0x03, 0x00);
	//if (ret)
	//	printf("[LK]otm9608----tps6132----cmd=0x03 write error----\n");
	//else
	//	printf("[LK]otm9608----tps6132----cmd=0x03 write success----\n");
		     
}
static void lcm_iic_reg_dump(void)
{
    kal_uint8 readbuff[3];
    
    ili9881_lcm_read_byte(0x00,&readbuff[0]);
    MDELAY(10);
    ili9881_lcm_read_byte(0x01,&readbuff[1]);
    MDELAY(10);    
    ili9881_lcm_read_byte(0x03,&readbuff[2]);
    MDELAY(10);    
    
    printf("lcm_iic_reg_dump: reg:0x00_readbuff==0x0A?: %x\n", readbuff[0]);
    printf("lcm_iic_reg_dump: reg:0x01_readbuff==0x0A?: %x\n", readbuff[1]);
    printf("lcm_iic_reg_dump: reg:0x03_readbuff==0x00?: %x\n", readbuff[2]);    
}
static void lcm_iic_65132_en(void)
{
	mt_set_gpio_mode(GPIO_65132_ENP, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_ENP, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_ENP, GPIO_OUT_ONE);
	
  mt_set_gpio_mode(GPIO_65132_ENN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_ENN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_ENN, GPIO_OUT_ONE);
	MDELAY(5);  
}

static void lcm_init(void)
{
    lcm_iic_65132_en();
    lcm_iic_write();
    //lcm_iic_reg_dump(); 
    //lcm_iic_65132_en(); 
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50);
	SET_RESET_PIN(1);
	MDELAY(120);

	//init_lcm_registers();

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
		//push_table(lcm_sleep_mode_in_setting, sizeof(lcm_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
		//SET_RESET_PIN(0);
		//MDELAY(1);
		//SET_RESET_PIN(1);
			unsigned int data_array[2];
#if 1
		//data_array[0] = 0x00000504; // Display Off
		//dsi_set_cmdq(&data_array, 1, 1);
		//MDELAY(100); 
		data_array[0] = 0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
		MDELAY(10); 
		data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
		MDELAY(100);
#endif
#ifdef BUILD_LK
		printf("zhibin uboot %s\n", __func__);
#else
		printk("zhibin kernel %s\n", __func__);
#endif

}


static void lcm_resume(void)
{
#ifdef BUILD_LK
		printf("zhibin uboot %s\n", __func__);
#else
		printk("zhibin kernel %s\n", __func__);
	
#endif
/* push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1); */
	lcm_init();
}


static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
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

#ifdef BUILD_LK
		printf("zhibin uboot %s\n", __func__);
#else
		printk("zhibin kernel %s\n", __func__);	
#endif

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);
}



LCM_DRIVER ili9881_hd720_dsi_vdo_tps65132_lcm_drv = {
	.name           = "ili9881_hd720_dsi_vdo_tps65132",
	.set_util_funcs = lcm_set_util_funcs,
	.compare_id     = lcm_compare_id,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if defined(LCM_DSI_CMD_MODE)
	.update         = lcm_update,
#endif
};
