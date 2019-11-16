/* For MTK android platform.
 *
 * mir3da.c - Linux kernel modules for 3-Axis Accelerometer
 *
 * Copyright (C) 2011-2013 MiraMEMS Sensing Technology Co., Ltd.
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

#include <cust_acc.h>
#include "mir3da_core.h"
#include "mir3da_cust.h"

#if MTK_ANDROID_M
#include <accel.h>
#else
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/sbsuspend.h>
#endif

#if  MIR3DA_SUPPORT_FAST_AUTO_CALI
#include <mach/mt_boot.h>
#endif

#if defined(MT6516)
#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>
#elif defined(MT6573)
#include <mach/mt6573_devs.h>
#include <mach/mt6573_typedefs.h>
#include <mach/mt6573_gpio.h>
#include <mach/mt6573_pll.h>
#elif defined(MT6575)
#include <mach/mt6575_devs.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_gpio.h>
#include <mach/mt6575_pm_ldo.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_boot.h>
#endif

#if defined(MT6516)
#define POWER_NONE_MACRO MT6516_POWER_NONE
#else
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif
#endif

#define MIR3DA_DRV_NAME                 	"mir3da"
#define MIR3DA_MISC_NAME                	"gsensor"
#define MIR3DA_PLATFORM_NAME         "gsensor"


#define MIR3DA_AXIS_X          			0
#define MIR3DA_AXIS_Y         		 	1
#define MIR3DA_AXIS_Z          			2
#define MIR3DA_AXES_NUM        		3

#define MTK_AUTO_MODE           		1 

struct scale_factor{
    u8  whole;
    u8  fraction;
};

struct data_resolution {
    struct scale_factor scalefactor;
    int                 sensitivity;
};

struct mir3da_i2c_data {
    struct i2c_client 		*client;
    struct acc_hw 			*hw;
    struct hwmsen_convert   	cvt;
    
    struct data_resolution 	*reso;
    atomic_t                		trace;
    atomic_t                		suspend;
    atomic_t                		selftest;
    s16                     		cali_sw[MIR3DA_AXES_NUM+1];

    s8                      		offset[MIR3DA_AXES_NUM+1]; 
    s16                     		data[MIR3DA_AXES_NUM+1];

//#if defined(CONFIG_HAS_EARLYSUSPEND)
#if 0
    struct early_suspend    	early_drv;
#endif     
};

static struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;
 
static struct data_resolution mir3da_data_resolution[] = {
    {{ 1, 0}, 1024},
};

#if !MTK_ANDROID_M
static struct i2c_board_info      mir3da_i2c_boardinfo = { I2C_BOARD_INFO(MIR3DA_DRV_NAME, MIR3DA_I2C_ADDR>>1) };
#endif

static bool sensor_power = false;
static struct GSENSOR_VECTOR3D gsensor_gain, gsensor_offset;
static MIR_HANDLE              mir_handle;
static int mir3da_init_flag =0;

extern int Log_level;
/*----------------------------------------------------------------------------*/
#define MI_DATA(format, ...)            if(DEBUG_DATA&Log_level){printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_MSG(format, ...)             if(DEBUG_MSG&Log_level){printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_ERR(format, ...)             if(DEBUG_ERR&Log_level){printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_FUN                          if(DEBUG_FUNC&Log_level){printk(KERN_ERR MI_TAG "%s is called, line: %d\n", __FUNCTION__,__LINE__);}
#define MI_ASSERT(expr)                 \
	if (!(expr)) {\
		printk(KERN_ERR "Assertion failed! %s,%d,%s,%s\n",\
			__FILE__, __LINE__, __func__, #expr);\
	}
/*----------------------------------------------------------------------------*/
#if MTK_AUTO_MODE
static int mir3da_local_init(void);
static int mir3da_local_remove(void);
#if MTK_ANDROID_M
static struct acc_init_info mir3da_init_info = {
#else
static struct sensor_init_info mir3da_init_info = {
#endif
    .name = MIR3DA_DRV_NAME,
    .init = mir3da_local_init,
    .uninit = mir3da_local_remove,
};
extern struct acc_hw *mir3da_get_cust_acc_hw(void);
#else
static int mir3da_platform_probe(struct platform_device *pdev); 
static int mir3da_platform_remove(struct platform_device *pdev);
static struct platform_driver mir3da_gsensor_driver = {
    .driver     = {
        .name  = MIR3DA_PLATFORM_NAME,
        .owner = THIS_MODULE,
    },	
    .probe      = mir3da_platform_probe,
    .remove     = mir3da_platform_remove,    
};
#endif
/*----------------------------------------------------------------------------*/
#if MIR3DA_OFFSET_TEMP_SOLUTION
static char OffsetFileName[] = "/data/misc/miraGSensorOffset.txt";
static char OffsetFolerName[] = "/data/misc/";
//static int bCaliResult = -1; 
#define OFFSET_STRING_LEN               26
struct work_info
{
    char        tst1[20];
    char        tst2[20];
    char        buffer[OFFSET_STRING_LEN];
    struct      workqueue_struct *wq;
    struct      delayed_work read_work;
    struct      delayed_work write_work;
    struct      completion completion;
    int         len;
    int         rst; 
};

static struct work_info m_work_info = {{0}};
/*----------------------------------------------------------------------------*/
static void sensor_write_work( struct work_struct *work )
{
    struct work_info*   pWorkInfo;
    struct file         *filep;
    u32                 orgfs;
    int                 ret;   

    orgfs = get_fs();
    set_fs(KERNEL_DS);

    pWorkInfo = container_of((struct delayed_work*)work, struct work_info, write_work);
    if (pWorkInfo == NULL){            
            MI_ERR("get pWorkInfo failed!");       
            return;
    }
    
    filep = filp_open(OffsetFileName, O_RDWR|O_CREAT, 0600);
    if (IS_ERR(filep)){
        MI_ERR("write, sys_open %s error!!.\n", OffsetFileName);
        ret =  -1;
    }
    else
    {   
        filep->f_op->write(filep, pWorkInfo->buffer, pWorkInfo->len, &filep->f_pos);
        filp_close(filep, NULL);
        ret = 0;        
    }
    
    set_fs(orgfs);   
    pWorkInfo->rst = ret;
    complete( &pWorkInfo->completion );
}
/*----------------------------------------------------------------------------*/
static void sensor_read_work( struct work_struct *work )
{
    u32 orgfs;
    struct file *filep;
    int ret; 
    struct work_info* pWorkInfo;
        
    orgfs = get_fs();
    set_fs(KERNEL_DS);
    
    pWorkInfo = container_of((struct delayed_work*)work, struct work_info, read_work);
    if (pWorkInfo == NULL){            
        MI_ERR("get pWorkInfo failed!");       
        return;
    }
 
    filep = filp_open(OffsetFileName, O_RDONLY, 0600);
    if (IS_ERR(filep)){
        MI_ERR("read, sys_open %s error!!.\n",OffsetFileName);
        set_fs(orgfs);
        ret =  -1;
    }
    else{
        filep->f_op->read(filep, pWorkInfo->buffer,  sizeof(pWorkInfo->buffer), &filep->f_pos);
        filp_close(filep, NULL);    
        set_fs(orgfs);
        ret = 0;
    }

    pWorkInfo->rst = ret;
    complete( &(pWorkInfo->completion) );
}
/*----------------------------------------------------------------------------*/
static int sensor_sync_read(u8* offset)
{
    int     err;
    int     off[MIR3DA_OFFSET_LEN] = {0};
    struct work_info* pWorkInfo = &m_work_info;
     
    init_completion( &pWorkInfo->completion );
    queue_delayed_work( pWorkInfo->wq, &pWorkInfo->read_work, msecs_to_jiffies(0) );
    err = wait_for_completion_timeout( &pWorkInfo->completion, msecs_to_jiffies( 2000 ) );
    if ( err == 0 ){
        MI_ERR("wait_for_completion_timeout TIMEOUT");
        return -1;
    }

    if (pWorkInfo->rst != 0){
        MI_ERR("work_info.rst  not equal 0");
        return pWorkInfo->rst;
    }
    
    sscanf(m_work_info.buffer, "%x,%x,%x,%x,%x,%x,%x,%x,%x", &off[0], &off[1], &off[2], &off[3], &off[4], &off[5],&off[6], &off[7], &off[8]);

    offset[0] = (u8)off[0];
    offset[1] = (u8)off[1];
    offset[2] = (u8)off[2];
    offset[3] = (u8)off[3];
    offset[4] = (u8)off[4];
    offset[5] = (u8)off[5];
    offset[6] = (u8)off[6];
    offset[7] = (u8)off[7];
    offset[8] = (u8)off[8];
    
    return 0;
}
/*----------------------------------------------------------------------------*/
static int sensor_sync_write(u8* off)
{
    int err = 0;
    struct work_info* pWorkInfo = &m_work_info;
       
    init_completion( &pWorkInfo->completion );
    
    sprintf(m_work_info.buffer, "%x,%x,%x,%x,%x,%x,%x,%x,%x\n", off[0],off[1],off[2],off[3],off[4],off[5],off[6],off[7],off[8]);
    
    pWorkInfo->len = sizeof(m_work_info.buffer);
        
    queue_delayed_work( pWorkInfo->wq, &pWorkInfo->write_work, msecs_to_jiffies(0) );
    err = wait_for_completion_timeout( &pWorkInfo->completion, msecs_to_jiffies( 2000 ) );
    if ( err == 0 ){
        MI_ERR("wait_for_completion_timeout TIMEOUT");
        return -1;
    }

    if (pWorkInfo->rst != 0){
        MI_ERR("work_info.rst  not equal 0");
        return pWorkInfo->rst;
    }
    
    return 0;
}
/*----------------------------------------------------------------------------*/
static int check_califolder_exist(void)
{
    u32     orgfs = 0;
    struct  file *filep;
        
    orgfs = get_fs();
    set_fs(KERNEL_DS);

    filep = filp_open(OffsetFolerName, O_RDONLY, 0600);
    if (IS_ERR(filep)) {
        MI_ERR("%s read, sys_open %s error!!.\n",__func__,OffsetFolerName);
        set_fs(orgfs);
        return 0;
    }

    filp_close(filep, NULL);    
    set_fs(orgfs); 

    return 1;
}
/*----------------------------------------------------------------------------*/
static int support_fast_auto_cali(void)
{
#if MIR3DA_SUPPORT_FAST_AUTO_CALI
    return (FACTORY_BOOT == get_boot_mode());
#else
    return 0;
#endif
}
#endif
/*----------------------------------------------------------------------------*/
static int get_address(PLAT_HANDLE handle)
{
    if(NULL == handle){
        MI_ERR("chip init failed !\n");
		    return -1;
    }
			
	return ((struct i2c_client *)handle)->addr; 		
}
/*----------------------------------------------------------------------------*/
static int mir3da_resetCalibration(struct i2c_client *client)
{
    struct mir3da_i2c_data *obj = i2c_get_clientdata(client);    

    MI_FUN;
  
    memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
    return 0;     
}
/*----------------------------------------------------------------------------*/
static int mir3da_readCalibration(struct i2c_client *client, int *dat)
{
    struct mir3da_i2c_data *obj = i2c_get_clientdata(client);

    MI_FUN;

    dat[obj->cvt.map[MIR3DA_AXIS_X]] = obj->cvt.sign[MIR3DA_AXIS_X]*obj->cali_sw[MIR3DA_AXIS_X];
    dat[obj->cvt.map[MIR3DA_AXIS_Y]] = obj->cvt.sign[MIR3DA_AXIS_Y]*obj->cali_sw[MIR3DA_AXIS_Y];
    dat[obj->cvt.map[MIR3DA_AXIS_Z]] = obj->cvt.sign[MIR3DA_AXIS_Z]*obj->cali_sw[MIR3DA_AXIS_Z];                        
                                       
    return 0;
}
/*----------------------------------------------------------------------------*/
static int mir3da_writeCalibration(struct i2c_client *client, int dat[MIR3DA_AXES_NUM])
{
    struct mir3da_i2c_data *obj = i2c_get_clientdata(client);
    int err = 0;
    int cali[MIR3DA_AXES_NUM];


    MI_FUN;
    if(!obj || ! dat)
    {
        MI_ERR("null ptr!!\n");
        return -EINVAL;
    }
    else
    {
        cali[obj->cvt.map[MIR3DA_AXIS_X]] = obj->cvt.sign[MIR3DA_AXIS_X]*obj->cali_sw[MIR3DA_AXIS_X];
        cali[obj->cvt.map[MIR3DA_AXIS_Y]] = obj->cvt.sign[MIR3DA_AXIS_Y]*obj->cali_sw[MIR3DA_AXIS_Y];
        cali[obj->cvt.map[MIR3DA_AXIS_Z]] = obj->cvt.sign[MIR3DA_AXIS_Z]*obj->cali_sw[MIR3DA_AXIS_Z]; 
        cali[MIR3DA_AXIS_X] += dat[MIR3DA_AXIS_X];
        cali[MIR3DA_AXIS_Y] += dat[MIR3DA_AXIS_Y];
        cali[MIR3DA_AXIS_Z] += dat[MIR3DA_AXIS_Z];

        obj->cali_sw[MIR3DA_AXIS_X] += obj->cvt.sign[MIR3DA_AXIS_X]*dat[obj->cvt.map[MIR3DA_AXIS_X]];
        obj->cali_sw[MIR3DA_AXIS_Y] += obj->cvt.sign[MIR3DA_AXIS_Y]*dat[obj->cvt.map[MIR3DA_AXIS_Y]];
        obj->cali_sw[MIR3DA_AXIS_Z] += obj->cvt.sign[MIR3DA_AXIS_Z]*dat[obj->cvt.map[MIR3DA_AXIS_Z]];
    } 

    return err;
}
/*----------------------------------------------------------------------------*/
static int mir3da_readChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
    if((NULL == buf)||(bufsize<=30)) {
        return -1;
    }

    if(NULL == client) {
        *buf = 0;
        return -2;
    }

    sprintf(buf, "%s\n", "mir3da");
		
    return 0;
}
/*----------------------------------------------------------------------------*/
static int mir3da_setPowerMode(struct i2c_client *client, bool enable)
{
    int ret;
    
    MI_MSG ("mir3da_setPowerMode(), enable = %d", enable);
    ret = mir3da_set_enable(client,enable);  
    if (ret == 0){
        sensor_power = enable;
    }
    return ret;
}
/*----------------------------------------------------------------------------*/
static int mir3da_readSensorData(struct i2c_client *client, char *buf)
{    
    struct mir3da_i2c_data *obj = (struct mir3da_i2c_data*)i2c_get_clientdata(client);
    unsigned char databuf[20];
    int acc[MIR3DA_AXES_NUM];
    int res = 0;
    memset(databuf, 0, sizeof(unsigned char)*10);

    if(NULL == buf)
    {
        return -1;
    }
    if(NULL == client)
    {
        *buf = 0;
        return -2;
    }

    if(sensor_power == false)
    {
        res = mir3da_setPowerMode(client, true);
        if(res)
        {
            MI_ERR("Power on mir3da error %d!\n", res);
        }
        msleep(20);
    }
    res = mir3da_read_data(client, &(obj->data[MIR3DA_AXIS_X]),&(obj->data[MIR3DA_AXIS_Y]),&(obj->data[MIR3DA_AXIS_Z]));
    if(res) 
    {        
        MI_ERR("I2C error: ret value=%d", res);
        return -3;
    }
    else
    {
	#if MIR3DA_OFFSET_TEMP_SOLUTION
        if( bLoad==FILE_NO_EXIST) 
	#endif    
        {
            obj->data[MIR3DA_AXIS_X] += obj->cali_sw[MIR3DA_AXIS_X];
            obj->data[MIR3DA_AXIS_Y] += obj->cali_sw[MIR3DA_AXIS_Y];
            obj->data[MIR3DA_AXIS_Z] += obj->cali_sw[MIR3DA_AXIS_Z];
        }
        
        acc[obj->cvt.map[MIR3DA_AXIS_X]] = obj->cvt.sign[MIR3DA_AXIS_X]*obj->data[MIR3DA_AXIS_X];
        acc[obj->cvt.map[MIR3DA_AXIS_Y]] = obj->cvt.sign[MIR3DA_AXIS_Y]*obj->data[MIR3DA_AXIS_Y];
        acc[obj->cvt.map[MIR3DA_AXIS_Z]] = obj->cvt.sign[MIR3DA_AXIS_Z]*obj->data[MIR3DA_AXIS_Z];
        
	      #if MIR3DA_OFFSET_TEMP_SOLUTION
        if( bLoad == FILE_NO_EXIST) 
	      #endif    
        {				
                 if(abs(obj->cali_sw[MIR3DA_AXIS_Z])> 1300)
                 acc[obj->cvt.map[MIR3DA_AXIS_Z]] = acc[obj->cvt.map[MIR3DA_AXIS_Z]] - 2048;
		    }
		            
#if MIR3DA_STK_TEMP_SOLUTION
      if(bzstk)
			   acc[MIR3DA_AXIS_Z] =squareRoot(1024*1024 - acc[MIR3DA_AXIS_X]*acc[MIR3DA_AXIS_X] - acc[MIR3DA_AXIS_Y]*acc[MIR3DA_AXIS_Y]); 	
#endif

        MI_DATA("mir3da data map: %d, %d, %d!\n", acc[MIR3DA_AXIS_X], acc[MIR3DA_AXIS_Y], acc[MIR3DA_AXIS_Z]);
        
        acc[MIR3DA_AXIS_X] = acc[MIR3DA_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        acc[MIR3DA_AXIS_Y] = acc[MIR3DA_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        acc[MIR3DA_AXIS_Z] = acc[MIR3DA_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;        

        sprintf(buf, "%04x %04x %04x", acc[MIR3DA_AXIS_X], acc[MIR3DA_AXIS_Y], acc[MIR3DA_AXIS_Z]);
        
        MI_DATA( "mir3da data mg: x= %d, y=%d, z=%d\n",  acc[MIR3DA_AXIS_X],acc[MIR3DA_AXIS_Y],acc[MIR3DA_AXIS_Z]); 
    }
    
    return 0;
}
/*----------------------------------------------------------------------------*/
static int mir3da_readRawData(struct i2c_client *client, char *buf)
{
	struct mir3da_i2c_data *obj = (struct mir3da_i2c_data*)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client) {
		return EINVAL;
	}
	
       res = mir3da_read_data(client, &(obj->data[MIR3DA_AXIS_X]),&(obj->data[MIR3DA_AXIS_Y]),&(obj->data[MIR3DA_AXIS_Z])); 
       if(res) {        
		MI_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else {
		sprintf(buf, "%04x %04x %04x", (obj->data[MIR3DA_AXIS_X]), (obj->data[MIR3DA_AXIS_Y]), (obj->data[MIR3DA_AXIS_Z]));
	
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static long mir3da_misc_ioctl( struct file *file,unsigned int cmd, unsigned long arg)
{
    struct i2c_client *client = mir_handle;
    struct mir3da_i2c_data *obj = (struct mir3da_i2c_data*)i2c_get_clientdata(client);    
    char strbuf[MIR3DA_BUFSIZE] = {0};
    void __user *data;
    struct SENSOR_DATA sensor_data;
    int err = 0;
    int cali[3]={0};
    
 printk("[mir3da_misc_ioctl] in!!!!! cmd===%d\n",cmd);
    if(_IOC_DIR(cmd) & _IOC_READ)
    {
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }
    else if(_IOC_DIR(cmd) & _IOC_WRITE)
    {
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }

    if(err)
    {
        MI_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
        return -EFAULT;
    }
    
    switch (cmd) {  
 
    case GSENSOR_IOCTL_INIT:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_INIT\n");

	 err = mir3da_chip_resume(client);
	 if(err) {
		MI_ERR("chip resume fail!!\n");
		return -EFAULT;
	 }        
        break;

    case GSENSOR_IOCTL_READ_CHIPINFO:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_READ_CHIPINFO\n");
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }

	mir3da_readChipInfo(client,strbuf,MIR3DA_BUFSIZE);
		
        if(copy_to_user(data, strbuf, strlen(strbuf)+1))
        {
            err = -EFAULT;
            break;
        }                 
        break;      

    case GSENSOR_IOCTL_READ_SENSORDATA:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_READ_SENSORDATA\n");
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }
        
        mir3da_readSensorData(client, strbuf);
        if(copy_to_user(data, strbuf, strlen(strbuf)+1))
        {
            err = -EFAULT;
            break;      
        }                 
        break;

    case GSENSOR_IOCTL_READ_GAIN:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_READ_GAIN\n");
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }            
        
        if(copy_to_user(data, &gsensor_gain, sizeof(struct GSENSOR_VECTOR3D)))
        {
            err = -EFAULT;
            break;
        }                 
        break;

    case GSENSOR_IOCTL_READ_OFFSET:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_READ_OFFSET\n");
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }
        
        if(copy_to_user(data, &gsensor_offset, sizeof(struct GSENSOR_VECTOR3D)))
        {
            err = -EFAULT;
            break;
        }                 
        break;

    case GSENSOR_IOCTL_READ_RAW_DATA:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_READ_RAW_DATA\n");
        data = (void __user *) arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }
        mir3da_readRawData(client, strbuf);
        if(copy_to_user(data, strbuf, strlen(strbuf)+1))
        {
            err = -EFAULT;
            break;      
        }
        break;      

    case GSENSOR_IOCTL_SET_CALI:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_SET_CALI\n");
        data = (void __user*)arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }
        
        if(copy_from_user(&sensor_data, data, sizeof(sensor_data)))
        {
            err = -EFAULT;
            break;      
        }

        if(atomic_read(&obj->suspend))
        {
            MI_ERR("Perform calibration in suspend state!!\n");
            err = -EINVAL;
        }
        else
        {
            cali[MIR3DA_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
            cali[MIR3DA_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
            cali[MIR3DA_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;              
            err = mir3da_writeCalibration(client, cali);
        }
        break;

    case GSENSOR_IOCTL_CLR_CALI:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_CLR_CALI\n");
        err = mir3da_resetCalibration(client);
        break;

    case GSENSOR_IOCTL_GET_CALI:
        MI_MSG("IOCTRL --- GSENSOR_IOCTL_GET_CALI\n");
        data = (void __user*)arg;
        if(data == NULL)
        {
            err = -EINVAL;
            break;      
        }
        err = mir3da_readCalibration(client, cali);
        if(err)
        {
            break;
        }
        
        sensor_data.x = cali[MIR3DA_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        sensor_data.y = cali[MIR3DA_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        sensor_data.z = cali[MIR3DA_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
        if(copy_to_user(data, &sensor_data, sizeof(sensor_data)))
        {
            err = -EFAULT;
            break;
        }        
        break;  
           
    default:
        return -EINVAL;
    }

    return err;
}
/*----------------------------------------------------------------------------*/
static const struct file_operations mir3da_misc_fops = {
        .owner = THIS_MODULE,
        .unlocked_ioctl = mir3da_misc_ioctl,
};

static struct miscdevice misc_mir3da = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = MIR3DA_MISC_NAME,
        .fops = &mir3da_misc_fops,
};
/*----------------------------------------------------------------------------*/

//runyee zhou del sys show
/*
static ssize_t mir3da_enable_show(struct device_driver *ddri, char *buf)
{
    int ret;
    char bEnable;
    struct i2c_client *client = mir_handle;

    MI_FUN;
	
    ret = mir3da_get_enable(client, &bEnable);   
    if (ret < 0){
        ret = -EINVAL;
    }
    else{
        ret = sprintf(buf, "%d\n", bEnable);
    }

    return ret;
}
//----------------------------------------------------------------------------
#if 0
static ssize_t mir3da_enable_store(struct device_driver *ddri, const char *buf, size_t count)
{
    int ret;
    char bEnable;
    unsigned long enable;
    struct i2c_client *client = mir_handle;

    if (buf == NULL){
        return -1;
    }

    enable = simple_strtoul(buf, NULL, 10);    
    bEnable = (enable > 0) ? true : false;

    ret = mir3da_set_enable (client, bEnable);
    if (ret < 0){
        ret = -EINVAL;
    }
    else{
        ret = count;
    }

    return ret;
}
#endif
//----------------------------------------------------------------------------
static ssize_t mir3da_axis_data_show(struct device_driver *ddri, char *buf)
{
    int result;
    short x,y,z;
    int count = 0;

    result = mir3da_read_data(mir_handle, &x, &y, &z);
    if (result == 0)
        count += sprintf(buf+count, "x= %d;y=%d;z=%d\n", x,y,z);
    else
        count += sprintf(buf+count, "reading failed!");

    return count;
}
//----------------------------------------------------------------------------
static ssize_t mir3da_reg_data_show(struct device_driver *ddri, char *buf)
{
    MIR_HANDLE          handle = mir_handle;
        
    return mir3da_get_reg_data(handle, buf);
}
//----------------------------------------------------------------------------
#if 0
static ssize_t mir3da_reg_data_store(struct device_driver *ddri, const char *buf, size_t count)
{
    int                 addr, data;
    int                 result;

    sscanf(buf, "0x%x, 0x%x\n", &addr, &data);
    
    result = mir3da_register_write(mir_handle, addr, data);
    
    MI_ASSERT(result==0);

    MI_MSG("set[0x%x]->[0x%x]\n",addr,data);	

    return count;
}
#endif
//----------------------------------------------------------------------------
static ssize_t mir3da_log_level_show(struct device_driver *ddri, char *buf)
{
    int ret;

    ret = sprintf(buf, "%d\n", Log_level);

    return ret;
}
//----------------------------------------------------------------------------
#if 0
static ssize_t mir3da_log_level_store(struct device_driver *ddri, const char *buf, size_t count)
{
    Log_level = simple_strtoul(buf, NULL, 10);
    return count;
}
#endif
//----------------------------------------------------------------------------
static ssize_t mir3da_primary_offset_show(struct device_driver *ddri, char *buf){    
    int x=0,y=0,z=0;
   
    mir3da_get_primary_offset(mir_handle,&x,&y,&z);

	  return sprintf(buf, "x=%d ,y=%d ,z=%d\n",x,y,z);
}
//----------------------------------------------------------------------------
static ssize_t mir3da_version_show(struct device_driver *ddri, char *buf)
{
	return sprintf(buf, "%s_%s\n", DRI_VER, CORE_VER);
}
//----------------------------------------------------------------------------
static ssize_t mir3da_vendor_show(struct device_driver *ddri, char *buf)
{
    return sprintf(buf, "%s\n", "MiraMEMS");
}
//----------------------------------------------------------------------------
#if MIR3DA_OFFSET_TEMP_SOLUTION
static ssize_t mir3da_offset_show(struct device_driver *ddri, char *buf)
{
    ssize_t count = 0;
    
    if(bLoad==FILE_EXIST)
    	count += sprintf(buf,"%s",m_work_info.buffer);   
    else
    	count += sprintf(buf,"%s","Calibration file not exist!\n");

    return count;
}
//----------------------------------------------------------------------------
static ssize_t mir3da_calibrate_show(struct device_driver *ddri, char *buf)
{
    int ret;       

    ret = sprintf(buf, "%d\n", bCaliResult);   
    return ret;
}
//----------------------------------------------------------------------------
#if 0
static ssize_t mir3da_calibrate_store(struct device_driver *ddri, const char *buf, size_t count)
{
    signed char     z_dir = 0;
   
    z_dir = simple_strtol(buf, NULL, 10);
    bCaliResult = mir3da_calibrate(mir_handle,z_dir);
    
    return count;
}
#endif
//----------------------------------------------------------------------------
#if FILTER_AVERAGE_ENHANCE
static ssize_t mir3da_average_enhance_show(struct device_driver *ddri, char *buf)
{
    int       ret = 0;
    struct mir3da_filter_param_s    param = {0};

    ret = mir3da_get_filter_param(&param);
    ret |= sprintf(buf, "%d %d %d\n", param.filter_param_l, param.filter_param_h, param.filter_threhold);

    return ret;
}
//----------------------------------------------------------------------------
static ssize_t mir3da_average_enhance_store(struct device_driver *ddri, const char *buf, size_t count)
{ 
    int       ret = 0;
    struct mir3da_filter_param_s    param = {0};
    
    sscanf(buf, "%d %d %d\n", &param.filter_param_l, &param.filter_param_h, &param.filter_threhold);
    
    ret = mir3da_set_filter_param(&param);
    
    return count;
}
#endif 
//----------------------------------------------------------------------------
static DRIVER_ATTR(enable,          S_IRUGO | S_IWUGO,  mir3da_enable_show,             NULL);
static DRIVER_ATTR(axis_data,       S_IRUGO,  mir3da_axis_data_show,          NULL);
static DRIVER_ATTR(reg_data,        S_IWUGO | S_IRUGO,  mir3da_reg_data_show,           NULL);
static DRIVER_ATTR(log_level,       S_IWUGO | S_IRUGO,  mir3da_log_level_show,          NULL);
static DRIVER_ATTR(primary_offset,  S_IRUGO,  mir3da_primary_offset_show,     NULL);
static DRIVER_ATTR(vendor,          S_IRUGO,  mir3da_vendor_show,             NULL);
static DRIVER_ATTR(version,         S_IRUGO,  mir3da_version_show,            NULL); 
#if MIR3DA_OFFSET_TEMP_SOLUTION
static DRIVER_ATTR(offset,          S_IRUGO,  mir3da_offset_show,             NULL);
static DRIVER_ATTR(calibrate_miraGSensor,       S_IWUGO | S_IRUGO,  mir3da_calibrate_show,          NULL);
#endif
#if FILTER_AVERAGE_ENHANCE
static DRIVER_ATTR(average_enhance, S_IWUGO | S_IRUGO,  mir3da_average_enhance_show,    mir3da_average_enhance_store);
#endif 
//----------------------------------------------------------------------------
static struct driver_attribute *mir3da_attributes[] = { 
    &driver_attr_enable,
    &driver_attr_axis_data,
    &driver_attr_reg_data,
    &driver_attr_log_level,
    &driver_attr_primary_offset,    
    &driver_attr_vendor,
    &driver_attr_version,
#if MIR3DA_OFFSET_TEMP_SOLUTION
    &driver_attr_offset,    
    &driver_attr_calibrate_miraGSensor,
#endif
#if FILTER_AVERAGE_ENHANCE
    &driver_attr_average_enhance,
#endif     
};
//----------------------------------------------------------------------------
static int mir3da_create_attr(struct device_driver *driver) 
{
    int idx, err = 0;
    int num = (int)(sizeof(mir3da_attributes)/sizeof(mir3da_attributes[0]));
    if (driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
		err = driver_create_file(driver, mir3da_attributes[idx]);
        if(err)
        {            
            MI_MSG("driver_create_file (%s) = %d\n", mir3da_attributes[idx]->attr.name, err);
            break;
        }
    }    
    return err;
}
//----------------------------------------------------------------------------
static int mir3da_delete_attr(struct device_driver *driver)
{
    int idx ,err = 0;
    int num = (int)(sizeof(mir3da_attributes)/sizeof(mir3da_attributes[0]));

    if(driver == NULL)
    {
        return -EINVAL;
    }

    for(idx = 0; idx < num; idx++)
    {
        driver_remove_file(driver, mir3da_attributes[idx]);
    }

    return err;
}
*/



/*----------------------------------------------------------------------------*/
#if MTK_ANDROID_M
static void mir3da_power(unsigned int on) 
{
#else
static void mir3da_power(struct acc_hw *hw, unsigned int on) 
{
#endif
    static unsigned int power_on = 0;

    MI_MSG("power %s\n", on ? "on" : "off");

#if !MTK_ANDROID_M
    if(hw->power_id != POWER_NONE_MACRO)       
    {        
        MI_MSG("power %s\n", on ? "on" : "off");
        if(power_on == on)    
        {
            MI_MSG("ignore power control: %d\n", on);
        }
        else if(on)   
        {
            if(!hwPowerOn(hw->power_id, hw->power_vol, "mir3da"))
            {
                MI_ERR("power on fails!!\n");
            }
        }
        else    
        {
            if (!hwPowerDown(hw->power_id, "mir3da"))
            {
                MI_ERR("power off fail!!\n");
            }              
        }
    }
#endif
    power_on = on;    
}

//#ifndef CONFIG_HAS_EARLYSUSPEND
#if 1
//---------------------------------------------------------------------------*/
static int mir3da_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct mir3da_i2c_data *obj = i2c_get_clientdata(client);    
	int err = 0;

	MI_FUN;    

	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(obj == NULL)
		{
			MI_ERR("null pointer!!\n");
			return -EINVAL;
		}
		
		err = mir3da_set_enable(obj->client, false);
        if(err) {
            return err;
        }
 #if !MTK_ANDROID_M
    mir3da_power(obj->hw,0);
#else
    mir3da_power(0);
#endif                

	}
	return err;
}
//----------------------------------------------------------------------------*/
static int mir3da_resume(struct i2c_client *client)
{
	struct mir3da_i2c_data *obj = i2c_get_clientdata(client);        
	int err;
	MI_FUN;

	if(obj == NULL)
	{
		MI_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
       err = mir3da_chip_resume(obj->client);
	if(err) {
		MI_ERR("chip resume fail!!\n");
		return err;
	}

        err = mir3da_setPowerMode(obj->client, true);
        if(err != 0) {
		return err;
        }		
 #if !MTK_ANDROID_M
    mir3da_power(obj->hw,1);
#else
    mir3da_power(1);
#endif    
    
	atomic_set(&obj->suspend, 0);

	return 0;
}
#else 
/*----------------------------------------------------------------------------*/
static void mir3da_early_suspend(struct early_suspend *h) 
{
	struct mir3da_i2c_data *obj = container_of(h, struct mir3da_i2c_data, early_drv);   
	int err;
	MI_FUN;    

	if(obj == NULL)
	{
		MI_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1); 

	err = mir3da_setPowerMode(obj->client, false);
	if(err) {
		MI_ERR("write power control fail!!\n");
		return;
	}

	sensor_power = false;
	
 #if !MTK_ANDROID_M
    mir3da_power(obj->hw,0);
#else
    mir3da_power(0);
#endif    
}

/*----------------------------------------------------------------------------*/
static void mir3da_late_resume(struct early_suspend *h)
{
	struct mir3da_i2c_data  *obj = container_of(h, struct mir3da_i2c_data, early_drv);         
	int                     err;

	MI_FUN;

	if(obj == NULL) {
		MI_ERR("null pointer!!\n");
		return;
	}

        err = mir3da_chip_resume(obj->client);
	if(err) {
		MI_ERR("chip resume fail!!\n");
		return;
	}


        err = mir3da_setPowerMode(obj->client, true);
        if(err != 0) {
		return err;
        }		

 #if !MTK_ANDROID_M
    mir3da_power(obj->hw,1);
#else
    mir3da_power(1);
#endif    

	atomic_set(&obj->suspend, 0);    
}
#endif 
/*----------------------------------------------------------------------------*/
static int mir3da_operate(void* self, uint32_t command, void* buff_in, int size_in,
        void* buff_out, int size_out, int* actualout)
{
    int err = 0;
    int value;    
    struct mir3da_i2c_data *priv = (struct mir3da_i2c_data*)self;
    struct hwm_sensor_data* gsensor_data;
    char buff[MIR3DA_BUFSIZE];
    
    switch (command)
    {
        case SENSOR_DELAY:
            MI_MSG("mir3da_operate, command is SENSOR_DELAY");

			  if((buff_in == NULL) || (size_in < sizeof(int)))
			  {
				 MI_ERR("Set delay parameter error!\n");
				 err = -EINVAL;
			  }
			  else
			  {
				 value = *(int *)buff_in;

				 err = mir3da_set_odr(priv->client,value); 
				 if(err) {
					 MI_ERR("mir3da_set_odr failed !");
					 err = -EINVAL;
				 }								
			 	}						
            break;

        case SENSOR_ENABLE:
            MI_MSG("mir3da_operate enable gsensor\n");
            if((buff_in == NULL) || (size_in < sizeof(int)))
            {
                MI_ERR("mir3da_operate Enable sensor parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                
                MI_MSG("mir3da_operate, command is SENSOR_ENABLE, value = %d\n", value);
                
                if(((value == 0) && (sensor_power == false)) ||((value == 1) && (sensor_power == true)))
                {
                    MI_MSG("Gsensor device have updated!\n");
                }
                else
                {
                    err = mir3da_setPowerMode( priv->client, !sensor_power);
                }
            }
            break;

        case SENSOR_GET_DATA:
            MI_MSG("mir3da_operate, command is SENSOR_GET_DATA\n");
            if((buff_out == NULL) || (size_out< sizeof(struct hwm_sensor_data)))
            {
                MI_MSG("get sensor data parameter error!\n");
                err = -EINVAL;
            }
            else
            {
                gsensor_data = (struct hwm_sensor_data *)buff_out;
               
                mir3da_readSensorData(priv->client, buff);  
                sscanf(buff, "%x %x %x", &gsensor_data->values[0], 
                &gsensor_data->values[1], &gsensor_data->values[2]);
                
                gsensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;                
                gsensor_data->value_divide = 1000;                
            }
            break;
        default:
                MI_MSG("gsensor operate function no this parameter %d!\n", command);
                err = -1;
            break;
    }
    
    return err;
}
/*----------------------------------------------------------------------------*/
int i2c_smbus_read(PLAT_HANDLE handle, u8 addr, u8 *data)
{
    int                 res = 0;
    struct i2c_client   *client = (struct i2c_client*)handle;
    
    *data = i2c_smbus_read_byte_data(client, addr);
    
    return res;
}
/*----------------------------------------------------------------------------*/
int i2c_smbus_read_block(PLAT_HANDLE handle, u8 addr, u8 count, u8 *data)
{
    int                 res = 0;
    struct i2c_client   *client = (struct i2c_client*)handle;
    
    res = i2c_smbus_read_i2c_block_data(client, addr, count, data);
    
    return res;
}
/*----------------------------------------------------------------------------*/
int i2c_smbus_write(PLAT_HANDLE handle, u8 addr, u8 data)
{
    int                 res = 0;
    struct i2c_client   *client = (struct i2c_client*)handle;
    
    res = i2c_smbus_write_byte_data(client, addr, data);
    
    return res;
}
/*----------------------------------------------------------------------------*/
void msdelay(int ms)
{
    mdelay(ms);
}

#if MTK_ANDROID_M
/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int mir3da_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */
static int mir3da_enable_nodata(int en)
{
	int res = 0;
	int retry = 0;
	char bEnable = false;

	if (1 == en)
		bEnable = true;
	if (0 == en)
		bEnable = false;

	for (retry = 0; retry < 3; retry++) {
		res = mir3da_setPowerMode(mir_handle, bEnable);
		if (res == 0) {
			MI_ERR("mir3da_SetPowerMode done\n");
			break;
		}
		MI_ERR("mir3da_SetPowerMode fail\n");
	}

	if (res != 0) {
		MI_ERR("mir3da_SetPowerMode fail!\n");
		return -1;
	}
	MI_MSG("mir3da_enable_nodata OK!\n");
	return 0;
}

static int mir3da_set_delay(u64 ns)
{
	int value = 0;

	value = (int)ns/1000/1000;
	MI_MSG("mir3daset_delay (%d), chip only use 1024HZ\n", value);
	return 0;
}

static int mir3da_get_data(int *x , int *y, int *z, int *status) 
{
	char buff[MIR3DA_BUFSIZE];
	int ret;

	mir3da_readSensorData(mir_handle, buff);
	ret = sscanf(buff, "%x %x %x", x, y, z);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}
#endif

#if MIR3DA_OFFSET_TEMP_SOLUTION
MIR_GENERAL_OPS_DECLARE(ops_handle, i2c_smbus_read, i2c_smbus_read_block, i2c_smbus_write, sensor_sync_write, sensor_sync_read, check_califolder_exist,get_address,support_fast_auto_cali,msdelay, printk, sprintf);
#else
MIR_GENERAL_OPS_DECLARE(ops_handle, i2c_smbus_read, i2c_smbus_read_block, i2c_smbus_write, NULL, NULL, NULL,get_address,NULL,msdelay, printk, sprintf);
#endif
/*----------------------------------------------------------------------------*/
static int mir3da_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int                 result;
    struct i2c_client *new_client;
    struct mir3da_i2c_data *obj;
    struct hwmsen_object sobj;    
	unsigned char chip_id=0;
#if MTK_ANDROID_M
	struct acc_control_path ctl = {0};
	struct acc_data_path data = {0};
#endif 

    MI_FUN;
 printk("mir3da_probe=========in!!!!!\n");
    
    if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
    {
        MI_ERR("kzalloc failed!");
        result = -ENOMEM;
        goto exit;
    }   
    
    memset(obj, 0, sizeof(struct mir3da_i2c_data));
    obj->client = client;
  
#ifdef FPGA_EARLY_PORTING
	obj->client->timing = 100;
#else
	obj->client->timing = 400;
#endif
   new_client = obj->client;
   
    i2c_set_clientdata(new_client,obj);	
#if MTK_ANDROID_M
    obj->hw = hw;
    	printk("mir3da_init=====%s: i2c_number=%d", __func__, hw->i2c_num);
#else
#if MTK_AUTO_MODE
    obj->hw = mir3da_get_cust_acc_hw();	
    	printk("MTK_AUTO_MODE is define!!!!!\n");
#else
    obj->hw = get_cust_acc_hw();	
#endif
#endif 
	result = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
    if(result)
    {
        MI_ERR("invalid direction: %d\n", obj->hw->direction);
        goto exit;
    }   
	
    obj->reso = &mir3da_data_resolution[0];
    gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;

    result = mir3da_resetCalibration(new_client);
    if(result != 0) {
	   return result;
    }	

    atomic_set(&obj->trace, 0);
    atomic_set(&obj->suspend, 0);

    if(mir3da_install_general_ops(&ops_handle)){
        printk("Install ops failed !\n");
        goto exit_init_failed;
    }

#if MIR3DA_OFFSET_TEMP_SOLUTION
    m_work_info.wq = create_singlethread_workqueue( "oo" );
    if(NULL==m_work_info.wq) {
        printk("Failed to create workqueue !");
        goto exit_init_failed;
    }
    
    INIT_DELAYED_WORK( &m_work_info.read_work, sensor_read_work );
    INIT_DELAYED_WORK( &m_work_info.write_work, sensor_write_work );
#endif
	i2c_smbus_read((PLAT_HANDLE) new_client, NSA_REG_WHO_AM_I, &chip_id);	
	if(chip_id != 0x13){
		client->addr = 0x27;
		printk("mir3da----chip_id!= 0x13");
	}	
    
    mir_handle = mir3da_core_init(client);
    if(NULL == mir_handle){
        printk("chip init failed !\n");
        goto exit_init_failed;        
    }

    result = misc_register(&misc_mir3da);
    if (result) {
        MI_ERR("%s: misc register failed !\n", __func__);
        goto exit_misc_device_register_failed;
    }
#if 0  //runyee zhou add
#if MTK_AUTO_MODE    
    result = mir3da_create_attr(&(mir3da_init_info.platform_diver_addr->driver));
    if(result)
    {
        MI_ERR("create attribute result = %d\n", result);
        result = -EINVAL;
	 goto exit_create_attr_failed;
    }
#else
    result = mir3da_create_attr(&mir3da_gsensor_driver.driver);
    if(result)
    {
        MI_ERR("create attribute result = %d\n", result);
        result = -EINVAL;		
        goto exit_create_attr_failed;
    }
#endif    
#endif  
    sobj.self = obj;
    sobj.polling = 1;
    sobj.sensor_operate =mir3da_operate;
    result = hwmsen_attach(ID_ACCELEROMETER, &sobj);
    if(result)
    {
        MI_ERR("attach fail = %d\n", result);
        goto exit_kfree;
    }
    
//#ifdef CONFIG_HAS_EARLYSUSPEND
#if 0
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = mir3da_early_suspend,
	obj->early_drv.resume   = mir3da_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif

#if MTK_ANDROID_M
    ctl.is_use_common_factory = false;
	ctl.open_report_data = mir3da_open_report_data;
	ctl.enable_nodata = mir3da_enable_nodata;
	ctl.set_delay  = mir3da_set_delay;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = obj->hw->is_batch_supported;
	result = acc_register_control_path(&ctl);
	if (result) {
		MI_ERR("register acc control path err\n");
		goto exit_kfree;
	}
	data.get_data = mir3da_get_data;
	data.vender_div = 1000;
	result = acc_register_data_path(&data);
	if (result) {
		MI_ERR("register acc data path err= %d\n", result);
		goto exit_kfree;
	}
#endif
    mir3da_init_flag = 0;
 printk("[mir3da_probe] out!!!!!\n");
    return result;
//exit_create_attr_failed:
	misc_deregister(&misc_mir3da);
	exit_misc_device_register_failed:
	exit_init_failed:
	mir3da_init_flag = -1;	
	exit_kfree:
	kfree(obj);
	exit:
	MI_ERR("%s: err = %d\n", __func__, result);        
	return result;
}
/*----------------------------------------------------------------------------*/
static int  mir3da_remove(struct i2c_client *client)
{
    //int err = 0;	
#if 0 //runye zhou add
#if MTK_AUTO_MODE    
    err = mir3da_delete_attr(&(mir3da_init_info.platform_diver_addr->driver));
    if(err)
    {
        MI_ERR("mir3da_delete_attr fail: %d\n", err);
    }
#else
    err = mir3da_delete_attr(&mir3da_gsensor_driver.driver);
    if(err)
    {
        MI_ERR("mir3da_delete_attr fail: %d\n", err);
    }
#endif 
#endif
    misc_deregister(&misc_mir3da);
    mir_handle = NULL;
    i2c_unregister_device(client);
    kfree(i2c_get_clientdata(client));    
    
    return 0;
}
#if !MTK_ANDROID_M
/*----------------------------------------------------------------------------*/
static int mir3da_detect(struct i2c_client *new_client,int kind,struct i2c_board_info *info)
{
      struct i2c_adapter *adapter = new_client->adapter;

      MI_MSG("mir3da_detect, bus[%d] addr[0x%x]\n", adapter->nr,new_client->addr);

      if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

       strcpy(info->type, MIR3DA_DRV_NAME);

       return 0;
}
#endif
//----------------------------------------------------------------------------
static const struct i2c_device_id mir3da_id[] = {
    { MIR3DA_DRV_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, mir3da_id);

#if MTK_ANDROID_23
static unsigned short mir3da_force[] = {0x00, MIR3DA_I2C_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
static const unsigned short *const mir3da_forces[] = { mir3da_force, NULL };
static struct i2c_client_address_data mir3da_addr_data = { .forces = mir3da_forces,};
#endif

#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,accel_double"},
	{},
};
#endif


/*----------------------------------------------------------------------------*/
static struct i2c_driver mir3da_driver = {
    .driver = 
    {
        .name    = MIR3DA_DRV_NAME,
        .owner   = THIS_MODULE,
#ifdef CONFIG_OF
	    .of_match_table = accel_of_match,
#endif
    },    
    .probe       = mir3da_probe,
    .remove      = mir3da_remove,
#if !MTK_ANDROID_M	
    .detect	     = mir3da_detect,
#endif

//#if !defined(CONFIG_HAS_EARLYSUSPEND) 
    .suspend     = mir3da_suspend,
    .resume      = mir3da_resume,
//#endif
    .id_table    = mir3da_id,
#if MTK_ANDROID_23
    .address_data = &mir3da_addr_data,    
#endif
};

/*----------------------------------------------------------------------------*/
#if MTK_AUTO_MODE
static int mir3da_local_init(void) 
{
#if !MTK_ANDROID_M
#if MTK_AUTO_MODE
    struct acc_hw *hw = mir3da_get_cust_acc_hw();
#else
    struct acc_hw *hw = get_cust_acc_hw();
#endif	
#endif	
    MI_FUN;
#if !MTK_ANDROID_M
    mir3da_power(hw,1);
#else
    mir3da_power(1);
#endif   
    if(i2c_add_driver(&mir3da_driver))
    {
        MI_ERR("add driver error\n");
        return -1;
    }

	if(-1 == mir3da_init_flag)
	{
	   return -1;
	}		

    return 0;
}
/*----------------------------------------------------------------------------*/
static int mir3da_local_remove(void)
{
#if !MTK_ANDROID_M
#if MTK_AUTO_MODE
    struct acc_hw *hw =mir3da_get_cust_acc_hw();
#else
    struct acc_hw *hw =get_cust_acc_hw();
#endif
#endif
    MI_FUN;    
	
#if !MTK_ANDROID_M
    mir3da_power(hw,0);
#else
    mir3da_power(0);
#endif     
	
    i2c_del_driver(&mir3da_driver);
	
    return 0;
}
/*----------------------------------------------------------------------------*/
#else
static int mir3da_platform_probe(struct platform_device *pdev) 
{
#if !MTK_ANDROID_M	
    struct acc_hw *hw = get_cust_acc_hw();
#endif
    MI_FUN;

#if !MTK_ANDROID_M
    mir3da_power(hw,1);
#else
    mir3da_power(1);
#endif 

    if(i2c_add_driver(&mir3da_driver))
    {
        MI_ERR("add driver error\n");
        return -1;
    }

    return 0;
}
/*----------------------------------------------------------------------------*/
static int mir3da_platform_remove(struct platform_device *pdev)
{
#if !MTK_ANDROID_M
    struct acc_hw *hw = get_cust_acc_hw();
#endif
    MI_FUN;    

#if !MTK_ANDROID_M
    mir3da_power(hw,0);
#else
    mir3da_power(0);
#endif 
	
    i2c_del_driver(&mir3da_driver);

    return 0;
}
#endif
/*----------------------------------------------------------------------------*/

static int __init mir3da_init(void)
{    
	const char *name = "mediatek,da226";

	hw = get_accel_dts_func(name, hw);
	if (!hw)
		printk("get dts info fail\n");
printk("mir3da_init=====%s: i2c_number=%d,i2c_addr=%s\n", __func__, hw->i2c_num,&hw->i2c_addr[0]);
	  
	acc_driver_add(&mir3da_init_info);
	
	return 0;
/*
#if MTK_ANDROID_M
	const char *name = "mediatek,da226";

	hw = get_accel_dts_func(name, hw);
	printk("mir3da_init=====%s: i2c_number=%d,i2c_addr=%s\n", __func__, hw->i2c_num,&hw->i2c_addr[0]);
	
	if (!hw)
		MI_ERR("get dts info fail\n");

	acc_driver_add(&mir3da_init_info);
#else
#if MTK_AUTO_MODE
    struct acc_hw *hw = mir3da_get_cust_acc_hw();
#else
    struct acc_hw *hw = get_cust_acc_hw();
#endif 
    
    MI_FUN;

#if !MTK_ANDROID_23
    i2c_register_board_info(hw->i2c_num, &mir3da_i2c_boardinfo, 1);
#endif

#if MTK_AUTO_MODE
    hwmsen_gsensor_add(&mir3da_init_info);
#else
    if(platform_driver_register(&mir3da_gsensor_driver))
    {
        MI_ERR("failed to register driver");
        return -ENODEV;
    }
#endif
#endif
    return 0;
*/    
}
/*----------------------------------------------------------------------------*/
static void __exit mir3da_exit(void)
{    
    MI_FUN;
	
#if MTK_ANDROID_M	
    MI_MSG("mir3da_exit\n");
#else
#if !MTK_AUTO_MODE
    platform_driver_unregister(&mir3da_gsensor_driver);
#endif
#endif
}
/*----------------------------------------------------------------------------*/

MODULE_AUTHOR("MiraMEMS <lschen@miramems.com>");
MODULE_DESCRIPTION("MirMEMS 3-Axis Accelerometer driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

module_init(mir3da_init);
module_exit(mir3da_exit);
