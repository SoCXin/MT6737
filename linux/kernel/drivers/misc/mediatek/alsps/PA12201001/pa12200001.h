/* 
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Definitions for TXC PA122 series als/ps sensor chip.
 */
#ifndef __PA122_H__
#define __PA122_H__

#include <linux/ioctl.h>

#define PA12_DRIVER_VERSION_H	"2.1.0"


/* DEVICE can change */ 
/*pa122 als/ps Default*/  
#define PA12_I2C_ADDRESS		0x1E  	/* 7 bit Address */
/*-----------------------------------------------------------------------------*/
#define PA12_ALS_TH_HIGH		65535
#define PA12_ALS_TH_LOW			0
/*-----------------------------------------------------------------------------*/
#define PA12_PS_TH_HIGH			40
#define PA12_PS_TH_LOW			25
#define PA12_PS_TH_MIN		  	0	/* Minimun value */
#define PA12_PS_TH_MAX 			255	/* 8 bit MAX */
/*-----------------------------------------------------------------------------*/
#define PA12_PS_TH_BASE_HIGH 		20
#define PA12_PS_TH_BASE_LOW		18
#define PA12_PS_TH_HIGH_MINIMUM		40
#define PA12_PS_TH_INTERVAL		15
/*-----------------------------------------------------------------------------*/
#define PA12_PS_OFFSET_DEFAULT		10	/* for X-talk cannceling */
#define PA12_PS_OFFSET_EXTRA		5
#define PA12_PS_OFFSET_MAX		150
#define PA12_PS_OFFSET_MIN		0
#define PA12_FAST_CAL			1
#define PA12_FAST_CAL_ONCE		0
#define PA12_FAST_CAL_TOLERANCE     30

/* pa122 als/ps parameter setting */
#define PA12_ALS_GAIN		3 	/* 0:125lux | 1:1000lux | 2:2000lux | 3:10000lux */
#define PA12_LED_CURR		1 	/* 0:150mA | 1:100mA | 2:50mA | 3:25mA | 4:15mA | 5:12mA | 6:10mA | 7:7mA*/

#define PA12_PS_PRST		3 /* 0:1point | 1:2points | 2:4points | 3:8points (for INT) */
#define PA12_ALS_PRST		0	/* 0:1point | 1:2points | 2:4points | 3:8points (for INT) */

#define PA12_PS_SET		1	/* 0:ALS only | 1:PS only | 3:BOTH */
#define PA12_PS_MODE		1	/* 0:OFFSET |1:NORMAL */
#define PA12_INT_TYPE		0 	/* 0:Window type | 1:Hysteresis type for Auto Clear flag */
#define PA12_PS_PERIOD		1	/* 0:6.25 ms | 1:12.5 ms | 2:25 ms | 3:50 ms | 4:100 ms | 5:200 ms | 6:400 ms | 7:800 ms */
#define PA12_ALS_PERIOD		0	/* 0:0 ms | 1:100 ms | 2:300 ms | 3:700 ms | 4:1500 ms  */

/*pa122 als/ps sensor register map*/
#define REG_CFG0 		0X00		/* ALS_GAIN(D5-4) | PS_ON(D1) | ALS_ON(D0) */
#define REG_CFG1 		0X01 	/* LED_CURR(D6-4) | PS_PRST(D3-2) | ALS_PRST(D1-0) */
#define REG_CFG2 		0X02 	/* PS_MODE(D6) | CLEAR(D4) | INT_SET(D3-2) | PS_INT(D1) | ALS_INT(D0) */
#define REG_CFG3		0X03		/* INT_TYPE(D6) | PS_PERIOD(D5-3) | ALS_PERIOD(D2-0) */
#define REG_ALS_TL_LSB		0X04		/* ALS Threshold Low LSB */
#define REG_ALS_TL_MSB		0X05		/* ALS Threshold Low MSB */
#define REG_ALS_TH_LSB		0X06		/* ALS Threshold high LSB */
#define REG_ALS_TH_MSB		0X07		/* ALS Threshold high MSB */
#define REG_PS_TL		0X08		/* PS Threshold Low */
#define REG_PS_TH		0X0A	/* PS Threshold High */
#define REG_ALS_DATA_LSB	0X0B	/* ALS DATA LSB */
#define REG_ALS_DATA_MSB	0X0C	/* ALS DATA MSB */
#define REG_PS_DATA		0X0E		/* PS DATA */
#define REG_PS_OFFSET		0X10		/* TBD */
#define REG_PS_SET			0X11  	/* 0x03 */
#define REG_SUNLIGHT_MODE   0x23    /* SET 0x08 or 0x0C */
#define SUN_LIGHT_CONT 		19000       

#define PS_CAL_FILE_PATH	"/protect_f/xtalk_cal"       
/* ALS Using average data */
#define ALS_USE_AVG_DATA 0

/* ALS: Update continuous lux or use discrete value defined in cust_alsps.c */
#define PA12_ALS_ADC_TO_LUX_USE_LEVEL	0

/* Interrupt step */
#define forward_step 	25
#define backward_step 	5
#endif

