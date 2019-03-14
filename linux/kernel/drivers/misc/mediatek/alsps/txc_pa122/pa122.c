/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.  
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/*  * 
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/version.h> 
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
//#include <hwmsensor.h>

#include <mach/gpio_const.h>
#include "cust_alsps.h"
#include "pa122.h"
#include "alsps.h"


/******************************************************************************
 * configuration
*******************************************************************************/
/**Global Variable**/
static int prevObj 	= 1;
static int intr_flag 	= 1;
static int bCal_flag	= 0;
static int pa122_has_load_cal_file = 0;
/*----------------------------------------------------------------------------*/
#define PA122_DEV_NAME		"pa122"
#define PA12_DRIVER_VERSION_C		"1.92.1.02"
/*----------------------------------------------------------------------------*/
#define PA122_DEBUG
#if defined(PA122_DEBUG)
#define APS_TAG		"[ALS/PS] "
#define APS_FUN(f)			printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)	printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define printk(fmt, args...)	printk(KERN_ERR APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)	printk(KERN_INFO APS_TAG fmt, ##args)    
#else
#define APS_FUN(f)
#define APS_ERR(fmt, args...)
#define printk(fmt, args...)
#define APS_DBG(fmt, args...)
#endif

#define I2C_FLAG_WRITE	0
#define I2C_FLAG_READ	1

/******************************************************************************
 * extern functions
*******************************************************************************/


/*----------------------------------------------------------------------------*/
static int pa122_init_client(struct i2c_client *client);		
static int pa122_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int pa122_i2c_remove(struct i2c_client *client);
static int pa122_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int pa122_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int pa122_i2c_resume(struct i2c_client *client);

static struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
/* For alsp driver get cust info */
struct alsps_hw *get_cust_alsps(void){	
	return &alsps_cust;
}

static int pa122_init_flag =-1; // 0<==>OK -1 <==> fail
static int pa122_local_init(void);
static int pa122_local_uninit(void);
static struct alsps_init_info pa122_init_info = {
		.name = "pa122",
		.init = pa122_local_init,
	.uninit = pa122_local_uninit,
};


static const struct i2c_device_id pa122_i2c_id[] = {{PA122_DEV_NAME,0},{}};
/*----------------------------------------------------------------------------*/
struct pa122_priv {
	struct alsps_hw  *hw;
	struct i2c_client *client;
	struct work_struct	eint_work;

	/* misc */
	u16 		als_modulus;
	atomic_t	i2c_retry;
	atomic_t	als_suspend;
	atomic_t	als_debounce;	/*debounce time after enabling als*/
	atomic_t	als_deb_on; 		/*indicates if the debounce is on*/
	atomic_t	als_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_mask;		/*mask ps: always return far away*/
	atomic_t	ps_debounce;	/*debounce time after enabling ps*/
	atomic_t	ps_deb_on;		/*indicates if the debounce is on*/
	atomic_t	ps_deb_end; 	/*the jiffies representing the end of debounce*/
	atomic_t	ps_suspend;
	atomic_t 	trace;
	atomic_t    init_done;
	struct      device_node *irq_node;
	int         irq;
	/* data */
	u16			als;
	u8 			ps;
	u8			_align;
	u16			als_level_num;
	u16			als_value_num;
	u32			als_level[C_CUST_ALS_LEVEL-1];
	u32			als_value[C_CUST_ALS_LEVEL];

	/* Mutex */
	struct mutex	update_lock;

	/* PS Calibration */
	u8 		crosstalk; 
	u8 		crosstalk_base; 

	/* threshold */
	u8		ps_to_max;
	u8		ps_thrd_low; 
	u8		ps_thrd_high; 

	atomic_t	als_cmd_val;		/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_cmd_val; 		/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_high;	 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_low; 		/*the cmd value can't be read, stored in ram*/
	atomic_t	als_thd_val_high;	/*the cmd value can't be read, stored in ram*/
	atomic_t	als_thd_val_low; 	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val;
	ulong		enable; 			/*enable mask*/
	ulong		pending_intr;	/*pending interrupt*/
	
	/* early suspend */
	#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend	early_drv;
	#endif     
};
/*----------------------------------------------------------------------------*/

#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps"},	
	{},
};
#endif
static struct i2c_driver pa122_i2c_driver = {	
.driver = {
	.name = PA122_DEV_NAME,
#ifdef CONFIG_OF	
	.of_match_table = alsps_of_match,
#endif
	},
	.probe	= pa122_i2c_probe,
	.remove	= pa122_i2c_remove,
	.detect	= pa122_i2c_detect,
	.suspend	= pa122_i2c_suspend,
	.resume	= pa122_i2c_resume,
	.id_table	= pa122_i2c_id,
};


/*----------------------------------------------------------------------------*/

static struct i2c_client *pa122_i2c_client = NULL;
static struct pa122_priv *g_pa122_ptr = NULL; 
static struct pa122_priv *pa122_obj = NULL;
struct platform_device *alspsPltFmDev;

/*----------------------------------------------------------------------------*/
struct pa122_parameters {
	u16 als_th_high;
	u16 als_th_low;
	u8 ps_th_high;
	u8 ps_th_low;
	u8 ps_offset_default;
	u8 ps_offset_extra;
	u8 ps_offset_max;
	u8 ps_offset_min;
	
	int fast_cal;
	int fast_cal_once;
	
	int als_gain;
	int led_curr;
	int ps_prst;
	int als_prst;
	int int_set;
	int int_type;
	int ps_period;
	int als_period;
};
/*----------------------------------------------------------------------------*/
//static struct pa122_parameters *pa122_param = NULL;
/*----------------------------------------------------------------------------*/
typedef enum {
	CMC_BIT_ALS	= 1,
	CMC_BIT_PS		= 2,
}CMC_BIT;
/*-----------------------------CMC for debugging-------------------------------*/
typedef enum {
    CMC_TRC_ALS_DATA	= 0x0001,
    CMC_TRC_PS_DATA	= 0x0002,
    CMC_TRC_EINT		= 0x0004,
    CMC_TRC_IOCTL		= 0x0008,
    CMC_TRC_I2C			= 0x0010,
    CMC_TRC_CVT_ALS	= 0x0020,
    CMC_TRC_CVT_PS		= 0x0040,
    CMC_TRC_DEBUG		= 0x8000,
} CMC_TRC;
/*-----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
// I2C Read
/*
static int i2c_read_reg(struct i2c_client *client,u8 reg,u8 *data)
{
  u8 reg_value[1];
  u8 databuf[2]; 
	int res = 0;
	databuf[0]= reg;
	res = i2c_master_send(client,databuf,0x1);
	if(res <= 0)
	{
		APS_ERR("i2c_master_send function err\n");
		return res;
	}
	res = i2c_master_recv(client,reg_value,0x1);
	if(res <= 0)
	{
		APS_ERR("i2c_master_recv function err\n");
		return res;
	}
	return reg_value[0];
}
*/
// I2C Write
/*
static int i2c_write_reg(struct i2c_client *client,u8 reg,u8 value)
{
	u8 databuf[2];    
	int res = 0;
   
	databuf[0] = reg;   
	databuf[1] = value;
	res = i2c_master_send(client,databuf,0x2);

	if (res < 0){
		return res;
		APS_ERR("i2c_master_send function err\n");
	}
		return 0;
}
*/
/*----------------------------------------------------------------------------*/
static int pa122_read_file(char *filename,u8* param) 
{
	struct file  *fop;
	mm_segment_t old_fs;

	fop = filp_open(filename,O_RDONLY,0);
	if(IS_ERR(fop))
	{
		printk("Filp_open error!! Path = %s\n",filename);
		return -1;
	}

	old_fs = get_fs();  
	set_fs(get_ds()); //set_fs(KERNEL_DS);  
	     
	fop->f_op->llseek(fop,0,0);
	fop->f_op->read(fop, param, strlen(param), &fop->f_pos);     

	set_fs(old_fs);  

	filp_close(fop,NULL);

	return 0;

}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_write_file(char *filename,u8* param) 
{
	struct file  *fop;
	mm_segment_t old_fs;	 

	fop = filp_open(filename,O_CREAT | O_RDWR,0666);
	if(IS_ERR(fop))
	{
		printk("Create file error!! Path = %s\n",filename);       
		return -1;
	}

	old_fs = get_fs();  
	set_fs(get_ds()); //set_fs(KERNEL_DS);  
	fop->f_op->write(fop, (char *)param, sizeof(param), &fop->f_pos);   
	set_fs(old_fs);  

	filp_close(fop,NULL);

	return 0;
}
/*----------------------------------------------------------------------------*/
void pa12_swap(u8 *x, u8 *y)
{
        u8 temp = *x;
        *x = *y;
        *y = temp;
}
static void pa122_load_calibration_param(struct i2c_client *client)
{
	int res;
	u8 buftemp[2];

	struct pa122_priv *obj = i2c_get_clientdata(client);

	/* Check ps calibration file */
	if(pa122_read_file(PS_CAL_FILE_PATH,buftemp) < 0)
	{
		obj->crosstalk = PA12_PS_OFFSET_DEFAULT;
		printk("Use Default ps offset , x-talk = %d\n", obj->crosstalk);
	}
	else
	{
		printk("Use PS Cal file , x-talk = %d sun = %d\n",buftemp[0],buftemp[1]);	
		obj->crosstalk = buftemp[0];
		obj->crosstalk_base = buftemp[1];
	}

	mutex_lock(&obj->update_lock);
	/* Write ps offset value to register 0x10 */
	hwmsen_write_byte(client, REG_PS_OFFSET, obj->crosstalk); 

	if(obj->hw->polling_mode_ps == 0)
	{
		/* Check threshold calibration file */
		if(pa122_read_file(THRD_CAL_FILE_PATH,buftemp) < 0)
		{
			printk("Use Default threhold , Low = %d , High = %d\n", obj->ps_thrd_low, obj->ps_thrd_high);
		}
		else
		{
			printk("Use Threshold Cal file , Low = %d , High = %d\n",buftemp[0],buftemp[1]);
			obj->ps_thrd_low = buftemp[0];
			obj->ps_thrd_high = buftemp[1];
		}	
		/* Set PS threshold */
		if(PA12_INT_TYPE == 0)
		{
			/*Window Type */	
			res=hwmsen_write_byte(client, REG_PS_TH, obj->ps_thrd_high);
			res=hwmsen_write_byte(client, REG_PS_TL, PA12_PS_TH_MIN);
		}
		else if(PA12_INT_TYPE == 1)
		{
			/*Hysteresis Type */
			res=hwmsen_write_byte(client, REG_PS_TH, obj->ps_thrd_high); 
			res=hwmsen_write_byte(client, REG_PS_TL, obj->ps_thrd_low); 
		}
	}

	mutex_unlock(&obj->update_lock);
}

static int pa122_run_calibration(struct i2c_client *client)
{
	struct pa122_priv *data = i2c_get_clientdata(client);
	int i, j;	
	int ret;
	u16 sum_of_pdata = 0;
	u8 temp_pdata[20],buftemp[2],cfg0data=0,cfg2data=0;
	unsigned int ArySize = 20;
	unsigned int cal_check_flag = 0;	
	
	printk("%s: START proximity sensor calibration\n", __func__);

RECALIBRATION:
	sum_of_pdata = 0;

	mutex_lock(&data->update_lock);
	ret = hwmsen_read_byte(client, REG_CFG0, &cfg0data);
	ret = hwmsen_read_byte(client, REG_CFG2, &cfg2data);
	
	/*PS On*/
	ret = hwmsen_write_byte(client, REG_CFG0, cfg0data | 0x02); 

	/*Set to offset mode & disable interrupt from ps*/
	ret = hwmsen_write_byte(client, REG_CFG2, cfg2data & 0x33); 

	/*Set crosstalk = 0*/
	ret = hwmsen_write_byte(client, REG_PS_OFFSET, 0x00); 	
	

	for(i = 0; i < 20; i++)
	{
		mdelay(50);
		ret = hwmsen_read_byte(client,REG_PS_DATA,temp_pdata+i);
		printk("temp_data = %d\n", temp_pdata[i]);	
	}	
	mutex_unlock(&data->update_lock);
	
	/* pdata sorting */
	for (i = 0; i < ArySize - 1; i++)
		for (j = i+1; j < ArySize; j++)
			if (temp_pdata[i] > temp_pdata[j])
				pa12_swap(temp_pdata + i, temp_pdata + j);	
	
	/* calculate the cross-talk using central 10 data */
	for (i = 5; i < 15; i++) 
	{
		printk("%s: temp_pdata = %d\n", __func__, temp_pdata[i]);
		sum_of_pdata = sum_of_pdata + temp_pdata[i];
	}

	data->crosstalk = sum_of_pdata/10;
    	printk("%s: sum_of_pdata = %d   cross_talk = %d\n",
                        __func__, sum_of_pdata, data->crosstalk);
	

	
	/* Restore CFG2 (Normal mode) and Measure base x-talk */
	mutex_lock(&data->update_lock);
	ret = hwmsen_write_byte(client, REG_CFG0, cfg0data);
	ret = hwmsen_write_byte(client, REG_CFG2, cfg2data | 0xC0); 
	mutex_unlock(&data->update_lock);
 	
	if (data->crosstalk > PA12_PS_OFFSET_MAX)
	{
		printk("%s: invalid calibrated data\n", __func__);

		if(cal_check_flag == 0)
		{
			printk("%s: RECALIBRATION start\n", __func__);
			cal_check_flag = 1;
			goto RECALIBRATION;
		}
		else
		{
			printk("%s: CALIBRATION FAIL -> "
                               "cross_talk is set to DEFAULT\n", __func__);
			data->crosstalk = PA12_PS_OFFSET_DEFAULT;
			ret = hwmsen_write_byte(client, REG_PS_OFFSET, data->crosstalk);
			return -EINVAL;
         	}
	}	

	data->crosstalk += PA12_PS_OFFSET_EXTRA;

CROSSTALKBASE_RECALIBRATION:
		
	
	mutex_lock(&data->update_lock);
	
	/*PS On*/
	ret = hwmsen_write_byte(client, REG_CFG0, cfg0data | 0x02); 

	/*Write offset value to register 0x10*/
	ret = hwmsen_write_byte(client, REG_PS_OFFSET, data->crosstalk);
	
	for(i = 0; i < 10; i++)
	{
		mdelay(50);
		ret = hwmsen_read_byte(client,REG_PS_DATA,temp_pdata+i);
		printk("temp_data = %d\n", temp_pdata[i]);	
	}	

	mutex_unlock(&data->update_lock);
 
	/* calculate the cross-talk_base using central 5 data */

	sum_of_pdata = 0;

	for (i = 3; i < 8; i++) 
	{
		printk("%s: temp_pdata = %d\n", __func__, temp_pdata[i]);
		sum_of_pdata = sum_of_pdata + temp_pdata[i];
	}
	
	data->crosstalk_base = sum_of_pdata/5;
    	printk("%s: sum_of_pdata = %d   cross_talk_base = %d\n",
                        __func__, sum_of_pdata, data->crosstalk_base);

	if(data->crosstalk_base > 0) 
	{
		data->crosstalk += 1;
		goto CROSSTALKBASE_RECALIBRATION;
	}	

  	 /* Restore CFG0  */
	mutex_lock(&data->update_lock);
	ret = hwmsen_write_byte(client, REG_CFG0, cfg0data);
	mutex_unlock(&data->update_lock);

	printk("%s: FINISH proximity sensor calibration\n", __func__);
	/*Write x-talk info to file*/  
	buftemp[0]=data->crosstalk;
    	buftemp[1]=data->crosstalk_base;

	if(pa122_write_file(PS_CAL_FILE_PATH,buftemp) < 0)
	{
		printk("Open PS x-talk calibration file error!!");
		return -1;
	}
	else
	{
		printk("Open PS x-talk calibration file Success!!");
		pa122_has_load_cal_file = 0;
		return data->crosstalk;
	}
}
static int pa122_run_fast_calibration(struct i2c_client *client)
{

	struct pa122_priv *data = i2c_get_clientdata(client);
	int i = 0;
	int j = 0;	
	u16 sum_of_pdata = 0;
	u8  xtalk_temp = 0;
    	u8 temp_pdata[4], cfg0data = 0,cfg2data = 0,cfg3data = 0;
   	unsigned int ArySize = 4;

	if(bCal_flag & PA12_FAST_CAL_ONCE)
	{
		printk("Ignore Fast Calibration\n");
		return data->crosstalk;
	}
	
   	printk("START proximity sensor calibration\n");

	mutex_lock(&data->update_lock);

	hwmsen_read_byte(client, REG_CFG0, &cfg0data);
	hwmsen_read_byte(client, REG_CFG2, &cfg2data);
	hwmsen_read_byte(client, REG_CFG3, &cfg3data);

	/*PS On*/
	hwmsen_write_byte(client, REG_CFG0, cfg0data | 0x02); 
	
	/*Offset mode & disable intr from ps*/
	hwmsen_write_byte(client, REG_CFG2, cfg2data & 0x33); 	
	
	/*PS sleep time 6.5ms */
	hwmsen_write_byte(client, REG_CFG3, cfg3data & 0xC7); 	

	/*Set crosstalk = 0*/
	hwmsen_write_byte(client, REG_PS_OFFSET, 0x00); 

	for(i = 0; i < 4; i++)
	{
		mdelay(50);
		hwmsen_read_byte(client,REG_PS_DATA,temp_pdata+i);
		printk("temp_data = %d\n", temp_pdata[i]);	
	}	

	mutex_unlock(&data->update_lock);
	
	/* pdata sorting */
	for (i = 0; i < ArySize - 1; i++)
		for (j = i+1; j < ArySize; j++)
			if (temp_pdata[i] > temp_pdata[j])
				pa12_swap(temp_pdata + i, temp_pdata + j);	
	
	/* calculate the cross-talk using central 2 data */
	for (i = 1; i < 3; i++) 
	{
		printk("%s: temp_pdata = %d\n", __func__, temp_pdata[i]);
		sum_of_pdata = sum_of_pdata + temp_pdata[i];
	}

	xtalk_temp = sum_of_pdata/2;
   	printk("%s: sum_of_pdata = %d   cross_talk = %d\n",
                        __func__, sum_of_pdata, data->crosstalk);
	
	/* Restore Data */
	mutex_lock(&data->update_lock);
	hwmsen_write_byte(client, REG_CFG0, cfg0data);
	hwmsen_write_byte(client, REG_CFG2, cfg2data | 0xC0); //make sure return normal mode
	hwmsen_write_byte(client, REG_CFG3, cfg3data);
	mutex_unlock(&data->update_lock);

	if (xtalk_temp >= (data->crosstalk - PA12_PS_OFFSET_EXTRA) && xtalk_temp < PA12_PS_OFFSET_MAX)
	{ 	
		printk("Fast calibrated data=%d\n",xtalk_temp);
		bCal_flag=1;
		/* Write offset value to 0x10 */
		mutex_lock(&data->update_lock);
		hwmsen_write_byte(client, REG_PS_OFFSET, xtalk_temp + PA12_PS_OFFSET_EXTRA);
		mutex_unlock(&data->update_lock);
		return xtalk_temp + PA12_PS_OFFSET_EXTRA;
	}
	else
	{
		printk("Fast calibration fail, xtalk=%d\n",xtalk_temp);
		
		mutex_lock(&data->update_lock);

		if(PA12_FAST_CAL_ONCE)
		{
			if(xtalk_temp >= PA12_PS_OFFSET_MAX)
			hwmsen_write_byte(client, REG_PS_OFFSET, xtalk_temp + PA12_PS_OFFSET_EXTRA);
			else
			hwmsen_write_byte(client, REG_PS_OFFSET, PA12_PS_OFFSET_DEFAULT);
		}
		else
		{
			hwmsen_write_byte(client, REG_PS_OFFSET, data->crosstalk);
			xtalk_temp = data->crosstalk;
		}
		mutex_unlock(&data->update_lock);
		
		return xtalk_temp;
        }   	

}
/**********************************************************************************************/
int pa122_enable_ps(struct i2c_client *client, int enable)
{
	struct pa122_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 regdata = 0;
	u8 sendvalue = 0;
	//struct hwm_sensor_data sensor_data;
	struct file  *fop;

	mutex_lock(&obj->update_lock);
	res = hwmsen_read_byte(client, REG_CFG0, &regdata); 
	mutex_unlock(&obj->update_lock);	

	if(res<0)
	{
		APS_ERR("i2c_read function err\n");
		return -1;
	}

	

	if(enable == 1)
	{
		printk("zhou====pa122 enable ps sensor\n");
			
		if(pa122_has_load_cal_file == 0)
		{
			pa122_has_load_cal_file = 1;
			fop = filp_open(PS_CAL_FILE_PATH, O_RDONLY, 0);
			if(IS_ERR(fop))
			{
				printk("pa122_enable_ps: open file error!! Path = %s\n", PS_CAL_FILE_PATH);
				if (PA12_FAST_CAL == 0)
					pa122_run_calibration(client);
			}
			else
			{
				filp_close(fop, NULL);
				pa122_load_calibration_param(client);
			}
			msleep(5);
		}

		if(PA12_FAST_CAL)
			pa122_run_fast_calibration(client);
		
		/**** SET INTERRUPT FLAG AS FAR ****/
		if(obj->hw->polling_mode_ps == 0)
		{
			if(intr_flag == 0)
			{						
				intr_flag = 1; 
				if(PA12_INT_TYPE == 0)
				{
					mutex_lock(&obj->update_lock);
					hwmsen_write_byte(client, REG_PS_TL, PA12_PS_TH_MIN);
					hwmsen_write_byte(client, REG_PS_TH, obj->ps_thrd_high);
					hwmsen_write_byte(client,REG_CFG1,
							(PA12_LED_CURR << 4)| (PA12_PS_PRST << 2)| (PA12_ALS_PRST));
					mutex_unlock(&obj->update_lock);
				}
				else if(PA12_INT_TYPE == 1)
				{
					res = hwmsen_read_byte(client,REG_CFG2,&regdata);		
					regdata=regdata & 0xFD ; 
					mutex_lock(&obj->update_lock);
					res = hwmsen_write_byte(client,REG_CFG2,regdata);
					res = hwmsen_write_byte(client,REG_CFG1,
								(PA12_LED_CURR << 4)| (PA12_PS_PRST << 2)| (PA12_ALS_PRST));
					mutex_unlock(&obj->update_lock);
				}

			}
			printk("zhou====PA122 interrupt value = %d\n", intr_flag);
		res = ps_report_interrupt_data(intr_flag);			
/*			
			sensor_data.values[0] = intr_flag;
			sensor_data.value_divide = 1;
			sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;				
			if((res = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
			{
				APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", res);
			}
*/
		}
		/***********************************/
		//printk("CFG0 Status: %d\n",regdata);
		//sendvalue = regdata & 0xFD;
		/* PS On */
		sendvalue = regdata | 0x02;

		mutex_lock(&obj->update_lock);
		res=hwmsen_write_byte(client,REG_CFG0,sendvalue); 
		mutex_unlock(&obj->update_lock);

		if(res<0)
		{
			APS_ERR("i2c_write function err\n");
			return res;
		}

		atomic_set(&obj->ps_deb_on, 1);
		atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
	}
	else
	{
		printk("pa122 disaple ps sensor\n");
		
		printk("CFG0 Status: %d\n",regdata);
		/* PS Off */
		sendvalue=regdata & 0xFD; 

		mutex_lock(&obj->update_lock);				
		res=hwmsen_write_byte(client,REG_CFG0,sendvalue); 
		mutex_unlock(&obj->update_lock);

		if(res<0)
		{
			APS_ERR("i2c_write function err\n");
			return res;
		}	  	
		atomic_set(&obj->ps_deb_on, 0);
	}

	return 0;
}


/********************************************************************/
int pa122_enable_als(struct i2c_client *client, int enable)
{
	struct pa122_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 regdata=0;
	u8 sendvalue=0;

	mutex_lock(&obj->update_lock);
	res=hwmsen_read_byte(client,REG_CFG0,&regdata); 
	mutex_unlock(&obj->update_lock);

	if(res<0)
	{
		APS_ERR("i2c_read function err\n");
		return -1;
	}	

	printk("CFG0 Status: %d\n",regdata);

	if(enable == 1)
	{
		printk("pa122 enable als sensor\n");
		//sendvalue=regdata & 0xFE; 
		/* ALS On */
		sendvalue=regdata | 0x01; 

		mutex_lock(&obj->update_lock);
		res=hwmsen_write_byte(client,REG_CFG0,sendvalue); 
		mutex_unlock(&obj->update_lock);

		if(res<0)
		{
			APS_ERR("i2c_write function err\n");
			return res;
		}


		atomic_set(&obj->als_deb_on, 1);
		atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
	}
	else
	{
		printk("pa122 disaple als sensor\n");
		/* ALS Off */
		sendvalue=regdata & 0xFE; 

		mutex_lock(&obj->update_lock);
		res=hwmsen_write_byte(client,REG_CFG0,sendvalue);
		mutex_unlock(&obj->update_lock);
		if(res<0)
		{
			APS_ERR("i2c_write function err\n");
			return res;
		}

		atomic_set(&obj->als_deb_on, 0);
	}

	return 0;
}

/********************************************************************/
int pa122_read_ps(struct i2c_client *client, u8 *data)
{
	int res;

	APS_FUN(f);

	mutex_lock(&pa122_obj->update_lock);
	res = hwmsen_read_byte(client, REG_PS_DATA, data); 
	mutex_unlock(&pa122_obj->update_lock);

	if(res < 0)
	{
		APS_ERR("i2c_send function err\n");
	}
	printk("PA122_PS_DATA value = %x\n",*data);	
	return res;
}
/********************************************************************/
int pa122_read_als(struct i2c_client *client, u16 *data)
{
	int res;
	u8 dataLSB;
	u8 dataMSB;
	u16 count;

	APS_FUN(f);

	mutex_lock(&pa122_obj->update_lock);
	res = hwmsen_read_byte(client, REG_ALS_DATA_LSB, &dataLSB); 
	mutex_unlock(&pa122_obj->update_lock);

	if(res < 0)
	{
		APS_ERR("i2c_send function err\n");
		return res;
	}

	mutex_lock(&pa122_obj->update_lock);
	res = hwmsen_read_byte(client, REG_ALS_DATA_MSB, &dataMSB);
	mutex_unlock(&pa122_obj->update_lock);

	if(res < 0)
	{
		APS_ERR("i2c_send function err\n");
		return res;
	}

	count = ((dataMSB<<8)|dataLSB);

	printk("PA122_ALS_DATA count=%d\n ",count);

	*data = count;

	return 0;
}

/**Change to near/far ****************************************************/
static int pa122_get_ps_value(struct pa122_priv *obj, u8 ps)
{
	int val = 0;
	int invalid = 0;
	int mask = atomic_read(&obj->ps_mask);

	if(ps > obj->ps_thrd_high)
	{
		val = 0;  /*close*/
		prevObj=0;
		return 0;
	}
	else if(ps < obj->ps_thrd_low)
	{
		val = 1;  /*far away*/
		prevObj=1;
		return 1;
	}

	return prevObj;


	if(atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}

	else if(1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);

		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}

		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		if(unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
		{
			if(mask)
			{
				APS_DBG("PS:  %05d => %05d [M] \n", ps, val);
			}
			else
			{
				APS_DBG("PS:  %05d => %05d\n", ps, val);
			}
		}
		if(0 == test_bit(CMC_BIT_PS,  &obj->enable))
		{
			//if ps is disable do not report value
			APS_DBG("PS: not enable and do not report this value\n");
			return -1;
		}
		else
		{
			return val;
		}
	}	
	else
	{
		if(unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
		{
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);    
		}
		return -1;
	}
}

/**Change to luxr************************************************/
static int pa122_get_als_value(struct pa122_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;	
	u64 lux=0;

	for(idx = 0; idx < obj->als_level_num; idx++)
	{
		if(als < obj->hw->als_level[idx])
		{
			break;
		}
	}
	if(idx >= obj->als_value_num)
	{
		APS_ERR("exceed range\n"); 
		idx = obj->als_value_num - 1;
	}

	if(1 == atomic_read(&obj->als_deb_on))
	{
		unsigned long endt = atomic_read(&obj->als_deb_end);

		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->als_deb_on, 0);
		}

		if(1 == atomic_read(&obj->als_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
		if (atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
		}
		if(PA12_ALS_ADC_TO_LUX_USE_LEVEL)
		{
			return obj->hw->als_value[idx];
		}
		else
		{
			lux = (als * PA12_ALS_ADC_TO_LUX_NUMERATOR) / PA12_ALS_ADC_TO_LUX_DENOMINATOR;		
			if(lux > 10240)		    
			return 10240;  
			else 
			return (int)lux;
		}
	}
	else
	{
		if(atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
		{
			APS_DBG("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);	  
		}
		return -1;
	}
}


/*-------------------------------attribute file for debugging----------------------------------*/

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t pa122_show_version(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, ".H Ver: %s\n.C Ver: %s\n",PA12_DRIVER_VERSION_H,PA12_DRIVER_VERSION_C); 
	return res;    
}
static ssize_t pa122_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	
	res = snprintf(buf, PAGE_SIZE, "(%d %d %d %d %d)\n", 
		atomic_read(&pa122_obj->i2c_retry), atomic_read(&pa122_obj->als_debounce), 
		atomic_read(&pa122_obj->ps_mask), atomic_read(&pa122_obj->ps_thd_val), atomic_read(&pa122_obj->ps_debounce));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	
	if(5 == sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb))
	{ 
		atomic_set(&pa122_obj->i2c_retry, retry);
		atomic_set(&pa122_obj->als_debounce, als_deb);
		atomic_set(&pa122_obj->ps_mask, mask);
		atomic_set(&pa122_obj->ps_thd_val, thres);        
		atomic_set(&pa122_obj->ps_debounce, ps_deb);
	}
	else
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, (int)count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&pa122_obj->trace));     
	return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
    int trace;
    if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&pa122_obj->trace, trace);
	}
	else 
	{
		APS_ERR("invalid content: '%s', length = %d\n", buf, (int)count);
	}
	return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	if((res = pa122_read_als(pa122_obj->client, &pa122_obj->als)))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	}
	else
	{
		return snprintf(buf, PAGE_SIZE, "%d\n", pa122_obj->als);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_show_ps(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	
	if((res = pa122_read_ps(pa122_obj->client, &pa122_obj->ps)))
	{
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", (int)res);
	}
	else
	{
		return snprintf(buf, PAGE_SIZE, "%d\n", pa122_obj->ps);     
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_show_reg(struct device_driver *ddri, char *buf)
{
	u8 regdata;
	int res=0;
	int count=0;
	int i=0	;
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	

	mutex_lock(&pa122_obj->update_lock);
	for(i=0;i <19 ;i++)
	{
		res=hwmsen_read_byte(pa122_obj->client,0x00+i,&regdata);

		if(res<0)
		{
		   break;
		}
		else
		count+=sprintf(buf+count,"[%x] = (%x)\n",0x00+i,regdata);
	}
	mutex_unlock(&pa122_obj->update_lock);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_show_send(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;


	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	else if(2 != sscanf(buf, "%x %x", &addr, &cmd))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	mutex_lock(&pa122_obj->update_lock);		
	hwmsen_write_byte(pa122_obj->client,addr,cmd);
	mutex_unlock(&pa122_obj->update_lock);
	//****************************
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_show_recv(struct device_driver *ddri, char *buf)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	//u8 dat;
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	else if(1 != sscanf(buf, "%x", &addr))
	{
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	//****************************
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	
	if(pa122_obj->hw)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			pa122_obj->hw->i2c_num, pa122_obj->hw->power_id, pa122_obj->hw->power_vol);
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}
	
	len += snprintf(buf+len, PAGE_SIZE-len, "REGS: %02X %02X %02X %02lX %02lX\n", 
				atomic_read(&pa122_obj->als_cmd_val), atomic_read(&pa122_obj->ps_cmd_val), 
				atomic_read(&pa122_obj->ps_thd_val),pa122_obj->enable, pa122_obj->pending_intr);
	
	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&pa122_obj->als_suspend), atomic_read(&pa122_obj->ps_suspend));

	return len;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct pa122_priv *obj, const char* buf, size_t count, u32 data[], int len)
{
	int idx = 0;
	char *cur = (char*)buf, *end = (char*)(buf+count);

	while(idx < len)
	{
		while((cur < end) && IS_SPACE(*cur))
		{
			cur++;        
		}

		if(1 != sscanf(cur, "%d", &data[idx]))
		{
			break;
		}

		idx++; 
		while((cur < end) && !IS_SPACE(*cur))
		{
			cur++;
		}
	}
	return idx;
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < pa122_obj->als_level_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", pa122_obj->hw->als_level[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_store_alslv(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(pa122_obj->als_level, pa122_obj->hw->als_level, sizeof(pa122_obj->als_level));
	}
	else if(pa122_obj->als_level_num != read_int_from_buf(pa122_obj, buf, count, 
			pa122_obj->hw->als_level, pa122_obj->als_level_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	
	for(idx = 0; idx < pa122_obj->als_value_num; idx++)
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "%d ", pa122_obj->hw->als_value[idx]);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	return len;    
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_store_alsval(struct device_driver *ddri, const char *buf, size_t count)
{
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}
	else if(!strcmp(buf, "def"))
	{
		memcpy(pa122_obj->als_value, pa122_obj->hw->als_value, sizeof(pa122_obj->als_value));
	}
	else if(pa122_obj->als_value_num != read_int_from_buf(pa122_obj, buf, count, 
			pa122_obj->hw->als_value, pa122_obj->als_value_num))
	{
		APS_ERR("invalid format: '%s'\n", buf);
	}    
	return count;
}

/*---Offset At-------------------------------------------------------------------------*/
static ssize_t pa122_show_ps_offset(struct device_driver *ddri, char *buf)
{
	if(!pa122_obj)
	{
		APS_ERR("pa122_obj is null!!\n");
		return 0;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", pa122_obj->crosstalk);     

}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_set_ps_offset(struct device_driver *ddri, const char *buf, size_t count)
{
	int ret;
	ret = pa122_run_calibration(pa122_obj->client);
	return ret;
}
/*----------------------------------------------------------------------------*/
//static ssize_t pa122_show_reg_add(struct device_driver *ddri, char *buf)
//{return 0;}
/*----------------------------------------------------------------------------*/
//static ssize_t pa122_show_reg_value(struct device_driver *ddri, char *buf)
//{return 0;}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_store_dev_init(struct device_driver *ddri, const char *buf, size_t count)
{
	int ret;
	ret = pa122_init_client(pa122_obj->client);
	return count;
}
/*----------------------------------------------------------------------------*/
 
//static ssize_t pa122_show_pthreshold_calibration(struct device_driver *ddri, char *buf)
//{
//	struct pa122_priv *data = i2c_get_clientdata(pa122_obj->client);
	
//	return sprintf(buf, "Low threshold = %d , High threshold = %d\n",data->ps_thrd_low,data->ps_thrd_high);	
//}
/*----------------------------------------------------------------------------*/
/* X-talk Calibration file */
static ssize_t pa122_show_calibration_file(struct device_driver *ddri, char *buf)
{
    struct file  *fop;
    mm_segment_t old_fs;
    u8 readtemp[4];

    fop = filp_open(PS_CAL_FILE_PATH,O_RDONLY,0);
    if(IS_ERR(fop)){
        printk("Filp_open error!!");
        return sprintf(buf, "Open File Error\n");
    }

     old_fs = get_fs();  
     set_fs(get_ds()); //set_fs(KERNEL_DS);  
	     
     fop->f_op->llseek(fop,0,0);
     fop->f_op->read(fop, readtemp, strlen(readtemp), &fop->f_pos);     

     set_fs(old_fs);  

     filp_close(fop,NULL);
     return sprintf(buf, "X-talk data= %d\n",readtemp[0]);
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_store_calibration_file(struct device_driver *ddri, const char *buf, size_t count)
{
    struct file  *fop;
    mm_segment_t old_fs;	 
    u8 tempbuf[2];
    int temp;
    sscanf(buf, "%d",&temp); 
    tempbuf[0]=(u8)temp;

    fop = filp_open(PS_CAL_FILE_PATH,O_CREAT | O_RDWR,0644);
    if(IS_ERR(fop)){
        printk("Test: filp_open error!!");
        return -1;
    }

     old_fs = get_fs();  
     set_fs(get_ds()); //set_fs(KERNEL_DS);  
     fop->f_op->write(fop, (char *)tempbuf, sizeof(tempbuf), &fop->f_pos);  
     set_fs(old_fs);  

     filp_close(fop,NULL);

    return count;
}
/*----------------------------------------------------------------------------*/
/* Threshold Calibration file */
static ssize_t pa122_show_pthrd_file(struct device_driver *ddri, char *buf)
{
     u8 readtemp[2];
     if(pa122_read_file(THRD_CAL_FILE_PATH,readtemp)<0){
  
       return sprintf(buf, "Open File Error\n");
     }
     
     return sprintf(buf, "Low threshold = %d , High threshold = %d\n",readtemp[0],readtemp[1]);
}
/*----------------------------------------------------------------------------*/
static ssize_t pa122_store_pthrd_file(struct device_driver *ddri, const char *buf, size_t count)
{	 
    u8 buftemp[2];
    int temp[2];


    if(2 != sscanf(buf, "%d %d",&temp[0],&temp[1]))
    {
	APS_ERR("invalid format: '%s'\n", buf);
	return -1;
    }
    buftemp[0]=(u8)temp[0];
    buftemp[1]=(u8)temp[1];

    if(pa122_write_file(THRD_CAL_FILE_PATH,buftemp)<0)
    printk("Create PS Thredhold calibration file error!!");
    else
    printk("Create PS Thredhold calibration file Success!!");
 
    return count;
} 
/*---------------------------------------------------------------------------------------*/
static DRIVER_ATTR(version,     S_IWUSR | S_IRUGO, pa122_show_version, NULL);
static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, pa122_show_als, NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, pa122_show_ps, NULL);
static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, pa122_show_config,	pa122_store_config);
static DRIVER_ATTR(alslv,   S_IWUSR | S_IRUGO, pa122_show_alslv, pa122_store_alslv);
static DRIVER_ATTR(alsval,  S_IWUSR | S_IRUGO, pa122_show_alsval, pa122_store_alsval);
static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO, pa122_show_trace,		pa122_store_trace);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, pa122_show_status, NULL);
static DRIVER_ATTR(send,    S_IWUSR | S_IRUGO, pa122_show_send, pa122_store_send); // No func
static DRIVER_ATTR(recv,    S_IWUSR | S_IRUGO, pa122_show_recv, pa122_store_recv);    // No func
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, pa122_show_reg, NULL);

static DRIVER_ATTR(pscalibration, S_IWUSR | S_IRUGO, pa122_show_ps_offset,pa122_set_ps_offset);
static DRIVER_ATTR(dev_init,S_IWUSR | S_IRUGO, NULL, pa122_store_dev_init);
//static DRIVER_ATTR(pthredcalibration, S_IWUSR | S_IRUGO,pa122_show_pthreshold_calibration, pa122_store_pthreshold_calibration);
static DRIVER_ATTR(xtalk_param, S_IWUSR | S_IRUGO,pa122_show_calibration_file, pa122_store_calibration_file);
static DRIVER_ATTR(pthrd_param, S_IWUSR | S_IRUGO,pa122_show_pthrd_file, pa122_store_pthrd_file);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *pa122_attr_list[] = {
	&driver_attr_version,    
	&driver_attr_als,
    &driver_attr_ps,    
    &driver_attr_trace,        /*trace log*/
    &driver_attr_config,
    &driver_attr_alslv,
    &driver_attr_alsval,
    &driver_attr_status,
    &driver_attr_send,
    &driver_attr_recv,
    &driver_attr_reg,
    &driver_attr_pscalibration,
    &driver_attr_dev_init,
   // &driver_attr_pthredcalibration,
    &driver_attr_xtalk_param,
    &driver_attr_pthrd_param,
};

/*----------------------------------------------------------------------------*/
static int pa122_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(pa122_attr_list)/sizeof(pa122_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, pa122_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", pa122_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
	static int pa122_delete_attr(struct device_driver *driver)
	{
	int idx ,err = 0;
	int num = (int)(sizeof(pa122_attr_list)/sizeof(pa122_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, pa122_attr_list[idx]);
	}
	
	return err;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------interrupt functions--------------------------------*/

/*----------------------------------------------------------------------------*/
static int pa122_check_intr(struct i2c_client *client) 
{
	struct pa122_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 psdata=0;
	u8 cfgdata=0;
	
	mutex_lock(&obj->update_lock);	
	res = hwmsen_read_byte(client, REG_PS_DATA, &psdata);
	mutex_unlock(&obj->update_lock);

	if(res<0)
	{
			APS_ERR("i2c_read function err res = %d\n",res);
			return -1;
	}
	

	switch (PA12_INT_TYPE)
	{
		case 1: /* Hysteresis Type */
			if(psdata > obj->ps_thrd_high){
	 			intr_flag = 0;
				mutex_lock(&obj->update_lock);
				hwmsen_write_byte(client,REG_CFG1,
								(PA12_LED_CURR << 4)| (1 << 2)| (PA12_ALS_PRST));
				mutex_unlock(&obj->update_lock);
				printk("------near------ps = %d\n",psdata);
			}else if(psdata < obj->ps_thrd_low){
				intr_flag = 1;
				mutex_lock(&obj->update_lock);
				hwmsen_write_byte(client,REG_CFG1,
								(PA12_LED_CURR << 4)| (PA12_PS_PRST << 2)| (PA12_ALS_PRST));
				mutex_unlock(&obj->update_lock);
				printk("------far------ps = %d\n",psdata);
			}
			/* No need to clear interrupt flag !! */
			goto EXIT_CHECK_INTR;
			break;			
		case 0: /* Window Type */
			if(intr_flag == 1){
				if(psdata > obj->ps_thrd_high){
					intr_flag = 0;

					mutex_lock(&obj->update_lock);
					hwmsen_write_byte(client, REG_PS_TL, obj->ps_thrd_low);
					hwmsen_write_byte(client, REG_PS_TH, PA12_PS_TH_MAX);
					hwmsen_write_byte(client,REG_CFG1,
								(PA12_LED_CURR << 4)| (1 << 2)| (PA12_ALS_PRST));
					mutex_unlock(&obj->update_lock);

					printk("------near------ps = %d\n",psdata);
				}
			}else if(intr_flag == 0){
				if(psdata < obj->ps_thrd_low){
					intr_flag = 1;

					mutex_lock(&obj->update_lock);
					hwmsen_write_byte(client, REG_PS_TL, PA12_PS_TH_MIN);
					hwmsen_write_byte(client, REG_PS_TH, obj->ps_thrd_high);
					hwmsen_write_byte(client,REG_CFG1,
								(PA12_LED_CURR << 4)| (PA12_PS_PRST << 2)| (PA12_ALS_PRST));
					mutex_unlock(&obj->update_lock);

					printk("------far------ps = %d\n",psdata);
				}		
			}
			break;
	}

	/* Clear PS INT FLAG */
	mutex_lock(&obj->update_lock);
	res = hwmsen_read_byte(client, REG_CFG2, &cfgdata);
	mutex_unlock(&obj->update_lock);

	if(res<0)
	{
		APS_ERR("i2c_read function err res = %d\n",res);
		return -1;
	}
	cfgdata = cfgdata & 0xFD ; 
	mutex_lock(&obj->update_lock);
	res = hwmsen_write_byte(client,REG_CFG2,cfgdata);
	mutex_unlock(&obj->update_lock);
  	if(res<0)
  	{		
		APS_ERR("i2c_send function err res = %d\n",res);
		return -1;
	}
EXIT_CHECK_INTR:
	return 0;
}
/*----------------------------------------------------------------------------*/
static void pa122_eint_work(struct work_struct *work)
{
	struct pa122_priv *obj = (struct pa122_priv *)container_of(work, struct pa122_priv, eint_work);
	struct hwm_sensor_data  sensor_data;
	int res = 0;

#if 1
	res = pa122_check_intr(obj->client);
	printk("pa122_ps int work !\n");

	if(res != 0){
		goto EXIT_INTR_ERR;
	}else{
		sensor_data.values[0] = intr_flag;
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;	

	}
	if((res = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
		{
		  APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", res);
		  goto EXIT_INTR_ERR;
		}
#endif

	
	enable_irq(obj->irq);
	return;

EXIT_INTR_ERR:
	enable_irq(obj->irq);
	APS_ERR("pa122_eint_work err: %d\n", res);
}

/*----------------------------------------------------------------------------*/
static irqreturn_t pa122_eint_handler(int irq, void *desc)
{
	struct pa122_priv *obj = g_pa122_ptr;
	printk("[PA122] %s  %d",__FUNCTION__,__LINE__);
	if(!obj)
	{
		return IRQ_HANDLED;
	}	
	schedule_work(&obj->eint_work);

	disable_irq_nosync(pa122_obj->irq);

	return IRQ_HANDLED;
}

int pa122_setup_eint(struct i2c_client *client)
{
	int ret;
	u32 ints[2] = {0, 0};
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;        

	/*configure to GPIO function, external interrupt*/

   //alspsPltFmDev = get_alsps_platformdev();
   //printk("zhou--pa122==node=%s,prop_name=%s,prop_value=%s\n",(char *)alspsPltFmDev->dev.of_node->name,(char *)alspsPltFmDev->dev.of_node->properties->name,(char *)alspsPltFmDev->dev.of_node->properties->value);
   
   pa122_obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek,als_ps"); 
   printk("zhou--pa122==pa122_obj->irq_node->name=%s\n\n", pa122_obj->irq_node->name);

   
   alspsPltFmDev = get_alsps_platformdev();
/* gpio setting */
	pinctrl = devm_pinctrl_get(&alspsPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		printk("zhou----pa122----Cannot find alsps pinctrl!\n");
	}
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		printk("zhou----Cannot find alsps pinctrl default!\n");

	}

	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		printk("zhou----Cannot find alsps pinctrl pin_cfg!\n");
	}
	
	pinctrl_select_state(pinctrl, pins_cfg);
	

/* eint request */
	if (pa122_obj->irq_node) {
		of_property_read_u32_array(pa122_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_request(ints[0], "p-sensor");
		gpio_set_debounce(ints[0], ints[1]);
		//pinctrl_select_state(pinctrl, pins_cfg);
		printk("zhou-----pa122 ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
    printk("zhou--pa122==pa122_obj->irq_node->name=%s\n\n", pa122_obj->irq_node->name); 
       
		pa122_obj->irq = irq_of_parse_and_map(pa122_obj->irq_node, 0);
		printk("zhou---pa122_obj->irq = %d\n", pa122_obj->irq);
		pa122_obj->irq=290;
		
		
		if (!pa122_obj->irq) {
			printk("zhou----pa122 irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}else
		  printk("zhou----pa122 irq_of_parse_and_map OK!!\n"); 
		     
		if (request_irq(pa122_obj->irq, pa122_eint_handler, IRQF_TRIGGER_LOW, "ALS-eint", NULL)) {
			printk("zhou---pa122 IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		enable_irq(pa122_obj->irq);
	} else {
		printk("zhou---null irq node!!\n");
		return -EINVAL;
	}
	return 0;
}
/*-------------------------------MISC device related------------------------------------------*/



/************************************************************/
static int pa122_open(struct inode *inode, struct file *file)
{
	file->private_data = pa122_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/************************************************************/

static int pa122_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/************************************************************/
static long pa122_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
		struct i2c_client *client = (struct i2c_client*)file->private_data;
		struct pa122_priv *obj = i2c_get_clientdata(client);  
		long err = 0;
		void __user *ptr = (void __user*) arg;
		int dat;
		uint32_t enable;
		//int ps_result;

		switch (cmd)
		{
			case ALSPS_SET_PS_MODE:
				if(copy_from_user(&enable, ptr, sizeof(enable)))
				{
					err = -EFAULT;
					goto err_out;
				}
				if(enable)
				{
					if((err = pa122_enable_ps(obj->client, 1)))
					{
						APS_ERR("enable ps fail: %ld\n", err); 
						goto err_out;
					}

					set_bit(CMC_BIT_PS, &obj->enable);
				}
				else
				{
					if((err = pa122_enable_ps(obj->client, 0)))
					{
						APS_ERR("disable ps fail: %ld\n", err); 
						goto err_out;
					}
					clear_bit(CMC_BIT_PS, &obj->enable);
				}
				break;

			case ALSPS_GET_PS_MODE:
				enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
				if(copy_to_user(ptr, &enable, sizeof(enable)))
				{
					err = -EFAULT;
					goto err_out;
				}
				break;

			case ALSPS_GET_PS_DATA:    
				if((err = pa122_read_ps(obj->client, &obj->ps)))
				{
					goto err_out;
				}

				dat = pa122_get_ps_value(obj, obj->ps);
				if(copy_to_user(ptr, &dat, sizeof(dat)))
				{
					err = -EFAULT;
					goto err_out;
				}
				break;

			case ALSPS_GET_PS_RAW_DATA:    
				if((err = pa122_read_ps(obj->client, &obj->ps)))
				{
					goto err_out;
				}

				dat = obj->ps;
				if(copy_to_user(ptr, &dat, sizeof(dat)))
				{
					err = -EFAULT;
					goto err_out;
				}
				break;			  

			case ALSPS_SET_ALS_MODE:

				if(copy_from_user(&enable, ptr, sizeof(enable)))
				{
					err = -EFAULT;
					goto err_out;
				}
				if(enable)
				{
					if((err = pa122_enable_als(obj->client, 1)))
					{
						APS_ERR("enable als fail: %ld\n", err); 
						goto err_out;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
				}
				else
				{
					if((err = pa122_enable_als(obj->client, 0)))
					{
						APS_ERR("disable als fail: %ld\n", err); 
						goto err_out;
					}
					clear_bit(CMC_BIT_ALS, &obj->enable);
				}
				break;

			case ALSPS_GET_ALS_MODE:
				enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
				if(copy_to_user(ptr, &enable, sizeof(enable)))
				{
					err = -EFAULT;
					goto err_out;
				}
				break;

			case ALSPS_GET_ALS_DATA: 
				if((err = pa122_read_als(obj->client, &obj->als)))
				{
					goto err_out;
				}
	
				dat = pa122_get_als_value(obj, obj->als);
				if(copy_to_user(ptr, &dat, sizeof(dat)))
				{
					err = -EFAULT;
					goto err_out;
				}
				break;

			case ALSPS_GET_ALS_RAW_DATA:	
				if((err = pa122_read_als(obj->client, &obj->als)))
				{
					goto err_out;
				}

				dat = obj->als;
				if(copy_to_user(ptr, &dat, sizeof(dat)))
				{
					err = -EFAULT;
					goto err_out;
				}
				break;

			case ALSPS_IOCTL_CLR_CALI:
				APS_ERR("%s ALSPS_IOCTL_CLR_CALI\n", __func__);
				if(copy_from_user(&dat, ptr, sizeof(dat)))
				{
					err = -EFAULT;
					goto err_out;
				}
				break;

			case ALSPS_IOCTL_GET_CALI:
				dat = obj->crosstalk ;
				APS_ERR("%s set ps_cali %x\n", __func__, dat);
				if(copy_to_user(ptr, &dat, sizeof(dat)))
				{
					err = -EFAULT;
					goto err_out;
				}
				break;

			case ALSPS_IOCTL_SET_CALI:
				APS_ERR("%s set ps_cali %x\n", __func__, obj->crosstalk); 
				break;

			default:
				APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
				err = -ENOIOCTLCMD;
				break;
		}

err_out:
		return err;    
}

/********************************************************************/
/*------------------------------misc device related operation functions------------------------------------*/
static struct file_operations pa122_fops = {
	.owner = THIS_MODULE,
	.open = pa122_open,
	.release = pa122_release,
	.unlocked_ioctl = pa122_unlocked_ioctl,
};

static struct miscdevice pa122_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &pa122_fops,
};

/*--------------------------------------------------------------------------------*/
static int pa122_init_client(struct i2c_client *client)
{
	struct pa122_priv *obj = i2c_get_clientdata(client);
	u8 sendvalue=0;
	int res = 0;
	int intmode ;
	// Initialize Sensor
	mutex_lock(&obj->update_lock);
	res=hwmsen_write_byte(client,REG_CFG0,
		PA12_ALS_GAIN << 4);

	if(res < 0)
	{
		APS_ERR("i2c write  err\n");
		goto EXIT_ERR;
	}	

	res=hwmsen_write_byte(client,REG_CFG1,
		(PA12_LED_CURR << 4)| (PA12_PS_PRST << 2)| (PA12_ALS_PRST));

	res=hwmsen_write_byte(client,REG_CFG2,
		(PA12_PS_MODE << 6)| (PA12_PS_SET << 2));

	res=hwmsen_write_byte(client,REG_CFG3,
		(PA12_INT_TYPE << 6)| (PA12_PS_PERIOD << 3)| (PA12_ALS_PERIOD));

	res=hwmsen_write_byte(client,REG_PS_SET,0x03); 

	obj->crosstalk_base = 10;
	obj->crosstalk = PA12_PS_OFFSET_DEFAULT;
	hwmsen_write_byte(client, REG_PS_OFFSET, obj->crosstalk); 

	obj->ps_thrd_low = PA12_PS_TH_LOW;
	obj->ps_thrd_high = PA12_PS_TH_HIGH;

	if(obj->hw->polling_mode_als == 0)
	{
		/* Set ALS threshold */
		res=hwmsen_write_byte(client, REG_ALS_TH_MSB, PA12_ALS_TH_HIGH >> 8); 
		res=hwmsen_write_byte(client, REG_ALS_TH_LSB, PA12_ALS_TH_HIGH & 0xFF); 
		res=hwmsen_write_byte(client, REG_ALS_TL_MSB, PA12_ALS_TH_LOW >> 8); 
		res=hwmsen_write_byte(client, REG_ALS_TL_LSB, PA12_ALS_TH_LOW & 0xFF); 
	}

	if(obj->hw->polling_mode_ps == 0)
	{
		/* Set PS threshold */
		if(PA12_INT_TYPE == 0)
		{
			/*Window Type */	
			res=hwmsen_write_byte(client, REG_PS_TH, obj->ps_thrd_high); 
			res=hwmsen_write_byte(client, REG_PS_TL, PA12_PS_TH_MIN); 
		}
		else if(PA12_INT_TYPE == 1)
		{
			/*Hysteresis Type */
			res=hwmsen_write_byte(client, REG_PS_TH, obj->ps_thrd_high); 
			res=hwmsen_write_byte(client, REG_PS_TL, obj->ps_thrd_low); 
		}
	}

	// Polling Setting	
	 intmode = obj->hw->polling_mode_ps << 1 | obj->hw->polling_mode_als;

	res=hwmsen_read_byte(client,REG_CFG2,&sendvalue);
	sendvalue &= 0xF0; 

	switch(intmode)
	{

		case 0:
			sendvalue |= 0x0C; 
			res=hwmsen_write_byte(client,REG_CFG2,sendvalue); //set int mode
			break;
		case 1:
			sendvalue |= 0x04; 
			res=hwmsen_write_byte(client,REG_CFG2,sendvalue); //set int mode
			break;
		case 2:
			sendvalue=sendvalue | 0x00; 
			res=hwmsen_write_byte(client,REG_CFG2,sendvalue); //set int mode
			break;
		default:
			sendvalue |= 0x04;  
			res=hwmsen_write_byte(client,REG_CFG2,sendvalue); //set int mode
			break;

	}

	mutex_unlock(&obj->update_lock);

	if(res < 0)
	{
		APS_ERR("i2c_send function err\n");
		goto EXIT_ERR;
	}

	// Regsit int
	res = pa122_setup_eint(client);
	if(res!=0)
	{
		APS_ERR("PA122 setup eint: %d\n", res);
		return res;
	}

	return 0;

EXIT_ERR:
	APS_ERR("pa122 init dev fail!!!!: %d\n", res);
	return res;
}
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
int pa122_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
		int err = 0;
		int value;
		struct hwm_sensor_data* sensor_data;
		struct pa122_priv *obj = (struct pa122_priv *)self;		
		//APS_FUN(f);
		switch (command)
		{
			case SENSOR_DELAY:
				APS_ERR("pa122 ps delay command!\n");
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					APS_ERR("Set delay parameter error!\n");
					err = -EINVAL;
				}
				break;

			case SENSOR_ENABLE:
				APS_ERR("pa122 ps enable command!\n");
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					APS_ERR("Enable sensor parameter error!\n");
					err = -EINVAL;
				}
				else
				{				
					value = *(int *)buff_in;
					if(value)
					{
						if((err = pa122_enable_ps(obj->client, 1)))
						{
							APS_ERR("enable ps fail: %d\n", err); 
							return -1;
						}
						set_bit(CMC_BIT_PS, &obj->enable);
					}
					else
					{
						if((err = pa122_enable_ps(obj->client, 0)))
						{
							APS_ERR("disable ps fail: %d\n", err); 
							return -1;
						}
						clear_bit(CMC_BIT_PS, &obj->enable);
					}
				}
				break;

			case SENSOR_GET_DATA:
				//APS_ERR("pa122 ps get data command!\n");
				if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
				{
					APS_ERR("get sensor data parameter error!\n");
					err = -EINVAL;
				}
				else
				{
					sensor_data = (struct hwm_sensor_data *)buff_out;				
					
					if((err = pa122_read_ps(obj->client, &obj->ps)))
					{
						err = -1;;
					}
					else
					{
						sensor_data->values[0] = pa122_get_ps_value(obj, obj->ps);
						sensor_data->value_divide = 1;
						sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
					}				
				}
				break;
			default:
				APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
				err = -1;
				break;
		}

		return err;

}

int pa122_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
		int err = 0;
		int value;
		struct hwm_sensor_data* sensor_data;
		struct pa122_priv *obj = (struct pa122_priv *)self;
		APS_FUN(f);
		switch (command)
		{
			case SENSOR_DELAY:
				APS_ERR("pa122 als delay command!\n");
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					APS_ERR("Set delay parameter error!\n");
					err = -EINVAL;
				}
				break;

			case SENSOR_ENABLE:
				APS_ERR("pa122 als enable command!\n");
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					APS_ERR("Enable sensor parameter error!\n");
					err = -EINVAL;
				}
				else
				{
					value = *(int *)buff_in;				
					if(value)
					{
						if((err = pa122_enable_als(obj->client, 1)))
						{
							APS_ERR("enable als fail: %d\n", err); 
							return -1;
						}
						set_bit(CMC_BIT_ALS, &obj->enable);
					}
					else
					{
						if((err = pa122_enable_als(obj->client, 0)))
						{
							APS_ERR("disable als fail: %d\n", err); 
							return -1;
						}
						clear_bit(CMC_BIT_ALS, &obj->enable);
					}

				}
				break;
	
			case SENSOR_GET_DATA:
				APS_ERR("pa122 als get data command!\n");
				if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
				{
					APS_ERR("get sensor data parameter error!\n");
					err = -EINVAL;
				}
				else
				{
					sensor_data = (struct hwm_sensor_data *)buff_out;
									
					if((err = pa122_read_als(obj->client, &obj->als)))
					{
						err = -1;
					}
					else
					{
						#if defined(MTK_AAL_SUPPORT)
						sensor_data->values[0] = obj->als;
						#else
						sensor_data->values[0] = pa122_get_als_value(obj, obj->als);
						#endif
						sensor_data->value_divide = 1;
						sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
					}				
				}
				break;
			default:
				APS_ERR("light sensor operate function no this parameter %d!\n", command);
				err = -1;
				break;
		}
		
		return err;

}
/*--------------------------------------------------------------------------------*/
/*
static ssize_t alsps_calibration_write(struct file *f, char __user *buf, size_t count, loff_t *pos)
{
	if(pa122_run_calibration(pa122_i2c_client) < 0)
	{
		return -EFAULT;
	}

	pa122_has_load_cal_file = 1;

	return count;
}

static int alsps_calibration_open(struct inode *inode, struct file *f)
{
	return 0;
}

static int alsps_calibration_release(struct inode *inode, struct file *f)
{
	return 0;
}
*/
/*
static const struct file_operations alsps_calibration_fops = {
	.owner = THIS_MODULE,
	.open = alsps_calibration_open,
	.release = alsps_calibration_release,
	.write = alsps_calibration_write,
};

static struct miscdevice alsps_calibration_struct = {
	.name = "alsps_cali",
	.fops = &alsps_calibration_fops,
	.minor = MISC_DYNAMIC_MINOR,
};
*/
/*-----------------------------------i2c operations----------------------------------*/
static int pa122_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct pa122_priv *obj;
	struct hwmsen_object obj_ps, obj_als;

	int err = 0;

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}

	pa122_obj = obj;


	obj->hw = hw;

	mutex_init(&obj->update_lock); 

	INIT_WORK(&obj->eint_work, pa122_eint_work);

	obj->client = client;
	i2c_set_clientdata(client, obj);

	/*-----------------------------value need to be confirmed-----------------------------------------*/
	atomic_set(&obj->als_debounce, 200);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 200);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->als_cmd_val, 0xDF);
	atomic_set(&obj->ps_cmd_val,  0xC1);
	atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
	atomic_set(&obj->als_thd_val_high,  obj->hw->als_threshold_high);
	atomic_set(&obj->als_thd_val_low,  obj->hw->als_threshold_low);

	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
	/*-----------------------------value need to be confirmed-----------------------------------------*/

//	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
//	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);

	pa122_i2c_client = client;

	if((err = pa122_init_client(client)))
	{
		goto exit_init_failed;
	}
	printk("pa122_init_client() OK!\n");

	if((err = misc_register(&pa122_device)))
	{
		printk("pa122_device register failed\n");
		goto exit_misc_device_register_failed;
	}
	printk("pa122_device misc_register OK!\n");

	/*------------------------sl22201001 attribute file for debug--------------------------------------*/
	if((err = pa122_create_attr(&(pa122_init_info.platform_diver_addr->driver))))
	{
	printk("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	/*------------------------s12201001 attribute file for debug--------------------------------------*/

	obj_ps.self = pa122_obj;
	obj_ps.polling = obj->hw->polling_mode_ps;	
	obj_ps.sensor_operate = pa122_ps_operate;
	if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	obj_als.self = pa122_obj;
	obj_als.polling = obj->hw->polling_mode_als;;
	obj_als.sensor_operate = pa122_als_operate;
	if((err = hwmsen_attach(ID_LIGHT, &obj_als)))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2,
	obj->early_drv.suspend  = pa122_early_suspend,
	obj->early_drv.resume   = pa122_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif
  printk("==mlk== %s  is ok !",__func__);
	//if (misc_register(&alsps_calibration_struct) < 0)
	//{
	//	APS_ERR("Creat alsps_calibration_struct device file error!!\n");
	//}

	pa122_init_flag =0;

	return 0;

exit_create_attr_failed:
exit_sensor_obj_attach_fail:
exit_misc_device_register_failed:
		misc_deregister(&pa122_device);
exit_init_failed:
	kfree(obj);
exit:
	pa122_i2c_client = NULL;           
	APS_ERR("%s: err = %d\n", __func__, err);
	pa122_init_flag =-1;
	return err;
}

static int pa122_i2c_remove(struct i2c_client *client)
{
	int err;	
	/*------------------------pa122 attribute file for debug--------------------------------------*/	
	if((err = pa122_delete_attr(&pa122_i2c_driver.driver)))
	{
		APS_ERR("pa122_delete_attr fail: %d\n", err);
	} 
	/*----------------------------------------------------------------------------------------*/
	
	if((err = misc_deregister(&pa122_device)))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
		
	pa122_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;

}

static int pa122_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, PA122_DEV_NAME);
	return 0;

}

static int pa122_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
	APS_FUN();
	return 0;
}

static int pa122_i2c_resume(struct i2c_client *client)
{
	APS_FUN();
	return 0;
}

/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
static int pa122_local_uninit(void)
{
	i2c_del_driver(&pa122_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/



static int pa122_local_init(void) {
	
	if(i2c_add_driver(&pa122_i2c_driver))	
	{		
		APS_ERR("add driver error\n");		
		return -1;	
	}
	
	if(-1 == pa122_init_flag)    
	{      
		return -1;    
	}
	
	return 0;
}

/*----------------------------------------------------------------------------*/
static int __init pa122_init(void)
{

	const char *name = "mediatek,pa122";	
	
	printk("pa122_init In");

	hw = get_alsps_dts_func(name, hw);
	if (!hw) 
	{
		printk("get dts info fail\n");
	}
   	alsps_driver_add(&pa122_init_info);

#if 0
	pa122_regulator = regulator_get(&alspsPltFmDev->dev,"vpa122");
	if(!pa122_regulator)
		printk("pa122_regulator is NULL\n");
	ret = regulator_set_voltage(pa122_regulator, 2800000, 2800000); /*set 2.8v*/
	if (ret){
		printk("regulator_set_voltage() failed!\n");
	}
	msleep(3);
#endif
	printk("pa122_init Out");
		
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit pa122_exit(void)
{
	APS_FUN();
}
/*----------------------------------------------------------------------------*/
module_init(pa122_init);
module_exit(pa122_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("TXC Corp");
MODULE_DESCRIPTION("pa122 driver");
MODULE_LICENSE("GPL");

