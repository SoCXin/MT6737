#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/upmu_common.h>
	#include <platform/upmu_hw.h>

	#include <platform/mt_gpio.h>
	#include <platform/mt_i2c.h> 
	#include <platform/mt_pmic.h>
	#include <string.h>
#else
	#include <mach/mt_pm_ldo.h>	/* hwPowerOn */
	#include <mach/upmu_common.h>
	#include <mach/upmu_sw.h>
	#include <mach/upmu_hw.h>

	#include <mach/mt_gpio.h>
#endif
#include <cust_gpio_usage.h>
#ifndef CONFIG_FPGA_EARLY_PORTING
#include <cust_i2c.h>
#endif
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define MDELAY(n) 											(lcm_util.mdelay(n))
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

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>  
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
//#include <linux/jiffies.h>
#include <linux/uaccess.h>
//#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
/***************************************************************************** 
 * Define
 *****************************************************************************/
#ifndef CONFIG_FPGA_EARLY_PORTING
#define TPS_I2C_BUSNUM  I2C_I2C_LCD_BIAS_CHANNEL//for I2C channel 0
#define I2C_ID_NAME "tps65132"
#define TPS_ADDR 0x3E
/***************************************************************************** 
 * GLobal Variable
 *****************************************************************************/
static struct i2c_board_info __initdata tps65132_board_info = {I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR)};
static struct i2c_client *tps65132_i2c_client = NULL;


/***************************************************************************** 
 * Function Prototype
 *****************************************************************************/ 
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tps65132_remove(struct i2c_client *client);
/***************************************************************************** 
 * Data Structure
 *****************************************************************************/

 struct tps65132_dev	{	
	struct i2c_client	*client;
	
};

static const struct i2c_device_id tps65132_id[] = {
	{ I2C_ID_NAME, 0 },
	{ }
};

//#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
//static struct i2c_client_address_data addr_data = { .forces = forces,};
//#endif
static struct i2c_driver tps65132_iic_driver = {
	.id_table	= tps65132_id,
	.probe		= tps65132_probe,
	.remove		= tps65132_remove,
	//.detect		= mt6605_detect,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "tps65132",
	},
 
};
/***************************************************************************** 
 * Extern Area
 *****************************************************************************/ 
 
 

/***************************************************************************** 
 * Function
 *****************************************************************************/ 
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id)
{  
	printk( "*********hx8394d tps65132_iic_probe\n");
	printk("*********hx8394d TPS: info==>name=%s addr=0x%x\n",client->name,client->addr);
	tps65132_i2c_client  = client;		
	return 0;      
}


static int tps65132_remove(struct i2c_client *client)
{  	
  printk( "*********hx8394d tps65132_remove\n");
  tps65132_i2c_client = NULL;
   i2c_unregister_device(client);
  return 0;
}


 static int tps65132_write_bytes(unsigned char addr, unsigned char value)
{	
	int ret = 0;
	struct i2c_client *client = tps65132_i2c_client;
	char write_data[2]={0};	
	write_data[0]= addr;
	write_data[1] = value;
    ret=i2c_master_send(client, write_data, 2);
	if(ret<0)
	printk("*********hx8394d tps65132 write data fail !!\n");	
	return ret ;
}



/*
 * module load/unload record keeping
 */

static int __init tps65132_iic_init(void)
{

   printk( "*********hx8394d tps65132_iic_init\n");
   i2c_register_board_info(TPS_I2C_BUSNUM, &tps65132_board_info, 1);
   printk( "*********hx8394d tps65132_iic_init2\n");
   i2c_add_driver(&tps65132_iic_driver);
   printk( "*********hx8394d tps65132_iic_init success\n");	
   return 0;
}

static void __exit tps65132_iic_exit(void)
{
  printk( "*********hx8394d tps65132_iic_exit\n");
  i2c_del_driver(&tps65132_iic_driver);  
}


module_init(tps65132_iic_init);
module_exit(tps65132_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK TPS65132 I2C Driver");
MODULE_LICENSE("GPL"); 
#endif
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE  0

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)
#ifndef CONFIG_FPGA_EARLY_PORTING
#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN
#endif

#define REGFLAG_DELAY             								0xFC
#define REGFLAG_UDELAY             								0xFB

#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER
#define REGFLAG_RESET_LOW       								0xFE
#define REGFLAG_RESET_HIGH      								0xFF


#define HX8394D_HD720_ID  (0x94)

static LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#ifndef BUILD_LK
static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
#endif
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------



#define _LCM_DEBUG_

#ifdef BUILD_LK
#define printk printf
#endif

#ifdef _LCM_DEBUG_
#define lcm_debug(fmt, args...) printk(fmt, ##args)
#else
#define lcm_debug(fmt, args...) do { } while (0)
#endif

#ifdef _LCM_INFO_
#define lcm_info(fmt, args...) printk(fmt, ##args)
#else
#define lcm_info(fmt, args...) do { } while (0)
#endif
#define lcm_err(fmt, args...) printk(fmt, ##args)

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28,0,{}},
	{0x10,0,{}},
	{REGFLAG_DELAY, 120, {}},
	{0x4F,1,{0x01}},
	{REGFLAG_DELAY, 120, {}}
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

	// Set EXTC
	{0xB9,  3 ,{0xFF, 0x83, 0x94}},

	// Set B0
	{0xB0,  4 ,{0x00, 0x00, 0x7D, 0x0C}},

	// Set MIPI
	{0xBA,  13 ,{0x33, 0x83, 0xA0, 0x6D, 
					0xB2, 0x00, 0x00, 0x40, 
					0x10, 0xFF, 0x0F, 0x00, 
					0x80}},

	// Set Power
	{0xB1,  15 ,{0x64, 0x15, 0x15, 0x34, 
					0x04, 0x11, 0xF1, 0x81, 
					0x76, 0x54, 0x23, 0x80, 
					0xC0, 0xD2, 0x5E}},

	// Set Display
	{0xB2,  15 ,{0x00, 0x64, 0x0E, 0x0D, 
					0x32, 0x1C, 0x08, 0x08, 
					0x1C, 0x4D, 0x00, 0x00, 
					0x30, 0x44, 0x24}},

	// Set CYC
	{0xB4,  22 ,{0x00, 0xFF, 0x03, 0x5A, 
					0x03, 0x5A, 0x03, 0x5A, 
					0x01, 0x70, 0x01, 0x70, 
					0x03, 0x5A, 0x03, 0x5A,  
					0x03, 0x5A, 0x01, 0x70, 
					0x1F, 0x70}},

	// Set D3
	{0xD3,  52 ,{0x00, 0x07, 0x00, 0x3C, 
					0x07, 0x10, 0x00, 0x08, 
					0x10, 0x09, 0x00, 0x09,
					0x54, 0x15, 0x0F, 0x05, 
					0x04, 0x02, 0x12, 0x10, 
					0x05, 0x07, 0x37, 0x33, 
					0x0B, 0x0B, 0x3B, 0x10, 
					0x07, 0x07, 0x08, 0x00, 
					0x00, 0x00, 0x0A, 0x00, 
					0x01, 0x00, 0x00, 0x00, 
					0x00, 0x00, 0x00, 0x00, 
					0x09, 0x05, 0x04, 0x02, 
					0x10, 0x0B, 0x10, 0x00}},

	// Set GIP
	{0xD5,  44 ,{0x1A, 0x1A, 0x1B, 0x1B,
					0x00, 0x01, 0x02, 0x03, 
					0x04, 0x05, 0x06, 0x07, 
					0x08, 0x09, 0x0A, 0x0B, 
					0x24, 0x25, 0x18, 0x18, 
					0x26, 0x27, 0x18, 0x18, 
					0x18, 0x18, 0x18, 0x18, 
					0x18, 0x18, 0x18, 0x18, 
					0x18, 0x18, 0x18, 0x18, 
					0x18, 0x18, 0x20, 0x21, 
					0x18, 0x18, 0x18, 0x18}},

		
		
	// Set D6
	{0xD6,  44 ,{0x1A, 0x1A, 0x1B, 0x1B,
					0x0B, 0x0A, 0x09, 0x08, 
					0x07, 0x06, 0x05, 0x04, 
					0x03, 0x02, 0x01, 0x00, 
					0x21, 0x20, 0x58, 0x58, 
					0x27, 0x26, 0x18, 0x18, 
					0x18, 0x18, 0x18, 0x18, 
					0x18, 0x18, 0x18, 0x18, 
					0x18, 0x18, 0x18, 0x18, 
					0x18, 0x18, 0x25, 0x24, 
					0x18, 0x18, 0x18, 0x18}},
	
	// Set Panel
	{0xCC,  1 ,{0x09}},
	
	// Set TCON Option
	{0xC7,  4 ,{0x00, 0x00, 0x00, 0xC0}},
	
	// Set BD
	{0xBD,  1 ,{0x00}},
	
	// Set D2
	{0xD2,  1 ,{0x66}},
	
	// Set D8
	{0xD8,  24 ,{0xFF, 0xFF, 0xFF, 0xFF, 
					0xFF, 0xF0, 0xFA, 0xAA, 
					0xAA, 0xAA, 0xAA, 0xA0, 
					0xAA, 0xAA, 0xAA, 0xAA, 
					0xAA, 0xA0, 0xAA, 0xAA, 
					0xAA, 0xAA, 0xAA, 0xA0}},
	
	// Set BD
	{0xBD,  1 ,{0x01}},
	
	// Set D8
	{0xD8,  36 ,{0xAA, 0xAA, 0xAA, 0xAA, 
					0xAA, 0xA0, 0xAA, 0xAA, 
					0xAA, 0xAA, 0xAA, 0xA0, 
					0xFF, 0xFF, 0xFF, 0xFF, 
					0xFF, 0xF0, 0xFF, 0xFF, 
					0xFF, 0xFF, 0xFF, 0xF0, 
					0x00, 0x00, 0x00, 0x00, 
					0x00, 0x00, 0x00, 0x00, 
					0x00, 0x00, 0x00, 0x00}},
	
	// Set BD
	{0xBD,  1 ,{0x02}},
	
	// Set D8
	{0xD8,  12 ,{0xFF, 0xFF, 0xFF, 0xFF, 
					0xFF, 0xF0, 0xFF, 0xFF, 
					0xFF, 0xFF, 0xFF, 0xF0}},
	
	// Set BD
	{0xBD,  1 ,{0x00}},
	
	// Set BE
	{0xBE,  2 ,{0x01, 0xE5}},
	
	// Set Power Option
	{0xBF,  3 ,{0x41, 0x0E, 0x01}},
	
	// Set D9
	{0xD9,  3 ,{0x04, 0x01, 0x01}},
	
	// Sleep Out
	{0x11,0,{}},
	{REGFLAG_DELAY, 120, {}},

	// Display ON
	{0x29,0,{}},
	{REGFLAG_DELAY, 20,{}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void init_lcm_registers(void)
{
	 unsigned int data_array[16];


	data_array[0]=0x00043902;
	data_array[1]=0x9483FFB9;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(1);
	
	data_array[0]=0x00033902;
	data_array[1]=0x008373BA;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(1);

	//BAh,1st para=73,2nd para=43,7th para=09,8th para=40,9th para=10,10th para=00,11th para=00
	data_array[0]=0x00103902;
	data_array[1]=0x110B6CB1;
	data_array[2]=0xF1110437;//vgl=-5.55*2
	data_array[3]=0x2395E380;
	data_array[4]=0x18D2C080;
	dsi_set_cmdq(data_array, 5, 1);
	MDELAY(10);
	
	data_array[0]=0x000C3902;
	data_array[1]=0x0E6400B2;
	data_array[2]=0x0823320D;
        data_array[3]=0x004D1C08;
	dsi_set_cmdq(data_array, 4, 1);
	MDELAY(1);
	
		
	data_array[0]=0x000D3902;
	data_array[1]=0x03FF00B4;
	data_array[2]=0x03500350;
	data_array[3]=0x016A0150;
	data_array[4]=0x0000006A;
	dsi_set_cmdq(data_array, 5, 1);
	MDELAY(1);
	
	data_array[0]=0x00043902;
	data_array[1]=0x010E41BF;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(1);

	data_array[0]=0x00263902;
	data_array[1]=0x000700D3;
	data_array[2]=0x00100000;
	data_array[3]=0x00051032;
	data_array[4]=0x00103200;
	data_array[5]=0x10320000;
	data_array[6]=0x36000000;
	data_array[7]=0x37090903;   
	data_array[8]=0x00370000;
	data_array[9]=0x0A000000;
	data_array[10]=0x00000100;
	dsi_set_cmdq(data_array, 11, 1);
	MDELAY(1);
	

	
	data_array[0]=0x002D3902;
	data_array[1]=0x000302D5;
	data_array[2]=0x04070601;
	data_array[3]=0x22212005;
	data_array[4]=0x18181823;
	data_array[5]=0x18181818;
	data_array[6]=0x18181818;
	data_array[7]=0x18181818;   
	data_array[8]=0x18181818;
	data_array[9]=0x18181818;
	data_array[10]=0x24181818;
	data_array[11]=0x19181825;
	data_array[12]=0x00000019;
	dsi_set_cmdq(data_array, 13, 1);
	MDELAY(1);


	data_array[0]=0x002D3902;
	data_array[1]=0x070405D6;
	data_array[2]=0x03000106;
	data_array[3]=0x21222302;
	data_array[4]=0x18181820;
	data_array[5]=0x58181818;
	data_array[6]=0x18181858;
	data_array[7]=0x18181818;   
	data_array[8]=0x18181818;
	data_array[9]=0x18181818;
	data_array[10]=0x25181818;
	data_array[11]=0x18191924;
	data_array[12]=0x00000018;
	dsi_set_cmdq(data_array, 13, 1);
	MDELAY(1);

	data_array[0]=0x002B3902;
	data_array[1]=0x211C07E0;
	data_array[2]=0x2D3F3E38;//	data_array[2]=0x1A332924;
	data_array[3]=0x0C0B064B;//	data_array[3]=0x0D0B083C;
	data_array[4]=0x15110F17;//	data_array[4]=0x15120F17;
	data_array[5]=0x12061413;
	data_array[6]=0x1C071814;
	data_array[7]=0x3F3E3922;   
	data_array[8]=0x0A064B2D;//	data_array[8]=0x0B083C1A;
	data_array[9]=0x110F170D;//	data_array[9]=0x120F170D;
	data_array[10]=0x07141315;//	data_array[10]=0x08141315;
	data_array[11]=0x00181412;
	dsi_set_cmdq(data_array, 12, 1);
	MDELAY(1);

	data_array[0]=0x00023902;
	data_array[1]=0x000009CC;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(1);

	data_array[0]=0x00053902;
	data_array[1]=0x40C000C7;
	data_array[2]=0x000000C0;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(10);
	
	//data_array[0]=0x00033902;
    //data_array[1]=0x007F69b6;//VCOM
	//dsi_set_cmdq(data_array, 2, 1);
	//MDELAY(1);

	data_array[0]=0x00033902;
        data_array[1]=0x001430C0;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(1);

	data_array[0]=0x00023902;
        data_array[1]=0x000007BC;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(1);

	data_array[0]=0x00043902;
        data_array[1]=0x14001FC9;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(1);
	
	data_array[0]=0x00352300;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);

	data_array[0]=0x00512300;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(1);

	data_array[0]=0x24532300;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(1);

	data_array[0]=0x01552300;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

	data_array[0]=0x00033902;
	data_array[1]=0x000111E4;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(1);
	
	data_array[0]= 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(200);
	
	data_array[0]= 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);   
 
}

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 0, {0x00}},
    {REGFLAG_DELAY, 150, {}},

    // Display ON
	{0x29, 0, {0x00}},
	{REGFLAG_DELAY, 20, {}},
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_in_setting[] = {
	// Display off sequence
	{0x28, 0, {0x00}},
	 {REGFLAG_DELAY, 10, {}},
	
      // Sleep Mode On
	{0x10, 0, {0x00}},
         {REGFLAG_DELAY, 150, {}},

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
				
			case REGFLAG_UDELAY :
				UDELAY(table[i].count);
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

#if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
#else
		params->dsi.mode   = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
#endif
	
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
		params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch					= 12;
		params->dsi.vertical_frontporch					= 16;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 24;
		params->dsi.horizontal_backporch				= 100;
		params->dsi.horizontal_frontporch				= 50;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	    //params->dsi.LPX=8; 

		// Bit rate calculation
		params->dsi.PLL_CLOCK = 240;

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x53;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x24;

}

#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef BUILD_LK

#define TPS65132_SLAVE_ADDR_WRITE  0x7C  
static struct mt_i2c_t TPS65132_i2c;

static int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    TPS65132_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL;//I2C2;
    /* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
    TPS65132_i2c.addr = (TPS65132_SLAVE_ADDR_WRITE >> 1);
    TPS65132_i2c.mode = ST_MODE;
    TPS65132_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&TPS65132_i2c, write_data, len);
    //printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

    return ret_code;
}

#else
  
//	extern int mt8193_i2c_write(u16 addr, u32 data);
//	extern int mt8193_i2c_read(u16 addr, u32 *data);
	
//	#define TPS65132_write_byte(add, data)  mt8193_i2c_write(add, data)
//	#define TPS65132_read_byte(add)  mt8193_i2c_read(add)
  
#endif
#endif


static void lcm_init_power(void)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef BUILD_LK
	pmic_set_register_value(PMIC_RG_VGP1_EN,1);
#else
	printk("%s, begin\n", __func__);
	hwPowerOn(MT6328_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");	
	printk("%s, end\n", __func__);
#endif
#endif
}

static void lcm_suspend_power(void)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef BUILD_LK
	pmic_set_register_value(PMIC_RG_VGP1_EN,0);
#else
	printk("%s, begin\n", __func__);
	hwPowerDown(MT6328_POWER_LDO_VGP1, "LCM_DRV");	
	printk("%s, end\n", __func__);
#endif
#endif
}

static void lcm_resume_power(void)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef BUILD_LK
	pmic_set_register_value(PMIC_RG_VGP1_EN,1);
#else
	printk("%s, begin\n", __func__);
	hwPowerOn(MT6328_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");	
	printk("%s, end\n", __func__);
#endif
#endif
}


static void lcm_init(void)
{
	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret=0;
	cmd=0x00;
	data=0x0A;
//data=0x0A;VSP=5V,//data=0x0E;VSP=5.4V

#ifndef CONFIG_FPGA_EARLY_PORTING
	mt_set_gpio_mode(GPIO_65132_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_EN, GPIO_OUT_ONE);
	MDELAY(10);

#ifdef BUILD_LK
	ret=TPS65132_write_byte(cmd,data);
    if(ret)    	
    dprintf(0, "[LK]TM050-----tps6132----cmd=%0x--i2c write error----\n",cmd);    	
	else
	dprintf(0, "[LK]TM050----tps6132----cmd=%0x--i2c write success----\n",cmd);    		
#else
	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
	printk("[KERNEL]TM050-----tps6132---cmd=%0x-- i2c write error-----\n",cmd);
	else
	printk("[KERNEL]TM050-----tps6132---cmd=%0x-- i2c write success-----\n",cmd);
#endif
	
	cmd=0x01;
	data=0x0A;
	//data=0x0A;VSN=-5V//data=0x0E;VSN=-5.4V
#ifdef BUILD_LK
	ret=TPS65132_write_byte(cmd,data);
    if(ret)    	
	dprintf(0, "[LK]TM050-----tps6132----cmd=%0x--i2c write error----\n",cmd);    	
	else
	dprintf(0, "[LK]TM050----tps6132----cmd=%0x--i2c write success----\n",cmd);   
#else
	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
	printk("[KERNEL]TM050-----tps6132---cmd=%0x-- i2c write error-----\n",cmd);
	else
	printk("[KERNEL]TM050-----tps6132---cmd=%0x-- i2c write success-----\n",cmd);
#endif
#endif

	
	lcm_debug("%s %d\n", __func__,__LINE__);
		SET_RESET_PIN(0);
		MDELAY(20); 
		SET_RESET_PIN(1);
		MDELAY(20); 

        init_lcm_registers();
}

static void lcm_suspend(void)
{
	mt_set_gpio_mode(GPIO_65132_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_EN, GPIO_OUT_ZERO);
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);  
	//SET_RESET_PIN(0);
	MDELAY(10);
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

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

static unsigned int lcm_compare_id(void)
{
	char  buffer;
	unsigned int data_array[2];

	data_array[0]= 0x00043902;
	data_array[1]= (0x94<<24)|(0x83<<16)|(0xff<<8)|0xb9;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]= 0x00023902;
	data_array[1]= (0x33<<8)|0xba;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0]= 0x00043902;
	data_array[1]= (0x94<<24)|(0x83<<16)|(0xff<<8)|0xb9;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00013700;
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0xf4, &buffer, 1);

	#ifdef BUILD_LK
		printf("%s, LK debug: hx8394d id = 0x%08x\n", __func__, buffer);
    #else
		printk("%s, kernel debug: hx8394d id = 0x%08x\n", __func__, buffer);
    #endif

	return (buffer == HX8394D_HD720_ID ? 1 : 0);

}


static unsigned int lcm_esd_check(void)
{
  #ifndef BUILD_LK

	if(lcm_esd_test)
	{
		lcm_esd_test = FALSE;
		return TRUE;
	}

	char  buffer;
	read_reg_v2(0x0a, &buffer, 1);
	printk("%s, kernel debug: reg = 0x%08x\n", __func__, buffer);

	return FALSE;
	
#else
	return FALSE;
#endif

}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();

	return TRUE;
}


static void* lcm_switch_mode(int mode)
{
#ifndef BUILD_LK
//customization: 1. V2C config 2 values, C2V config 1 value; 2. config mode control register
	if(mode == 0)
	{//V2C
		lcm_switch_mode_cmd.mode = CMD_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;// mode control addr
		lcm_switch_mode_cmd.val[0]= 0x13;//enabel GRAM firstly, ensure writing one frame to GRAM
		lcm_switch_mode_cmd.val[1]= 0x10;//disable video mode secondly
	}
	else
	{//C2V
		lcm_switch_mode_cmd.mode = SYNC_PULSE_VDO_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;
		lcm_switch_mode_cmd.val[0]= 0x03;//disable GRAM and enable video mode
	}
	return (void*)(&lcm_switch_mode_cmd);
#else
	return NULL;
#endif
}



LCM_DRIVER hx8394d_hd720_dsi_vdo_tianma_lcm_drv = 
{
    .name			= "hx8394d_hd720_dsi_tianma_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
     .init_power		= lcm_init_power,
     .resume_power = lcm_resume_power,
     .suspend_power = lcm_suspend_power,
	//.esd_check 	= lcm_esd_check,
	//.esd_recover	= lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
};
