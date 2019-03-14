/*****************************
 *  Finger config
 *****************************/

#ifndef __FINGER_CONFIG_H__
#define __FINGER_CONFIG_H__

//------------------------------------------------------------------------------
//平台选择,默认mtk
#define __PALTFORM_SPREAD_EN          0   // 展迅平台宏开关
//------------------------------------------------------------------------------

#define __SUNWAVE_DETECT_ID_EN        1   // sunwave read id switch default: off
#define __SUNWAVE_QUIK_WK_CPU_EN      1   //quickly wake up cpu core
#define __SUNWAVE_PRIZE_HW_EN         0   // support for Ku Sai's hardware_info default: off
//optional
#define __SUNWAVE_SPI_DMA_MODE_EN     1   // SPI dma transfer mode switch 1: DMA; 0: FIFO 
#define __SUNWAVE_KEY_BACK_EN         1   // __SUNWAVE_KEY_BACK_EN
#define __SUNWAVE_SCREEN_LOCK_EN      0   // fingerprint cannot unlock, when screen is off 
#define __SUNWAVE_HW_INFO_EN          1   // enable SUNWAVE_SET_VERSION_INFO in ioctl

#define SUNWAVE_CHIP_DEF   "swXXXX"
#define SUNWAVE_VENDOR_DEF "sunwave"

#define SUNWAVE_VERSION "V1.1.1.20170425"

#endif