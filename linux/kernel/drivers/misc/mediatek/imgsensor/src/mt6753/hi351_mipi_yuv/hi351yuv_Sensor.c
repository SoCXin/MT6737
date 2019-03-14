
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"
#include "kd_camera_typedef.h"

#include "hi351yuv_Sensor.h"
#include "hi351yuv_Camera_Sensor_para.h"
#include "hi351yuv_CameraCustomized.h"

#define HI351_DEBUG
#ifdef HI351_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

MSDK_SENSOR_CONFIG_STRUCT HI351SensorConfigData;
#define SENSOR_CORE_PCLK	83200000	//48M PCLK Output 78000000 

#define WINMO_USE 0
#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

kal_bool HI351_VEDIO_MPEG4 = KAL_FALSE; //Picture(Jpeg) or Video(Mpeg4);

kal_uint8 HI351_Sleep_Mode;
kal_uint32 HI351_PV_dummy_pixels=616,HI351_PV_dummy_lines=20,HI351_isp_master_clock=260/*0*/;

//static HI351_SENSOR_INFO_ST HI351_sensor;
static HI351_OPERATION_STATE_ST HI351_op_state;

static kal_uint32 zoom_factor = 0; 
static kal_bool HI351_gPVmode = KAL_TRUE; //PV size or Full size
static kal_bool HI351_VEDIO_encode_mode = KAL_FALSE; //Picture(Jpeg) or Video(Mpeg4)
static kal_bool HI351_sensor_cap_state = KAL_FALSE; //Preview or Capture

static kal_uint8 HI351_Banding_setting = AE_FLICKER_MODE_50HZ;  //Wonder add
//static kal_uint16  HI351_PV_Shutter = 0;
static kal_uint32  HI351_sensor_pclk=260;//520 //20110518

//static kal_uint32 HI351_pv_HI351_exposure_lines=0x017a00, HI351_cp_HI351_exposure_lines=0;
//static kal_uint16 HI351_Capture_Max_Gain16= 6*16;
//static kal_uint16 HI351_Capture_Gain16=0 ;    
//static kal_uint16 HI351_Capture_Shutter=0;
//static kal_uint16 HI351_Capture_Extra_Lines=0;

//static kal_uint16  HI351_PV_Gain16 = 0;
//static kal_uint16  HI351_PV_Extra_Lines = 0;
kal_uint32 HI351_capture_pclk_in_M=520,HI351_preview_pclk_in_M=390;

//extern static CAMERA_DUAL_CAMERA_SENSOR_ENUM g_currDualSensorIdx;
//extern static char g_currSensorName[32];
//extern int kdModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name);

extern int iReadReg_Byte(u8 addr, u8 *buf, u8 i2cId);
extern int iWriteReg_Byte(u8 addr, u8 buf, u32 size, u16 i2cId);
//SENSOR_REG_STRUCT HI351SensorCCT[FACTORY_END_ADDR]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
//SENSOR_REG_STRUCT HI351SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
//	camera_para.SENSOR.cct	SensorCCT	=> SensorCCT
//	camera_para.SENSOR.reg	SensorReg

BOOL HI351_set_param_banding(UINT16 para);

//extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff, u16 i2cId);
//extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes, u16 i2cId);
//extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
///*ergate-017*/
//extern int iWriteRegI2C_ext(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId, u16 speed);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
kal_uint16 HI351_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
  //char puSendCmd[2] = {(char)(addr & 0xFF) ,(char)(para & 0xFF)};

	//iWriteRegI2C_ext(puSendCmd , 2,HI351_WRITE_ID_0, 50);
	//iWriteReg_Byte(addr, para, 1, HI351_WRITE_ID_0);

    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 2,HI351_WRITE_ID_0);
	
	    return 0;
}

kal_uint8 HI351_read_cmos_sensor(kal_uint8 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte,1,HI351_WRITE_ID_0);
	
    return get_byte;
//	kal_uint8 get_byte=0;
//    iReadRegI2C(addr, &get_byte,1, HI351_WRITE_ID_0);
//    return get_byte;
}
/*
bool HI351_checkID(void)
{

	kal_uint16 sensor_id=0;
	sensor_id = HI351_read_cmos_sensor(0x04);
	printk("[Hi351] sensor id = 0x%x\n", sensor_id);
	if (HI351_SENSOR_ID != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;
}*/


void HI351_Init_Cmds(void) 
{
	//volatile signed char i;
	//kal_uint16 sensor_id=0;
	zoom_factor = 0; 

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x01, 0xf1);
HI351_write_cmos_sensor(0x01, 0xf3);
HI351_write_cmos_sensor(0x01, 0xf1);

///////////////////////////////////////////
// 0 Page PLL setting
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x07, 0x25); //24/(5+1) = 4Mhz
HI351_write_cmos_sensor(0x08, 0x60); // 108Mhz
HI351_write_cmos_sensor(0x09, 0x82);
HI351_write_cmos_sensor(0x07, 0xa5);
HI351_write_cmos_sensor(0x07, 0xa5);
HI351_write_cmos_sensor(0x09, 0xa2);

HI351_write_cmos_sensor(0x0A, 0x01); // MCU hardware reset
HI351_write_cmos_sensor(0x0A, 0x00);
HI351_write_cmos_sensor(0x0A, 0x01);
HI351_write_cmos_sensor(0x0A, 0x00);

///////////////////////////////////////////
// 20 Page OTP/ROM LSC download select setting
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x20);
HI351_write_cmos_sensor(0x3a, 0x00);
HI351_write_cmos_sensor(0x3b, 0x00);
HI351_write_cmos_sensor(0x3c, 0x00);

///////////////////////////////////////////
// 30 Page MCU reset, enable setting
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x30);
HI351_write_cmos_sensor(0x30, 0x86);
HI351_write_cmos_sensor(0x31, 0x00);
HI351_write_cmos_sensor(0x32, 0x0c);
HI351_write_cmos_sensor(0xe0, 0x00); //new version
HI351_write_cmos_sensor(0x10, 0x80); // mcu reset high
HI351_write_cmos_sensor(0x10, 0x8a); // mcu enable high
HI351_write_cmos_sensor(0x11, 0x08); // xdata memory reset high
HI351_write_cmos_sensor(0x11, 0x00); // xdata memory reset low

///////////////////////////////////////////
// Copy OTP register to Xdata.
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x07); // otp page
HI351_write_cmos_sensor(0x12, 0x01); // memory mux on(mcu)  //new version  just use no change  system setting
//108M
HI351_write_cmos_sensor(0x40, 0x35); // otp_cfg1 value is different by PLLMCLK
HI351_write_cmos_sensor(0x47, 0x0B); // otp_cfg8 value is different by PLLMCLK
//DPC reg in otp down to xdata
HI351_write_cmos_sensor(0x2E, 0x00); // OTP (DPC block) start addr h(OTP DPC -> xdata DPC)
HI351_write_cmos_sensor(0x2F, 0x20); // OTP (DPC block) start addr l
HI351_write_cmos_sensor(0x30, 0x00); // MCU (DPC block) sram addr h(OTP DPC -> xdata DPC)
HI351_write_cmos_sensor(0x31, 0xD6); // MCU (DPC block) sram addr l
HI351_write_cmos_sensor(0x32, 0x00); // Copy reg sram size h(DPC reg size)
HI351_write_cmos_sensor(0x33, 0xFF); // Copy reg sram size l
HI351_write_cmos_sensor(0x10, 0x02); // Copy mcu down set(OTP DPC -> xdata DPC)
HI351_write_cmos_sensor(0x8C, 0x08); //OTP DPC pos offset
HI351_write_cmos_sensor(0x8F, 0x20);
HI351_write_cmos_sensor(0x92, 0x00);
HI351_write_cmos_sensor(0x93, 0x54); //Full size normal No_flip case
HI351_write_cmos_sensor(0x94, 0x00);
HI351_write_cmos_sensor(0x95, 0x11); //Full size normal No_flip case
mdelay(50);
// Color ratio reg in otp down to xdata
HI351_write_cmos_sensor(0x2E, 0x03); // OTP (ColorRatio block) start addr h(OTP ColorRatio->xdata ColorRatio)
HI351_write_cmos_sensor(0x2F, 0x20); // OTP (ColorRatio block) start addr l
HI351_write_cmos_sensor(0x30, 0x20); // MCU(ColorRatio block) sram addr h(OTP ColorRatio->xdata ColorRatio)
HI351_write_cmos_sensor(0x31, 0xA6); // MCU (ColorRatio block) sram addr l
HI351_write_cmos_sensor(0x32, 0x01); // Copy reg sram size h(MCU reg size)
HI351_write_cmos_sensor(0x33, 0x00); // Copy reg sram size l
HI351_write_cmos_sensor(0x10, 0x02); // Copy mcu down set(OTP ColorRatio -> xdata ColorRatio)
mdelay(50);
HI351_write_cmos_sensor(0x12, 0x00); // memory mux off
HI351_write_cmos_sensor(0x98, 0x00); // dpc_mem_ctl1
HI351_write_cmos_sensor(0x97, 0x01); // otp_dpc_ctl1

///////////////////////////////////////////
// 30 Page MCU reset, enable setting
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x30);
HI351_write_cmos_sensor(0x10, 0x0a);// mcu reset low  = mcu start!!, mcu clk div = 1/3

///////////////////////////////////////////
// 0 Page
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x0B, 0x02); //PLL lock time
HI351_write_cmos_sensor(0x10, 0x20); //Full
HI351_write_cmos_sensor(0x11, 0x90); //1 frame skip
HI351_write_cmos_sensor(0x13, 0x80);
HI351_write_cmos_sensor(0x14, 0x00);
HI351_write_cmos_sensor(0x15, 0x03);
HI351_write_cmos_sensor(0x17, 0x04); //Parallel, MIPI : 04, JPEG : 0c

HI351_write_cmos_sensor(0x20, 0x00); //Start Height
HI351_write_cmos_sensor(0x21, 0x0a);
HI351_write_cmos_sensor(0x22, 0x00); //Start Width
HI351_write_cmos_sensor(0x23, 0x0c);

HI351_write_cmos_sensor(0x24, 0x06);
HI351_write_cmos_sensor(0x25, 0x00);
HI351_write_cmos_sensor(0x26, 0x08);
HI351_write_cmos_sensor(0x27, 0x00);

HI351_write_cmos_sensor(0x03, 0x00); //Page 0
HI351_write_cmos_sensor(0x50, 0x01); //Hblank 20
HI351_write_cmos_sensor(0x51, 0x40);
HI351_write_cmos_sensor(0x52, 0x00); //Vblank 14
HI351_write_cmos_sensor(0x53, 0x32);
//BLC
HI351_write_cmos_sensor(0x80, 0x02);
HI351_write_cmos_sensor(0x81, 0x87);
HI351_write_cmos_sensor(0x82, 0x28);
HI351_write_cmos_sensor(0x83, 0x08);
HI351_write_cmos_sensor(0x84, 0x8c);
HI351_write_cmos_sensor(0x85, 0x0c); //blc on
HI351_write_cmos_sensor(0x86, 0x00);
HI351_write_cmos_sensor(0x87, 0x00);
HI351_write_cmos_sensor(0x88, 0x98);
HI351_write_cmos_sensor(0x89, 0x10);
HI351_write_cmos_sensor(0x8a, 0x80);
HI351_write_cmos_sensor(0x8b, 0x00);
HI351_write_cmos_sensor(0x8e, 0x80);
HI351_write_cmos_sensor(0x8f, 0x0f);
HI351_write_cmos_sensor(0x90, 0x0f); //BLC_TIME_TH_ON
HI351_write_cmos_sensor(0x91, 0x0f); //BLC_TIME_TH_OFF
HI351_write_cmos_sensor(0x92, 0xa0); //BLC_AG_TH_ON
HI351_write_cmos_sensor(0x93, 0x90); //BLC_AG_TH_OFF
HI351_write_cmos_sensor(0x96, 0xfe); //BLC_OUT_TH
HI351_write_cmos_sensor(0x97, 0xE0); //BLC_OUT_TH
HI351_write_cmos_sensor(0x98, 0x20);
HI351_write_cmos_sensor(0xa1, 0x81); //odd_adj_out
HI351_write_cmos_sensor(0xa2, 0x82); //odd_adj_in
HI351_write_cmos_sensor(0xa3, 0x85); //odd_adj_dark
HI351_write_cmos_sensor(0xa5, 0x81); //even_adj_out
HI351_write_cmos_sensor(0xa6, 0x82); //even_adj_in
HI351_write_cmos_sensor(0xa7, 0x85); //even_adj_dark
HI351_write_cmos_sensor(0xbb, 0x20);

///////////////////////////////////////////
// 2 Page
///////////////////////////////////////////

HI351_write_cmos_sensor(0x03, 0x02);
HI351_write_cmos_sensor(0x10, 0x00);
HI351_write_cmos_sensor(0x13, 0x00);
HI351_write_cmos_sensor(0x14, 0x00);
HI351_write_cmos_sensor(0x15, 0x08);
HI351_write_cmos_sensor(0x1a, 0x00);//ncp adaptive off
HI351_write_cmos_sensor(0x1b, 0x00);
HI351_write_cmos_sensor(0x1c, 0xc0);
HI351_write_cmos_sensor(0x1d, 0x00);//MCU update bit[4]
HI351_write_cmos_sensor(0x20, 0x44);
HI351_write_cmos_sensor(0x21, 0x02);
HI351_write_cmos_sensor(0x22, 0x22);
HI351_write_cmos_sensor(0x23, 0x30);//clamp on 10 -30
HI351_write_cmos_sensor(0x24, 0x77);
HI351_write_cmos_sensor(0x2b, 0x00);
HI351_write_cmos_sensor(0x2c, 0x0C);
HI351_write_cmos_sensor(0x2d, 0x80);
HI351_write_cmos_sensor(0x2e, 0x00);
HI351_write_cmos_sensor(0x2f, 0x00);
HI351_write_cmos_sensor(0x30, 0x00);
HI351_write_cmos_sensor(0x31, 0xf0);
HI351_write_cmos_sensor(0x32, 0x22);
HI351_write_cmos_sensor(0x33, 0x42); //auto flicker
HI351_write_cmos_sensor(0x34, 0x30);
HI351_write_cmos_sensor(0x35, 0x00);
HI351_write_cmos_sensor(0x36, 0x08);
HI351_write_cmos_sensor(0x37, 0x40); //auto flicker
HI351_write_cmos_sensor(0x38, 0x14);
HI351_write_cmos_sensor(0x39, 0x02);
HI351_write_cmos_sensor(0x3a, 0x00);

HI351_write_cmos_sensor(0x3d, 0x70);
HI351_write_cmos_sensor(0x3e, 0x04);
HI351_write_cmos_sensor(0x3f, 0x00);
HI351_write_cmos_sensor(0x40, 0x01);
HI351_write_cmos_sensor(0x41, 0x8a);
HI351_write_cmos_sensor(0x42, 0x00);
HI351_write_cmos_sensor(0x43, 0x25);
HI351_write_cmos_sensor(0x44, 0x00);
HI351_write_cmos_sensor(0x46, 0x00);
HI351_write_cmos_sensor(0x47, 0x00);
HI351_write_cmos_sensor(0x48, 0x3C);
HI351_write_cmos_sensor(0x49, 0x10);
HI351_write_cmos_sensor(0x4a, 0x00);
HI351_write_cmos_sensor(0x4b, 0x10);
HI351_write_cmos_sensor(0x4c, 0x08);
HI351_write_cmos_sensor(0x4d, 0x70);
HI351_write_cmos_sensor(0x4e, 0x04);
HI351_write_cmos_sensor(0x4f, 0x38);
HI351_write_cmos_sensor(0x50, 0xa0);
HI351_write_cmos_sensor(0x51, 0x00);
HI351_write_cmos_sensor(0x52, 0x70);
HI351_write_cmos_sensor(0x53, 0x00);
HI351_write_cmos_sensor(0x54, 0xc0);
HI351_write_cmos_sensor(0x55, 0x40);
HI351_write_cmos_sensor(0x56, 0x11);
HI351_write_cmos_sensor(0x57, 0x00);
HI351_write_cmos_sensor(0x58, 0x10);
HI351_write_cmos_sensor(0x59, 0x0E);
HI351_write_cmos_sensor(0x5a, 0x00);
HI351_write_cmos_sensor(0x5b, 0x00);
HI351_write_cmos_sensor(0x5c, 0x00);
HI351_write_cmos_sensor(0x5d, 0x00);
HI351_write_cmos_sensor(0x60, 0x04);
HI351_write_cmos_sensor(0x61, 0xe2);
HI351_write_cmos_sensor(0x62, 0x00);
HI351_write_cmos_sensor(0x63, 0xc8);
HI351_write_cmos_sensor(0x64, 0x00);
HI351_write_cmos_sensor(0x65, 0x00);
HI351_write_cmos_sensor(0x66, 0x00);
HI351_write_cmos_sensor(0x67, 0x3f);
HI351_write_cmos_sensor(0x68, 0x3f);
HI351_write_cmos_sensor(0x69, 0x3f);
HI351_write_cmos_sensor(0x6a, 0x04);
HI351_write_cmos_sensor(0x6b, 0x38);
HI351_write_cmos_sensor(0x6c, 0x00);
HI351_write_cmos_sensor(0x6d, 0x00);
HI351_write_cmos_sensor(0x6e, 0x00);
HI351_write_cmos_sensor(0x6f, 0x00);
HI351_write_cmos_sensor(0x70, 0x00);
HI351_write_cmos_sensor(0x71, 0x50);
HI351_write_cmos_sensor(0x72, 0x05);
HI351_write_cmos_sensor(0x73, 0xa5);
HI351_write_cmos_sensor(0x74, 0x00);
HI351_write_cmos_sensor(0x75, 0x50);
HI351_write_cmos_sensor(0x76, 0x02);
HI351_write_cmos_sensor(0x77, 0xfa);
HI351_write_cmos_sensor(0x78, 0x01);
HI351_write_cmos_sensor(0x79, 0xb4);
HI351_write_cmos_sensor(0x7a, 0x01);
HI351_write_cmos_sensor(0x7b, 0xb8);
HI351_write_cmos_sensor(0x7c, 0x00);
HI351_write_cmos_sensor(0x7d, 0x00);
HI351_write_cmos_sensor(0x7e, 0x00);
HI351_write_cmos_sensor(0x7f, 0x00);
HI351_write_cmos_sensor(0xa0, 0x00);
HI351_write_cmos_sensor(0xa1, 0xEB);
HI351_write_cmos_sensor(0xa2, 0x02);
HI351_write_cmos_sensor(0xa3, 0x2D);
HI351_write_cmos_sensor(0xa4, 0x02);
HI351_write_cmos_sensor(0xa5, 0xB9);
HI351_write_cmos_sensor(0xa6, 0x05);
HI351_write_cmos_sensor(0xa7, 0xED);
HI351_write_cmos_sensor(0xa8, 0x00);
HI351_write_cmos_sensor(0xa9, 0xEB);
HI351_write_cmos_sensor(0xaa, 0x01);
HI351_write_cmos_sensor(0xab, 0xED);
HI351_write_cmos_sensor(0xac, 0x02);
HI351_write_cmos_sensor(0xad, 0x79);
HI351_write_cmos_sensor(0xae, 0x04);
HI351_write_cmos_sensor(0xaf, 0x2D);
HI351_write_cmos_sensor(0xb0, 0x00);
HI351_write_cmos_sensor(0xb1, 0x56);
HI351_write_cmos_sensor(0xb2, 0x01);
HI351_write_cmos_sensor(0xb3, 0x08);
HI351_write_cmos_sensor(0xb4, 0x00);
HI351_write_cmos_sensor(0xb5, 0x2B);
HI351_write_cmos_sensor(0xb6, 0x03);
HI351_write_cmos_sensor(0xb7, 0x2B);
HI351_write_cmos_sensor(0xb8, 0x00);
HI351_write_cmos_sensor(0xb9, 0x56);
HI351_write_cmos_sensor(0xba, 0x00);
HI351_write_cmos_sensor(0xbb, 0xC8);
HI351_write_cmos_sensor(0xbc, 0x00);
HI351_write_cmos_sensor(0xbd, 0x2B);
HI351_write_cmos_sensor(0xbe, 0x01);
HI351_write_cmos_sensor(0xbf, 0xAB);
HI351_write_cmos_sensor(0xc0, 0x00);
HI351_write_cmos_sensor(0xc1, 0x54);
HI351_write_cmos_sensor(0xc2, 0x01);
HI351_write_cmos_sensor(0xc3, 0x0A);
HI351_write_cmos_sensor(0xc4, 0x00);
HI351_write_cmos_sensor(0xc5, 0x29);
HI351_write_cmos_sensor(0xc6, 0x03);
HI351_write_cmos_sensor(0xc7, 0x2D);
HI351_write_cmos_sensor(0xc8, 0x00);
HI351_write_cmos_sensor(0xc9, 0x54);
HI351_write_cmos_sensor(0xca, 0x00);
HI351_write_cmos_sensor(0xcb, 0xCA);
HI351_write_cmos_sensor(0xcc, 0x00);
HI351_write_cmos_sensor(0xcd, 0x29);
HI351_write_cmos_sensor(0xce, 0x01);
HI351_write_cmos_sensor(0xcf, 0xAD);
HI351_write_cmos_sensor(0xd0, 0x10);
HI351_write_cmos_sensor(0xd1, 0x14);
HI351_write_cmos_sensor(0xd2, 0x20);
HI351_write_cmos_sensor(0xd3, 0x00);
HI351_write_cmos_sensor(0xd4, 0x0c); //DCDC_TIME_TH_ON
HI351_write_cmos_sensor(0xd5, 0x0c); //DCDC_TIME_TH_OFF
HI351_write_cmos_sensor(0xd6, 0x78); //DCDC_AG_TH_ON
HI351_write_cmos_sensor(0xd7, 0x70); //DCDC_AG_TH_OFF
HI351_write_cmos_sensor(0xE0, 0xf0);//ncp adaptive
HI351_write_cmos_sensor(0xE1, 0xf0);//ncp adaptive
HI351_write_cmos_sensor(0xE2, 0xf0);//ncp adaptive
HI351_write_cmos_sensor(0xE3, 0xf0);//ncp adaptive
HI351_write_cmos_sensor(0xE4, 0xd0);//ncp adaptive e1 -) d0
HI351_write_cmos_sensor(0xE5, 0x00);//ncp adaptive
HI351_write_cmos_sensor(0xE6, 0x00);
HI351_write_cmos_sensor(0xE7, 0x00);
HI351_write_cmos_sensor(0xE8, 0x00);
HI351_write_cmos_sensor(0xE9, 0x00);
HI351_write_cmos_sensor(0xEA, 0x15);
HI351_write_cmos_sensor(0xEB, 0x15);
HI351_write_cmos_sensor(0xEC, 0x15);
HI351_write_cmos_sensor(0xED, 0x05);
HI351_write_cmos_sensor(0xEE, 0x05);
HI351_write_cmos_sensor(0xEF, 0x65);
HI351_write_cmos_sensor(0xF0, 0x0c);
HI351_write_cmos_sensor(0xF3, 0x05);
HI351_write_cmos_sensor(0xF4, 0x0a);
HI351_write_cmos_sensor(0xF5, 0x05);
HI351_write_cmos_sensor(0xF6, 0x05);
HI351_write_cmos_sensor(0xF7, 0x15);
HI351_write_cmos_sensor(0xF8, 0x15);
HI351_write_cmos_sensor(0xF9, 0x15);
HI351_write_cmos_sensor(0xFA, 0x15);
HI351_write_cmos_sensor(0xFB, 0x15);
HI351_write_cmos_sensor(0xFC, 0x55);
HI351_write_cmos_sensor(0xFD, 0x55);
HI351_write_cmos_sensor(0xFE, 0x05);
HI351_write_cmos_sensor(0xFF, 0x00);
///////////////////////////////////////////
//3Page
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x03);
HI351_write_cmos_sensor(0x10, 0x00);
HI351_write_cmos_sensor(0x11, 0x64);
HI351_write_cmos_sensor(0x12, 0x00);
HI351_write_cmos_sensor(0x13, 0x32);
HI351_write_cmos_sensor(0x14, 0x02);
HI351_write_cmos_sensor(0x15, 0x51);
HI351_write_cmos_sensor(0x16, 0x02);
HI351_write_cmos_sensor(0x17, 0x59);
HI351_write_cmos_sensor(0x18, 0x00);
HI351_write_cmos_sensor(0x19, 0x97);
HI351_write_cmos_sensor(0x1a, 0x01);
HI351_write_cmos_sensor(0x1b, 0x7C);
HI351_write_cmos_sensor(0x1c, 0x00);
HI351_write_cmos_sensor(0x1d, 0x97);
HI351_write_cmos_sensor(0x1e, 0x01);
HI351_write_cmos_sensor(0x1f, 0x7C);
HI351_write_cmos_sensor(0x20, 0x00);
HI351_write_cmos_sensor(0x21, 0x97);
HI351_write_cmos_sensor(0x22, 0x00);
HI351_write_cmos_sensor(0x23, 0xe3); //cds 2 off time sunspot
HI351_write_cmos_sensor(0x24, 0x00);
HI351_write_cmos_sensor(0x25, 0x97);
HI351_write_cmos_sensor(0x26, 0x00);
HI351_write_cmos_sensor(0x27, 0xe3); //cds 2 off time  sunspot

HI351_write_cmos_sensor(0x28, 0x00);
HI351_write_cmos_sensor(0x29, 0x97);
HI351_write_cmos_sensor(0x2a, 0x00);
HI351_write_cmos_sensor(0x2b, 0xE6);
HI351_write_cmos_sensor(0x2c, 0x00);
HI351_write_cmos_sensor(0x2d, 0x97);
HI351_write_cmos_sensor(0x2e, 0x00);
HI351_write_cmos_sensor(0x2f, 0xE6);
HI351_write_cmos_sensor(0x30, 0x00);
HI351_write_cmos_sensor(0x31, 0x0a);
HI351_write_cmos_sensor(0x32, 0x03);
HI351_write_cmos_sensor(0x33, 0x31);
HI351_write_cmos_sensor(0x34, 0x00);
HI351_write_cmos_sensor(0x35, 0x0a);
HI351_write_cmos_sensor(0x36, 0x03);
HI351_write_cmos_sensor(0x37, 0x31);
HI351_write_cmos_sensor(0x38, 0x00);
HI351_write_cmos_sensor(0x39, 0x0A);
HI351_write_cmos_sensor(0x3a, 0x01);
HI351_write_cmos_sensor(0x3b, 0xB0);
HI351_write_cmos_sensor(0x3c, 0x00);
HI351_write_cmos_sensor(0x3d, 0x0A);
HI351_write_cmos_sensor(0x3e, 0x01);
HI351_write_cmos_sensor(0x3f, 0xB0);
HI351_write_cmos_sensor(0x40, 0x00);
HI351_write_cmos_sensor(0x41, 0x04);
HI351_write_cmos_sensor(0x42, 0x00);
HI351_write_cmos_sensor(0x43, 0x1c);
HI351_write_cmos_sensor(0x44, 0x00);
HI351_write_cmos_sensor(0x45, 0x02);
HI351_write_cmos_sensor(0x46, 0x00);
HI351_write_cmos_sensor(0x47, 0x34);
HI351_write_cmos_sensor(0x48, 0x00);
HI351_write_cmos_sensor(0x49, 0x06);
HI351_write_cmos_sensor(0x4a, 0x00);
HI351_write_cmos_sensor(0x4b, 0x1a);
HI351_write_cmos_sensor(0x4c, 0x00);
HI351_write_cmos_sensor(0x4d, 0x06);
HI351_write_cmos_sensor(0x4e, 0x00);
HI351_write_cmos_sensor(0x4f, 0x1a);
HI351_write_cmos_sensor(0x50, 0x00);
HI351_write_cmos_sensor(0x51, 0x08);
HI351_write_cmos_sensor(0x52, 0x00);
HI351_write_cmos_sensor(0x53, 0x18);
HI351_write_cmos_sensor(0x54, 0x00);
HI351_write_cmos_sensor(0x55, 0x08);
HI351_write_cmos_sensor(0x56, 0x00);
HI351_write_cmos_sensor(0x57, 0x18);
HI351_write_cmos_sensor(0x58, 0x00);
HI351_write_cmos_sensor(0x59, 0x08);
HI351_write_cmos_sensor(0x5A, 0x00);
HI351_write_cmos_sensor(0x5b, 0x18);
HI351_write_cmos_sensor(0x5c, 0x00);
HI351_write_cmos_sensor(0x5d, 0x06);
HI351_write_cmos_sensor(0x5e, 0x00);
HI351_write_cmos_sensor(0x5f, 0x1c);
HI351_write_cmos_sensor(0x60, 0x00);
HI351_write_cmos_sensor(0x61, 0x00);
HI351_write_cmos_sensor(0x62, 0x00);
HI351_write_cmos_sensor(0x63, 0x00);
HI351_write_cmos_sensor(0x64, 0x00);
HI351_write_cmos_sensor(0x65, 0x00);
HI351_write_cmos_sensor(0x66, 0x00);
HI351_write_cmos_sensor(0x67, 0x00);
HI351_write_cmos_sensor(0x68, 0x00);
HI351_write_cmos_sensor(0x69, 0x02);
HI351_write_cmos_sensor(0x6A, 0x00);
HI351_write_cmos_sensor(0x6B, 0x1e);
HI351_write_cmos_sensor(0x6C, 0x00);
HI351_write_cmos_sensor(0x6D, 0x00);
HI351_write_cmos_sensor(0x6E, 0x00);
HI351_write_cmos_sensor(0x6F, 0x00);
HI351_write_cmos_sensor(0x70, 0x00);
HI351_write_cmos_sensor(0x71, 0x66);
HI351_write_cmos_sensor(0x72, 0x01);
HI351_write_cmos_sensor(0x73, 0x86);
HI351_write_cmos_sensor(0x74, 0x00);
HI351_write_cmos_sensor(0x75, 0x6B);
HI351_write_cmos_sensor(0x76, 0x00);
HI351_write_cmos_sensor(0x77, 0x93);
HI351_write_cmos_sensor(0x78, 0x01);
HI351_write_cmos_sensor(0x79, 0x84);
HI351_write_cmos_sensor(0x7a, 0x01);
HI351_write_cmos_sensor(0x7b, 0x88);
HI351_write_cmos_sensor(0x7c, 0x01);
HI351_write_cmos_sensor(0x7d, 0x84);
HI351_write_cmos_sensor(0x7e, 0x01);
HI351_write_cmos_sensor(0x7f, 0x88);
HI351_write_cmos_sensor(0x80, 0x01);
HI351_write_cmos_sensor(0x81, 0x13);
HI351_write_cmos_sensor(0x82, 0x01);
HI351_write_cmos_sensor(0x83, 0x3B);
HI351_write_cmos_sensor(0x84, 0x01);
HI351_write_cmos_sensor(0x85, 0x84);
HI351_write_cmos_sensor(0x86, 0x01);
HI351_write_cmos_sensor(0x87, 0x88);
HI351_write_cmos_sensor(0x88, 0x01);
HI351_write_cmos_sensor(0x89, 0x84);
HI351_write_cmos_sensor(0x8a, 0x01);
HI351_write_cmos_sensor(0x8b, 0x88);
HI351_write_cmos_sensor(0x8c, 0x01);
HI351_write_cmos_sensor(0x8d, 0x16);
HI351_write_cmos_sensor(0x8e, 0x01);
HI351_write_cmos_sensor(0x8f, 0x42);
HI351_write_cmos_sensor(0x90, 0x00);
HI351_write_cmos_sensor(0x91, 0x68);
HI351_write_cmos_sensor(0x92, 0x01);
HI351_write_cmos_sensor(0x93, 0x80);
HI351_write_cmos_sensor(0x94, 0x00);
HI351_write_cmos_sensor(0x95, 0x68);
HI351_write_cmos_sensor(0x96, 0x01);
HI351_write_cmos_sensor(0x97, 0x80);
HI351_write_cmos_sensor(0x98, 0x01);
HI351_write_cmos_sensor(0x99, 0x80);
HI351_write_cmos_sensor(0x9a, 0x00);
HI351_write_cmos_sensor(0x9b, 0x68);
HI351_write_cmos_sensor(0x9c, 0x01);
HI351_write_cmos_sensor(0x9d, 0x80);
HI351_write_cmos_sensor(0x9e, 0x00);
HI351_write_cmos_sensor(0x9f, 0x68);
HI351_write_cmos_sensor(0xa0, 0x00);
HI351_write_cmos_sensor(0xa1, 0x08);
HI351_write_cmos_sensor(0xa2, 0x00);
HI351_write_cmos_sensor(0xa3, 0x04);
HI351_write_cmos_sensor(0xa4, 0x00);
HI351_write_cmos_sensor(0xa5, 0x08);
HI351_write_cmos_sensor(0xa6, 0x00);
HI351_write_cmos_sensor(0xa7, 0x04);
HI351_write_cmos_sensor(0xa8, 0x00);
HI351_write_cmos_sensor(0xa9, 0x73);
HI351_write_cmos_sensor(0xaa, 0x00);
HI351_write_cmos_sensor(0xab, 0x64);
HI351_write_cmos_sensor(0xac, 0x00);
HI351_write_cmos_sensor(0xad, 0x73);
HI351_write_cmos_sensor(0xae, 0x00);
HI351_write_cmos_sensor(0xaf, 0x64);
HI351_write_cmos_sensor(0xc0, 0x00);
HI351_write_cmos_sensor(0xc1, 0x1d);
HI351_write_cmos_sensor(0xc2, 0x00);
HI351_write_cmos_sensor(0xc3, 0x2f);
HI351_write_cmos_sensor(0xc4, 0x00);
HI351_write_cmos_sensor(0xc5, 0x1d);
HI351_write_cmos_sensor(0xc6, 0x00);
HI351_write_cmos_sensor(0xc7, 0x2f);
HI351_write_cmos_sensor(0xc8, 0x00);
HI351_write_cmos_sensor(0xc9, 0x1f);
HI351_write_cmos_sensor(0xca, 0x00);
HI351_write_cmos_sensor(0xcb, 0x2d);
HI351_write_cmos_sensor(0xcc, 0x00);
HI351_write_cmos_sensor(0xcd, 0x1f);
HI351_write_cmos_sensor(0xce, 0x00);
HI351_write_cmos_sensor(0xcf, 0x2d);
HI351_write_cmos_sensor(0xd0, 0x00);
HI351_write_cmos_sensor(0xd1, 0x21);
HI351_write_cmos_sensor(0xd2, 0x00);
HI351_write_cmos_sensor(0xd3, 0x2b);
HI351_write_cmos_sensor(0xd4, 0x00);
HI351_write_cmos_sensor(0xd5, 0x21);
HI351_write_cmos_sensor(0xd6, 0x00);
HI351_write_cmos_sensor(0xd7, 0x2b);
HI351_write_cmos_sensor(0xd8, 0x00);
HI351_write_cmos_sensor(0xd9, 0x23);
HI351_write_cmos_sensor(0xdA, 0x00);
HI351_write_cmos_sensor(0xdB, 0x29);
HI351_write_cmos_sensor(0xdC, 0x00);
HI351_write_cmos_sensor(0xdD, 0x23);
HI351_write_cmos_sensor(0xdE, 0x00);
HI351_write_cmos_sensor(0xdF, 0x29);
HI351_write_cmos_sensor(0xe0, 0x00);
HI351_write_cmos_sensor(0xe1, 0x6B);
HI351_write_cmos_sensor(0xe2, 0x00);
HI351_write_cmos_sensor(0xe3, 0xE8);
HI351_write_cmos_sensor(0xe4, 0x00);
HI351_write_cmos_sensor(0xe5, 0xEB);
HI351_write_cmos_sensor(0xe6, 0x01);
HI351_write_cmos_sensor(0xe7, 0x7E);
HI351_write_cmos_sensor(0xe8, 0x00);
HI351_write_cmos_sensor(0xe9, 0x95);
HI351_write_cmos_sensor(0xea, 0x00);
HI351_write_cmos_sensor(0xeb, 0xF1);
HI351_write_cmos_sensor(0xec, 0x00);
HI351_write_cmos_sensor(0xed, 0xdd);
HI351_write_cmos_sensor(0xee, 0x00);
HI351_write_cmos_sensor(0xef, 0x00);

HI351_write_cmos_sensor(0xf0, 0x00);
HI351_write_cmos_sensor(0xf1, 0x34);
HI351_write_cmos_sensor(0xf2, 0x00);

///////////////////////////////////////////
// 10 Page
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x10);
HI351_write_cmos_sensor(0xe0, 0xff); //don't touch
HI351_write_cmos_sensor(0xe1, 0x3f); //don't touch
HI351_write_cmos_sensor(0xe2, 0xff); //don't touch
HI351_write_cmos_sensor(0xe3, 0xff); //don't touch
HI351_write_cmos_sensor(0xe4, 0xf7); //don't touch
HI351_write_cmos_sensor(0xe5, 0x79); //don't touch
HI351_write_cmos_sensor(0xe6, 0xce); //don't touch
HI351_write_cmos_sensor(0xe7, 0x1f); //don't touch
HI351_write_cmos_sensor(0xe8, 0x5f); //don't touch
HI351_write_cmos_sensor(0xe9, 0x00); // don't touch update
HI351_write_cmos_sensor(0xea, 0x00); // don't touch update
HI351_write_cmos_sensor(0xeb, 0x00); // don't touch update
HI351_write_cmos_sensor(0xec, 0x00); // don't touch update
HI351_write_cmos_sensor(0xed, 0x00); // don't touch update
HI351_write_cmos_sensor(0xf0, 0x3f); // don't touch update
HI351_write_cmos_sensor(0xf1, 0x00); // don't touch update
HI351_write_cmos_sensor(0xf2, 0x40); // don't touch update
HI351_write_cmos_sensor(0x10, 0x03); //YUV422-YUYV
HI351_write_cmos_sensor(0x12, 0x20); //0x30); //0220 test dyoffset on//0x10); //Y,DY offset Enb
HI351_write_cmos_sensor(0x13, 0x0a); //Bright2, Contrast Enb
HI351_write_cmos_sensor(0x20, 0x80);
HI351_write_cmos_sensor(0x48, 0x74); // contrast
HI351_write_cmos_sensor(0x50, 0x90); //0220 add, AGbgt//
HI351_write_cmos_sensor(0x60, 0x03); //Sat, Trans Enb
HI351_write_cmos_sensor(0x61, 0x7c);
HI351_write_cmos_sensor(0x62, 0x7c);
//Desat - Chroma
HI351_write_cmos_sensor(0x70, 0x0c);
HI351_write_cmos_sensor(0x71, 0x00);
HI351_write_cmos_sensor(0x72, 0x7a);
HI351_write_cmos_sensor(0x73, 0x28);
HI351_write_cmos_sensor(0x74, 0x14);
HI351_write_cmos_sensor(0x75, 0x0d);
HI351_write_cmos_sensor(0x76, 0x40);
HI351_write_cmos_sensor(0x77, 0x49);
HI351_write_cmos_sensor(0x78, 0x99);
HI351_write_cmos_sensor(0x79, 0x4c);
HI351_write_cmos_sensor(0x7a, 0xcc);
HI351_write_cmos_sensor(0x7b, 0x49);
HI351_write_cmos_sensor(0x7c, 0x99);
HI351_write_cmos_sensor(0x7d, 0x14);
HI351_write_cmos_sensor(0x7e, 0x28);
HI351_write_cmos_sensor(0x7f, 0x50);

HI351_write_cmos_sensor(0xe0, 0xff); //don't touch
HI351_write_cmos_sensor(0xe1, 0x3f); //don't touch
HI351_write_cmos_sensor(0xe2, 0xff); //don't touch
HI351_write_cmos_sensor(0xe3, 0xff); //don't touch
HI351_write_cmos_sensor(0xe4, 0xf7); //don't touch
HI351_write_cmos_sensor(0xe5, 0x79); //don't touch
HI351_write_cmos_sensor(0xe6, 0xce); //don't touch
HI351_write_cmos_sensor(0xe7, 0x1f); //don't touch
HI351_write_cmos_sensor(0xe8, 0x5f); //don't touch
HI351_write_cmos_sensor(0xf0, 0x3f); //don't touch

///////////////////////////////////////////
// 11 page D-LPF
///////////////////////////////////////////
//DLPF
HI351_write_cmos_sensor(0x03, 0x11);
HI351_write_cmos_sensor(0x10, 0x13); //DLPF
HI351_write_cmos_sensor(0xf0, 0x40); //lpf_auto_ctl1
HI351_write_cmos_sensor(0xf2, 0x6e); //LPF_AG_TH_ON
HI351_write_cmos_sensor(0xf3, 0x64); //LPF_AG_TH_OFF
HI351_write_cmos_sensor(0xf4, 0xfe); //lpf_out_th_h
HI351_write_cmos_sensor(0xf5, 0xfd); //lpf_out_th_lo

HI351_write_cmos_sensor(0xf6, 0x00); //lpf_ymean_th_hi
HI351_write_cmos_sensor(0xf7, 0x00); //lpf_ymean_th_lo

// STEVE Luminanace level setting
HI351_write_cmos_sensor(0x32, 0x8b);
HI351_write_cmos_sensor(0x33, 0x54);
HI351_write_cmos_sensor(0x34, 0x2c);
HI351_write_cmos_sensor(0x35, 0x29);
HI351_write_cmos_sensor(0x36, 0x18);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x38, 0x17);
///////////////////////////////////////////
// 12 page DPC / GBGR /LensDebulr
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x12);
HI351_write_cmos_sensor(0x12, 0x08);
HI351_write_cmos_sensor(0x2b, 0x08); //white
HI351_write_cmos_sensor(0x2c, 0x08); //mid_high
HI351_write_cmos_sensor(0x2d, 0x08); //mid_low
HI351_write_cmos_sensor(0x2e, 0x06); //dark

HI351_write_cmos_sensor(0x33, 0x09);
HI351_write_cmos_sensor(0x35, 0x03);
HI351_write_cmos_sensor(0x36, 0x0f);
HI351_write_cmos_sensor(0x37, 0x0d);
HI351_write_cmos_sensor(0x38, 0x02);

HI351_write_cmos_sensor(0x60, 0x21);
HI351_write_cmos_sensor(0x61, 0x0e);
HI351_write_cmos_sensor(0x62, 0x70);
HI351_write_cmos_sensor(0x63, 0x70);
HI351_write_cmos_sensor(0x65, 0x01);

HI351_write_cmos_sensor(0xE1, 0x58);
HI351_write_cmos_sensor(0xEC, 0x32);
HI351_write_cmos_sensor(0xEE, 0x03);

///////////////////////////////////////////
// 13 page YC2D LPF
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x13);

HI351_write_cmos_sensor(0x10, 0x33); //Don't touch
HI351_write_cmos_sensor(0xa0, 0x0f); //Don't touch

HI351_write_cmos_sensor(0xe1, 0x07);


///////////////////////////////////////////
// 14 page Sharpness
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x14);

HI351_write_cmos_sensor(0x10, 0x2f); //Don't touch   //preview turn off sharpness  capture turn on //off: 00  on: 27
HI351_write_cmos_sensor(0x11, 0x02); //Don't touch
HI351_write_cmos_sensor(0x12, 0x40); //Don't touch
HI351_write_cmos_sensor(0x20, 0x82); //Don't touch
HI351_write_cmos_sensor(0x30, 0x82); //Don't touch
HI351_write_cmos_sensor(0x40, 0x84); //Don't touch
HI351_write_cmos_sensor(0x50, 0x84); //Don't touch

mdelay(10);

///////////////////////////////////////////
// 15 Page LSC off
/////////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x15);
HI351_write_cmos_sensor(0x10, 0x82);
HI351_write_cmos_sensor(0x03, 0x07);
HI351_write_cmos_sensor(0x12, 0x04);
HI351_write_cmos_sensor(0x34, 0x00);
HI351_write_cmos_sensor(0x35, 0x00);
HI351_write_cmos_sensor(0x13, 0x85);
HI351_write_cmos_sensor(0x13, 0x05);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x50);
HI351_write_cmos_sensor(0x37, 0x4f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x4f);
HI351_write_cmos_sensor(0x37, 0x50);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x35);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x35);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x46);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x1b);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x1b);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x46);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x17);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x17);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x17);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x17);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x46);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x1b);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x1b);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x46);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x35);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x35);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x50);
HI351_write_cmos_sensor(0x37, 0x4f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x4f);
HI351_write_cmos_sensor(0x37, 0x50);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x63);
HI351_write_cmos_sensor(0x37, 0x62);
HI351_write_cmos_sensor(0x37, 0x5b);
HI351_write_cmos_sensor(0x37, 0x53);
HI351_write_cmos_sensor(0x37, 0x4a);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x3b);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x34);
HI351_write_cmos_sensor(0x37, 0x34);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x3b);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x4a);
HI351_write_cmos_sensor(0x37, 0x53);
HI351_write_cmos_sensor(0x37, 0x5b);
HI351_write_cmos_sensor(0x37, 0x62);
HI351_write_cmos_sensor(0x37, 0x63);
HI351_write_cmos_sensor(0x37, 0x62);
HI351_write_cmos_sensor(0x37, 0x60);
HI351_write_cmos_sensor(0x37, 0x57);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x3a);
HI351_write_cmos_sensor(0x37, 0x33);
HI351_write_cmos_sensor(0x37, 0x2e);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x2e);
HI351_write_cmos_sensor(0x37, 0x33);
HI351_write_cmos_sensor(0x37, 0x3a);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x57);
HI351_write_cmos_sensor(0x37, 0x60);
HI351_write_cmos_sensor(0x37, 0x62);
HI351_write_cmos_sensor(0x37, 0x5c);
HI351_write_cmos_sensor(0x37, 0x58);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x1f);
HI351_write_cmos_sensor(0x37, 0x1d);
HI351_write_cmos_sensor(0x37, 0x1d);
HI351_write_cmos_sensor(0x37, 0x1f);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x58);
HI351_write_cmos_sensor(0x37, 0x5c);
HI351_write_cmos_sensor(0x37, 0x55);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x19);
HI351_write_cmos_sensor(0x37, 0x13);
HI351_write_cmos_sensor(0x37, 0x11);
HI351_write_cmos_sensor(0x37, 0x11);
HI351_write_cmos_sensor(0x37, 0x13);
HI351_write_cmos_sensor(0x37, 0x19);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x55);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x08);
HI351_write_cmos_sensor(0x37, 0x08);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x47);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x05);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x05);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x47);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x45);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x45);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x45);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x45);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x47);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x05);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x05);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x47);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x08);
HI351_write_cmos_sensor(0x37, 0x08);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x55);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x19);
HI351_write_cmos_sensor(0x37, 0x13);
HI351_write_cmos_sensor(0x37, 0x11);
HI351_write_cmos_sensor(0x37, 0x11);
HI351_write_cmos_sensor(0x37, 0x13);
HI351_write_cmos_sensor(0x37, 0x19);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x55);
HI351_write_cmos_sensor(0x37, 0x5c);
HI351_write_cmos_sensor(0x37, 0x58);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x1f);
HI351_write_cmos_sensor(0x37, 0x1d);
HI351_write_cmos_sensor(0x37, 0x1d);
HI351_write_cmos_sensor(0x37, 0x1f);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x58);
HI351_write_cmos_sensor(0x37, 0x5c);
HI351_write_cmos_sensor(0x37, 0x62);
HI351_write_cmos_sensor(0x37, 0x60);
HI351_write_cmos_sensor(0x37, 0x57);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x3a);
HI351_write_cmos_sensor(0x37, 0x33);
HI351_write_cmos_sensor(0x37, 0x2e);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x2e);
HI351_write_cmos_sensor(0x37, 0x33);
HI351_write_cmos_sensor(0x37, 0x3a);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x4d);
HI351_write_cmos_sensor(0x37, 0x57);
HI351_write_cmos_sensor(0x37, 0x60);
HI351_write_cmos_sensor(0x37, 0x62);
HI351_write_cmos_sensor(0x37, 0x63);
HI351_write_cmos_sensor(0x37, 0x62);
HI351_write_cmos_sensor(0x37, 0x5b);
HI351_write_cmos_sensor(0x37, 0x53);
HI351_write_cmos_sensor(0x37, 0x4a);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x3b);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x34);
HI351_write_cmos_sensor(0x37, 0x34);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x3b);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x4a);
HI351_write_cmos_sensor(0x37, 0x53);
HI351_write_cmos_sensor(0x37, 0x5b);
HI351_write_cmos_sensor(0x37, 0x62);
HI351_write_cmos_sensor(0x37, 0x63);
HI351_write_cmos_sensor(0x37, 0x40);
HI351_write_cmos_sensor(0x37, 0x41);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x31);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x31);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x41);
HI351_write_cmos_sensor(0x37, 0x40);
HI351_write_cmos_sensor(0x37, 0x40);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x33);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x33);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x40);
HI351_write_cmos_sensor(0x37, 0x3b);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x1d);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x1d);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x3b);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x34);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x34);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x16);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x05);
HI351_write_cmos_sensor(0x37, 0x05);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x16);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x0b);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x01);
HI351_write_cmos_sensor(0x37, 0x01);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x0b);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x2e);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x01);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x01);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x2e);
HI351_write_cmos_sensor(0x37, 0x2e);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x01);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x01);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x2e);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x0b);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x01);
HI351_write_cmos_sensor(0x37, 0x01);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x0b);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x16);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x05);
HI351_write_cmos_sensor(0x37, 0x05);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x16);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x2f);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x34);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x0a);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x34);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x3b);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x1d);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x12);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x1d);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x3b);
HI351_write_cmos_sensor(0x37, 0x40);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x33);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x27);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x33);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x40);
HI351_write_cmos_sensor(0x37, 0x40);
HI351_write_cmos_sensor(0x37, 0x41);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x31);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x21);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x31);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x41);
HI351_write_cmos_sensor(0x37, 0x40);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x50);
HI351_write_cmos_sensor(0x37, 0x4f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x4f);
HI351_write_cmos_sensor(0x37, 0x50);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x35);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x35);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x46);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x1b);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x1b);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x46);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x17);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x17);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x06);
HI351_write_cmos_sensor(0x37, 0x0c);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x20);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x17);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x02);
HI351_write_cmos_sensor(0x37, 0x04);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x0f);
HI351_write_cmos_sensor(0x37, 0x17);
HI351_write_cmos_sensor(0x37, 0x22);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x39);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x07);
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x14);
HI351_write_cmos_sensor(0x37, 0x1c);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x32);
HI351_write_cmos_sensor(0x37, 0x3d);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x46);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x1b);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x0e);
HI351_write_cmos_sensor(0x37, 0x10);
HI351_write_cmos_sensor(0x37, 0x15);
HI351_write_cmos_sensor(0x37, 0x1b);
HI351_write_cmos_sensor(0x37, 0x23);
HI351_write_cmos_sensor(0x37, 0x2d);
HI351_write_cmos_sensor(0x37, 0x38);
HI351_write_cmos_sensor(0x37, 0x42);
HI351_write_cmos_sensor(0x37, 0x46);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x35);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x18);
HI351_write_cmos_sensor(0x37, 0x1a);
HI351_write_cmos_sensor(0x37, 0x1e);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x35);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x50);
HI351_write_cmos_sensor(0x37, 0x4f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x24);
HI351_write_cmos_sensor(0x37, 0x26);
HI351_write_cmos_sensor(0x37, 0x2a);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x37);
HI351_write_cmos_sensor(0x37, 0x3f);
HI351_write_cmos_sensor(0x37, 0x48);
HI351_write_cmos_sensor(0x37, 0x4f);
HI351_write_cmos_sensor(0x37, 0x50);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x2b);
HI351_write_cmos_sensor(0x37, 0x2c);
HI351_write_cmos_sensor(0x37, 0x30);
HI351_write_cmos_sensor(0x37, 0x36);
HI351_write_cmos_sensor(0x37, 0x3c);
HI351_write_cmos_sensor(0x37, 0x44);
HI351_write_cmos_sensor(0x37, 0x4b);
HI351_write_cmos_sensor(0x37, 0x51);
HI351_write_cmos_sensor(0x37, 0x51);

HI351_write_cmos_sensor(0x12, 0x00);
HI351_write_cmos_sensor(0x13, 0x00);

HI351_write_cmos_sensor(0x03, 0x15);
HI351_write_cmos_sensor(0x10, 0x83); // LSC ON

///////////////////////////////////////////
// 16 Page CMC
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x16);

HI351_write_cmos_sensor(0x10, 0x0f); //cmc
HI351_write_cmos_sensor(0x60, 0xff); //mcmc
//automatic saturation
HI351_write_cmos_sensor(0x8a, 0x68);
HI351_write_cmos_sensor(0x8b, 0x7c);
HI351_write_cmos_sensor(0x8c, 0x7f);
HI351_write_cmos_sensor(0x8d, 0x7f);
HI351_write_cmos_sensor(0x8e, 0x7f);
HI351_write_cmos_sensor(0x8f, 0x7f);
HI351_write_cmos_sensor(0x90, 0x7f);
HI351_write_cmos_sensor(0x91, 0x7f);
HI351_write_cmos_sensor(0x92, 0x7f);
HI351_write_cmos_sensor(0x93, 0x7f);
HI351_write_cmos_sensor(0x94, 0x7a);
HI351_write_cmos_sensor(0x95, 0x78);
HI351_write_cmos_sensor(0x96, 0x74);
HI351_write_cmos_sensor(0x97, 0x70);
HI351_write_cmos_sensor(0x98, 0x6c);
HI351_write_cmos_sensor(0x99, 0x68);
HI351_write_cmos_sensor(0x9a, 0x64);
//WB gain
HI351_write_cmos_sensor(0xa0, 0x81); //Manual WB gain enable
HI351_write_cmos_sensor(0xa1, 0x00);
HI351_write_cmos_sensor(0xa2, 0x58); //R_start_gain
HI351_write_cmos_sensor(0xa3, 0x82); //B_start_gain
HI351_write_cmos_sensor(0xa6, 0xf0); //r max
HI351_write_cmos_sensor(0xa8, 0xf0); //b max
// Pre WB gain setting(after AWB setting)
HI351_write_cmos_sensor(0xF0, 0x01); //Pre WB gain enable Gain resolution_1x
HI351_write_cmos_sensor(0xF1, 0x40);
HI351_write_cmos_sensor(0xF2, 0x40);
HI351_write_cmos_sensor(0xF3, 0x40);
HI351_write_cmos_sensor(0xF4, 0x40);
/////////////////////////////////////////////
// 17 Page Gamma
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x17);
HI351_write_cmos_sensor(0x10, 0x01);

///////////////////////////////////////////
// 18 Page Histogram
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x18);
HI351_write_cmos_sensor(0x10, 0x00);
HI351_write_cmos_sensor(0xc0, 0x01);
HI351_write_cmos_sensor(0xC4, 0x7a); //FLK200 = EXP100/2/Oneline
HI351_write_cmos_sensor(0xC5, 0x66); //FLK240 = EXP120/2/Oneline

///////////////////////////////////////////
// 20 Page AE
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x20);
HI351_write_cmos_sensor(0x10, 0x9f); //M 50hz//0xdf);//auto 50hz flicker select
HI351_write_cmos_sensor(0x12, 0x2d); //Dgain off
HI351_write_cmos_sensor(0x17, 0xa0);
HI351_write_cmos_sensor(0x1f, 0x1f);

HI351_write_cmos_sensor(0x03, 0x20); //Page 20
HI351_write_cmos_sensor(0x20, 0x00); //EXP Normal 10.00 fps 
HI351_write_cmos_sensor(0x21, 0x49); 
HI351_write_cmos_sensor(0x22, 0x3e); 
HI351_write_cmos_sensor(0x23, 0x00); 
HI351_write_cmos_sensor(0x24, 0x00); //EXP Max 6.25 fps 
HI351_write_cmos_sensor(0x25, 0x75); 
HI351_write_cmos_sensor(0x26, 0x30); 
HI351_write_cmos_sensor(0x27, 0x00); 
HI351_write_cmos_sensor(0x28, 0x00); //EXPMin 19200.00 fps
HI351_write_cmos_sensor(0x29, 0x09); 
HI351_write_cmos_sensor(0x2a, 0xc4); 
HI351_write_cmos_sensor(0x30, 0x07); //EXP100 
HI351_write_cmos_sensor(0x31, 0x53); 
HI351_write_cmos_sensor(0x32, 0x00); 
HI351_write_cmos_sensor(0x33, 0x06); //EXP120 
HI351_write_cmos_sensor(0x34, 0x1a); 
HI351_write_cmos_sensor(0x35, 0x80); 
HI351_write_cmos_sensor(0x36, 0x00); //EXP Unit 
HI351_write_cmos_sensor(0x37, 0x09); 
HI351_write_cmos_sensor(0x38, 0xc4); 


HI351_write_cmos_sensor(0x43, 0x04); //pga time th
HI351_write_cmos_sensor(0x51, 0xf0); //pga_max_total
HI351_write_cmos_sensor(0x52, 0x28); //pga_min_total

HI351_write_cmos_sensor(0x71, 0xd0); //DG MAX
HI351_write_cmos_sensor(0x72, 0x80); //DG MIN

HI351_write_cmos_sensor(0x80, 0x30); //ae target

///////////////////////////////////////////
// 30 page MCU Set
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x30); //system setting important
HI351_write_cmos_sensor(0x12, 0x00); //mcu iic setting
HI351_write_cmos_sensor(0x20, 0x08); //need to set before mcu page
HI351_write_cmos_sensor(0x50, 0x00);
HI351_write_cmos_sensor(0xe0, 0x02); //Don't touch
HI351_write_cmos_sensor(0xf0, 0x00);
HI351_write_cmos_sensor(0x11, 0x05); //B[0]: MCU holding
HI351_write_cmos_sensor(0x03, 0xc0);
HI351_write_cmos_sensor(0xe4, 0xA0); //delay

///////////////////////////////////////////
// 30 Page DMA address set
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x30); //DMA
HI351_write_cmos_sensor(0x7c, 0x2c); //Extra str
HI351_write_cmos_sensor(0x7d, 0xce);
HI351_write_cmos_sensor(0x7e, 0x2c); //Extra end
HI351_write_cmos_sensor(0x7f, 0xd1);
HI351_write_cmos_sensor(0x80, 0x24); //Outdoor str
HI351_write_cmos_sensor(0x81, 0x70);
HI351_write_cmos_sensor(0x82, 0x24); //Outdoor end
HI351_write_cmos_sensor(0x83, 0x73);
HI351_write_cmos_sensor(0x84, 0x21); //Indoor str
HI351_write_cmos_sensor(0x85, 0xa6);
HI351_write_cmos_sensor(0x86, 0x21); //Indoor end
HI351_write_cmos_sensor(0x87, 0xa9);
HI351_write_cmos_sensor(0x88, 0x27); //Dark1 str
HI351_write_cmos_sensor(0x89, 0x3a);
HI351_write_cmos_sensor(0x8a, 0x27); //Dark1 end
HI351_write_cmos_sensor(0x8b, 0x3d);
HI351_write_cmos_sensor(0x8c, 0x2a); //Dark2 str
HI351_write_cmos_sensor(0x8d, 0x04);
HI351_write_cmos_sensor(0x8e, 0x2a); //Dark2 end
HI351_write_cmos_sensor(0x8f, 0x07);

HI351_write_cmos_sensor(0x03, 0xC0);
HI351_write_cmos_sensor(0x2F, 0xf0); //DMA busy flag check
HI351_write_cmos_sensor(0x31, 0x20); //Delay before DMA write_setting
HI351_write_cmos_sensor(0x33, 0x20); //DMA full stuck mode
HI351_write_cmos_sensor(0x32, 0x01); //DMA on first

HI351_write_cmos_sensor(0x03, 0xC0);
HI351_write_cmos_sensor(0x2F, 0xf0); //DMA busy flag check
HI351_write_cmos_sensor(0x31, 0x20); //Delay before DMA write_setting
HI351_write_cmos_sensor(0x33, 0x20);
HI351_write_cmos_sensor(0x32, 0x01); //DMA on second

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x10, 0x13); // Preview2
HI351_write_cmos_sensor(0x01, 0xF0); // sleep off

///////////////////////////////////////////
// 30 page
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x30);
HI351_write_cmos_sensor(0xDE, 0x20); //OTP color ratio xdata address set
HI351_write_cmos_sensor(0xDF, 0xA5);
HI351_write_cmos_sensor(0x03, 0xE7);
HI351_write_cmos_sensor(0x1F, 0x18); //OTP color ratio Rg typical set = 6300
HI351_write_cmos_sensor(0x20, 0x9c);
HI351_write_cmos_sensor(0x21, 0x0F); //OTP color ratio Bg typical set = 3920
HI351_write_cmos_sensor(0x22, 0x50);

/////////////////////////////////////////////
// CD Page Adaptive Mode(Color ratio)
/////////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0xCD);//
HI351_write_cmos_sensor(0x47, 0x00);//
HI351_write_cmos_sensor(0x12, 0x40);//
HI351_write_cmos_sensor(0x13, 0x40);//Ratio WB R gain min
HI351_write_cmos_sensor(0x14, 0x48);//Ratio WB R gain max
HI351_write_cmos_sensor(0x15, 0x40);//Ratio WB B gain min
HI351_write_cmos_sensor(0x16, 0x48);//Ratio WB B gain max
HI351_write_cmos_sensor(0x10, 0xB9);//enable

///////////////////////////////////////////
// 1F Page SSD
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x1f); //1F page
HI351_write_cmos_sensor(0x11, 0x00); //bit[5:4]: debug mode
HI351_write_cmos_sensor(0x12, 0x60);
HI351_write_cmos_sensor(0x13, 0x14);
HI351_write_cmos_sensor(0x14, 0x10);
HI351_write_cmos_sensor(0x15, 0x00);
HI351_write_cmos_sensor(0x20, 0x18); //ssd_x_start_pos
HI351_write_cmos_sensor(0x21, 0x14); //ssd_y_start_pos
HI351_write_cmos_sensor(0x22, 0x8C); //ssd_blk_width
HI351_write_cmos_sensor(0x23, 0x60); //ssd_blk_height  //ae & awb input data block  //full size use 9c //pre1 use 60
HI351_write_cmos_sensor(0x28, 0x18);
HI351_write_cmos_sensor(0x29, 0x02);
HI351_write_cmos_sensor(0x3B, 0x18);
HI351_write_cmos_sensor(0x3C, 0x8C);
HI351_write_cmos_sensor(0x10, 0x19); //SSD enable

///////////////////////////////////////////
// C4 Page MCU AE
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0xc4);
HI351_write_cmos_sensor(0x11, 0x30); // ae speed B[7:6] 0 (SLOW) ~ 3 (FAST), 0x70 - 0x30
HI351_write_cmos_sensor(0x12, 0x10);
HI351_write_cmos_sensor(0x19, 0x4a); //0x30); //0x50); //17th//0x30); // band0 gain 40fps 0x2d
HI351_write_cmos_sensor(0x1a, 0x54); //0x38); //0x58); //17th//0x38); // band1 gain 20fps STEVE
HI351_write_cmos_sensor(0x1b, 0x5c); //0x4c); //0x60); //0x6c); //17th//0x4c); // band2 gain 12fps

HI351_write_cmos_sensor(0x1c, 0x04);
HI351_write_cmos_sensor(0x1d, 0x80);

HI351_write_cmos_sensor(0x1e, 0x00); // band1 min exposure time	1/33.33s // correction point
HI351_write_cmos_sensor(0x1f, 0x18);
HI351_write_cmos_sensor(0x20, 0xac);
HI351_write_cmos_sensor(0x21, 0x68);

HI351_write_cmos_sensor(0x22, 0x00); // band2 min exposure time	1/20s
HI351_write_cmos_sensor(0x23, 0x29);
HI351_write_cmos_sensor(0x24, 0x1f);
HI351_write_cmos_sensor(0x25, 0x58);

HI351_write_cmos_sensor(0x26, 0x00);// band3 min exposure time  1/12.5s
HI351_write_cmos_sensor(0x27, 0x41);
HI351_write_cmos_sensor(0x28, 0xcb);
HI351_write_cmos_sensor(0x29, 0xc0);

HI351_write_cmos_sensor(0x36, 0x22); // AE Yth

HI351_write_cmos_sensor(0x66, 0x00); //jktest 0131//manual 50hz
HI351_write_cmos_sensor(0x69, 0x00); //jktest 0131//manual 50hz //auto off

//HI351_write_cmos_sensor(0x03, 0x20);
//HI351_write_cmos_sensor(0x12, 0x2d); // STEVE 2d (AE digital gain OFF)

///////////////////////////////////////////
// c3 Page MCU AE Weight
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0xc3);
HI351_write_cmos_sensor(0x10, 0x00);
HI351_write_cmos_sensor(0x38, 0xFF);
HI351_write_cmos_sensor(0x39, 0xFF);
//AE_CenterWeighted
HI351_write_cmos_sensor(0x70, 0x00);
HI351_write_cmos_sensor(0x71, 0x00);
HI351_write_cmos_sensor(0x72, 0x00);
HI351_write_cmos_sensor(0x73, 0x00);
HI351_write_cmos_sensor(0x74, 0x00);
HI351_write_cmos_sensor(0x75, 0x00);
HI351_write_cmos_sensor(0x76, 0x00);
HI351_write_cmos_sensor(0x77, 0x00);
HI351_write_cmos_sensor(0x78, 0x00);
HI351_write_cmos_sensor(0x79, 0x00);
HI351_write_cmos_sensor(0x7A, 0x00);
HI351_write_cmos_sensor(0x7B, 0x00);
HI351_write_cmos_sensor(0x7C, 0x11);
HI351_write_cmos_sensor(0x7D, 0x11);
HI351_write_cmos_sensor(0x7E, 0x11);
HI351_write_cmos_sensor(0x7F, 0x11);
HI351_write_cmos_sensor(0x80, 0x11);
HI351_write_cmos_sensor(0x81, 0x11);
HI351_write_cmos_sensor(0x82, 0x11);
HI351_write_cmos_sensor(0x83, 0x21);
HI351_write_cmos_sensor(0x84, 0x44);
HI351_write_cmos_sensor(0x85, 0x44);
HI351_write_cmos_sensor(0x86, 0x12);
HI351_write_cmos_sensor(0x87, 0x11);
HI351_write_cmos_sensor(0x88, 0x11);
HI351_write_cmos_sensor(0x89, 0x22);
HI351_write_cmos_sensor(0x8A, 0x64);
HI351_write_cmos_sensor(0x8B, 0x46);
HI351_write_cmos_sensor(0x8C, 0x22);
HI351_write_cmos_sensor(0x8D, 0x11);
HI351_write_cmos_sensor(0x8E, 0x21);
HI351_write_cmos_sensor(0x8F, 0x33);
HI351_write_cmos_sensor(0x90, 0x64);
HI351_write_cmos_sensor(0x91, 0x46);
HI351_write_cmos_sensor(0x92, 0x33);
HI351_write_cmos_sensor(0x93, 0x12);
HI351_write_cmos_sensor(0x94, 0x21);
HI351_write_cmos_sensor(0x95, 0x33);
HI351_write_cmos_sensor(0x96, 0x44);
HI351_write_cmos_sensor(0x97, 0x44);
HI351_write_cmos_sensor(0x98, 0x33);
HI351_write_cmos_sensor(0x99, 0x12);
HI351_write_cmos_sensor(0x9A, 0x21);
HI351_write_cmos_sensor(0x9B, 0x33);
HI351_write_cmos_sensor(0x9C, 0x33);
HI351_write_cmos_sensor(0x9D, 0x33);
HI351_write_cmos_sensor(0x9E, 0x33);
HI351_write_cmos_sensor(0x9F, 0x12);
HI351_write_cmos_sensor(0xA0, 0x11);
HI351_write_cmos_sensor(0xA1, 0x11);
HI351_write_cmos_sensor(0xA2, 0x11);
HI351_write_cmos_sensor(0xA3, 0x11);
HI351_write_cmos_sensor(0xA4, 0x11);
HI351_write_cmos_sensor(0xA5, 0x11);
HI351_write_cmos_sensor(0xE1, 0x29); //Outdoor AG Max
HI351_write_cmos_sensor(0xE2, 0x03);

///////////////////////////////////////////
// Capture Setting
///////////////////////////////////////////

HI351_write_cmos_sensor(0x03, 0xd5);
HI351_write_cmos_sensor(0x11, 0xb9); //manual sleep onoff
HI351_write_cmos_sensor(0x14, 0xfd); // STEVE EXPMIN x2
HI351_write_cmos_sensor(0x1e, 0x02); //capture clock set
HI351_write_cmos_sensor(0x86, 0x04); //preview clock set

HI351_write_cmos_sensor(0x1f, 0x00); //Capture Hblank  20
HI351_write_cmos_sensor(0x20, 0x14);
HI351_write_cmos_sensor(0x21, 0x08); //Capture oneline 2200  //hblank+hline  20+2180
HI351_write_cmos_sensor(0x22, 0x98);

HI351_write_cmos_sensor(0x8c, 0x00); //Preview Hblank  20
HI351_write_cmos_sensor(0x8d, 0x14);
HI351_write_cmos_sensor(0x92, 0x08); //Preview oneline 2200	//hblank+hline  20+2180
HI351_write_cmos_sensor(0x93, 0x98);
HI351_write_cmos_sensor(0x33, 0x00);

///////////////////////////////////////////
// C0 Page Firmware system
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0xc0);
HI351_write_cmos_sensor(0x16, 0x81); //MCU main roof holding on

///////////////////////////////////////////
// DAWB control : Page Mode = 0xC5 ~ 0xC9
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0xc5);
HI351_write_cmos_sensor(0x0e, 0x01); //burst start
HI351_write_cmos_sensor(0x10, 0x30);
HI351_write_cmos_sensor(0x11, 0xa5);
HI351_write_cmos_sensor(0x12, 0x9b);
HI351_write_cmos_sensor(0x13, 0x1d);
HI351_write_cmos_sensor(0x14, 0x02); //bLockTh
HI351_write_cmos_sensor(0x15, 0x0A); //bTraceStep
HI351_write_cmos_sensor(0x16, 0x06); //bTraceStepLmt
HI351_write_cmos_sensor(0x17, 0x08); //bBlkPtBndWdhTh
HI351_write_cmos_sensor(0x18, 0x0a); //bBlkPtBndCntTh
HI351_write_cmos_sensor(0x19, 0x03); //bBlkPtBndCntLmt
HI351_write_cmos_sensor(0x1a, 0xa0); //bBrtUnStbYThhi
HI351_write_cmos_sensor(0x1b, 0x08); //bBrtUnStbYThLo
HI351_write_cmos_sensor(0x1c, 0x10); //bBrtUnStbCntTh
HI351_write_cmos_sensor(0x1d, 0x40); //bGgainDef
HI351_write_cmos_sensor(0x1e, 0x00); //iOut2AglMaxLmt
HI351_write_cmos_sensor(0x1f, 0xd0); //0xc8
HI351_write_cmos_sensor(0x20, 0x00); //iOut2AglMinLmt
HI351_write_cmos_sensor(0x21, 0xa3); //0x9b
HI351_write_cmos_sensor(0x22, 0x00); //iOut1AglMaxLmt
HI351_write_cmos_sensor(0x23, 0xd0); //0xc8
HI351_write_cmos_sensor(0x24, 0x00); //iOut1AglMinLmt
HI351_write_cmos_sensor(0x25, 0xad); //0xa5
HI351_write_cmos_sensor(0x26, 0x01); //iInAglMaxLmt
HI351_write_cmos_sensor(0x27, 0x09);
HI351_write_cmos_sensor(0x28, 0x00); //iInAglMinLmt
HI351_write_cmos_sensor(0x29, 0x55);
HI351_write_cmos_sensor(0x2a, 0x00); //iDakAglMaxLmt
HI351_write_cmos_sensor(0x2b, 0xd2);
HI351_write_cmos_sensor(0x2c, 0x00); //iDakAglMinLmt
HI351_write_cmos_sensor(0x2d, 0x55);
HI351_write_cmos_sensor(0x2e, 0x00); //dwOut2LmtTh
HI351_write_cmos_sensor(0x2f, 0x02);
HI351_write_cmos_sensor(0x30, 0x0f);
HI351_write_cmos_sensor(0x31, 0x58);
HI351_write_cmos_sensor(0x32, 0x00); //dwOut2StrLmtTh
HI351_write_cmos_sensor(0x33, 0x02);
HI351_write_cmos_sensor(0x34, 0x51);
HI351_write_cmos_sensor(0x35, 0x43);
HI351_write_cmos_sensor(0x36, 0x00); //dwOut1LmtTh
HI351_write_cmos_sensor(0x37, 0x09);
HI351_write_cmos_sensor(0x38, 0x45);
HI351_write_cmos_sensor(0x39, 0x0c);
HI351_write_cmos_sensor(0x3a, 0x00); //dwOut1StrLmtTh
HI351_write_cmos_sensor(0x3b, 0x12);
HI351_write_cmos_sensor(0x3c, 0x8a);
HI351_write_cmos_sensor(0x3d, 0x18);
HI351_write_cmos_sensor(0x3e, 0x01); //dwDakStrLmtTh
HI351_write_cmos_sensor(0x3f, 0xee);
HI351_write_cmos_sensor(0x40, 0x62);
HI351_write_cmos_sensor(0x41, 0x80);
HI351_write_cmos_sensor(0x42, 0x02); //dwDakLmtTh
HI351_write_cmos_sensor(0x43, 0x51);
HI351_write_cmos_sensor(0x44, 0x43);
HI351_write_cmos_sensor(0x45, 0x00);
HI351_write_cmos_sensor(0x46, 0x00); //dwOutdoorCondTh
HI351_write_cmos_sensor(0x47, 0x09);
HI351_write_cmos_sensor(0x48, 0x45);
HI351_write_cmos_sensor(0x49, 0x0c);
HI351_write_cmos_sensor(0x4a, 0x00); //dwOutdoorCondOutTh
HI351_write_cmos_sensor(0x4b, 0x12);
HI351_write_cmos_sensor(0x4c, 0x8a);
HI351_write_cmos_sensor(0x4d, 0x18);
HI351_write_cmos_sensor(0x4e, 0x02); //bRgOfs
HI351_write_cmos_sensor(0x4f, 0x82); //bBgOfs
HI351_write_cmos_sensor(0x50, 0x55); //aSsdBlkWgt
HI351_write_cmos_sensor(0x51, 0x55);
HI351_write_cmos_sensor(0x52, 0x55);
HI351_write_cmos_sensor(0x53, 0x55);
HI351_write_cmos_sensor(0x54, 0x55);
HI351_write_cmos_sensor(0x55, 0x55);
HI351_write_cmos_sensor(0x56, 0x55);
HI351_write_cmos_sensor(0x57, 0x55);
HI351_write_cmos_sensor(0x58, 0x55);
HI351_write_cmos_sensor(0x59, 0x55);
HI351_write_cmos_sensor(0x5a, 0xaa);
HI351_write_cmos_sensor(0x5b, 0x55);
HI351_write_cmos_sensor(0x5c, 0x55);
HI351_write_cmos_sensor(0x5d, 0xaa);
HI351_write_cmos_sensor(0x5e, 0x55);
HI351_write_cmos_sensor(0x5f, 0x55);
HI351_write_cmos_sensor(0x60, 0xaa);
HI351_write_cmos_sensor(0x61, 0x55);
HI351_write_cmos_sensor(0x62, 0x55);
HI351_write_cmos_sensor(0x63, 0x55);
HI351_write_cmos_sensor(0x64, 0x55);
HI351_write_cmos_sensor(0x65, 0x55);
HI351_write_cmos_sensor(0x66, 0x55);
HI351_write_cmos_sensor(0x67, 0x55);
HI351_write_cmos_sensor(0x68, 0x55);
HI351_write_cmos_sensor(0x69, 0x55);
HI351_write_cmos_sensor(0x6a, 0x55);
HI351_write_cmos_sensor(0x6b, 0x18); //aInWhtRgnBg_a00
HI351_write_cmos_sensor(0x6c, 0x27);
HI351_write_cmos_sensor(0x6d, 0x2B);
HI351_write_cmos_sensor(0x6e, 0x32);
HI351_write_cmos_sensor(0x6f, 0x35);
HI351_write_cmos_sensor(0x70, 0x3B);
HI351_write_cmos_sensor(0x71, 0x44);
HI351_write_cmos_sensor(0x72, 0x50);
HI351_write_cmos_sensor(0x73, 0x5B);
HI351_write_cmos_sensor(0x74, 0x66);
HI351_write_cmos_sensor(0x75, 0xAF);
HI351_write_cmos_sensor(0x76, 0xC0); //aInWhtRgnRgLeftLmt_a00
HI351_write_cmos_sensor(0x77, 0x83);
HI351_write_cmos_sensor(0x78, 0x71);
HI351_write_cmos_sensor(0x79, 0x5C);
HI351_write_cmos_sensor(0x7a, 0x4E);
HI351_write_cmos_sensor(0x7b, 0x45);
HI351_write_cmos_sensor(0x7c, 0x3e);
HI351_write_cmos_sensor(0x7d, 0x3a);
HI351_write_cmos_sensor(0x7e, 0x37);
HI351_write_cmos_sensor(0x7f, 0x35);
HI351_write_cmos_sensor(0x80, 0x2a);
HI351_write_cmos_sensor(0x81, 0xD0); //aInWhtRgnRgRightLmt_a00
HI351_write_cmos_sensor(0x82, 0xA7);
HI351_write_cmos_sensor(0x83, 0x9F);
HI351_write_cmos_sensor(0x84, 0x86);
HI351_write_cmos_sensor(0x85, 0x7c);
HI351_write_cmos_sensor(0x86, 0x71);
HI351_write_cmos_sensor(0x87, 0x62);
HI351_write_cmos_sensor(0x88, 0x4f);
HI351_write_cmos_sensor(0x89, 0x45);
HI351_write_cmos_sensor(0x8a, 0x3f);
HI351_write_cmos_sensor(0x8b, 0x31);
HI351_write_cmos_sensor(0x8c, 0x18); //aInWhtLineBg_a00
HI351_write_cmos_sensor(0x8d, 0x26);
HI351_write_cmos_sensor(0x8e, 0x29);
HI351_write_cmos_sensor(0x8f, 0x2D);
HI351_write_cmos_sensor(0x90, 0x30);
HI351_write_cmos_sensor(0x91, 0x34);
HI351_write_cmos_sensor(0x92, 0x39);
HI351_write_cmos_sensor(0x93, 0x40);
HI351_write_cmos_sensor(0x94, 0x49);
HI351_write_cmos_sensor(0x95, 0x64);
HI351_write_cmos_sensor(0x96, 0xAF);
HI351_write_cmos_sensor(0x97, 0xC8); //aInWhtLineRg_a00
HI351_write_cmos_sensor(0x98, 0x9F);
HI351_write_cmos_sensor(0x99, 0x90);
HI351_write_cmos_sensor(0x9a, 0x83);
HI351_write_cmos_sensor(0x9b, 0x73);
HI351_write_cmos_sensor(0x9c, 0x68);
HI351_write_cmos_sensor(0x9d, 0x5A);
HI351_write_cmos_sensor(0x9e, 0x50);
HI351_write_cmos_sensor(0x9f, 0x45);
HI351_write_cmos_sensor(0xa0, 0x3b);
HI351_write_cmos_sensor(0xa1, 0x2d);
HI351_write_cmos_sensor(0xa2, 0x31); //aInTgtAngle_a00
HI351_write_cmos_sensor(0xa3, 0x38); //01
HI351_write_cmos_sensor(0xa4, 0x43); //02
HI351_write_cmos_sensor(0xa5, 0x44); //03
HI351_write_cmos_sensor(0xa6, 0x4b); //04
HI351_write_cmos_sensor(0xa7, 0x50); //05
HI351_write_cmos_sensor(0xa8, 0x5a); //06
HI351_write_cmos_sensor(0xa9, 0x6e); //07
HI351_write_cmos_sensor(0xaa, 0x13); //aInRgTgtOfs_a00
HI351_write_cmos_sensor(0xab, 0x03); //01
HI351_write_cmos_sensor(0xac, 0x03); //02
HI351_write_cmos_sensor(0xad, 0x03); //03
HI351_write_cmos_sensor(0xae, 0x00); //04
HI351_write_cmos_sensor(0xaf, 0x00); //05
HI351_write_cmos_sensor(0xb0, 0x85); //06
HI351_write_cmos_sensor(0xb1, 0x85); //07
HI351_write_cmos_sensor(0xb2, 0x82); //aInBgTgtOfs_a00
HI351_write_cmos_sensor(0xb3, 0x82); //01
HI351_write_cmos_sensor(0xb4, 0x82); //02
HI351_write_cmos_sensor(0xb5, 0x82); //03
HI351_write_cmos_sensor(0xb6, 0x00); //04
HI351_write_cmos_sensor(0xb7, 0x81); //05
HI351_write_cmos_sensor(0xb8, 0x81); //06
HI351_write_cmos_sensor(0xb9, 0x81); //07
HI351_write_cmos_sensor(0xba, 0x00); //aInLeftOfs_a00
HI351_write_cmos_sensor(0xbb, 0x00); //01
HI351_write_cmos_sensor(0xbc, 0x00); //02
HI351_write_cmos_sensor(0xbd, 0x00); //03
HI351_write_cmos_sensor(0xbe, 0x00); //04
HI351_write_cmos_sensor(0xbf, 0x00); //05
HI351_write_cmos_sensor(0xc0, 0x00); //06
HI351_write_cmos_sensor(0xc1, 0x00); //07
HI351_write_cmos_sensor(0xc2, 0x00); //aInRightOfs_a00
HI351_write_cmos_sensor(0xc3, 0x00); //01
HI351_write_cmos_sensor(0xc4, 0x00); //02
HI351_write_cmos_sensor(0xc5, 0x00); //03
HI351_write_cmos_sensor(0xc6, 0x00); //04
HI351_write_cmos_sensor(0xc7, 0x00); //05
HI351_write_cmos_sensor(0xc8, 0x00); //06
HI351_write_cmos_sensor(0xc9, 0x00); //07
HI351_write_cmos_sensor(0xca, 0x00); //aInCorWhtPtWgt_a00
HI351_write_cmos_sensor(0xcb, 0x00); //01
HI351_write_cmos_sensor(0xcc, 0x00); //02
HI351_write_cmos_sensor(0xcd, 0x00); //03
HI351_write_cmos_sensor(0xce, 0x20); //04
HI351_write_cmos_sensor(0xcf, 0x80); //05
HI351_write_cmos_sensor(0xd0, 0x20); //06
HI351_write_cmos_sensor(0xd1, 0x00); //07
HI351_write_cmos_sensor(0xd2, 0x0a); //aInYlvlWgt
HI351_write_cmos_sensor(0xd3, 0x19);
HI351_write_cmos_sensor(0xd4, 0x2d);
HI351_write_cmos_sensor(0xd5, 0x3c);
HI351_write_cmos_sensor(0xd6, 0x4b);
HI351_write_cmos_sensor(0xd7, 0x55);
HI351_write_cmos_sensor(0xd8, 0x64);
HI351_write_cmos_sensor(0xd9, 0x64);
HI351_write_cmos_sensor(0xda, 0x5a);
HI351_write_cmos_sensor(0xdb, 0x4b);
HI351_write_cmos_sensor(0xdc, 0x3c);
HI351_write_cmos_sensor(0xdd, 0x28);
HI351_write_cmos_sensor(0xde, 0x1e);
HI351_write_cmos_sensor(0xdf, 0x14);
HI351_write_cmos_sensor(0xe0, 0x0a);
HI351_write_cmos_sensor(0xe1, 0x0a);
HI351_write_cmos_sensor(0xe2, 0xff); //aInHiTmpWgtHiLmt_a00_n00
HI351_write_cmos_sensor(0xe3, 0xff); //01
HI351_write_cmos_sensor(0xe4, 0xff); //02
HI351_write_cmos_sensor(0xe5, 0xff); //03
HI351_write_cmos_sensor(0xe6, 0xff); //04
HI351_write_cmos_sensor(0xe7, 0xff); //05
HI351_write_cmos_sensor(0xe8, 0x80); //06
HI351_write_cmos_sensor(0xe9, 0x30); //07
HI351_write_cmos_sensor(0xea, 0x1e); //08
HI351_write_cmos_sensor(0xeb, 0x1e); //09
HI351_write_cmos_sensor(0xec, 0x1e); //10
HI351_write_cmos_sensor(0xed, 0x80); //aInHiTmpWgtLoLmt_a00_n00
HI351_write_cmos_sensor(0xee, 0x80); //01
HI351_write_cmos_sensor(0xef, 0x80); //02
HI351_write_cmos_sensor(0xf0, 0x80); //03
HI351_write_cmos_sensor(0xf1, 0x80); //04
HI351_write_cmos_sensor(0xf2, 0x40); //05
HI351_write_cmos_sensor(0xf3, 0x20); //06
HI351_write_cmos_sensor(0xf4, 0x0a); //07
HI351_write_cmos_sensor(0xf5, 0x0a); //08
HI351_write_cmos_sensor(0xf6, 0x0a); //09
HI351_write_cmos_sensor(0xf7, 0x0a); //10
HI351_write_cmos_sensor(0xf8, 0x32); //aInHiTmpWgtRatio_50%
HI351_write_cmos_sensor(0xf9, 0x02); //bInDyAglDiffMin
HI351_write_cmos_sensor(0xfa, 0x32); //bInDyAglDiffMax
HI351_write_cmos_sensor(0xfb, 0x28); //bInDyMinMaxTempWgt
HI351_write_cmos_sensor(0xfc, 0x64); //bInSplTmpAgl
HI351_write_cmos_sensor(0xfd, 0x1E); //bInSplTmpAglOfs
HI351_write_cmos_sensor(0x0e, 0x00); //burst end
HI351_write_cmos_sensor(0x03, 0xc6);
HI351_write_cmos_sensor(0x0e, 0x01); //burst start
HI351_write_cmos_sensor(0x10, 0x28); //bInSplTmpBpCntTh
HI351_write_cmos_sensor(0x11, 0x50); //bInSplTmpPtCorWgt
HI351_write_cmos_sensor(0x12, 0x1e); //bInSplTmpPtWgtRatio
HI351_write_cmos_sensor(0x13, 0x32); //bInSplTmpAglMinLmt
HI351_write_cmos_sensor(0x14, 0x4B); //bInSplTmpAglMaxLmt
HI351_write_cmos_sensor(0x15, 0x1e); //bInSplTmpDistTh
HI351_write_cmos_sensor(0x16, 0x08); //bInYlvlMin
HI351_write_cmos_sensor(0x17, 0xe0); //bInYlvlMax
HI351_write_cmos_sensor(0x18, 0x46); //bInRgainMin
HI351_write_cmos_sensor(0x19, 0x90); //bInRgainMax
HI351_write_cmos_sensor(0x1a, 0x40); //bInBgainMin
HI351_write_cmos_sensor(0x1b, 0xa4); //bInBgainMax
HI351_write_cmos_sensor(0x1c, 0x0a); //bInCntLmtHiTh
HI351_write_cmos_sensor(0x1d, 0x04); //bInCntLmtLoTh
HI351_write_cmos_sensor(0x1e, 0x16); //aOutWhtRgnBg
HI351_write_cmos_sensor(0x1f, 0x2B);
HI351_write_cmos_sensor(0x20, 0x30);
HI351_write_cmos_sensor(0x21, 0x36);
HI351_write_cmos_sensor(0x22, 0x3D);
HI351_write_cmos_sensor(0x23, 0x45);
HI351_write_cmos_sensor(0x24, 0x4C);
HI351_write_cmos_sensor(0x25, 0x53);
HI351_write_cmos_sensor(0x26, 0x5B);
HI351_write_cmos_sensor(0x27, 0x63);
HI351_write_cmos_sensor(0x28, 0xA2);
HI351_write_cmos_sensor(0x29, 0x9F); //aOutWhtRgnRgLeftLmt
HI351_write_cmos_sensor(0x2a, 0x58);
HI351_write_cmos_sensor(0x2b, 0x54);
HI351_write_cmos_sensor(0x2c, 0x4F);
HI351_write_cmos_sensor(0x2d, 0x49);
HI351_write_cmos_sensor(0x2e, 0x46);
HI351_write_cmos_sensor(0x2f, 0x44);
HI351_write_cmos_sensor(0x30, 0x42);
HI351_write_cmos_sensor(0x31, 0x41);
HI351_write_cmos_sensor(0x32, 0x40);
HI351_write_cmos_sensor(0x33, 0x3C);
HI351_write_cmos_sensor(0x34, 0xAA); //aOutWhtRgnRgRightLmt
HI351_write_cmos_sensor(0x35, 0x68);
HI351_write_cmos_sensor(0x36, 0x66);
HI351_write_cmos_sensor(0x37, 0x64);
HI351_write_cmos_sensor(0x38, 0x62);
HI351_write_cmos_sensor(0x39, 0x5F);
HI351_write_cmos_sensor(0x3a, 0x5C);
HI351_write_cmos_sensor(0x3b, 0x59);
HI351_write_cmos_sensor(0x3c, 0x53);
HI351_write_cmos_sensor(0x3d, 0x50);
HI351_write_cmos_sensor(0x3e, 0x4A);
HI351_write_cmos_sensor(0x3f, 0x16); //aOutWhtLineBg
HI351_write_cmos_sensor(0x40, 0x2C);
HI351_write_cmos_sensor(0x41, 0x32);
HI351_write_cmos_sensor(0x42, 0x39);
HI351_write_cmos_sensor(0x43, 0x40);
HI351_write_cmos_sensor(0x44, 0x48);
HI351_write_cmos_sensor(0x45, 0x4F);
HI351_write_cmos_sensor(0x46, 0x56);
HI351_write_cmos_sensor(0x47, 0x5D);
HI351_write_cmos_sensor(0x48, 0x63);
HI351_write_cmos_sensor(0x49, 0xA2);
HI351_write_cmos_sensor(0x4a, 0xA5); //aOutWhtLineRg
HI351_write_cmos_sensor(0x4b, 0x5F);
HI351_write_cmos_sensor(0x4c, 0x5C);
HI351_write_cmos_sensor(0x4d, 0x57);
HI351_write_cmos_sensor(0x4e, 0x54);
HI351_write_cmos_sensor(0x4f, 0x50);
HI351_write_cmos_sensor(0x50, 0x4E);
HI351_write_cmos_sensor(0x51, 0x4B);
HI351_write_cmos_sensor(0x52, 0x48);
HI351_write_cmos_sensor(0x53, 0x47);
HI351_write_cmos_sensor(0x54, 0x43);
HI351_write_cmos_sensor(0x55, 0x50); //aOutTgtAngle_a00
HI351_write_cmos_sensor(0x56, 0x55); //01
HI351_write_cmos_sensor(0x57, 0x5a); //02
HI351_write_cmos_sensor(0x58, 0x5f); //03
HI351_write_cmos_sensor(0x59, 0x64); //04
HI351_write_cmos_sensor(0x5a, 0x69); //05
HI351_write_cmos_sensor(0x5b, 0x6e); //06
HI351_write_cmos_sensor(0x5c, 0x73); //07
HI351_write_cmos_sensor(0x5d, 0x00); //aOutRgTgtOfs_a00
HI351_write_cmos_sensor(0x5e, 0x87); //01
HI351_write_cmos_sensor(0x5f, 0x8b); //02
HI351_write_cmos_sensor(0x60, 0x87); //03
HI351_write_cmos_sensor(0x61, 0x00); //04
HI351_write_cmos_sensor(0x62, 0x00); //05
HI351_write_cmos_sensor(0x63, 0x00); //06
HI351_write_cmos_sensor(0x64, 0x00); //07
HI351_write_cmos_sensor(0x65, 0x00); //aOutBgTgtOfs_a00_n00
HI351_write_cmos_sensor(0x66, 0x00); //01
HI351_write_cmos_sensor(0x67, 0x00); //02
HI351_write_cmos_sensor(0x68, 0x00); //03
HI351_write_cmos_sensor(0x69, 0x00); //04
HI351_write_cmos_sensor(0x6a, 0x00); //05
HI351_write_cmos_sensor(0x6b, 0x00); //06
HI351_write_cmos_sensor(0x6c, 0x00); //07
HI351_write_cmos_sensor(0x6d, 0x00); //aOutLeftOfs_a00_n00
HI351_write_cmos_sensor(0x6e, 0x00); //01
HI351_write_cmos_sensor(0x6f, 0x00); //02
HI351_write_cmos_sensor(0x70, 0x00); //03
HI351_write_cmos_sensor(0x71, 0x00); //04
HI351_write_cmos_sensor(0x72, 0x00); //05
HI351_write_cmos_sensor(0x73, 0x00); //06
HI351_write_cmos_sensor(0x74, 0x00); //07
HI351_write_cmos_sensor(0x75, 0x00); //aOutRightOfs_a00_n00
HI351_write_cmos_sensor(0x76, 0x00); //01
HI351_write_cmos_sensor(0x77, 0x00); //02
HI351_write_cmos_sensor(0x78, 0x00); //03
HI351_write_cmos_sensor(0x79, 0x00); //04
HI351_write_cmos_sensor(0x7a, 0x00); //05
HI351_write_cmos_sensor(0x7b, 0x00); //06
HI351_write_cmos_sensor(0x7c, 0x00); //07
HI351_write_cmos_sensor(0x7d, 0x00); //aOutCorWhtPtWgt_a00
HI351_write_cmos_sensor(0x7e, 0x00); //01
HI351_write_cmos_sensor(0x7f, 0x00); //02
HI351_write_cmos_sensor(0x80, 0x11); //03
HI351_write_cmos_sensor(0x81, 0x22); //04
HI351_write_cmos_sensor(0x82, 0x22); //05
HI351_write_cmos_sensor(0x83, 0x33); //06
HI351_write_cmos_sensor(0x84, 0x33); //07
HI351_write_cmos_sensor(0x85, 0x12); //aOutYlvlWgt
HI351_write_cmos_sensor(0x86, 0x1e);
HI351_write_cmos_sensor(0x87, 0x28);
HI351_write_cmos_sensor(0x88, 0x2d);
HI351_write_cmos_sensor(0x89, 0x2d);
HI351_write_cmos_sensor(0x8a, 0x12);
HI351_write_cmos_sensor(0x8b, 0x0a);
HI351_write_cmos_sensor(0x8c, 0x15);
HI351_write_cmos_sensor(0x8d, 0x1f);
HI351_write_cmos_sensor(0x8e, 0x16);
HI351_write_cmos_sensor(0x8f, 0x15);
HI351_write_cmos_sensor(0x90, 0x0a);
HI351_write_cmos_sensor(0x91, 0x0a);
HI351_write_cmos_sensor(0x92, 0x0a);
HI351_write_cmos_sensor(0x93, 0x0a);
HI351_write_cmos_sensor(0x94, 0x0a);
HI351_write_cmos_sensor(0x95, 0xff); //aOutHiTmpWgtHiLmt_a00_n00
HI351_write_cmos_sensor(0x96, 0xff); //01
HI351_write_cmos_sensor(0x97, 0xff); //02
HI351_write_cmos_sensor(0x98, 0xff); //03
HI351_write_cmos_sensor(0x99, 0xff); //04
HI351_write_cmos_sensor(0x9a, 0xff); //05
HI351_write_cmos_sensor(0x9b, 0xff); //06
HI351_write_cmos_sensor(0x9c, 0xff); //07
HI351_write_cmos_sensor(0x9d, 0xff); //08
HI351_write_cmos_sensor(0x9e, 0xff); //09
HI351_write_cmos_sensor(0x9f, 0xff); //10
HI351_write_cmos_sensor(0xa0, 0x08); //aOutHiTmpWgtLoLmt_a00_n00
HI351_write_cmos_sensor(0xa1, 0x08); //01
HI351_write_cmos_sensor(0xa2, 0x08); //02
HI351_write_cmos_sensor(0xa3, 0x0d); //03
HI351_write_cmos_sensor(0xa4, 0x10); //04
HI351_write_cmos_sensor(0xa5, 0x12); //05
HI351_write_cmos_sensor(0xa6, 0x12); //06
HI351_write_cmos_sensor(0xa7, 0x12); //07
HI351_write_cmos_sensor(0xa8, 0x13); //08
HI351_write_cmos_sensor(0xa9, 0x13); //09
HI351_write_cmos_sensor(0xaa, 0x14); //10
HI351_write_cmos_sensor(0xab, 0x0a); //aOutHiTmpWgtRatio
HI351_write_cmos_sensor(0xac, 0x01); //bOutDyAglDiffMin
HI351_write_cmos_sensor(0xad, 0x14); //bOutDyAglDiffMax
HI351_write_cmos_sensor(0xae, 0x19); //bOutDyMinMaxTempWgt
HI351_write_cmos_sensor(0xaf, 0x55); //bOutSplTmpAgl
HI351_write_cmos_sensor(0xb0, 0x1E); //bOutSplTmpAglOfs
HI351_write_cmos_sensor(0xb1, 0x28); //bOutSplTmpBpCntTh
HI351_write_cmos_sensor(0xb2, 0x1E); //bOutSplTmpPtCorWgt
HI351_write_cmos_sensor(0xb3, 0x50); //bOutSplTmpPtWgtRatio
HI351_write_cmos_sensor(0xb4, 0x1e); //bOutSplTmpAglMinLmt
HI351_write_cmos_sensor(0xb5, 0x3c); //bOutSplTmpAglMaxLmt
HI351_write_cmos_sensor(0xb6, 0x1e); //bOutSplTmpDistTh
HI351_write_cmos_sensor(0xb7, 0x08); //bOutYlvlMin
HI351_write_cmos_sensor(0xb8, 0xd2); //bOutYlvlMax
HI351_write_cmos_sensor(0xb9, 0x60); //bOutRgainMin
HI351_write_cmos_sensor(0xba, 0x66); //bOutRgainMax
HI351_write_cmos_sensor(0xbb, 0x66); //bOutBgainMin
HI351_write_cmos_sensor(0xbc, 0x80); //bOutBgainMax
HI351_write_cmos_sensor(0xbd, 0x0a); //bOutCntLmtHiTh
HI351_write_cmos_sensor(0xbe, 0x04); //bOutCntLmtLoTh
HI351_write_cmos_sensor(0x0e, 0x00); //burst end
HI351_write_cmos_sensor(0x03, 0xd4);
HI351_write_cmos_sensor(0x31, 0x5e); //out2 r_min
HI351_write_cmos_sensor(0x32, 0x6a); //out2 r_max
HI351_write_cmos_sensor(0x35, 0x6d); //out2 b_min
HI351_write_cmos_sensor(0x36, 0x80); //out2 b_max
HI351_write_cmos_sensor(0x37, 0x5e); //out1 r_min
HI351_write_cmos_sensor(0x38, 0x72); //out1 r_max
HI351_write_cmos_sensor(0x3b, 0x6c); //out1 b_min
HI351_write_cmos_sensor(0x3c, 0x80); //out1 b_max  74->80
HI351_write_cmos_sensor(0x3d, 0x40); //in r_min
HI351_write_cmos_sensor(0x3e, 0x90); //in r_max
HI351_write_cmos_sensor(0x41, 0x40); //in b_min
HI351_write_cmos_sensor(0x42, 0xb6); //in b_max  8a->90
HI351_write_cmos_sensor(0x03, 0xc9);
HI351_write_cmos_sensor(0x42, 0x68); //Rgain (16a2)
HI351_write_cmos_sensor(0x43, 0x40); //Ggain
HI351_write_cmos_sensor(0x44, 0x70); //Bgain (16a4)
/////////////////////////////////////////////
// CD Page (Color ratio)
/////////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0xCD); //
HI351_write_cmos_sensor(0x47, 0x06); //
HI351_write_cmos_sensor(0x10, 0xB8); //enable
/////////////////////////////////////////////
// Cf Page Adaptive Mode
/////////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0xcf); //page cf
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x00); //control
HI351_write_cmos_sensor(0x11, 0x05);
HI351_write_cmos_sensor(0x12, 0x01);
HI351_write_cmos_sensor(0x13, 0x02); //Y_LUM_MAX
HI351_write_cmos_sensor(0x14, 0x82);
HI351_write_cmos_sensor(0x15, 0xb3);
HI351_write_cmos_sensor(0x16, 0x40);
HI351_write_cmos_sensor(0x17, 0x00); //Y_LUM mid1
HI351_write_cmos_sensor(0x18, 0xcf);
HI351_write_cmos_sensor(0x19, 0x06);
HI351_write_cmos_sensor(0x1a, 0x0c);
HI351_write_cmos_sensor(0x1b, 0x00); //Y_LUM mid2
HI351_write_cmos_sensor(0x1c, 0x09);
HI351_write_cmos_sensor(0x1d, 0x45);
HI351_write_cmos_sensor(0x1e, 0x0c);
HI351_write_cmos_sensor(0x1f, 0x00); //Y_LUM min
HI351_write_cmos_sensor(0x20, 0x00);
HI351_write_cmos_sensor(0x21, 0x20);
HI351_write_cmos_sensor(0x22, 0xF6);
HI351_write_cmos_sensor(0x23, 0x78); //CTEM high2
HI351_write_cmos_sensor(0x24, 0x58); //CTEM middle
HI351_write_cmos_sensor(0x25, 0x38); //CTEM low
HI351_write_cmos_sensor(0x26, 0x60); //YCON high
HI351_write_cmos_sensor(0x27, 0x40); //YCON middle
HI351_write_cmos_sensor(0x28, 0x01); //YCON low
HI351_write_cmos_sensor(0x29, 0x00); //Y_LUM max_TH
HI351_write_cmos_sensor(0x2a, 0x00);
HI351_write_cmos_sensor(0x2b, 0x00);
HI351_write_cmos_sensor(0x2c, 0x00);
HI351_write_cmos_sensor(0x2d, 0x00); //Y_LUM mid1_TH
HI351_write_cmos_sensor(0x2e, 0xbc);
HI351_write_cmos_sensor(0x2f, 0x7b);
HI351_write_cmos_sensor(0x30, 0xf4);
HI351_write_cmos_sensor(0x31, 0x00); //Y_LUM mid2_TH
HI351_write_cmos_sensor(0x32, 0x00);
HI351_write_cmos_sensor(0x33, 0x00);
HI351_write_cmos_sensor(0x34, 0x00);
HI351_write_cmos_sensor(0x35, 0x00); //Y_LUM min_TH
HI351_write_cmos_sensor(0x36, 0x00);
HI351_write_cmos_sensor(0x37, 0x00);
HI351_write_cmos_sensor(0x38, 0x00);
HI351_write_cmos_sensor(0x39, 0x00); //CTEM high_TH
HI351_write_cmos_sensor(0x3a, 0x14); //CTEM middle_TH
HI351_write_cmos_sensor(0x3b, 0x00); //CTEM low_TH
HI351_write_cmos_sensor(0x3c, 0x00); //YCON high_TH
HI351_write_cmos_sensor(0x3d, 0x00); //YCON middle_TH
HI351_write_cmos_sensor(0x3e, 0x00); //YCON low_TH
//////////////////////////////////////////////
// CF Page Adaptive Y Target
/////////////////////////////////////////////
HI351_write_cmos_sensor(0x3f, 0x30); //YLVL_00
HI351_write_cmos_sensor(0x40, 0x30); //YLVL_01
HI351_write_cmos_sensor(0x41, 0x30); //YLVL_02
HI351_write_cmos_sensor(0x42, 0x33); //YLVL_03
HI351_write_cmos_sensor(0x43, 0x33); //YLVL_04
HI351_write_cmos_sensor(0x44, 0x33); //YLVL_05
HI351_write_cmos_sensor(0x45, 0x33); //YLVL_06
HI351_write_cmos_sensor(0x46, 0x33); //YLVL_07
HI351_write_cmos_sensor(0x47, 0x33); //YLVL_08
HI351_write_cmos_sensor(0x48, 0x33); //YLVL_09
HI351_write_cmos_sensor(0x49, 0x33); //YLVL_10
HI351_write_cmos_sensor(0x4a, 0x33); //YLVL_11
/////////////////////////////////////////////
// CF Page Adaptive Y Contrast (4b~56)
/////////////////////////////////////////////
HI351_write_cmos_sensor(0x4b, 0x80); //YCON_00
HI351_write_cmos_sensor(0x4c, 0x80); //YCON_01
HI351_write_cmos_sensor(0x4d, 0x80); //YCON_02
HI351_write_cmos_sensor(0x4e, 0x80); //YCON_03
HI351_write_cmos_sensor(0x4f, 0x80); //YCON_04
HI351_write_cmos_sensor(0x50, 0x80); //YCON_05
HI351_write_cmos_sensor(0x51, 0x80); //YCON_06
HI351_write_cmos_sensor(0x52, 0x80); //YCON_07
HI351_write_cmos_sensor(0x53, 0x80); //YCON_08
HI351_write_cmos_sensor(0x54, 0x80); //YCON_09
HI351_write_cmos_sensor(0x55, 0x80); //YCON_10
HI351_write_cmos_sensor(0x56, 0x80); //YCON_11
/////////////////////////////////////////////
// CF Page Adaptive Y OFFSET (57~62)
/////////////////////////////////////////////
HI351_write_cmos_sensor(0x57, 0x00); //y offset 0
HI351_write_cmos_sensor(0x58, 0x00); //y offset 1
HI351_write_cmos_sensor(0x59, 0x00); //y offset 2
HI351_write_cmos_sensor(0x5a, 0x00); //y offset 3
HI351_write_cmos_sensor(0x5b, 0x00); //y offset 4
HI351_write_cmos_sensor(0x5c, 0x00); //y offset 5
HI351_write_cmos_sensor(0x5d, 0x00); //y offset 6
HI351_write_cmos_sensor(0x5e, 0x00); //y offset 7
HI351_write_cmos_sensor(0x5f, 0x00); //y offset 8
HI351_write_cmos_sensor(0x60, 0x00); //y offset 9
HI351_write_cmos_sensor(0x61, 0x00); //y offset 10
HI351_write_cmos_sensor(0x62, 0x00); //y offset 11
/////////////////////////////////////////////
// CF~D0~D1 Page Adaptive GAMMA
/////////////////////////////////////////////
HI351_write_cmos_sensor(0x63, 0x00); //GMA00
HI351_write_cmos_sensor(0x64, 0x02);
HI351_write_cmos_sensor(0x65, 0x04);
HI351_write_cmos_sensor(0x66, 0x0a);
HI351_write_cmos_sensor(0x67, 0x11);
HI351_write_cmos_sensor(0x68, 0x23);
HI351_write_cmos_sensor(0x69, 0x36);
HI351_write_cmos_sensor(0x6a, 0x46);
HI351_write_cmos_sensor(0x6b, 0x57);
HI351_write_cmos_sensor(0x6c, 0x64);
HI351_write_cmos_sensor(0x6d, 0x6f);
HI351_write_cmos_sensor(0x6e, 0x78);
HI351_write_cmos_sensor(0x6f, 0x80);
HI351_write_cmos_sensor(0x70, 0x87);
HI351_write_cmos_sensor(0x71, 0x8c);
HI351_write_cmos_sensor(0x72, 0x91);
HI351_write_cmos_sensor(0x73, 0x96);
HI351_write_cmos_sensor(0x74, 0x9a);
HI351_write_cmos_sensor(0x75, 0x9f);
HI351_write_cmos_sensor(0x76, 0xa3);
HI351_write_cmos_sensor(0x77, 0xa8);
HI351_write_cmos_sensor(0x78, 0xaf);
HI351_write_cmos_sensor(0x79, 0xb6);
HI351_write_cmos_sensor(0x7a, 0xbc);
HI351_write_cmos_sensor(0x7b, 0xc6);
HI351_write_cmos_sensor(0x7c, 0xd1);
HI351_write_cmos_sensor(0x7d, 0xdb);
HI351_write_cmos_sensor(0x7e, 0xe2);
HI351_write_cmos_sensor(0x7f, 0xe9);
HI351_write_cmos_sensor(0x80, 0xf0);
HI351_write_cmos_sensor(0x81, 0xf6);
HI351_write_cmos_sensor(0x82, 0xfa);
HI351_write_cmos_sensor(0x83, 0xfe);
HI351_write_cmos_sensor(0x84, 0xff);
HI351_write_cmos_sensor(0x85, 0x00); //GMA01
HI351_write_cmos_sensor(0x86, 0x02);
HI351_write_cmos_sensor(0x87, 0x04);
HI351_write_cmos_sensor(0x88, 0x0a);
HI351_write_cmos_sensor(0x89, 0x11);
HI351_write_cmos_sensor(0x8a, 0x23);
HI351_write_cmos_sensor(0x8b, 0x36);
HI351_write_cmos_sensor(0x8c, 0x46);
HI351_write_cmos_sensor(0x8d, 0x57);
HI351_write_cmos_sensor(0x8e, 0x64);
HI351_write_cmos_sensor(0x8f, 0x6f);
HI351_write_cmos_sensor(0x90, 0x78);
HI351_write_cmos_sensor(0x91, 0x80);
HI351_write_cmos_sensor(0x92, 0x87);
HI351_write_cmos_sensor(0x93, 0x8c);
HI351_write_cmos_sensor(0x94, 0x91);
HI351_write_cmos_sensor(0x95, 0x96);
HI351_write_cmos_sensor(0x96, 0x9a);
HI351_write_cmos_sensor(0x97, 0x9f);
HI351_write_cmos_sensor(0x98, 0xa3);
HI351_write_cmos_sensor(0x99, 0xa8);
HI351_write_cmos_sensor(0x9a, 0xaf);
HI351_write_cmos_sensor(0x9b, 0xb6);
HI351_write_cmos_sensor(0x9c, 0xbc);
HI351_write_cmos_sensor(0x9d, 0xc6);
HI351_write_cmos_sensor(0x9e, 0xd1);
HI351_write_cmos_sensor(0x9f, 0xdb);
HI351_write_cmos_sensor(0xa0, 0xe2);
HI351_write_cmos_sensor(0xa1, 0xe9);
HI351_write_cmos_sensor(0xa2, 0xf0);
HI351_write_cmos_sensor(0xa3, 0xf6);
HI351_write_cmos_sensor(0xa4, 0xfa);
HI351_write_cmos_sensor(0xa5, 0xfe);
HI351_write_cmos_sensor(0xa6, 0xff);
HI351_write_cmos_sensor(0xa7, 0x00); //GMA02
HI351_write_cmos_sensor(0xa8, 0x02);
HI351_write_cmos_sensor(0xa9, 0x04);
HI351_write_cmos_sensor(0xaa, 0x0a);
HI351_write_cmos_sensor(0xab, 0x11);
HI351_write_cmos_sensor(0xac, 0x23);
HI351_write_cmos_sensor(0xad, 0x36);
HI351_write_cmos_sensor(0xae, 0x46);
HI351_write_cmos_sensor(0xaf, 0x57);
HI351_write_cmos_sensor(0xb0, 0x64);
HI351_write_cmos_sensor(0xb1, 0x6f);
HI351_write_cmos_sensor(0xb2, 0x78);
HI351_write_cmos_sensor(0xb3, 0x80);
HI351_write_cmos_sensor(0xb4, 0x87);
HI351_write_cmos_sensor(0xb5, 0x8c);
HI351_write_cmos_sensor(0xb6, 0x91);
HI351_write_cmos_sensor(0xb7, 0x96);
HI351_write_cmos_sensor(0xb8, 0x9a);
HI351_write_cmos_sensor(0xb9, 0x9f);
HI351_write_cmos_sensor(0xba, 0xa3);
HI351_write_cmos_sensor(0xbb, 0xa8);
HI351_write_cmos_sensor(0xbc, 0xaf);
HI351_write_cmos_sensor(0xbd, 0xb6);
HI351_write_cmos_sensor(0xbe, 0xbc);
HI351_write_cmos_sensor(0xbf, 0xc6);
HI351_write_cmos_sensor(0xc0, 0xd1);
HI351_write_cmos_sensor(0xc1, 0xdb);
HI351_write_cmos_sensor(0xc2, 0xe2);
HI351_write_cmos_sensor(0xc3, 0xe9);
HI351_write_cmos_sensor(0xc4, 0xf0);
HI351_write_cmos_sensor(0xc5, 0xf6);
HI351_write_cmos_sensor(0xc6, 0xfa);
HI351_write_cmos_sensor(0xc7, 0xfe);
HI351_write_cmos_sensor(0xc8, 0xff);

HI351_write_cmos_sensor(0xc9, 0x00);	//GMA03
HI351_write_cmos_sensor(0xca, 0x06);
HI351_write_cmos_sensor(0xcb, 0x0b);
HI351_write_cmos_sensor(0xcc, 0x15);
HI351_write_cmos_sensor(0xcd, 0x1f);
HI351_write_cmos_sensor(0xce, 0x2e);
HI351_write_cmos_sensor(0xcf, 0x3c);
HI351_write_cmos_sensor(0xd0, 0x48);
HI351_write_cmos_sensor(0xd1, 0x52);
HI351_write_cmos_sensor(0xd2, 0x5b);
HI351_write_cmos_sensor(0xd3, 0x63);
HI351_write_cmos_sensor(0xd4, 0x6a);
HI351_write_cmos_sensor(0xd5, 0x72);
HI351_write_cmos_sensor(0xd6, 0x78);
HI351_write_cmos_sensor(0xd7, 0x7e);
HI351_write_cmos_sensor(0xd8, 0x86);
HI351_write_cmos_sensor(0xd9, 0x8b);
HI351_write_cmos_sensor(0xda, 0x90);
HI351_write_cmos_sensor(0xdb, 0x95);
HI351_write_cmos_sensor(0xdc, 0x9a);
HI351_write_cmos_sensor(0xdd, 0x9f);
HI351_write_cmos_sensor(0xde, 0xa9);
HI351_write_cmos_sensor(0xdf, 0xb2);
HI351_write_cmos_sensor(0xe0, 0xb9);
HI351_write_cmos_sensor(0xe1, 0xcb);
HI351_write_cmos_sensor(0xe2, 0xd7);
HI351_write_cmos_sensor(0xe3, 0xe2);
HI351_write_cmos_sensor(0xe4, 0xed);
HI351_write_cmos_sensor(0xe5, 0xf4);
HI351_write_cmos_sensor(0xe6, 0xf8);
HI351_write_cmos_sensor(0xe7, 0xfa);
HI351_write_cmos_sensor(0xe8, 0xfc);
HI351_write_cmos_sensor(0xe9, 0xfe);
HI351_write_cmos_sensor(0xea, 0xff);

HI351_write_cmos_sensor(0xeb, 0x00);	//GMA04
HI351_write_cmos_sensor(0xec, 0x06);
HI351_write_cmos_sensor(0xed, 0x0b);
HI351_write_cmos_sensor(0xee, 0x15);
HI351_write_cmos_sensor(0xef, 0x1f);
HI351_write_cmos_sensor(0xf0, 0x2e);
HI351_write_cmos_sensor(0xf1, 0x3c);
HI351_write_cmos_sensor(0xf2, 0x48);
HI351_write_cmos_sensor(0xf3, 0x52);
HI351_write_cmos_sensor(0xf4, 0x5b);
HI351_write_cmos_sensor(0xf5, 0x63);
HI351_write_cmos_sensor(0xf6, 0x6a);
HI351_write_cmos_sensor(0xf7, 0x72);
HI351_write_cmos_sensor(0xf8, 0x78);
HI351_write_cmos_sensor(0xf9, 0x7e);
HI351_write_cmos_sensor(0xfa, 0x86);
HI351_write_cmos_sensor(0xfb, 0x8b);
HI351_write_cmos_sensor(0xfc, 0x90);
HI351_write_cmos_sensor(0xfd, 0x95);
HI351_write_cmos_sensor(0x0e, 0x00); //burst end  
HI351_write_cmos_sensor(0x03, 0xd0); //Page d0    
HI351_write_cmos_sensor(0x0e, 0x01); //burst start
HI351_write_cmos_sensor(0x10, 0x9a);
HI351_write_cmos_sensor(0x11, 0x9f);
HI351_write_cmos_sensor(0x12, 0xa9);
HI351_write_cmos_sensor(0x13, 0xb2);
HI351_write_cmos_sensor(0x14, 0xb9);
HI351_write_cmos_sensor(0x15, 0xcb);
HI351_write_cmos_sensor(0x16, 0xd7);
HI351_write_cmos_sensor(0x17, 0xe2);
HI351_write_cmos_sensor(0x18, 0xed);
HI351_write_cmos_sensor(0x19, 0xf4);
HI351_write_cmos_sensor(0x1a, 0xf8);
HI351_write_cmos_sensor(0x1b, 0xfa);
HI351_write_cmos_sensor(0x1c, 0xfc);
HI351_write_cmos_sensor(0x1d, 0xfe);
HI351_write_cmos_sensor(0x1e, 0xff);

HI351_write_cmos_sensor(0x1f, 0x00);	//GMA05
HI351_write_cmos_sensor(0x20, 0x06);
HI351_write_cmos_sensor(0x21, 0x0b);
HI351_write_cmos_sensor(0x22, 0x15);
HI351_write_cmos_sensor(0x23, 0x1f);
HI351_write_cmos_sensor(0x24, 0x2e);
HI351_write_cmos_sensor(0x25, 0x3c);
HI351_write_cmos_sensor(0x26, 0x48);
HI351_write_cmos_sensor(0x27, 0x52);
HI351_write_cmos_sensor(0x28, 0x5b);
HI351_write_cmos_sensor(0x29, 0x63);
HI351_write_cmos_sensor(0x2a, 0x6a);
HI351_write_cmos_sensor(0x2b, 0x72);
HI351_write_cmos_sensor(0x2c, 0x78);
HI351_write_cmos_sensor(0x2d, 0x7e);
HI351_write_cmos_sensor(0x2e, 0x86);
HI351_write_cmos_sensor(0x2f, 0x8b);
HI351_write_cmos_sensor(0x30, 0x90);
HI351_write_cmos_sensor(0x31, 0x95);
HI351_write_cmos_sensor(0x32, 0x9a);
HI351_write_cmos_sensor(0x33, 0x9f);
HI351_write_cmos_sensor(0x34, 0xa9);
HI351_write_cmos_sensor(0x35, 0xb2);
HI351_write_cmos_sensor(0x36, 0xb9);
HI351_write_cmos_sensor(0x37, 0xcb);
HI351_write_cmos_sensor(0x38, 0xd7);
HI351_write_cmos_sensor(0x39, 0xe2);
HI351_write_cmos_sensor(0x3a, 0xed);
HI351_write_cmos_sensor(0x3b, 0xf4);
HI351_write_cmos_sensor(0x3c, 0xf8);
HI351_write_cmos_sensor(0x3d, 0xfa);
HI351_write_cmos_sensor(0x3e, 0xfc);
HI351_write_cmos_sensor(0x3f, 0xfe);
HI351_write_cmos_sensor(0x40, 0xff);

HI351_write_cmos_sensor(0x41, 0x00); //GMA06
HI351_write_cmos_sensor(0x42, 0x01);
HI351_write_cmos_sensor(0x43, 0x04);
HI351_write_cmos_sensor(0x44, 0x0a);
HI351_write_cmos_sensor(0x45, 0x14);
HI351_write_cmos_sensor(0x46, 0x23);
HI351_write_cmos_sensor(0x47, 0x30);
HI351_write_cmos_sensor(0x48, 0x40);
HI351_write_cmos_sensor(0x49, 0x4d);
HI351_write_cmos_sensor(0x4a, 0x57);
HI351_write_cmos_sensor(0x4b, 0x62);
HI351_write_cmos_sensor(0x4c, 0x6b);
HI351_write_cmos_sensor(0x4d, 0x73);
HI351_write_cmos_sensor(0x4e, 0x7a);
HI351_write_cmos_sensor(0x4f, 0x82);
HI351_write_cmos_sensor(0x50, 0x89);
HI351_write_cmos_sensor(0x51, 0x8e);
HI351_write_cmos_sensor(0x52, 0x94);
HI351_write_cmos_sensor(0x53, 0x99);
HI351_write_cmos_sensor(0x54, 0x9e);
HI351_write_cmos_sensor(0x55, 0xa2);
HI351_write_cmos_sensor(0x56, 0xaa);
HI351_write_cmos_sensor(0x57, 0xb1);
HI351_write_cmos_sensor(0x58, 0xb9);
HI351_write_cmos_sensor(0x59, 0xc4);
HI351_write_cmos_sensor(0x5a, 0xce);
HI351_write_cmos_sensor(0x5b, 0xd6);
HI351_write_cmos_sensor(0x5c, 0xdc);
HI351_write_cmos_sensor(0x5d, 0xe1);
HI351_write_cmos_sensor(0x5e, 0xe6);
HI351_write_cmos_sensor(0x5f, 0xec);
HI351_write_cmos_sensor(0x60, 0xf1);
HI351_write_cmos_sensor(0x61, 0xf5);
HI351_write_cmos_sensor(0x62, 0xf9);
HI351_write_cmos_sensor(0x63, 0x00); //GMA07
HI351_write_cmos_sensor(0x64, 0x01);
HI351_write_cmos_sensor(0x65, 0x04);
HI351_write_cmos_sensor(0x66, 0x0a);
HI351_write_cmos_sensor(0x67, 0x14);
HI351_write_cmos_sensor(0x68, 0x23);
HI351_write_cmos_sensor(0x69, 0x30);
HI351_write_cmos_sensor(0x6a, 0x40);
HI351_write_cmos_sensor(0x6b, 0x4d);
HI351_write_cmos_sensor(0x6c, 0x57);
HI351_write_cmos_sensor(0x6d, 0x62);
HI351_write_cmos_sensor(0x6e, 0x6b);
HI351_write_cmos_sensor(0x6f, 0x73);
HI351_write_cmos_sensor(0x70, 0x7a);
HI351_write_cmos_sensor(0x71, 0x82);
HI351_write_cmos_sensor(0x72, 0x89);
HI351_write_cmos_sensor(0x73, 0x8e);
HI351_write_cmos_sensor(0x74, 0x94);
HI351_write_cmos_sensor(0x75, 0x99);
HI351_write_cmos_sensor(0x76, 0x9e);
HI351_write_cmos_sensor(0x77, 0xa2);
HI351_write_cmos_sensor(0x78, 0xaa);
HI351_write_cmos_sensor(0x79, 0xb1);
HI351_write_cmos_sensor(0x7a, 0xb9);
HI351_write_cmos_sensor(0x7b, 0xc4);
HI351_write_cmos_sensor(0x7c, 0xce);
HI351_write_cmos_sensor(0x7d, 0xd6);
HI351_write_cmos_sensor(0x7e, 0xdc);
HI351_write_cmos_sensor(0x7f, 0xe1);
HI351_write_cmos_sensor(0x80, 0xe6);
HI351_write_cmos_sensor(0x81, 0xec);
HI351_write_cmos_sensor(0x82, 0xf1);
HI351_write_cmos_sensor(0x83, 0xf5);
HI351_write_cmos_sensor(0x84, 0xf9);
HI351_write_cmos_sensor(0x85, 0x00); //GMA08
HI351_write_cmos_sensor(0x86, 0x01);
HI351_write_cmos_sensor(0x87, 0x04);
HI351_write_cmos_sensor(0x88, 0x0a);
HI351_write_cmos_sensor(0x89, 0x14);
HI351_write_cmos_sensor(0x8a, 0x23);
HI351_write_cmos_sensor(0x8b, 0x30);
HI351_write_cmos_sensor(0x8c, 0x40);
HI351_write_cmos_sensor(0x8d, 0x4d);
HI351_write_cmos_sensor(0x8e, 0x57);
HI351_write_cmos_sensor(0x8f, 0x62);
HI351_write_cmos_sensor(0x90, 0x6b);
HI351_write_cmos_sensor(0x91, 0x73);
HI351_write_cmos_sensor(0x92, 0x7a);
HI351_write_cmos_sensor(0x93, 0x82);
HI351_write_cmos_sensor(0x94, 0x89);
HI351_write_cmos_sensor(0x95, 0x8e);
HI351_write_cmos_sensor(0x96, 0x94);
HI351_write_cmos_sensor(0x97, 0x99);
HI351_write_cmos_sensor(0x98, 0x9e);
HI351_write_cmos_sensor(0x99, 0xa2);
HI351_write_cmos_sensor(0x9a, 0xaa);
HI351_write_cmos_sensor(0x9b, 0xb1);
HI351_write_cmos_sensor(0x9c, 0xb9);
HI351_write_cmos_sensor(0x9d, 0xc4);
HI351_write_cmos_sensor(0x9e, 0xce);
HI351_write_cmos_sensor(0x9f, 0xd6);
HI351_write_cmos_sensor(0xa0, 0xdc);
HI351_write_cmos_sensor(0xa1, 0xe1);
HI351_write_cmos_sensor(0xa2, 0xe6);
HI351_write_cmos_sensor(0xa3, 0xec);
HI351_write_cmos_sensor(0xa4, 0xf1);
HI351_write_cmos_sensor(0xa5, 0xf5);
HI351_write_cmos_sensor(0xa6, 0xf9);
HI351_write_cmos_sensor(0xa7, 0x00); //GMA09
HI351_write_cmos_sensor(0xa8, 0x01);
HI351_write_cmos_sensor(0xa9, 0x04);
HI351_write_cmos_sensor(0xaa, 0x0a);
HI351_write_cmos_sensor(0xab, 0x14);
HI351_write_cmos_sensor(0xac, 0x23);
HI351_write_cmos_sensor(0xad, 0x30);
HI351_write_cmos_sensor(0xae, 0x40);
HI351_write_cmos_sensor(0xaf, 0x4d);
HI351_write_cmos_sensor(0xb0, 0x57);
HI351_write_cmos_sensor(0xb1, 0x62);
HI351_write_cmos_sensor(0xb2, 0x6b);
HI351_write_cmos_sensor(0xb3, 0x73);
HI351_write_cmos_sensor(0xb4, 0x7a);
HI351_write_cmos_sensor(0xb5, 0x82);
HI351_write_cmos_sensor(0xb6, 0x89);
HI351_write_cmos_sensor(0xb7, 0x8e);
HI351_write_cmos_sensor(0xb8, 0x94);
HI351_write_cmos_sensor(0xb9, 0x99);
HI351_write_cmos_sensor(0xba, 0x9e);
HI351_write_cmos_sensor(0xbb, 0xa2);
HI351_write_cmos_sensor(0xbc, 0xaa);
HI351_write_cmos_sensor(0xbd, 0xb1);
HI351_write_cmos_sensor(0xbe, 0xb9);
HI351_write_cmos_sensor(0xbf, 0xc4);
HI351_write_cmos_sensor(0xc0, 0xce);
HI351_write_cmos_sensor(0xc1, 0xd6);
HI351_write_cmos_sensor(0xc2, 0xdc);
HI351_write_cmos_sensor(0xc3, 0xe1);
HI351_write_cmos_sensor(0xc4, 0xe6);
HI351_write_cmos_sensor(0xc5, 0xec);
HI351_write_cmos_sensor(0xc6, 0xf1);
HI351_write_cmos_sensor(0xc7, 0xf5);
HI351_write_cmos_sensor(0xc8, 0xf9);
HI351_write_cmos_sensor(0xc9, 0x00); //GMA10
HI351_write_cmos_sensor(0xca, 0x01);
HI351_write_cmos_sensor(0xcb, 0x04);
HI351_write_cmos_sensor(0xcc, 0x0a);
HI351_write_cmos_sensor(0xcd, 0x14);
HI351_write_cmos_sensor(0xce, 0x23);
HI351_write_cmos_sensor(0xcf, 0x30);
HI351_write_cmos_sensor(0xd0, 0x40);
HI351_write_cmos_sensor(0xd1, 0x4d);
HI351_write_cmos_sensor(0xd2, 0x57);
HI351_write_cmos_sensor(0xd3, 0x62);
HI351_write_cmos_sensor(0xd4, 0x6b);
HI351_write_cmos_sensor(0xd5, 0x73);
HI351_write_cmos_sensor(0xd6, 0x7a);
HI351_write_cmos_sensor(0xd7, 0x82);
HI351_write_cmos_sensor(0xd8, 0x89);
HI351_write_cmos_sensor(0xd9, 0x8e);
HI351_write_cmos_sensor(0xda, 0x94);
HI351_write_cmos_sensor(0xdb, 0x99);
HI351_write_cmos_sensor(0xdc, 0x9e);
HI351_write_cmos_sensor(0xdd, 0xa2);
HI351_write_cmos_sensor(0xde, 0xaa);
HI351_write_cmos_sensor(0xdf, 0xb1);
HI351_write_cmos_sensor(0xe0, 0xb9);
HI351_write_cmos_sensor(0xe1, 0xc4);
HI351_write_cmos_sensor(0xe2, 0xce);
HI351_write_cmos_sensor(0xe3, 0xd6);
HI351_write_cmos_sensor(0xe4, 0xdc);
HI351_write_cmos_sensor(0xe5, 0xe1);
HI351_write_cmos_sensor(0xe6, 0xe6);
HI351_write_cmos_sensor(0xe7, 0xec);
HI351_write_cmos_sensor(0xe8, 0xf1);
HI351_write_cmos_sensor(0xe9, 0xf5);
HI351_write_cmos_sensor(0xea, 0xf9);
HI351_write_cmos_sensor(0xeb, 0x00); //GMA11
HI351_write_cmos_sensor(0xec, 0x01);
HI351_write_cmos_sensor(0xed, 0x04);
HI351_write_cmos_sensor(0xee, 0x0a);
HI351_write_cmos_sensor(0xef, 0x14);
HI351_write_cmos_sensor(0xf0, 0x23);
HI351_write_cmos_sensor(0xf1, 0x30);
HI351_write_cmos_sensor(0xf2, 0x40);
HI351_write_cmos_sensor(0xf3, 0x4d);
HI351_write_cmos_sensor(0xf4, 0x57);
HI351_write_cmos_sensor(0xf5, 0x62);
HI351_write_cmos_sensor(0xf6, 0x6b);
HI351_write_cmos_sensor(0xf7, 0x73);
HI351_write_cmos_sensor(0xf8, 0x7a);
HI351_write_cmos_sensor(0xf9, 0x82);
HI351_write_cmos_sensor(0xfa, 0x89);
HI351_write_cmos_sensor(0xfb, 0x8e);
HI351_write_cmos_sensor(0xfc, 0x94);
HI351_write_cmos_sensor(0xfd, 0x99);
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
HI351_write_cmos_sensor(0x03, 0xd1); //Page d1
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x9e);
HI351_write_cmos_sensor(0x11, 0xa2);
HI351_write_cmos_sensor(0x12, 0xaa);
HI351_write_cmos_sensor(0x13, 0xb1);
HI351_write_cmos_sensor(0x14, 0xb9);
HI351_write_cmos_sensor(0x15, 0xc4);
HI351_write_cmos_sensor(0x16, 0xce);
HI351_write_cmos_sensor(0x17, 0xd6);
HI351_write_cmos_sensor(0x18, 0xdc);
HI351_write_cmos_sensor(0x19, 0xe1);
HI351_write_cmos_sensor(0x1a, 0xe6);
HI351_write_cmos_sensor(0x1b, 0xec);
HI351_write_cmos_sensor(0x1c, 0xf1);
HI351_write_cmos_sensor(0x1d, 0xf5);
HI351_write_cmos_sensor(0x1e, 0xf9);

///////////////////////////////////////////
// D1 Page Adaptive Y Target delta
///////////////////////////////////////////
HI351_write_cmos_sensor(0x1f, 0x50); //Y target delta 0
HI351_write_cmos_sensor(0x20, 0x58); //Y target delta 1
HI351_write_cmos_sensor(0x21, 0x60); //Y target delta 2
HI351_write_cmos_sensor(0x22, 0x60); //Y target delta 3
HI351_write_cmos_sensor(0x23, 0x65); //Y target delta 4
HI351_write_cmos_sensor(0x24, 0x70); //Y target delta 5
HI351_write_cmos_sensor(0x25, 0x75); //Y target delta 6
HI351_write_cmos_sensor(0x26, 0x80); //Y target delta 7
HI351_write_cmos_sensor(0x27, 0x80); //Y target delta 8
HI351_write_cmos_sensor(0x28, 0x80); //Y target delta 9
HI351_write_cmos_sensor(0x29, 0x80); //Y target delta 10
HI351_write_cmos_sensor(0x2a, 0x80); //Y target delta 11
///////////////////////////////////////////
// D1 Page Adaptive R/B saturation
///////////////////////////////////////////
//SATB
HI351_write_cmos_sensor(0x2b, 0x60); //SATB_00
HI351_write_cmos_sensor(0x2c, 0x60); //SATB_01
HI351_write_cmos_sensor(0x2d, 0x60); //SATB_02
HI351_write_cmos_sensor(0x2e, 0x78); //SATB_03
HI351_write_cmos_sensor(0x2f, 0x80); //SATB_04
HI351_write_cmos_sensor(0x30, 0x80); //SATB_05
HI351_write_cmos_sensor(0x31, 0x94); //SATB_06
HI351_write_cmos_sensor(0x32, 0x94); //SATB_07
HI351_write_cmos_sensor(0x33, 0xa0); //SATB_08
HI351_write_cmos_sensor(0x34, 0xa0); //SATB_09
HI351_write_cmos_sensor(0x35, 0x90); //SATB_10
HI351_write_cmos_sensor(0x36, 0x86); //SATB_11
//SATR
HI351_write_cmos_sensor(0x37, 0x60); //SATR_00
HI351_write_cmos_sensor(0x38, 0x60); //SATR_01
HI351_write_cmos_sensor(0x39, 0x60); //SATR_02
HI351_write_cmos_sensor(0x3a, 0x78); //SATR_03
HI351_write_cmos_sensor(0x3b, 0x80); //SATR_04
HI351_write_cmos_sensor(0x3c, 0x90); //SATR_05
HI351_write_cmos_sensor(0x3d, 0xaa); //SATR_06
HI351_write_cmos_sensor(0x3e, 0xaa); //SATR_07
HI351_write_cmos_sensor(0x3f, 0xa0); //SATR_08
HI351_write_cmos_sensor(0x40, 0xa0); //SATR_09
HI351_write_cmos_sensor(0x41, 0x90); //SATR_10
HI351_write_cmos_sensor(0x42, 0x86); //SATR_11
/////////////////////////////////////////////
// D1 Page Adaptive CMC
/////////////////////////////////////////////

HI351_write_cmos_sensor(0x43, 0x2f); //CMC_00
HI351_write_cmos_sensor(0x44, 0x68);
HI351_write_cmos_sensor(0x45, 0x29);
HI351_write_cmos_sensor(0x46, 0x01);
HI351_write_cmos_sensor(0x47, 0x19);
HI351_write_cmos_sensor(0x48, 0x6c);
HI351_write_cmos_sensor(0x49, 0x13);
HI351_write_cmos_sensor(0x4a, 0x13);
HI351_write_cmos_sensor(0x4b, 0x1e);
HI351_write_cmos_sensor(0x4c, 0x71);

HI351_write_cmos_sensor(0x4d, 0x2f); //CMC_01
HI351_write_cmos_sensor(0x4e, 0x68);
HI351_write_cmos_sensor(0x4f, 0x29);
HI351_write_cmos_sensor(0x50, 0x01);
HI351_write_cmos_sensor(0x51, 0x19);
HI351_write_cmos_sensor(0x52, 0x6c);
HI351_write_cmos_sensor(0x53, 0x13);
HI351_write_cmos_sensor(0x54, 0x13);
HI351_write_cmos_sensor(0x55, 0x1e);
HI351_write_cmos_sensor(0x56, 0x71);

HI351_write_cmos_sensor(0x57, 0x2f); //CMC_02
HI351_write_cmos_sensor(0x58, 0x68);
HI351_write_cmos_sensor(0x59, 0x29);
HI351_write_cmos_sensor(0x5a, 0x01);
HI351_write_cmos_sensor(0x5b, 0x19);
HI351_write_cmos_sensor(0x5c, 0x6c);
HI351_write_cmos_sensor(0x5d, 0x13);
HI351_write_cmos_sensor(0x5e, 0x13);
HI351_write_cmos_sensor(0x5f, 0x1e);
HI351_write_cmos_sensor(0x60, 0x71);

HI351_write_cmos_sensor(0x61, 0x2f); //CMC_03
HI351_write_cmos_sensor(0x62, 0x6a);
HI351_write_cmos_sensor(0x63, 0x32);
HI351_write_cmos_sensor(0x64, 0x08);
HI351_write_cmos_sensor(0x65, 0x1a);
HI351_write_cmos_sensor(0x66, 0x6c);
HI351_write_cmos_sensor(0x67, 0x12);
HI351_write_cmos_sensor(0x68, 0x03);
HI351_write_cmos_sensor(0x69, 0x30);
HI351_write_cmos_sensor(0x6a, 0x73);

HI351_write_cmos_sensor(0x6b, 0x2f);	//CMC_04
HI351_write_cmos_sensor(0x6c, 0x68);
HI351_write_cmos_sensor(0x6d, 0x29);
HI351_write_cmos_sensor(0x6e, 0x01);
HI351_write_cmos_sensor(0x6f, 0x17);
HI351_write_cmos_sensor(0x70, 0x6c);
HI351_write_cmos_sensor(0x71, 0x15);
HI351_write_cmos_sensor(0x72, 0x01);
HI351_write_cmos_sensor(0x73, 0x30);
HI351_write_cmos_sensor(0x74, 0x71);

HI351_write_cmos_sensor(0x75, 0x2f);	//CMC_05
HI351_write_cmos_sensor(0x76, 0x68);
HI351_write_cmos_sensor(0x77, 0x29);
HI351_write_cmos_sensor(0x78, 0x01);
HI351_write_cmos_sensor(0x79, 0x17);
HI351_write_cmos_sensor(0x7a, 0x6c);
HI351_write_cmos_sensor(0x7b, 0x15);
HI351_write_cmos_sensor(0x7c, 0x01);
HI351_write_cmos_sensor(0x7d, 0x30);
HI351_write_cmos_sensor(0x7e, 0x71);

HI351_write_cmos_sensor(0x7f, 0x2f); //CMC_06
HI351_write_cmos_sensor(0x80, 0x6a);
HI351_write_cmos_sensor(0x81, 0x3c);
HI351_write_cmos_sensor(0x82, 0x12);
HI351_write_cmos_sensor(0x83, 0x30);
HI351_write_cmos_sensor(0x84, 0x70);
HI351_write_cmos_sensor(0x85, 0x00);
HI351_write_cmos_sensor(0x86, 0x06);
HI351_write_cmos_sensor(0x87, 0x2d);
HI351_write_cmos_sensor(0x88, 0x73);

HI351_write_cmos_sensor(0x89, 0x2f); //CMC_07
HI351_write_cmos_sensor(0x8a, 0x6a);
HI351_write_cmos_sensor(0x8b, 0x3c);
HI351_write_cmos_sensor(0x8c, 0x12);
HI351_write_cmos_sensor(0x8d, 0x30);
HI351_write_cmos_sensor(0x8e, 0x70);
HI351_write_cmos_sensor(0x8f, 0x00);
HI351_write_cmos_sensor(0x90, 0x06);
HI351_write_cmos_sensor(0x91, 0x2d);
HI351_write_cmos_sensor(0x92, 0x73);

HI351_write_cmos_sensor(0x93, 0x2f); //CMC_08
HI351_write_cmos_sensor(0x94, 0x6a);
HI351_write_cmos_sensor(0x95, 0x3c);
HI351_write_cmos_sensor(0x96, 0x12);
HI351_write_cmos_sensor(0x97, 0x30);
HI351_write_cmos_sensor(0x98, 0x70);
HI351_write_cmos_sensor(0x99, 0x00);
HI351_write_cmos_sensor(0x9a, 0x06);
HI351_write_cmos_sensor(0x9b, 0x2d);
HI351_write_cmos_sensor(0x9c, 0x73);

HI351_write_cmos_sensor(0x9d, 0x2f); //CMC_09
HI351_write_cmos_sensor(0x9e, 0x6a);
HI351_write_cmos_sensor(0x9f, 0x3c);
HI351_write_cmos_sensor(0xa0, 0x12);
HI351_write_cmos_sensor(0xa1, 0x30);
HI351_write_cmos_sensor(0xa2, 0x70);
HI351_write_cmos_sensor(0xa3, 0x00);
HI351_write_cmos_sensor(0xa4, 0x06);
HI351_write_cmos_sensor(0xa5, 0x2d);
HI351_write_cmos_sensor(0xa6, 0x73);

HI351_write_cmos_sensor(0xa7, 0x2f); //CMC_10
HI351_write_cmos_sensor(0xa8, 0x6a);
HI351_write_cmos_sensor(0xa9, 0x3c);
HI351_write_cmos_sensor(0xaa, 0x12);
HI351_write_cmos_sensor(0xab, 0x30);
HI351_write_cmos_sensor(0xac, 0x70);
HI351_write_cmos_sensor(0xad, 0x00);
HI351_write_cmos_sensor(0xae, 0x06);
HI351_write_cmos_sensor(0xaf, 0x2d);
HI351_write_cmos_sensor(0xb0, 0x73);

HI351_write_cmos_sensor(0xb1, 0x2f); //CMC_11
HI351_write_cmos_sensor(0xb2, 0x6a);
HI351_write_cmos_sensor(0xb3, 0x3c);
HI351_write_cmos_sensor(0xb4, 0x12);
HI351_write_cmos_sensor(0xb5, 0x30);
HI351_write_cmos_sensor(0xb6, 0x70);
HI351_write_cmos_sensor(0xb7, 0x00);
HI351_write_cmos_sensor(0xb8, 0x06);
HI351_write_cmos_sensor(0xb9, 0x2d);
HI351_write_cmos_sensor(0xba, 0x73);

///////////////////////////////////////////
// D1~D2~D3 Page Adaptive Multi-CMC
///////////////////////////////////////////
//MCMC_00
HI351_write_cmos_sensor(0xbb, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0xbc, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0xbd, 0x80); //0_GAIN
HI351_write_cmos_sensor(0xbe, 0x00); //0_HUE
HI351_write_cmos_sensor(0xbf, 0x35); //0_CENTER
HI351_write_cmos_sensor(0xc0, 0x13); //0_DELTA
HI351_write_cmos_sensor(0xc1, 0x80); //1_GAIN
HI351_write_cmos_sensor(0xc2, 0x00); //1_HUE
HI351_write_cmos_sensor(0xc3, 0x6a); //1_CENTER
HI351_write_cmos_sensor(0xc4, 0x1c); //1_DELTA
HI351_write_cmos_sensor(0xc5, 0x80); //2_GAIN
HI351_write_cmos_sensor(0xc6, 0x00); //2_HUE
HI351_write_cmos_sensor(0xc7, 0xac); //2_CENTER
HI351_write_cmos_sensor(0xc8, 0x1c); //2_DELTA
HI351_write_cmos_sensor(0xc9, 0x95); //3_GAIN
HI351_write_cmos_sensor(0xca, 0x00); //3_HUE
HI351_write_cmos_sensor(0xcb, 0x50); //3_CENTER
HI351_write_cmos_sensor(0xcc, 0x20); //3_DELTA
HI351_write_cmos_sensor(0xcd, 0x80); //4_GAIN
HI351_write_cmos_sensor(0xce, 0x00); //4_HUE
HI351_write_cmos_sensor(0xcf, 0x7c); //4_CENTER
HI351_write_cmos_sensor(0xd0, 0x1c); //4_DELTA
HI351_write_cmos_sensor(0xd1, 0x80); //5_GAIN
HI351_write_cmos_sensor(0xd2, 0x00); //5_HUE
HI351_write_cmos_sensor(0xd3, 0x99); //5_CENTER
HI351_write_cmos_sensor(0xd4, 0x1d); //5_DELTA
//MCMC_01
HI351_write_cmos_sensor(0xd5, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0xd6, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0xd7, 0x80); //0_GAIN
HI351_write_cmos_sensor(0xd8, 0x00); //0_HUE
HI351_write_cmos_sensor(0xd9, 0x35); //0_CENTER
HI351_write_cmos_sensor(0xda, 0x13); //0_DELTA
HI351_write_cmos_sensor(0xdb, 0x80); //1_GAIN
HI351_write_cmos_sensor(0xdc, 0x00); //1_HUE
HI351_write_cmos_sensor(0xdd, 0x6a); //1_CENTER
HI351_write_cmos_sensor(0xde, 0x1c); //1_DELTA
HI351_write_cmos_sensor(0xdf, 0x80); //2_GAIN
HI351_write_cmos_sensor(0xe0, 0x00); //2_HUE
HI351_write_cmos_sensor(0xe1, 0xac); //2_CENTER
HI351_write_cmos_sensor(0xe2, 0x1c); //2_DELTA
HI351_write_cmos_sensor(0xe3, 0x95); //3_GAIN
HI351_write_cmos_sensor(0xe4, 0x00); //3_HUE
HI351_write_cmos_sensor(0xe5, 0x50); //3_CENTER
HI351_write_cmos_sensor(0xe6, 0x20); //3_DELTA
HI351_write_cmos_sensor(0xe7, 0x80); //4_GAIN
HI351_write_cmos_sensor(0xe8, 0x00); //4_HUE
HI351_write_cmos_sensor(0xe9, 0x7c); //4_CENTER
HI351_write_cmos_sensor(0xea, 0x1c); //4_DELTA
HI351_write_cmos_sensor(0xeb, 0x80); //5_GAIN
HI351_write_cmos_sensor(0xec, 0x00); //5_HUE
HI351_write_cmos_sensor(0xed, 0x99); //5_CENTER
HI351_write_cmos_sensor(0xee, 0x1d); //5_DELTA
//MCMC_02
HI351_write_cmos_sensor(0xef, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0xf0, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0xf1, 0x80); //0_GAIN
HI351_write_cmos_sensor(0xf2, 0x00); //0_HUE
HI351_write_cmos_sensor(0xf3, 0x35); //0_CENTER
HI351_write_cmos_sensor(0xf4, 0x13); //0_DELTA
HI351_write_cmos_sensor(0xf5, 0x80); //1_GAIN
HI351_write_cmos_sensor(0xf6, 0x00); //1_HUE
HI351_write_cmos_sensor(0xf7, 0x6a); //1_CENTER
HI351_write_cmos_sensor(0xf8, 0x1c); //1_DELTA
HI351_write_cmos_sensor(0xf9, 0x80); //2_GAIN
HI351_write_cmos_sensor(0xfa, 0x00); //2_HUE
HI351_write_cmos_sensor(0xfb, 0xac); //2_CENTER
HI351_write_cmos_sensor(0xfc, 0x1c); //2_DELTA
HI351_write_cmos_sensor(0xfd, 0x95); //3_GAIN
HI351_write_cmos_sensor(0x0e, 0x00); //burst end
HI351_write_cmos_sensor(0x03, 0xd2); //Page d2
HI351_write_cmos_sensor(0x0e, 0x01); //burst start
HI351_write_cmos_sensor(0x10, 0x00); //3_HUE
HI351_write_cmos_sensor(0x11, 0x50); //3_CENTER
HI351_write_cmos_sensor(0x12, 0x20); //3_DELTA
HI351_write_cmos_sensor(0x13, 0x80); //4_GAIN
HI351_write_cmos_sensor(0x14, 0x00); //4_HUE
HI351_write_cmos_sensor(0x15, 0x7c); //4_CENTER
HI351_write_cmos_sensor(0x16, 0x1c); //4_DELTA
HI351_write_cmos_sensor(0x17, 0x80); //5_GAIN
HI351_write_cmos_sensor(0x18, 0x00); //5_HUE
HI351_write_cmos_sensor(0x19, 0x99); //5_CENTER
HI351_write_cmos_sensor(0x1a, 0x1d); //5_DELTA
//MCMC_03
HI351_write_cmos_sensor(0x1b, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0x1c, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0x1d, 0x70); //0_GAIN
HI351_write_cmos_sensor(0x1e, 0x04); //0_HUE
HI351_write_cmos_sensor(0x1f, 0x36); //0_CENTER
HI351_write_cmos_sensor(0x20, 0x0d); //0_DELTA
HI351_write_cmos_sensor(0x21, 0xbe); //1_GAIN
HI351_write_cmos_sensor(0x22, 0x10); //1_HUE
HI351_write_cmos_sensor(0x23, 0x6b); //1_CENTER
HI351_write_cmos_sensor(0x24, 0x1e); //1_DELTA
HI351_write_cmos_sensor(0x25, 0x80); //2_GAIN
HI351_write_cmos_sensor(0x26, 0x00); //2_HUE
HI351_write_cmos_sensor(0x27, 0xaf); //2_CENTER
HI351_write_cmos_sensor(0x28, 0x1c); //2_DELTA
HI351_write_cmos_sensor(0x29, 0x80); //3_GAIN
HI351_write_cmos_sensor(0x2a, 0x87); //3_HUE
HI351_write_cmos_sensor(0x2b, 0x51); //3_CENTER
HI351_write_cmos_sensor(0x2c, 0x1c); //3_DELTA
HI351_write_cmos_sensor(0x2d, 0xbe); //4_GAIN
HI351_write_cmos_sensor(0x2e, 0x10); //4_HUE
HI351_write_cmos_sensor(0x2f, 0x76); //4_CENTER
HI351_write_cmos_sensor(0x30, 0x1e); //4_DELTA
HI351_write_cmos_sensor(0x31, 0x80); //5_GAIN
HI351_write_cmos_sensor(0x32, 0x00); //5_HUE
HI351_write_cmos_sensor(0x33, 0x9a); //5_CENTER
HI351_write_cmos_sensor(0x34, 0x14); //5_DELTA
//MCMC_04
HI351_write_cmos_sensor(0x35, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0x36, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0x37, 0x80); //0_GAIN
HI351_write_cmos_sensor(0x38, 0x0a); //0_HUE
HI351_write_cmos_sensor(0x39, 0x32); //0_CENTER
HI351_write_cmos_sensor(0x3a, 0x0f); //0_DELTA
HI351_write_cmos_sensor(0x3b, 0x90); //1_GAIN
HI351_write_cmos_sensor(0x3c, 0x14); //1_HUE
HI351_write_cmos_sensor(0x3d, 0x6a); //1_CENTER
HI351_write_cmos_sensor(0x3e, 0x14); //1_DELTA
HI351_write_cmos_sensor(0x3f, 0x70); //2_GAIN
HI351_write_cmos_sensor(0x40, 0x8e); //2_HUE
HI351_write_cmos_sensor(0x41, 0xaf); //2_CENTER
HI351_write_cmos_sensor(0x42, 0x1c); //2_DELTA
HI351_write_cmos_sensor(0x43, 0x9a); //3_GAIN
HI351_write_cmos_sensor(0x44, 0x00); //3_HUE
HI351_write_cmos_sensor(0x45, 0x51); //3_CENTER
HI351_write_cmos_sensor(0x46, 0x18); //3_DELTA
HI351_write_cmos_sensor(0x47, 0xa9); //4_GAIN
HI351_write_cmos_sensor(0x48, 0x10); //4_HUE
HI351_write_cmos_sensor(0x49, 0x7c); //4_CENTER
HI351_write_cmos_sensor(0x4a, 0x18); //4_DELTA
HI351_write_cmos_sensor(0x4b, 0xa0); //5_GAIN
HI351_write_cmos_sensor(0x4c, 0x00); //5_HUE
HI351_write_cmos_sensor(0x4d, 0x99); //5_CENTER
HI351_write_cmos_sensor(0x4e, 0x1e); //5_DELTA
//MCMC_05
HI351_write_cmos_sensor(0x4f, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0x50, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0x51, 0x78); //0_GAIN
HI351_write_cmos_sensor(0x52, 0x08); //0_HUE
HI351_write_cmos_sensor(0x53, 0x32); //0_CENTER
HI351_write_cmos_sensor(0x54, 0x0f); //0_DELTA
HI351_write_cmos_sensor(0x55, 0x9a); //1_GAIN
HI351_write_cmos_sensor(0x56, 0x0a); //1_HUE
HI351_write_cmos_sensor(0x57, 0x6a); //1_CENTER
HI351_write_cmos_sensor(0x58, 0x14); //1_DELTA
HI351_write_cmos_sensor(0x59, 0x70); //2_GAIN
HI351_write_cmos_sensor(0x5a, 0x88); //2_HUE
HI351_write_cmos_sensor(0x5b, 0xaf); //2_CENTER
HI351_write_cmos_sensor(0x5c, 0x1c); //2_DELTA
HI351_write_cmos_sensor(0x5d, 0xa6); //3_GAIN
HI351_write_cmos_sensor(0x5e, 0x08); //3_HUE
HI351_write_cmos_sensor(0x5f, 0x4b); //3_CENTER
HI351_write_cmos_sensor(0x60, 0x1e); //3_DELTA
HI351_write_cmos_sensor(0x61, 0x90); //4_GAIN
HI351_write_cmos_sensor(0x62, 0x10); //4_HUE
HI351_write_cmos_sensor(0x63, 0x7c); //4_CENTER
HI351_write_cmos_sensor(0x64, 0x18); //4_DELTA
HI351_write_cmos_sensor(0x65, 0xa0); //5_GAIN
HI351_write_cmos_sensor(0x66, 0x00); //5_HUE
HI351_write_cmos_sensor(0x67, 0x99); //5_CENTER
HI351_write_cmos_sensor(0x68, 0x1e); //5_DELTA
//MCMC_06
HI351_write_cmos_sensor(0x69, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0x6a, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0x6b, 0x60); //0_GAIN
HI351_write_cmos_sensor(0x6c, 0x08); //0_HUE
HI351_write_cmos_sensor(0x6d, 0x34); //0_CENTER
HI351_write_cmos_sensor(0x6e, 0x0c); //0_DELTA
HI351_write_cmos_sensor(0x6f, 0x93); //1_GAIN
HI351_write_cmos_sensor(0x70, 0x10); //1_HUE
HI351_write_cmos_sensor(0x71, 0x5e); //1_CENTER
HI351_write_cmos_sensor(0x72, 0x14); //1_DELTA
HI351_write_cmos_sensor(0x73, 0xa0); //2_GAIN
HI351_write_cmos_sensor(0x74, 0x88); //2_HUE
HI351_write_cmos_sensor(0x75, 0xaf); //2_CENTER
HI351_write_cmos_sensor(0x76, 0x28); //2_DELTA
HI351_write_cmos_sensor(0x77, 0x80); //3_GAIN
HI351_write_cmos_sensor(0x78, 0x8a); //3_HUE
HI351_write_cmos_sensor(0x79, 0x48); //3_CENTER
HI351_write_cmos_sensor(0x7a, 0x14); //3_DELTA
HI351_write_cmos_sensor(0x7b, 0x80); //4_GAIN
HI351_write_cmos_sensor(0x7c, 0x0c); //4_HUE
HI351_write_cmos_sensor(0x7d, 0x72); //4_CENTER
HI351_write_cmos_sensor(0x7e, 0x14); //4_DELTA
HI351_write_cmos_sensor(0x7f, 0x80); //5_GAIN
HI351_write_cmos_sensor(0x80, 0x00); //5_HUE
HI351_write_cmos_sensor(0x81, 0x9a); //5_CENTER
HI351_write_cmos_sensor(0x82, 0x14); //5_DELTA
//MCMC_07
HI351_write_cmos_sensor(0x83, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0x84, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0x85, 0x60); //0_GAIN
HI351_write_cmos_sensor(0x86, 0x08); //0_HUE
HI351_write_cmos_sensor(0x87, 0x34); //0_CENTER
HI351_write_cmos_sensor(0x88, 0x0c); //0_DELTA
HI351_write_cmos_sensor(0x89, 0x93); //1_GAIN
HI351_write_cmos_sensor(0x8a, 0x10); //1_HUE
HI351_write_cmos_sensor(0x8b, 0x5e); //1_CENTER
HI351_write_cmos_sensor(0x8c, 0x14); //1_DELTA
HI351_write_cmos_sensor(0x8d, 0xa0); //2_GAIN
HI351_write_cmos_sensor(0x8e, 0x88); //2_HUE
HI351_write_cmos_sensor(0x8f, 0xaf); //2_CENTER
HI351_write_cmos_sensor(0x90, 0x28); //2_DELTA
HI351_write_cmos_sensor(0x91, 0x80); //3_GAIN
HI351_write_cmos_sensor(0x92, 0x8a); //3_HUE
HI351_write_cmos_sensor(0x93, 0x48); //3_CENTER
HI351_write_cmos_sensor(0x94, 0x14); //3_DELTA
HI351_write_cmos_sensor(0x95, 0x80); //4_GAIN
HI351_write_cmos_sensor(0x96, 0x0c); //4_HUE
HI351_write_cmos_sensor(0x97, 0x72); //4_CENTER
HI351_write_cmos_sensor(0x98, 0x14); //4_DELTA
HI351_write_cmos_sensor(0x99, 0x80); //5_GAIN
HI351_write_cmos_sensor(0x9a, 0x00); //5_HUE
HI351_write_cmos_sensor(0x9b, 0x9a); //5_CENTER
HI351_write_cmos_sensor(0x9c, 0x14); //5_DELTA
//MCMC_08
HI351_write_cmos_sensor(0x9d, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0x9e, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0x9f, 0x60); //0_GAIN
HI351_write_cmos_sensor(0xa0, 0x08); //0_HUE
HI351_write_cmos_sensor(0xa1, 0x34); //0_CENTER
HI351_write_cmos_sensor(0xa2, 0x0c); //0_DELTA
HI351_write_cmos_sensor(0xa3, 0x93); //1_GAIN
HI351_write_cmos_sensor(0xa4, 0x10); //1_HUE
HI351_write_cmos_sensor(0xa5, 0x5e); //1_CENTER
HI351_write_cmos_sensor(0xa6, 0x14); //1_DELTA
HI351_write_cmos_sensor(0xa7, 0xa0); //2_GAIN
HI351_write_cmos_sensor(0xa8, 0x88); //2_HUE
HI351_write_cmos_sensor(0xa9, 0xaf); //2_CENTER
HI351_write_cmos_sensor(0xaa, 0x28); //2_DELTA
HI351_write_cmos_sensor(0xab, 0x80); //3_GAIN
HI351_write_cmos_sensor(0xac, 0x8a); //3_HUE
HI351_write_cmos_sensor(0xad, 0x48); //3_CENTER
HI351_write_cmos_sensor(0xae, 0x14); //3_DELTA
HI351_write_cmos_sensor(0xaf, 0x80); //4_GAIN
HI351_write_cmos_sensor(0xb0, 0x0c); //4_HUE
HI351_write_cmos_sensor(0xb1, 0x72); //4_CENTER
HI351_write_cmos_sensor(0xb2, 0x14); //4_DELTA
HI351_write_cmos_sensor(0xb3, 0x80); //5_GAIN
HI351_write_cmos_sensor(0xb4, 0x00); //5_HUE
HI351_write_cmos_sensor(0xb5, 0x9a); //5_CENTER
HI351_write_cmos_sensor(0xb6, 0x14); //5_DELTA
//MCMC_09
HI351_write_cmos_sensor(0xb7, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0xb8, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0xb9, 0x60); //0_GAIN
HI351_write_cmos_sensor(0xba, 0x08); //0_HUE
HI351_write_cmos_sensor(0xbb, 0x34); //0_CENTER
HI351_write_cmos_sensor(0xbc, 0x0c); //0_DELTA
HI351_write_cmos_sensor(0xbd, 0x93); //1_GAIN
HI351_write_cmos_sensor(0xbe, 0x10); //1_HUE
HI351_write_cmos_sensor(0xbf, 0x5e); //1_CENTER
HI351_write_cmos_sensor(0xc0, 0x14); //1_DELTA
HI351_write_cmos_sensor(0xc1, 0xa0); //2_GAIN
HI351_write_cmos_sensor(0xc2, 0x88); //2_HUE
HI351_write_cmos_sensor(0xc3, 0xaf); //2_CENTER
HI351_write_cmos_sensor(0xc4, 0x28); //2_DELTA
HI351_write_cmos_sensor(0xc5, 0x80); //3_GAIN
HI351_write_cmos_sensor(0xc6, 0x05); //3_HUE
HI351_write_cmos_sensor(0xc7, 0x48); //3_CENTER
HI351_write_cmos_sensor(0xc8, 0x14); //3_DELTA
HI351_write_cmos_sensor(0xc9, 0x80); //4_GAIN
HI351_write_cmos_sensor(0xca, 0x0c); //4_HUE
HI351_write_cmos_sensor(0xcb, 0x72); //4_CENTER
HI351_write_cmos_sensor(0xcc, 0x14); //4_DELTA
HI351_write_cmos_sensor(0xcd, 0x80); //5_GAIN
HI351_write_cmos_sensor(0xce, 0x00); //5_HUE
HI351_write_cmos_sensor(0xcf, 0x9a); //5_CENTER
HI351_write_cmos_sensor(0xd0, 0x14); //5_DELTA
//MCMC_10
HI351_write_cmos_sensor(0xd1, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0xd2, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0xd3, 0x60); //0_GAIN
HI351_write_cmos_sensor(0xd4, 0x08); //0_HUE
HI351_write_cmos_sensor(0xd5, 0x34); //0_CENTER
HI351_write_cmos_sensor(0xd6, 0x0c); //0_DELTA
HI351_write_cmos_sensor(0xd7, 0x93); //1_GAIN
HI351_write_cmos_sensor(0xd8, 0x10); //1_HUE
HI351_write_cmos_sensor(0xd9, 0x5e); //1_CENTER
HI351_write_cmos_sensor(0xda, 0x14); //1_DELTA
HI351_write_cmos_sensor(0xdb, 0xa0); //2_GAIN
HI351_write_cmos_sensor(0xdc, 0x88); //2_HUE
HI351_write_cmos_sensor(0xdd, 0xaf); //2_CENTER
HI351_write_cmos_sensor(0xde, 0x28); //2_DELTA
HI351_write_cmos_sensor(0xdf, 0x80); //3_GAIN
HI351_write_cmos_sensor(0xe0, 0x05); //3_HUE
HI351_write_cmos_sensor(0xe1, 0x48); //3_CENTER
HI351_write_cmos_sensor(0xe2, 0x14); //3_DELTA
HI351_write_cmos_sensor(0xe3, 0x80); //4_GAIN
HI351_write_cmos_sensor(0xe4, 0x0c); //4_HUE
HI351_write_cmos_sensor(0xe5, 0x72); //4_CENTER
HI351_write_cmos_sensor(0xe6, 0x14); //4_DELTA
HI351_write_cmos_sensor(0xe7, 0x80); //5_GAIN
HI351_write_cmos_sensor(0xe8, 0x00); //5_HUE
HI351_write_cmos_sensor(0xe9, 0x9a); //5_CENTER
HI351_write_cmos_sensor(0xea, 0x14); //5_DELTA
//MCMC_11
HI351_write_cmos_sensor(0xeb, 0x80); //GLB_GAIN
HI351_write_cmos_sensor(0xec, 0x00); //GLB_HUE
HI351_write_cmos_sensor(0xed, 0x60); //0_GAIN
HI351_write_cmos_sensor(0xee, 0x08); //0_HUE
HI351_write_cmos_sensor(0xef, 0x34); //0_CENTER
HI351_write_cmos_sensor(0xf0, 0x0c); //0_DELTA
HI351_write_cmos_sensor(0xf1, 0x93); //1_GAIN
HI351_write_cmos_sensor(0xf2, 0x10); //1_HUE
HI351_write_cmos_sensor(0xf3, 0x5e); //1_CENTER
HI351_write_cmos_sensor(0xf4, 0x14); //1_DELTA
HI351_write_cmos_sensor(0xf5, 0xa0); //2_GAIN
HI351_write_cmos_sensor(0xf6, 0x88); //2_HUE
HI351_write_cmos_sensor(0xf7, 0xaf); //2_CENTER
HI351_write_cmos_sensor(0xf8, 0x28); //2_DELTA
HI351_write_cmos_sensor(0xf9, 0x80); //3_GAIN
HI351_write_cmos_sensor(0xfa, 0x05); //3_HUE
HI351_write_cmos_sensor(0xfb, 0x48); //3_CENTER
HI351_write_cmos_sensor(0xfc, 0x14); //3_DELTA
HI351_write_cmos_sensor(0xfd, 0x80); //4_GAIN
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
HI351_write_cmos_sensor(0x03, 0xd3); //Page d3
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x0c); //4_HUE
HI351_write_cmos_sensor(0x11, 0x72); //4_CENTER
HI351_write_cmos_sensor(0x12, 0x14); //4_DELTA
HI351_write_cmos_sensor(0x13, 0x80); //5_GAIN
HI351_write_cmos_sensor(0x14, 0x00); //5_HUE
HI351_write_cmos_sensor(0x15, 0x9a); //5_CENTER
HI351_write_cmos_sensor(0x16, 0x14); //5_DELTA
/////////////////////////////////////////////
// D3 Page Adaptive LSC
/////////////////////////////////////////////
HI351_write_cmos_sensor(0x17, 0x00); //LSC 00 ofs GB
HI351_write_cmos_sensor(0x18, 0x00); //LSC 00 ofs B
HI351_write_cmos_sensor(0x19, 0x00); //LSC 00 ofs R
HI351_write_cmos_sensor(0x1a, 0x00); //LSC 00 ofs GR
HI351_write_cmos_sensor(0x1b, 0x5c); //LSC 00 Gain GB
HI351_write_cmos_sensor(0x1c, 0x60); //LSC 00 Gain B
HI351_write_cmos_sensor(0x1d, 0x5e); //LSC 00 Gain R
HI351_write_cmos_sensor(0x1e, 0x5c); //LSC 00 Gain GR
HI351_write_cmos_sensor(0x1f, 0x00); //LSC 01 ofs GB
HI351_write_cmos_sensor(0x20, 0x00); //LSC 01 ofs B
HI351_write_cmos_sensor(0x21, 0x00); //LSC 01 ofs R
HI351_write_cmos_sensor(0x22, 0x00); //LSC 01 ofs GR
HI351_write_cmos_sensor(0x23, 0x5c); //LSC 01 Gain GB
HI351_write_cmos_sensor(0x24, 0x60); //LSC 01 Gain B
HI351_write_cmos_sensor(0x25, 0x5e); //LSC 01 Gain R
HI351_write_cmos_sensor(0x26, 0x5c); //LSC 01 Gain GR
HI351_write_cmos_sensor(0x27, 0x00); //LSC 02 ofs GB
HI351_write_cmos_sensor(0x28, 0x00); //LSC 02 ofs B
HI351_write_cmos_sensor(0x29, 0x00); //LSC 02 ofs R
HI351_write_cmos_sensor(0x2a, 0x00); //LSC 02 ofs GR
HI351_write_cmos_sensor(0x2b, 0x5c); //LSC 02 Gain GB
HI351_write_cmos_sensor(0x2c, 0x69); //LSC 02 Gain B
HI351_write_cmos_sensor(0x2d, 0x65); //LSC 02 Gain R
HI351_write_cmos_sensor(0x2e, 0x5c); //LSC 02 Gain GR
HI351_write_cmos_sensor(0x2f, 0x00); //LSC 03 ofs GB
HI351_write_cmos_sensor(0x30, 0x00); //LSC 03 ofs B
HI351_write_cmos_sensor(0x31, 0x00); //LSC 03 ofs R
HI351_write_cmos_sensor(0x32, 0x00); //LSC 03 ofs GR
HI351_write_cmos_sensor(0x33, 0x80); //LSC 03 Gain GB
HI351_write_cmos_sensor(0x34, 0x85); //LSC 03 Gain B
HI351_write_cmos_sensor(0x35, 0x78); //LSC 03 Gain R
HI351_write_cmos_sensor(0x36, 0x80); //LSC 03 Gain GR
HI351_write_cmos_sensor(0x37, 0x00); //LSC 04 ofs GB
HI351_write_cmos_sensor(0x38, 0x00); //LSC 04 ofs B
HI351_write_cmos_sensor(0x39, 0x00); //LSC 04 ofs R
HI351_write_cmos_sensor(0x3a, 0x00); //LSC 04 ofs GR
HI351_write_cmos_sensor(0x3b, 0x80); //LSC 04 Gain GB
HI351_write_cmos_sensor(0x3c, 0x80); //LSC 04 Gain B
HI351_write_cmos_sensor(0x3d, 0x77); //LSC 04 Gain R
HI351_write_cmos_sensor(0x3e, 0x80); //LSC 04 Gain GR
HI351_write_cmos_sensor(0x3f, 0x00); //LSC 05 ofs GB
HI351_write_cmos_sensor(0x40, 0x00); //LSC 05 ofs B
HI351_write_cmos_sensor(0x41, 0x00); //LSC 05 ofs R
HI351_write_cmos_sensor(0x42, 0x00); //LSC 05 ofs GR
HI351_write_cmos_sensor(0x43, 0x80); //LSC 05 Gain GB
HI351_write_cmos_sensor(0x44, 0x80); //LSC 05 Gain B
HI351_write_cmos_sensor(0x45, 0x81); //LSC 05 Gain R//98->80
HI351_write_cmos_sensor(0x46, 0x80); //LSC 05 Gain GR
HI351_write_cmos_sensor(0x47, 0x00); //LSC 06 ofs GB
HI351_write_cmos_sensor(0x48, 0x00); //LSC 06 ofs B
HI351_write_cmos_sensor(0x49, 0x00); //LSC 06 ofs R
HI351_write_cmos_sensor(0x4a, 0x00); //LSC 06 ofs GR
HI351_write_cmos_sensor(0x4b, 0x80); //LSC 06 Gain GB
HI351_write_cmos_sensor(0x4c, 0x84); //LSC 06 Gain B
HI351_write_cmos_sensor(0x4d, 0x80); //LSC 06 Gain R
HI351_write_cmos_sensor(0x4e, 0x80); //LSC 06 Gain GR
HI351_write_cmos_sensor(0x4f, 0x00); //LSC 07 ofs GB
HI351_write_cmos_sensor(0x50, 0x00); //LSC 07 ofs B
HI351_write_cmos_sensor(0x51, 0x00); //LSC 07 ofs R
HI351_write_cmos_sensor(0x52, 0x00); //LSC 07 ofs GR
HI351_write_cmos_sensor(0x53, 0x80); //LSC 07 Gain GB
HI351_write_cmos_sensor(0x54, 0x84); //LSC 07 Gain B
HI351_write_cmos_sensor(0x55, 0x78); //LSC 07 Gain R
HI351_write_cmos_sensor(0x56, 0x80); //LSC 07 Gain GR
HI351_write_cmos_sensor(0x57, 0x00); //LSC 08 ofs GB
HI351_write_cmos_sensor(0x58, 0x00); //LSC 08 ofs B
HI351_write_cmos_sensor(0x59, 0x00); //LSC 08 ofs R
HI351_write_cmos_sensor(0x5a, 0x00); //LSC 08 ofs GR
HI351_write_cmos_sensor(0x5b, 0x80); //LSC 08 Gain GB
HI351_write_cmos_sensor(0x5c, 0x84); //LSC 08 Gain B
HI351_write_cmos_sensor(0x5d, 0x8f); //LSC 08 Gain R
HI351_write_cmos_sensor(0x5e, 0x80); //LSC 08 Gain GR
HI351_write_cmos_sensor(0x5f, 0x00); //LSC 09 ofs GB
HI351_write_cmos_sensor(0x60, 0x00); //LSC 09 ofs B
HI351_write_cmos_sensor(0x61, 0x00); //LSC 09 ofs R
HI351_write_cmos_sensor(0x62, 0x00); //LSC 09 ofs GR
HI351_write_cmos_sensor(0x63, 0x80); //LSC 09 Gain GB
HI351_write_cmos_sensor(0x64, 0x84); //LSC 09 Gain B
HI351_write_cmos_sensor(0x65, 0x8f); //LSC 09 Gain R
HI351_write_cmos_sensor(0x66, 0x80); //LSC 09 Gain GR
HI351_write_cmos_sensor(0x67, 0x00); //LSC 10 ofs GB
HI351_write_cmos_sensor(0x68, 0x00); //LSC 10 ofs B
HI351_write_cmos_sensor(0x69, 0x00); //LSC 10 ofs R
HI351_write_cmos_sensor(0x6a, 0x00); //LSC 10 ofs GR
HI351_write_cmos_sensor(0x6b, 0x80); //LSC 10 Gain GB
HI351_write_cmos_sensor(0x6c, 0x84); //LSC 10 Gain B
HI351_write_cmos_sensor(0x6d, 0x8f); //LSC 10 Gain R
HI351_write_cmos_sensor(0x6e, 0x80); //LSC 10 Gain GR
HI351_write_cmos_sensor(0x6f, 0x00); //LSC 11 ofs GB
HI351_write_cmos_sensor(0x70, 0x00); //LSC 11 ofs B
HI351_write_cmos_sensor(0x71, 0x00); //LSC 11 ofs R
HI351_write_cmos_sensor(0x72, 0x00); //LSC 11 ofs GR
HI351_write_cmos_sensor(0x73, 0x80); //LSC 11 Gain GB
HI351_write_cmos_sensor(0x74, 0x84); //LSC 11 Gain B
HI351_write_cmos_sensor(0x75, 0x8f); //LSC 11 Gain R
HI351_write_cmos_sensor(0x76, 0x80); //LSC 11 Gain GR
///////////////////////////////////////////
// D3 Page OTP, ROM Select TH
///////////////////////////////////////////
HI351_write_cmos_sensor(0x77, 0x60); //2 ROM High
HI351_write_cmos_sensor(0x78, 0x20); //2 ROM Low
HI351_write_cmos_sensor(0x79, 0x60); //3 OTP High
HI351_write_cmos_sensor(0x7a, 0x40); //3 OTP Mid
HI351_write_cmos_sensor(0x7b, 0x20); //3 OTP Low
///////////////////////////////////////////
// D3 Page Adaptive DNP
/////////////////////////////////////////////
HI351_write_cmos_sensor(0x7c, 0x00); //EV max 200fps
HI351_write_cmos_sensor(0x7d, 0x04);
HI351_write_cmos_sensor(0x7e, 0xa2);
HI351_write_cmos_sensor(0x7f, 0x86);
HI351_write_cmos_sensor(0x80, 0x00); //EV min 1000fps
HI351_write_cmos_sensor(0x81, 0x00);
HI351_write_cmos_sensor(0x82, 0xe6);
HI351_write_cmos_sensor(0x83, 0xb6);
HI351_write_cmos_sensor(0x84, 0x76); //CTEM Thresold max
HI351_write_cmos_sensor(0x85, 0x62); //CTEM Threshold min
HI351_write_cmos_sensor(0x86, 0x08); //Y Thresold max
HI351_write_cmos_sensor(0x87, 0x00); //Y Thresold min
HI351_write_cmos_sensor(0x88, 0x00); //offset
HI351_write_cmos_sensor(0x89, 0x00);
HI351_write_cmos_sensor(0x8a, 0x00);
HI351_write_cmos_sensor(0x8b, 0x00);
HI351_write_cmos_sensor(0x8c, 0x80); //gain
HI351_write_cmos_sensor(0x8d, 0x84);	//Unform scene B gain
HI351_write_cmos_sensor(0x8e, 0x8f);	//Unform scene R gain
HI351_write_cmos_sensor(0x8f, 0x80);
HI351_write_cmos_sensor(0x90, 0x50);
HI351_write_cmos_sensor(0x91, 0x50);
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
/////////////////////////////////////
// D9 Page DMA EXTRA
/////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0xd9);
HI351_write_cmos_sensor(0x0e, 0x01); //BURST_START
HI351_write_cmos_sensor(0x10, 0x03); //Page 10
HI351_write_cmos_sensor(0x11, 0x10);
HI351_write_cmos_sensor(0x12, 0x61);
HI351_write_cmos_sensor(0x13, 0x88); //1061 : SATB
HI351_write_cmos_sensor(0x14, 0x62);
HI351_write_cmos_sensor(0x15, 0x88); //1062 : SATR
HI351_write_cmos_sensor(0x16, 0x40);
HI351_write_cmos_sensor(0x17, 0x00); //1040 : Brightness
HI351_write_cmos_sensor(0x18, 0x48);
HI351_write_cmos_sensor(0x19, 0x88); //1048 : Contrast
HI351_write_cmos_sensor(0x1a, 0x03); //Page 16
HI351_write_cmos_sensor(0x1b, 0x16);
HI351_write_cmos_sensor(0x1c, 0x30);
HI351_write_cmos_sensor(0x1d, 0x7f); //1630 : CMC
HI351_write_cmos_sensor(0x1e, 0x31);
HI351_write_cmos_sensor(0x1f, 0x42); //1631
HI351_write_cmos_sensor(0x20, 0x32);
HI351_write_cmos_sensor(0x21, 0x03); //1632
HI351_write_cmos_sensor(0x22, 0x33);
HI351_write_cmos_sensor(0x23, 0x22); //1633
HI351_write_cmos_sensor(0x24, 0x34);
HI351_write_cmos_sensor(0x25, 0x7b); //1634
HI351_write_cmos_sensor(0x26, 0x35);
HI351_write_cmos_sensor(0x27, 0x19); //1635
HI351_write_cmos_sensor(0x28, 0x36);
HI351_write_cmos_sensor(0x29, 0x01); //1636
HI351_write_cmos_sensor(0x2a, 0x37);
HI351_write_cmos_sensor(0x2b, 0x43); //1637
HI351_write_cmos_sensor(0x2c, 0x38);
HI351_write_cmos_sensor(0x2d, 0x84); //1638
HI351_write_cmos_sensor(0x2e, 0x70);
HI351_write_cmos_sensor(0x2f, 0x80); //1670 : MCMC
HI351_write_cmos_sensor(0x30, 0x71);
HI351_write_cmos_sensor(0x31, 0x00); //1671
HI351_write_cmos_sensor(0x32, 0x72);
HI351_write_cmos_sensor(0x33, 0x9b); //1672
HI351_write_cmos_sensor(0x34, 0x73);
HI351_write_cmos_sensor(0x35, 0x05); //1673
HI351_write_cmos_sensor(0x36, 0x74);
HI351_write_cmos_sensor(0x37, 0x34); //1674
HI351_write_cmos_sensor(0x38, 0x75);
HI351_write_cmos_sensor(0x39, 0x1e); //1675
HI351_write_cmos_sensor(0x3a, 0x76);
HI351_write_cmos_sensor(0x3b, 0xa6); //1676
HI351_write_cmos_sensor(0x3c, 0x77);
HI351_write_cmos_sensor(0x3d, 0x10); //1677
HI351_write_cmos_sensor(0x3e, 0x78);
HI351_write_cmos_sensor(0x3f, 0x69); //1678
HI351_write_cmos_sensor(0x40, 0x79);
HI351_write_cmos_sensor(0x41, 0x1e); //1679
HI351_write_cmos_sensor(0x42, 0x7a);
HI351_write_cmos_sensor(0x43, 0x80); //167a
HI351_write_cmos_sensor(0x44, 0x7b);
HI351_write_cmos_sensor(0x45, 0x80); //167b
HI351_write_cmos_sensor(0x46, 0x7c);
HI351_write_cmos_sensor(0x47, 0xad); //167c
HI351_write_cmos_sensor(0x48, 0x7d);
HI351_write_cmos_sensor(0x49, 0x1e); //167d
HI351_write_cmos_sensor(0x4a, 0x7e);
HI351_write_cmos_sensor(0x4b, 0x98); //167e
HI351_write_cmos_sensor(0x4c, 0x7f);
HI351_write_cmos_sensor(0x4d, 0x80); //167f
HI351_write_cmos_sensor(0x4e, 0x80);
HI351_write_cmos_sensor(0x4f, 0x51); //1680
HI351_write_cmos_sensor(0x50, 0x81);
HI351_write_cmos_sensor(0x51, 0x1e); //1681
HI351_write_cmos_sensor(0x52, 0x82);
HI351_write_cmos_sensor(0x53, 0x80); //1682
HI351_write_cmos_sensor(0x54, 0x83);
HI351_write_cmos_sensor(0x55, 0x0c); //1683
HI351_write_cmos_sensor(0x56, 0x84);
HI351_write_cmos_sensor(0x57, 0x23); //1684
HI351_write_cmos_sensor(0x58, 0x85);
HI351_write_cmos_sensor(0x59, 0x1e); //1685
HI351_write_cmos_sensor(0x5a, 0x86);
HI351_write_cmos_sensor(0x5b, 0xb3); //1686
HI351_write_cmos_sensor(0x5c, 0x87);
HI351_write_cmos_sensor(0x5d, 0x8a); //1687
HI351_write_cmos_sensor(0x5e, 0x88);
HI351_write_cmos_sensor(0x5f, 0x52); //1688
HI351_write_cmos_sensor(0x60, 0x89);
HI351_write_cmos_sensor(0x61, 0x1e); //1689
HI351_write_cmos_sensor(0x62, 0x03); //Page 17 Gamma
HI351_write_cmos_sensor(0x63, 0x17);
HI351_write_cmos_sensor(0x64, 0x20);
HI351_write_cmos_sensor(0x65, 0x00); //1720 : GMA
HI351_write_cmos_sensor(0x66, 0x21);
HI351_write_cmos_sensor(0x67, 0x02); //1721
HI351_write_cmos_sensor(0x68, 0x22);
HI351_write_cmos_sensor(0x69, 0x04); //1722
HI351_write_cmos_sensor(0x6a, 0x23);
HI351_write_cmos_sensor(0x6b, 0x09); //1723
HI351_write_cmos_sensor(0x6c, 0x24);
HI351_write_cmos_sensor(0x6d, 0x12); //1724
HI351_write_cmos_sensor(0x6e, 0x25);
HI351_write_cmos_sensor(0x6f, 0x23); //1725
HI351_write_cmos_sensor(0x70, 0x26);
HI351_write_cmos_sensor(0x71, 0x37); //1726
HI351_write_cmos_sensor(0x72, 0x27);
HI351_write_cmos_sensor(0x73, 0x47); //1727
HI351_write_cmos_sensor(0x74, 0x28);
HI351_write_cmos_sensor(0x75, 0x57); //1728
HI351_write_cmos_sensor(0x76, 0x29);
HI351_write_cmos_sensor(0x77, 0x61); //1729
HI351_write_cmos_sensor(0x78, 0x2a);
HI351_write_cmos_sensor(0x79, 0x6b); //172a
HI351_write_cmos_sensor(0x7a, 0x2b);
HI351_write_cmos_sensor(0x7b, 0x71); //172b
HI351_write_cmos_sensor(0x7c, 0x2c);
HI351_write_cmos_sensor(0x7d, 0x76); //172c
HI351_write_cmos_sensor(0x7e, 0x2d);
HI351_write_cmos_sensor(0x7f, 0x7a); //172d
HI351_write_cmos_sensor(0x80, 0x2e);
HI351_write_cmos_sensor(0x81, 0x7f); //172e
HI351_write_cmos_sensor(0x82, 0x2f);
HI351_write_cmos_sensor(0x83, 0x84); //172f
HI351_write_cmos_sensor(0x84, 0x30);
HI351_write_cmos_sensor(0x85, 0x88); //1730
HI351_write_cmos_sensor(0x86, 0x31);
HI351_write_cmos_sensor(0x87, 0x8c); //1731
HI351_write_cmos_sensor(0x88, 0x32);
HI351_write_cmos_sensor(0x89, 0x91); //1732
HI351_write_cmos_sensor(0x8a, 0x33);
HI351_write_cmos_sensor(0x8b, 0x94); //1733
HI351_write_cmos_sensor(0x8c, 0x34);
HI351_write_cmos_sensor(0x8d, 0x98); //1734
HI351_write_cmos_sensor(0x8e, 0x35);
HI351_write_cmos_sensor(0x8f, 0x9f); //1735
HI351_write_cmos_sensor(0x90, 0x36);
HI351_write_cmos_sensor(0x91, 0xa6); //1736
HI351_write_cmos_sensor(0x92, 0x37);
HI351_write_cmos_sensor(0x93, 0xae); //1737
HI351_write_cmos_sensor(0x94, 0x38);
HI351_write_cmos_sensor(0x95, 0xbb); //1738
HI351_write_cmos_sensor(0x96, 0x39);
HI351_write_cmos_sensor(0x97, 0xc9); //1739
HI351_write_cmos_sensor(0x98, 0x3a);
HI351_write_cmos_sensor(0x99, 0xd3); //173a
HI351_write_cmos_sensor(0x9a, 0x3b);
HI351_write_cmos_sensor(0x9b, 0xdc); //173b
HI351_write_cmos_sensor(0x9c, 0x3c);
HI351_write_cmos_sensor(0x9d, 0xe2); //173c
HI351_write_cmos_sensor(0x9e, 0x3d);
HI351_write_cmos_sensor(0x9f, 0xe8); //173d
HI351_write_cmos_sensor(0xa0, 0x3e);
HI351_write_cmos_sensor(0xa1, 0xed); //173e
HI351_write_cmos_sensor(0xa2, 0x3f);
HI351_write_cmos_sensor(0xa3, 0xf4); //173f
HI351_write_cmos_sensor(0xa4, 0x40);
HI351_write_cmos_sensor(0xa5, 0xfa); //1740
HI351_write_cmos_sensor(0xa6, 0x41);
HI351_write_cmos_sensor(0xa7, 0xff); //1741
HI351_write_cmos_sensor(0xa8, 0x03); //page 20 AE
HI351_write_cmos_sensor(0xa9, 0x20);
HI351_write_cmos_sensor(0xaa, 0x39);
HI351_write_cmos_sensor(0xab, 0x40); //2039 : Y target
HI351_write_cmos_sensor(0xac, 0x03); //Page 15 SHD
HI351_write_cmos_sensor(0xad, 0x15);
HI351_write_cmos_sensor(0xae, 0x24);
HI351_write_cmos_sensor(0xaf, 0x00); //1524 : Shading
HI351_write_cmos_sensor(0xb0, 0x25);
HI351_write_cmos_sensor(0xb1, 0x00); //1525
HI351_write_cmos_sensor(0xb2, 0x26);
HI351_write_cmos_sensor(0xb3, 0x00); //1526
HI351_write_cmos_sensor(0xb4, 0x27);
HI351_write_cmos_sensor(0xb5, 0x00); //1527
HI351_write_cmos_sensor(0xb6, 0x28);
HI351_write_cmos_sensor(0xb7, 0x80); //1528
HI351_write_cmos_sensor(0xb8, 0x29);
HI351_write_cmos_sensor(0xb9, 0x80); //1529
HI351_write_cmos_sensor(0xba, 0x2a);
HI351_write_cmos_sensor(0xbb, 0x80); //152a
HI351_write_cmos_sensor(0xbc, 0x2b);
HI351_write_cmos_sensor(0xbd, 0x80); //152b
HI351_write_cmos_sensor(0xbe, 0x11);
HI351_write_cmos_sensor(0xbf, 0x40); //1511
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
/////////////////////////////////////
// DA Page (DMA Outdoor)
/////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0xda);
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x03);
HI351_write_cmos_sensor(0x11, 0x11); //11 page
HI351_write_cmos_sensor(0x12, 0x10);
HI351_write_cmos_sensor(0x13, 0x13); //Outdoor 1110
HI351_write_cmos_sensor(0x14, 0x11);
HI351_write_cmos_sensor(0x15, 0x25); //Outdoor 1111
HI351_write_cmos_sensor(0x16, 0x12);
HI351_write_cmos_sensor(0x17, 0x22); //Outdoor 1112
HI351_write_cmos_sensor(0x18, 0x13);
HI351_write_cmos_sensor(0x19, 0x11); //Outdoor 1113
HI351_write_cmos_sensor(0x1a, 0x14);
HI351_write_cmos_sensor(0x1b, 0x21); //Outdoor 1114
HI351_write_cmos_sensor(0x1c, 0x30);
HI351_write_cmos_sensor(0x1d, 0x20); //Outdoor 1130
HI351_write_cmos_sensor(0x1e, 0x31);
HI351_write_cmos_sensor(0x1f, 0x20); //Outdoor 1131
HI351_write_cmos_sensor(0x20, 0x32);
HI351_write_cmos_sensor(0x21, 0x70); //Outdoor 1132
HI351_write_cmos_sensor(0x22, 0x33);
HI351_write_cmos_sensor(0x23, 0x4a); //Outdoor 1133
HI351_write_cmos_sensor(0x24, 0x34);
HI351_write_cmos_sensor(0x25, 0x2a); //Outdoor 1134
HI351_write_cmos_sensor(0x26, 0x35);
HI351_write_cmos_sensor(0x27, 0x2b); //Outdoor 1135
HI351_write_cmos_sensor(0x28, 0x36);
HI351_write_cmos_sensor(0x29, 0x16); //Outdoor 1136
HI351_write_cmos_sensor(0x2a, 0x37);
HI351_write_cmos_sensor(0x2b, 0x1b); //Outdoor 1137
HI351_write_cmos_sensor(0x2c, 0x38);
HI351_write_cmos_sensor(0x2d, 0x14); //Outdoor 1138
HI351_write_cmos_sensor(0x2e, 0x39);
HI351_write_cmos_sensor(0x2f, 0x10); //Outdoor 1139 : R2 Lum Lv 01 NR Gain
HI351_write_cmos_sensor(0x30, 0x3a);
HI351_write_cmos_sensor(0x31, 0x10); //Outdoor 113a : R2 Lum Lv 02 NR Gain
HI351_write_cmos_sensor(0x32, 0x3b);
HI351_write_cmos_sensor(0x33, 0x2c); //Outdoor 113b : R2 Lum Lv 03 NR Gain
HI351_write_cmos_sensor(0x34, 0x3c);
HI351_write_cmos_sensor(0x35, 0x60); //Outdoor 113c : R2 Lum Lv 04 NR Gain
HI351_write_cmos_sensor(0x36, 0x3d);
HI351_write_cmos_sensor(0x37, 0x38); //Outdoor 113d : R2 Lum Lv 05 NR Gain
HI351_write_cmos_sensor(0x38, 0x3e);
HI351_write_cmos_sensor(0x39, 0x68); //Outdoor 113e : R2 Lum Lv 06 NR Gain
HI351_write_cmos_sensor(0x3a, 0x3f);
HI351_write_cmos_sensor(0x3b, 0x68); //Outdoor 113f : R2 Lum Lv 07 NR Gain
HI351_write_cmos_sensor(0x3c, 0x40);
HI351_write_cmos_sensor(0x3d, 0x68); //Outdoor 1140 : R2 Lum Lv 08 NR Gain
HI351_write_cmos_sensor(0x3e, 0x41);
HI351_write_cmos_sensor(0x3f, 0x1a); //Outdoor 1141 : R2 Lum Lv 01 NR offset
HI351_write_cmos_sensor(0x40, 0x42);
HI351_write_cmos_sensor(0x41, 0x1a); //Outdoor 1142 : R2 Lum Lv 02 NR offset
HI351_write_cmos_sensor(0x42, 0x43);
HI351_write_cmos_sensor(0x43, 0x1a); //Outdoor 1143 : R2 Lum Lv 03 NR offset
HI351_write_cmos_sensor(0x44, 0x44);
HI351_write_cmos_sensor(0x45, 0x30); //Outdoor 1144 : R2 Lum Lv 04 NR offset
HI351_write_cmos_sensor(0x46, 0x45);
HI351_write_cmos_sensor(0x47, 0x30); //Outdoor 1145 : R2 Lum Lv 05 NR offset
HI351_write_cmos_sensor(0x48, 0x46);
HI351_write_cmos_sensor(0x49, 0x40); //Outdoor 1146 : R2 Lum Lv 06 NR offset
HI351_write_cmos_sensor(0x4a, 0x47);
HI351_write_cmos_sensor(0x4b, 0x40); //Outdoor 1147 : R2 Lum Lv 07 NR offset
HI351_write_cmos_sensor(0x4c, 0x48);
HI351_write_cmos_sensor(0x4d, 0x40); //Outdoor 1148 : R2 Lum Lv 08 NR offset
HI351_write_cmos_sensor(0x4e, 0x49);
HI351_write_cmos_sensor(0x4f, 0xf0); //Outdoor 1149
HI351_write_cmos_sensor(0x50, 0x4a);
HI351_write_cmos_sensor(0x51, 0xf0); //Outdoor 114a
HI351_write_cmos_sensor(0x52, 0x4b);
HI351_write_cmos_sensor(0x53, 0xf0); //Outdoor 114b
HI351_write_cmos_sensor(0x54, 0x4c);
HI351_write_cmos_sensor(0x55, 0xf0); //Outdoor 114c
HI351_write_cmos_sensor(0x56, 0x4d);
HI351_write_cmos_sensor(0x57, 0xf0); //Outdoor 114d
HI351_write_cmos_sensor(0x58, 0x4e);
HI351_write_cmos_sensor(0x59, 0xf0); //Outdoor 114e
HI351_write_cmos_sensor(0x5a, 0x4f);
HI351_write_cmos_sensor(0x5b, 0xf0); //Outdoor 114f
HI351_write_cmos_sensor(0x5c, 0x50);
HI351_write_cmos_sensor(0x5d, 0xf0); //Outdoor 1150
HI351_write_cmos_sensor(0x5e, 0x51);
HI351_write_cmos_sensor(0x5f, 0x60); //Outdoor 1151
HI351_write_cmos_sensor(0x60, 0x52);
HI351_write_cmos_sensor(0x61, 0x60); //Outdoor 1152
HI351_write_cmos_sensor(0x62, 0x53);
HI351_write_cmos_sensor(0x63, 0x60); //Outdoor 1153
HI351_write_cmos_sensor(0x64, 0x54);
HI351_write_cmos_sensor(0x65, 0x60); //Outdoor 1154
HI351_write_cmos_sensor(0x66, 0x55);
HI351_write_cmos_sensor(0x67, 0x60); //Outdoor 1155
HI351_write_cmos_sensor(0x68, 0x56);
HI351_write_cmos_sensor(0x69, 0x60); //Outdoor 1156
HI351_write_cmos_sensor(0x6a, 0x57);
HI351_write_cmos_sensor(0x6b, 0x60); //Outdoor 1157
HI351_write_cmos_sensor(0x6c, 0x58);
HI351_write_cmos_sensor(0x6d, 0x60); //Outdoor 1158
HI351_write_cmos_sensor(0x6e, 0x59);
HI351_write_cmos_sensor(0x6f, 0x80); //Outdoor 1159
HI351_write_cmos_sensor(0x70, 0x5a);
HI351_write_cmos_sensor(0x71, 0x80); //Outdoor 115a
HI351_write_cmos_sensor(0x72, 0x5b);
HI351_write_cmos_sensor(0x73, 0x80); //Outdoor 115b
HI351_write_cmos_sensor(0x74, 0x5c);
HI351_write_cmos_sensor(0x75, 0x80); //Outdoor 115c
HI351_write_cmos_sensor(0x76, 0x5d);
HI351_write_cmos_sensor(0x77, 0x80); //Outdoor 115d
HI351_write_cmos_sensor(0x78, 0x5e);
HI351_write_cmos_sensor(0x79, 0x80); //Outdoor 115e
HI351_write_cmos_sensor(0x7a, 0x5f);
HI351_write_cmos_sensor(0x7b, 0x80); //Outdoor 115f
HI351_write_cmos_sensor(0x7c, 0x60);
HI351_write_cmos_sensor(0x7d, 0x80); //Outdoor 1160
HI351_write_cmos_sensor(0x7e, 0x61);
HI351_write_cmos_sensor(0x7f, 0xf0); //Outdoor 1161
HI351_write_cmos_sensor(0x80, 0x62);
HI351_write_cmos_sensor(0x81, 0xfc); //Outdoor 1162
HI351_write_cmos_sensor(0x82, 0x63);
HI351_write_cmos_sensor(0x83, 0x60); //Outdoor 1163
HI351_write_cmos_sensor(0x84, 0x64);
HI351_write_cmos_sensor(0x85, 0x20); //Outdoor 1164
HI351_write_cmos_sensor(0x86, 0x65);
HI351_write_cmos_sensor(0x87, 0x24); //Outdoor 1165 Imp Lv1 Hi Gain
HI351_write_cmos_sensor(0x88, 0x66);
HI351_write_cmos_sensor(0x89, 0x24); //Outdoor 1166 Imp Lv1 Middle Gain
HI351_write_cmos_sensor(0x8a, 0x67);
HI351_write_cmos_sensor(0x8b, 0x1a); //Outdoor 1167 Imp Lv1 Low Gain
HI351_write_cmos_sensor(0x8c, 0x68);
HI351_write_cmos_sensor(0x8d, 0x5a); //Outdoor 1168
HI351_write_cmos_sensor(0x8e, 0x69);
HI351_write_cmos_sensor(0x8f, 0x24); //Outdoor 1169
HI351_write_cmos_sensor(0x90, 0x6a);
HI351_write_cmos_sensor(0x91, 0x24); //Outdoor 116a Imp Lv2 Hi Gain
HI351_write_cmos_sensor(0x92, 0x6b);
HI351_write_cmos_sensor(0x93, 0x24); //Outdoor 116b Imp Lv2 Middle Gain
HI351_write_cmos_sensor(0x94, 0x6c);
HI351_write_cmos_sensor(0x95, 0x1a); //Outdoor 116c Imp Lv2 Low Gain
HI351_write_cmos_sensor(0x96, 0x6d);
HI351_write_cmos_sensor(0x97, 0x5c); //Outdoor 116d
HI351_write_cmos_sensor(0x98, 0x6e);
HI351_write_cmos_sensor(0x99, 0x20); //Outdoor 116e
HI351_write_cmos_sensor(0x9a, 0x6f);
HI351_write_cmos_sensor(0x9b, 0x26); //Outdoor 116f Imp Lv3 Hi Gain
HI351_write_cmos_sensor(0x9c, 0x70);
HI351_write_cmos_sensor(0x9d, 0x26); //Outdoor 1170 Imp Lv3 Middle Gain
HI351_write_cmos_sensor(0x9e, 0x71);
HI351_write_cmos_sensor(0x9f, 0x30); //Outdoor 1171 Imp Lv3 Low Gain
HI351_write_cmos_sensor(0xa0, 0x72);
HI351_write_cmos_sensor(0xa1, 0x5c); //Outdoor 1172
HI351_write_cmos_sensor(0xa2, 0x73);
HI351_write_cmos_sensor(0xa3, 0x20); //Outdoor 1173
HI351_write_cmos_sensor(0xa4, 0x74);
HI351_write_cmos_sensor(0xa5, 0x90); //Outdoor 1174 Imp Lv4 Hi Gain
HI351_write_cmos_sensor(0xa6, 0x75);
HI351_write_cmos_sensor(0xa7, 0x90); //Outdoor 1175 Imp Lv4 Middle Gain
HI351_write_cmos_sensor(0xa8, 0x76);
HI351_write_cmos_sensor(0xa9, 0x80); //Outdoor 1176 Imp Lv4 Low Gain
HI351_write_cmos_sensor(0xaa, 0x77);
HI351_write_cmos_sensor(0xab, 0x40); //Outdoor 1177
HI351_write_cmos_sensor(0xac, 0x78);
HI351_write_cmos_sensor(0xad, 0x26); //Outdoor 1178
HI351_write_cmos_sensor(0xae, 0x79);
HI351_write_cmos_sensor(0xaf, 0x90); //Outdoor 1179 Imp Lv5 Hi Gain
HI351_write_cmos_sensor(0xb0, 0x7a);
HI351_write_cmos_sensor(0xb1, 0x90); //Outdoor 117a Imp Lv5 Middle Gain
HI351_write_cmos_sensor(0xb2, 0x7b);
HI351_write_cmos_sensor(0xb3, 0x80); //Outdoor 117b Imp Lv5 Low Gain
HI351_write_cmos_sensor(0xb4, 0x7c);
HI351_write_cmos_sensor(0xb5, 0x38); //Outdoor 117c
HI351_write_cmos_sensor(0xb6, 0x7d);
HI351_write_cmos_sensor(0xb7, 0x1c); //Outdoor 117d
HI351_write_cmos_sensor(0xb8, 0x7e);
HI351_write_cmos_sensor(0xb9, 0x68); //Outdoor 117e Imp Lv6 Hi Gain
HI351_write_cmos_sensor(0xba, 0x7f);
HI351_write_cmos_sensor(0xbb, 0x58); //Outdoor 117f Imp Lv6 Middle Gain
HI351_write_cmos_sensor(0xbc, 0x80);
HI351_write_cmos_sensor(0xbd, 0x48); //Outdoor 1180 Imp Lv6 Low Gain
HI351_write_cmos_sensor(0xbe, 0x81);
HI351_write_cmos_sensor(0xbf, 0x32); //Outdoor 1181
HI351_write_cmos_sensor(0xc0, 0x82);
HI351_write_cmos_sensor(0xc1, 0x10); //Outdoor 1182
HI351_write_cmos_sensor(0xc2, 0x83);
HI351_write_cmos_sensor(0xc3, 0x68); //Outdoor 1183 Imp Lv7 Hi Gain
HI351_write_cmos_sensor(0xc4, 0x84);
HI351_write_cmos_sensor(0xc5, 0x58); //Outdoor 1184 Imp Lv7 Middle Gain
HI351_write_cmos_sensor(0xc6, 0x85);
HI351_write_cmos_sensor(0xc7, 0x48); //Outdoor 1185 Imp Lv7 Low Gain
HI351_write_cmos_sensor(0xc8, 0x86);
HI351_write_cmos_sensor(0xc9, 0x1c); //Outdoor 1186
HI351_write_cmos_sensor(0xca, 0x87);
HI351_write_cmos_sensor(0xcb, 0x08); //Outdoor 1187
HI351_write_cmos_sensor(0xcc, 0x88);
HI351_write_cmos_sensor(0xcd, 0x64); //Outdoor 1188
HI351_write_cmos_sensor(0xce, 0x89);
HI351_write_cmos_sensor(0xcf, 0x64); //Outdoor 1189
HI351_write_cmos_sensor(0xd0, 0x8a);
HI351_write_cmos_sensor(0xd1, 0x48); //Outdoor 118a
HI351_write_cmos_sensor(0xd2, 0x90);
HI351_write_cmos_sensor(0xd3, 0x02); //Outdoor 1190
HI351_write_cmos_sensor(0xd4, 0x91);
HI351_write_cmos_sensor(0xd5, 0x48); //Outdoor 1191
HI351_write_cmos_sensor(0xd6, 0x92);
HI351_write_cmos_sensor(0xd7, 0x00); //Outdoor 1192
HI351_write_cmos_sensor(0xd8, 0x93);
HI351_write_cmos_sensor(0xd9, 0x04); //Outdoor 1193
HI351_write_cmos_sensor(0xda, 0x94);
HI351_write_cmos_sensor(0xdb, 0x02); //Outdoor 1194
HI351_write_cmos_sensor(0xdc, 0x95);
HI351_write_cmos_sensor(0xdd, 0x64); //Outdoor 1195
HI351_write_cmos_sensor(0xde, 0x96);
HI351_write_cmos_sensor(0xdf, 0x14); //Outdoor 1196
HI351_write_cmos_sensor(0xe0, 0x97);
HI351_write_cmos_sensor(0xe1, 0x90); //Outdoor 1197
HI351_write_cmos_sensor(0xe2, 0xb0);
HI351_write_cmos_sensor(0xe3, 0x60); //Outdoor 11b0
HI351_write_cmos_sensor(0xe4, 0xb1);
HI351_write_cmos_sensor(0xe5, 0x90); //Outdoor 11b1
HI351_write_cmos_sensor(0xe6, 0xb2);
HI351_write_cmos_sensor(0xe7, 0x10); //Outdoor 11b2
HI351_write_cmos_sensor(0xe8, 0xb3);
HI351_write_cmos_sensor(0xe9, 0x08); //Outdoor 11b3
HI351_write_cmos_sensor(0xea, 0xb4);
HI351_write_cmos_sensor(0xeb, 0x04); //Outdoor 11b4
HI351_write_cmos_sensor(0xec, 0x03);
HI351_write_cmos_sensor(0xed, 0x12);
HI351_write_cmos_sensor(0xee, 0x10);
HI351_write_cmos_sensor(0xef, 0x03); //Outdoor 1210
HI351_write_cmos_sensor(0xf0, 0x11);
HI351_write_cmos_sensor(0xf1, 0x29); //Outdoor 1211
HI351_write_cmos_sensor(0xf2, 0x12);
HI351_write_cmos_sensor(0xf3, 0x30); //Outdoor 1212
HI351_write_cmos_sensor(0xf4, 0x40);
HI351_write_cmos_sensor(0xf5, 0x37); //Outdoor 1240
HI351_write_cmos_sensor(0xf6, 0x41);
HI351_write_cmos_sensor(0xf7, 0x24); //Outdoor 1241
HI351_write_cmos_sensor(0xf8, 0x42);
HI351_write_cmos_sensor(0xf9, 0x00); //Outdoor 1242
HI351_write_cmos_sensor(0xfa, 0x43);
HI351_write_cmos_sensor(0xfb, 0x62); //Outdoor 1243
HI351_write_cmos_sensor(0xfc, 0x44);
HI351_write_cmos_sensor(0xfd, 0x02); //Outdoor 1244
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
HI351_write_cmos_sensor(0x03, 0xdb);
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x45);
HI351_write_cmos_sensor(0x11, 0x0a); //Outdoor 1245
HI351_write_cmos_sensor(0x12, 0x46);
HI351_write_cmos_sensor(0x13, 0x40); //Outdoor 1246
HI351_write_cmos_sensor(0x14, 0x60);
HI351_write_cmos_sensor(0x15, 0x02); //Outdoor 1260
HI351_write_cmos_sensor(0x16, 0x61);
HI351_write_cmos_sensor(0x17, 0x04); //Outdoor 1261
HI351_write_cmos_sensor(0x18, 0x62);
HI351_write_cmos_sensor(0x19, 0x4b); //Outdoor 1262
HI351_write_cmos_sensor(0x1a, 0x63);
HI351_write_cmos_sensor(0x1b, 0x41); //Outdoor 1263
HI351_write_cmos_sensor(0x1c, 0x64);
HI351_write_cmos_sensor(0x1d, 0x14); //Outdoor 1264
HI351_write_cmos_sensor(0x1e, 0x65);
HI351_write_cmos_sensor(0x1f, 0x00); //Outdoor 1265
HI351_write_cmos_sensor(0x20, 0x68);
HI351_write_cmos_sensor(0x21, 0x0a); //Outdoor 1268
HI351_write_cmos_sensor(0x22, 0x69);
HI351_write_cmos_sensor(0x23, 0x04); //Outdoor 1269
HI351_write_cmos_sensor(0x24, 0x6a);
HI351_write_cmos_sensor(0x25, 0x0a); //Outdoor 126a
HI351_write_cmos_sensor(0x26, 0x6b);
HI351_write_cmos_sensor(0x27, 0x0a); //Outdoor 126b
HI351_write_cmos_sensor(0x28, 0x6c);
HI351_write_cmos_sensor(0x29, 0x24); //Outdoor 126c
HI351_write_cmos_sensor(0x2a, 0x6d);
HI351_write_cmos_sensor(0x2b, 0x01); //Outdoor 126d
HI351_write_cmos_sensor(0x2c, 0x70);
HI351_write_cmos_sensor(0x2d, 0x01); //Outdoor 1270
HI351_write_cmos_sensor(0x2e, 0x71);
HI351_write_cmos_sensor(0x2f, 0x3d); //Outdoor 1271
HI351_write_cmos_sensor(0x30, 0x80);
HI351_write_cmos_sensor(0x31, 0x80); //Outdoor 1280
HI351_write_cmos_sensor(0x32, 0x81);
HI351_write_cmos_sensor(0x33, 0x8a); //Outdoor 1281
HI351_write_cmos_sensor(0x34, 0x82);
HI351_write_cmos_sensor(0x35, 0x0a); //Outdoor 1282
HI351_write_cmos_sensor(0x36, 0x83);
HI351_write_cmos_sensor(0x37, 0x12); //Outdoor 1283
HI351_write_cmos_sensor(0x38, 0x84);
HI351_write_cmos_sensor(0x39, 0xc8); //Outdoor 1284
HI351_write_cmos_sensor(0x3a, 0x85);
HI351_write_cmos_sensor(0x3b, 0x92); //Outdoor 1285
HI351_write_cmos_sensor(0x3c, 0x86);
HI351_write_cmos_sensor(0x3d, 0x20); //Outdoor 1286
HI351_write_cmos_sensor(0x3e, 0x87);
HI351_write_cmos_sensor(0x3f, 0x00); //Outdoor 1287
HI351_write_cmos_sensor(0x40, 0x88);
HI351_write_cmos_sensor(0x41, 0x70); //Outdoor 1288 lmt center
HI351_write_cmos_sensor(0x42, 0x89);
HI351_write_cmos_sensor(0x43, 0xaa); //Outdoor 1289
HI351_write_cmos_sensor(0x44, 0x8a);
HI351_write_cmos_sensor(0x45, 0x50); //Outdoor 128a
HI351_write_cmos_sensor(0x46, 0x8b);
HI351_write_cmos_sensor(0x47, 0x10); //Outdoor 128b
HI351_write_cmos_sensor(0x48, 0x8c);
HI351_write_cmos_sensor(0x49, 0x04); //Outdoor 128c
HI351_write_cmos_sensor(0x4a, 0x8d);
HI351_write_cmos_sensor(0x4b, 0x02); //Outdoor 128d
HI351_write_cmos_sensor(0x4c, 0xe6);
HI351_write_cmos_sensor(0x4d, 0xff); //Outdoor 12e6
HI351_write_cmos_sensor(0x4e, 0xe7);
HI351_write_cmos_sensor(0x4f, 0x18); //Outdoor 12e7
HI351_write_cmos_sensor(0x50, 0xe8);
HI351_write_cmos_sensor(0x51, 0x20); //Outdoor 12e8
HI351_write_cmos_sensor(0x52, 0xe9);
HI351_write_cmos_sensor(0x53, 0x04); //Outdoor 12e9
HI351_write_cmos_sensor(0x54, 0x03);
HI351_write_cmos_sensor(0x55, 0x13);
HI351_write_cmos_sensor(0x56, 0x10);
HI351_write_cmos_sensor(0x57, 0x3f); //Outdoor 1310 Skin Color On
HI351_write_cmos_sensor(0x58, 0x20);
HI351_write_cmos_sensor(0x59, 0x20); //Outdoor 1320
HI351_write_cmos_sensor(0x5a, 0x21);
HI351_write_cmos_sensor(0x5b, 0x30); //Outdoor 1321
HI351_write_cmos_sensor(0x5c, 0x22);
HI351_write_cmos_sensor(0x5d, 0x36); //Outdoor 1322
HI351_write_cmos_sensor(0x5e, 0x23);
HI351_write_cmos_sensor(0x5f, 0x6a); //Outdoor 1323
HI351_write_cmos_sensor(0x60, 0x24);
HI351_write_cmos_sensor(0x61, 0xa0); //Outdoor 1324
HI351_write_cmos_sensor(0x62, 0x25);
HI351_write_cmos_sensor(0x63, 0xc0); //Outdoor 1325
HI351_write_cmos_sensor(0x64, 0x26);
HI351_write_cmos_sensor(0x65, 0xe0); //Outdoor 1326
HI351_write_cmos_sensor(0x66, 0x27);
HI351_write_cmos_sensor(0x67, 0x0a); //Outdoor 1327
HI351_write_cmos_sensor(0x68, 0x28);
HI351_write_cmos_sensor(0x69, 0x0a); //Outdoor 1328
HI351_write_cmos_sensor(0x6a, 0x29);
HI351_write_cmos_sensor(0x6b, 0x0a); //Outdoor 1329
HI351_write_cmos_sensor(0x6c, 0x2a);
HI351_write_cmos_sensor(0x6d, 0x08); //Outdoor 132a
HI351_write_cmos_sensor(0x6e, 0x2b);
HI351_write_cmos_sensor(0x6f, 0x06); //Outdoor 132b
HI351_write_cmos_sensor(0x70, 0x2c);
HI351_write_cmos_sensor(0x71, 0x05); //Outdoor 132c
HI351_write_cmos_sensor(0x72, 0x2d);
HI351_write_cmos_sensor(0x73, 0x04); //Outdoor 132d
HI351_write_cmos_sensor(0x74, 0x2e);
HI351_write_cmos_sensor(0x75, 0x03); //Outdoor 132e
HI351_write_cmos_sensor(0x76, 0x2f);
HI351_write_cmos_sensor(0x77, 0x01); //Outdoor 132f weight skin
HI351_write_cmos_sensor(0x78, 0x30);
HI351_write_cmos_sensor(0x79, 0x02); //Outdoor 1330 weight blue
HI351_write_cmos_sensor(0x7a, 0x31);
HI351_write_cmos_sensor(0x7b, 0x78); //Outdoor 1331 weight green
HI351_write_cmos_sensor(0x7c, 0x32);
HI351_write_cmos_sensor(0x7d, 0x01); //Outdoor 1332 weight strong color
HI351_write_cmos_sensor(0x7e, 0x33);
HI351_write_cmos_sensor(0x7f, 0x80); //Outdoor 1333
HI351_write_cmos_sensor(0x80, 0x34);
HI351_write_cmos_sensor(0x81, 0xf0); //Outdoor 1334
HI351_write_cmos_sensor(0x82, 0x35);
HI351_write_cmos_sensor(0x83, 0x10); //Outdoor 1335
HI351_write_cmos_sensor(0x84, 0x36);
HI351_write_cmos_sensor(0x85, 0xf0); //Outdoor 1336
HI351_write_cmos_sensor(0x86, 0xa0);
HI351_write_cmos_sensor(0x87, 0x07); //Outdoor 13a0
HI351_write_cmos_sensor(0x88, 0xa8);
HI351_write_cmos_sensor(0x89, 0x24); //Outdoor 13a8 Outdoor Cb-filter
HI351_write_cmos_sensor(0x8a, 0xa9);
HI351_write_cmos_sensor(0x8b, 0x24); //Outdoor 13a9 Outdoor Cr-filter
HI351_write_cmos_sensor(0x8c, 0xaa);
HI351_write_cmos_sensor(0x8d, 0x20); //Outdoor 13aa
HI351_write_cmos_sensor(0x8e, 0xab);
HI351_write_cmos_sensor(0x8f, 0x02); //Outdoor 13ab
HI351_write_cmos_sensor(0x90, 0xc0);
HI351_write_cmos_sensor(0x91, 0x27); //Outdoor 13c0
HI351_write_cmos_sensor(0x92, 0xc2);
HI351_write_cmos_sensor(0x93, 0x08); //Outdoor 13c2
HI351_write_cmos_sensor(0x94, 0xc3);
HI351_write_cmos_sensor(0x95, 0x08); //Outdoor 13c3
HI351_write_cmos_sensor(0x96, 0xc4);
HI351_write_cmos_sensor(0x97, 0x40); //Outdoor 13c4 Green Detect
HI351_write_cmos_sensor(0x98, 0xc5);
HI351_write_cmos_sensor(0x99, 0x78); //Outdoor 13c5
HI351_write_cmos_sensor(0x9a, 0xc6);
HI351_write_cmos_sensor(0x9b, 0xf0); //Outdoor 13c6
HI351_write_cmos_sensor(0x9c, 0xc7);
HI351_write_cmos_sensor(0x9d, 0x10); //Outdoor 13c7
HI351_write_cmos_sensor(0x9e, 0xc8);
HI351_write_cmos_sensor(0x9f, 0x44); //Outdoor 13c8
HI351_write_cmos_sensor(0xa0, 0xc9);
HI351_write_cmos_sensor(0xa1, 0x87); //Outdoor 13c9
HI351_write_cmos_sensor(0xa2, 0xca);
HI351_write_cmos_sensor(0xa3, 0xff); //Outdoor 13ca
HI351_write_cmos_sensor(0xa4, 0xcb);
HI351_write_cmos_sensor(0xa5, 0x20); //Outdoor 13cb
HI351_write_cmos_sensor(0xa6, 0xcc);
HI351_write_cmos_sensor(0xa7, 0x61); //Outdoor 13cc skin range_cb_l
HI351_write_cmos_sensor(0xa8, 0xcd);
HI351_write_cmos_sensor(0xa9, 0x87); //Outdoor 13cd skin range_cb_h
HI351_write_cmos_sensor(0xaa, 0xce);
HI351_write_cmos_sensor(0xab, 0x8a); //Outdoor 13ce skin range_cr_l
HI351_write_cmos_sensor(0xac, 0xcf);
HI351_write_cmos_sensor(0xad, 0xa5); //Outdoor 13cf skin range_cr_h
HI351_write_cmos_sensor(0xae, 0x03);
HI351_write_cmos_sensor(0xaf, 0x14);
HI351_write_cmos_sensor(0xb0, 0x11);
HI351_write_cmos_sensor(0xb1, 0x02); //Outdoor 1410
HI351_write_cmos_sensor(0xb2, 0x11);
HI351_write_cmos_sensor(0xb3, 0x02); //Outdoor 1411
HI351_write_cmos_sensor(0xb4, 0x12);
HI351_write_cmos_sensor(0xb5, 0x60); //Outdoor 1412 Top H_Clip
HI351_write_cmos_sensor(0xb6, 0x13);
HI351_write_cmos_sensor(0xb7, 0x62); //Outdoor 1413
HI351_write_cmos_sensor(0xb8, 0x14);
HI351_write_cmos_sensor(0xb9, 0x20); //Outdoor 1414
HI351_write_cmos_sensor(0xba, 0x15);
HI351_write_cmos_sensor(0xbb, 0x30); //Outdoor 1415 sharp positive ya
HI351_write_cmos_sensor(0xbc, 0x16);
HI351_write_cmos_sensor(0xbd, 0x30); //Outdoor 1416 sharp positive mi
HI351_write_cmos_sensor(0xbe, 0x17);
HI351_write_cmos_sensor(0xbf, 0x30); //Outdoor 1417 sharp positive low
HI351_write_cmos_sensor(0xc0, 0x18);
HI351_write_cmos_sensor(0xc1, 0x54); //Outdoor 1418 sharp negative ya
HI351_write_cmos_sensor(0xc2, 0x19);
HI351_write_cmos_sensor(0xc3, 0x54); //Outdoor 1419 sharp negative mi
HI351_write_cmos_sensor(0xc4, 0x1a);
HI351_write_cmos_sensor(0xc5, 0x40); //Outdoor 141a sharp negative low
HI351_write_cmos_sensor(0xc6, 0x20);
HI351_write_cmos_sensor(0xc7, 0xd2); //Outdoor 1420
HI351_write_cmos_sensor(0xc8, 0x21);
HI351_write_cmos_sensor(0xc9, 0x03); //Outdoor 1421
HI351_write_cmos_sensor(0xca, 0x22);
HI351_write_cmos_sensor(0xcb, 0x04); //Outdoor 1422
HI351_write_cmos_sensor(0xcc, 0x23);
HI351_write_cmos_sensor(0xcd, 0x04); //Outdoor 1423
HI351_write_cmos_sensor(0xce, 0x24);
HI351_write_cmos_sensor(0xcf, 0x05); //Outdoor 1424
HI351_write_cmos_sensor(0xd0, 0x25);
HI351_write_cmos_sensor(0xd1, 0x32); //Outdoor 1425
HI351_write_cmos_sensor(0xd2, 0x26);
HI351_write_cmos_sensor(0xd3, 0x30); //Outdoor 1426
HI351_write_cmos_sensor(0xd4, 0x27);
HI351_write_cmos_sensor(0xd5, 0x20); //Outdoor 1427
HI351_write_cmos_sensor(0xd6, 0x28);
HI351_write_cmos_sensor(0xd7, 0x3c); //Outdoor 1428
HI351_write_cmos_sensor(0xd8, 0x29);
HI351_write_cmos_sensor(0xd9, 0x00); //Outdoor 1429
HI351_write_cmos_sensor(0xda, 0x2a);
HI351_write_cmos_sensor(0xdb, 0x16); //Outdoor 142a S_Diff Hi Dr_Gain
HI351_write_cmos_sensor(0xdc, 0x2b);
HI351_write_cmos_sensor(0xdd, 0x16); //Outdoor 142b S_Diff Middle Dr_Gain
HI351_write_cmos_sensor(0xde, 0x2c);
HI351_write_cmos_sensor(0xdf, 0x16); //Outdoor 142c S_Diff Low Dr_Gain
HI351_write_cmos_sensor(0xe0, 0x2d);
HI351_write_cmos_sensor(0xe1, 0x54); //Outdoor 142d
HI351_write_cmos_sensor(0xe2, 0x2e);
HI351_write_cmos_sensor(0xe3, 0x54); //Outdoor 142e
HI351_write_cmos_sensor(0xe4, 0x2f);
HI351_write_cmos_sensor(0xe5, 0x54); //Outdoor 142f
HI351_write_cmos_sensor(0xe6, 0x30);
HI351_write_cmos_sensor(0xe7, 0x82); //Outdoor 1430
HI351_write_cmos_sensor(0xe8, 0x31);
HI351_write_cmos_sensor(0xe9, 0x02); //Outdoor 1431
HI351_write_cmos_sensor(0xea, 0x32);
HI351_write_cmos_sensor(0xeb, 0x04); //Outdoor 1432
HI351_write_cmos_sensor(0xec, 0x33);
HI351_write_cmos_sensor(0xed, 0x04); //Outdoor 1433
HI351_write_cmos_sensor(0xee, 0x34);
HI351_write_cmos_sensor(0xef, 0x0a); //Outdoor 1434
HI351_write_cmos_sensor(0xf0, 0x35);
HI351_write_cmos_sensor(0xf1, 0x46); //Outdoor 1435
HI351_write_cmos_sensor(0xf2, 0x36);
HI351_write_cmos_sensor(0xf3, 0x32); //Outdoor 1436
HI351_write_cmos_sensor(0xf4, 0x37);
HI351_write_cmos_sensor(0xf5, 0x28); //Outdoor 1437
HI351_write_cmos_sensor(0xf6, 0x38);
HI351_write_cmos_sensor(0xf7, 0x12); //Outdoor 1438
HI351_write_cmos_sensor(0xf8, 0x39);
HI351_write_cmos_sensor(0xf9, 0x00); //Outdoor 1439
HI351_write_cmos_sensor(0xfa, 0x3a);
HI351_write_cmos_sensor(0xfb, 0x24); //Outdoor 143a L_Diff Hi Dr_Gain
HI351_write_cmos_sensor(0xfc, 0x3b);
HI351_write_cmos_sensor(0xfd, 0x24); //Outdoor 143b L_Diff Middle Dr_Gain
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
HI351_write_cmos_sensor(0x03, 0xdc);
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x3c);
HI351_write_cmos_sensor(0x11, 0x24); //Outdoor 143c L_Diff Low Dr_Gain
HI351_write_cmos_sensor(0x12, 0x3d);
HI351_write_cmos_sensor(0x13, 0x24); //Outdoor 143d L_Diff Hi_Normal Gain
HI351_write_cmos_sensor(0x14, 0x3e);
HI351_write_cmos_sensor(0x15, 0x24); //Outdoor 143e L_Diff Middle_Normal Gain
HI351_write_cmos_sensor(0x16, 0x3f);
HI351_write_cmos_sensor(0x17, 0x14); //Outdoor 143f L_Diff Low_Normal Gain
HI351_write_cmos_sensor(0x18, 0x40);
HI351_write_cmos_sensor(0x19, 0x82); //Outdoor 1440
HI351_write_cmos_sensor(0x1a, 0x41);
HI351_write_cmos_sensor(0x1b, 0x1a); //Outdoor 1441 H Clip
HI351_write_cmos_sensor(0x1c, 0x42);
HI351_write_cmos_sensor(0x1d, 0x70); //Outdoor 1442
HI351_write_cmos_sensor(0x1e, 0x43);
HI351_write_cmos_sensor(0x1f, 0x20); //Outdoor 1443
HI351_write_cmos_sensor(0x20, 0x44);
HI351_write_cmos_sensor(0x21, 0x26); //Outdoor 1444 M_Diff Hi Dr_Gain
HI351_write_cmos_sensor(0x22, 0x45);
HI351_write_cmos_sensor(0x23, 0x1e); //Outdoor 1445 M_Diff Low Dr_Gain
HI351_write_cmos_sensor(0x24, 0x46);
HI351_write_cmos_sensor(0x25, 0x24); //Outdoor 1446 M_Diff Middle Dr_Gain
HI351_write_cmos_sensor(0x26, 0x47);
HI351_write_cmos_sensor(0x27, 0x18); //Outdoor 1447
HI351_write_cmos_sensor(0x28, 0x48);
HI351_write_cmos_sensor(0x29, 0x0a); //Outdoor 1448
HI351_write_cmos_sensor(0x2a, 0x49);
HI351_write_cmos_sensor(0x2b, 0x10); //Outdoor 1449
HI351_write_cmos_sensor(0x2c, 0x50);
HI351_write_cmos_sensor(0x2d, 0x82); //Outdoor 1450
HI351_write_cmos_sensor(0x2e, 0x51);
HI351_write_cmos_sensor(0x2f, 0x38); //Outdoor 1451
HI351_write_cmos_sensor(0x30, 0x52);
HI351_write_cmos_sensor(0x31, 0xf0); //Outdoor 1452
HI351_write_cmos_sensor(0x32, 0x53);
HI351_write_cmos_sensor(0x33, 0x78); //Outdoor 1453
HI351_write_cmos_sensor(0x34, 0x54);
HI351_write_cmos_sensor(0x35, 0x24); //Outdoor 1454
HI351_write_cmos_sensor(0x36, 0x55);
HI351_write_cmos_sensor(0x37, 0x24); //Outdoor 1455
HI351_write_cmos_sensor(0x38, 0x56);
HI351_write_cmos_sensor(0x39, 0x24); //Outdoor 1456
HI351_write_cmos_sensor(0x3a, 0x57);
HI351_write_cmos_sensor(0x3b, 0x10); //Outdoor 1457
HI351_write_cmos_sensor(0x3c, 0x58);
HI351_write_cmos_sensor(0x3d, 0x14); //Outdoor 1458
HI351_write_cmos_sensor(0x3e, 0x59);
HI351_write_cmos_sensor(0x3f, 0x10); //Outdoor 1459
HI351_write_cmos_sensor(0x40, 0x60);
HI351_write_cmos_sensor(0x41, 0x01); //Outdoor 1460 skin opt
HI351_write_cmos_sensor(0x42, 0x61);
HI351_write_cmos_sensor(0x43, 0xa0); //Outdoor 1461
HI351_write_cmos_sensor(0x44, 0x62);
HI351_write_cmos_sensor(0x45, 0x98); //Outdoor 1462
HI351_write_cmos_sensor(0x46, 0x63);
HI351_write_cmos_sensor(0x47, 0xe4); //Outdoor 1463 skin_std_th_h
HI351_write_cmos_sensor(0x48, 0x64);
HI351_write_cmos_sensor(0x49, 0xa4); //Outdoor 1464 skin_std_th_l
HI351_write_cmos_sensor(0x4a, 0x65);
HI351_write_cmos_sensor(0x4b, 0x7d); //Outdoor 1465 sharp_std_th_h
HI351_write_cmos_sensor(0x4c, 0x66);
HI351_write_cmos_sensor(0x4d, 0x4b); //Outdoor 1466 sharp_std_th_l
HI351_write_cmos_sensor(0x4e, 0x70);
HI351_write_cmos_sensor(0x4f, 0x10); //Outdoor 1470
HI351_write_cmos_sensor(0x50, 0x71);
HI351_write_cmos_sensor(0x51, 0x10); //Outdoor 1471
HI351_write_cmos_sensor(0x52, 0x72);
HI351_write_cmos_sensor(0x53, 0x10); //Outdoor 1472
HI351_write_cmos_sensor(0x54, 0x73);
HI351_write_cmos_sensor(0x55, 0x10); //Outdoor 1473
HI351_write_cmos_sensor(0x56, 0x74);
HI351_write_cmos_sensor(0x57, 0x10); //Outdoor 1474
HI351_write_cmos_sensor(0x58, 0x75);
HI351_write_cmos_sensor(0x59, 0x10); //Outdoor 1475
HI351_write_cmos_sensor(0x5a, 0x76);
HI351_write_cmos_sensor(0x5b, 0x38); //Outdoor 1476 green sharp pos High
HI351_write_cmos_sensor(0x5c, 0x77);
HI351_write_cmos_sensor(0x5d, 0x38); //Outdoor 1477 green sharp pos Middle
HI351_write_cmos_sensor(0x5e, 0x78);
HI351_write_cmos_sensor(0x5f, 0x38); //Outdoor 1478 green sharp pos Low
HI351_write_cmos_sensor(0x60, 0x79);
HI351_write_cmos_sensor(0x61, 0x58); //Outdoor 1479 green sharp nega High
HI351_write_cmos_sensor(0x62, 0x7a);
HI351_write_cmos_sensor(0x63, 0x58); //Outdoor 147a green sharp nega Middle
HI351_write_cmos_sensor(0x64, 0x7b);
HI351_write_cmos_sensor(0x65, 0x58); //Outdoor 147b green sharp nega Low
HI351_write_cmos_sensor(0x66, 0x03);
HI351_write_cmos_sensor(0x67, 0x10); //10 page
HI351_write_cmos_sensor(0x68, 0x60);
HI351_write_cmos_sensor(0x69, 0x03); //1060
HI351_write_cmos_sensor(0x6a, 0x70);
HI351_write_cmos_sensor(0x6b, 0x0c); //1070
HI351_write_cmos_sensor(0x6c, 0x71);
HI351_write_cmos_sensor(0x6d, 0x00); //1071
HI351_write_cmos_sensor(0x6e, 0x72);
HI351_write_cmos_sensor(0x6f, 0x7a); //1072
HI351_write_cmos_sensor(0x70, 0x73);
HI351_write_cmos_sensor(0x71, 0x28); //1073
HI351_write_cmos_sensor(0x72, 0x74);
HI351_write_cmos_sensor(0x73, 0x14); //1074
HI351_write_cmos_sensor(0x74, 0x75);
HI351_write_cmos_sensor(0x75, 0x0d); //1075
HI351_write_cmos_sensor(0x76, 0x76);
HI351_write_cmos_sensor(0x77, 0x40); //1076
HI351_write_cmos_sensor(0x78, 0x77);
HI351_write_cmos_sensor(0x79, 0x49); //1077
HI351_write_cmos_sensor(0x7a, 0x78);
HI351_write_cmos_sensor(0x7b, 0x99); //1078
HI351_write_cmos_sensor(0x7c, 0x79);
HI351_write_cmos_sensor(0x7d, 0x4c); //1079
HI351_write_cmos_sensor(0x7e, 0x7a);
HI351_write_cmos_sensor(0x7f, 0xcc); //107a
HI351_write_cmos_sensor(0x80, 0x7b);
HI351_write_cmos_sensor(0x81, 0x49); //107b
HI351_write_cmos_sensor(0x82, 0x7c);
HI351_write_cmos_sensor(0x83, 0x99); //107c
HI351_write_cmos_sensor(0x84, 0x7d);
HI351_write_cmos_sensor(0x85, 0x14); //107d
HI351_write_cmos_sensor(0x86, 0x7e);
HI351_write_cmos_sensor(0x87, 0x28); //107e
HI351_write_cmos_sensor(0x88, 0x7f);
HI351_write_cmos_sensor(0x89, 0x50); //107f
HI351_write_cmos_sensor(0x8a, 0x03); //16 page
HI351_write_cmos_sensor(0x8b, 0x16);
HI351_write_cmos_sensor(0x8c, 0x8a); //168a
HI351_write_cmos_sensor(0x8d, 0x68);
HI351_write_cmos_sensor(0x8e, 0x8b); //168b
HI351_write_cmos_sensor(0x8f, 0x7c);
HI351_write_cmos_sensor(0x90, 0x8c); //168c
HI351_write_cmos_sensor(0x91, 0x7f);
HI351_write_cmos_sensor(0x92, 0x8d); //168d
HI351_write_cmos_sensor(0x93, 0x7f);
HI351_write_cmos_sensor(0x94, 0x8e); //168e
HI351_write_cmos_sensor(0x95, 0x7f);
HI351_write_cmos_sensor(0x96, 0x8f); //168f
HI351_write_cmos_sensor(0x97, 0x7f);
HI351_write_cmos_sensor(0x98, 0x90); //1690
HI351_write_cmos_sensor(0x99, 0x7f);
HI351_write_cmos_sensor(0x9a, 0x91); //1691
HI351_write_cmos_sensor(0x9b, 0x7f);
HI351_write_cmos_sensor(0x9c, 0x92); //1692
HI351_write_cmos_sensor(0x9d, 0x7f);
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
////////////////////
// dd Page (DMA Indoor)
////////////////////
HI351_write_cmos_sensor(0x03, 0xdd);
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x03);
HI351_write_cmos_sensor(0x11, 0x11);
HI351_write_cmos_sensor(0x12, 0x10);
    HI351_write_cmos_sensor(0x13, 0x13);
    HI351_write_cmos_sensor(0x14, 0x11);//Indoor 0x1111
    HI351_write_cmos_sensor(0x15, 0x0c);
    HI351_write_cmos_sensor(0x16, 0x12);//Indoor 0x1112
    HI351_write_cmos_sensor(0x17, 0x22);
    HI351_write_cmos_sensor(0x18, 0x13);//Indoor 0x1113
    HI351_write_cmos_sensor(0x19, 0x22);
    HI351_write_cmos_sensor(0x1a, 0x14);//Indoor 0x1114
    HI351_write_cmos_sensor(0x1b, 0x3a);
    HI351_write_cmos_sensor(0x1c, 0x30);//Indoor 0x1130
    HI351_write_cmos_sensor(0x1d, 0x20);
    HI351_write_cmos_sensor(0x1e, 0x31);//Indoor 0x1131
    HI351_write_cmos_sensor(0x1f, 0x20);

    HI351_write_cmos_sensor(0x20, 0x32); //Indoor 0x1132 //STEVE Lum. Level. in DLPF
    HI351_write_cmos_sensor(0x21, 0x8b); //52
    HI351_write_cmos_sensor(0x22, 0x33); //Indoor 0x1133
    HI351_write_cmos_sensor(0x23, 0x54); //3b
    HI351_write_cmos_sensor(0x24, 0x34); //Indoor 0x1134
    HI351_write_cmos_sensor(0x25, 0x2c); //1d
    HI351_write_cmos_sensor(0x26, 0x35); //Indoor 0x1135
    HI351_write_cmos_sensor(0x27, 0x29); //21
    HI351_write_cmos_sensor(0x28, 0x36); //Indoor 0x1136
    HI351_write_cmos_sensor(0x29, 0x18); //1b
    HI351_write_cmos_sensor(0x2a, 0x37); //Indoor 0x1137
    HI351_write_cmos_sensor(0x2b, 0x1e); //21
    HI351_write_cmos_sensor(0x2c, 0x38); //Indoor 0x1138
    HI351_write_cmos_sensor(0x2d, 0x17); //18

    HI351_write_cmos_sensor(0x2e, 0x39);//Indoor 0x1139 gain 1
    HI351_write_cmos_sensor(0x2f, 0x34);    //r2 1
    HI351_write_cmos_sensor(0x30, 0x3a);//Indoor 0x113a
    HI351_write_cmos_sensor(0x31, 0x38);
    HI351_write_cmos_sensor(0x32, 0x3b);//Indoor 0x113b
    HI351_write_cmos_sensor(0x33, 0x3a);
    HI351_write_cmos_sensor(0x34, 0x3c);//Indoor 0x113c
    HI351_write_cmos_sensor(0x35, 0x38);   //18
    HI351_write_cmos_sensor(0x36, 0x3d);//Indoor 0x113d
    HI351_write_cmos_sensor(0x37, 0x2a);   //18
    HI351_write_cmos_sensor(0x38, 0x3e);//Indoor 0x113e
    HI351_write_cmos_sensor(0x39, 0x26);   //18
    HI351_write_cmos_sensor(0x3a, 0x3f);//Indoor 0x113f
    HI351_write_cmos_sensor(0x3b, 0x22);
    HI351_write_cmos_sensor(0x3c, 0x40);//Indoor 0x1140 gain 8
    HI351_write_cmos_sensor(0x3d, 0x20);
    HI351_write_cmos_sensor(0x3e, 0x41);//Indoor 0x1141
    HI351_write_cmos_sensor(0x3f, 0x20); //50);
    HI351_write_cmos_sensor(0x40, 0x42);//Indoor 0x1142
    HI351_write_cmos_sensor(0x41, 0x20); //50);
    HI351_write_cmos_sensor(0x42, 0x43);//Indoor 0x1143
    HI351_write_cmos_sensor(0x43, 0x20); //50);
    HI351_write_cmos_sensor(0x44, 0x44);//Indoor 0x1144
    HI351_write_cmos_sensor(0x45, 0x20); //80);
    HI351_write_cmos_sensor(0x46, 0x45);//Indoor 0x1145
    HI351_write_cmos_sensor(0x47, 0x20); //80);
    HI351_write_cmos_sensor(0x48, 0x46);//Indoor 0x1146
    HI351_write_cmos_sensor(0x49, 0x20); //80);
    HI351_write_cmos_sensor(0x4a, 0x47);//Indoor 0x1147
    HI351_write_cmos_sensor(0x4b, 0x20); //80);
    HI351_write_cmos_sensor(0x4c, 0x48);//Indoor 0x1148
    HI351_write_cmos_sensor(0x4d, 0x20); //80);
    HI351_write_cmos_sensor(0x4e, 0x49);//Indoor 0x1149
    HI351_write_cmos_sensor(0x4f, 0xf0); //fc); //high_clip_start
    HI351_write_cmos_sensor(0x50, 0x4a);//Indoor 0x114a
    HI351_write_cmos_sensor(0x51, 0xf0); //fc);
    HI351_write_cmos_sensor(0x52, 0x4b);//Indoor 0x114b
    HI351_write_cmos_sensor(0x53, 0xf0); //fc);
    HI351_write_cmos_sensor(0x54, 0x4c);//Indoor 0x114c //NR Gain MAx Limit
    HI351_write_cmos_sensor(0x55, 0xf0);//0224 jk//0xfc);
    HI351_write_cmos_sensor(0x56, 0x4d);//Indoor 0x114d
    HI351_write_cmos_sensor(0x57, 0xf0); //fc);
    HI351_write_cmos_sensor(0x58, 0x4e); //10);//Indoor 0x114e
    HI351_write_cmos_sensor(0x59, 0xf0);   //Lv 6 h_clip
    HI351_write_cmos_sensor(0x5a, 0x4f);//Indoor 0x114f
    HI351_write_cmos_sensor(0x5b, 0xf0);   //Lv 7 h_clip
    HI351_write_cmos_sensor(0x5c, 0x50);//Indoor 0x1150 clip 8
    HI351_write_cmos_sensor(0x5d, 0xf0);
    HI351_write_cmos_sensor(0x5e, 0x51);//Indoor 0x1151
    HI351_write_cmos_sensor(0x5f, 0x68); //test//0x08); //color gain start
    HI351_write_cmos_sensor(0x60, 0x52);//Indoor 0x1152
    HI351_write_cmos_sensor(0x61, 0x68); //test//0x08);
    HI351_write_cmos_sensor(0x62, 0x53);//Indoor 0x1153
    HI351_write_cmos_sensor(0x63, 0x68); //test//0x08);
    HI351_write_cmos_sensor(0x64, 0x54);//Indoor 0x1154
    HI351_write_cmos_sensor(0x65, 0x68); //test//0x08);
    HI351_write_cmos_sensor(0x66, 0x55);//Indoor 0x1155
    HI351_write_cmos_sensor(0x67, 0x68); //test//0x08);
    HI351_write_cmos_sensor(0x68, 0x56);//Indoor 0x1156
    HI351_write_cmos_sensor(0x69, 0x68); //test//0x08);
    HI351_write_cmos_sensor(0x6a, 0x57);//Indoor 0x1157
    HI351_write_cmos_sensor(0x6b, 0x68); //test//0x08);
    HI351_write_cmos_sensor(0x6c, 0x58);//Indoor 0x1158
    HI351_write_cmos_sensor(0x6d, 0x68); //test//0x08); //color gain end
    HI351_write_cmos_sensor(0x6e, 0x59);//Indoor 0x1159
    HI351_write_cmos_sensor(0x6f, 0x80); //test//0x10); //color ofs lmt start
    HI351_write_cmos_sensor(0x70, 0x5a);//Indoor 0x115a
    HI351_write_cmos_sensor(0x71, 0x80); //test//0x10);
    HI351_write_cmos_sensor(0x72, 0x5b);//Indoor 0x115b
    HI351_write_cmos_sensor(0x73, 0x80); //test//0x10);
    HI351_write_cmos_sensor(0x74, 0x5c);//Indoor 0x115c
    HI351_write_cmos_sensor(0x75, 0x80); //test//0x10);
    HI351_write_cmos_sensor(0x76, 0x5d);//Indoor 0x115d
    HI351_write_cmos_sensor(0x77, 0x80); //test//0x10);
    HI351_write_cmos_sensor(0x78, 0x5e);//Indoor 0x115e
    HI351_write_cmos_sensor(0x79, 0x80); //test//0x10);
    HI351_write_cmos_sensor(0x7a, 0x5f);//Indoor 0x115f
    HI351_write_cmos_sensor(0x7b, 0x80); //test//0x10);
    HI351_write_cmos_sensor(0x7c, 0x60);//Indoor 0x1160
    HI351_write_cmos_sensor(0x7d, 0x80); //test//0x10);//color ofs lmt end
    HI351_write_cmos_sensor(0x7e, 0x61);//Indoor 0x1161
    HI351_write_cmos_sensor(0x7f, 0xc0);
    HI351_write_cmos_sensor(0x80, 0x62);//Indoor 0x1162
    HI351_write_cmos_sensor(0x81, 0xf0);
    HI351_write_cmos_sensor(0x82, 0x63);//Indoor 0x1163
    HI351_write_cmos_sensor(0x83, 0x80);
    HI351_write_cmos_sensor(0x84, 0x64);//Indoor 0x1164
    HI351_write_cmos_sensor(0x85, 0x40);
    HI351_write_cmos_sensor(0x86, 0x65);//Indoor 0x1165
    HI351_write_cmos_sensor(0x87, 0x60);
    HI351_write_cmos_sensor(0x88, 0x66);//Indoor 0x1166
    HI351_write_cmos_sensor(0x89, 0x60);
    HI351_write_cmos_sensor(0x8a, 0x67);//Indoor 0x1167
    HI351_write_cmos_sensor(0x8b, 0x60);
    HI351_write_cmos_sensor(0x8c, 0x68);//Indoor 0x1168
    HI351_write_cmos_sensor(0x8d, 0x80);
    HI351_write_cmos_sensor(0x8e, 0x69);//Indoor 0x1169
    HI351_write_cmos_sensor(0x8f, 0x40);
    HI351_write_cmos_sensor(0x90, 0x6a);//Indoor 0x116a     //Imp Lv2 High Gain
    HI351_write_cmos_sensor(0x91, 0x60); //20);//0224 jk//0x60);
    HI351_write_cmos_sensor(0x92, 0x6b);//Indoor 0x116b     //Imp Lv2 Middle Gain
    HI351_write_cmos_sensor(0x93, 0x60); //20);//0224 jk//0x60);
    HI351_write_cmos_sensor(0x94, 0x6c);//Indoor 0x116c     //Imp Lv2 Low Gain
    HI351_write_cmos_sensor(0x95, 0x60); //20);//0224 jk//0x60);
    HI351_write_cmos_sensor(0x96, 0x6d);//Indoor 0x116d
    HI351_write_cmos_sensor(0x97, 0x80);
    HI351_write_cmos_sensor(0x98, 0x6e);//Indoor 0x116e
    HI351_write_cmos_sensor(0x99, 0x40);
    HI351_write_cmos_sensor(0x9a, 0x6f);//Indoor 0x116f    //Imp Lv3 Hi Gain
    HI351_write_cmos_sensor(0x9b, 0x60); //10);//0224 jk//0x60);
    HI351_write_cmos_sensor(0x9c, 0x70);//Indoor 0x1170    //Imp Lv3 Middle Gain
    HI351_write_cmos_sensor(0x9d, 0x60); //10);//0224 jk//0x60);
    HI351_write_cmos_sensor(0x9e, 0x71);//Indoor 0x1171    //Imp Lv3 Low Gain
    HI351_write_cmos_sensor(0x9f, 0x40); //10);//0224 jk//0x60);
    HI351_write_cmos_sensor(0xa0, 0x72);//Indoor 0x1172
    HI351_write_cmos_sensor(0xa1, 0x6e);
    HI351_write_cmos_sensor(0xa2, 0x73);//Indoor 0x1173
    HI351_write_cmos_sensor(0xa3, 0x3a);
    HI351_write_cmos_sensor(0xa4, 0x74);//Indoor 0x1174    //Imp Lv4 Hi Gain
    HI351_write_cmos_sensor(0xa5, 0x60); //10);//0224 jk//0x60);
    HI351_write_cmos_sensor(0xa6, 0x75);//Indoor 0x1175    //Imp Lv4 Middle Gain
    HI351_write_cmos_sensor(0xa7, 0x60); //10);//0224 jk//0x60);
    HI351_write_cmos_sensor(0xa8, 0x76);//Indoor 0x1176    //Imp Lv4 Low Gain
    HI351_write_cmos_sensor(0xa9, 0x40); //10);//0224 jk//0x60);//18
    HI351_write_cmos_sensor(0xaa, 0x77);//Indoor 0x1177    //Imp Lv5 Hi Th
    HI351_write_cmos_sensor(0xab, 0x6e);
    HI351_write_cmos_sensor(0xac, 0x78);//Indoor 0x1178    //Imp Lv5 Middle Th
    HI351_write_cmos_sensor(0xad, 0x66);
    HI351_write_cmos_sensor(0xae, 0x79);//Indoor 0x1179    //Imp Lv5 Hi Gain
    HI351_write_cmos_sensor(0xaf, 0x60); //10);//0224 jk//0x50);
    HI351_write_cmos_sensor(0xb0, 0x7a);//Indoor 0x117a    //Imp Lv5 Middle Gain
    HI351_write_cmos_sensor(0xb1, 0x40); //10);//0224 jk//0x50);
    HI351_write_cmos_sensor(0xb2, 0x7b);//Indoor 0x117b    //Imp Lv5 Low Gain
    HI351_write_cmos_sensor(0xb3, 0x20); //10);//0224 jk//0x50);
    HI351_write_cmos_sensor(0xb4, 0x7c);//Indoor 0x117c    //Imp Lv6 Hi Th
    HI351_write_cmos_sensor(0xb5, 0x5c);
    HI351_write_cmos_sensor(0xb6, 0x7d);//Indoor 0x117d    //Imp Lv6 Middle Th
    HI351_write_cmos_sensor(0xb7, 0x30);
    HI351_write_cmos_sensor(0xb8, 0x7e);//Indoor 0x117e    //Imp Lv6 Hi Gain
    HI351_write_cmos_sensor(0xb9, 0x10);//0224 jk//0x44);
    HI351_write_cmos_sensor(0xba, 0x7f);//Indoor 0x117f    //Imp Lv6 Middle Gain
    HI351_write_cmos_sensor(0xbb, 0x10);//0224 jk//0x44);
    HI351_write_cmos_sensor(0xbc, 0x80);//Indoor 0x1180    //Imp Lv6 Low Gain
    HI351_write_cmos_sensor(0xbd, 0x10);//0224 jk//0x44);
    HI351_write_cmos_sensor(0xbe, 0x81);//Indoor 0x1181
    HI351_write_cmos_sensor(0xbf, 0x62);
    HI351_write_cmos_sensor(0xc0, 0x82);//Indoor 0x1182
    HI351_write_cmos_sensor(0xc1, 0x26);
    HI351_write_cmos_sensor(0xc2, 0x83);//Indoor 0x1183    //Imp Lv7 Hi Gain
    HI351_write_cmos_sensor(0xc3, 0x40); //10);//0224 jk//0x3e);
    HI351_write_cmos_sensor(0xc4, 0x84);//Indoor 0x1184    //Imp Lv7 Middle Gain
    HI351_write_cmos_sensor(0xc5, 0x40); //10);//0224 jk//0x3e);
    HI351_write_cmos_sensor(0xc6, 0x85);//Indoor 0x1185    //Imp Lv7 Low Gain
    HI351_write_cmos_sensor(0xc7, 0x20); //10);//0224 jk//0x3e);
    HI351_write_cmos_sensor(0xc8, 0x86);//Indoor 0x1186
    HI351_write_cmos_sensor(0xc9, 0x62);
    HI351_write_cmos_sensor(0xca, 0x87);//Indoor 0x1187
    HI351_write_cmos_sensor(0xcb, 0x26);
    HI351_write_cmos_sensor(0xcc, 0x88);//Indoor 0x1188
    HI351_write_cmos_sensor(0xcd, 0x30);
    HI351_write_cmos_sensor(0xce, 0x89);//Indoor 0x1189
    HI351_write_cmos_sensor(0xcf, 0x30);
    HI351_write_cmos_sensor(0xd0, 0x8a);//Indoor 0x118a
    HI351_write_cmos_sensor(0xd1, 0x30);
    HI351_write_cmos_sensor(0xd2, 0x90);//Indoor 0x1190
    HI351_write_cmos_sensor(0xd3, 0x00);
    HI351_write_cmos_sensor(0xd4, 0x91);//Indoor 0x1191
    HI351_write_cmos_sensor(0xd5, 0x4e);
    HI351_write_cmos_sensor(0xd6, 0x92);//Indoor 0x1192
    HI351_write_cmos_sensor(0xd7, 0x00);
    HI351_write_cmos_sensor(0xd8, 0x93);//Indoor 0x1193
    HI351_write_cmos_sensor(0xd9, 0x16);
    HI351_write_cmos_sensor(0xda, 0x94);//Indoor 0x1194
    HI351_write_cmos_sensor(0xdb, 0x01);
    HI351_write_cmos_sensor(0xdc, 0x95);//Indoor 0x1195
    HI351_write_cmos_sensor(0xdd, 0x80);
    HI351_write_cmos_sensor(0xde, 0x96);//Indoor 0x1196
    HI351_write_cmos_sensor(0xdf, 0x55);
    HI351_write_cmos_sensor(0xe0, 0x97);//Indoor 0x1197
    HI351_write_cmos_sensor(0xe1, 0x8d);
    HI351_write_cmos_sensor(0xe2, 0xb0);//Indoor 0x11b0
    HI351_write_cmos_sensor(0xe3, 0x60);
    HI351_write_cmos_sensor(0xe4, 0xb1);//Indoor 0x11b1
    HI351_write_cmos_sensor(0xe5, 0x99);
    HI351_write_cmos_sensor(0xe6, 0xb2);//Indoor 0x11b2
    HI351_write_cmos_sensor(0xe7, 0x19);
    HI351_write_cmos_sensor(0xe8, 0xb3);//Indoor 0x11b3
    HI351_write_cmos_sensor(0xe9, 0x00);
    HI351_write_cmos_sensor(0xea, 0xb4);//Indoor 0x11b4
    HI351_write_cmos_sensor(0xeb, 0x00);
    HI351_write_cmos_sensor(0xec, 0x03); //12 page
    HI351_write_cmos_sensor(0xed, 0x12);
    HI351_write_cmos_sensor(0xee, 0x10); //Indoor 0x1210
    HI351_write_cmos_sensor(0xef, 0x03);
    HI351_write_cmos_sensor(0xf0, 0x11); //Indoor 0x1211
    HI351_write_cmos_sensor(0xf1, 0x29);
    HI351_write_cmos_sensor(0xf2, 0x12); //Indoor 0x1212
    HI351_write_cmos_sensor(0xf3, 0x08);
    HI351_write_cmos_sensor(0xf4, 0x40);//Indoor 0x1240
    HI351_write_cmos_sensor(0xf5, 0x33); //07
    HI351_write_cmos_sensor(0xf6, 0x41);//Indoor 0x1241
    HI351_write_cmos_sensor(0xf7, 0x0a); //32
    HI351_write_cmos_sensor(0xf8, 0x42);//Indoor 0x1242
    HI351_write_cmos_sensor(0xf9, 0x6a); //8c
    HI351_write_cmos_sensor(0xfa, 0x43);//Indoor 0x1243
    HI351_write_cmos_sensor(0xfb, 0x80);
    HI351_write_cmos_sensor(0xfc, 0x44); //Indoor 0x1244
    HI351_write_cmos_sensor(0xfd, 0x02);
    HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
    HI351_write_cmos_sensor(0x03, 0xde);
    HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
    HI351_write_cmos_sensor(0x10, 0x45); //Indoor 0x1245
    HI351_write_cmos_sensor(0x11, 0x0a);
    HI351_write_cmos_sensor(0x12, 0x46); //Indoor 0x1246
    HI351_write_cmos_sensor(0x13, 0x80);
    HI351_write_cmos_sensor(0x14, 0x60); //Indoor 0x1260
    HI351_write_cmos_sensor(0x15, 0x02);
    HI351_write_cmos_sensor(0x16, 0x61); //Indoor 0x1261
    HI351_write_cmos_sensor(0x17, 0x04);
    HI351_write_cmos_sensor(0x18, 0x62); //Indoor 0x1262
    HI351_write_cmos_sensor(0x19, 0x4b);
    HI351_write_cmos_sensor(0x1a, 0x63); //Indoor 0x1263
    HI351_write_cmos_sensor(0x1b, 0x41);
    HI351_write_cmos_sensor(0x1c, 0x64); //Indoor 0x1264
    HI351_write_cmos_sensor(0x1d, 0x14);
    HI351_write_cmos_sensor(0x1e, 0x65); //Indoor 0x1265
    HI351_write_cmos_sensor(0x1f, 0x00);
    HI351_write_cmos_sensor(0x20, 0x68); //Indoor 0x1268
    HI351_write_cmos_sensor(0x21, 0x0a);
    HI351_write_cmos_sensor(0x22, 0x69); //Indoor 0x1269
    HI351_write_cmos_sensor(0x23, 0x04);
    HI351_write_cmos_sensor(0x24, 0x6a); //Indoor 0x126a
    HI351_write_cmos_sensor(0x25, 0x0a);
    HI351_write_cmos_sensor(0x26, 0x6b); //Indoor 0x126b
    HI351_write_cmos_sensor(0x27, 0x0a);
    HI351_write_cmos_sensor(0x28, 0x6c); //Indoor 0x126c
    HI351_write_cmos_sensor(0x29, 0x24);
    HI351_write_cmos_sensor(0x2a, 0x6d); //Indoor 0x126d
    HI351_write_cmos_sensor(0x2b, 0x01);
    HI351_write_cmos_sensor(0x2c, 0x70); //Indoor 0x1270
    HI351_write_cmos_sensor(0x2d, 0x25);
    HI351_write_cmos_sensor(0x2e, 0x71);//Indoor 0x1271
    HI351_write_cmos_sensor(0x2f, 0x7f);
    HI351_write_cmos_sensor(0x30, 0x80);//Indoor 0x1280
    HI351_write_cmos_sensor(0x31, 0x82);//88
    HI351_write_cmos_sensor(0x32, 0x81);//Indoor 0x1281
    HI351_write_cmos_sensor(0x33, 0x86); //05
    HI351_write_cmos_sensor(0x34, 0x82);//Indoor 0x1282
    HI351_write_cmos_sensor(0x35, 0x06);//13
    HI351_write_cmos_sensor(0x36, 0x83);//Indoor 0x1283
    HI351_write_cmos_sensor(0x37, 0x04);//40
    HI351_write_cmos_sensor(0x38, 0x84);//Indoor 0x1284
    HI351_write_cmos_sensor(0x39, 0x10);
    HI351_write_cmos_sensor(0x3a, 0x85);//Indoor 0x1285
    HI351_write_cmos_sensor(0x3b, 0x86);
    HI351_write_cmos_sensor(0x3c, 0x86);//Indoor 0x1286
    HI351_write_cmos_sensor(0x3d, 0x90);//15
    HI351_write_cmos_sensor(0x3e, 0x87);//Indoor 0x1287
    HI351_write_cmos_sensor(0x3f, 0x10);
    HI351_write_cmos_sensor(0x40, 0x88);//Indoor 0x1288
    HI351_write_cmos_sensor(0x41, 0x3a);
    HI351_write_cmos_sensor(0x42, 0x89);//Indoor 0x1289
    HI351_write_cmos_sensor(0x43, 0x80);//c0
    HI351_write_cmos_sensor(0x44, 0x8a);//Indoor 0x128a
    HI351_write_cmos_sensor(0x45, 0xa0);//18
    HI351_write_cmos_sensor(0x46, 0x8b); //Indoor 0x128b
    HI351_write_cmos_sensor(0x47, 0x03);//05
    HI351_write_cmos_sensor(0x48, 0x8c); //Indoor 0x128c
    HI351_write_cmos_sensor(0x49, 0x02);
    HI351_write_cmos_sensor(0x4a, 0x8d); //Indoor 0x128d
    HI351_write_cmos_sensor(0x4b, 0x02);
    HI351_write_cmos_sensor(0x4c, 0xe6); //Indoor 0x12e6
    HI351_write_cmos_sensor(0x4d, 0xff);
    HI351_write_cmos_sensor(0x4e, 0xe7); //Indoor 0x12e7
    HI351_write_cmos_sensor(0x4f, 0x18);
    HI351_write_cmos_sensor(0x50, 0xe8); //Indoor 0x12e8
    HI351_write_cmos_sensor(0x51, 0x0a);
    HI351_write_cmos_sensor(0x52, 0xe9); //Indoor 0x12e9
    HI351_write_cmos_sensor(0x53, 0x04);
    HI351_write_cmos_sensor(0x54, 0x03);//Indoor Page13
    HI351_write_cmos_sensor(0x55, 0x13);
    HI351_write_cmos_sensor(0x56, 0x10);//Indoor 0x1310
    HI351_write_cmos_sensor(0x57, 0x33);
    HI351_write_cmos_sensor(0x58, 0x20);//Indoor 0x1320
    HI351_write_cmos_sensor(0x59, 0x20);
    HI351_write_cmos_sensor(0x5a, 0x21);//Indoor 0x1321
    HI351_write_cmos_sensor(0x5b, 0x30);
    HI351_write_cmos_sensor(0x5c, 0x22);//Indoor 0x1322
    HI351_write_cmos_sensor(0x5d, 0x36);
    HI351_write_cmos_sensor(0x5e, 0x23);//Indoor 0x1323
    HI351_write_cmos_sensor(0x5f, 0x6a);
    HI351_write_cmos_sensor(0x60, 0x24);//Indoor 0x1324
    HI351_write_cmos_sensor(0x61, 0xa0);
    HI351_write_cmos_sensor(0x62, 0x25);//Indoor 0x1325
    HI351_write_cmos_sensor(0x63, 0xc0);
    HI351_write_cmos_sensor(0x64, 0x26);//Indoor 0x1326
    HI351_write_cmos_sensor(0x65, 0xe0);
    HI351_write_cmos_sensor(0x66, 0x27);//Indoor 0x1327
    HI351_write_cmos_sensor(0x67, 0x02);
    HI351_write_cmos_sensor(0x68, 0x28);//Indoor 0x1328
    HI351_write_cmos_sensor(0x69, 0x00); //test//0x03);
    HI351_write_cmos_sensor(0x6a, 0x29);//Indoor 0x1329
    HI351_write_cmos_sensor(0x6b, 0x00); //test//0x03);
    HI351_write_cmos_sensor(0x6c, 0x2a);//Indoor 0x132a
    HI351_write_cmos_sensor(0x6d, 0x00); //test//0x03);
    HI351_write_cmos_sensor(0x6e, 0x2b);//Indoor 0x132b
    HI351_write_cmos_sensor(0x6f, 0x00); //test//0x03);
    HI351_write_cmos_sensor(0x70, 0x2c);//Indoor 0x132c
    HI351_write_cmos_sensor(0x71, 0x03);
    HI351_write_cmos_sensor(0x72, 0x2d);//Indoor 0x132d
    HI351_write_cmos_sensor(0x73, 0x03);
    HI351_write_cmos_sensor(0x74, 0x2e);//Indoor 0x132e
    HI351_write_cmos_sensor(0x75, 0x03);
    HI351_write_cmos_sensor(0x76, 0x2f);//Indoor 0x132f
    HI351_write_cmos_sensor(0x77, 0x03);
    HI351_write_cmos_sensor(0x78, 0x30);//Indoor 0x1330
    HI351_write_cmos_sensor(0x79, 0x03);
    HI351_write_cmos_sensor(0x7a, 0x31);//Indoor 0x1331
    HI351_write_cmos_sensor(0x7b, 0x03);
    HI351_write_cmos_sensor(0x7c, 0x32);//Indoor 0x1332
    HI351_write_cmos_sensor(0x7d, 0x03);
    HI351_write_cmos_sensor(0x7e, 0x33);//Indoor 0x1333
    HI351_write_cmos_sensor(0x7f, 0x40);
    HI351_write_cmos_sensor(0x80, 0x34);//Indoor 0x1334
    HI351_write_cmos_sensor(0x81, 0x10); //test//0x80);
    HI351_write_cmos_sensor(0x82, 0x35);//Indoor 0x1335
    HI351_write_cmos_sensor(0x83, 0x00);
    HI351_write_cmos_sensor(0x84, 0x36);//Indoor 0x1336
    HI351_write_cmos_sensor(0x85, 0xf0);
    HI351_write_cmos_sensor(0x86, 0xa0);//Indoor 0x13a0
    HI351_write_cmos_sensor(0x87, 0x0f);
    HI351_write_cmos_sensor(0x88, 0xa8);//Indoor 0x13a8
    HI351_write_cmos_sensor(0x89, 0x10);
    HI351_write_cmos_sensor(0x8a, 0xa9);//Indoor 0x13a9
    HI351_write_cmos_sensor(0x8b, 0x16);
    HI351_write_cmos_sensor(0x8c, 0xaa);//Indoor 0x13aa
    HI351_write_cmos_sensor(0x8d, 0x0a);
    HI351_write_cmos_sensor(0x8e, 0xab);//Indoor 0x13ab
    HI351_write_cmos_sensor(0x8f, 0x02);
    HI351_write_cmos_sensor(0x90, 0xc0);//Indoor 0x13c0
    HI351_write_cmos_sensor(0x91, 0x27);
    HI351_write_cmos_sensor(0x92, 0xc2);//Indoor 0x13c2
    HI351_write_cmos_sensor(0x93, 0x08);
    HI351_write_cmos_sensor(0x94, 0xc3);//Indoor 0x13c3
    HI351_write_cmos_sensor(0x95, 0x08);
    HI351_write_cmos_sensor(0x96, 0xc4);//Indoor 0x13c4
    HI351_write_cmos_sensor(0x97, 0x40);
    HI351_write_cmos_sensor(0x98, 0xc5);//Indoor 0x13c5
    HI351_write_cmos_sensor(0x99, 0x38);
    HI351_write_cmos_sensor(0x9a, 0xc6);//Indoor 0x13c6
    HI351_write_cmos_sensor(0x9b, 0xf0);
    HI351_write_cmos_sensor(0x9c, 0xc7);//Indoor 0x13c7
    HI351_write_cmos_sensor(0x9d, 0x10);
    HI351_write_cmos_sensor(0x9e, 0xc8);//Indoor 0x13c8
    HI351_write_cmos_sensor(0x9f, 0x44);
    HI351_write_cmos_sensor(0xa0, 0xc9);//Indoor 0x13c9
    HI351_write_cmos_sensor(0xa1, 0x87);
    HI351_write_cmos_sensor(0xa2, 0xca);//Indoor 0x13ca
    HI351_write_cmos_sensor(0xa3, 0xff);
    HI351_write_cmos_sensor(0xa4, 0xcb);//Indoor 0x13cb
    HI351_write_cmos_sensor(0xa5, 0x20);
    HI351_write_cmos_sensor(0xa6, 0xcc);//Indoor 0x13cc
    HI351_write_cmos_sensor(0xa7, 0x61);
    HI351_write_cmos_sensor(0xa8, 0xcd);//Indoor 0x13cd
    HI351_write_cmos_sensor(0xa9, 0x87);
    HI351_write_cmos_sensor(0xaa, 0xce);//Indoor 0x13ce
    HI351_write_cmos_sensor(0xab, 0x8a);//07
    HI351_write_cmos_sensor(0xac, 0xcf);//Indoor 0x13cf
    HI351_write_cmos_sensor(0xad, 0xa5);//07
    HI351_write_cmos_sensor(0xae, 0x03);//Indoor Page14
    HI351_write_cmos_sensor(0xaf, 0x14);
    HI351_write_cmos_sensor(0xb0, 0x10);//Indoor 0x1410
    HI351_write_cmos_sensor(0xb1, 0x27);
    HI351_write_cmos_sensor(0xb2, 0x11);//Indoor 0x1411
    HI351_write_cmos_sensor(0xb3, 0x02);
    HI351_write_cmos_sensor(0xb4, 0x12);//Indoor 0x1412
    HI351_write_cmos_sensor(0xb5, 0x40);
    HI351_write_cmos_sensor(0xb6, 0x13);//Indoor 0x1413
    HI351_write_cmos_sensor(0xb7, 0x98);
    HI351_write_cmos_sensor(0xb8, 0x14);//Indoor 0x1414
    HI351_write_cmos_sensor(0xb9, 0x3a);
    HI351_write_cmos_sensor(0xba, 0x15);//Indoor 0x1415
    HI351_write_cmos_sensor(0xbb, 0x24);
    HI351_write_cmos_sensor(0xbc, 0x16);//Indoor 0x1416
    HI351_write_cmos_sensor(0xbd, 0x1a);
    HI351_write_cmos_sensor(0xbe, 0x17);//Indoor 0x1417
    HI351_write_cmos_sensor(0xbf, 0x1a);
    HI351_write_cmos_sensor(0xc0, 0x18);//Indoor 0x1418    Negative High Gain
    HI351_write_cmos_sensor(0xc1, 0x60);//3a
    HI351_write_cmos_sensor(0xc2, 0x19);//Indoor 0x1419    Negative Middle Gain
    HI351_write_cmos_sensor(0xc3, 0x68);//3a
    HI351_write_cmos_sensor(0xc4, 0x1a);//Indoor 0x141a    Negative Low Gain
    HI351_write_cmos_sensor(0xc5, 0x68); //
    HI351_write_cmos_sensor(0xc6, 0x20);//Indoor 0x1420
    HI351_write_cmos_sensor(0xc7, 0x82);  // s_diff L_clip
    HI351_write_cmos_sensor(0xc8, 0x21);//Indoor 0x1421
    HI351_write_cmos_sensor(0xc9, 0x03);
    HI351_write_cmos_sensor(0xca, 0x22);//Indoor 0x1422
    HI351_write_cmos_sensor(0xcb, 0x05);
    HI351_write_cmos_sensor(0xcc, 0x23);//Indoor 0x1423
    HI351_write_cmos_sensor(0xcd, 0x07);
    HI351_write_cmos_sensor(0xce, 0x24);//Indoor 0x1424
    HI351_write_cmos_sensor(0xcf, 0x0a);
    HI351_write_cmos_sensor(0xd0, 0x25);//Indoor 0x1425
    HI351_write_cmos_sensor(0xd1, 0x46); //19
    HI351_write_cmos_sensor(0xd2, 0x26);//Indoor 0x1426
    HI351_write_cmos_sensor(0xd3, 0x32);
    HI351_write_cmos_sensor(0xd4, 0x27);//Indoor 0x1427
    HI351_write_cmos_sensor(0xd5, 0x1e);
    HI351_write_cmos_sensor(0xd6, 0x28);//Indoor 0x1428
    HI351_write_cmos_sensor(0xd7, 0x10);
    HI351_write_cmos_sensor(0xd8, 0x29);//Indoor 0x1429
    HI351_write_cmos_sensor(0xd9, 0x00);
    HI351_write_cmos_sensor(0xda, 0x2a);//Indoor 0x142a
    HI351_write_cmos_sensor(0xdb, 0x18);//40
    HI351_write_cmos_sensor(0xdc, 0x2b);//Indoor 0x142b
    HI351_write_cmos_sensor(0xdd, 0x18);
    HI351_write_cmos_sensor(0xde, 0x2c);//Indoor 0x142c
    HI351_write_cmos_sensor(0xdf, 0x18);
    HI351_write_cmos_sensor(0xe0, 0x2d);//Indoor 0x142d
    HI351_write_cmos_sensor(0xe1, 0x30);
    HI351_write_cmos_sensor(0xe2, 0x2e);//Indoor 0x142e
    HI351_write_cmos_sensor(0xe3, 0x30);
    HI351_write_cmos_sensor(0xe4, 0x2f);//Indoor 0x142f
    HI351_write_cmos_sensor(0xe5, 0x30);
    HI351_write_cmos_sensor(0xe6, 0x30);//Indoor 0x1430
    HI351_write_cmos_sensor(0xe7, 0x87); //test//0x82);   //Ldiff_L_cip
    HI351_write_cmos_sensor(0xe8, 0x31);//Indoor 0x1431
    HI351_write_cmos_sensor(0xe9, 0x02);
    HI351_write_cmos_sensor(0xea, 0x32);//Indoor 0x1432
    HI351_write_cmos_sensor(0xeb, 0x04);
    HI351_write_cmos_sensor(0xec, 0x33);//Indoor 0x1433
    HI351_write_cmos_sensor(0xed, 0x04);
    HI351_write_cmos_sensor(0xee, 0x34);//Indoor 0x1434
    HI351_write_cmos_sensor(0xef, 0x0a);
    HI351_write_cmos_sensor(0xf0, 0x35);//Indoor 0x1435
    HI351_write_cmos_sensor(0xf1, 0x46);//12
    HI351_write_cmos_sensor(0xf2, 0x36);//Indoor 0x1436
    HI351_write_cmos_sensor(0xf3, 0x15); //test//0x32);
    HI351_write_cmos_sensor(0xf4, 0x37);//Indoor 0x1437
    HI351_write_cmos_sensor(0xf5, 0x32);
    HI351_write_cmos_sensor(0xf6, 0x38);//Indoor 0x1438
    HI351_write_cmos_sensor(0xf7, 0x18); //test//0x22);
    HI351_write_cmos_sensor(0xf8, 0x39);//Indoor 0x1439
    HI351_write_cmos_sensor(0xf9, 0x00);
    HI351_write_cmos_sensor(0xfa, 0x3a);//Indoor 0x143a
    HI351_write_cmos_sensor(0xfb, 0x48);
    HI351_write_cmos_sensor(0xfc, 0x3b);//Indoor 0x143b
    HI351_write_cmos_sensor(0xfd, 0x19); //test//0x30);
    HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
    HI351_write_cmos_sensor(0x03, 0xdf);
    HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
    HI351_write_cmos_sensor(0x10, 0x3c);//Indoor 0x143c
    HI351_write_cmos_sensor(0x11, 0x19); //test//0x30);
    HI351_write_cmos_sensor(0x12, 0x3d);//Indoor 0x143d
    HI351_write_cmos_sensor(0x13, 0x20);
    HI351_write_cmos_sensor(0x14, 0x3e);//Indoor 0x143e
    HI351_write_cmos_sensor(0x15, 0x15); //test//0x22);//12
    HI351_write_cmos_sensor(0x16, 0x3f);//Indoor 0x143f
    HI351_write_cmos_sensor(0x17, 0x0d); //test//0x10);
    HI351_write_cmos_sensor(0x18, 0x40);//Indoor 0x1440
    HI351_write_cmos_sensor(0x19, 0x84);
    HI351_write_cmos_sensor(0x1a, 0x41);//Indoor 0x1441
    HI351_write_cmos_sensor(0x1b, 0x10);//20
    HI351_write_cmos_sensor(0x1c, 0x42);//Indoor 0x1442
    HI351_write_cmos_sensor(0x1d, 0xb0);//20
    HI351_write_cmos_sensor(0x1e, 0x43);//Indoor 0x1443
    HI351_write_cmos_sensor(0x1f, 0x40);//20
    HI351_write_cmos_sensor(0x20, 0x44);//Indoor 0x1444
    HI351_write_cmos_sensor(0x21, 0x14);
    HI351_write_cmos_sensor(0x22, 0x45);//Indoor 0x1445
    HI351_write_cmos_sensor(0x23, 0x10);
    HI351_write_cmos_sensor(0x24, 0x46);//Indoor 0x1446
    HI351_write_cmos_sensor(0x25, 0x14);
    HI351_write_cmos_sensor(0x26, 0x47);//Indoor 0x1447
    HI351_write_cmos_sensor(0x27, 0x04);
    HI351_write_cmos_sensor(0x28, 0x48);//Indoor 0x1448
    HI351_write_cmos_sensor(0x29, 0x04);
    HI351_write_cmos_sensor(0x2a, 0x49);//Indoor 0x1449
    HI351_write_cmos_sensor(0x2b, 0x04);
    HI351_write_cmos_sensor(0x2c, 0x50);//Indoor 0x1450
    HI351_write_cmos_sensor(0x2d, 0x84);//19
    HI351_write_cmos_sensor(0x2e, 0x51);//Indoor 0x1451
    HI351_write_cmos_sensor(0x2f, 0x30);//60
    HI351_write_cmos_sensor(0x30, 0x52);//Indoor 0x1452
    HI351_write_cmos_sensor(0x31, 0xb0);
    HI351_write_cmos_sensor(0x32, 0x53);//Indoor 0x1453
    HI351_write_cmos_sensor(0x33, 0x37);//58
    HI351_write_cmos_sensor(0x34, 0x54);//Indoor 0x1454
    HI351_write_cmos_sensor(0x35, 0x44);
    HI351_write_cmos_sensor(0x36, 0x55);//Indoor 0x1455
    HI351_write_cmos_sensor(0x37, 0x44);
    HI351_write_cmos_sensor(0x38, 0x56);//Indoor 0x1456
    HI351_write_cmos_sensor(0x39, 0x44);
    HI351_write_cmos_sensor(0x3a, 0x57);//Indoor 0x1457
    HI351_write_cmos_sensor(0x3b, 0x10);//03
    HI351_write_cmos_sensor(0x3c, 0x58);//Indoor 0x1458
    HI351_write_cmos_sensor(0x3d, 0x14);
    HI351_write_cmos_sensor(0x3e, 0x59);//Indoor 0x1459
    HI351_write_cmos_sensor(0x3f, 0x14);
    HI351_write_cmos_sensor(0x40, 0x60);//Indoor 0x1460
    HI351_write_cmos_sensor(0x41, 0x02);
    HI351_write_cmos_sensor(0x42, 0x61);//Indoor 0x1461
    HI351_write_cmos_sensor(0x43, 0xa0);
    HI351_write_cmos_sensor(0x44, 0x62);//Indoor 0x1462
    HI351_write_cmos_sensor(0x45, 0x98);
    HI351_write_cmos_sensor(0x46, 0x63);//Indoor 0x1463
    HI351_write_cmos_sensor(0x47, 0xe4);
    HI351_write_cmos_sensor(0x48, 0x64);//Indoor 0x1464
    HI351_write_cmos_sensor(0x49, 0xa4);
    HI351_write_cmos_sensor(0x4a, 0x65);//Indoor 0x1465
    HI351_write_cmos_sensor(0x4b, 0x7d);
    HI351_write_cmos_sensor(0x4c, 0x66);//Indoor 0x1466
    HI351_write_cmos_sensor(0x4d, 0x4b);
    HI351_write_cmos_sensor(0x4e, 0x70);//Indoor 0x1470
    HI351_write_cmos_sensor(0x4f, 0x10);
    HI351_write_cmos_sensor(0x50, 0x71);//Indoor 0x1471
    HI351_write_cmos_sensor(0x51, 0x10);
    HI351_write_cmos_sensor(0x52, 0x72);//Indoor 0x1472
    HI351_write_cmos_sensor(0x53, 0x10);
    HI351_write_cmos_sensor(0x54, 0x73);//Indoor 0x1473
    HI351_write_cmos_sensor(0x55, 0x10);
    HI351_write_cmos_sensor(0x56, 0x74);//Indoor 0x1474
    HI351_write_cmos_sensor(0x57, 0x10);
    HI351_write_cmos_sensor(0x58, 0x75);//Indoor 0x1475
    HI351_write_cmos_sensor(0x59, 0x10);
    HI351_write_cmos_sensor(0x5a, 0x76);//Indoor 0x1476      //green sharp pos High
    HI351_write_cmos_sensor(0x5b, 0x10);
    HI351_write_cmos_sensor(0x5c, 0x77);//Indoor 0x1477      //green sharp pos Middle
    HI351_write_cmos_sensor(0x5d, 0x20);
    HI351_write_cmos_sensor(0x5e, 0x78);//Indoor 0x1478      //green sharp pos Low
    HI351_write_cmos_sensor(0x5f, 0x18);
    HI351_write_cmos_sensor(0x60, 0x79);//Indoor 0x1479       //green sharp nega High
    HI351_write_cmos_sensor(0x61, 0x60);
    HI351_write_cmos_sensor(0x62, 0x7a);//Indoor 0x147a       //green sharp nega Middle
    HI351_write_cmos_sensor(0x63, 0x60);
    HI351_write_cmos_sensor(0x64, 0x7b);//Indoor 0x147b       //green sharp nega Low
    HI351_write_cmos_sensor(0x65, 0x60);//Indoor 0x147b       //green sharp nega Low
HI351_write_cmos_sensor(0x66, 0x03);
HI351_write_cmos_sensor(0x67, 0x10); //10 page
HI351_write_cmos_sensor(0x68, 0x60);
HI351_write_cmos_sensor(0x69, 0x03); //1060
HI351_write_cmos_sensor(0x6a, 0x70);
HI351_write_cmos_sensor(0x6b, 0x0c); //1070
HI351_write_cmos_sensor(0x6c, 0x71);
HI351_write_cmos_sensor(0x6d, 0x07); //1071
HI351_write_cmos_sensor(0x6e, 0x72);
HI351_write_cmos_sensor(0x6f, 0x3a); //1072
HI351_write_cmos_sensor(0x70, 0x73);
HI351_write_cmos_sensor(0x71, 0x99); //1073
HI351_write_cmos_sensor(0x72, 0x74);
HI351_write_cmos_sensor(0x73, 0x1B); //1074
HI351_write_cmos_sensor(0x74, 0x75);
HI351_write_cmos_sensor(0x75, 0x0b); //1075
HI351_write_cmos_sensor(0x76, 0x76);
HI351_write_cmos_sensor(0x77, 0x06); //1076
HI351_write_cmos_sensor(0x78, 0x77);
HI351_write_cmos_sensor(0x79, 0x33); //1077
HI351_write_cmos_sensor(0x7a, 0x78);
HI351_write_cmos_sensor(0x7b, 0x33); //1078
HI351_write_cmos_sensor(0x7c, 0x79);
HI351_write_cmos_sensor(0x7d, 0x4C); //1079
HI351_write_cmos_sensor(0x7e, 0x7a);
HI351_write_cmos_sensor(0x7f, 0xCC); //107a
HI351_write_cmos_sensor(0x80, 0x7b);
HI351_write_cmos_sensor(0x81, 0x49); //107b
HI351_write_cmos_sensor(0x82, 0x7c);
HI351_write_cmos_sensor(0x83, 0x99); //107c
HI351_write_cmos_sensor(0x84, 0x7d);
HI351_write_cmos_sensor(0x85, 0x0e); //107d
HI351_write_cmos_sensor(0x86, 0x7e);
HI351_write_cmos_sensor(0x87, 0x1e); //107e
HI351_write_cmos_sensor(0x88, 0x7f);
HI351_write_cmos_sensor(0x89, 0x3c); //107f
HI351_write_cmos_sensor(0x8a, 0x03); //16 page
HI351_write_cmos_sensor(0x8b, 0x16);
HI351_write_cmos_sensor(0x8c, 0x8a); //168a	SP1
HI351_write_cmos_sensor(0x8d, 0x68);
HI351_write_cmos_sensor(0x8e, 0x8b); //168b  SP2
HI351_write_cmos_sensor(0x8f, 0x70);
HI351_write_cmos_sensor(0x90, 0x8c); //168c	SP3
HI351_write_cmos_sensor(0x91, 0x79);
HI351_write_cmos_sensor(0x92, 0x8d); //168d	SP4
HI351_write_cmos_sensor(0x93, 0x7a);
HI351_write_cmos_sensor(0x94, 0x8e); //168e	SP5
HI351_write_cmos_sensor(0x95, 0x7f);
HI351_write_cmos_sensor(0x96, 0x8f); //168f	SP6
HI351_write_cmos_sensor(0x97, 0x7f);
HI351_write_cmos_sensor(0x98, 0x90); //1690	SP7
HI351_write_cmos_sensor(0x99, 0x7f);
HI351_write_cmos_sensor(0x9a, 0x91); //1691	SP8
HI351_write_cmos_sensor(0x9b, 0x7f);
HI351_write_cmos_sensor(0x9c, 0x92); //1692	SP9
HI351_write_cmos_sensor(0x9d, 0x7f);

HI351_write_cmos_sensor(0x0e, 0x00); //burst_end

////////////////////
// e0 Page (DMA Dark1)
////////////////////
HI351_write_cmos_sensor(0x03, 0xe0);
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x03);
HI351_write_cmos_sensor(0x11, 0x11);
HI351_write_cmos_sensor(0x12, 0x10);
HI351_write_cmos_sensor(0x13, 0x13); //Dark1 1110
HI351_write_cmos_sensor(0x14, 0x11);
HI351_write_cmos_sensor(0x15, 0x08); //Dark1 1111
HI351_write_cmos_sensor(0x16, 0x12);
HI351_write_cmos_sensor(0x17, 0x1e); //Dark1 1112
HI351_write_cmos_sensor(0x18, 0x13);
HI351_write_cmos_sensor(0x19, 0x0e); //Dark1 1113
HI351_write_cmos_sensor(0x1a, 0x14);
HI351_write_cmos_sensor(0x1b, 0x31); //Dark1 1114
HI351_write_cmos_sensor(0x1c, 0x30);
HI351_write_cmos_sensor(0x1d, 0x20); //Dark1 1130
HI351_write_cmos_sensor(0x1e, 0x31);
HI351_write_cmos_sensor(0x1f, 0x20); //Dark1 1131
HI351_write_cmos_sensor(0x20, 0x32);
HI351_write_cmos_sensor(0x21, 0x52); //Dark1 1132
HI351_write_cmos_sensor(0x22, 0x33);
HI351_write_cmos_sensor(0x23, 0x3b); //Dark1 1133
HI351_write_cmos_sensor(0x24, 0x34);
HI351_write_cmos_sensor(0x25, 0x1d); //Dark1 1134
HI351_write_cmos_sensor(0x26, 0x35);
HI351_write_cmos_sensor(0x27, 0x21); //Dark1 1135
HI351_write_cmos_sensor(0x28, 0x36);
HI351_write_cmos_sensor(0x29, 0x1b); //Dark1 1136
HI351_write_cmos_sensor(0x2a, 0x37);
HI351_write_cmos_sensor(0x2b, 0x21); //Dark1 1137
HI351_write_cmos_sensor(0x2c, 0x38);
HI351_write_cmos_sensor(0x2d, 0x18); //Dark1 1138
HI351_write_cmos_sensor(0x2e, 0x39);
HI351_write_cmos_sensor(0x2f, 0x18); //Dark1 1139 R2 lvl1 gain
HI351_write_cmos_sensor(0x30, 0x3a);
HI351_write_cmos_sensor(0x31, 0x20); //Dark1 113a R2 lvl2 gain
HI351_write_cmos_sensor(0x32, 0x3b);
HI351_write_cmos_sensor(0x33, 0x28); //Dark1 113b R2 lvl3 gain
HI351_write_cmos_sensor(0x34, 0x3c);
HI351_write_cmos_sensor(0x35, 0x20); //Dark1 113c R2 lvl4 gain
HI351_write_cmos_sensor(0x36, 0x3d);
HI351_write_cmos_sensor(0x37, 0x20); //Dark1 113d R2 lvl5 gain
HI351_write_cmos_sensor(0x38, 0x3e);
HI351_write_cmos_sensor(0x39, 0x1e); //Dark1 113e R2 lvl6 gain
HI351_write_cmos_sensor(0x3a, 0x3f);
HI351_write_cmos_sensor(0x3b, 0x1e); //Dark1 113f R2 lvl7 gain
HI351_write_cmos_sensor(0x3c, 0x40);
HI351_write_cmos_sensor(0x3d, 0x1e); //Dark1 1140 R2 lvl8 gain
HI351_write_cmos_sensor(0x3e, 0x41);
HI351_write_cmos_sensor(0x3f, 0x10); //Dark1 1141 R2 Lvl1 offset
HI351_write_cmos_sensor(0x40, 0x42);
HI351_write_cmos_sensor(0x41, 0x10); //Dark1 1142 R2 Lvl2 offset
HI351_write_cmos_sensor(0x42, 0x43);
HI351_write_cmos_sensor(0x43, 0x20); //Dark1 1143 R2 Lvl3 offset
HI351_write_cmos_sensor(0x44, 0x44);
HI351_write_cmos_sensor(0x45, 0x2a); //Dark1 1144 R2 Lvl4 offset
HI351_write_cmos_sensor(0x46, 0x45);
HI351_write_cmos_sensor(0x47, 0x30); //Dark1 1145 R2 Lvl5 offset
HI351_write_cmos_sensor(0x48, 0x46);
HI351_write_cmos_sensor(0x49, 0x50); //Dark1 1146 R2 Lvl6 offset
HI351_write_cmos_sensor(0x4a, 0x47);
HI351_write_cmos_sensor(0x4b, 0x40); //Dark1 1147 R2 Lvl7 offset
HI351_write_cmos_sensor(0x4c, 0x48);
HI351_write_cmos_sensor(0x4d, 0x40); //Dark1 1148 R2 Lvl8 offset
HI351_write_cmos_sensor(0x4e, 0x49);
HI351_write_cmos_sensor(0x4f, 0x60); //Dark1 1149 Lv1 h_clip
HI351_write_cmos_sensor(0x50, 0x4a);
HI351_write_cmos_sensor(0x51, 0x80); //Dark1 114a Lv2 h_clip
HI351_write_cmos_sensor(0x52, 0x4b);
HI351_write_cmos_sensor(0x53, 0xa0); //Dark1 114b Lv3 h_clip
HI351_write_cmos_sensor(0x54, 0x4c);
HI351_write_cmos_sensor(0x55, 0xa0); //Dark1 114c Lv4 h_clip
HI351_write_cmos_sensor(0x56, 0x4d);
HI351_write_cmos_sensor(0x57, 0xa0); //Dark1 114d Lv5 h_clip
HI351_write_cmos_sensor(0x58, 0x4e);
HI351_write_cmos_sensor(0x59, 0x90); //Dark1 114e Lv6 h_clip
HI351_write_cmos_sensor(0x5a, 0x4f);
HI351_write_cmos_sensor(0x5b, 0x90); //Dark1 114f Lv7 h_clip
HI351_write_cmos_sensor(0x5c, 0x50);
HI351_write_cmos_sensor(0x5d, 0x90); //Dark1 1150 Lv8 h_clip
HI351_write_cmos_sensor(0x5e, 0x51);
HI351_write_cmos_sensor(0x5f, 0x68); //Dark1 1151 color gain start
HI351_write_cmos_sensor(0x60, 0x52);
HI351_write_cmos_sensor(0x61, 0x68); //Dark1 1152
HI351_write_cmos_sensor(0x62, 0x53);
HI351_write_cmos_sensor(0x63, 0x68); //Dark1 1153
HI351_write_cmos_sensor(0x64, 0x54);
HI351_write_cmos_sensor(0x65, 0x68); //Dark1 1154
HI351_write_cmos_sensor(0x66, 0x55);
HI351_write_cmos_sensor(0x67, 0x68); //Dark1 1155
HI351_write_cmos_sensor(0x68, 0x56);
HI351_write_cmos_sensor(0x69, 0x68); //Dark1 1156
HI351_write_cmos_sensor(0x6a, 0x57);
HI351_write_cmos_sensor(0x6b, 0x68); //Dark1 1157
HI351_write_cmos_sensor(0x6c, 0x58);
HI351_write_cmos_sensor(0x6d, 0x68); //Dark1 1158 color gain end
HI351_write_cmos_sensor(0x6e, 0x59);
HI351_write_cmos_sensor(0x6f, 0x70); //Dark1 1159 color ofs lmt start
HI351_write_cmos_sensor(0x70, 0x5a);
HI351_write_cmos_sensor(0x71, 0x70); //Dark1 115a
HI351_write_cmos_sensor(0x72, 0x5b);
HI351_write_cmos_sensor(0x73, 0x70); //Dark1 115b
HI351_write_cmos_sensor(0x74, 0x5c);
HI351_write_cmos_sensor(0x75, 0x70); //Dark1 115c
HI351_write_cmos_sensor(0x76, 0x5d);
HI351_write_cmos_sensor(0x77, 0x70); //Dark1 115d
HI351_write_cmos_sensor(0x78, 0x5e);
HI351_write_cmos_sensor(0x79, 0x70); //Dark1 115e
HI351_write_cmos_sensor(0x7a, 0x5f);
HI351_write_cmos_sensor(0x7b, 0x70); //Dark1 115f
HI351_write_cmos_sensor(0x7c, 0x60);
HI351_write_cmos_sensor(0x7d, 0x70); //Dark1 1160 color ofs lmt end
HI351_write_cmos_sensor(0x7e, 0x61);
HI351_write_cmos_sensor(0x7f, 0xc0); //Dark1 1161
HI351_write_cmos_sensor(0x80, 0x62);
HI351_write_cmos_sensor(0x81, 0xf0); //Dark1 1162
HI351_write_cmos_sensor(0x82, 0x63);
HI351_write_cmos_sensor(0x83, 0x80); //Dark1 1163
HI351_write_cmos_sensor(0x84, 0x64);
HI351_write_cmos_sensor(0x85, 0x40); //Dark1 1164
HI351_write_cmos_sensor(0x86, 0x65);
HI351_write_cmos_sensor(0x87, 0x40); //Dark1 1165
HI351_write_cmos_sensor(0x88, 0x66);
HI351_write_cmos_sensor(0x89, 0x40); //Dark1 1166
HI351_write_cmos_sensor(0x8a, 0x67);
HI351_write_cmos_sensor(0x8b, 0x40); //Dark1 1167
HI351_write_cmos_sensor(0x8c, 0x68);
HI351_write_cmos_sensor(0x8d, 0x80); //Dark1 1168
HI351_write_cmos_sensor(0x8e, 0x69);
HI351_write_cmos_sensor(0x8f, 0x40); //Dark1 1169
HI351_write_cmos_sensor(0x90, 0x6a);
HI351_write_cmos_sensor(0x91, 0x40); //Dark1 116a Imp Lv2 High Gain
HI351_write_cmos_sensor(0x92, 0x6b);
HI351_write_cmos_sensor(0x93, 0x40); //Dark1 116b Imp Lv2 Middle Gain
HI351_write_cmos_sensor(0x94, 0x6c);
HI351_write_cmos_sensor(0x95, 0x40); //Dark1 116c Imp Lv2 Low Gain
HI351_write_cmos_sensor(0x96, 0x6d);
HI351_write_cmos_sensor(0x97, 0x80); //Dark1 116d
HI351_write_cmos_sensor(0x98, 0x6e);
HI351_write_cmos_sensor(0x99, 0x40); //Dark1 116e
HI351_write_cmos_sensor(0x9a, 0x6f);
HI351_write_cmos_sensor(0x9b, 0x50); //Dark1 116f Imp Lv3 Hi Gain
HI351_write_cmos_sensor(0x9c, 0x70);
HI351_write_cmos_sensor(0x9d, 0x50); //Dark1 1170 Imp Lv3 Middle Gain
HI351_write_cmos_sensor(0x9e, 0x71);
HI351_write_cmos_sensor(0x9f, 0x50); //Dark1 1171 Imp Lv3 Low Gain
HI351_write_cmos_sensor(0xa0, 0x72);
HI351_write_cmos_sensor(0xa1, 0x6e); //Dark1 1172
HI351_write_cmos_sensor(0xa2, 0x73);
HI351_write_cmos_sensor(0xa3, 0x3a); //Dark1 1173
HI351_write_cmos_sensor(0xa4, 0x74);
HI351_write_cmos_sensor(0xa5, 0x50); //Dark1 1174 Imp Lv4 Hi Gain
HI351_write_cmos_sensor(0xa6, 0x75);
HI351_write_cmos_sensor(0xa7, 0x50); //Dark1 1175 Imp Lv4 Middle Gain
HI351_write_cmos_sensor(0xa8, 0x76);
HI351_write_cmos_sensor(0xa9, 0x50); //Dark1 1176 Imp Lv4 Low Gain
HI351_write_cmos_sensor(0xaa, 0x77);
HI351_write_cmos_sensor(0xab, 0x6e); //Dark1 1177 Imp Lv5 Hi Th
HI351_write_cmos_sensor(0xac, 0x78);
HI351_write_cmos_sensor(0xad, 0x66); //Dark1 1178 Imp Lv5 Middle Th
HI351_write_cmos_sensor(0xae, 0x79);
HI351_write_cmos_sensor(0xaf, 0x40); //Dark1 1179 Imp Lv5 Hi Gain
HI351_write_cmos_sensor(0xb0, 0x7a);
HI351_write_cmos_sensor(0xb1, 0x40); //Dark1 117a Imp Lv5 Middle Gain
HI351_write_cmos_sensor(0xb2, 0x7b);
HI351_write_cmos_sensor(0xb3, 0x40); //Dark1 117b Imp Lv5 Low Gain
HI351_write_cmos_sensor(0xb4, 0x7c);
HI351_write_cmos_sensor(0xb5, 0x5c); //Dark1 117c Imp Lv6 Hi Th
HI351_write_cmos_sensor(0xb6, 0x7d);
HI351_write_cmos_sensor(0xb7, 0x30); //Dark1 117d Imp Lv6 Middle Th
HI351_write_cmos_sensor(0xb8, 0x7e);
HI351_write_cmos_sensor(0xb9, 0x40); //Dark1 117e Imp Lv6 Hi Gain
HI351_write_cmos_sensor(0xba, 0x7f);
HI351_write_cmos_sensor(0xbb, 0x40); //Dark1 117f Imp Lv6 Middle Gain
HI351_write_cmos_sensor(0xbc, 0x80);
HI351_write_cmos_sensor(0xbd, 0x40); //Dark1 1180 Imp Lv6 Low Gain
HI351_write_cmos_sensor(0xbe, 0x81);
HI351_write_cmos_sensor(0xbf, 0x62); //Dark1 1181
HI351_write_cmos_sensor(0xc0, 0x82);
HI351_write_cmos_sensor(0xc1, 0x26); //Dark1 1182
HI351_write_cmos_sensor(0xc2, 0x83);
HI351_write_cmos_sensor(0xc3, 0x40); //Dark1 1183 Imp Lv7 Hi Gain
HI351_write_cmos_sensor(0xc4, 0x84);
HI351_write_cmos_sensor(0xc5, 0x40); //Dark1 1184 Imp Lv7 Middle Gain
HI351_write_cmos_sensor(0xc6, 0x85);
HI351_write_cmos_sensor(0xc7, 0x40); //Dark1 1185 Imp Lv7 Low Gain
HI351_write_cmos_sensor(0xc8, 0x86);
HI351_write_cmos_sensor(0xc9, 0x62); //Dark1 1186
HI351_write_cmos_sensor(0xca, 0x87);
HI351_write_cmos_sensor(0xcb, 0x26); //Dark1 1187
HI351_write_cmos_sensor(0xcc, 0x88);
HI351_write_cmos_sensor(0xcd, 0x30); //Dark1 1188
HI351_write_cmos_sensor(0xce, 0x89);
HI351_write_cmos_sensor(0xcf, 0x30); //Dark1 1189
HI351_write_cmos_sensor(0xd0, 0x8a);
HI351_write_cmos_sensor(0xd1, 0x30); //Dark1 118a
HI351_write_cmos_sensor(0xd2, 0x90);
HI351_write_cmos_sensor(0xd3, 0x00); //Dark1 1190
HI351_write_cmos_sensor(0xd4, 0x91);
HI351_write_cmos_sensor(0xd5, 0x4e); //Dark1 1191
HI351_write_cmos_sensor(0xd6, 0x92);
HI351_write_cmos_sensor(0xd7, 0x00); //Dark1 1192
HI351_write_cmos_sensor(0xd8, 0x93);
HI351_write_cmos_sensor(0xd9, 0x16); //Dark1 1193
HI351_write_cmos_sensor(0xda, 0x94);
HI351_write_cmos_sensor(0xdb, 0x01); //Dark1 1194
HI351_write_cmos_sensor(0xdc, 0x95);
HI351_write_cmos_sensor(0xdd, 0x80); //Dark1 1195
HI351_write_cmos_sensor(0xde, 0x96);
HI351_write_cmos_sensor(0xdf, 0x55); //Dark1 1196
HI351_write_cmos_sensor(0xe0, 0x97);
HI351_write_cmos_sensor(0xe1, 0x8d); //Dark1 1197
HI351_write_cmos_sensor(0xe2, 0xb0);
HI351_write_cmos_sensor(0xe3, 0x60); //Dark1 11b0
HI351_write_cmos_sensor(0xe4, 0xb1);
HI351_write_cmos_sensor(0xe5, 0xb0); //Dark1 11b1
HI351_write_cmos_sensor(0xe6, 0xb2);
HI351_write_cmos_sensor(0xe7, 0x88); //Dark1 11b2
HI351_write_cmos_sensor(0xe8, 0xb3);
HI351_write_cmos_sensor(0xe9, 0x10); //Dark1 11b3
HI351_write_cmos_sensor(0xea, 0xb4);
HI351_write_cmos_sensor(0xeb, 0x04); //Dark1 11b4
HI351_write_cmos_sensor(0xec, 0x03);
HI351_write_cmos_sensor(0xed, 0x12);
HI351_write_cmos_sensor(0xee, 0x10);
HI351_write_cmos_sensor(0xef, 0x03); //Dark1 1210
HI351_write_cmos_sensor(0xf0, 0x11);
HI351_write_cmos_sensor(0xf1, 0x29); //Dark1 1211
HI351_write_cmos_sensor(0xf2, 0x12);
HI351_write_cmos_sensor(0xf3, 0x04); //Dark1 1212
HI351_write_cmos_sensor(0xf4, 0x40);
HI351_write_cmos_sensor(0xf5, 0x33); //Dark1 1240
HI351_write_cmos_sensor(0xf6, 0x41);
HI351_write_cmos_sensor(0xf7, 0x0a); //Dark1 1241
HI351_write_cmos_sensor(0xf8, 0x42);
HI351_write_cmos_sensor(0xf9, 0x6a); //Dark1 1242
HI351_write_cmos_sensor(0xfa, 0x43);
HI351_write_cmos_sensor(0xfb, 0x80); //Dark1 1243
HI351_write_cmos_sensor(0xfc, 0x44);
HI351_write_cmos_sensor(0xfd, 0x02); //Dark1 1244
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
HI351_write_cmos_sensor(0x03, 0xe1);
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x45);
HI351_write_cmos_sensor(0x11, 0x0a); //Dark1 1245
HI351_write_cmos_sensor(0x12, 0x46);
HI351_write_cmos_sensor(0x13, 0x80); //Dark1 1246
HI351_write_cmos_sensor(0x14, 0x60);
HI351_write_cmos_sensor(0x15, 0x21); //Dark1 1260
HI351_write_cmos_sensor(0x16, 0x61);
HI351_write_cmos_sensor(0x17, 0x0e); //Dark1 1261
HI351_write_cmos_sensor(0x18, 0x62);
HI351_write_cmos_sensor(0x19, 0x70); //Dark1 1262
HI351_write_cmos_sensor(0x1a, 0x63);
HI351_write_cmos_sensor(0x1b, 0x70); //Dark1 1263
HI351_write_cmos_sensor(0x1c, 0x64);
HI351_write_cmos_sensor(0x1d, 0x14); //Dark1 1264
HI351_write_cmos_sensor(0x1e, 0x65);
HI351_write_cmos_sensor(0x1f, 0x01); //Dark1 1265
HI351_write_cmos_sensor(0x20, 0x68);
HI351_write_cmos_sensor(0x21, 0x0a); //Dark1 1268
HI351_write_cmos_sensor(0x22, 0x69);
HI351_write_cmos_sensor(0x23, 0x04); //Dark1 1269
HI351_write_cmos_sensor(0x24, 0x6a);
HI351_write_cmos_sensor(0x25, 0x0a); //Dark1 126a
HI351_write_cmos_sensor(0x26, 0x6b);
HI351_write_cmos_sensor(0x27, 0x0a); //Dark1 126b
HI351_write_cmos_sensor(0x28, 0x6c);
HI351_write_cmos_sensor(0x29, 0x24); //Dark1 126c
HI351_write_cmos_sensor(0x2a, 0x6d);
HI351_write_cmos_sensor(0x2b, 0x01); //Dark1 126d
HI351_write_cmos_sensor(0x2c, 0x70);
HI351_write_cmos_sensor(0x2d, 0x01); //Dark1 1270
HI351_write_cmos_sensor(0x2e, 0x71);
HI351_write_cmos_sensor(0x2f, 0x3d); //Dark1 1271
HI351_write_cmos_sensor(0x30, 0x80);
HI351_write_cmos_sensor(0x31, 0x80); //Dark1 1280
HI351_write_cmos_sensor(0x32, 0x81);
HI351_write_cmos_sensor(0x33, 0x88); //Dark1 1281
HI351_write_cmos_sensor(0x34, 0x82);
HI351_write_cmos_sensor(0x35, 0x08); //Dark1 1282
HI351_write_cmos_sensor(0x36, 0x83);
HI351_write_cmos_sensor(0x37, 0x0c); //Dark1 1283
HI351_write_cmos_sensor(0x38, 0x84);
HI351_write_cmos_sensor(0x39, 0x90); //Dark1 1284
HI351_write_cmos_sensor(0x3a, 0x85);
HI351_write_cmos_sensor(0x3b, 0x92); //Dark1 1285
HI351_write_cmos_sensor(0x3c, 0x86);
HI351_write_cmos_sensor(0x3d, 0x20); //Dark1 1286
HI351_write_cmos_sensor(0x3e, 0x87);
HI351_write_cmos_sensor(0x3f, 0x00); //Dark1 1287
HI351_write_cmos_sensor(0x40, 0x88);
HI351_write_cmos_sensor(0x41, 0x70); //Dark1 1288
HI351_write_cmos_sensor(0x42, 0x89);
HI351_write_cmos_sensor(0x43, 0xaa); //Dark1 1289
HI351_write_cmos_sensor(0x44, 0x8a);
HI351_write_cmos_sensor(0x45, 0x50); //Dark1 128a
HI351_write_cmos_sensor(0x46, 0x8b);
HI351_write_cmos_sensor(0x47, 0x10); //Dark1 128b
HI351_write_cmos_sensor(0x48, 0x8c);
HI351_write_cmos_sensor(0x49, 0x04); //Dark1 128c
HI351_write_cmos_sensor(0x4a, 0x8d);
HI351_write_cmos_sensor(0x4b, 0x02); //Dark1 128d
HI351_write_cmos_sensor(0x4c, 0xe6);
HI351_write_cmos_sensor(0x4d, 0xff); //Dark1 12e6
HI351_write_cmos_sensor(0x4e, 0xe7);
HI351_write_cmos_sensor(0x4f, 0x18); //Dark1 12e7
HI351_write_cmos_sensor(0x50, 0xe8);
HI351_write_cmos_sensor(0x51, 0x20); //Dark1 12e8
HI351_write_cmos_sensor(0x52, 0xe9);
HI351_write_cmos_sensor(0x53, 0x06); //Dark1 12e9
HI351_write_cmos_sensor(0x54, 0x03);
HI351_write_cmos_sensor(0x55, 0x13);
HI351_write_cmos_sensor(0x56, 0x10);
HI351_write_cmos_sensor(0x57, 0x31); //Dark1 1310
HI351_write_cmos_sensor(0x58, 0x20);
HI351_write_cmos_sensor(0x59, 0x20); //Dark1 1320
HI351_write_cmos_sensor(0x5a, 0x21);
HI351_write_cmos_sensor(0x5b, 0x30); //Dark1 1321
HI351_write_cmos_sensor(0x5c, 0x22);
HI351_write_cmos_sensor(0x5d, 0x36); //Dark1 1322
HI351_write_cmos_sensor(0x5e, 0x23);
HI351_write_cmos_sensor(0x5f, 0x6a); //Dark1 1323
HI351_write_cmos_sensor(0x60, 0x24);
HI351_write_cmos_sensor(0x61, 0xa0); //Dark1 1324
HI351_write_cmos_sensor(0x62, 0x25);
HI351_write_cmos_sensor(0x63, 0xc0); //Dark1 1325
HI351_write_cmos_sensor(0x64, 0x26);
HI351_write_cmos_sensor(0x65, 0xe0); //Dark1 1326
HI351_write_cmos_sensor(0x66, 0x27);
HI351_write_cmos_sensor(0x67, 0x02); //Dark1 1327
HI351_write_cmos_sensor(0x68, 0x28);
HI351_write_cmos_sensor(0x69, 0x03); //Dark1 1328
HI351_write_cmos_sensor(0x6a, 0x29);
HI351_write_cmos_sensor(0x6b, 0x03); //Dark1 1329
HI351_write_cmos_sensor(0x6c, 0x2a);
HI351_write_cmos_sensor(0x6d, 0x02); //Dark1 132a
HI351_write_cmos_sensor(0x6e, 0x2b);
HI351_write_cmos_sensor(0x6f, 0x04); //Dark1 132b
HI351_write_cmos_sensor(0x70, 0x2c);
HI351_write_cmos_sensor(0x71, 0x04); //Dark1 132c
HI351_write_cmos_sensor(0x72, 0x2d);
HI351_write_cmos_sensor(0x73, 0x03); //Dark1 132d
HI351_write_cmos_sensor(0x74, 0x2e);
HI351_write_cmos_sensor(0x75, 0x03); //Dark1 132e
HI351_write_cmos_sensor(0x76, 0x2f);
HI351_write_cmos_sensor(0x77, 0x14); //Dark1 132f
HI351_write_cmos_sensor(0x78, 0x30);
HI351_write_cmos_sensor(0x79, 0x03); //Dark1 1330
HI351_write_cmos_sensor(0x7a, 0x31);
HI351_write_cmos_sensor(0x7b, 0x03); //Dark1 1331
HI351_write_cmos_sensor(0x7c, 0x32);
HI351_write_cmos_sensor(0x7d, 0x03); //Dark1 1332
HI351_write_cmos_sensor(0x7e, 0x33);
HI351_write_cmos_sensor(0x7f, 0x40); //Dark1 1333
HI351_write_cmos_sensor(0x80, 0x34);
HI351_write_cmos_sensor(0x81, 0x80); //Dark1 1334
HI351_write_cmos_sensor(0x82, 0x35);
HI351_write_cmos_sensor(0x83, 0x00); //Dark1 1335
HI351_write_cmos_sensor(0x84, 0x36);
HI351_write_cmos_sensor(0x85, 0xf0); //Dark1 1336
HI351_write_cmos_sensor(0x86, 0xa0);
HI351_write_cmos_sensor(0x87, 0x07); //Dark1 13a0
HI351_write_cmos_sensor(0x88, 0xa8);
HI351_write_cmos_sensor(0x89, 0x24); //Dark1 13a8 Cb_Filter
HI351_write_cmos_sensor(0x8a, 0xa9);
HI351_write_cmos_sensor(0x8b, 0x24); //Dark1 13a9 Cr_Filter
HI351_write_cmos_sensor(0x8c, 0xaa);
HI351_write_cmos_sensor(0x8d, 0x20); //Dark1 13aa
HI351_write_cmos_sensor(0x8e, 0xab);
HI351_write_cmos_sensor(0x8f, 0x02); //Dark1 13ab
HI351_write_cmos_sensor(0x90, 0xc0);
HI351_write_cmos_sensor(0x91, 0x27); //Dark1 13c0
HI351_write_cmos_sensor(0x92, 0xc2);
HI351_write_cmos_sensor(0x93, 0x08); //Dark1 13c2
HI351_write_cmos_sensor(0x94, 0xc3);
HI351_write_cmos_sensor(0x95, 0x08); //Dark1 13c3
HI351_write_cmos_sensor(0x96, 0xc4);
HI351_write_cmos_sensor(0x97, 0x40); //Dark1 13c4
HI351_write_cmos_sensor(0x98, 0xc5);
HI351_write_cmos_sensor(0x99, 0x38); //Dark1 13c5
HI351_write_cmos_sensor(0x9a, 0xc6);
HI351_write_cmos_sensor(0x9b, 0xf0); //Dark1 13c6
HI351_write_cmos_sensor(0x9c, 0xc7);
HI351_write_cmos_sensor(0x9d, 0x10); //Dark1 13c7
HI351_write_cmos_sensor(0x9e, 0xc8);
HI351_write_cmos_sensor(0x9f, 0x44); //Dark1 13c8
HI351_write_cmos_sensor(0xa0, 0xc9);
HI351_write_cmos_sensor(0xa1, 0x87); //Dark1 13c9
HI351_write_cmos_sensor(0xa2, 0xca);
HI351_write_cmos_sensor(0xa3, 0xff); //Dark1 13ca
HI351_write_cmos_sensor(0xa4, 0xcb);
HI351_write_cmos_sensor(0xa5, 0x20); //Dark1 13cb
HI351_write_cmos_sensor(0xa6, 0xcc);
HI351_write_cmos_sensor(0xa7, 0x61); //Dark1 13cc
HI351_write_cmos_sensor(0xa8, 0xcd);
HI351_write_cmos_sensor(0xa9, 0x87); //Dark1 13cd
HI351_write_cmos_sensor(0xaa, 0xce);
HI351_write_cmos_sensor(0xab, 0x8a); //Dark1 13ce
HI351_write_cmos_sensor(0xac, 0xcf);
HI351_write_cmos_sensor(0xad, 0xa5); //Dark1 13cf
HI351_write_cmos_sensor(0xae, 0x03);
HI351_write_cmos_sensor(0xaf, 0x14);
HI351_write_cmos_sensor(0xb0, 0x11);
HI351_write_cmos_sensor(0xb1, 0x04); //Dark1 1410
HI351_write_cmos_sensor(0xb2, 0x11);
HI351_write_cmos_sensor(0xb3, 0x04); //Dark1 1411 TOP L_clip
HI351_write_cmos_sensor(0xb4, 0x12);
HI351_write_cmos_sensor(0xb5, 0x40); //Dark1 1412
HI351_write_cmos_sensor(0xb6, 0x13);
HI351_write_cmos_sensor(0xb7, 0x98); //Dark1 1413
HI351_write_cmos_sensor(0xb8, 0x14);
HI351_write_cmos_sensor(0xb9, 0x3a); //Dark1 1414
HI351_write_cmos_sensor(0xba, 0x15);
HI351_write_cmos_sensor(0xbb, 0x30); //Dark1 1415
HI351_write_cmos_sensor(0xbc, 0x16);
HI351_write_cmos_sensor(0xbd, 0x30); //Dark1 1416
HI351_write_cmos_sensor(0xbe, 0x17);
HI351_write_cmos_sensor(0xbf, 0x30); //Dark1 1417
HI351_write_cmos_sensor(0xc0, 0x18);
HI351_write_cmos_sensor(0xc1, 0x38); //Dark1 1418  Negative High Gain
HI351_write_cmos_sensor(0xc2, 0x19);
HI351_write_cmos_sensor(0xc3, 0x40); //Dark1 1419  Negative Middle Gain
HI351_write_cmos_sensor(0xc4, 0x1a);
HI351_write_cmos_sensor(0xc5, 0x40); //Dark1 141a  Negative Low Gain
HI351_write_cmos_sensor(0xc6, 0x20);
HI351_write_cmos_sensor(0xc7, 0x84); //Dark1 1420  s_diff L_clip
HI351_write_cmos_sensor(0xc8, 0x21);
HI351_write_cmos_sensor(0xc9, 0x03); //Dark1 1421
HI351_write_cmos_sensor(0xca, 0x22);
HI351_write_cmos_sensor(0xcb, 0x05); //Dark1 1422
HI351_write_cmos_sensor(0xcc, 0x23);
HI351_write_cmos_sensor(0xcd, 0x07); //Dark1 1423
HI351_write_cmos_sensor(0xce, 0x24);
HI351_write_cmos_sensor(0xcf, 0x0a); //Dark1 1424
HI351_write_cmos_sensor(0xd0, 0x25);
HI351_write_cmos_sensor(0xd1, 0x46); //Dark1 1425
HI351_write_cmos_sensor(0xd2, 0x26);
HI351_write_cmos_sensor(0xd3, 0x32); //Dark1 1426
HI351_write_cmos_sensor(0xd4, 0x27);
HI351_write_cmos_sensor(0xd5, 0x1e); //Dark1 1427
HI351_write_cmos_sensor(0xd6, 0x28);
HI351_write_cmos_sensor(0xd7, 0x10); //Dark1 1428
HI351_write_cmos_sensor(0xd8, 0x29);
HI351_write_cmos_sensor(0xd9, 0x00); //Dark1 1429
HI351_write_cmos_sensor(0xda, 0x2a);
HI351_write_cmos_sensor(0xdb, 0x18); //Dark1 142a
HI351_write_cmos_sensor(0xdc, 0x2b);
HI351_write_cmos_sensor(0xdd, 0x18); //Dark1 142b
HI351_write_cmos_sensor(0xde, 0x2c);
HI351_write_cmos_sensor(0xdf, 0x18); //Dark1 142c
HI351_write_cmos_sensor(0xe0, 0x2d);
HI351_write_cmos_sensor(0xe1, 0x30); //Dark1 142d
HI351_write_cmos_sensor(0xe2, 0x2e);
HI351_write_cmos_sensor(0xe3, 0x30); //Dark1 142e
HI351_write_cmos_sensor(0xe4, 0x2f);
HI351_write_cmos_sensor(0xe5, 0x30); //Dark1 142f
HI351_write_cmos_sensor(0xe6, 0x30);
HI351_write_cmos_sensor(0xe7, 0x84); //Dark1 1430 Ldiff_L_cip
HI351_write_cmos_sensor(0xe8, 0x31);
HI351_write_cmos_sensor(0xe9, 0x02); //Dark1 1431
HI351_write_cmos_sensor(0xea, 0x32);
HI351_write_cmos_sensor(0xeb, 0x04); //Dark1 1432
HI351_write_cmos_sensor(0xec, 0x33);
HI351_write_cmos_sensor(0xed, 0x04); //Dark1 1433
HI351_write_cmos_sensor(0xee, 0x34);
HI351_write_cmos_sensor(0xef, 0x0a); //Dark1 1434
HI351_write_cmos_sensor(0xf0, 0x35);
HI351_write_cmos_sensor(0xf1, 0x46); //Dark1 1435
HI351_write_cmos_sensor(0xf2, 0x36);
HI351_write_cmos_sensor(0xf3, 0x32); //Dark1 1436
HI351_write_cmos_sensor(0xf4, 0x37);
HI351_write_cmos_sensor(0xf5, 0x28); //Dark1 1437
HI351_write_cmos_sensor(0xf6, 0x38);
HI351_write_cmos_sensor(0xf7, 0x12); //Dark1 1438
HI351_write_cmos_sensor(0xf8, 0x39);
HI351_write_cmos_sensor(0xf9, 0x00); //Dark1 1439
HI351_write_cmos_sensor(0xfa, 0x3a);
HI351_write_cmos_sensor(0xfb, 0x20); //Dark1 143a
HI351_write_cmos_sensor(0xfc, 0x3b);
HI351_write_cmos_sensor(0xfd, 0x20); //Dark1 143b
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
HI351_write_cmos_sensor(0x03, 0xe2);
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x3c);
HI351_write_cmos_sensor(0x11, 0x20); //Dark1 143c
HI351_write_cmos_sensor(0x12, 0x3d);
HI351_write_cmos_sensor(0x13, 0x10); //Dark1 143d
HI351_write_cmos_sensor(0x14, 0x3e);
HI351_write_cmos_sensor(0x15, 0x18); //Dark1 143e
HI351_write_cmos_sensor(0x16, 0x3f);
HI351_write_cmos_sensor(0x17, 0x14); //Dark1 143f
HI351_write_cmos_sensor(0x18, 0x40);
HI351_write_cmos_sensor(0x19, 0x04); //Dark11440  Mdiff Low Clip off
HI351_write_cmos_sensor(0x1a, 0x41);
HI351_write_cmos_sensor(0x1b, 0x10); //Dark1 1441
HI351_write_cmos_sensor(0x1c, 0x42);
HI351_write_cmos_sensor(0x1d, 0x70); //Dark1 1442
HI351_write_cmos_sensor(0x1e, 0x43);
HI351_write_cmos_sensor(0x1f, 0x20); //Dark1 1443
HI351_write_cmos_sensor(0x20, 0x44);
HI351_write_cmos_sensor(0x21, 0x10); //Dark1 1444
HI351_write_cmos_sensor(0x22, 0x45);
HI351_write_cmos_sensor(0x23, 0x0c); //Dark1 1445
HI351_write_cmos_sensor(0x24, 0x46);
HI351_write_cmos_sensor(0x25, 0x10); //Dark1 1446
HI351_write_cmos_sensor(0x26, 0x47);
HI351_write_cmos_sensor(0x27, 0x18); //Dark1 1447
HI351_write_cmos_sensor(0x28, 0x48);
HI351_write_cmos_sensor(0x29, 0x0a); //Dark1 1448
HI351_write_cmos_sensor(0x2a, 0x49);
HI351_write_cmos_sensor(0x2b, 0x10); //Dark1 1449
HI351_write_cmos_sensor(0x2c, 0x50);
HI351_write_cmos_sensor(0x2d, 0x85); //Dark1 1450 Hdiff Low Clip
HI351_write_cmos_sensor(0x2e, 0x51);
HI351_write_cmos_sensor(0x2f, 0x30); //Dark1 1451 hclip
HI351_write_cmos_sensor(0x30, 0x52);
HI351_write_cmos_sensor(0x31, 0xb0); //Dark1 1452
HI351_write_cmos_sensor(0x32, 0x53);
HI351_write_cmos_sensor(0x33, 0x37); //Dark1 1453
HI351_write_cmos_sensor(0x34, 0x54);
HI351_write_cmos_sensor(0x35, 0x33); //Dark1 1454
HI351_write_cmos_sensor(0x36, 0x55);
HI351_write_cmos_sensor(0x37, 0x33); //Dark1 1455
HI351_write_cmos_sensor(0x38, 0x56);
HI351_write_cmos_sensor(0x39, 0x33); //Dark1 1456
HI351_write_cmos_sensor(0x3a, 0x57);
HI351_write_cmos_sensor(0x3b, 0x10); //Dark1 1457
HI351_write_cmos_sensor(0x3c, 0x58);
HI351_write_cmos_sensor(0x3d, 0x14); //Dark1 1458
HI351_write_cmos_sensor(0x3e, 0x59);
HI351_write_cmos_sensor(0x3f, 0x10); //Dark1 1459
HI351_write_cmos_sensor(0x40, 0x60);
HI351_write_cmos_sensor(0x41, 0x01); //Dark1 1460
HI351_write_cmos_sensor(0x42, 0x61);
HI351_write_cmos_sensor(0x43, 0xa0); //Dark1 1461
HI351_write_cmos_sensor(0x44, 0x62);
HI351_write_cmos_sensor(0x45, 0x98); //Dark1 1462
HI351_write_cmos_sensor(0x46, 0x63);
HI351_write_cmos_sensor(0x47, 0xe4); //Dark1 1463
HI351_write_cmos_sensor(0x48, 0x64);
HI351_write_cmos_sensor(0x49, 0xa4); //Dark1 1464
HI351_write_cmos_sensor(0x4a, 0x65);
HI351_write_cmos_sensor(0x4b, 0x7d); //Dark1 1465
HI351_write_cmos_sensor(0x4c, 0x66);
HI351_write_cmos_sensor(0x4d, 0x4b); //Dark1 1466
HI351_write_cmos_sensor(0x4e, 0x70);
HI351_write_cmos_sensor(0x4f, 0x10); //Dark1 1470
HI351_write_cmos_sensor(0x50, 0x71);
HI351_write_cmos_sensor(0x51, 0x10); //Dark1 1471
HI351_write_cmos_sensor(0x52, 0x72);
HI351_write_cmos_sensor(0x53, 0x10); //Dark1 1472
HI351_write_cmos_sensor(0x54, 0x73);
HI351_write_cmos_sensor(0x55, 0x10); //Dark1 1473
HI351_write_cmos_sensor(0x56, 0x74);
HI351_write_cmos_sensor(0x57, 0x10); //Dark1 1474
HI351_write_cmos_sensor(0x58, 0x75);
HI351_write_cmos_sensor(0x59, 0x10); //Dark1 1475
HI351_write_cmos_sensor(0x5a, 0x76);
HI351_write_cmos_sensor(0x5b, 0x38); //Dark1 1476    green sharp pos High
HI351_write_cmos_sensor(0x5c, 0x77);
HI351_write_cmos_sensor(0x5d, 0x38); //Dark1 1477    green sharp pos Middle
HI351_write_cmos_sensor(0x5e, 0x78);
HI351_write_cmos_sensor(0x5f, 0x38); //Dark1 1478    green sharp pos Low
HI351_write_cmos_sensor(0x60, 0x79);
HI351_write_cmos_sensor(0x61, 0x70); //Dark1 1479    green sharp nega High
HI351_write_cmos_sensor(0x62, 0x7a);
HI351_write_cmos_sensor(0x63, 0x70); //Dark1 147a    green sharp nega Middle
HI351_write_cmos_sensor(0x64, 0x7b);
HI351_write_cmos_sensor(0x65, 0x70); //Dark1 147b    green sharp nega Low
HI351_write_cmos_sensor(0x66, 0x03);
HI351_write_cmos_sensor(0x67, 0x10); //10 page
HI351_write_cmos_sensor(0x68, 0x60);
HI351_write_cmos_sensor(0x69, 0x03); //1060
HI351_write_cmos_sensor(0x6a, 0x70);
HI351_write_cmos_sensor(0x6b, 0x0c); //1070
HI351_write_cmos_sensor(0x6c, 0x71);
HI351_write_cmos_sensor(0x6d, 0x05); //1071
HI351_write_cmos_sensor(0x6e, 0x72);
HI351_write_cmos_sensor(0x6f, 0x5f); //1072
HI351_write_cmos_sensor(0x70, 0x73);
HI351_write_cmos_sensor(0x71, 0x33); //1073
HI351_write_cmos_sensor(0x72, 0x74);
HI351_write_cmos_sensor(0x73, 0x1b); //1074
HI351_write_cmos_sensor(0x74, 0x75);
HI351_write_cmos_sensor(0x75, 0x03); //1075
HI351_write_cmos_sensor(0x76, 0x76);
HI351_write_cmos_sensor(0x77, 0x20); //1076
HI351_write_cmos_sensor(0x78, 0x77);
HI351_write_cmos_sensor(0x79, 0x33); //1077
HI351_write_cmos_sensor(0x7a, 0x78);
HI351_write_cmos_sensor(0x7b, 0x33); //1078
HI351_write_cmos_sensor(0x7c, 0x79);
HI351_write_cmos_sensor(0x7d, 0x46); //1079
HI351_write_cmos_sensor(0x7e, 0x7a);
HI351_write_cmos_sensor(0x7f, 0x66); //107a
HI351_write_cmos_sensor(0x80, 0x7b);
HI351_write_cmos_sensor(0x81, 0x43); //107b
HI351_write_cmos_sensor(0x82, 0x7c);
HI351_write_cmos_sensor(0x83, 0x33); //107c
HI351_write_cmos_sensor(0x84, 0x7d);
HI351_write_cmos_sensor(0x85, 0x0e); //107d
HI351_write_cmos_sensor(0x86, 0x7e);
HI351_write_cmos_sensor(0x87, 0x1e); //107e
HI351_write_cmos_sensor(0x88, 0x7f);
HI351_write_cmos_sensor(0x89, 0x3c); //107f
HI351_write_cmos_sensor(0x8a, 0x03); //16 page
HI351_write_cmos_sensor(0x8b, 0x16);
HI351_write_cmos_sensor(0x8c, 0x8a); //168a	SP1
HI351_write_cmos_sensor(0x8d, 0x68);
HI351_write_cmos_sensor(0x8e, 0x8b); //168b  SP2
HI351_write_cmos_sensor(0x8f, 0x70);
HI351_write_cmos_sensor(0x90, 0x8c); //168c	SP3
HI351_write_cmos_sensor(0x91, 0x79);
HI351_write_cmos_sensor(0x92, 0x8d); //168d	SP4
HI351_write_cmos_sensor(0x93, 0x7a);
HI351_write_cmos_sensor(0x94, 0x8e); //168e	SP5
HI351_write_cmos_sensor(0x95, 0x7f);
HI351_write_cmos_sensor(0x96, 0x8f); //168f	SP6
HI351_write_cmos_sensor(0x97, 0x7f);
HI351_write_cmos_sensor(0x98, 0x90); //1690	SP7
HI351_write_cmos_sensor(0x99, 0x7f);
HI351_write_cmos_sensor(0x9a, 0x91); //1691	SP8
HI351_write_cmos_sensor(0x9b, 0x7f);
HI351_write_cmos_sensor(0x9c, 0x92); //1692	SP9
HI351_write_cmos_sensor(0x9d, 0x7f);
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
//end

//////////////////
// e3 Page (DMA Dark2)
//////////////////

HI351_write_cmos_sensor(0x03, 0xe3);
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x03);//Dark2 Page11
HI351_write_cmos_sensor(0x11, 0x11);
HI351_write_cmos_sensor(0x12, 0x10);
HI351_write_cmos_sensor(0x13, 0x1f); //Dark2 1110
HI351_write_cmos_sensor(0x14, 0x11);
HI351_write_cmos_sensor(0x15, 0x2a); //Dark2 1111
HI351_write_cmos_sensor(0x16, 0x12);
HI351_write_cmos_sensor(0x17, 0x1c); //Dark2 1112
HI351_write_cmos_sensor(0x18, 0x13);
HI351_write_cmos_sensor(0x19, 0x1c); //Dark2 1113
HI351_write_cmos_sensor(0x1a, 0x14);
HI351_write_cmos_sensor(0x1b, 0x3a); //Dark2 1114
HI351_write_cmos_sensor(0x1c, 0x30);
HI351_write_cmos_sensor(0x1d, 0x20); //Dark2 1130
HI351_write_cmos_sensor(0x1e, 0x31);
HI351_write_cmos_sensor(0x1f, 0x20); //Dark2 1131
HI351_write_cmos_sensor(0x20, 0x32);
HI351_write_cmos_sensor(0x21, 0x40); //Dark2 1132 :Lum level1
HI351_write_cmos_sensor(0x22, 0x33);
HI351_write_cmos_sensor(0x23, 0x28); //Dark2 1133 :Lum level2
HI351_write_cmos_sensor(0x24, 0x34);
HI351_write_cmos_sensor(0x25, 0x1a); //Dark2 1134 :Lum level3
HI351_write_cmos_sensor(0x26, 0x35);
HI351_write_cmos_sensor(0x27, 0x14); //Dark2 1135 :Lum level4
HI351_write_cmos_sensor(0x28, 0x36);
HI351_write_cmos_sensor(0x29, 0x0c); //Dark2 1136 :Lum level5
HI351_write_cmos_sensor(0x2a, 0x37);
HI351_write_cmos_sensor(0x2b, 0x0a); //Dark2 1137 :Lum level6
HI351_write_cmos_sensor(0x2c, 0x38);
HI351_write_cmos_sensor(0x2d, 0x00); //Dark2 1138 :Lum level7
HI351_write_cmos_sensor(0x2e, 0x39);
HI351_write_cmos_sensor(0x2f, 0x8a); //Dark2 1139 gain 1
HI351_write_cmos_sensor(0x30, 0x3a);
HI351_write_cmos_sensor(0x31, 0x8a); //Dark2 113a
HI351_write_cmos_sensor(0x32, 0x3b);
HI351_write_cmos_sensor(0x33, 0x8a); //Dark2 113b
HI351_write_cmos_sensor(0x34, 0x3c);
HI351_write_cmos_sensor(0x35, 0x8a); //Dark2 113c
HI351_write_cmos_sensor(0x36, 0x3d);
HI351_write_cmos_sensor(0x37, 0x8a); //Dark2 113d
HI351_write_cmos_sensor(0x38, 0x3e);
HI351_write_cmos_sensor(0x39, 0x8a); //Dark2 113e
HI351_write_cmos_sensor(0x3a, 0x3f);
HI351_write_cmos_sensor(0x3b, 0x8a); //Dark2 113f
HI351_write_cmos_sensor(0x3c, 0x40);
HI351_write_cmos_sensor(0x3d, 0x8a); //Dark2 1140 gain 8
HI351_write_cmos_sensor(0x3e, 0x41);
HI351_write_cmos_sensor(0x3f, 0x40); //Dark2 1141 offset 1
HI351_write_cmos_sensor(0x40, 0x42);
HI351_write_cmos_sensor(0x41, 0x10); //Dark2 1142 offset 2
HI351_write_cmos_sensor(0x42, 0x43);
HI351_write_cmos_sensor(0x43, 0x10); //Dark2 1143 offset 3
HI351_write_cmos_sensor(0x44, 0x44);
HI351_write_cmos_sensor(0x45, 0x10); //Dark2 1144 offset 4
HI351_write_cmos_sensor(0x46, 0x45);
HI351_write_cmos_sensor(0x47, 0x10); //Dark2 1145 offset 5
HI351_write_cmos_sensor(0x48, 0x46);
HI351_write_cmos_sensor(0x49, 0x10); //Dark2 1146 offset 6
HI351_write_cmos_sensor(0x4a, 0x47);
HI351_write_cmos_sensor(0x4b, 0x10); //Dark2 1147 offset 7
HI351_write_cmos_sensor(0x4c, 0x48);
HI351_write_cmos_sensor(0x4d, 0x10); //Dark2 1148 offset 8
HI351_write_cmos_sensor(0x4e, 0x49);
HI351_write_cmos_sensor(0x4f, 0x40); //Dark2 1149 high_clip_start
HI351_write_cmos_sensor(0x50, 0x4a);
HI351_write_cmos_sensor(0x51, 0x40); //Dark2 114a
HI351_write_cmos_sensor(0x52, 0x4b);
HI351_write_cmos_sensor(0x53, 0x40); //Dark2 114b
HI351_write_cmos_sensor(0x54, 0x4c);
HI351_write_cmos_sensor(0x55, 0x40); //Dark2 114c
HI351_write_cmos_sensor(0x56, 0x4d);
HI351_write_cmos_sensor(0x57, 0x40); //Dark2 114d
HI351_write_cmos_sensor(0x58, 0x4e);
HI351_write_cmos_sensor(0x59, 0x40); //Dark2 114e Lv 6 h_clip
HI351_write_cmos_sensor(0x5a, 0x4f);
HI351_write_cmos_sensor(0x5b, 0x40); //Dark2 114f Lv 7 h_clip
HI351_write_cmos_sensor(0x5c, 0x50);
HI351_write_cmos_sensor(0x5d, 0x40); //Dark2 1150 clip 8
HI351_write_cmos_sensor(0x5e, 0x51);
HI351_write_cmos_sensor(0x5f, 0xf0); //Dark2 1151 color gain start
HI351_write_cmos_sensor(0x60, 0x52);
HI351_write_cmos_sensor(0x61, 0xf0); //Dark2 1152
HI351_write_cmos_sensor(0x62, 0x53);
HI351_write_cmos_sensor(0x63, 0xf0); //Dark2 1153
HI351_write_cmos_sensor(0x64, 0x54);
HI351_write_cmos_sensor(0x65, 0xf0); //Dark2 1154
HI351_write_cmos_sensor(0x66, 0x55);
HI351_write_cmos_sensor(0x67, 0xf0); //Dark2 1155
HI351_write_cmos_sensor(0x68, 0x56);
HI351_write_cmos_sensor(0x69, 0xf0); //Dark2 1156
HI351_write_cmos_sensor(0x6a, 0x57);
HI351_write_cmos_sensor(0x6b, 0xf0); //Dark2 1157
HI351_write_cmos_sensor(0x6c, 0x58);
HI351_write_cmos_sensor(0x6d, 0xf0); //Dark2 1158 color gain end
HI351_write_cmos_sensor(0x6e, 0x59);
HI351_write_cmos_sensor(0x6f, 0xf8); //Dark2 1159 color ofs lmt start
HI351_write_cmos_sensor(0x70, 0x5a);
HI351_write_cmos_sensor(0x71, 0xf8); //Dark2 115a
HI351_write_cmos_sensor(0x72, 0x5b);
HI351_write_cmos_sensor(0x73, 0xf8); //Dark2 115b
HI351_write_cmos_sensor(0x74, 0x5c);
HI351_write_cmos_sensor(0x75, 0xf8); //Dark2 115c
HI351_write_cmos_sensor(0x76, 0x5d);
HI351_write_cmos_sensor(0x77, 0xf8); //Dark2 115d
HI351_write_cmos_sensor(0x78, 0x5e);
HI351_write_cmos_sensor(0x79, 0xf8); //Dark2 115e
HI351_write_cmos_sensor(0x7a, 0x5f);
HI351_write_cmos_sensor(0x7b, 0xf8); //Dark2 115f
HI351_write_cmos_sensor(0x7c, 0x60);
HI351_write_cmos_sensor(0x7d, 0xf8); //Dark2 1160 color ofs lmt end
HI351_write_cmos_sensor(0x7e, 0x61);
HI351_write_cmos_sensor(0x7f, 0xc0); //Dark2 1161
HI351_write_cmos_sensor(0x80, 0x62);
HI351_write_cmos_sensor(0x81, 0xf0); //Dark2 1162
HI351_write_cmos_sensor(0x82, 0x63);
HI351_write_cmos_sensor(0x83, 0x80); //Dark2 1163
HI351_write_cmos_sensor(0x84, 0x64);
HI351_write_cmos_sensor(0x85, 0x40); //Dark2 1164
HI351_write_cmos_sensor(0x86, 0x65);
HI351_write_cmos_sensor(0x87, 0x02); //Dark2 1165 : lmp_1_gain_h
HI351_write_cmos_sensor(0x88, 0x66);
HI351_write_cmos_sensor(0x89, 0x02); //Dark2 1166 : lmp_1_gain_m
HI351_write_cmos_sensor(0x8a, 0x67);
HI351_write_cmos_sensor(0x8b, 0x02); //Dark2 1167 : lmp_1_gain_l
HI351_write_cmos_sensor(0x8c, 0x68);
HI351_write_cmos_sensor(0x8d, 0x80); //Dark2 1168
HI351_write_cmos_sensor(0x8e, 0x69);
HI351_write_cmos_sensor(0x8f, 0x40); //Dark2 1169
HI351_write_cmos_sensor(0x90, 0x6a);
HI351_write_cmos_sensor(0x91, 0x01); //Dark2 116a : lmp_2_gain_h
HI351_write_cmos_sensor(0x92, 0x6b);
HI351_write_cmos_sensor(0x93, 0x01); //Dark2 116b : lmp_2_gain_m
HI351_write_cmos_sensor(0x94, 0x6c);
HI351_write_cmos_sensor(0x95, 0x01); //Dark2 116c : lmp_2_gain_l
HI351_write_cmos_sensor(0x96, 0x6d);
HI351_write_cmos_sensor(0x97, 0x80); //Dark2 116d
HI351_write_cmos_sensor(0x98, 0x6e);
HI351_write_cmos_sensor(0x99, 0x40); //Dark2 116e
HI351_write_cmos_sensor(0x9a, 0x6f);
HI351_write_cmos_sensor(0x9b, 0x01); //Dark2 116f : lmp_3_gain_h
HI351_write_cmos_sensor(0x9c, 0x70);
HI351_write_cmos_sensor(0x9d, 0x01); //Dark2 1170 : lmp_3_gain_m
HI351_write_cmos_sensor(0x9e, 0x71);
HI351_write_cmos_sensor(0x9f, 0x01); //Dark2 1171 : lmp_3_gain_l
HI351_write_cmos_sensor(0xa0, 0x72);
HI351_write_cmos_sensor(0xa1, 0x6e); //Dark2 1172
HI351_write_cmos_sensor(0xa2, 0x73);
HI351_write_cmos_sensor(0xa3, 0x3a); //Dark2 1173
HI351_write_cmos_sensor(0xa4, 0x74);
HI351_write_cmos_sensor(0xa5, 0x01); //Dark2 1174 : lmp_4_gain_h
HI351_write_cmos_sensor(0xa6, 0x75);
HI351_write_cmos_sensor(0xa7, 0x01); //Dark2 1175 : lmp_4_gain_m
HI351_write_cmos_sensor(0xa8, 0x76);
HI351_write_cmos_sensor(0xa9, 0x01); //Dark2 1176 : lmp_4_gain_l
HI351_write_cmos_sensor(0xaa, 0x77);
HI351_write_cmos_sensor(0xab, 0x6e); //Dark2 1177
HI351_write_cmos_sensor(0xac, 0x78);
HI351_write_cmos_sensor(0xad, 0x3a); //Dark2 1178
HI351_write_cmos_sensor(0xae, 0x79);
HI351_write_cmos_sensor(0xaf, 0x01); //Dark2 1179 : lmp_5_gain_h
HI351_write_cmos_sensor(0xb0, 0x7a);
HI351_write_cmos_sensor(0xb1, 0x01); //Dark2 117a : lmp_5_gain_m
HI351_write_cmos_sensor(0xb2, 0x7b);
HI351_write_cmos_sensor(0xb3, 0x01); //Dark2 117b : lmp_5_gain_l
HI351_write_cmos_sensor(0xb4, 0x7c);
HI351_write_cmos_sensor(0xb5, 0x5c); //Dark2 117c
HI351_write_cmos_sensor(0xb6, 0x7d);
HI351_write_cmos_sensor(0xb7, 0x30); //Dark2 117d
HI351_write_cmos_sensor(0xb8, 0x7e);
HI351_write_cmos_sensor(0xb9, 0x01); //Dark2 117e : lmp_6_gain_h
HI351_write_cmos_sensor(0xba, 0x7f);
HI351_write_cmos_sensor(0xbb, 0x01); //Dark2 117f : lmp_6_gain_m
HI351_write_cmos_sensor(0xbc, 0x80);
HI351_write_cmos_sensor(0xbd, 0x01); //Dark2 1180 : lmp_6_gain_l
HI351_write_cmos_sensor(0xbe, 0x81);
HI351_write_cmos_sensor(0xbf, 0x62); //Dark2 1181
HI351_write_cmos_sensor(0xc0, 0x82);
HI351_write_cmos_sensor(0xc1, 0x26); //Dark2 1182
HI351_write_cmos_sensor(0xc2, 0x83);
HI351_write_cmos_sensor(0xc3, 0x01); //Dark2 1183 : lmp_7_gain_h
HI351_write_cmos_sensor(0xc4, 0x84);
HI351_write_cmos_sensor(0xc5, 0x01); //Dark2 1184 : lmp_7_gain_m
HI351_write_cmos_sensor(0xc6, 0x85);
HI351_write_cmos_sensor(0xc7, 0x01); //Dark2 1185 : lmp_7_gain_l
HI351_write_cmos_sensor(0xc8, 0x86);
HI351_write_cmos_sensor(0xc9, 0x62); //Dark2 1186
HI351_write_cmos_sensor(0xca, 0x87);
HI351_write_cmos_sensor(0xcb, 0x26); //Dark2 1187
HI351_write_cmos_sensor(0xcc, 0x88);
HI351_write_cmos_sensor(0xcd, 0x01); //Dark2 1188 : lmp_8_gain_h
HI351_write_cmos_sensor(0xce, 0x89);
HI351_write_cmos_sensor(0xcf, 0x01); //Dark2 1189 : lmp_8_gain_m
HI351_write_cmos_sensor(0xd0, 0x8a);
HI351_write_cmos_sensor(0xd1, 0x01); //Dark2 118a : lmp_8_gain_l
HI351_write_cmos_sensor(0xd2, 0x90);
HI351_write_cmos_sensor(0xd3, 0x00); //Dark2 1190
HI351_write_cmos_sensor(0xd4, 0x91);
HI351_write_cmos_sensor(0xd5, 0x4e); //Dark2 1191
HI351_write_cmos_sensor(0xd6, 0x92);
HI351_write_cmos_sensor(0xd7, 0x00); //Dark2 1192
HI351_write_cmos_sensor(0xd8, 0x93);
HI351_write_cmos_sensor(0xd9, 0x16); //Dark2 1193
HI351_write_cmos_sensor(0xda, 0x94);
HI351_write_cmos_sensor(0xdb, 0x01); //Dark2 1194
HI351_write_cmos_sensor(0xdc, 0x95);
HI351_write_cmos_sensor(0xdd, 0x80); //Dark2 1195
HI351_write_cmos_sensor(0xde, 0x96);
HI351_write_cmos_sensor(0xdf, 0x55); //Dark2 1196
HI351_write_cmos_sensor(0xe0, 0x97);
HI351_write_cmos_sensor(0xe1, 0x8d); //Dark2 1197
HI351_write_cmos_sensor(0xe2, 0xb0);
HI351_write_cmos_sensor(0xe3, 0x30); //Dark2 11b0
HI351_write_cmos_sensor(0xe4, 0xb1);
HI351_write_cmos_sensor(0xe5, 0x90); //Dark2 11b1
HI351_write_cmos_sensor(0xe6, 0xb2);
HI351_write_cmos_sensor(0xe7, 0x08); //Dark2 11b2
HI351_write_cmos_sensor(0xe8, 0xb3);
HI351_write_cmos_sensor(0xe9, 0x00); //Dark2 11b3
HI351_write_cmos_sensor(0xea, 0xb4);
HI351_write_cmos_sensor(0xeb, 0x04); //Dark2 11b4
HI351_write_cmos_sensor(0xec, 0x03);
HI351_write_cmos_sensor(0xed, 0x12);
HI351_write_cmos_sensor(0xee, 0x10);
HI351_write_cmos_sensor(0xef, 0x03); //Dark2 1210
HI351_write_cmos_sensor(0xf0, 0x11);
HI351_write_cmos_sensor(0xf1, 0x29); //Dark2 1211
HI351_write_cmos_sensor(0xf2, 0x12);
HI351_write_cmos_sensor(0xf3, 0x03); //Dark2 1212
HI351_write_cmos_sensor(0xf4, 0x40);
HI351_write_cmos_sensor(0xf5, 0x02); //Dark2 1240
HI351_write_cmos_sensor(0xf6, 0x41);
HI351_write_cmos_sensor(0xf7, 0x0a); //Dark2 1241
HI351_write_cmos_sensor(0xf8, 0x42);
HI351_write_cmos_sensor(0xf9, 0x6a); //Dark2 1242
HI351_write_cmos_sensor(0xfa, 0x43);
HI351_write_cmos_sensor(0xfb, 0x80); //Dark2 1243
HI351_write_cmos_sensor(0xfc, 0x44);
HI351_write_cmos_sensor(0xfd, 0x02); //Dark2 1244
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
HI351_write_cmos_sensor(0x03, 0xe4);
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x45);
HI351_write_cmos_sensor(0x11, 0x0a); //Dark2 1245
HI351_write_cmos_sensor(0x12, 0x46);
HI351_write_cmos_sensor(0x13, 0x80); //Dark2 1246
HI351_write_cmos_sensor(0x14, 0x60);
HI351_write_cmos_sensor(0x15, 0x02); //Dark2 1260
HI351_write_cmos_sensor(0x16, 0x61);
HI351_write_cmos_sensor(0x17, 0x04); //Dark2 1261
HI351_write_cmos_sensor(0x18, 0x62);
HI351_write_cmos_sensor(0x19, 0x4b); //Dark2 1262
HI351_write_cmos_sensor(0x1a, 0x63);
HI351_write_cmos_sensor(0x1b, 0x41); //Dark2 1263
HI351_write_cmos_sensor(0x1c, 0x64);
HI351_write_cmos_sensor(0x1d, 0x14); //Dark2 1264
HI351_write_cmos_sensor(0x1e, 0x65);
HI351_write_cmos_sensor(0x1f, 0x00); //Dark2 1265
HI351_write_cmos_sensor(0x20, 0x68);
HI351_write_cmos_sensor(0x21, 0x0a); //Dark2 1268
HI351_write_cmos_sensor(0x22, 0x69);
HI351_write_cmos_sensor(0x23, 0x04); //Dark2 1269
HI351_write_cmos_sensor(0x24, 0x6a);
HI351_write_cmos_sensor(0x25, 0x0a); //Dark2 126a
HI351_write_cmos_sensor(0x26, 0x6b);
HI351_write_cmos_sensor(0x27, 0x0a); //Dark2 126b
HI351_write_cmos_sensor(0x28, 0x6c);
HI351_write_cmos_sensor(0x29, 0x24); //Dark2 126c
HI351_write_cmos_sensor(0x2a, 0x6d);
HI351_write_cmos_sensor(0x2b, 0x01); //Dark2 126d
HI351_write_cmos_sensor(0x2c, 0x70);
HI351_write_cmos_sensor(0x2d, 0x18); //Dark2 1270
HI351_write_cmos_sensor(0x2e, 0x71);
HI351_write_cmos_sensor(0x2f, 0xbf); //Dark2 1271
HI351_write_cmos_sensor(0x30, 0x80);
HI351_write_cmos_sensor(0x31, 0x64); //Dark2 1280
HI351_write_cmos_sensor(0x32, 0x81);
HI351_write_cmos_sensor(0x33, 0xb1); //Dark2 1281
HI351_write_cmos_sensor(0x34, 0x82);
HI351_write_cmos_sensor(0x35, 0x2c); //Dark2 1282
HI351_write_cmos_sensor(0x36, 0x83);
HI351_write_cmos_sensor(0x37, 0x02); //Dark2 1283
HI351_write_cmos_sensor(0x38, 0x84);
HI351_write_cmos_sensor(0x39, 0x30); //Dark2 1284
HI351_write_cmos_sensor(0x3a, 0x85);
HI351_write_cmos_sensor(0x3b, 0x90); //Dark2 1285
HI351_write_cmos_sensor(0x3c, 0x86);
HI351_write_cmos_sensor(0x3d, 0x10); //Dark2 1286
HI351_write_cmos_sensor(0x3e, 0x87);
HI351_write_cmos_sensor(0x3f, 0x01); //Dark2 1287
HI351_write_cmos_sensor(0x40, 0x88);
HI351_write_cmos_sensor(0x41, 0x3a); //Dark2 1288
HI351_write_cmos_sensor(0x42, 0x89);
HI351_write_cmos_sensor(0x43, 0x90); //Dark2 1289
HI351_write_cmos_sensor(0x44, 0x8a);
HI351_write_cmos_sensor(0x45, 0x0e); //Dark2 128a
HI351_write_cmos_sensor(0x46, 0x8b);
HI351_write_cmos_sensor(0x47, 0x0c); //Dark2 128b
HI351_write_cmos_sensor(0x48, 0x8c);
HI351_write_cmos_sensor(0x49, 0x05); //Dark2 128c
HI351_write_cmos_sensor(0x4a, 0x8d);
HI351_write_cmos_sensor(0x4b, 0x03); //Dark2 128d
HI351_write_cmos_sensor(0x4c, 0xe6);
HI351_write_cmos_sensor(0x4d, 0xff); //Dark2 12e6
HI351_write_cmos_sensor(0x4e, 0xe7);
HI351_write_cmos_sensor(0x4f, 0x18); //Dark2 12e7
HI351_write_cmos_sensor(0x50, 0xe8);
HI351_write_cmos_sensor(0x51, 0x0a); //Dark2 12e8
HI351_write_cmos_sensor(0x52, 0xe9);
HI351_write_cmos_sensor(0x53, 0x06); //Dark2 12e9
HI351_write_cmos_sensor(0x54, 0x03);
HI351_write_cmos_sensor(0x55, 0x13);
HI351_write_cmos_sensor(0x56, 0x10);
HI351_write_cmos_sensor(0x57, 0x3f); //Dark2 1310
HI351_write_cmos_sensor(0x58, 0x20);
HI351_write_cmos_sensor(0x59, 0x20); //Dark2 1320
HI351_write_cmos_sensor(0x5a, 0x21);
HI351_write_cmos_sensor(0x5b, 0x30); //Dark2 1321
HI351_write_cmos_sensor(0x5c, 0x22);
HI351_write_cmos_sensor(0x5d, 0x36); //Dark2 1322
HI351_write_cmos_sensor(0x5e, 0x23);
HI351_write_cmos_sensor(0x5f, 0x6a); //Dark2 1323
HI351_write_cmos_sensor(0x60, 0x24);
HI351_write_cmos_sensor(0x61, 0xa0); //Dark2 1324
HI351_write_cmos_sensor(0x62, 0x25);
HI351_write_cmos_sensor(0x63, 0xc0); //Dark2 1325
HI351_write_cmos_sensor(0x64, 0x26);
HI351_write_cmos_sensor(0x65, 0xe0); //Dark2 1326
HI351_write_cmos_sensor(0x66, 0x27);
HI351_write_cmos_sensor(0x67, 0x00); //Dark2 1327 lum 0
HI351_write_cmos_sensor(0x68, 0x28);
HI351_write_cmos_sensor(0x69, 0x00); //Dark2 1328
HI351_write_cmos_sensor(0x6a, 0x29);
HI351_write_cmos_sensor(0x6b, 0x00); //Dark2 1329
HI351_write_cmos_sensor(0x6c, 0x2a);
HI351_write_cmos_sensor(0x6d, 0x00); //Dark2 132a
HI351_write_cmos_sensor(0x6e, 0x2b);
HI351_write_cmos_sensor(0x6f, 0x00); //Dark2 132b
HI351_write_cmos_sensor(0x70, 0x2c);
HI351_write_cmos_sensor(0x71, 0x00); //Dark2 132c
HI351_write_cmos_sensor(0x72, 0x2d);
HI351_write_cmos_sensor(0x73, 0x00); //Dark2 132d
HI351_write_cmos_sensor(0x74, 0x2e);
HI351_write_cmos_sensor(0x75, 0x00); //Dark2 132e lum7
HI351_write_cmos_sensor(0x76, 0x2f);
HI351_write_cmos_sensor(0x77, 0x04); //Dark2 132f weight skin
HI351_write_cmos_sensor(0x78, 0x30);
HI351_write_cmos_sensor(0x79, 0x04); //Dark2 1330 weight blue
HI351_write_cmos_sensor(0x7a, 0x31);
HI351_write_cmos_sensor(0x7b, 0x04); //Dark2 1331 weight green
HI351_write_cmos_sensor(0x7c, 0x32);
HI351_write_cmos_sensor(0x7d, 0x04); //Dark2 1332 weight strong color
HI351_write_cmos_sensor(0x7e, 0x33);
HI351_write_cmos_sensor(0x7f, 0x10); //Dark2 1333
HI351_write_cmos_sensor(0x80, 0x34);
HI351_write_cmos_sensor(0x81, 0x10); //Dark2 1334
HI351_write_cmos_sensor(0x82, 0x35);
HI351_write_cmos_sensor(0x83, 0x00); //Dark2 1335
HI351_write_cmos_sensor(0x84, 0x36);
HI351_write_cmos_sensor(0x85, 0x80); //Dark2 1336
HI351_write_cmos_sensor(0x86, 0xa0);
HI351_write_cmos_sensor(0x87, 0x07); //Dark2 13a0
HI351_write_cmos_sensor(0x88, 0xa8);
HI351_write_cmos_sensor(0x89, 0x30); //Dark2 13a8 Dark2 Cb-filter
HI351_write_cmos_sensor(0x8a, 0xa9);
HI351_write_cmos_sensor(0x8b, 0x30); //Dark2 13a9 Dark2 Cr-filter
HI351_write_cmos_sensor(0x8c, 0xaa);
HI351_write_cmos_sensor(0x8d, 0x30); //Dark2 13aa
HI351_write_cmos_sensor(0x8e, 0xab);
HI351_write_cmos_sensor(0x8f, 0x02); //Dark2 13ab
HI351_write_cmos_sensor(0x90, 0xc0);
HI351_write_cmos_sensor(0x91, 0x27); //Dark2 13c0
HI351_write_cmos_sensor(0x92, 0xc2);
HI351_write_cmos_sensor(0x93, 0x08); //Dark2 13c2
HI351_write_cmos_sensor(0x94, 0xc3);
HI351_write_cmos_sensor(0x95, 0x08); //Dark2 13c3
HI351_write_cmos_sensor(0x96, 0xc4);
HI351_write_cmos_sensor(0x97, 0x46); //Dark2 13c4
HI351_write_cmos_sensor(0x98, 0xc5);
HI351_write_cmos_sensor(0x99, 0x78); //Dark2 13c5
HI351_write_cmos_sensor(0x9a, 0xc6);
HI351_write_cmos_sensor(0x9b, 0xf0); //Dark2 13c6
HI351_write_cmos_sensor(0x9c, 0xc7);
HI351_write_cmos_sensor(0x9d, 0x10); //Dark2 13c7
HI351_write_cmos_sensor(0x9e, 0xc8);
HI351_write_cmos_sensor(0x9f, 0x44); //Dark2 13c8
HI351_write_cmos_sensor(0xa0, 0xc9);
HI351_write_cmos_sensor(0xa1, 0x87); //Dark2 13c9
HI351_write_cmos_sensor(0xa2, 0xca);
HI351_write_cmos_sensor(0xa3, 0xff); //Dark2 13ca
HI351_write_cmos_sensor(0xa4, 0xcb);
HI351_write_cmos_sensor(0xa5, 0x20); //Dark2 13cb
HI351_write_cmos_sensor(0xa6, 0xcc);
HI351_write_cmos_sensor(0xa7, 0x61); //Dark2 13cc skin range_cb_l
HI351_write_cmos_sensor(0xa8, 0xcd);
HI351_write_cmos_sensor(0xa9, 0x87); //Dark2 13cd skin range_cb_h
HI351_write_cmos_sensor(0xaa, 0xce);
HI351_write_cmos_sensor(0xab, 0x8a); //Dark2 13ce skin range_cr_l
HI351_write_cmos_sensor(0xac, 0xcf);
HI351_write_cmos_sensor(0xad, 0xa5); //Dark2 13cf skin range_cr_h
HI351_write_cmos_sensor(0xae, 0x03);
HI351_write_cmos_sensor(0xaf, 0x14);
HI351_write_cmos_sensor(0xb0, 0x11);
HI351_write_cmos_sensor(0xb1, 0x03); //Dark2 1410
HI351_write_cmos_sensor(0xb2, 0x11);
HI351_write_cmos_sensor(0xb3, 0x03); //Dark2 1411
HI351_write_cmos_sensor(0xb4, 0x12);
HI351_write_cmos_sensor(0xb5, 0x40); //Dark2 1412 Top H_Clip
HI351_write_cmos_sensor(0xb6, 0x13);
HI351_write_cmos_sensor(0xb7, 0x88); //Dark2 1413
HI351_write_cmos_sensor(0xb8, 0x14);
HI351_write_cmos_sensor(0xb9, 0x34); //Dark2 1414
HI351_write_cmos_sensor(0xba, 0x15);
HI351_write_cmos_sensor(0xbb, 0x00); //Dark2 1415 sharp positive ya
HI351_write_cmos_sensor(0xbc, 0x16);
HI351_write_cmos_sensor(0xbd, 0x00); //Dark2 1416 sharp positive mi
HI351_write_cmos_sensor(0xbe, 0x17);
HI351_write_cmos_sensor(0xbf, 0x00); //Dark2 1417 sharp positive low
HI351_write_cmos_sensor(0xc0, 0x18);
HI351_write_cmos_sensor(0xc1, 0x10); //Dark2 1418 sharp negative ya
HI351_write_cmos_sensor(0xc2, 0x19);
HI351_write_cmos_sensor(0xc3, 0x10); //Dark2 1419 sharp negative mi
HI351_write_cmos_sensor(0xc4, 0x1a);
HI351_write_cmos_sensor(0xc5, 0x10); //Dark2 141a sharp negative low
HI351_write_cmos_sensor(0xc6, 0x20);
HI351_write_cmos_sensor(0xc7, 0x83); //Dark2 1420
HI351_write_cmos_sensor(0xc8, 0x21);
HI351_write_cmos_sensor(0xc9, 0x03); //Dark2 1421
HI351_write_cmos_sensor(0xca, 0x22);
HI351_write_cmos_sensor(0xcb, 0x05); //Dark2 1422
HI351_write_cmos_sensor(0xcc, 0x23);
HI351_write_cmos_sensor(0xcd, 0x07); //Dark2 1423
HI351_write_cmos_sensor(0xce, 0x24);
HI351_write_cmos_sensor(0xcf, 0x0a); //Dark2 1424
HI351_write_cmos_sensor(0xd0, 0x25);
HI351_write_cmos_sensor(0xd1, 0x46); //Dark2 1425
HI351_write_cmos_sensor(0xd2, 0x26);
HI351_write_cmos_sensor(0xd3, 0x32); //Dark2 1426
HI351_write_cmos_sensor(0xd4, 0x27);
HI351_write_cmos_sensor(0xd5, 0x1e); //Dark2 1427
HI351_write_cmos_sensor(0xd6, 0x28);
HI351_write_cmos_sensor(0xd7, 0x19); //Dark2 1428
HI351_write_cmos_sensor(0xd8, 0x29);
HI351_write_cmos_sensor(0xd9, 0x00); //Dark2 1429
HI351_write_cmos_sensor(0xda, 0x2a);
HI351_write_cmos_sensor(0xdb, 0x10); //Dark2 142a
HI351_write_cmos_sensor(0xdc, 0x2b);
HI351_write_cmos_sensor(0xdd, 0x10); //Dark2 142b
HI351_write_cmos_sensor(0xde, 0x2c);
HI351_write_cmos_sensor(0xdf, 0x10); //Dark2 142c
HI351_write_cmos_sensor(0xe0, 0x2d);
HI351_write_cmos_sensor(0xe1, 0x80); //Dark2 142d
HI351_write_cmos_sensor(0xe2, 0x2e);
HI351_write_cmos_sensor(0xe3, 0x80); //Dark2 142e
HI351_write_cmos_sensor(0xe4, 0x2f);
HI351_write_cmos_sensor(0xe5, 0x80); //Dark2 142f
HI351_write_cmos_sensor(0xe6, 0x30);
HI351_write_cmos_sensor(0xe7, 0x83); //Dark2 1430
HI351_write_cmos_sensor(0xe8, 0x31);
HI351_write_cmos_sensor(0xe9, 0x02); //Dark2 1431
HI351_write_cmos_sensor(0xea, 0x32);
HI351_write_cmos_sensor(0xeb, 0x04); //Dark2 1432
HI351_write_cmos_sensor(0xec, 0x33);
HI351_write_cmos_sensor(0xed, 0x04); //Dark2 1433
HI351_write_cmos_sensor(0xee, 0x34);
HI351_write_cmos_sensor(0xef, 0x0a); //Dark2 1434
HI351_write_cmos_sensor(0xf0, 0x35);
HI351_write_cmos_sensor(0xf1, 0x46); //Dark2 1435
HI351_write_cmos_sensor(0xf2, 0x36);
HI351_write_cmos_sensor(0xf3, 0x32); //Dark2 1436
HI351_write_cmos_sensor(0xf4, 0x37);
HI351_write_cmos_sensor(0xf5, 0x28); //Dark2 1437
HI351_write_cmos_sensor(0xf6, 0x38);
HI351_write_cmos_sensor(0xf7, 0x12); //Dark2 1438
HI351_write_cmos_sensor(0xf8, 0x39);
HI351_write_cmos_sensor(0xf9, 0x00); //Dark2 1439
HI351_write_cmos_sensor(0xfa, 0x3a);
HI351_write_cmos_sensor(0xfb, 0x18); //Dark2 143a dr gain
HI351_write_cmos_sensor(0xfc, 0x3b);
HI351_write_cmos_sensor(0xfd, 0x20); //Dark2 143b
HI351_write_cmos_sensor(0x0e, 0x00); //burst_end

HI351_write_cmos_sensor(0x03, 0xe5);
HI351_write_cmos_sensor(0x0e, 0x01); //burst_start
HI351_write_cmos_sensor(0x10, 0x3c);
HI351_write_cmos_sensor(0x11, 0x18); //Dark2 143c
HI351_write_cmos_sensor(0x12, 0x3d);
HI351_write_cmos_sensor(0x13, 0x20); //Dark2 143d nor gain
HI351_write_cmos_sensor(0x14, 0x3e);
HI351_write_cmos_sensor(0x15, 0x22); //Dark2 143e
HI351_write_cmos_sensor(0x16, 0x3f);
HI351_write_cmos_sensor(0x17, 0x10); //Dark2 143f
HI351_write_cmos_sensor(0x18, 0x40);
HI351_write_cmos_sensor(0x19, 0x03); //Dark2 1440
HI351_write_cmos_sensor(0x1a, 0x41);
HI351_write_cmos_sensor(0x1b, 0x12); //Dark2 1441
HI351_write_cmos_sensor(0x1c, 0x42);
HI351_write_cmos_sensor(0x1d, 0xb0); //Dark2 1442
HI351_write_cmos_sensor(0x1e, 0x43);
HI351_write_cmos_sensor(0x1f, 0x20); //Dark2 1443
HI351_write_cmos_sensor(0x20, 0x44);
HI351_write_cmos_sensor(0x21, 0x0a); //Dark2 1444
HI351_write_cmos_sensor(0x22, 0x45);
HI351_write_cmos_sensor(0x23, 0x0a); //Dark2 1445
HI351_write_cmos_sensor(0x24, 0x46);
HI351_write_cmos_sensor(0x25, 0x0a); //Dark2 1446
HI351_write_cmos_sensor(0x26, 0x47);
HI351_write_cmos_sensor(0x27, 0x08); //Dark2 1447
HI351_write_cmos_sensor(0x28, 0x48);
HI351_write_cmos_sensor(0x29, 0x08); //Dark2 1448
HI351_write_cmos_sensor(0x2a, 0x49);
HI351_write_cmos_sensor(0x2b, 0x08); //Dark2 1449
HI351_write_cmos_sensor(0x2c, 0x50);
HI351_write_cmos_sensor(0x2d, 0x03); //Dark2 1450
HI351_write_cmos_sensor(0x2e, 0x51);
HI351_write_cmos_sensor(0x2f, 0x32); //Dark2 1451
HI351_write_cmos_sensor(0x30, 0x52);
HI351_write_cmos_sensor(0x31, 0x40); //Dark2 1452
HI351_write_cmos_sensor(0x32, 0x53);
HI351_write_cmos_sensor(0x33, 0x19); //Dark2 1453
HI351_write_cmos_sensor(0x34, 0x54);
HI351_write_cmos_sensor(0x35, 0x60); //Dark2 1454
HI351_write_cmos_sensor(0x36, 0x55);
HI351_write_cmos_sensor(0x37, 0x60); //Dark2 1455
HI351_write_cmos_sensor(0x38, 0x56);
HI351_write_cmos_sensor(0x39, 0x60); //Dark2 1456
HI351_write_cmos_sensor(0x3a, 0x57);
HI351_write_cmos_sensor(0x3b, 0x20); //Dark2 1457
HI351_write_cmos_sensor(0x3c, 0x58);
HI351_write_cmos_sensor(0x3d, 0x20); //Dark2 1458
HI351_write_cmos_sensor(0x3e, 0x59);
HI351_write_cmos_sensor(0x3f, 0x20); //Dark2 1459
HI351_write_cmos_sensor(0x40, 0x60);
HI351_write_cmos_sensor(0x41, 0x03); //Dark2 1460 skin opt en
HI351_write_cmos_sensor(0x42, 0x61);
HI351_write_cmos_sensor(0x43, 0xa0); //Dark2 1461
HI351_write_cmos_sensor(0x44, 0x62);
HI351_write_cmos_sensor(0x45, 0x98); //Dark2 1462
HI351_write_cmos_sensor(0x46, 0x63);
HI351_write_cmos_sensor(0x47, 0xe4); //Dark2 1463 skin_std_th_h
HI351_write_cmos_sensor(0x48, 0x64);
HI351_write_cmos_sensor(0x49, 0xa4); //Dark2 1464 skin_std_th_l
HI351_write_cmos_sensor(0x4a, 0x65);
HI351_write_cmos_sensor(0x4b, 0x7d); //Dark2 1465 sharp_std_th_h
HI351_write_cmos_sensor(0x4c, 0x66);
HI351_write_cmos_sensor(0x4d, 0x4b); //Dark2 1466 sharp_std_th_l
HI351_write_cmos_sensor(0x4e, 0x70);
HI351_write_cmos_sensor(0x4f, 0x10); //Dark2 1470
HI351_write_cmos_sensor(0x50, 0x71);
HI351_write_cmos_sensor(0x51, 0x10); //Dark2 1471
HI351_write_cmos_sensor(0x52, 0x72);
HI351_write_cmos_sensor(0x53, 0x10); //Dark2 1472
HI351_write_cmos_sensor(0x54, 0x73);
HI351_write_cmos_sensor(0x55, 0x10); //Dark2 1473
HI351_write_cmos_sensor(0x56, 0x74);
HI351_write_cmos_sensor(0x57, 0x10); //Dark2 1474
HI351_write_cmos_sensor(0x58, 0x75);
HI351_write_cmos_sensor(0x59, 0x10); //Dark2 1475
HI351_write_cmos_sensor(0x5a, 0x76);
HI351_write_cmos_sensor(0x5b, 0x28); //Dark2 1476
HI351_write_cmos_sensor(0x5c, 0x77);
HI351_write_cmos_sensor(0x5d, 0x28); //Dark2 1477
HI351_write_cmos_sensor(0x5e, 0x78);
HI351_write_cmos_sensor(0x5f, 0x28); //Dark2 1478
HI351_write_cmos_sensor(0x60, 0x79);
HI351_write_cmos_sensor(0x61, 0x28); //Dark2 1479
HI351_write_cmos_sensor(0x62, 0x7a);
HI351_write_cmos_sensor(0x63, 0x28); //Dark2 147a
HI351_write_cmos_sensor(0x64, 0x7b);
HI351_write_cmos_sensor(0x65, 0x28); //Dark2 147b
HI351_write_cmos_sensor(0x66, 0x03);
HI351_write_cmos_sensor(0x67, 0x10); //10 page
HI351_write_cmos_sensor(0x68, 0x60);
HI351_write_cmos_sensor(0x69, 0x03); //1060
HI351_write_cmos_sensor(0x6a, 0x70);
HI351_write_cmos_sensor(0x6b, 0x0c); //1070
HI351_write_cmos_sensor(0x6c, 0x71);
HI351_write_cmos_sensor(0x6d, 0x08); //1071
HI351_write_cmos_sensor(0x6e, 0x72);
HI351_write_cmos_sensor(0x6f, 0xe0); //1072
HI351_write_cmos_sensor(0x70, 0x73);
HI351_write_cmos_sensor(0x71, 0x7a); //1073
HI351_write_cmos_sensor(0x72, 0x74);
HI351_write_cmos_sensor(0x73, 0x36); //1074
HI351_write_cmos_sensor(0x74, 0x75);
HI351_write_cmos_sensor(0x75, 0x08); //1075
HI351_write_cmos_sensor(0x76, 0x76);
HI351_write_cmos_sensor(0x77, 0x10); //1076
HI351_write_cmos_sensor(0x78, 0x77);
HI351_write_cmos_sensor(0x79, 0x19); //1077
HI351_write_cmos_sensor(0x7a, 0x78);
HI351_write_cmos_sensor(0x7b, 0x99); //1078
HI351_write_cmos_sensor(0x7c, 0x79);
HI351_write_cmos_sensor(0x7d, 0x40); //1079
HI351_write_cmos_sensor(0x7e, 0x7a);
HI351_write_cmos_sensor(0x7f, 0x00); //107a
HI351_write_cmos_sensor(0x80, 0x7b);
HI351_write_cmos_sensor(0x81, 0x39); //107b
HI351_write_cmos_sensor(0x82, 0x7c);
HI351_write_cmos_sensor(0x83, 0x99); //107c
HI351_write_cmos_sensor(0x84, 0x7d);
HI351_write_cmos_sensor(0x85, 0x0e); //107d
HI351_write_cmos_sensor(0x86, 0x7e);
HI351_write_cmos_sensor(0x87, 0x1e); //107e
HI351_write_cmos_sensor(0x88, 0x7f);
HI351_write_cmos_sensor(0x89, 0x3c); //107f
HI351_write_cmos_sensor(0x8a, 0x03); //16 page
HI351_write_cmos_sensor(0x8b, 0x16);
HI351_write_cmos_sensor(0x8c, 0x8a); //168a
HI351_write_cmos_sensor(0x8d, 0x50);
HI351_write_cmos_sensor(0x8e, 0x8b); //168b
HI351_write_cmos_sensor(0x8f, 0x60);
HI351_write_cmos_sensor(0x90, 0x8c); //168c
HI351_write_cmos_sensor(0x91, 0x66);
HI351_write_cmos_sensor(0x92, 0x8d); //168d
HI351_write_cmos_sensor(0x93, 0x6e);
HI351_write_cmos_sensor(0x94, 0x8e); //168e
HI351_write_cmos_sensor(0x95, 0x78);
HI351_write_cmos_sensor(0x96, 0x8f); //168f
HI351_write_cmos_sensor(0x97, 0x7a);
HI351_write_cmos_sensor(0x98, 0x90); //1690
HI351_write_cmos_sensor(0x99, 0x7c);
HI351_write_cmos_sensor(0x9a, 0x91); //1691
HI351_write_cmos_sensor(0x9b, 0x7f);
HI351_write_cmos_sensor(0x9c, 0x92); //1692
HI351_write_cmos_sensor(0x9d, 0x7f);

HI351_write_cmos_sensor(0x0e, 0x00); //burst_end
// DMA End

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x01, 0xF1); //Sleep mode on

HI351_write_cmos_sensor(0x03, 0xc0);
HI351_write_cmos_sensor(0x16, 0x80); //MCU main roof holding off

HI351_write_cmos_sensor(0x03, 0xC0);
HI351_write_cmos_sensor(0x33, 0x01); //DMA hand shake mode set
HI351_write_cmos_sensor(0x32, 0x01); //DMA off
HI351_write_cmos_sensor(0x03, 0x30);
HI351_write_cmos_sensor(0x11, 0x04); //Bit[0]: MCU hold off

HI351_write_cmos_sensor(0x03, 0xc0);
HI351_write_cmos_sensor(0xe1, 0x00);

HI351_write_cmos_sensor(0x03, 0x30);
HI351_write_cmos_sensor(0x25, 0x0e);
HI351_write_cmos_sensor(0x25, 0x1e);
///////////////////////////////////////////
// 1F Page SSD
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x1f); //1F page
HI351_write_cmos_sensor(0x11, 0x00); //bit[5:4]: debug mode
HI351_write_cmos_sensor(0x12, 0x60);
HI351_write_cmos_sensor(0x13, 0x14);
HI351_write_cmos_sensor(0x14, 0x10);
HI351_write_cmos_sensor(0x15, 0x00);
HI351_write_cmos_sensor(0x20, 0x18); //ssd_x_start_pos
HI351_write_cmos_sensor(0x21, 0x14); //ssd_y_start_pos
HI351_write_cmos_sensor(0x22, 0x8C); //ssd_blk_width
HI351_write_cmos_sensor(0x23, 0x9A); //for Oneline 2200 0x9C -> 0x9A,ssd_blk_height 
HI351_write_cmos_sensor(0x28, 0x18);
HI351_write_cmos_sensor(0x29, 0x02);
HI351_write_cmos_sensor(0x3B, 0x18);
HI351_write_cmos_sensor(0x3C, 0x8C);
HI351_write_cmos_sensor(0x10, 0x19); //SSD enable

HI351_write_cmos_sensor(0x03, 0xc4); //AE en
HI351_write_cmos_sensor(0x66, 0x08); //AE stat
HI351_write_cmos_sensor(0x67, 0x00); //AE stat
HI351_write_cmos_sensor(0x10, 0xff); //AE ON & Reset
HI351_write_cmos_sensor(0x03, 0xc3); //AE Static en
HI351_write_cmos_sensor(0x10, 0x84);

mdelay(20);

///////////////////////////////////////////
// 30 Page DMA address set
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x30); //DMA
HI351_write_cmos_sensor(0x7c, 0x2c); //Extra str
HI351_write_cmos_sensor(0x7d, 0xce);
HI351_write_cmos_sensor(0x7e, 0x2d); //Extra end
HI351_write_cmos_sensor(0x7f, 0xbb);
HI351_write_cmos_sensor(0x80, 0x24); //Outdoor str
HI351_write_cmos_sensor(0x81, 0x70);
HI351_write_cmos_sensor(0x82, 0x27); //Outdoor end
HI351_write_cmos_sensor(0x83, 0x39);
HI351_write_cmos_sensor(0x84, 0x21); //Indoor str
HI351_write_cmos_sensor(0x85, 0xa6);
HI351_write_cmos_sensor(0x86, 0x24); //Indoor end
HI351_write_cmos_sensor(0x87, 0x6f);
HI351_write_cmos_sensor(0x88, 0x27); //Dark1 str
HI351_write_cmos_sensor(0x89, 0x3a);
HI351_write_cmos_sensor(0x8a, 0x2a); //Dark1 end
HI351_write_cmos_sensor(0x8b, 0x03);
HI351_write_cmos_sensor(0x8c, 0x2a); //Dark2 str
HI351_write_cmos_sensor(0x8d, 0x04);
HI351_write_cmos_sensor(0x8e, 0x2c); //Dark2 end
HI351_write_cmos_sensor(0x8f, 0xcd);



/////////////////////////////////////////////
// CD Page (Color ratio)
/////////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0xCD);//
HI351_write_cmos_sensor(0x10, 0x38);//ColorRatio disable


HI351_write_cmos_sensor(0x03, 0xc9); //AWB Start Point
HI351_write_cmos_sensor(0x2a, 0x00);
HI351_write_cmos_sensor(0x2b, 0xb2);
HI351_write_cmos_sensor(0x2c, 0x00);
HI351_write_cmos_sensor(0x2d, 0x82);
HI351_write_cmos_sensor(0x2e, 0x00);
HI351_write_cmos_sensor(0x2f, 0xb2);
HI351_write_cmos_sensor(0x30, 0x00);
HI351_write_cmos_sensor(0x31, 0x82);

HI351_write_cmos_sensor(0x03, 0xc5); //AWB en
HI351_write_cmos_sensor(0x10, 0xb1);

HI351_write_cmos_sensor(0x03, 0xcf); //Adative en
HI351_write_cmos_sensor(0x10, 0xaf); //

///////////////////////////////////////////
// 48 Page MIPI setting
///////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x48);
HI351_write_cmos_sensor(0x09, 0xa6); //MIPI CLK
HI351_write_cmos_sensor(0x10, 0x1C); //MIPI ON
HI351_write_cmos_sensor(0x11, 0x00); //Normal Mode //[4] '1' contenous, '0'non-contenous
HI351_write_cmos_sensor(0x14, 0x50); //Skew
HI351_write_cmos_sensor(0x16, 0x04);

HI351_write_cmos_sensor(0x1a, 0x11);
HI351_write_cmos_sensor(0x1b, 0x0d); //Short Packet
HI351_write_cmos_sensor(0x1c, 0x01); //Control DP
HI351_write_cmos_sensor(0x1d, 0x0f); //Control DN
HI351_write_cmos_sensor(0x1e, 0x09);
//HI351_write_cmos_sensor(0x1f, 0x04); // 20120305 LSW 0x07);
HI351_write_cmos_sensor(0x1f, 0x05);
HI351_write_cmos_sensor(0x20, 0x00);
HI351_write_cmos_sensor(0x24, 0x1e); //Bayer8 : 2a, Bayer10 : 2b, YUV : 1e

HI351_write_cmos_sensor(0x30, 0x00); //2048*2
HI351_write_cmos_sensor(0x31, 0x05);
//HI351_write_cmos_sensor(0x32, 0x0f); // Tclk zero
//HI351_write_cmos_sensor(0x32, 0x0b); //tck 00 384Mbps 0b
//HI351_write_cmos_sensor(0x34, 0x03); // Tclk prepare 384Mbps 03
HI351_write_cmos_sensor(0x34, 0x04); // Tclk prepare 432Mbps
//HI351_write_cmos_sensor(0x35, 0x04); //20120305 LSW 0x06(default)

HI351_write_cmos_sensor(0x39, 0x03); //Drivability 00

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x0c, 0xf0); //Parallel Line off

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x11, 0x83); // STEVE 0frame skip, XY flip
HI351_write_cmos_sensor(0x01, 0xf0); //sleep off

HI351_write_cmos_sensor(0x03, 0xC0);
HI351_write_cmos_sensor(0x33, 0x00);
HI351_write_cmos_sensor(0x32, 0x01); //DMA on

//////////////////////////////////////////////
// Delay
//////////////////////////////////////////////
HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x03, 0x00);


}

#if 0
kal_uint32 HI351_read_shutter(void)
{
	//kal_uint8 temp_reg0, temp_reg1, temp_reg2;
	kal_uint32 shutter;

	return shutter;
}
#endif   
#if 0
static void HI351_write_shutter(kal_uint16 shutter)
{
		
}    
#endif
void HI351_night_mode(kal_bool enable)	
{

if (HI351_sensor_cap_state == KAL_TRUE) 
{
return ;	
}

if (enable) 
{
if (HI351_VEDIO_encode_mode == KAL_TRUE) 
{
HI351_write_cmos_sensor(0x03, 0x10);
HI351_write_cmos_sensor(0x41, 0x20);
}
else 
{
HI351_write_cmos_sensor(0x03, 0x10);
HI351_write_cmos_sensor(0x41, 0x20);
}
} 
else 
{
if (HI351_VEDIO_encode_mode == KAL_TRUE) 
{
HI351_write_cmos_sensor(0x03, 0x10);
HI351_write_cmos_sensor(0x41, 0x00);
}
else 
{
HI351_write_cmos_sensor(0x03, 0x10);
HI351_write_cmos_sensor(0x41, 0x00);
}
}
}

#if 0
static void HI351_set_XGA_mode(void)
{

	return;

}
#endif
void HI351_Initial_Cmds(void)
{
	//kal_uint16 i,cnt;
    HI351_Init_Cmds(); 
}

//static void HI351_set_mirror_flip(kal_uint8 image_mirror);

UINT32 HI351Open(void)
{
	volatile signed char i;
	kal_uint32 sensor_id=0;
	//kal_uint8 temp_sccb_addr = 0;

	HI351_write_cmos_sensor(0x03, 0x00);
	HI351_write_cmos_sensor(0x01, 0xf1);
	HI351_write_cmos_sensor(0x01, 0xf3);
	HI351_write_cmos_sensor(0x01, 0xf1);

	HI351_write_cmos_sensor(0x01, 0xf1);
	HI351_write_cmos_sensor(0x01, 0xf3);
	HI351_write_cmos_sensor(0x01, 0xf1);
    
    do{
        for (i=0; i < 3; i++)
        {

            sensor_id = HI351_read_cmos_sensor(0x04);
        
            if (sensor_id == HI351_SENSOR_ID)
            {
#ifdef HI351_DEBUG
                printk("[HI351YUV]:Read Sensor ID succ:0x%x\n", sensor_id);  
#endif
                break;
            }
        }

        mdelay(20);
    }while(0);


    if (sensor_id != HI351_SENSOR_ID)
	{
	
#ifdef HI351_DEBUG
	    printk("[HI351YUV]:Read Sensor ID fail:0x%x\n", sensor_id);  
#endif
		return ERROR_SENSOR_CONNECT_FAIL;
	}

#ifdef HI351_DEBUG
        printk("[HI351YUV]:Read Sensor ID pass:0x%x\n", sensor_id);
#endif

	HI351_Initial_Cmds();
   // HI351_set_mirror_flip(3);
     
	return ERROR_NONE;
}	

UINT32 HI351Close(void)
{
	//CISModulePowerOn(FALSE);
	//kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM) g_currDualSensorIdx, g_currSensorName,false, CAMERA_HW_DRVNAME);
	return ERROR_NONE;
}	
#if 0
static void HI351_set_mirror_flip(kal_uint8 image_mirror)
{
    kal_uint8 HI351_HV_Mirror;

    HI351_write_cmos_sensor(0x03,0x00); 	
	HI351_HV_Mirror = (HI351_read_cmos_sensor(0x11) & 0xfc);

    switch (image_mirror) {
    case IMAGE_NORMAL:
    	HI351_HV_Mirror |= 0x00; 
        break;
    case IMAGE_H_MIRROR:
        HI351_HV_Mirror |= 0x02;
        break;
    case IMAGE_V_MIRROR:
        HI351_HV_Mirror |= 0x01; 
        break;
    case IMAGE_HV_MIRROR:
        HI351_HV_Mirror |= 0x03;
        break;
    default:
        break;
    }
	HI351_write_cmos_sensor(0x11, HI351_HV_Mirror);


}
#endif
UINT32 HI351Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{	
kal_uint16  iStartX = 0, iStartY = 0;

HI351_sensor_cap_state = KAL_FALSE;
HI351_sensor_pclk=390;

HI351_gPVmode = KAL_TRUE;

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x01, 0xf1); //Sleep on

HI351_write_cmos_sensor(0x03, 0x30); //DMA&Adaptive Off
HI351_write_cmos_sensor(0x36, 0xa3);
mdelay(10);

HI351_write_cmos_sensor(0x03, 0xc4); //AE off
HI351_write_cmos_sensor(0x10, 0x60);
HI351_write_cmos_sensor(0x03, 0xc5); //AWB off
HI351_write_cmos_sensor(0x10, 0x30);

//preview mode off
HI351_write_cmos_sensor(0x03, 0x15);  //Shading
HI351_write_cmos_sensor(0x10, 0x81);  //
HI351_write_cmos_sensor(0x20, 0x08);  //Shading Width 2048
HI351_write_cmos_sensor(0x21, 0x00);
HI351_write_cmos_sensor(0x22, 0x06);  //Shading Height 768
HI351_write_cmos_sensor(0x23, 0x00);
HI351_write_cmos_sensor(0x03, 0x19);
HI351_write_cmos_sensor(0x10, 0x87);//MODE_ZOOM
HI351_write_cmos_sensor(0x11, 0x00);//MODE_ZOOM2
HI351_write_cmos_sensor(0x12, 0x06);//ZOOM_CONFIG
HI351_write_cmos_sensor(0x13, 0x01);//Test Setting

HI351_write_cmos_sensor(0x20, 0x05);//ZOOM_DST_WIDTH_H
HI351_write_cmos_sensor(0x21, 0x00);//ZOOM_DST_WIDTH_L
HI351_write_cmos_sensor(0x22, 0x03);//ZOOM_DST_HEIGHT_H
HI351_write_cmos_sensor(0x23, 0xc0);//ZOOM_DST_HEIGHT_L
HI351_write_cmos_sensor(0x24, 0x00);//ZOOM_WIN_STX_H
HI351_write_cmos_sensor(0x25, 0x00);//ZOOM_WIN_STX_L	 // STEVE00 3-) 1
HI351_write_cmos_sensor(0x26, 0x00);//ZOOM_WIN_STY_H
HI351_write_cmos_sensor(0x27, 0x00);//ZOOM_WIN_STY_L
HI351_write_cmos_sensor(0x28, 0x05);//ZOOM_WIN_ENX_H
HI351_write_cmos_sensor(0x29, 0x00);//ZOOM_WIN_ENX_L	 //STEVE00 83 -) 81
HI351_write_cmos_sensor(0x2a, 0x03);//ZOOM_WIN_ENY_H
HI351_write_cmos_sensor(0x2b, 0xc0);//ZOOM_WIN_ENY_L
HI351_write_cmos_sensor(0x2c, 0x0c);//ZOOM_VER_STEP_H
HI351_write_cmos_sensor(0x2d, 0xcc);//ZOOM_VER_STEP_L
HI351_write_cmos_sensor(0x2e, 0x0c);//ZOOM_HOR_STEP_H
HI351_write_cmos_sensor(0x2f, 0xcc);//ZOOM_HOR_STEP_L
HI351_write_cmos_sensor(0x30, 0x7c);//ZOOM_FIFO_DELAY


HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x10, 0x10); //Sub1/2 + Pre1
HI351_write_cmos_sensor(0x11, 0x93); // STEVE 0 skip Fix Frame Off, XY Flip
HI351_write_cmos_sensor(0x13, 0x80); //Fix AE Set Off
HI351_write_cmos_sensor(0x14, 0x20);
HI351_write_cmos_sensor(0x17, 0x04); // for Preview

HI351_write_cmos_sensor(0x20, 0x00); //Start Height
HI351_write_cmos_sensor(0x21, 0x04);
HI351_write_cmos_sensor(0x22, 0x00); //Start Width
HI351_write_cmos_sensor(0x23, 0x0a);

HI351_write_cmos_sensor(0x24, 0x06);
HI351_write_cmos_sensor(0x25, 0x00);
HI351_write_cmos_sensor(0x26, 0x08);
HI351_write_cmos_sensor(0x27, 0x00);

HI351_write_cmos_sensor(0x03, 0xFE);
HI351_write_cmos_sensor(0xFE, 0x2c); //Delay 10ms //important
mdelay(10);

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x20, 0x00);
HI351_write_cmos_sensor(0x21, 0x01); //preview row start set.

HI351_write_cmos_sensor(0x03, 0x48);
HI351_write_cmos_sensor(0x10, 0x1C); //MIPI On
HI351_write_cmos_sensor(0x16, 0x04);
HI351_write_cmos_sensor(0x30, 0x00); //640 * 2
HI351_write_cmos_sensor(0x31, 0x08);

HI351_write_cmos_sensor(0x03, 0x30);
HI351_write_cmos_sensor(0x36, 0x28); //Preview set //important

HI351_write_cmos_sensor(0x03, 0xFE);
HI351_write_cmos_sensor(0xFE, 0x2c); //Delay 10ms
mdelay(10);

HI351_write_cmos_sensor(0x03, 0x11);
HI351_write_cmos_sensor(0xf0, 0x20); //jktest 0203//for longhceer aw551//0x0d); // STEVE Dark mode for Sawtooth

HI351_write_cmos_sensor(0x03, 0x19);  //Shading
HI351_write_cmos_sensor(0x10, 0x00);



HI351_write_cmos_sensor(0x03, 0xc4); //AE en
HI351_write_cmos_sensor(0x10, 0xef); //JH guide 0xea); //50hz//0xe1);//auto

HI351_write_cmos_sensor(0x03, 0xFE);
HI351_write_cmos_sensor(0xFE, 0x2c); //Delay 20ms
mdelay(20);

HI351_write_cmos_sensor(0x03, 0xc5); //AWB en
HI351_write_cmos_sensor(0x10, 0xb1);

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x01, 0xf0); //sleep off

HI351_write_cmos_sensor(0x03, 0xcf); //Adaptive On
HI351_write_cmos_sensor(0x10, 0xaf);

HI351_write_cmos_sensor(0x03, 0xc0);
HI351_write_cmos_sensor(0x33, 0x00);
HI351_write_cmos_sensor(0x32, 0x01); //DMA On

image_window->GrabStartX = iStartX;
image_window->GrabStartY = iStartY;
image_window->ExposureWindowWidth = HI351_IMAGE_SENSOR_PV_WIDTH - 16;
image_window->ExposureWindowHeight = HI351_IMAGE_SENSOR_PV_HEIGHT - 12;
msleep(10);
// copy sensor_config_data
memcpy(&HI351SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
return ERROR_NONE;
}	

UINT32 HI351Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
//kal_uint8 temp_AE_reg;
//kal_uint8 CLK_DIV_REG = 0;

/* 2M FULL Mode */
HI351_gPVmode = KAL_FALSE;

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x01, 0xf1); //Sleep on

HI351_write_cmos_sensor(0x03, 0x30); //DMA & Adaptive Off
HI351_write_cmos_sensor(0x36, 0xa3);//delay 10ms
mdelay(10);

HI351_write_cmos_sensor(0x03, 0xc4); //AE off
HI351_write_cmos_sensor(0x10, 0x68);
HI351_write_cmos_sensor(0x03, 0xc5); //AWB off
HI351_write_cmos_sensor(0x10, 0x30);

HI351_write_cmos_sensor(0x03, 0x19); //Scaler Off
HI351_write_cmos_sensor(0x10, 0x00);

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x10, 0x00); //Full
HI351_write_cmos_sensor(0x14, 0x20);
HI351_write_cmos_sensor(0x17, 0x05); // for Full


HI351_write_cmos_sensor(0x20, 0x00); //Start Height
HI351_write_cmos_sensor(0x21, 0x04);
HI351_write_cmos_sensor(0x22, 0x00); //Start Width
HI351_write_cmos_sensor(0x23, 0x0a);

HI351_write_cmos_sensor(0x24, 0x06);
HI351_write_cmos_sensor(0x25, 0x00);
HI351_write_cmos_sensor(0x26, 0x08);
HI351_write_cmos_sensor(0x27, 0x00);

HI351_write_cmos_sensor(0x03, 0xFE);
HI351_write_cmos_sensor(0xFE, 0x2c); //Delay 10ms
mdelay(10);

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x20, 0x00);
HI351_write_cmos_sensor(0x21, 0x03); //preview row start set.

HI351_write_cmos_sensor(0x03, 0x14);
HI351_write_cmos_sensor(0x10, 0x2f); //sharp on

HI351_write_cmos_sensor(0x03, 0x15); //Shading
HI351_write_cmos_sensor(0x10, 0x83); // 00 - 83 LSC ON
HI351_write_cmos_sensor(0x20, 0x08);
HI351_write_cmos_sensor(0x21, 0x00);
HI351_write_cmos_sensor(0x22, 0x06);
HI351_write_cmos_sensor(0x23, 0x00);

HI351_write_cmos_sensor(0x03, 0x48); //MIPI Setting
HI351_write_cmos_sensor(0x10, 0x1C);
HI351_write_cmos_sensor(0x16, 0x04);
HI351_write_cmos_sensor(0x30, 0x00); //2048 * 2
HI351_write_cmos_sensor(0x31, 0x10);


HI351_write_cmos_sensor(0x03, 0x30);
HI351_write_cmos_sensor(0x36, 0x29); //Capture

HI351_write_cmos_sensor(0x03, 0xFE);
HI351_write_cmos_sensor(0xFE, 0x2c);
mdelay(20);

HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x01, 0xf0); //sleep off

image_window->GrabStartX=1;
image_window->GrabStartY=1;
image_window->ExposureWindowWidth=HI351_IMAGE_SENSOR_FULL_WIDTH - 16;
image_window->ExposureWindowHeight=HI351_IMAGE_SENSOR_FULL_HEIGHT - 12;

mdelay(10);
// copy sensor_config_data
memcpy(&HI351SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

return ERROR_NONE;
}

UINT32 HI351GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth=HI351_FULL_GRAB_WIDTH;  
	pSensorResolution->SensorFullHeight=HI351_FULL_GRAB_HEIGHT;
	pSensorResolution->SensorPreviewWidth=HI351_PV_GRAB_WIDTH;
	pSensorResolution->SensorPreviewHeight=HI351_PV_GRAB_HEIGHT;

	return ERROR_NONE;
}	

UINT32 HI351GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX=HI351_PV_GRAB_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=HI351_PV_GRAB_HEIGHT;
	pSensorInfo->SensorFullResolutionX=HI351_FULL_GRAB_WIDTH;
	pSensorInfo->SensorFullResolutionY=HI351_FULL_GRAB_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;

	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;

	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorInterruptDelayLines = 1;
#ifdef MIPI_INTERFACE
	pSensorInfo->SensroInterfaceType = SENSOR_INTERFACE_TYPE_MIPI;
#else
	pSensorInfo->SensroInterfaceType = SENSOR_INTERFACE_TYPE_PARALLEL;
#endif

	pSensorInfo->CaptureDelayFrame = 2; 
	pSensorInfo->PreviewDelayFrame = 3; 
	pSensorInfo->VideoDelayFrame = 20; 
	pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;   	
	
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
		pSensorInfo->SensorClockFreq=24;
		pSensorInfo->SensorClockDividCount=	3;
		pSensorInfo->SensorClockRisingCount= 0;
		pSensorInfo->SensorClockFallingCount= 2;
		pSensorInfo->SensorPixelClockCount= 3;
		pSensorInfo->SensorDataLatchCount= 2;
                /*ergate-004*/
		pSensorInfo->SensorGrabStartX = HI351_PV_GRAB_START_X;//0; 
		pSensorInfo->SensorGrabStartY = HI351_PV_GRAB_START_Y;//0;    
	#ifdef MIPI_INTERFACE
        pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
        pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
        pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
        pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
        pSensorInfo->SensorPacketECCOrder = 1;
	#endif
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		//case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		pSensorInfo->SensorClockFreq=24;
		pSensorInfo->SensorClockDividCount=	3;
		pSensorInfo->SensorClockRisingCount= 0;
		pSensorInfo->SensorClockFallingCount= 2;
		pSensorInfo->SensorPixelClockCount= 3;
		pSensorInfo->SensorDataLatchCount= 2;
                /*ergate-004*/
		pSensorInfo->SensorGrabStartX = HI351_FULL_GRAB_START_X;//0; 
		pSensorInfo->SensorGrabStartY = HI351_FULL_GRAB_START_Y;//0;  
	#ifdef MIPI_INTERFACE
        pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
        pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
        pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
        pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
        pSensorInfo->SensorPacketECCOrder = 1;
	#endif			
		break;
		default:
		pSensorInfo->SensorClockFreq=24;
		pSensorInfo->SensorClockDividCount=3;
		pSensorInfo->SensorClockRisingCount=0;
		pSensorInfo->SensorClockFallingCount=2;
		pSensorInfo->SensorPixelClockCount=3;
		pSensorInfo->SensorDataLatchCount=2;
                /*ergate-004*/
		pSensorInfo->SensorGrabStartX = HI351_PV_GRAB_START_X;//0; 
		pSensorInfo->SensorGrabStartY = HI351_PV_GRAB_START_Y;//0;     			
		break;
	}
	//HI351_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &HI351SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	

UINT32 HI351Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId)
	{
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
	//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
	HI351Preview(pImageWindow, pSensorConfigData);
	break;
	
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	//case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
	HI351Capture(pImageWindow, pSensorConfigData);
	break;
	
	default:
	return ERROR_INVALID_SCENARIO_ID;
	}
	return TRUE;
}	

BOOL HI351_set_param_wb(UINT16 para)
{
	switch (para)
	{
    case AWB_MODE_AUTO:
		HI351_write_cmos_sensor(0x03, 0xc5); /*Page c5*/
		HI351_write_cmos_sensor(0x11, 0xa4); /*adaptive on*/
		HI351_write_cmos_sensor(0x12, 0x97); /*adaptive on*/
		HI351_write_cmos_sensor(0x26, 0x00); /*indoor agl max*/
		HI351_write_cmos_sensor(0x27, 0xeb);
		HI351_write_cmos_sensor(0x28, 0x00);  /*indoor agl min*/
		HI351_write_cmos_sensor(0x29, 0x55);
		HI351_write_cmos_sensor(0x03, 0xc6); /*Page c6*/
		HI351_write_cmos_sensor(0x18, 0x46); /*RgainMin*/
		HI351_write_cmos_sensor(0x19, 0x90); /*RgainMax*/
		HI351_write_cmos_sensor(0x1a, 0x40); /*BgainMin*/
		HI351_write_cmos_sensor(0x1b, 0xa4); /*BgainMax*/
		HI351_write_cmos_sensor(0x03, 0xc5);
		HI351_write_cmos_sensor(0x10, 0xb1);
        break;
    case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy//YINGTIAN
		HI351_write_cmos_sensor(0x03, 0xc5); /*Page c5*/
		HI351_write_cmos_sensor(0x11, 0xa0); /*adaptive off*/
		HI351_write_cmos_sensor(0x12, 0x07); /*adaptive off*/
		HI351_write_cmos_sensor(0x26, 0x00); /*indoor agl max*/
		HI351_write_cmos_sensor(0x27, 0xff);
		HI351_write_cmos_sensor(0x28, 0x00);  /*indoor agl min*/
		HI351_write_cmos_sensor(0x29, 0xf0);
		HI351_write_cmos_sensor(0x03, 0xc6); /*Page c6*/
		HI351_write_cmos_sensor(0x18, 0x58); /*indoor R gain Min*/
		HI351_write_cmos_sensor(0x19, 0x78); /*indoor R gain Max*/
		HI351_write_cmos_sensor(0x1a, 0x60); /*indoor B gain Min*/
		HI351_write_cmos_sensor(0x1b, 0x78); /*indoor B gain Max*/
		HI351_write_cmos_sensor(0x03, 0xc5);
		HI351_write_cmos_sensor(0x10, 0xb1);
        break;
    case AWB_MODE_DAYLIGHT: //sunny//RIGUANG
		HI351_write_cmos_sensor(0x03, 0xc5); /*Page c5*/
		HI351_write_cmos_sensor(0x11, 0xa0); /*adaptive off*/
		HI351_write_cmos_sensor(0x12, 0x07); /*adaptive off*/
		HI351_write_cmos_sensor(0x26, 0x00); /*indoor agl max*/
		HI351_write_cmos_sensor(0x27, 0xf0);
		HI351_write_cmos_sensor(0x28, 0x00);  /*indoor agl min*/
		HI351_write_cmos_sensor(0x29, 0xc8);
		HI351_write_cmos_sensor(0x03, 0xc6); /*Page c6*/
		HI351_write_cmos_sensor(0x18, 0x58); /*indoor R gain Min*/
		HI351_write_cmos_sensor(0x19, 0x5b); /*indoor R gain Max*/
		HI351_write_cmos_sensor(0x1a, 0x76); /*indoor B gain Min*/
		HI351_write_cmos_sensor(0x1b, 0x7a); /*indoor B gain Max*/
		HI351_write_cmos_sensor(0x03, 0xc5);
		HI351_write_cmos_sensor(0x10, 0xb1);
        break;
    case AWB_MODE_INCANDESCENT: //office//BAICHIDENG
		HI351_write_cmos_sensor(0x03, 0xc5); /*Page c5*/
		HI351_write_cmos_sensor(0x11, 0xa0); /*adaptive off*/
		HI351_write_cmos_sensor(0x12, 0x07); /*adaptive off*/
		HI351_write_cmos_sensor(0x26, 0x00); /*indoor agl max*/
		HI351_write_cmos_sensor(0x27, 0x82);
		HI351_write_cmos_sensor(0x28, 0x00);  /*indoor agl min*/
		HI351_write_cmos_sensor(0x29, 0x50);
		HI351_write_cmos_sensor(0x03, 0xc6); /*Page c6*/
		HI351_write_cmos_sensor(0x18, 0x44); /*indoor R gain Min*/
		HI351_write_cmos_sensor(0x19, 0x90); /*indoor R gain Max*/
		HI351_write_cmos_sensor(0x1a, 0x40); /*indoor B gain Min*/
		HI351_write_cmos_sensor(0x1b, 0xa4); /*indoor B gain Max*/
		HI351_write_cmos_sensor(0x03, 0xc5);
		HI351_write_cmos_sensor(0x10, 0xb1);
        break;
    case AWB_MODE_TUNGSTEN: //home
        break;
    case AWB_MODE_FLUORESCENT://YINGGUANGDENG
		HI351_write_cmos_sensor(0x03, 0xc5); /*Page c5*/
		HI351_write_cmos_sensor(0x11, 0xa0); /*adaptive off*/
		HI351_write_cmos_sensor(0x12, 0x07); /*adaptive off*/
		HI351_write_cmos_sensor(0x26, 0x00); /*indoor agl max*/
		HI351_write_cmos_sensor(0x27, 0xb4);
		HI351_write_cmos_sensor(0x28, 0x00);  /*indoor agl min*/
		HI351_write_cmos_sensor(0x29, 0x82);
		HI351_write_cmos_sensor(0x03, 0xc6); /*Page c6*/
		HI351_write_cmos_sensor(0x18, 0x44); /*indoor R gain Min*/
		HI351_write_cmos_sensor(0x19, 0x90); /*indoor R gain Max*/
		HI351_write_cmos_sensor(0x1a, 0x40); /*indoor B gain Min*/
		HI351_write_cmos_sensor(0x1b, 0xa4); /*indoor B gain Max*/
		HI351_write_cmos_sensor(0x03, 0xc5);
		HI351_write_cmos_sensor(0x10, 0xb1);
        break;	
	default:
	    return FALSE;
	}
	return TRUE;



}

BOOL HI351_set_param_effect(UINT16 para)
{

kal_uint32 ret = KAL_TRUE;
  switch (para)
  {
	case MEFFECT_OFF:
		HI351_write_cmos_sensor(0x03, 0x10);
		HI351_write_cmos_sensor(0x11, 0x03); 
		HI351_write_cmos_sensor(0x12, 0xf0); /*constant off	*/
		HI351_write_cmos_sensor(0x42, 0x00); /*cb_offset	   */
		HI351_write_cmos_sensor(0x43, 0x00); /*cr_offset	   */
		HI351_write_cmos_sensor(0x44, 0x80); /*cb_constant   */
		HI351_write_cmos_sensor(0x45, 0x80); /*cr_constant   */
	break;

	case MEFFECT_SEPIA:
		HI351_write_cmos_sensor(0x03, 0x10);
		HI351_write_cmos_sensor(0x11, 0x03); /*solar off		  */
		HI351_write_cmos_sensor(0x12, 0xf3); /*constant on  */
		HI351_write_cmos_sensor(0x42, 0x00); /*cb_offset		  */
		HI351_write_cmos_sensor(0x43, 0x00); /*cr_offset		  */
		HI351_write_cmos_sensor(0x44, 0x60); /*cb_constant  */
		HI351_write_cmos_sensor(0x45, 0xa3); /*cr_constant  */
	break;	

	case MEFFECT_NEGATIVE:
		HI351_write_cmos_sensor(0x03, 0x10);
		HI351_write_cmos_sensor(0x11, 0x03); /*solar off		 */
		HI351_write_cmos_sensor(0x12, 0xf8); /*negative on	*/
		HI351_write_cmos_sensor(0x42, 0x00); /*cb_offset		 */
		HI351_write_cmos_sensor(0x43, 0x00); /*cr_offset		 */
		HI351_write_cmos_sensor(0x44, 0x80); /*cb_constant	*/
		HI351_write_cmos_sensor(0x45, 0x80); /*cr_constant	*/
	break; 

	case MEFFECT_SEPIAGREEN:	
		HI351_write_cmos_sensor(0x03, 0x10);
		HI351_write_cmos_sensor(0x11, 0x03); /*solar off		  */
		HI351_write_cmos_sensor(0x12, 0xf3); /*constant on  */
		HI351_write_cmos_sensor(0x42, 0x00); /*cb_offset		  */
		HI351_write_cmos_sensor(0x43, 0x00); /*cr_offset		  */
		HI351_write_cmos_sensor(0x44, 0x60); /*cb_constant  */
		HI351_write_cmos_sensor(0x45, 0xa3); /*cr_constant  */
		HI351_write_cmos_sensor(0x03, 0x14); /*			  */
		HI351_write_cmos_sensor(0x80, 0x20); /*effect_ctl1_off */
	break;

	case MEFFECT_SEPIABLUE:
		HI351_write_cmos_sensor(0x03, 0x10);
		HI351_write_cmos_sensor(0x11, 0x03); /*solar off		  */
		HI351_write_cmos_sensor(0x12, 0xf3); /*constant on  */
		HI351_write_cmos_sensor(0x42, 0x00); /*cb_offset		  */
		HI351_write_cmos_sensor(0x43, 0x00); /*cr_offset		  */
		HI351_write_cmos_sensor(0x44, 0xc0); /*cb_constant  */
		HI351_write_cmos_sensor(0x45, 0x80); /*cr_constant  */
		HI351_write_cmos_sensor(0x03, 0x14); /*			  */
		HI351_write_cmos_sensor(0x80, 0x20); /*effect_ctl1_off */
	break;

	case MEFFECT_MONO:		
		HI351_write_cmos_sensor(0x03, 0x10);
		HI351_write_cmos_sensor(0x11, 0x03); /*solar off		*/
		HI351_write_cmos_sensor(0x12, 0xf3); /*constant on	*/
		HI351_write_cmos_sensor(0x42, 0x00); /*cb_offset		*/
		HI351_write_cmos_sensor(0x43, 0x00); /*cr_offset		*/
		HI351_write_cmos_sensor(0x44, 0x80); /*cb_constant	*/
		HI351_write_cmos_sensor(0x45, 0x80); /*cr_constant	*/
	break;

	default:
	ret = FALSE;
	}

	return ret;



}

BOOL HI351_set_param_banding(UINT16 para)
{
    //kal_uint8 banding;

	switch (para)
	{
	case AE_FLICKER_MODE_50HZ:
		HI351_Banding_setting = AE_FLICKER_MODE_50HZ;
		HI351_write_cmos_sensor(0x03,0x20);
		HI351_write_cmos_sensor(0x10,0x97);
	break;
	case AE_FLICKER_MODE_60HZ:		  
		HI351_Banding_setting = AE_FLICKER_MODE_60HZ;
		HI351_write_cmos_sensor(0x03,0x20);
		HI351_write_cmos_sensor(0x10,0x87);		   
	break;
	default:
	return FALSE;
	}

    return TRUE;
} 

BOOL HI351_set_param_exposure(UINT16 para)
{
 
  
  HI351_write_cmos_sensor(0x03,0x10);
  HI351_write_cmos_sensor(0x12,(HI351_read_cmos_sensor(0x12)|0x10));//make sure the Yoffset control is opened.
  
  switch (para)
  {
  case AE_EV_COMP_n13:
	  HI351_write_cmos_sensor(0x40,0x30);
	  break;
  case AE_EV_COMP_n10:
	  HI351_write_cmos_sensor(0x40,0x30);
	  break;
  case AE_EV_COMP_n07:
	  HI351_write_cmos_sensor(0x40,0x30);
	  break;
  case AE_EV_COMP_n03:
	  HI351_write_cmos_sensor(0x40,0x30);
	  break;
  case AE_EV_COMP_00:
	  HI351_write_cmos_sensor(0x40,0x30);
	  break;
  case AE_EV_COMP_03:
	  HI351_write_cmos_sensor(0x40,0x30);
	  break;
  case AE_EV_COMP_07:
	  HI351_write_cmos_sensor(0x40,0x30);
	  break;
  case AE_EV_COMP_10:
	  HI351_write_cmos_sensor(0x40,0x30);
	  break;
  case AE_EV_COMP_13:
	  HI351_write_cmos_sensor(0x40,0x30);
	  break;
  default:
	  return FALSE;
  }
  

    return TRUE;
	
} 


UINT32 HI351YUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
  if (HI351_op_state.sensor_cap_state == KAL_TRUE)	/* Don't need it when capture mode. */
	{
		return KAL_TRUE;
	}

	switch (iCmd) 
	{
	case FID_SCENE_MODE:	    
	if (iPara == SCENE_MODE_OFF)
	{
	HI351_night_mode(0); 
	}
	else if (iPara == SCENE_MODE_NIGHTSCENE)
	{
	HI351_night_mode(1); 
	}	    
	break; 
	
	case FID_AWB_MODE:
	HI351_set_param_wb(iPara);
	break;
	
	case FID_COLOR_EFFECT:	    
	HI351_set_param_effect(iPara);
	break;
	
	case FID_AE_EV:	       	      
	HI351_set_param_exposure(iPara);
	break;
	
	case FID_AE_FLICKER:    	    	    
	HI351_set_param_banding(iPara);
	break;
	
	case FID_AE_SCENE_MODE: 
	break; 

	case FID_ZOOM_FACTOR:
	zoom_factor = iPara; 		
	break; 
	
	default:
	break;
	}
	return TRUE;
}   

UINT32 HI351YUVSetVideoMode(UINT16 u2FrameRate)
{
	kal_uint16 temp_AE_reg = 0;

	return TRUE;
	HI351_write_cmos_sensor(0x03, 0x20); 
	temp_AE_reg = HI351_read_cmos_sensor(0x10);


HI351_write_cmos_sensor(0x03, 0x00);
HI351_write_cmos_sensor(0x13, 0xa8); //Fix AE Set on
HI351_write_cmos_sensor(0x11, 0x97); //Fix On

    HI351_VEDIO_encode_mode = KAL_TRUE; 

  
    return TRUE;
}

UINT32 HI351GetSensorID(UINT32 *sensorID)
{

	int retry=3;

	printk("HI351GetSensorID \n");

	do{
		*sensorID=HI351_read_cmos_sensor(0x04);
printk("HI351GetSensorID==%d\n",*sensorID);

		if(*sensorID == HI351_SENSOR_ID)
        	break;
		
		SENSORDB("HI351GetSensorID Read Sendor ID Fail:0x%04x \n",*sensorID);
		retry--;
	}while(retry>0);

	if(*sensorID!=HI351_SENSOR_ID){

		*sensorID=0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	else{
		SENSORDB("HI351 Read Sendor ID SUCCESS:0x%04x \n",*sensorID);
		return ERROR_NONE;
	}

}
UINT32 HI351FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	//UINT16 u2Temp = 0;  
	
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	//UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	
	//PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;
	unsigned long long *feature_data=(unsigned long long *) pFeaturePara;


	switch (FeatureId)
	{
	case SENSOR_FEATURE_GET_RESOLUTION:
	*pFeatureReturnPara16++=HI351_FULL_GRAB_WIDTH;
	*pFeatureReturnPara16=HI351_FULL_GRAB_HEIGHT;
	*pFeatureParaLen=4;
	break;
	
	case SENSOR_FEATURE_GET_PERIOD:
	*pFeatureReturnPara16++=HI351_PV_PERIOD_PIXEL_NUMS+HI351_PV_dummy_pixels;
	*pFeatureReturnPara16=HI351_PV_PERIOD_LINE_NUMS+HI351_PV_dummy_lines;
	*pFeatureParaLen=4;
	break;
	
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
	*pFeatureReturnPara32 = HI351_sensor_pclk/10;
	*pFeatureParaLen=4;
	break;
	
	case SENSOR_FEATURE_SET_ESHUTTER:
	//u2Temp = HI351_read_shutter();  		
	break;
	
	case SENSOR_FEATURE_SET_NIGHTMODE:
	HI351_night_mode((BOOL) *feature_data);
	break;
	
	case SENSOR_FEATURE_SET_GAIN:
	break; 
	
	case SENSOR_FEATURE_SET_FLASHLIGHT:
	break;
	
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
	HI351_isp_master_clock=*pFeatureData32;
	break;
	
	case SENSOR_FEATURE_SET_REGISTER:
	HI351_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
	break;
	
	case SENSOR_FEATURE_GET_REGISTER:
	pSensorRegData->RegData = HI351_read_cmos_sensor(pSensorRegData->RegAddr);
	break;
	
	case SENSOR_FEATURE_GET_CONFIG_PARA:
	memcpy(pSensorConfigData, &HI351SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	*pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
	break;
	
	case SENSOR_FEATURE_SET_CCT_REGISTER:
	case SENSOR_FEATURE_GET_CCT_REGISTER:
	case SENSOR_FEATURE_SET_ENG_REGISTER:
	case SENSOR_FEATURE_GET_ENG_REGISTER:
	case SENSOR_FEATURE_GET_REGISTER_DEFAULT:

	case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
	case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
	case SENSOR_FEATURE_GET_GROUP_INFO:
	case SENSOR_FEATURE_GET_ITEM_INFO:
	case SENSOR_FEATURE_SET_ITEM_INFO:
	case SENSOR_FEATURE_GET_ENG_INFO:
	break;
	
	case SENSOR_FEATURE_GET_GROUP_COUNT:
	*pFeatureReturnPara32++=0;
	*pFeatureParaLen=4;
	break; 

	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
	// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
	// if EEPROM does not exist in camera module.
	*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
	*pFeatureParaLen=4;
	break;
	
	case SENSOR_FEATURE_SET_YUV_CMD:
	HI351YUVSensorSetting((FEATURE_ID)*feature_data, *(feature_data+1));
	break;	
	
	case SENSOR_FEATURE_SET_VIDEO_MODE:
	HI351YUVSetVideoMode(*feature_data);
	break; 
    
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		 HI351GetSensorID(pFeatureData32);
		 break;
             
	default:
	break;			
	}
	return ERROR_NONE;
}	

SENSOR_FUNCTION_STRUCT	SensorFuncHI351=
{
	HI351Open,
	HI351GetInfo,
	HI351GetResolution,
	HI351FeatureControl,
	HI351Control,
	HI351Close
};

UINT32 HI351_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc!=NULL)
	*pfFunc=&SensorFuncHI351;

	return ERROR_NONE;
}	

