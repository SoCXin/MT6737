/*****************************************************************************
 *
 * Filename:
 * ---------
 *     OV5693mipi_Sensor.c
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
//#include <asm/system.h>
//#include <linux/xlog.h>

#include "kd_camera_typedef.h"
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k3l2mipiraw_Sensor.h"

/****************************Modify Following Strings for Debug****************************/
#define PFX "S5K3L2_camera_sensor"
//#define LOG_1 //LOG_INF("S5K3L2,MIPI 4LANE\n")
//#define LOG_2 //LOG_INF("preview 2096*1554@30fps,864Mbps/lane; video 4196*3108@30fps,864Mbps/lane; capture 13M@30fps,864Mbps/lane\n")
/****************************   Modify end    *******************************************/

//#define //LOG_INF(format, args...)    xlog_printk(ANDROID_//LOG_INFO   , PFX, "[%s] " format, __FUNCTION__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);


static imgsensor_info_struct imgsensor_info = {
    .sensor_id = S5K3L2YX_SENSOR_ID,        //record sensor id defined in Kd_imgsensor.h

	.checksum_value = 0x752cfcc1,	//0xc3dd012b,

    .pre = {
        .pclk = 432000000,              
        .linelength = 5504,            
        .framelength = 2616,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 2096,
		.grabwindow_height = 1552,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300, 
    },
    .cap = {
        .pclk = 432000000,
        .linelength = 4512,
        .framelength = 3180,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 4192,
        .grabwindow_height = 3104,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300,
    },
    .cap1 = {
        .pclk = 432000000,
        .linelength = 4512,
        .framelength = 3180,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 4192,
        .grabwindow_height = 3104,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300,
    },
    .normal_video = {
        .pclk = 432000000,              
        .linelength = 5504,            
        .framelength = 2616,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 2096,
				.grabwindow_height = 1552,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300, 
    },
    .hs_video = {
        .pclk = 448000000,
        .linelength = 4560,
        .framelength = 818,
        .startx = 4,
        .starty = 4,
        .grabwindow_width = 640,
        .grabwindow_height = 480,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 1200,
    },
    .slim_video = {
        .pclk = 432000000,              
        .linelength = 5504,            
        .framelength = 2616,
        .startx = 0,
        .starty = 0,
        .grabwindow_width = 2096,
				.grabwindow_height = 1552,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300, 
    },
    .margin = 16,            //sensor framelength & shutter margin
    .min_shutter = 3,        //min shutter
    .max_frame_length = 0xffff,//max framelength by sensor register's limitation
    .ae_shut_delay_frame = 0,    //shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
    .ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
    .ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
    .ihdr_support = 0,      //1, support; 0,not support
    .ihdr_le_firstline = 0,  //1,le first ; 0, se first
    .sensor_mode_num = 5,      //support sensor mode num

    .cap_delay_frame = 3,        //enter capture delay frame num
    .pre_delay_frame = 3,         //enter preview delay frame num
    .video_delay_frame = 2,        //enter video delay frame num
    .hs_video_delay_frame = 3,    //enter high speed video  delay frame num
    .slim_video_delay_frame = 3,//enter slim video delay frame num

    .isp_driving_current = ISP_DRIVING_8MA,
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
    .sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,	//Gr,
    .mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
    .mipi_lane_num = SENSOR_MIPI_4_LANE,
    .i2c_addr_table = {0x20, 0x5a, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
    .i2c_speed = 300, 
};


static imgsensor_struct imgsensor = {
    .mirror = IMAGE_NORMAL,                //mirrorflip information
    .sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
    .shutter = 0x3D0,                    //current shutter
    .gain = 0x100,                        //current gain
    .dummy_pixel = 0,                    //current dummypixel
    .dummy_line = 0,                    //current dummyline
    .current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
    .autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
    .test_pattern = KAL_FALSE,        //test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
    .current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
    .ihdr_en = 0, //sensor need support LE, SE with HDR feature
    .i2c_write_id = 0x20,//record current sensor's i2c write id
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{{ 4192, 3104, 0000, 0000, 4192, 3104, 2096, 1552, 0000, 0000, 2096, 1552, 000, 0000, 2096, 1552}, // Preview 
 { 4192, 3104, 0000, 0000, 4192, 3104, 4192, 3104, 0000, 0000, 4192, 3104, 000, 0000, 4192, 3104}, // capture 
 { 4192, 3104, 0000, 0000, 4192, 3104, 2096, 1552, 0000, 0000, 2096, 1552, 000, 0000, 2096, 1552}, // video 
 { 4192, 3104,  174,  108, 3888, 2928, 648,   488, 0000, 0000, 648,  488,  004, 0004, 640,  480}, //hight speed video 
 { 4192, 3104,  000,  000, 4192, 3104, 2096, 1552, 0000, 0000, 2096, 1552, 000, 0000, 2096, 1552}
};
static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char puSendCmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF)};
		
	kdSetI2CSpeed(imgsensor_info.i2c_speed);
	iReadRegI2C(puSendCmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);
		
	return get_byte&0x00ff;
}


static void write_cmos_sensor1(kal_uint32 addr, kal_uint32 para)
{
	char puSendCmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF),  (char)(para >> 8), (char)(para & 0xFF)};
	
	kdSetI2CSpeed(imgsensor_info.i2c_speed);
	iWriteRegI2C(puSendCmd, 4, imgsensor.i2c_write_id);
}



static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char puSendCmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
	
	kdSetI2CSpeed(imgsensor_info.i2c_speed);
	iWriteRegI2C(puSendCmd, 3, imgsensor.i2c_write_id);
}



static void check_output_stream_on(void)
{
	kal_uint16 read_count = 0, read_register0005_value = 0;
	
	for(read_count = 0; read_count <= 4; read_count++)
	{		
		read_register0005_value = read_cmos_sensor(0x0005);				

		if(read_register0005_value != 0xff)			
			break;		
		
		mdelay(50);
		
		if(read_count == 4)			
			printk("[error][0x0005, 0xff]sensor not output\n");	
	}
}



static void check_output_stream_off(void)
{
	kal_uint16 read_count = 0, read_register0005_value = 0;
	
	for(read_count = 0; read_count <= 4; read_count++)
	{		
		read_register0005_value = read_cmos_sensor(0x0005);			

		if(read_register0005_value == 0xff)			
			break;		
		mdelay(50);
		
		if(read_count == 4)			
			printk("stream off error\n");	
	}
}


static void set_dummy(void)
{
    //LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
    /* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel, or you can set dummy by imgsensor.frame_length and imgsensor.line_length */

	write_cmos_sensor(0x0104, 0x01);		
	write_cmos_sensor1(0x0340, imgsensor.frame_length);	
	write_cmos_sensor1(0x0342, imgsensor.line_length);	
	write_cmos_sensor(0x0104, 0x00);
}    /*    set_dummy  */

static kal_uint32 return_sensor_id(void)
{
	//while(1)
	//{
	//	read_cmos_sensor(0x0000);
	//	read_cmos_sensor(0x0001);
	//	mdelay(100);
	//}
    return ((read_cmos_sensor(0x0000)<<8)|read_cmos_sensor(0x0001));
    //return 0x30c2;
}
static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
    //kal_int16 dummy_line;
    kal_uint32 frame_length = imgsensor.frame_length;
    //unsigned long flags;

    //LOG_INF("framerate = %d, min framelength should enable? \n", framerate,min_framelength_en);

    frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
    spin_lock(&imgsensor_drv_lock);
    imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
    imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
    //dummy_line = frame_length - imgsensor.min_frame_length;
    //if (dummy_line < 0)
        //imgsensor.dummy_line = 0;
    //else
        //imgsensor.dummy_line = dummy_line;
    //imgsensor.frame_length = frame_length + imgsensor.dummy_line;
    if (imgsensor.frame_length > imgsensor_info.max_frame_length)
    {
        imgsensor.frame_length = imgsensor_info.max_frame_length;
        imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
    }
    if (min_framelength_en)
        imgsensor.min_frame_length = imgsensor.frame_length;
    spin_unlock(&imgsensor_drv_lock);
    set_dummy();
}    /*    set_max_framerate  */



/*************************************************************************
* FUNCTION
*    set_shutter
*
* DESCRIPTION
*    This function set e-shutter of sensor to change exposure time.
*
* PARAMETERS
*    iShutter : exposured lines
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
	kal_uint16 frame_length = 0, line_length = 0, framerate = 0 , frame_length_min = 0, read_register0005_value = 0;
	kal_uint32 pixelclock = 0;
	
	#define SHUTTER_FRAMELENGTH_MARGIN 16
		
		if (shutter < 3)
			shutter = 3;
	
		if(imgsensor.sensor_mode == IMGSENSOR_MODE_HIGH_SPEED_VIDEO)
		{
			if(shutter > imgsensor.min_frame_length - SHUTTER_FRAMELENGTH_MARGIN)
				shutter = imgsensor.min_frame_length - SHUTTER_FRAMELENGTH_MARGIN;
	
			write_cmos_sensor(0x0104, 0x01);	
			write_cmos_sensor1(0x0202, shutter);
			write_cmos_sensor(0x0104, 0x00);
			return;
		}
		
		frame_length = shutter + SHUTTER_FRAMELENGTH_MARGIN; 
		frame_length_min = imgsensor.min_frame_length;
	
		if(frame_length < frame_length_min)
			frame_length = frame_length_min;
		
		if(imgsensor.autoflicker_en == KAL_TRUE)
		{
			line_length = imgsensor.line_length;
			pixelclock = imgsensor.pclk;
			framerate = (10 * pixelclock) / (frame_length * line_length);
			  
			if(framerate > 290)
			{
				framerate = 290;
				frame_length = (10 * pixelclock) / (framerate * line_length);
			}
			else if(framerate > 147 && framerate < 152)
			{
				framerate = 147;
				frame_length = (10 * pixelclock) / (framerate * line_length);
			}
		}
	
		write_cmos_sensor(0x0104, 0x01);	
		write_cmos_sensor1(0x0340, frame_length);
		write_cmos_sensor1(0x0202, shutter);
		write_cmos_sensor(0x0104, 0x00);	 
		
		read_register0005_value = read_cmos_sensor(0x0005);
		if(read_register0005_value == 0xff)
			printk("[error][0x0005, 0xff]sensor not output\n");
		
		//LOG_INF("shutter=%d, frame_length=%d, frame_length_min=%d, framerate=%d\n", shutter,frame_length, frame_length_min, framerate);
	}



#if 0
static kal_uint16 gain2reg(const kal_uint16 gain)
{
}
#endif
/*************************************************************************
* FUNCTION
*    set_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    iGain : sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{
	gain = gain / 2;		
	write_cmos_sensor(0x0104, 0x01);		
	write_cmos_sensor(0x0204,(gain>>8));	
	write_cmos_sensor(0x0205,(gain&0xff));	
	write_cmos_sensor(0x0104, 0x00);
    return gain;
}    /*    set_gain  */

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
}


#if 0
static void set_mirror_flip(kal_uint8 image_mirror)
{
    switch (image_mirror) {
       case IMAGE_NORMAL:            
	   	write_cmos_sensor(0x0101, 0x03);            
		break;        
		case IMAGE_V_MIRROR:             
			write_cmos_sensor(0x0101, 0x02);	            
			break;        
			case IMAGE_H_MIRROR:             
				write_cmos_sensor(0x0101, 0x01);	            
				break;        
				case IMAGE_HV_MIRROR:             
					write_cmos_sensor(0x0101, 0x00);	            
					break;
        default:
            printk("Error image_mirror setting\n");
    }

}
#endif
/*************************************************************************
* FUNCTION
*    night_mode
*
* DESCRIPTION
*    This function night mode of sensor.
*
* PARAMETERS
*    bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}    /*    night_mode    */

static void sensor_init(void)
{
	write_cmos_sensor1(0x6028, 0xD000);
	write_cmos_sensor1(0x602A, 0x6010);
	write_cmos_sensor1(0x6F12, 0x0001);
	mDELAY(3);

	write_cmos_sensor(0x0101, 0x00);  //mirror  
  write_cmos_sensor1(0x6028, 0xD000);
	write_cmos_sensor1(0x602A, 0x6214);
	write_cmos_sensor1(0x6F12, 0x7970);
	write_cmos_sensor1(0x602A, 0x6218);
	write_cmos_sensor1(0x6F12, 0x7150);
	write_cmos_sensor1(0x6028, 0x7000);
	write_cmos_sensor1(0x602A, 0x2200);
	write_cmos_sensor1(0x6F12, 0x2DE9);
	write_cmos_sensor1(0x6F12, 0x7040);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x9010);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x9000);
	write_cmos_sensor1(0x6F12, 0x80E5);
	write_cmos_sensor1(0x6F12, 0x0010);
	write_cmos_sensor1(0x6F12, 0x90E5);
	write_cmos_sensor1(0x6F12, 0x2C20);
	write_cmos_sensor1(0x6F12, 0x42E0);
	write_cmos_sensor1(0x6F12, 0x0110);
	write_cmos_sensor1(0x6F12, 0xC0E1);
	write_cmos_sensor1(0x6F12, 0xB410);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x8010);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x8000);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x2301);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x7C50);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x7C10);
	write_cmos_sensor1(0x6F12, 0xC5E1);
	write_cmos_sensor1(0x6F12, 0xB000);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x7800);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x2001);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x7400);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x7410);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x81E5);
	write_cmos_sensor1(0x6F12, 0x1000);
	write_cmos_sensor1(0x6F12, 0x85E2);
	write_cmos_sensor1(0x6F12, 0x0400);
	write_cmos_sensor1(0x6F12, 0xC0E1);
	write_cmos_sensor1(0x6F12, 0xB240);
	write_cmos_sensor1(0x6F12, 0xC0E1);
	write_cmos_sensor1(0x6F12, 0xB040);
	write_cmos_sensor1(0x6F12, 0xC0E1);
	write_cmos_sensor1(0x6F12, 0xB440);
	write_cmos_sensor1(0x6F12, 0xC0E1);
	write_cmos_sensor1(0x6F12, 0xB640);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x5800);
	write_cmos_sensor1(0x6F12, 0x81E5);
	write_cmos_sensor1(0x6F12, 0x2400);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x5410);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x5400);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x1001);
	write_cmos_sensor1(0x6F12, 0xC5E1);
	write_cmos_sensor1(0x6F12, 0xB200);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x85E2);
	write_cmos_sensor1(0x6F12, 0x0C10);
	write_cmos_sensor1(0x6F12, 0x81E0);
	write_cmos_sensor1(0x6F12, 0x8020);
	write_cmos_sensor1(0x6F12, 0x80E2);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x50E3);
	write_cmos_sensor1(0x6F12, 0x0A00);
	write_cmos_sensor1(0x6F12, 0xC2E5);
	write_cmos_sensor1(0x6F12, 0xA540);
	write_cmos_sensor1(0x6F12, 0xFF3A);
	write_cmos_sensor1(0x6F12, 0xFAFF);
	write_cmos_sensor1(0x6F12, 0xBDE8);
	write_cmos_sensor1(0x6F12, 0x7040);
	write_cmos_sensor1(0x6F12, 0x2FE1);
	write_cmos_sensor1(0x6F12, 0x1EFF);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x0C28);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x6018);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x2426);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xA436);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x4827);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0xCC25);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x08C5);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x7024);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0xF004);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0xDC23);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0xCC22);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x7CBC);
	write_cmos_sensor1(0x6F12, 0x2DE9);
	write_cmos_sensor1(0x6F12, 0x7040);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0xA043);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0010);
	write_cmos_sensor1(0x6F12, 0xD4E1);
	write_cmos_sensor1(0x6F12, 0xB200);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0xF900);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x9403);
	write_cmos_sensor1(0x6F12, 0x80E2);
	write_cmos_sensor1(0x6F12, 0x522F);
	write_cmos_sensor1(0x6F12, 0x80E2);
	write_cmos_sensor1(0x6F12, 0x5010);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x8C03);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0xF600);
	write_cmos_sensor1(0x6F12, 0x84E2);
	write_cmos_sensor1(0x6F12, 0x0C50);
	write_cmos_sensor1(0x6F12, 0xD5E1);
	write_cmos_sensor1(0x6F12, 0xB000);
	write_cmos_sensor1(0x6F12, 0x50E3);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x001A);
	write_cmos_sensor1(0x6F12, 0x3100);
	write_cmos_sensor1(0x6F12, 0xD5E1);
	write_cmos_sensor1(0x6F12, 0xB2E0);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x7403);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xB420);
	write_cmos_sensor1(0x6F12, 0x5EE3);
	write_cmos_sensor1(0x6F12, 0x0A00);
	write_cmos_sensor1(0x6F12, 0xA083);
	write_cmos_sensor1(0x6F12, 0x0AE0);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x00EA);
	write_cmos_sensor1(0x6F12, 0x2400);
	write_cmos_sensor1(0x6F12, 0xD1E1);
	write_cmos_sensor1(0x6F12, 0xB430);
	write_cmos_sensor1(0x6F12, 0x52E1);
	write_cmos_sensor1(0x6F12, 0x8301);
	write_cmos_sensor1(0x6F12, 0xA093);
	write_cmos_sensor1(0x6F12, 0x0030);
	write_cmos_sensor1(0x6F12, 0x009A);
	write_cmos_sensor1(0x6F12, 0x0C00);
	write_cmos_sensor1(0x6F12, 0xD1E1);
	write_cmos_sensor1(0x6F12, 0xB630);
	write_cmos_sensor1(0x6F12, 0x52E1);
	write_cmos_sensor1(0x6F12, 0x8301);
	write_cmos_sensor1(0x6F12, 0x002A);
	write_cmos_sensor1(0x6F12, 0x0500);
	write_cmos_sensor1(0x6F12, 0x85E0);
	write_cmos_sensor1(0x6F12, 0x80C0);
	write_cmos_sensor1(0x6F12, 0xDCE5);
	write_cmos_sensor1(0x6F12, 0xA530);
	write_cmos_sensor1(0x6F12, 0x53E3);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x001A);
	write_cmos_sensor1(0x6F12, 0x0F00);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0130);
	write_cmos_sensor1(0x6F12, 0x00EA);
	write_cmos_sensor1(0x6F12, 0x0D00);
	write_cmos_sensor1(0x6F12, 0xD1E1);
	write_cmos_sensor1(0x6F12, 0xB830);
	write_cmos_sensor1(0x6F12, 0x52E1);
	write_cmos_sensor1(0x6F12, 0x8301);
	write_cmos_sensor1(0x6F12, 0x008A);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0130);
	write_cmos_sensor1(0x6F12, 0x85E0);
	write_cmos_sensor1(0x6F12, 0x80C0);
	write_cmos_sensor1(0x6F12, 0x00EA);
	write_cmos_sensor1(0x6F12, 0x0700);
	write_cmos_sensor1(0x6F12, 0xD1E1);
	write_cmos_sensor1(0x6F12, 0xBA30);
	write_cmos_sensor1(0x6F12, 0x52E1);
	write_cmos_sensor1(0x6F12, 0x8301);
	write_cmos_sensor1(0x6F12, 0xA023);
	write_cmos_sensor1(0x6F12, 0x0230);
	write_cmos_sensor1(0x6F12, 0xFF2A);
	write_cmos_sensor1(0x6F12, 0xF9FF);
	write_cmos_sensor1(0x6F12, 0x85E0);
	write_cmos_sensor1(0x6F12, 0x80C0);
	write_cmos_sensor1(0x6F12, 0xDCE5);
	write_cmos_sensor1(0x6F12, 0xA530);
	write_cmos_sensor1(0x6F12, 0x53E3);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xFF0A);
	write_cmos_sensor1(0x6F12, 0xEFFF);
	write_cmos_sensor1(0x6F12, 0xCCE5);
	write_cmos_sensor1(0x6F12, 0xA430);
	write_cmos_sensor1(0x6F12, 0x85E0);
	write_cmos_sensor1(0x6F12, 0x8030);
	write_cmos_sensor1(0x6F12, 0xD3E5);
	write_cmos_sensor1(0x6F12, 0xA4C0);
	write_cmos_sensor1(0x6F12, 0xC3E5);
	write_cmos_sensor1(0x6F12, 0xA5C0);
	write_cmos_sensor1(0x6F12, 0x81E0);
	write_cmos_sensor1(0x6F12, 0x8C30);
	write_cmos_sensor1(0x6F12, 0xD1E1);
	write_cmos_sensor1(0x6F12, 0xBC10);
	write_cmos_sensor1(0x6F12, 0xD3E1);
	write_cmos_sensor1(0x6F12, 0xBE30);
	write_cmos_sensor1(0x6F12, 0x81E2);
	write_cmos_sensor1(0x6F12, 0x0D12);
	write_cmos_sensor1(0x6F12, 0xC1E1);
	write_cmos_sensor1(0x6F12, 0xB030);
	write_cmos_sensor1(0x6F12, 0x80E2);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x50E1);
	write_cmos_sensor1(0x6F12, 0x0E00);
	write_cmos_sensor1(0x6F12, 0x002A);
	write_cmos_sensor1(0x6F12, 0x0300);
	write_cmos_sensor1(0x6F12, 0x85E0);
	write_cmos_sensor1(0x6F12, 0x0012);
	write_cmos_sensor1(0x6F12, 0xD1E1);
	write_cmos_sensor1(0x6F12, 0xBC30);
	write_cmos_sensor1(0x6F12, 0x53E3);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xFF1A);
	write_cmos_sensor1(0x6F12, 0xD4FF);
	write_cmos_sensor1(0x6F12, 0xD4E1);
	write_cmos_sensor1(0x6F12, 0xB200);
	write_cmos_sensor1(0x6F12, 0xBDE8);
	write_cmos_sensor1(0x6F12, 0x7040);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0110);
	write_cmos_sensor1(0x6F12, 0x00EA);
	write_cmos_sensor1(0x6F12, 0xBA00);
	write_cmos_sensor1(0x6F12, 0x2DE9);
	write_cmos_sensor1(0x6F12, 0x3840);
	write_cmos_sensor1(0x6F12, 0x9DE5);
	write_cmos_sensor1(0x6F12, 0x1040);
	write_cmos_sensor1(0x6F12, 0x8DE5);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0xBA00);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x9402);
	write_cmos_sensor1(0x6F12, 0xD4E1);
	write_cmos_sensor1(0x6F12, 0xB420);
	write_cmos_sensor1(0x6F12, 0x90E5);
	write_cmos_sensor1(0x6F12, 0x0030);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0010);
	write_cmos_sensor1(0x6F12, 0x83E0);
	write_cmos_sensor1(0x6F12, 0x8101);
	write_cmos_sensor1(0x6F12, 0x80E2);
	write_cmos_sensor1(0x6F12, 0x250E);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xB0C0);
	write_cmos_sensor1(0x6F12, 0x5CE1);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x00CA);
	write_cmos_sensor1(0x6F12, 0x1300);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xB2C0);
	write_cmos_sensor1(0x6F12, 0x5CE1);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x00BA);
	write_cmos_sensor1(0x6F12, 0x1000);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xF450);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xF610);
	write_cmos_sensor1(0x6F12, 0x41E0);
	write_cmos_sensor1(0x6F12, 0x0530);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xB210);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xB000);
	write_cmos_sensor1(0x6F12, 0x41E0);
	write_cmos_sensor1(0x6F12, 0x0010);
	write_cmos_sensor1(0x6F12, 0x42E0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x00E0);
	write_cmos_sensor1(0x6F12, 0x9300);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0xA700);
	write_cmos_sensor1(0x6F12, 0xD4E1);
	write_cmos_sensor1(0x6F12, 0xB610);
	write_cmos_sensor1(0x6F12, 0x80E0);
	write_cmos_sensor1(0x6F12, 0x0500);
	write_cmos_sensor1(0x6F12, 0x00E0);
	write_cmos_sensor1(0x6F12, 0x9100);
	write_cmos_sensor1(0x6F12, 0xA0E1);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0xA0E1);
	write_cmos_sensor1(0x6F12, 0x2008);
	write_cmos_sensor1(0x6F12, 0xC4E1);
	write_cmos_sensor1(0x6F12, 0xB600);
	write_cmos_sensor1(0x6F12, 0xBDE8);
	write_cmos_sensor1(0x6F12, 0x3840);
	write_cmos_sensor1(0x6F12, 0x2FE1);
	write_cmos_sensor1(0x6F12, 0x1EFF);
	write_cmos_sensor1(0x6F12, 0x81E2);
	write_cmos_sensor1(0x6F12, 0x0110);
	write_cmos_sensor1(0x6F12, 0x51E3);
	write_cmos_sensor1(0x6F12, 0x0500);
	write_cmos_sensor1(0x6F12, 0xFF3A);
	write_cmos_sensor1(0x6F12, 0xE3FF);
	write_cmos_sensor1(0x6F12, 0xFFEA);
	write_cmos_sensor1(0x6F12, 0xF9FF);
	write_cmos_sensor1(0x6F12, 0x2DE9);
	write_cmos_sensor1(0x6F12, 0x7040);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x0C52);
	write_cmos_sensor1(0x6F12, 0x95E5);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x80E2);
	write_cmos_sensor1(0x6F12, 0x020C);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xB012);
	write_cmos_sensor1(0x6F12, 0x51E3);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x9F15);
	write_cmos_sensor1(0x6F12, 0xFC11);
	write_cmos_sensor1(0x6F12, 0xD115);
	write_cmos_sensor1(0x6F12, 0x0B10);
	write_cmos_sensor1(0x6F12, 0x5113);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x000A);
	write_cmos_sensor1(0x6F12, 0x3E00);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0xF011);
	write_cmos_sensor1(0x6F12, 0xD1E1);
	write_cmos_sensor1(0x6F12, 0xB210);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xB222);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0xE841);
	write_cmos_sensor1(0x6F12, 0x51E1);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0xA023);
	write_cmos_sensor1(0x6F12, 0x0010);
	write_cmos_sensor1(0x6F12, 0xA033);
	write_cmos_sensor1(0x6F12, 0x0110);
	write_cmos_sensor1(0x6F12, 0xC4E1);
	write_cmos_sensor1(0x6F12, 0xB410);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xB402);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x8900);
	write_cmos_sensor1(0x6F12, 0xA0E1);
	write_cmos_sensor1(0x6F12, 0x0060);
	write_cmos_sensor1(0x6F12, 0x95E5);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x80E2);
	write_cmos_sensor1(0x6F12, 0x020C);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xB602);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x8400);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0xA811);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0120);
	write_cmos_sensor1(0x6F12, 0xD1E1);
	write_cmos_sensor1(0x6F12, 0xB811);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0030);
	write_cmos_sensor1(0x6F12, 0x51E1);
	write_cmos_sensor1(0x6F12, 0x0600);
	write_cmos_sensor1(0x6F12, 0xC491);
	write_cmos_sensor1(0x6F12, 0xB220);
	write_cmos_sensor1(0x6F12, 0x009A);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x51E1);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xC481);
	write_cmos_sensor1(0x6F12, 0xB230);
	write_cmos_sensor1(0x6F12, 0x95E5);
	write_cmos_sensor1(0x6F12, 0x0010);
	write_cmos_sensor1(0x6F12, 0x91E5);
	write_cmos_sensor1(0x6F12, 0x3002);
	write_cmos_sensor1(0x6F12, 0x50E3);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x001A);
	write_cmos_sensor1(0x6F12, 0x0B00);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x8D0F);
	write_cmos_sensor1(0x6F12, 0x90E1);
	write_cmos_sensor1(0x6F12, 0xB1E0);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x8001);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x80C1);
	write_cmos_sensor1(0x6F12, 0x5EE3);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xD001);
	write_cmos_sensor1(0x6F12, 0xB0E6);
	write_cmos_sensor1(0x6F12, 0xD001);
	write_cmos_sensor1(0x6F12, 0xB206);
	write_cmos_sensor1(0x6F12, 0xD011);
	write_cmos_sensor1(0x6F12, 0xB4E5);
	write_cmos_sensor1(0x6F12, 0xD011);
	write_cmos_sensor1(0x6F12, 0xB605);
	write_cmos_sensor1(0x6F12, 0x8EE1);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x8C05);
	write_cmos_sensor1(0x6F12, 0x2C00);
	write_cmos_sensor1(0x6F12, 0x8C15);
	write_cmos_sensor1(0x6F12, 0x1800);
	write_cmos_sensor1(0x6F12, 0x91E5);
	write_cmos_sensor1(0x6F12, 0x2CC2);
	write_cmos_sensor1(0x6F12, 0x5CE1);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xC4D1);
	write_cmos_sensor1(0x6F12, 0xB020);
	write_cmos_sensor1(0x6F12, 0x00DA);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x91E5);
	write_cmos_sensor1(0x6F12, 0x28C2);
	write_cmos_sensor1(0x6F12, 0x5CE1);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xC4C1);
	write_cmos_sensor1(0x6F12, 0xB030);
	write_cmos_sensor1(0x6F12, 0xD4E1);
	write_cmos_sensor1(0x6F12, 0xB400);
	write_cmos_sensor1(0x6F12, 0x50E3);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xD411);
	write_cmos_sensor1(0x6F12, 0xB200);
	write_cmos_sensor1(0x6F12, 0x5013);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xD411);
	write_cmos_sensor1(0x6F12, 0xB000);
	write_cmos_sensor1(0x6F12, 0x5013);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x9F15);
	write_cmos_sensor1(0x6F12, 0x2C01);
	write_cmos_sensor1(0x6F12, 0x9011);
	write_cmos_sensor1(0x6F12, 0xB100);
	write_cmos_sensor1(0x6F12, 0xC411);
	write_cmos_sensor1(0x6F12, 0xB600);
	write_cmos_sensor1(0x6F12, 0xD4E1);
	write_cmos_sensor1(0x6F12, 0xB600);
	write_cmos_sensor1(0x6F12, 0x50E3);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x4012);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0xC411);
	write_cmos_sensor1(0x6F12, 0xB600);
	write_cmos_sensor1(0x6F12, 0x9F15);
	write_cmos_sensor1(0x6F12, 0x1401);
	write_cmos_sensor1(0x6F12, 0xC011);
	write_cmos_sensor1(0x6F12, 0xB030);
	write_cmos_sensor1(0x6F12, 0xC011);
	write_cmos_sensor1(0x6F12, 0xB820);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x5500);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x5600);
	write_cmos_sensor1(0x6F12, 0xA0E1);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x5500);
	write_cmos_sensor1(0x6F12, 0x54E1);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x4480);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xBD88);
	write_cmos_sensor1(0x6F12, 0x7040);
	write_cmos_sensor1(0x6F12, 0xA081);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0xA081);
	write_cmos_sensor1(0x6F12, 0x2118);
	write_cmos_sensor1(0x6F12, 0xA083);
	write_cmos_sensor1(0x6F12, 0x2100);
	write_cmos_sensor1(0x6F12, 0x008A);
	write_cmos_sensor1(0x6F12, 0x5000);
	write_cmos_sensor1(0x6F12, 0xBDE8);
	write_cmos_sensor1(0x6F12, 0x7040);
	write_cmos_sensor1(0x6F12, 0x2FE1);
	write_cmos_sensor1(0x6F12, 0x1EFF);
	write_cmos_sensor1(0x6F12, 0x2DE9);
	write_cmos_sensor1(0x6F12, 0x1040);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x4E00);
	write_cmos_sensor1(0x6F12, 0x50E3);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x000B);
	write_cmos_sensor1(0x6F12, 0x4E00);
	write_cmos_sensor1(0x6F12, 0x00EA);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x4D00);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x4E00);
	write_cmos_sensor1(0x6F12, 0x50E3);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xFF0A);
	write_cmos_sensor1(0x6F12, 0xFAFF);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0xB000);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0020);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0810);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x4A00);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x8000);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0xA010);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xB020);
	write_cmos_sensor1(0x6F12, 0xC1E1);
	write_cmos_sensor1(0x6F12, 0xB421);
	write_cmos_sensor1(0x6F12, 0xD0E1);
	write_cmos_sensor1(0x6F12, 0xB200);
	write_cmos_sensor1(0x6F12, 0xC1E1);
	write_cmos_sensor1(0x6F12, 0xB801);
	write_cmos_sensor1(0x6F12, 0xBDE8);
	write_cmos_sensor1(0x6F12, 0x1040);
	write_cmos_sensor1(0x6F12, 0x2FE1);
	write_cmos_sensor1(0x6F12, 0x1EFF);
	write_cmos_sensor1(0x6F12, 0x2DE9);
	write_cmos_sensor1(0x6F12, 0x7040);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x4850);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x7440);
	write_cmos_sensor1(0x6F12, 0xD5E1);
	write_cmos_sensor1(0x6F12, 0xB000);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0010);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x2200);
	write_cmos_sensor1(0x6F12, 0xD4E1);
	write_cmos_sensor1(0x6F12, 0xB200);
	write_cmos_sensor1(0x6F12, 0xD4E5);
	write_cmos_sensor1(0x6F12, 0x4210);
	write_cmos_sensor1(0x6F12, 0x80E0);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x3A00);
	write_cmos_sensor1(0x6F12, 0xD4E1);
	write_cmos_sensor1(0x6F12, 0xB612);
	write_cmos_sensor1(0x6F12, 0xC0E3);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x80E0);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x9FE5);
	write_cmos_sensor1(0x6F12, 0x5410);
	write_cmos_sensor1(0x6F12, 0x81E5);
	write_cmos_sensor1(0x6F12, 0x0400);
	write_cmos_sensor1(0x6F12, 0xA0E1);
	write_cmos_sensor1(0x6F12, 0x0400);
	write_cmos_sensor1(0x6F12, 0x00EB);
	write_cmos_sensor1(0x6F12, 0x3500);
	write_cmos_sensor1(0x6F12, 0xD5E1);
	write_cmos_sensor1(0x6F12, 0xB000);
	write_cmos_sensor1(0x6F12, 0xBDE8);
	write_cmos_sensor1(0x6F12, 0x7040);
	write_cmos_sensor1(0x6F12, 0xA0E3);
	write_cmos_sensor1(0x6F12, 0x0110);
	write_cmos_sensor1(0x6F12, 0x00EA);
	write_cmos_sensor1(0x6F12, 0x1300);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x4827);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x9018);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x1018);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x901F);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x4005);
	write_cmos_sensor1(0x6F12, 0x00D0);
	write_cmos_sensor1(0x6F12, 0x00C2);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x4C27);
	write_cmos_sensor1(0x6F12, 0x00D0);
	write_cmos_sensor1(0x6F12, 0x0096);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x801E);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x3602);
	write_cmos_sensor1(0x6F12, 0x00D0);
	write_cmos_sensor1(0x6F12, 0x00A6);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0xE018);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x1662);
	write_cmos_sensor1(0x6F12, 0x00D0);
	write_cmos_sensor1(0x6F12, 0x0062);
	write_cmos_sensor1(0x6F12, 0x0070);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xD0DD);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x44DD);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x68DC);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x7CBC);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xA0B1);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x7CE4);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x5055);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x9090);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x50B8);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x5C9D);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x1402);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x187F);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x00C0);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x98C4);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x2C4B);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xDC0C);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xFCE5);
	write_cmos_sensor1(0x6F12, 0x1FE5);
	write_cmos_sensor1(0x6F12, 0x04F0);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0xA436);
	write_cmos_sensor1(0x6028, 0xD000);
	write_cmos_sensor1(0x602A, 0x3690);
	write_cmos_sensor(0x6F12, 0x01  );
	write_cmos_sensor1(0x602A, 0x3692);
	write_cmos_sensor1(0x6F12, 0x0060);
	write_cmos_sensor1(0x602A, 0x369A);
	write_cmos_sensor1(0x6F12, 0xF446);
	write_cmos_sensor1(0x6F12, 0x5176);
	write_cmos_sensor1(0x6F12, 0x5C76);
	write_cmos_sensor1(0x6F12, 0x5C76);
	write_cmos_sensor1(0x602A, 0x36AE);
	write_cmos_sensor1(0x6F12, 0xF40E);
	write_cmos_sensor1(0x6F12, 0x0B00);
	write_cmos_sensor1(0x6F12, 0x4B00);
	write_cmos_sensor1(0x6F12, 0x4B00);
	write_cmos_sensor1(0x602A, 0xF412);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x602A, 0x3664);
	write_cmos_sensor1(0x6F12, 0x0BB8);
	write_cmos_sensor1(0x602A, 0x35E4);
	write_cmos_sensor1(0x6F12, 0x4776);
	write_cmos_sensor1(0x602A, 0x3675);
	write_cmos_sensor(0x6F12, 0x76  );
	write_cmos_sensor1(0x602A, 0x3672);
	write_cmos_sensor1(0x6F12, 0x0520);
	write_cmos_sensor1(0x602A, 0x367A);
	write_cmos_sensor1(0x6F12, 0x00ED);
	write_cmos_sensor1(0x602A, 0x372A);
	write_cmos_sensor(0x6F12, 0x01  );
	write_cmos_sensor1(0x602A, 0x3246);
	write_cmos_sensor1(0x6F12, 0x0092);
	write_cmos_sensor1(0x602A, 0x324A);
	write_cmos_sensor1(0x6F12, 0x009C);
	write_cmos_sensor1(0x602A, 0x3256);
	write_cmos_sensor1(0x6F12, 0x01BD);
	write_cmos_sensor1(0x602A, 0x325A);
	write_cmos_sensor1(0x6F12, 0x01C7);
	write_cmos_sensor1(0x602A, 0x3248);
	write_cmos_sensor1(0x6F12, 0x0092);
	write_cmos_sensor1(0x602A, 0x324C);
	write_cmos_sensor1(0x6F12, 0x009C);
	write_cmos_sensor1(0x602A, 0x3258);
	write_cmos_sensor1(0x6F12, 0x01B2);
	write_cmos_sensor1(0x602A, 0x325C);
	write_cmos_sensor1(0x6F12, 0x01BC);
	write_cmos_sensor1(0x602A, 0x3792);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x602A, 0x379A);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x602A, 0x33DA);
	write_cmos_sensor1(0x6F12, 0x00BC);
	write_cmos_sensor1(0x6F12, 0x00BC);
	write_cmos_sensor1(0x6F12, 0x00BE);
	write_cmos_sensor1(0x6F12, 0x00BE);
	write_cmos_sensor1(0x602A, 0x33EA);
	write_cmos_sensor1(0x6F12, 0x00BC);
	write_cmos_sensor1(0x6F12, 0x00BC);
	write_cmos_sensor1(0x6F12, 0x00D8);
	write_cmos_sensor1(0x6F12, 0x00D8);
	write_cmos_sensor1(0x602A, 0x36A2);
	write_cmos_sensor1(0x6F12, 0x5176);
	write_cmos_sensor1(0x6F12, 0x2F76);
	write_cmos_sensor1(0x6F12, 0x2F76);
	write_cmos_sensor1(0x602A, 0x36B6);
	write_cmos_sensor1(0x6F12, 0x0B00);
	write_cmos_sensor1(0x6F12, 0x4B00);
	write_cmos_sensor1(0x6F12, 0x4B00);
	write_cmos_sensor1(0x602A, 0x3690);
	write_cmos_sensor(0x6F12, 0x01  );
	write_cmos_sensor1(0x602A, 0x372C);
	write_cmos_sensor(0x6F12, 0x48  );
	write_cmos_sensor1(0x602A, 0x372D);
	write_cmos_sensor(0x6F12, 0x48  );
	write_cmos_sensor1(0x602A, 0x372E);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x1106);
	write_cmos_sensor1(0x6028, 0x7000);
	write_cmos_sensor1(0x602A, 0x2754);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x0300);
	write_cmos_sensor1(0x602A, 0x2760);
	write_cmos_sensor1(0x6F12, 0x080B);
	write_cmos_sensor1(0x602A, 0x2758);
	write_cmos_sensor1(0x6F12, 0x4500);
	write_cmos_sensor1(0x6F12, 0x4700);
	write_cmos_sensor1(0x6F12, 0xF800);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x602A, 0x2762);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x602A, 0x2770);
	write_cmos_sensor1(0x6F12, 0xE63B);
	write_cmos_sensor1(0x602A, 0x2768);
	write_cmos_sensor1(0x6F12, 0x4500);
	write_cmos_sensor1(0x6F12, 0x4700);
	write_cmos_sensor1(0x6F12, 0xF800);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x602A, 0x2772);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x602A, 0x2780);
	write_cmos_sensor1(0x6F12, 0xFA3B);
	write_cmos_sensor1(0x602A, 0x2778);
	write_cmos_sensor1(0x6F12, 0x4500);
	write_cmos_sensor1(0x6F12, 0x4700);
	write_cmos_sensor1(0x6F12, 0xF800);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x602A, 0x2782);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6028, 0xD000);
	write_cmos_sensor1(0x602A, 0x0C00);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x6F12, 0x01FF);
	write_cmos_sensor1(0x6F12, 0x0400);
	write_cmos_sensor1(0x6F12, 0x0438);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x03FF);
	write_cmos_sensor1(0x6F12, 0x0415);
	write_cmos_sensor1(0x6F12, 0x045B);
	write_cmos_sensor1(0x6F12, 0x0400);
	write_cmos_sensor1(0x6F12, 0x07FF);
	write_cmos_sensor1(0x6F12, 0x0421);
	write_cmos_sensor1(0x6F12, 0x043B);
	write_cmos_sensor1(0x6F12, 0x0800);
	write_cmos_sensor1(0x6F12, 0x0FFF);
	write_cmos_sensor1(0x6F12, 0x0423);
	write_cmos_sensor1(0x6F12, 0x043B);
	write_cmos_sensor1(0x6F12, 0x1000);
	write_cmos_sensor1(0x6F12, 0x1FFF);
	write_cmos_sensor1(0x6F12, 0x0426);
	write_cmos_sensor1(0x6F12, 0x0426);
	write_cmos_sensor1(0x602A, 0x0B80);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x602A, 0x0B86);
	write_cmos_sensor1(0x6F12, 0x0020);
	write_cmos_sensor1(0x602A, 0x0B84);
	write_cmos_sensor1(0x6F12, 0x0020);
	write_cmos_sensor1(0x602A, 0x0B82);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x602A, 0x0B8C);
	write_cmos_sensor1(0x6F12, 0x000A);
	write_cmos_sensor1(0x602A, 0x0B88);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x602A, 0x0B96);
	write_cmos_sensor1(0x6F12, 0x0005);
	write_cmos_sensor1(0x602A, 0x0B94);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x602A, 0x0602);
	write_cmos_sensor1(0x6F12, 0x03FF);
	write_cmos_sensor1(0x6F12, 0x03FF);
	write_cmos_sensor1(0x6F12, 0x03FF);
	write_cmos_sensor1(0x6F12, 0x03FF);
	write_cmos_sensor1(0x602A, 0x3BE0);
	write_cmos_sensor1(0x6F12, 0x03DA);
	write_cmos_sensor1(0x6F12, 0x01D6);
	write_cmos_sensor1(0x6F12, 0x01BF);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x000A);
	write_cmos_sensor1(0x6F12, 0x0022);
	write_cmos_sensor1(0x6F12, 0x000A);
	write_cmos_sensor1(0x6F12, 0x0022);
	write_cmos_sensor1(0x6F12, 0x000A);
	write_cmos_sensor1(0x6F12, 0x0022);
	write_cmos_sensor1(0x6F12, 0x000A);
	write_cmos_sensor1(0x6F12, 0x0022);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x000A);
	write_cmos_sensor1(0x6F12, 0x0022);
	write_cmos_sensor1(0x6F12, 0x000A);
	write_cmos_sensor1(0x6F12, 0x0022);
	write_cmos_sensor1(0x6F12, 0x000A);
	write_cmos_sensor1(0x6F12, 0x0022);
	write_cmos_sensor1(0x6F12, 0x000A);
	write_cmos_sensor1(0x6F12, 0x0022);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0014);
	write_cmos_sensor1(0x6F12, 0x0040);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0018);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0200);
	write_cmos_sensor1(0x6F12, 0x0F00);
	write_cmos_sensor1(0x6F12, 0x4B00);
	write_cmos_sensor1(0x6F12, 0x8700);
	write_cmos_sensor1(0x6F12, 0xC300);
	write_cmos_sensor1(0x6F12, 0xFF00);
	write_cmos_sensor1(0x6F12, 0x0032);
	write_cmos_sensor(0x6F12, 0x42  );
	write_cmos_sensor1(0x602A, 0x3DAB);
	write_cmos_sensor(0x6F12, 0xFF  );
	write_cmos_sensor1(0x602A, 0x3DAC);
	write_cmos_sensor(0x6F12, 0x08  );
	write_cmos_sensor1(0x602A, 0x3DAE);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor(0x6F12, 0x00  );
	write_cmos_sensor1(0x602A, 0x3DB1);
	write_cmos_sensor(0x6F12, 0x08  );
	write_cmos_sensor1(0x602A, 0x3DB2);
	write_cmos_sensor1(0x6F12, 0x00FF);
	write_cmos_sensor(0x6F12, 0x00  );
	write_cmos_sensor1(0x602A, 0x3DB5);
	write_cmos_sensor(0x6F12, 0x08  );
	write_cmos_sensor1(0x602A, 0x3DB6);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor(0x6F12, 0x01  );
	write_cmos_sensor1(0x602A, 0x3DBA);
	write_cmos_sensor1(0x6F12, 0x0F00);
	write_cmos_sensor1(0x602A, 0x300C);
	write_cmos_sensor1(0x6F12, 0x0001);
}    /*    sensor_init  */


static void preview_setting(void)
{	
	write_cmos_sensor(0x0100, 0x00  );
	mDELAY(10);                          
	check_output_stream_off();                          
	write_cmos_sensor1(0x0200, 0x000A);
	write_cmos_sensor1(0x0136, 0x1800);
	write_cmos_sensor1(0x0304, 0x0006);
	write_cmos_sensor1(0x0306, 0x00D8);
	write_cmos_sensor1(0x030C, 0x0006);
	write_cmos_sensor1(0x030E, 0x00EB);
	write_cmos_sensor1(0x0302, 0x0001);
	write_cmos_sensor1(0x0300, 0x0002);
	write_cmos_sensor1(0x030A, 0x0001);
	write_cmos_sensor1(0x0308, 0x0008);
	write_cmos_sensor(0x304F, 0x00  );
	write_cmos_sensor(0x35D9, 0x00  );
	write_cmos_sensor1(0x303A, 0x02BC);
	write_cmos_sensor(0x0B05, 0x01  );
	write_cmos_sensor(0x0B08, 0x01  );
	write_cmos_sensor(0x380E, 0x01  );
	write_cmos_sensor(0x3027, 0x10  );
	write_cmos_sensor1(0x0202, 0x0014);
	write_cmos_sensor1(0x0340, 0x0A38);
	write_cmos_sensor1(0x0342, 0x1580);
	write_cmos_sensor1(0x034C, 0x0830);
	write_cmos_sensor1(0x034E, 0x0610);
	write_cmos_sensor(0x0114, 0x03  );
	write_cmos_sensor1(0x0344, 0x000C);
	write_cmos_sensor1(0x0346, 0x000c);
	write_cmos_sensor1(0x0348, 0x106b);
	write_cmos_sensor1(0x034A, 0x0C2b);
	write_cmos_sensor1(0x3010, 0x0000);
	write_cmos_sensor1(0x3012, 0x0000);
	write_cmos_sensor1(0x0386, 0x0003);
	write_cmos_sensor1(0x0382, 0x0003);
	write_cmos_sensor(0x0900, 0x01  );
	write_cmos_sensor(0x0901, 0x22  );
	write_cmos_sensor(0x3027, 0x10  );
	write_cmos_sensor1(0x0400, 0x0000);
	write_cmos_sensor1(0x0404, 0x0010);
	write_cmos_sensor(0x0111, 0x02  );
	write_cmos_sensor1(0x0112, 0x0A0A);
	write_cmos_sensor(0x302A, 0x00  );
	write_cmos_sensor1(0x0500, 0x0000);
	write_cmos_sensor1(0x303E, 0x00A0);
	write_cmos_sensor(0x30DE, 0x01  );
	write_cmos_sensor(0x0100, 0x01  );
	mDELAY(10);  
	check_output_stream_on();
	}    /*    preview_setting  */

static void capture_setting(kal_uint16 currefps)
{	
	write_cmos_sensor(0x0100, 0x00  );
	mDELAY(10);                        
  check_output_stream_off();                        
	write_cmos_sensor1(0x0200, 0x000A);
	write_cmos_sensor1(0x0136, 0x1800);
	write_cmos_sensor1(0x0304, 0x0006);
	write_cmos_sensor1(0x0306, 0x00D8);
	write_cmos_sensor1(0x030C, 0x0006);
	write_cmos_sensor1(0x030E, 0x010E);
	write_cmos_sensor1(0x0302, 0x0001);
	write_cmos_sensor1(0x0300, 0x0002);
	write_cmos_sensor1(0x030A, 0x0001);
	write_cmos_sensor1(0x0308, 0x0008);
	write_cmos_sensor(0x304F, 0x00  );
	write_cmos_sensor(0x35D9, 0x00  );
	write_cmos_sensor1(0x303A, 0x02BC);
	write_cmos_sensor(0x0B05, 0x01  );
	write_cmos_sensor(0x0B08, 0x01  );
	write_cmos_sensor(0x380E, 0x01  );
	write_cmos_sensor(0x3027, 0x10  );
	write_cmos_sensor1(0x0202, 0x0014);
	write_cmos_sensor1(0x0340, 0x0c6c);
	write_cmos_sensor1(0x0342, 0x11A0);
	write_cmos_sensor1(0x034C, 0x1060);
	write_cmos_sensor1(0x034E, 0x0C20);
	write_cmos_sensor(0x0114, 0x03  );
	write_cmos_sensor1(0x0344, 0x000C);
	write_cmos_sensor1(0x0346, 0x000C);
	write_cmos_sensor1(0x0348, 0x106b);
	write_cmos_sensor1(0x034A, 0x0C2B);
	write_cmos_sensor1(0x3010, 0x0000);
	write_cmos_sensor1(0x3012, 0x0000);
	write_cmos_sensor1(0x0386, 0x0001);
	write_cmos_sensor1(0x0382, 0x0001);
	write_cmos_sensor(0x0900, 0x00  );
	write_cmos_sensor(0x0901, 0x11  );
	write_cmos_sensor(0x3027, 0x10  );
	write_cmos_sensor1(0x0400, 0x0000);
	write_cmos_sensor1(0x0404, 0x0010);
	write_cmos_sensor(0x0111, 0x02  );
	write_cmos_sensor1(0x0112, 0x0A0A);
	write_cmos_sensor(0x302A, 0x00  );
	write_cmos_sensor1(0x0500, 0x0000);
	write_cmos_sensor1(0x303E, 0x00A0);
	write_cmos_sensor(0x30DE, 0x01  );
	write_cmos_sensor(0x0100, 0x01  );
	mDELAY(10);
	check_output_stream_on();
	}

static void normal_video_setting(kal_uint16 currefps)
{
	preview_setting();
}


static void hs_video_setting(void)
{	
	write_cmos_sensor1(0x602A, 0x0100);
	write_cmos_sensor(0x6F12, 0x00  );
	mDELAY(10);                         
	check_output_stream_off();                      
	write_cmos_sensor1(0x602A, 0x0200);
	write_cmos_sensor1(0x6F12, 0x000A);
	write_cmos_sensor1(0x6F12, 0x0100);
	write_cmos_sensor1(0x602A, 0x0136);
	write_cmos_sensor1(0x6F12, 0x1800);
	write_cmos_sensor1(0x602A, 0x0304);
	write_cmos_sensor1(0x6F12, 0x0006);
	write_cmos_sensor1(0x6F12, 0x00E0);
	write_cmos_sensor1(0x602A, 0x030C);
	write_cmos_sensor1(0x6F12, 0x0006);
	write_cmos_sensor1(0x6F12, 0x00C6);
	write_cmos_sensor1(0x602A, 0x0300);
	write_cmos_sensor1(0x6F12, 0x0002);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x602A, 0x0308);
	write_cmos_sensor1(0x6F12, 0x0008);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x602A, 0x304F);
	write_cmos_sensor1(0x6F12, 0x0001);
	write_cmos_sensor1(0x602A, 0x35D9);
	write_cmos_sensor(0x6F12, 0x03  );
	write_cmos_sensor1(0x602A, 0x303A);
	write_cmos_sensor1(0x6F12, 0x017C);
	write_cmos_sensor1(0x602A, 0x0B05);
	write_cmos_sensor(0x6F12, 0x01  );
	write_cmos_sensor1(0x602A, 0x0B08);
	write_cmos_sensor(0x6F12, 0x01  );
	write_cmos_sensor1(0x602A, 0x380E);
	write_cmos_sensor(0x6F12, 0x01  );
	write_cmos_sensor1(0x602A, 0x0342);
	write_cmos_sensor1(0x6F12, 0x11D0);
	write_cmos_sensor1(0x602A, 0x0340);
	write_cmos_sensor1(0x6F12, 0x0332);
	write_cmos_sensor1(0x602A, 0x034C);
	write_cmos_sensor1(0x6F12, 0x0288);
	write_cmos_sensor1(0x6F12, 0x01E8);
	write_cmos_sensor1(0x602A, 0x0114);
	write_cmos_sensor(0x6F12, 0x03  );
	write_cmos_sensor1(0x602A, 0x0344);
	write_cmos_sensor1(0x6F12, 0x00BA);
	write_cmos_sensor1(0x6F12, 0x0078);
	write_cmos_sensor1(0x6F12, 0x0FE9);
	write_cmos_sensor1(0x6F12, 0x0BE7);
	write_cmos_sensor1(0x602A, 0x3010);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x602A, 0x0386);
	write_cmos_sensor1(0x6F12, 0x000B);
	write_cmos_sensor1(0x602A, 0x0382);
	write_cmos_sensor1(0x6F12, 0x0003);
	write_cmos_sensor1(0x602A, 0x0900);
	write_cmos_sensor(0x6F12, 0x01  );
	write_cmos_sensor1(0x602A, 0x0901);
	write_cmos_sensor(0x6F12, 0x21  );
	write_cmos_sensor1(0x602A, 0x3027);
	write_cmos_sensor(0x6F12, 0x30  );
	write_cmos_sensor1(0x602A, 0x3BCE);
	write_cmos_sensor(0x6F12, 0x20  );
	write_cmos_sensor1(0x602A, 0x3BCF);
	write_cmos_sensor(0x6F12, 0x20  );
	write_cmos_sensor1(0x602A, 0x0400);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x602A, 0x0404);
	write_cmos_sensor1(0x6F12, 0x0010);
	write_cmos_sensor1(0x602A, 0x0111);
	write_cmos_sensor(0x6F12, 0x02  );
	write_cmos_sensor1(0x602A, 0x0112);
	write_cmos_sensor1(0x6F12, 0x0A0A);
	write_cmos_sensor1(0x602A, 0x302A);
	write_cmos_sensor(0x6F12, 0x00  );
	write_cmos_sensor1(0x602A, 0x0500);
	write_cmos_sensor1(0x6F12, 0x0000);
	write_cmos_sensor1(0x602A, 0x303E);
	write_cmos_sensor1(0x6F12, 0x00A0);
	write_cmos_sensor1(0x602A, 0x30DE);
	write_cmos_sensor(0x6F12, 0x01  );
	write_cmos_sensor1(0x602A, 0x0100);
	write_cmos_sensor(0x6F12, 0x01  );
	mDELAY(10);
  check_output_stream_on();
  }

static void slim_video_setting(void)
{
	write_cmos_sensor(0x0100, 0x00  );
	mDELAY(10);                        
	check_output_stream_off();                       
	write_cmos_sensor1(0x0200, 0x000A);
	write_cmos_sensor1(0x0202, 0x0014);
	write_cmos_sensor1(0x0136, 0x1800);
	write_cmos_sensor1(0x0304, 0x0006);
	write_cmos_sensor1(0x0306, 0x00D8);
	write_cmos_sensor1(0x030C, 0x0006);
	write_cmos_sensor1(0x030E, 0x00EB);
	write_cmos_sensor1(0x0302, 0x0001);
	write_cmos_sensor1(0x0300, 0x0002);
	write_cmos_sensor1(0x030A, 0x0001);
	write_cmos_sensor1(0x0308, 0x0008);
	write_cmos_sensor(0x304F, 0x00  );
	write_cmos_sensor(0x35D9, 0x00  );
	write_cmos_sensor1(0x303A, 0x02BC);
	write_cmos_sensor(0x0B05, 0x01  );
	write_cmos_sensor(0x0B08, 0x01  );
	write_cmos_sensor(0x380E, 0x01  );
	write_cmos_sensor(0x3027, 0x10  );
	write_cmos_sensor1(0x0342, 0x1580);
	write_cmos_sensor1(0x0340, 0x0A38);
	write_cmos_sensor1(0x034C, 0x0830);
	write_cmos_sensor1(0x034E, 0x0610);
	write_cmos_sensor(0x0114, 0x03  );
	write_cmos_sensor1(0x0344, 0x000C);
	write_cmos_sensor1(0x0346, 0x000c);
	write_cmos_sensor1(0x0348, 0x106b);
	write_cmos_sensor1(0x034A, 0x0C2b);
	write_cmos_sensor1(0x3010, 0x0000);
	write_cmos_sensor1(0x3012, 0x0000);
	write_cmos_sensor1(0x0386, 0x0003);
	write_cmos_sensor1(0x0382, 0x0003);
	write_cmos_sensor(0x0900, 0x01  );
	write_cmos_sensor(0x0901, 0x22  );
	write_cmos_sensor(0x3027, 0x10  );
	write_cmos_sensor1(0x0400, 0x0000);
	write_cmos_sensor1(0x0404, 0x0010);
	write_cmos_sensor(0x0111, 0x02  );
	write_cmos_sensor1(0x0112, 0x0A0A);
	write_cmos_sensor(0x302A, 0x00  );
	write_cmos_sensor1(0x0500, 0x0000);
	write_cmos_sensor1(0x303E, 0x00A0);
	write_cmos_sensor(0x30DE, 0x01  );
	write_cmos_sensor(0x0100, 0x01  );
	mDELAY(10);
	check_output_stream_on();
	}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
    //LOG_INF("enable: %d\n", enable);

    if (enable) {
		write_cmos_sensor1(0x6028,0xD000);		
		write_cmos_sensor1(0x602A,0x0600); 		
		write_cmos_sensor1(0x6F12,0x0002); 

    } else {
		write_cmos_sensor1(0x6028,0xD000);		
		write_cmos_sensor1(0x602A,0x0600);		
		write_cmos_sensor1(0x6F12,0x0000); 

    }
    spin_lock(&imgsensor_drv_lock);
    imgsensor.test_pattern = enable;
    spin_unlock(&imgsensor_drv_lock);
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    get_imgsensor_id
*
* DESCRIPTION
*    This function get the sensor ID
*
* PARAMETERS
*    *sensorID : return the sensor ID
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 2;
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            *sensor_id = return_sensor_id();
            if (*sensor_id == imgsensor_info.sensor_id) {
                printk("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
                return ERROR_NONE;
            }
            printk("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
            retry--;
        } while(retry > 0);
        i++;
        retry = 2;
    }
    if (*sensor_id != imgsensor_info.sensor_id) {
        // if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF
        *sensor_id = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*    open
*
* DESCRIPTION
*    This function initialize the registers of CMOS sensor
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 2;
    kal_uint32 sensor_id = 0;
    //LOG_1;
    //LOG_2;
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            sensor_id = return_sensor_id();
            if (sensor_id == imgsensor_info.sensor_id) {
                printk("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
                break;
            }
            printk("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
            retry--;
        } while(retry > 0);
        i++;
        if (sensor_id == imgsensor_info.sensor_id)
            break;
        retry = 2;
    }
    if (imgsensor_info.sensor_id != sensor_id)
        return ERROR_SENSOR_CONNECT_FAIL;

    /* initail sequence write in  */
    sensor_init();

    spin_lock(&imgsensor_drv_lock);

    imgsensor.autoflicker_en= KAL_FALSE;
    imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
    imgsensor.pclk = imgsensor_info.pre.pclk;
    imgsensor.frame_length = imgsensor_info.pre.framelength;
    imgsensor.line_length = imgsensor_info.pre.linelength;
    imgsensor.min_frame_length = imgsensor_info.pre.framelength;
    imgsensor.dummy_pixel = 0;
    imgsensor.dummy_line = 0;
    imgsensor.ihdr_en = 0;
    imgsensor.test_pattern = KAL_FALSE;
    imgsensor.current_fps = imgsensor_info.pre.max_framerate;
    spin_unlock(&imgsensor_drv_lock);

    return ERROR_NONE;
}    /*    open  */



/*************************************************************************
* FUNCTION
*    close
*
* DESCRIPTION
*
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
    //LOG_INF("E\n");

    /*No Need to implement this function*/

    return ERROR_NONE;
}    /*    close  */


/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*    This function start the sensor preview.
*
* PARAMETERS
*    *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    //LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
    imgsensor.pclk = imgsensor_info.pre.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.pre.linelength;
    imgsensor.frame_length = imgsensor_info.pre.framelength;
    imgsensor.min_frame_length = imgsensor_info.pre.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
    return ERROR_NONE;
}    /*    preview   */

/*************************************************************************
* FUNCTION
*    capture
*
* DESCRIPTION
*    This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                          MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    //LOG_INF("E\n");
	
    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
    if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
        imgsensor.pclk = imgsensor_info.cap1.pclk;
        imgsensor.line_length = imgsensor_info.cap1.linelength;
        imgsensor.frame_length = imgsensor_info.cap1.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    } else {
        if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
            //LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap.max_framerate/10);
        printk("Warning: current_fps\n");
        imgsensor.pclk = imgsensor_info.cap.pclk;
        imgsensor.line_length = imgsensor_info.cap.linelength;
        imgsensor.frame_length = imgsensor_info.cap.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    }
    spin_unlock(&imgsensor_drv_lock);
    capture_setting(imgsensor.current_fps);
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
    return ERROR_NONE;
}    /* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    //LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
    imgsensor.pclk = imgsensor_info.normal_video.pclk;
    imgsensor.line_length = imgsensor_info.normal_video.linelength;
    imgsensor.frame_length = imgsensor_info.normal_video.framelength;
    imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
    //imgsensor.current_fps = 300;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    normal_video_setting(imgsensor.current_fps);	
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
    return ERROR_NONE;
}    /*    normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    //LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
    imgsensor.pclk = imgsensor_info.hs_video.pclk;
    //imgsensor.video_mode = KAL_TRUE;
    imgsensor.line_length = imgsensor_info.hs_video.linelength;
    imgsensor.frame_length = imgsensor_info.hs_video.framelength;
    imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
    imgsensor.dummy_line = 0;
    imgsensor.dummy_pixel = 0;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    hs_video_setting();	
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
    return ERROR_NONE;
}    /*    hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    //LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
    imgsensor.pclk = imgsensor_info.slim_video.pclk;
    imgsensor.line_length = imgsensor_info.slim_video.linelength;
    imgsensor.frame_length = imgsensor_info.slim_video.framelength;
    imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
    imgsensor.dummy_line = 0;
    imgsensor.dummy_pixel = 0;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    slim_video_setting();
	//set_mirror_flip(sensor_config_data->SensorImageMirror);

    return ERROR_NONE;
}    /*    slim_video     */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
    //LOG_INF("E\n");
	printk("imgsensor_info.cap.grabwindow_width: %d\n", imgsensor_info.cap.grabwindow_width);

    sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
    sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

    sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
    sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

    sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
    sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


    sensor_resolution->SensorHighSpeedVideoWidth     = imgsensor_info.hs_video.grabwindow_width;
    sensor_resolution->SensorHighSpeedVideoHeight     = imgsensor_info.hs_video.grabwindow_height;

    sensor_resolution->SensorSlimVideoWidth     = imgsensor_info.slim_video.grabwindow_width;
    sensor_resolution->SensorSlimVideoHeight     = imgsensor_info.slim_video.grabwindow_height;
	//LOG_INF("L\n");
    return ERROR_NONE;
}    /*    get_resolution    */

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
                      MSDK_SENSOR_INFO_STRUCT *sensor_info,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    //LOG_INF("scenario_id = %d\n", scenario_id);


    //sensor_info->SensorVideoFrameRate = imgsensor_info.normal_video.max_framerate/10; /* not use */
    //sensor_info->SensorStillCaptureFrameRate= imgsensor_info.cap.max_framerate/10; /* not use */
    //imgsensor_info->SensorWebCamCaptureFrameRate= imgsensor_info.v.max_framerate; /* not use */

    sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
    sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
    sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; // inverse with datasheet
    sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    sensor_info->SensorInterruptDelayLines = 4; /* not use */
    sensor_info->SensorResetActiveHigh = FALSE; /* not use */
    sensor_info->SensorResetDelayCount = 5; /* not use */

    sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
    sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
    sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
    sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

    sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
    sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
    sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
    sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
    sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;

    sensor_info->SensorMasterClockSwitch = 0; /* not use */
    sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

    sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;          /* The frame of setting shutter default 0 for TG int */
    sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;    /* The frame of setting sensor gain */
    sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
    sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
    sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
    sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

    sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
    sensor_info->SensorClockFreq = imgsensor_info.mclk;
    sensor_info->SensorClockDividCount = 3; /* not use */
    sensor_info->SensorClockRisingCount = 0;
    sensor_info->SensorClockFallingCount = 2; /* not use */
    sensor_info->SensorPixelClockCount = 3; /* not use */
    sensor_info->SensorDataLatchCount = 2; /* not use */

    sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
    sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
    sensor_info->SensorWidthSampling = 0;  // 0 is default 1x
    sensor_info->SensorHightSampling = 0;    // 0 is default 1x
    sensor_info->SensorPacketECCOrder = 1;

    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

            sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

            break;
        default:
            sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
            break;
    }

    return ERROR_NONE;
}    /*    get_info  */


static kal_uint32 control(MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    //LOG_INF("scenario_id = %d\n", scenario_id);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.current_scenario_id = scenario_id;
    spin_unlock(&imgsensor_drv_lock);
    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            preview(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            capture(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            normal_video(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            hs_video(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            slim_video(image_window, sensor_config_data);
            break;
        default:
            //LOG_INF("Error ScenarioId setting");
            preview(image_window, sensor_config_data);
            return ERROR_INVALID_SCENARIO_ID;
    }
    return ERROR_NONE;
}    /* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{//This Function not used after ROME
    //LOG_INF("framerate = %d\n ", framerate);
    // SetVideoMode Function should fix framerate
    if (framerate == 0)
        // Dynamic frame rate
        return ERROR_NONE;
    spin_lock(&imgsensor_drv_lock);
    if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
        imgsensor.current_fps = 296;
    else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
        imgsensor.current_fps = 146;
    else
        imgsensor.current_fps = framerate;
    spin_unlock(&imgsensor_drv_lock);
    set_max_framerate(imgsensor.current_fps,1);

    return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
    //LOG_INF("enable = %d, framerate = %d \n", enable, framerate);
    spin_lock(&imgsensor_drv_lock);
    if (enable) //enable auto flicker
        imgsensor.autoflicker_en = KAL_TRUE;
    else //Cancel Auto flick
        imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
    kal_uint32 frame_length;

    //LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            if(framerate == 0)
                return ERROR_NONE;
            frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        	  if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
                frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            } else {
        		    if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
                    //LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",framerate,imgsensor_info.cap.max_framerate/10);
                printk("Warning: current_fps\n");
                frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            }
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
            imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        default:  //coding with  preview scenario by default
            frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            //LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
            break;
    }
    return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
    //LOG_INF("scenario_id = %d\n", scenario_id);

    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            *framerate = imgsensor_info.pre.max_framerate;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            *framerate = imgsensor_info.normal_video.max_framerate;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            *framerate = imgsensor_info.cap.max_framerate;
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            *framerate = imgsensor_info.hs_video.max_framerate;
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            *framerate = imgsensor_info.slim_video.max_framerate;
            break;
        default:
            break;
    }

    return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
                             UINT8 *feature_para,UINT32 *feature_para_len)
{
    UINT16 *feature_return_para_16=(UINT16 *) feature_para;
    UINT16 *feature_data_16=(UINT16 *) feature_para;
    UINT32 *feature_return_para_32=(UINT32 *) feature_para;
    UINT32 *feature_data_32=(UINT32 *) feature_para;
    unsigned long long *feature_data=(unsigned long long *) feature_para;
    //unsigned long long *feature_return_para=(unsigned long long *) feature_para;

    SENSOR_WINSIZE_INFO_STRUCT *wininfo;
    MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

    printk("feature_id = %d\n", feature_id);
    switch (feature_id) {
        case SENSOR_FEATURE_GET_PERIOD:
            *feature_return_para_16++ = imgsensor.line_length;
            *feature_return_para_16 = imgsensor.frame_length;
            *feature_para_len=4;
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            //LOG_INF("feature_Control imgsensor.pclk = %d,imgsensor.current_fps = %d\n", imgsensor.pclk,imgsensor.current_fps);
            *feature_return_para_32 = imgsensor.pclk;
            *feature_para_len=4;
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            set_shutter(*feature_data);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            night_mode((BOOL) *feature_data);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            set_gain((UINT16) *feature_data);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *feature_return_para_32=LENS_DRIVER_ID_DO_NOT_CARE;
            *feature_para_len=4;
            break;
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            set_video_mode(*feature_data);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            get_imgsensor_id(feature_return_para_32);
            break;
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            set_auto_flicker_mode((BOOL)*feature_data_16,*(feature_data_16+1));
            break;
        case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
            set_max_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
            break;
        case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
            get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*(feature_data), (MUINT32 *)(uintptr_t)(*(feature_data+1)));
            break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            set_test_pattern_mode((BOOL)*feature_data);
            break;
        case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
            *feature_return_para_32 = imgsensor_info.checksum_value;
            *feature_para_len=4;
            break;
        case SENSOR_FEATURE_SET_FRAMERATE:
            //LOG_INF("current fps :%d\n", *feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.current_fps = *feature_data;
            spin_unlock(&imgsensor_drv_lock);
            break;
        case SENSOR_FEATURE_SET_SENSOR_OTP_AWB_CMD:
            ////LOG_INF("Update sensor awb from otp :%d\n", (BOOL)*feature_data);
            //spin_lock(&imgsensor_drv_lock);
            //imgsensor.update_sensor_otp_awb = (BOOL)*feature_data;
            //spin_unlock(&imgsensor_drv_lock);
            //if(0 != imgsensor.update_sensor_otp_awb || 0 != imgsensor.update_sensor_otp_lsc) {
            //    otp_update(imgsensor.update_sensor_otp_awb, imgsensor.update_sensor_otp_lsc);
            //}
            break;
        case SENSOR_FEATURE_SET_SENSOR_OTP_LSC_CMD:
            ////LOG_INF("Update sensor lsc from otp :%d\n", (BOOL)*feature_data);
            //spin_lock(&imgsensor_drv_lock);
            //imgsensor.update_sensor_otp_lsc = (BOOL)*feature_data;
            //spin_unlock(&imgsensor_drv_lock);
            //if(0 != imgsensor.update_sensor_otp_awb || 0 != imgsensor.update_sensor_otp_lsc) {
            //    otp_update(imgsensor.update_sensor_otp_awb, imgsensor.update_sensor_otp_lsc);
            //}
            break;
        case SENSOR_FEATURE_SET_HDR:
            //LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.ihdr_en = (BOOL)*feature_data;
            spin_unlock(&imgsensor_drv_lock);
            break;
        case SENSOR_FEATURE_GET_CROP_INFO:
            //LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", *feature_data);

            wininfo = (SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

            switch (*feature_data_32) {
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                    memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[1],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
                    break;
                case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                    memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[2],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
                    break;
                case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
                    memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[3],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
                    break;
                case MSDK_SCENARIO_ID_SLIM_VIDEO:
                    memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[4],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
                    break;
                case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
                default:
                    memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[0],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
                    break;
            }
        case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
            //LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            break;
        default:
            break;
    }

    return ERROR_NONE;
}   /*  feature_control()  */


static SENSOR_FUNCTION_STRUCT sensor_func = {
    open,
    get_info,
    get_resolution,
    feature_control,
    control,
    close
};

UINT32 S5K3L2YX_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&sensor_func;
    return ERROR_NONE;
}    /*    OV5693_MIPI_RAW_SensorInit    */
