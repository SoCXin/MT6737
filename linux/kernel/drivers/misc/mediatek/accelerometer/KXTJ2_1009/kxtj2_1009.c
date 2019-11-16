/* BMA150 motion sensor driver
 *
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

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include "upmu_sw.h"
#include "upmu_common.h"
#include "batch.h"
#include <hwmsen_helper.h>

#define POWER_NONE_MACRO MT65XX_POWER_NONE

#include <cust_acc.h>
#include "kxtj2_1009.h"

#include <accel.h>
#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
#include <SCP_sensorHub.h>
#endif				/* #ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB */

/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_KXTJ2_1009 222
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/

#define SW_CALIBRATION

/*----------------------------------------------------------------------------*/
#define KXTJ2_1009_AXIS_X          0
#define KXTJ2_1009_AXIS_Y          1
#define KXTJ2_1009_AXIS_Z          2
#define KXTJ2_1009_AXES_NUM        3
#define KXTJ2_1009_DATA_LEN        6
#define KXTJ2_1009_DEV_NAME        "KXTJ2_1009"
/*----------------------------------------------------------------------------*/

/*********/
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id kxtj2_1009_i2c_id[] = { {KXTJ2_1009_DEV_NAME, 0}, {} };



#define COMPATIABLE_NAME "mediatek,kxtj2_1009"

/*----------------------------------------------------------------------------*/
static int kxtj2_1009_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int kxtj2_1009_i2c_remove(struct i2c_client *client);
static int kxtj2_1009_suspend(struct i2c_client *client, pm_message_t msg);
static int kxtj2_1009_resume(struct i2c_client *client);

static int gsensor_local_init(void);
static int gsensor_remove(void);
#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
static int gsensor_setup_irq(void);
#endif				/* #ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB */
static int gsensor_set_delay(u64 ns);
/*----------------------------------------------------------------------------*/
enum ADX_TRC {
	ADX_TRC_FILTER = 0x01,
	ADX_TRC_RAWDATA = 0x02,
	ADX_TRC_IOCTL = 0x04,
	ADX_TRC_CALI = 0X08,
	ADX_TRC_INFO = 0X10,
};
/*----------------------------------------------------------------------------*/
struct scale_factor {
	u8 whole;
	u8 fraction;
};
/*----------------------------------------------------------------------------*/
struct data_resolution {
	struct scale_factor scalefactor;
	int sensitivity;
};
/*----------------------------------------------------------------------------*/
#define C_MAX_FIR_LENGTH (32)
/*----------------------------------------------------------------------------*/
struct data_filter {
	s16 raw[C_MAX_FIR_LENGTH][KXTJ2_1009_AXES_NUM];
	int sum[KXTJ2_1009_AXES_NUM];
	int num;
	int idx;
};
/*----------------------------------------------------------------------------*/
struct kxtj2_1009_i2c_data {
	struct i2c_client *client;
	struct acc_hw *hw;
	struct hwmsen_convert cvt;
#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
	struct work_struct irq_work;
#endif				/* #ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB */

	/*misc */
	struct data_resolution *reso;
	atomic_t trace;
	atomic_t suspend;
	atomic_t selftest;
	atomic_t filter;
	s16 cali_sw[KXTJ2_1009_AXES_NUM + 1];

	/*data */
	s8 offset[KXTJ2_1009_AXES_NUM + 1];	/*+1: for 4-byte alignment */
	s16 data[KXTJ2_1009_AXES_NUM + 1];

#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
	int SCP_init_done;
#endif				/* #ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB */


#if defined(CONFIG_KXTJ2_1009_LOWPASS)
	atomic_t firlen;
	atomic_t fir_en;
	struct data_filter fir;
#endif
	u8 bandwidth;
};
/*----------------------------------------------------------------------------*/

static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,gsensor"},
	{},
};

static struct i2c_driver kxtj2_1009_i2c_driver = {
	.driver = {
/* .owner          = THIS_MODULE, */
		   .name = KXTJ2_1009_DEV_NAME,
		   .of_match_table = accel_of_match,
		   },
	.probe = kxtj2_1009_i2c_probe,
	.remove = kxtj2_1009_i2c_remove,
	.suspend = kxtj2_1009_suspend,
	.resume = kxtj2_1009_resume,
	.id_table = kxtj2_1009_i2c_id,
/* .address_data = &kxtj2_1009_addr_data, */
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *kxtj2_1009_i2c_client;
static struct kxtj2_1009_i2c_data *obj_i2c_data;
static bool sensor_power = true;

static struct GSENSOR_VECTOR3D gsensor_gain;
/* static char selftestRes[8]= {0}; */
static DEFINE_MUTEX(gsensor_mutex);
static DEFINE_MUTEX(gsensor_scp_en_mutex);


static bool enable_status = false;

static int gsensor_init_flag = -1;	/* 0<==>OK -1 <==> fail */
static struct acc_init_info kxtj2_1009_init_info = {
	.name = KXTJ2_1009_DEV_NAME,
	.init = gsensor_local_init,
	.uninit = gsensor_remove,
};

/*----------------------------------------------------------------------------*/
#define KXTJ2_1009E_DEBUG 0
#define GSE_TAG                  "[Gsensor] "
#define GSE_ERR(fmt, args...)    pr_err(GSE_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#ifdef KXTJ2_1009E_DEBUG
#define GSE_FUN(f)               pr_debug(GSE_TAG"%s\n", __func__)
#define GSE_LOG(fmt, args...)    pr_debug(GSE_TAG fmt, ##args)
#else
#define GSE_FUN(f)
#define GSE_LOG(fmt, args...)
#endif

struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;


/*----------------------------------------------------------------------------*/
static struct data_resolution kxtj2_1009_data_resolution[1] = {
	/* combination by {FULL_RES,RANGE} */
	{{0, 9}, 1024},
};

/*----------------------------------------------------------------------------*/
static struct data_resolution kxtj2_1009_offset_resolution = { {15, 6}, 64 };

/*----------------------------------------------------------------------------*/

/*--------------------Add by Susan----------------------------------*/
#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
int KXTJ2_1009_SCP_SetPowerMode(bool enable, int sensorType)
{
	static bool gsensor_scp_en_status;
	static unsigned int gsensor_scp_en_map;
	SCP_SENSOR_HUB_DATA req;
	int len;
	int err = 0;

	mutex_lock(&gsensor_scp_en_mutex);

	if (sensorType >= 32) {
		GSE_ERR("Out of index!\n");
		return -1;
	}

	if (true == enable)
		gsensor_scp_en_map |= (1 << sensorType);
	else
		gsensor_scp_en_map &= ~(1 << sensorType);

	if (0 == gsensor_scp_en_map)
		enable = false;
	else
		enable = true;

	if (gsensor_scp_en_status != enable) {
		gsensor_scp_en_status = enable;

		req.activate_req.sensorType = ID_ACCELEROMETER;
		req.activate_req.action = SENSOR_HUB_ACTIVATE;
		req.activate_req.enable = enable;
		len = sizeof(req.activate_req);
		err = SCP_sensorHub_req_send(&req, &len, 1);
		if (err)
			GSE_ERR("SCP_sensorHub_req_send fail\n");
	}

	mutex_unlock(&gsensor_scp_en_mutex);

	return err;
}
EXPORT_SYMBOL(KXTJ2_1009_SCP_SetPowerMode);
#endif
/*----------------------------------------------------------------------------*/
/*--------------------KXTJ2_1009 power control function----------------------------------*/
static void KXTJ2_1009_power(struct acc_hw *hw, unsigned int on)
{
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_SetDataResolution(struct kxtj2_1009_i2c_data *obj)
{

	obj->reso = &kxtj2_1009_data_resolution[0];
	return 0;
}

/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_ReadData(struct i2c_client *client, s16 data[KXTJ2_1009_AXES_NUM])
{
	struct kxtj2_1009_i2c_data *priv = i2c_get_clientdata(client);        
	u8 addr = KXTJ2_1009_REG_DATAX0;
	u8 buf[KXTJ2_1009_DATA_LEN] = {0};
	int err = 0;
	int i;

	if(NULL == client)
	{
		err = -EINVAL;
	}
	else if((err = hwmsen_read_block(client, addr, buf, 0x06)) != 0)
	{
		GSE_ERR("error: %d\n", err);
	}
	else
	{
		data[KXTJ2_1009_AXIS_X] = (s16)((buf[KXTJ2_1009_AXIS_X*2] >> 4) |
		         (buf[KXTJ2_1009_AXIS_X*2+1] << 4));
		data[KXTJ2_1009_AXIS_Y] = (s16)((buf[KXTJ2_1009_AXIS_Y*2] >> 4) |
		         (buf[KXTJ2_1009_AXIS_Y*2+1] << 4));
		data[KXTJ2_1009_AXIS_Z] = (s16)((buf[KXTJ2_1009_AXIS_Z*2] >> 4) |
		         (buf[KXTJ2_1009_AXIS_Z*2+1] << 4));

		for(i=0;i<3;i++)				
		{								//because the data is store in binary complement number formation in computer system
			if ( data[i] == 0x0800 )	//so we want to calculate actual number here
				data[i]= -2048;			//10bit resolution, 512= 2^(12-1)
			else if ( data[i] & 0x0800 )//transfor format
			{							//printk("data 0 step %x \n",data[i]);
				data[i] -= 0x1; 		//printk("data 1 step %x \n",data[i]);
				data[i] = ~data[i]; 	//printk("data 2 step %x \n",data[i]);
				data[i] &= 0x07ff;		//printk("data 3 step %x \n\n",data[i]);
				data[i] = -data[i]; 	
			}
		}	


		if(atomic_read(&priv->trace) & ADX_TRC_RAWDATA)
		{
			GSE_LOG("[%08X %08X %08X] => [%5d %5d %5d]\n", data[KXTJ2_1009_AXIS_X], data[KXTJ2_1009_AXIS_Y], data[KXTJ2_1009_AXIS_Z],
		                               data[KXTJ2_1009_AXIS_X], data[KXTJ2_1009_AXIS_Y], data[KXTJ2_1009_AXIS_Z]);
		}
#ifdef CONFIG_KXTJ2_1009_LOWPASS
		if(atomic_read(&priv->filter))
		{
			if(atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend))
			{
				int idx, firlen = atomic_read(&priv->firlen);   
				if(priv->fir.num < firlen)
				{                
					priv->fir.raw[priv->fir.num][KXTJ2_1009_AXIS_X] = data[KXTJ2_1009_AXIS_X];
					priv->fir.raw[priv->fir.num][KXTJ2_1009_AXIS_Y] = data[KXTJ2_1009_AXIS_Y];
					priv->fir.raw[priv->fir.num][KXTJ2_1009_AXIS_Z] = data[KXTJ2_1009_AXIS_Z];
					priv->fir.sum[KXTJ2_1009_AXIS_X] += data[KXTJ2_1009_AXIS_X];
					priv->fir.sum[KXTJ2_1009_AXIS_Y] += data[KXTJ2_1009IK_AXIS_Y];
					priv->fir.sum[KXTJ2_1009_AXIS_Z] += data[KXTJ2_1009_AXIS_Z];
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
							priv->fir.raw[priv->fir.num][KXTJ2_1009_AXIS_X], priv->fir.raw[priv->fir.num][KXTJ2_1009_AXIS_Y], priv->fir.raw[priv->fir.num][KXTJ2_1009_AXIS_Z],
							priv->fir.sum[KXTJ2_1009_AXIS_X], priv->fir.sum[KXTJ2_1009_AXIS_Y], priv->fir.sum[KXTJ2_1009_AXIS_Z]);
					}
					priv->fir.num++;
					priv->fir.idx++;
				}
				else
				{
					idx = priv->fir.idx % firlen;
					priv->fir.sum[KXTJ2_1009_AXIS_X] -= priv->fir.raw[idx][KXTJ2_1009_AXIS_X];
					priv->fir.sum[KXTJ2_1009_AXIS_Y] -= priv->fir.raw[idx][KXTJ2_1009_AXIS_Y];
					priv->fir.sum[KXTJ2_1009_AXIS_Z] -= priv->fir.raw[idx][KXTJ2_1009_AXIS_Z];
					priv->fir.raw[idx][KXTJ2_1009_AXIS_X] = data[KXTJ2_1009_AXIS_X];
					priv->fir.raw[idx][KXTJ2_1009_AXIS_Y] = data[KXTJ2_1009_AXIS_Y];
					priv->fir.raw[idx][KXTJ2_1009_AXIS_Z] = data[KXTJ2_1009_AXIS_Z];
					priv->fir.sum[KXTJ2_1009_AXIS_X] += data[KXTJ2_1009_AXIS_X];
					priv->fir.sum[KXTJ2_1009_AXIS_Y] += data[KXTJ2_1009_AXIS_Y];
					priv->fir.sum[KXTJ2_1009_AXIS_Z] += data[KXTJ2_1009_AXIS_Z];
					priv->fir.idx++;
					data[KXTJ2_1009_AXIS_X] = priv->fir.sum[KXTJ2_1009_AXIS_X]/firlen;
					data[KXTJ2_1009_AXIS_Y] = priv->fir.sum[KXTJ2_1009_AXIS_Y]/firlen;
					data[KXTJ2_1009_AXIS_Z] = priv->fir.sum[KXTJ2_1009_AXIS_Z]/firlen;
					if(atomic_read(&priv->trace) & ADX_TRC_FILTER)
					{
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
						priv->fir.raw[idx][KXTJ2_1009_AXIS_X], priv->fir.raw[idx][KXTJ2_1009_AXIS_Y], priv->fir.raw[idx][KXTJ2_1009_AXIS_Z],
						priv->fir.sum[KXTJ2_1009_AXIS_X], priv->fir.sum[KXTJ2_1009_AXIS_Y], priv->fir.sum[KXTJ2_1009_AXIS_Z],
						data[KXTJ2_1009_AXIS_X], data[KXTJ2_1009_AXIS_Y], data[KXTJ2_1009_AXIS_Z]);
					}
				}
			}
		}	
#endif         
	}
	return err;
}

/*----------------------------------------------------------------------------*/

static int KXTJ2_1009_ReadOffset(struct i2c_client *client, s8 ofs[KXTJ2_1009_AXES_NUM])
{    
	int err = 0;

	ofs[1]=ofs[2]=ofs[0]=0x00;

	printk("offesx=%x, y=%x, z=%x",ofs[0],ofs[1],ofs[2]);
	
	return err;
}

/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_ResetCalibration(struct i2c_client *client)
{
	struct kxtj2_1009_i2c_data *obj = i2c_get_clientdata(client);
	/* u8 ofs[4]={0,0,0,0}; */
	int err = 0;
#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA data;
	union KXTJ2_1009_CUST_DATA *pCustData;
	unsigned int len;
#endif

#ifdef GSENSOR_UT
	GSE_FUN();
#endif

#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
	if (0 != obj->SCP_init_done) {
		pCustData = (union KXTJ2_1009_CUST_DATA *) &data.set_cust_req.custData;

		data.set_cust_req.sensorType = ID_ACCELEROMETER;
		data.set_cust_req.action = SENSOR_HUB_SET_CUST;
		pCustData->resetCali.action = KXTJ2_1009_CUST_ACTION_RESET_CALI;
		len =
		    offsetof(SCP_SENSOR_HUB_SET_CUST_REQ, custData) + sizeof(pCustData->resetCali);
		SCP_sensorHub_req_send(&data, &len, 1);
	}
#endif

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	memset(obj->offset, 0x00, sizeof(obj->offset));
	return err;
}

/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_ReadCalibration(struct i2c_client *client, int dat[KXTJ2_1009_AXES_NUM])
{
    struct kxtj2_1009_i2c_data *obj = i2c_get_clientdata(client);
    int mul;

	#ifdef SW_CALIBRATION
		mul = 0;//only SW Calibration, disable HW Calibration
	#else
	    if ((err = KXTJ2_1009_ReadOffset(client, obj->offset))) {
        GSE_ERR("read offset fail, %d\n", err);
        return err;
    	}    
    	mul = obj->reso->sensitivity/kxtj2_1009_offset_resolution.sensitivity;
	#endif

    dat[obj->cvt.map[KXTJ2_1009_AXIS_X]] = obj->cvt.sign[KXTJ2_1009_AXIS_X]*(obj->offset[KXTJ2_1009_AXIS_X]*mul + obj->cali_sw[KXTJ2_1009_AXIS_X]);
    dat[obj->cvt.map[KXTJ2_1009_AXIS_Y]] = obj->cvt.sign[KXTJ2_1009_AXIS_Y]*(obj->offset[KXTJ2_1009_AXIS_Y]*mul + obj->cali_sw[KXTJ2_1009_AXIS_Y]);
    dat[obj->cvt.map[KXTJ2_1009_AXIS_Z]] = obj->cvt.sign[KXTJ2_1009_AXIS_Z]*(obj->offset[KXTJ2_1009_AXIS_Z]*mul + obj->cali_sw[KXTJ2_1009_AXIS_Z]);                        
                                       
    return 0;
}

/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_ReadCalibrationEx(struct i2c_client *client, int act[KXTJ2_1009_AXES_NUM], int raw[KXTJ2_1009_AXES_NUM])
{  
	/*raw: the raw calibration data; act: the actual calibration data*/
	struct kxtj2_1009_i2c_data *obj = i2c_get_clientdata(client);
    #ifdef SW_CALIBRATION
    #else
	int err;
    #endif
	int mul;

 

	#ifdef SW_CALIBRATION
		mul = 0;//only SW Calibration, disable HW Calibration
	#else
		if(err = KXTJ2_1009_ReadOffset(client, obj->offset))
		{
			GSE_ERR("read offset fail, %d\n", err);
			return err;
		}   
		mul = obj->reso->sensitivity/kxtj2_1009_offset_resolution.sensitivity;
	#endif
	
	raw[KXTJ2_1009_AXIS_X] = obj->offset[KXTJ2_1009_AXIS_X]*mul + obj->cali_sw[KXTJ2_1009_AXIS_X];
	raw[KXTJ2_1009_AXIS_Y] = obj->offset[KXTJ2_1009_AXIS_Y]*mul + obj->cali_sw[KXTJ2_1009_AXIS_Y];
	raw[KXTJ2_1009_AXIS_Z] = obj->offset[KXTJ2_1009_AXIS_Z]*mul + obj->cali_sw[KXTJ2_1009_AXIS_Z];

	act[obj->cvt.map[KXTJ2_1009_AXIS_X]] = obj->cvt.sign[KXTJ2_1009_AXIS_X]*raw[KXTJ2_1009_AXIS_X];
	act[obj->cvt.map[KXTJ2_1009_AXIS_Y]] = obj->cvt.sign[KXTJ2_1009_AXIS_Y]*raw[KXTJ2_1009_AXIS_Y];
	act[obj->cvt.map[KXTJ2_1009_AXIS_Z]] = obj->cvt.sign[KXTJ2_1009_AXIS_Z]*raw[KXTJ2_1009_AXIS_Z];                        
	                       
	return 0;
}

/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_WriteCalibration(struct i2c_client *client, int dat[KXTJ2_1009_AXES_NUM])
{
	struct kxtj2_1009_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	int cali[KXTJ2_1009_AXES_NUM], raw[KXTJ2_1009_AXES_NUM];
#ifdef SW_CALIBRATION
#else
    int lsb = kxtj2_1009_offset_resolution.sensitivity;
	int divisor = obj->reso->sensitivity/lsb;
#endif

	if(0 != (err = KXTJ2_1009_ReadCalibrationEx(client, cali, raw)))	/*offset will be updated in obj->offset*/
	{ 
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		raw[KXTJ2_1009_AXIS_X], raw[KXTJ2_1009_AXIS_Y], raw[KXTJ2_1009_AXIS_Z],
		obj->offset[KXTJ2_1009_AXIS_X], obj->offset[KXTJ2_1009_AXIS_Y], obj->offset[KXTJ2_1009_AXIS_Z],
		obj->cali_sw[KXTJ2_1009_AXIS_X], obj->cali_sw[KXTJ2_1009_AXIS_Y], obj->cali_sw[KXTJ2_1009_AXIS_Z]);

	/*calculate the real offset expected by caller*/
	cali[KXTJ2_1009_AXIS_X] += dat[KXTJ2_1009_AXIS_X];
	cali[KXTJ2_1009_AXIS_Y] += dat[KXTJ2_1009_AXIS_Y];
	cali[KXTJ2_1009_AXIS_Z] += dat[KXTJ2_1009_AXIS_Z];

	GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n", 
		dat[KXTJ2_1009_AXIS_X], dat[KXTJ2_1009_AXIS_Y], dat[KXTJ2_1009_AXIS_Z]);

#ifdef SW_CALIBRATION
	obj->cali_sw[KXTJ2_1009_AXIS_X] = obj->cvt.sign[KXTJ2_1009_AXIS_X]*(cali[obj->cvt.map[KXTJ2_1009_AXIS_X]]);
	obj->cali_sw[KXTJ2_1009_AXIS_Y] = obj->cvt.sign[KXTJ2_1009_AXIS_Y]*(cali[obj->cvt.map[KXTJ2_1009_AXIS_Y]]);
	obj->cali_sw[KXTJ2_1009_AXIS_Z] = obj->cvt.sign[KXTJ2_1009_AXIS_Z]*(cali[obj->cvt.map[KXTJ2_1009_AXIS_Z]]);	
#else
	obj->offset[KXTJ2_1009_AXIS_X] = (s8)(obj->cvt.sign[KXTJ2_1009_AXIS_X]*(cali[obj->cvt.map[KXTJ2_1009_AXIS_X]])/(divisor));
	obj->offset[KXTJ2_1009_AXIS_Y] = (s8)(obj->cvt.sign[KXTJ2_1009_AXIS_Y]*(cali[obj->cvt.map[KXTJ2_1009_AXIS_Y]])/(divisor));
	obj->offset[KXTJ2_1009_AXIS_Z] = (s8)(obj->cvt.sign[KXTJ2_1009_AXIS_Z]*(cali[obj->cvt.map[KXTJ2_1009_AXIS_Z]])/(divisor));

	/*convert software calibration using standard calibration*/
	obj->cali_sw[KXTJ2_1009_AXIS_X] = obj->cvt.sign[KXTJ2_1009_AXIS_X]*(cali[obj->cvt.map[KXTJ2_1009_AXIS_X]])%(divisor);
	obj->cali_sw[KXTJ2_1009_AXIS_Y] = obj->cvt.sign[KXTJ2_1009_AXIS_Y]*(cali[obj->cvt.map[KXTJ2_1009_AXIS_Y]])%(divisor);
	obj->cali_sw[KXTJ2_1009_AXIS_Z] = obj->cvt.sign[KXTJ2_1009_AXIS_Z]*(cali[obj->cvt.map[KXTJ2_1009_AXIS_Z]])%(divisor);

	GSE_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
		obj->offset[KXTJ2_1009_AXIS_X]*divisor + obj->cali_sw[KXTJ2_1009_AXIS_X], 
		obj->offset[KXTJ2_1009_AXIS_Y]*divisor + obj->cali_sw[KXTJ2_1009_AXIS_Y], 
		obj->offset[KXTJ2_1009_AXIS_Z]*divisor + obj->cali_sw[KXTJ2_1009_AXIS_Z], 
		obj->offset[KXTJ2_1009_AXIS_X], obj->offset[KXTJ2_1009_AXIS_Y], obj->offset[KXTJ2_1009_AXIS_Z],
		obj->cali_sw[KXTJ2_1009_AXIS_X], obj->cali_sw[KXTJ2_1009_AXIS_Y], obj->cali_sw[KXTJ2_1009_AXIS_Z]);

	if(err = hwmsen_write_block(obj->client, KXTJ2_1009_REG_OFSX, obj->offset, KXTJ2_1009_AXES_NUM))
	{
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
#endif

	return err;
}

/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = KXTJ2_1009_REG_DEVID;   


   printk("zhoucheng=KXTJ2_1009_CheckDeviceID\n");
	res = i2c_master_send(client, databuf, 0x1); 
	if(res <= 0)
	{
		 printk("zhoucheng=KXTJ2_1009_CheckDeviceID send error\n");
		goto exit_KXTJ2_1009_CheckDeviceID;
	}
	
	udelay(500);

	databuf[0] = 0x0;        
	res = i2c_master_recv(client, databuf, 0x01);
	if(res <= 0)
	{
	  printk("zhoucheng=KXTJ2_1009_CheckDeviceID recv error\n");
		goto exit_KXTJ2_1009_CheckDeviceID;
	}
	

	if(false)
	{
		printk("KXTJ2_1009_CheckDeviceID 0x%x failt!\n ", databuf[0]);
		return KXTJ2_1009_ERR_IDENTIFICATION;
	}
	else
	{
		printk("KXTJ2_1009_CheckDeviceID 0x%x pass!\n ", databuf[0]);
	}
	
	exit_KXTJ2_1009_CheckDeviceID:
	if (res <= 0)
	{
		return KXTJ2_1009_ERR_I2C;
	}
	
	return KXTJ2_1009_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2];    
	int res = 0;
	u8 addr = KXTJ2_1009_REG_POWER_CTL;
	
	if(enable == sensor_power)
	{
		GSE_LOG("Sensor power status is newest!\n");
		return KXTJ2_1009_SUCCESS;
	}

	if(hwmsen_read_block(client, addr, databuf, 0x01))
	{
		GSE_ERR("read power ctl register err!\n");
		return KXTJ2_1009_ERR_I2C;
	}

	
	if(enable == true)
	{
		databuf[0] |= KXTJ2_1009_MEASURE_MODE;
	}
	else
	{
		databuf[0] &= ~KXTJ2_1009_MEASURE_MODE;
	}
	databuf[1] = databuf[0];
	databuf[0] = KXTJ2_1009_REG_POWER_CTL;
	

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return KXTJ2_1009_ERR_I2C;
	}


	GSE_LOG("KXTJ2_1009_SetPowerMode %d!\n ",enable);


	sensor_power = enable;

	mdelay(5);
	
	return KXTJ2_1009_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	struct kxtj2_1009_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[10];    
	int res = 0;
	bool cur_sensor_power = sensor_power;

	memset(databuf, 0, sizeof(u8)*10);  

	KXTJ2_1009_SetPowerMode(client, false);

	if(hwmsen_read_block(client, KXTJ2_1009_REG_DATA_FORMAT, databuf, 0x01))
	{
		printk("kxtj2_1009 read Dataformat failt \n");
		return KXTJ2_1009_ERR_I2C;
	}

	databuf[0] &= ~KXTJ2_1009_RANGE_MASK;
	databuf[0] |= dataformat;
	databuf[1] = databuf[0];
	databuf[0] = KXTJ2_1009_REG_DATA_FORMAT;


	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return KXTJ2_1009_ERR_I2C;
	}

	KXTJ2_1009_SetPowerMode(client, cur_sensor_power/*true*/);
	
	printk("KXTJ2_1009_SetDataFormat OK! \n");
	

	return KXTJ2_1009_SetDataResolution(obj);    
}
/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[10];    
	int res = 0;
	bool cur_sensor_power = sensor_power;

	memset(databuf, 0, sizeof(u8)*10);    

	
	KXTJ2_1009_SetPowerMode(client, false);

	if(hwmsen_read_block(client, KXTJ2_1009_REG_BW_RATE, databuf, 0x01))
	{
		printk("kxtj2_1009 read rate failt \n");
		return KXTJ2_1009_ERR_I2C;
	}

	databuf[0] &= 0xf0;
	databuf[0] |= bwrate;
	databuf[1] = databuf[0];
	databuf[0] = KXTJ2_1009_REG_BW_RATE;


	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return KXTJ2_1009_ERR_I2C;
	}

	
	KXTJ2_1009_SetPowerMode(client, cur_sensor_power/*true*/);
	printk("KXTJ2_1009_SetBWRate OK! \n");
	
	return KXTJ2_1009_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	u8 databuf[10];    
	int res = 0;

	memset(databuf, 0, sizeof(u8)*10);    
	databuf[0] = KXTJ2_1009_REG_INT_ENABLE;    
	databuf[1] = 0x00;

	res = i2c_master_send(client, databuf, 0x2);

	if(res <= 0)
	{
		return KXTJ2_1009_ERR_I2C;
	}
	
	return KXTJ2_1009_SUCCESS;    
}
/*----------------------------------------------------------------------------*/


static int kxtj2_1009_init_client(struct i2c_client *client, int reset_cali)
{
	struct kxtj2_1009_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;
	
	printk("zhoucheng=kxtj2_1009_init_client\n");

	res = KXTJ2_1009_CheckDeviceID(client); 
	if(res != KXTJ2_1009_SUCCESS)
	{
		return res;
	}	

	res = KXTJ2_1009_SetPowerMode(client, enable_status/*false*/);
	if(res != KXTJ2_1009_SUCCESS)
	{
		return res;
	}
	

	res = KXTJ2_1009_SetBWRate(client, KXTJ2_1009_BW_100HZ);
	if(res != KXTJ2_1009_SUCCESS ) //0x2C->BW=100Hz
	{
		return res;
	}

	res = KXTJ2_1009_SetDataFormat(client, KXTJ2_1009_RANGE_2G);
	if(res != KXTJ2_1009_SUCCESS) //0x2C->BW=100Hz
	{
		return res;
	}

	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;


	res = KXTJ2_1009_SetIntEnable(client, 0x00);        
	if(res != KXTJ2_1009_SUCCESS)//0x2E->0x80
	{
		return res;
	}

	if(0 != reset_cali)
	{ 
		/*reset calibration only in power on*/
		res = KXTJ2_1009_ResetCalibration(client);
		if(res != KXTJ2_1009_SUCCESS)
		{
			return res;
		}
	}
	printk("kxtj2_1009_init_client OK!\n");
#ifdef CONFIG_KXTJ2_1009_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));  
#endif

	return KXTJ2_1009_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
	u8 databuf[10];

	memset(databuf, 0, sizeof(u8) * 10);

	if ((NULL == buf) || (bufsize <= 30))
		return -1;

	if (NULL == client) {
		*buf = 0;
		return -2;
	}

	sprintf(buf, "KXTJ2_1009 Chip");
	return 0;
}

/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)  
{
	struct kxtj2_1009_i2c_data *obj = (struct kxtj2_1009_i2c_data*)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[KXTJ2_1009_AXES_NUM];
	int res = 0;
	memset(databuf, 0, sizeof(u8)*10);

	if(NULL == buf)
	{
		return -1;
	}
	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	if (atomic_read(&obj->suspend))
	{
		return 0;
	}
	/*if(sensor_power == FALSE)
	{
		res = KXTJ2_1009_SetPowerMode(client, true);
		if(res)
		{
			GSE_ERR("Power on kxtj2_1009 error %d!\n", res);
		}
	}*/

	if(0 != (res = KXTJ2_1009_ReadData(client, obj->data)))
	{        
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	else
	{
		//printk("raw data x=%d, y=%d, z=%d \n",obj->data[KXTJ2_1009_AXIS_X],obj->data[KXTJ2_1009_AXIS_Y],obj->data[KXTJ2_1009_AXIS_Z]);
		obj->data[KXTJ2_1009_AXIS_X] += obj->cali_sw[KXTJ2_1009_AXIS_X];
		obj->data[KXTJ2_1009_AXIS_Y] += obj->cali_sw[KXTJ2_1009_AXIS_Y];
		obj->data[KXTJ2_1009_AXIS_Z] += obj->cali_sw[KXTJ2_1009_AXIS_Z];
		
		//printk("cali_sw x=%d, y=%d, z=%d \n",obj->cali_sw[KXTJ2_1009_AXIS_X],obj->cali_sw[KXTJ2_1009_AXIS_Y],obj->cali_sw[KXTJ2_1009_AXIS_Z]);
		
		/*remap coordinate*/
		acc[obj->cvt.map[KXTJ2_1009_AXIS_X]] = obj->cvt.sign[KXTJ2_1009_AXIS_X]*obj->data[KXTJ2_1009_AXIS_X];
		acc[obj->cvt.map[KXTJ2_1009_AXIS_Y]] = obj->cvt.sign[KXTJ2_1009_AXIS_Y]*obj->data[KXTJ2_1009_AXIS_Y];
		acc[obj->cvt.map[KXTJ2_1009_AXIS_Z]] = obj->cvt.sign[KXTJ2_1009_AXIS_Z]*obj->data[KXTJ2_1009_AXIS_Z];
		//printk("cvt x=%d, y=%d, z=%d \n",obj->cvt.sign[KXTJ2_1009_AXIS_X],obj->cvt.sign[KXTJ2_1009_AXIS_Y],obj->cvt.sign[KXTJ2_1009_AXIS_Z]);


		//GSE_LOG("Mapped gsensor data: %d, %d, %d!\n", acc[KXTJ2_1009_AXIS_X], acc[KXTJ2_1009_AXIS_Y], acc[KXTJ2_1009_AXIS_Z]);

		//Out put the mg
		//printk("mg acc=%d, GRAVITY=%d, sensityvity=%d \n",acc[KXTJ2_1009_AXIS_X],GRAVITY_EARTH_1000,obj->reso->sensitivity);
		acc[KXTJ2_1009_AXIS_X] = acc[KXTJ2_1009_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[KXTJ2_1009_AXIS_Y] = acc[KXTJ2_1009_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		acc[KXTJ2_1009_AXIS_Z] = acc[KXTJ2_1009_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;		
		
	

		sprintf(buf, "%04x %04x %04x", acc[KXTJ2_1009_AXIS_X], acc[KXTJ2_1009_AXIS_Y], acc[KXTJ2_1009_AXIS_Z]);
		if(atomic_read(&obj->trace) & ADX_TRC_IOCTL)
		{
			GSE_LOG("gsensor data: %s!\n", buf);
		}
	}
	
	return 0;
}

/*----------------------------------------------------------------------------*/
static int KXTJ2_1009_ReadRawData(struct i2c_client *client, char *buf)
{
	struct kxtj2_1009_i2c_data *obj;
	int res = 0;

	obj = (struct kxtj2_1009_i2c_data *)i2c_get_clientdata(client);

	if (!buf || !client)
		return -EINVAL;

	res = KXTJ2_1009_ReadData(client, obj->data);
	if (0 != res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -EIO;
	}

	sprintf(buf, "KXTJ2_1009_ReadRawData %04x %04x %04x", obj->data[KXTJ2_1009_AXIS_X],
		obj->data[KXTJ2_1009_AXIS_Y], obj->data[KXTJ2_1009_AXIS_Z]);


	return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = kxtj2_1009_i2c_client;
	char strbuf[KXTJ2_1009_BUFSIZE];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	KXTJ2_1009_ReadChipInfo(client, strbuf, KXTJ2_1009_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

#if 0
static ssize_t gsensor_init(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = kxtj2_1009_i2c_client;
	char strbuf[KXTJ2_1009_BUFSIZE];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	kxtj2_1009_init_client(client, 1);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}
#endif


/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = kxtj2_1009_i2c_client;
	char strbuf[KXTJ2_1009_BUFSIZE];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	KXTJ2_1009_ReadSensorData(client, strbuf, KXTJ2_1009_BUFSIZE);
	/* BMA150_ReadRawData(client, strbuf); */
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

#if 0
static ssize_t show_sensorrawdata_value(struct device_driver *ddri, char *buf, size_t count)
{
	struct i2c_client *client = kxtj2_1009_i2c_client;
	char strbuf[KXTJ2_1009_BUFSIZE];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	/* BMA150_ReadSensorData(client, strbuf, BMA150_BUFSIZE); */
	KXTJ2_1009_ReadRawData(client, strbuf);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}
#endif

/*----------------------------------------------------------------------------*/
#if 1
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = kxtj2_1009_i2c_client;
	struct kxtj2_1009_i2c_data *obj;
	int err, len = 0, mul;
	int tmp[KXTJ2_1009_AXES_NUM];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);
	err = KXTJ2_1009_ReadOffset(client, obj->offset);
	if (0 != err)
		return -EINVAL;

	err = KXTJ2_1009_ReadCalibration(client, tmp);
	if (0 != err)
		return -EINVAL;

	mul = obj->reso->sensitivity / kxtj2_1009_offset_resolution.sensitivity;
	len +=
	    snprintf(buf + len, PAGE_SIZE - len,
		     "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,
		     obj->offset[KXTJ2_1009_AXIS_X], obj->offset[KXTJ2_1009_AXIS_Y],
		     obj->offset[KXTJ2_1009_AXIS_Z], obj->offset[KXTJ2_1009_AXIS_X],
		     obj->offset[KXTJ2_1009_AXIS_Y], obj->offset[KXTJ2_1009_AXIS_Z]);
	len +=
	    snprintf(buf + len, PAGE_SIZE - len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
		     obj->cali_sw[KXTJ2_1009_AXIS_X], obj->cali_sw[KXTJ2_1009_AXIS_Y],
		     obj->cali_sw[KXTJ2_1009_AXIS_Z]);

	len +=
	    snprintf(buf + len, PAGE_SIZE - len,
		     "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
		     obj->offset[KXTJ2_1009_AXIS_X] * mul + obj->cali_sw[KXTJ2_1009_AXIS_X],
		     obj->offset[KXTJ2_1009_AXIS_Y] * mul + obj->cali_sw[KXTJ2_1009_AXIS_Y],
		     obj->offset[KXTJ2_1009_AXIS_Z] * mul + obj->cali_sw[KXTJ2_1009_AXIS_Z],
		     tmp[KXTJ2_1009_AXIS_X], tmp[KXTJ2_1009_AXIS_Y], tmp[KXTJ2_1009_AXIS_Z]);

	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = kxtj2_1009_i2c_client;
	int err, x, y, z;
	int dat[KXTJ2_1009_AXES_NUM];

	if (!strncmp(buf, "rst", 3)) {
		err = KXTJ2_1009_ResetCalibration(client);
		if (0 != err)
			GSE_ERR("reset offset err = %d\n", err);
	} else if (3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z)) {
		dat[KXTJ2_1009_AXIS_X] = x;
		dat[KXTJ2_1009_AXIS_Y] = y;
		dat[KXTJ2_1009_AXIS_Z] = z;

		err = KXTJ2_1009_WriteCalibration(client, dat);
		if (0 != err)
			GSE_ERR("write calibration err = %d\n", err);
	} else {
		GSE_ERR("invalid format\n");
	}

	return count;
}
#endif

/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_KXTJ2_1009_LOWPASS
	struct i2c_client *client = kxtj2_1009_i2c_client;
	struct kxtj2_1009_i2c_data *obj = i2c_get_clientdata(client);

	if (atomic_read(&obj->firlen)) {
		int idx, len = atomic_read(&obj->firlen);

		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for (idx = 0; idx < len; idx++) {
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][KXTJ2_1009_AXIS_X],
				obj->fir.raw[idx][KXTJ2_1009_AXIS_Y], obj->fir.raw[idx][KXTJ2_1009_AXIS_Z]);
		}

		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[KXTJ2_1009_AXIS_X],
			obj->fir.sum[KXTJ2_1009_AXIS_Y], obj->fir.sum[KXTJ2_1009_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[KXTJ2_1009_AXIS_X] / len,
			obj->fir.sum[KXTJ2_1009_AXIS_Y] / len, obj->fir.sum[KXTJ2_1009_AXIS_Z] / len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}

/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, const char *buf, size_t count)
{
#ifdef CONFIG_KXTJ2_1009_LOWPASS
	struct i2c_client *client = kxtj2_1009_i2c_client;
	struct kxtj2_1009_i2c_data *obj = i2c_get_clientdata(client);
	int firlen;

	if (!kstrtoint(buf, 10, &firlen)) {
		GSE_ERR("invallid format\n");
	} else if (firlen > C_MAX_FIR_LENGTH) {
		GSE_ERR("exceeds maximum filter length\n");
	} else {
		atomic_set(&obj->firlen, firlen);
		if (NULL == firlen) {
			atomic_set(&obj->fir_en, 0);
		} else {
			memset(&obj->fir, 0x00, sizeof(obj->fir));
			atomic_set(&obj->fir_en, 1);
		}
	}
#endif
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct kxtj2_1009_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct kxtj2_1009_i2c_data *obj = obj_i2c_data;
	int trace;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (!kstrtoint(buf, 16, &trace))
		atomic_set(&obj->trace, trace);
	else
		GSE_ERR("invalid content: '%s', length = %d\n", buf, (int)count);

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct kxtj2_1009_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (obj->hw) {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: %d %d (%d %d)\n",
				obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id,
				obj->hw->power_vol);
	} else {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");
	}
	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_power_status_value(struct device_driver *ddri, char *buf)
{

	u8 databuf[2];
	/* int res = 0; */
	u8 addr = KXTJ2_1009_REG_POWER_CTL;
	struct kxtj2_1009_i2c_data *obj = obj_i2c_data;

	if (hwmsen_read_block(obj->client, addr, databuf, 0x01)) {
		GSE_ERR("read power ctl register err!\n");
		return 1;
	}

	if (sensor_power)
		GSE_LOG("G sensor is in work mode, sensor_power = %d\n", sensor_power);
	else
		GSE_LOG("G sensor is in standby mode, sensor_power = %d\n", sensor_power);

	return snprintf(buf, PAGE_SIZE, "%x\n", databuf[0]);
}

static ssize_t show_chip_orientation(struct device_driver *ddri, char *pbBuf)
{
	ssize_t _tLength = 0;

	GSE_LOG("[%s] default direction: %d\n", __func__, hw->direction);

	_tLength = snprintf(pbBuf, PAGE_SIZE, "default direction = %d\n", hw->direction);

	return _tLength;
}


static ssize_t store_chip_orientation(struct device_driver *ddri, const char *pbBuf, size_t tCount)
{
	int _nDirection = 0;
	struct kxtj2_1009_i2c_data *_pt_i2c_obj = obj_i2c_data;

	if (NULL == _pt_i2c_obj)
		return 0;

	if (!kstrtoint(pbBuf, 10, &_nDirection)) {
		if (hwmsen_get_convert(_nDirection, &_pt_i2c_obj->cvt))
			GSE_ERR("ERR: fail to set direction\n");
	}

	GSE_LOG("[%s] set direction: %d\n", __func__, _nDirection);

	return tCount;
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo, S_IWUSR | S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(cali, S_IWUSR | S_IRUGO, show_cali_value, store_cali_value);
static DRIVER_ATTR(firlen, S_IWUSR | S_IRUGO, show_firlen_value, store_firlen_value);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, show_trace_value, store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(powerstatus, S_IRUGO, show_power_status_value, NULL);
static DRIVER_ATTR(orientation, S_IWUSR | S_IRUGO, show_chip_orientation, store_chip_orientation);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *kxtj2_1009_attr_list[] = {
	&driver_attr_chipinfo,	/*chip information */
	&driver_attr_sensordata,	/*dump sensor data */
	&driver_attr_cali,	/*show calibration data */
	&driver_attr_firlen,	/*filter length: 0: disable, others: enable */
	&driver_attr_trace,	/*trace log */
	&driver_attr_status,
	&driver_attr_powerstatus,
	&driver_attr_orientation,
};

/*----------------------------------------------------------------------------*/
static int kxtj2_1009_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(kxtj2_1009_attr_list) / sizeof(kxtj2_1009_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, kxtj2_1009_attr_list[idx]);
		if (0 != err) {
			GSE_ERR("driver_create_file (%s) = %d\n", kxtj2_1009_attr_list[idx]->attr.name,
				err);
			break;
		}
	}
	return err;
}

/*----------------------------------------------------------------------------*/
static int kxtj2_1009_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(kxtj2_1009_attr_list) / sizeof(kxtj2_1009_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;


	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, kxtj2_1009_attr_list[idx]);


	return err;
}

/*----------------------------------------------------------------------------*/
#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
static void gsensor_irq_work(struct work_struct *work)
{
	struct kxtj2_1009_i2c_data *obj = obj_i2c_data;
	struct scp_acc_hw scp_hw;
	union KXTJ2_1009_CUST_DATA *p_cust_data;
	SCP_SENSOR_HUB_DATA data;
	int max_cust_data_size_per_packet;
	int i;
	uint sizeOfCustData;
	uint len;
	char *p = (char *)&scp_hw;

	GSE_FUN();

	scp_hw.i2c_num = obj->hw->i2c_num;
	scp_hw.direction = obj->hw->direction;
	scp_hw.power_id = obj->hw->power_id;
	scp_hw.power_vol = obj->hw->power_vol;
	scp_hw.firlen = obj->hw->firlen;
	memcpy(scp_hw.i2c_addr, obj->hw->i2c_addr, sizeof(obj->hw->i2c_addr));
	scp_hw.power_vio_id = obj->hw->power_vio_id;
	scp_hw.power_vio_vol = obj->hw->power_vio_vol;
	scp_hw.is_batch_supported = obj->hw->is_batch_supported;

	p_cust_data = (union KXTJ2_1009_CUST_DATA *) data.set_cust_req.custData;
	sizeOfCustData = sizeof(scp_hw);
	max_cust_data_size_per_packet =
	    sizeof(data.set_cust_req.custData) - offsetof(struct KXTJ2_1009_SET_CUST, data);

	GSE_ERR("sizeOfCustData = %d, max_cust_data_size_per_packet = %d\n", sizeOfCustData,
		max_cust_data_size_per_packet);
	GSE_ERR("offset %d\n", offsetof(struct KXTJ2_1009_SET_CUST, data));

	for (i = 0; sizeOfCustData > 0; i++) {
		data.set_cust_req.sensorType = ID_ACCELEROMETER;
		data.set_cust_req.action = SENSOR_HUB_SET_CUST;
		p_cust_data->setCust.action = KXTJ2_1009_CUST_ACTION_SET_CUST;
		p_cust_data->setCust.part = i;
		if (sizeOfCustData > max_cust_data_size_per_packet)
			len = max_cust_data_size_per_packet;
		else
			len = sizeOfCustData;

		memcpy(p_cust_data->setCust.data, p, len);
		sizeOfCustData -= len;
		p += len;

		GSE_ERR("i= %d, sizeOfCustData = %d, len = %d\n", i, sizeOfCustData, len);
		len +=
		    offsetof(SCP_SENSOR_HUB_SET_CUST_REQ, custData) + offsetof(struct KXTJ2_1009_SET_CUST,
									       data);
		GSE_ERR("data.set_cust_req.sensorType= %d\n", data.set_cust_req.sensorType);
		SCP_sensorHub_req_send(&data, &len, 1);

	}
	p_cust_data = (union KXTJ2_1009_CUST_DATA *) &data.set_cust_req.custData;
	data.set_cust_req.sensorType = ID_ACCELEROMETER;
	data.set_cust_req.action = SENSOR_HUB_SET_CUST;
	p_cust_data->resetCali.action = KXTJ2_1009_CUST_ACTION_RESET_CALI;
	len = offsetof(SCP_SENSOR_HUB_SET_CUST_REQ, custData) + sizeof(p_cust_data->resetCali);
	SCP_sensorHub_req_send(&data, &len, 1);
	obj->SCP_init_done = 1;
}

/*----------------------------------------------------------------------------*/
static int gsensor_irq_handler(void *data, uint len)
{
	struct kxtj2_1009_i2c_data *obj = obj_i2c_data;
	SCP_SENSOR_HUB_DATA_P rsp = (SCP_SENSOR_HUB_DATA_P) data;

	GSE_ERR("gsensor_irq_handler len = %d, type = %d, action = %d, errCode = %d\n", len,
		rsp->rsp.sensorType, rsp->rsp.action, rsp->rsp.errCode);
	if (!obj)
		return -1;

	switch (rsp->rsp.action) {
	case SENSOR_HUB_NOTIFY:
		switch (rsp->notify_rsp.event) {
		case SCP_INIT_DONE:
			schedule_work(&obj->irq_work);
			GSE_ERR("OK sensor hub notify\n");
			break;
		default:
			GSE_ERR("Error sensor hub notify\n");
			break;
		}
		break;
	default:
		GSE_ERR("Error sensor hub action\n");
		break;
	}

	return 0;
}

static int gsensor_setup_irq(void)
{
	int err = 0;



	err = SCP_sensorHub_rsp_registration(ID_ACCELEROMETER, gsensor_irq_handler);

	return err;
}
#endif				/* #ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB */
/******************************************************************************
 * Function Configuration
******************************************************************************/
static int kxtj2_1009_open(struct inode *inode, struct file *file)
{
	file->private_data = kxtj2_1009_i2c_client;

	if (file->private_data == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

/*----------------------------------------------------------------------------*/
static int kxtj2_1009_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

/*----------------------------------------------------------------------------*/
/* static int kxtj2_1009_ioctl(struct inode *inode, struct file *file, unsigned int cmd, */
/* unsigned long arg) */
static long kxtj2_1009_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct kxtj2_1009_i2c_data *obj = (struct kxtj2_1009_i2c_data *)i2c_get_clientdata(client);
	char strbuf[KXTJ2_1009_BUFSIZE];
	void __user *data;
	struct SENSOR_DATA sensor_data;
	long err = 0;
	int cali[3];

	/* GSE_FUN(f); */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) {
		GSE_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case GSENSOR_IOCTL_INIT:
		kxtj2_1009_init_client(client, 0);
		break;

	case GSENSOR_IOCTL_READ_CHIPINFO:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		KXTJ2_1009_ReadChipInfo(client, strbuf, KXTJ2_1009_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_SENSORDATA:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		printk("=======zhoucheng612===GSENSOR_IOCTL_READ_SENSORDATA\n");
		KXTJ2_1009_SetPowerMode(client, true);
		KXTJ2_1009_ReadSensorData(client, strbuf, KXTJ2_1009_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_GAIN:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		if (copy_to_user(data, &gsensor_gain, sizeof(struct GSENSOR_VECTOR3D))) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_RAW_DATA:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		printk("=======zhoucheng612===GSENSOR_IOCTL_READ_RAW_DATA\n");
		KXTJ2_1009_ReadRawData(client, strbuf);
		if (copy_to_user(data, &strbuf, strlen(strbuf) + 1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_SET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_from_user(&sensor_data, data, sizeof(sensor_data))) {
			err = -EFAULT;
			break;
		}
		if (atomic_read(&obj->suspend)) {
			GSE_ERR("Perform calibration in suspend state!!\n");
			err = -EINVAL;
		} else {
			cali[KXTJ2_1009_AXIS_X] =
			    sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			cali[KXTJ2_1009_AXIS_Y] =
			    sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			cali[KXTJ2_1009_AXIS_Z] =
			    sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			err = KXTJ2_1009_WriteCalibration(client, cali);
		}
		break;

	case GSENSOR_IOCTL_CLR_CALI:
		err = KXTJ2_1009_ResetCalibration(client);
		break;

	case GSENSOR_IOCTL_GET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		err = KXTJ2_1009_ReadCalibration(client, cali);
		if (0 != err)
			break;

		sensor_data.x = cali[KXTJ2_1009_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		sensor_data.y = cali[KXTJ2_1009_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		sensor_data.z = cali[KXTJ2_1009_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		if (copy_to_user(data, &sensor_data, sizeof(sensor_data))) {
			err = -EFAULT;
			break;
		}
		break;


	default:
		GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
		err = -ENOIOCTLCMD;
		break;

	}

	return err;
}


#ifdef CONFIG_COMPAT
static long kxtj2_1009_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err = 0;

	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_GSENSOR_IOCTL_READ_SENSORDATA:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_READ_SENSORDATA,
					       (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_READ_SENSORDATA unlocked_ioctl failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_SET_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_SET_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_SET_CALI unlocked_ioctl failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_GET_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_GET_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_GET_CALI unlocked_ioctl failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_CLR_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}

		err =
		    file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_CLR_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_CLR_CALI unlocked_ioctl failed.");
			return err;
		}
		break;

	default:
		GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
		err = -ENOIOCTLCMD;
		break;

	}

	return err;
}
#endif
/*----------------------------------------------------------------------------*/
static const struct file_operations kxtj2_1009_fops = {
	.owner = THIS_MODULE,
	.open = kxtj2_1009_open,
	.release = kxtj2_1009_release,
	.unlocked_ioctl = kxtj2_1009_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = kxtj2_1009_compat_ioctl,
#endif
};

/*----------------------------------------------------------------------------*/
static struct miscdevice kxtj2_1009_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &kxtj2_1009_fops,
};

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static int kxtj2_1009_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct kxtj2_1009_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;

	mutex_lock(&gsensor_scp_en_mutex);
	if (msg.event == PM_EVENT_SUSPEND) {
		if (obj == NULL) {
			GSE_ERR("null pointer!!\n");
			mutex_unlock(&gsensor_scp_en_mutex);
			return -EINVAL;
		}
		atomic_set(&obj->suspend, 1);
#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
		err = KXTJ2_1009_SCP_SetPowerMode(false, ID_ACCELEROMETER);
#else
		err = KXTJ2_1009_SetPowerMode(obj->client, false);
#endif
		if (err != 0) {
			GSE_ERR("write power control fail!!\n");
			mutex_unlock(&gsensor_scp_en_mutex);
			return -EINVAL;
		}
#ifndef CONFIG_CUSTOM_KERNEL_SENSORHUB
		KXTJ2_1009_power(obj->hw, 0);
#endif
	}
	mutex_unlock(&gsensor_scp_en_mutex);
	return err;
}

/*----------------------------------------------------------------------------*/
static int kxtj2_1009_resume(struct i2c_client *client)
{
	struct kxtj2_1009_i2c_data *obj = i2c_get_clientdata(client);
	int err;

	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
#ifndef CONFIG_CUSTOM_KERNEL_SENSORHUB
	KXTJ2_1009_power(obj->hw, 1);
#endif

#ifndef CONFIG_CUSTOM_KERNEL_SENSORHUB
	err = kxtj2_1009_init_client(client, 0);
#else
	err = KXTJ2_1009_SCP_SetPowerMode(enable_status, ID_ACCELEROMETER);
#endif
	if (err != 0) {
		GSE_ERR("initialize client fail!!\n");
		return err;
	}
	atomic_set(&obj->suspend, 0);

	return 0;
}

/*----------------------------------------------------------------------------*/
/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int gsensor_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/*----------------------------------------------------------------------------*/
/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */
static int gsensor_enable_nodata(int en)
{
	int err = 0;



	if (((en == 0) && (sensor_power == false)) || ((en == 1) && (sensor_power == true))) {
		enable_status = sensor_power;
		GSE_LOG("Gsensor device have updated!\n");
	} else {
		enable_status = !sensor_power;
		if (atomic_read(&obj_i2c_data->suspend) == 0) {
#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
			err = KXTJ2_1009_SCP_SetPowerMode(enable_status, ID_ACCELEROMETER);
			if (0 == err)
				sensor_power = enable_status;
#else
			err = KXTJ2_1009_SetPowerMode(obj_i2c_data->client, enable_status);
#endif
			GSE_LOG("Gsensor not in suspend KXTJ2_1009_SetPowerMode!, enable_status = %d\n",
				enable_status);
		} else {
			GSE_LOG
			    ("Gsensor in suspend and can not enable or disable!enable_status = %d\n",
			     enable_status);
		}
	}


	if (err != KXTJ2_1009_SUCCESS) {
		GSE_ERR("gsensor_enable_nodata fail!\n");
		return -1;
	}

	GSE_ERR("gsensor_enable_nodata OK!\n");
	return 0;
}

/*----------------------------------------------------------------------------*/
static int gsensor_set_delay(u64 ns)
{
	int err = 0;
	int value;
#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#else				/* #ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB */
	int sample_delay;
#endif				/* #ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB */

	value = (int)ns / 1000 / 1000;

#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
	req.set_delay_req.sensorType = ID_ACCELEROMETER;
	req.set_delay_req.action = SENSOR_HUB_SET_DELAY;
	req.set_delay_req.delay = value;
	len = sizeof(req.activate_req);
	err = SCP_sensorHub_req_send(&req, &len, 1);
	if (err) {
		GSE_ERR("SCP_sensorHub_req_send!\n");
		return err;
	}
#else				/* #ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB */
	if (value <= 5)
		sample_delay = KXTJ2_1009_BW_200HZ;
	else if (value <= 10)
		sample_delay = KXTJ2_1009_BW_100HZ;
	else
		sample_delay = KXTJ2_1009_BW_100HZ;

	mutex_lock(&gsensor_scp_en_mutex);
	err = KXTJ2_1009_SetBWRate(obj_i2c_data->client, sample_delay);
	mutex_unlock(&gsensor_scp_en_mutex);
	if (err != KXTJ2_1009_SUCCESS) {
		GSE_ERR("Set delay parameter error!\n");
		return -1;
	}

	if (value >= 50) {
		atomic_set(&obj_i2c_data->filter, 0);
	} else {
#if defined(CONFIG_KXTJ2_1009_LOWPASS)
		priv->fir.num = 0;
		priv->fir.idx = 0;
		priv->fir.sum[KXTJ2_1009_AXIS_X] = 0;
		priv->fir.sum[KXTJ2_1009_AXIS_Y] = 0;
		priv->fir.sum[KXTJ2_1009_AXIS_Z] = 0;
		atomic_set(&priv->filter, 1);
#endif
	}
#endif				/* #ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB */

	GSE_LOG("gsensor_set_delay (%d)\n", value);

	return 0;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static int gsensor_get_data(int *x, int *y, int *z, int *status)
{
#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
	int err = 0;
#else
	char buff[KXTJ2_1009_BUFSIZE];
	int ret;
#endif

#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
	req.get_data_req.sensorType = ID_ACCELEROMETER;
	req.get_data_req.action = SENSOR_HUB_GET_DATA;
	len = sizeof(req.get_data_req);
	err = SCP_sensorHub_req_send(&req, &len, 1);
	if (err) {
		GSE_ERR("SCP_sensorHub_req_send!\n");
		return err;
	}

	if (ID_ACCELEROMETER != req.get_data_rsp.sensorType ||
	    SENSOR_HUB_GET_DATA != req.get_data_rsp.action || 0 != req.get_data_rsp.errCode) {
		GSE_ERR("error : %d\n", req.get_data_rsp.errCode);
		return req.get_data_rsp.errCode;
	}

	*x = (int)req.get_data_rsp.int16_Data[0] * GRAVITY_EARTH_1000 / 1000;
	*y = (int)req.get_data_rsp.int16_Data[1] * GRAVITY_EARTH_1000 / 1000;
	*z = (int)req.get_data_rsp.int16_Data[2] * GRAVITY_EARTH_1000 / 1000;
	GSE_ERR("x = %d, y = %d, z = %d\n", *x, *y, *z);

	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
#else
	mutex_lock(&gsensor_scp_en_mutex);
	KXTJ2_1009_ReadSensorData(obj_i2c_data->client, buff, KXTJ2_1009_BUFSIZE);
	mutex_unlock(&gsensor_scp_en_mutex);
	ret = sscanf(buff, "%x %x %x", x, y, z);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
#endif
	return 0;
}

/*----------------------------------------------------------------------------*/
static int kxtj2_1009_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct kxtj2_1009_i2c_data *obj;
	struct acc_control_path ctl = { 0 };
	struct acc_data_path data = { 0 };
	int err = 0;
	int retry = 0;

	GSE_FUN();

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(struct kxtj2_1009_i2c_data));

	obj->hw = hw;

	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if (0 != err) {
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}
#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
	INIT_WORK(&obj->irq_work, gsensor_irq_work);
#endif				/* #ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB */

	obj_i2c_data = obj;
	obj->client = client;
#ifdef FPGA_EARLY_PORTING
	obj->client->timing = 100;
#else
	obj->client->timing = 400;
#endif
	new_client = obj->client;
	i2c_set_clientdata(new_client, obj);

	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);

#ifdef CONFIG_KXTJ2_1009_LOWPASS
	if (obj->hw->firlen > C_MAX_FIR_LENGTH)
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	else
		atomic_set(&obj->firlen, obj->hw->firlen);

	if (atomic_read(&obj->firlen) > 0)
		atomic_set(&obj->fir_en, 1);
#endif

	kxtj2_1009_i2c_client = new_client;

	for (retry = 0; retry < 3; retry++) {
		
		err = kxtj2_1009_init_client(new_client, 1);
		if (0 != err) {
			GSE_ERR("kxtj2_1009_device init cilent fail time: %d\n", retry);
			continue;
		}
	}
	if (err != 0)
		goto exit_init_failed;


	err = misc_register(&kxtj2_1009_device);
	if (0 != err) {
		GSE_ERR("kxtj2_1009_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	err = kxtj2_1009_create_attr(&kxtj2_1009_init_info.platform_diver_addr->driver);
	if (0 != err) {
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	ctl.open_report_data = gsensor_open_report_data;
	ctl.enable_nodata = gsensor_enable_nodata;
	ctl.set_delay = gsensor_set_delay;
	/* ctl.batch = gsensor_set_batch; */
	ctl.is_report_input_direct = false;

#ifdef CONFIG_CUSTOM_KERNEL_SENSORHUB
	ctl.is_support_batch = obj->hw->is_batch_supported;
#else
	ctl.is_support_batch = false;
#endif

	err = acc_register_control_path(&ctl);
	if (err) {
		GSE_ERR("register acc control path err\n");
		goto exit_kfree;
	}

	data.get_data = gsensor_get_data;
	data.vender_div = 1000;
	err = acc_register_data_path(&data);
	if (err) {
		GSE_ERR("register acc data path err\n");
		goto exit_kfree;
	}

	err = batch_register_support_info(ID_ACCELEROMETER,
					  ctl.is_support_batch, 102, 0);
	if (err) {
		GSE_ERR("register gsensor batch support err = %d\n", err);
		goto exit_create_attr_failed;
	}

	gsensor_init_flag = 0;
	GSE_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
	misc_deregister(&kxtj2_1009_device);
exit_misc_device_register_failed:
exit_init_failed:
	/* i2c_detach_client(new_client); */
exit_kfree:
	kfree(obj);
exit:
	GSE_ERR("%s: err = %d\n", __func__, err);
	gsensor_init_flag = -1;
	return err;
}

/*----------------------------------------------------------------------------*/
static int kxtj2_1009_i2c_remove(struct i2c_client *client)
{
	int err = 0;

	err = kxtj2_1009_delete_attr(&kxtj2_1009_init_info.platform_diver_addr->driver);
	if (err != 0)
		GSE_ERR("bma150_delete_attr fail: %d\n", err);

	err = misc_deregister(&kxtj2_1009_device);
	if (0 != err)
		GSE_ERR("misc_deregister fail: %d\n", err);

	kxtj2_1009_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}

/*----------------------------------------------------------------------------*/
static int gsensor_local_init(void)
{
	GSE_FUN();

	KXTJ2_1009_power(hw, 1);
	if (i2c_add_driver(&kxtj2_1009_i2c_driver)) {
		GSE_ERR("add driver error\n");
		return -1;
	}
	if (-1 == gsensor_init_flag)
		return -1;
	/* GSE_LOG("fwq loccal init---\n"); */
	return 0;
}

/*----------------------------------------------------------------------------*/
static int gsensor_remove(void)
{
	GSE_FUN();
	KXTJ2_1009_power(hw, 0);
	i2c_del_driver(&kxtj2_1009_i2c_driver);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int __init kxtj2_1009_init(void)
{
	GSE_FUN();
	hw = get_accel_dts_func(COMPATIABLE_NAME, hw);
	if (!hw)
		GSE_ERR("get dts info fail\n");
	GSE_LOG("%s: i2c_number=%d\n", __func__, hw->i2c_num);
	acc_driver_add(&kxtj2_1009_init_info);
	return 0;
}

/*----------------------------------------------------------------------------*/
static void __exit kxtj2_1009_exit(void)
{
	GSE_FUN();
}

/*----------------------------------------------------------------------------*/
module_init(kxtj2_1009_init);
module_exit(kxtj2_1009_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("KXTJ2_1009 I2C driver");
MODULE_AUTHOR("Xiaoli.li@mediatek.com");
