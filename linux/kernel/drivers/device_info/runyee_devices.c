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
//#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

#include "runyee_devices.h"
#include "runyee_MemoryDevice.h"

typedef int (*hct_dev_print)(struct seq_file *se_f);

typedef int (*hct_dev_set_used)(char * module_name, int pdata);


struct memory_temp_info
{
       char   cs_part_number[50];
       //char   cs_part_number_temp[50]; 
       int    support_num;    
};
struct memory_temp_info memory_temp[10];

struct hct_memory_device_dinfo
{
    struct list_head hct_list; 
    char memory_module_descrip[50];
    struct list_head memory_list; 
    struct memory_temp_info *pdata;     
    campatible_type type;
};

struct hct_lcm_device_idnfo
{
    struct list_head hct_list;  // list in hct_devices_info
    char lcm_module_descrip[50];
    struct list_head lcm_list;  // list in hct_lcm_info
    LCM_DRIVER* pdata;
    campatible_type type;
};

struct hct_camera_device_dinfo
{
    struct list_head hct_list;
    char camera_module_descrip[20];    
    struct list_head camera_list;
    int  camera_id;
    ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT* pdata;
    campatible_type type;
    CAMERA_DUAL_CAMERA_SENSOR_ENUM  cam_type;
};

struct hct_accsensor_device_dinfo
{
    struct list_head hct_list;
    char sensor_descrip[20];
    struct list_head sensor_list;
    int dev_addr;
    int ps_direction;   // ps is throthold, als/msensor is diriction
    campatible_type type;
    struct acc_init_info * pdata; 
};
//****zhou
struct hct_alsps_device_dinfo
{
    struct list_head hct_list;
    char sensor_descrip[20];
    struct list_head sensor_list;
    int dev_addr;
    //int ps_direction;   // ps is throthold, als/msensor is diriction
    campatible_type type;
    struct alsps_init_info * pdata;
};

struct hct_touchpanel_device_dinfo
{
    struct list_head hct_list;     // list in hct_devices_info
    char touch_descrip[20];
    struct list_head touch_list;
    int dev_addr;
    struct tpd_driver_t * pdata;
//    int ps_direction;   // ps is throthold, als/msensor is diriction
    campatible_type type;
};

struct hct_type_info
{
    struct list_head hct_device;  // list in hct_devices_info
    struct list_head hct_dinfo;    // list in hct_touch_device_dinfo
    hct_dev_print type_print;
    hct_dev_set_used set_used;
    int dev_type;
    void * current_used_dinfo;
    struct mutex	info_mutex;
};


struct hct_devices_info
{
         struct list_head  type_list;   // hct_type_info list
         struct list_head  dev_list;    // all_list

        struct mutex	de_mutex;
};

struct hct_devices_info * hct_devices;
static int hct_memory_info_print(struct seq_file *se_f);
static int hct_camera_info_print(struct seq_file *se_f);
static int hct_touchpanel_info_print(struct seq_file *se_f);
static int hct_accsensor_info_print(struct seq_file *se_f);
static int hct_alsps_info_print(struct seq_file *se_f);

//static int hct_memory_set_used(char * module_name,int pdata);
static int hct_lcm_set_used(char * module_name,int pdata);
static int hct_camera_set_used(char * module_name, int pdata);
static int hct_touchpanel_set_used(char * module_name, int pdata);
static int hct_accsensor_set_used(char * module_name, int pdata);
static int hct_alsps_set_used(char * module_name, int pdata);


static int hct_memory_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * mem_type_info;    
    struct hct_memory_device_dinfo * mem_device;
    int flag = -1;

    seq_printf(se_f, "MEMORY--------------------\t \n");
    
    list_for_each_entry(mem_type_info,&hct_devices->type_list,hct_device){
            if(mem_type_info->dev_type == ID_MEMORY_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(mem_device,&mem_type_info->hct_dinfo,memory_list){
                seq_printf(se_f, "      %20s\t\t:   ",mem_device->memory_module_descrip);
                if(mem_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                    seq_printf(se_f, "  used \n");
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
    
    return 0;

}
static int hct_lcm_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * lcm_type_info;    
    struct hct_lcm_device_idnfo * plcm_device;
    int flag = -1;

    seq_printf(se_f, "LCM-----------------------\t \n");
    
    list_for_each_entry(lcm_type_info,&hct_devices->type_list,hct_device){
            if(lcm_type_info->dev_type == ID_LCM_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(plcm_device,&lcm_type_info->hct_dinfo,lcm_list){
                seq_printf(se_f, "      %20s\t\t:   ",plcm_device->lcm_module_descrip);
                if(plcm_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                    seq_printf(se_f, "  used \n");
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
    
    return 0;

}

static void init_device_type_info(struct hct_type_info * pdevice, void *used_info, int type)
{
       memset(pdevice,0, sizeof(struct hct_type_info));
       INIT_LIST_HEAD(&pdevice->hct_device);
       INIT_LIST_HEAD(&pdevice->hct_dinfo);
       pdevice ->dev_type = type;
       if(used_info)
          pdevice->current_used_dinfo=used_info;
          
       if(type == ID_MEMORY_TYPE)
       {
          pdevice->type_print = hct_memory_info_print;
         // pdevice->set_used= hct_memory_set_used;
       }
       else if(type == ID_LCM_TYPE)
       {
          pdevice->type_print = hct_lcm_info_print;
          pdevice->set_used= hct_lcm_set_used;
       }
       else if(type == ID_CAMERA_TYPE)
       {
           pdevice->type_print = hct_camera_info_print;
           pdevice->set_used = hct_camera_set_used;
       }
       else if(type == ID_TOUCH_TYPE)
       {
           pdevice->type_print = hct_touchpanel_info_print;
           pdevice->set_used = hct_touchpanel_set_used;
       }
       else if(type == ID_ACCSENSOR_TYPE)
       {
           pdevice->type_print = hct_accsensor_info_print;
           pdevice->set_used = hct_accsensor_set_used;
       }
       else if(type == ID_ALSPS_TYPE)
       {
           pdevice->type_print = hct_alsps_info_print;
           pdevice->set_used = hct_alsps_set_used;
       }
          
       mutex_init(&pdevice->info_mutex);
       list_add(&pdevice->hct_device,&hct_devices->type_list);
}

static void init_lcm_device(struct hct_lcm_device_idnfo *pdevice, LCM_DRIVER* nLcm)
{
    memset(pdevice,0, sizeof(struct hct_lcm_device_idnfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->lcm_list);
    strcpy(pdevice->lcm_module_descrip,nLcm->name);
    pdevice->pdata=nLcm;
    
}
#if 0
int hct_memory_set_used(char * module_name, int pdata)
{
    struct hct_type_info * mem_type_info;    
    struct hct_memory_device_idnfo * mem_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(mem_type_info,&hct_devices->type_list,hct_device){
             if(mem_type_info->dev_type == ID_MEMORY_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(mem_device,&mem_type_info->hct_dinfo,memory_list){
           if(!strcmp(module_name,mem_device->memory_module_descrip))
           {
               mem_device->type = DEVICE_USED;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}
#endif
int hct_lcm_set_used(char * module_name, int pdata)
{
    struct hct_type_info * lcm_type_info;    
    struct hct_lcm_device_idnfo * plcm_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(lcm_type_info,&hct_devices->type_list,hct_device){
             if(lcm_type_info->dev_type == ID_LCM_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(plcm_device,&lcm_type_info->hct_dinfo,lcm_list){
           if(!strcmp(module_name,plcm_device->lcm_module_descrip))
           {
               plcm_device->type = DEVICE_USED;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}

static void init_memory_device(struct hct_memory_device_dinfo *pdevice, struct memory_temp_info* pdata)
{
    memset(pdevice,0, sizeof(struct hct_memory_device_dinfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->memory_list);
    strcpy(pdevice->memory_module_descrip, pdata->cs_part_number);
    pdevice->pdata=pdata;
    
}
int hct_memory_device_add(struct memory_temp_info* pdata, campatible_type isUsed)
{
    struct hct_type_info * mem_type_info;    
    struct hct_memory_device_dinfo * mem_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(mem_type_info,&hct_devices->type_list,hct_device){
             if(mem_type_info->dev_type == ID_MEMORY_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           mem_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);

           if(mem_type_info == NULL)
            {
                  printk("memory alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
           
           if(isUsed)
             init_device_type_info(mem_type_info, pdata, ID_MEMORY_TYPE);
           else
             init_device_type_info(mem_type_info, NULL, ID_MEMORY_TYPE);
       }
       else
       {
           if(isUsed &&(mem_type_info->current_used_dinfo!=NULL))
             printk("~~~~add memory error , duplicated current used memory \n");
       }
     
       flag =-1;
       
       list_for_each_entry(mem_device,&mem_type_info->hct_dinfo,memory_list){
              if(!strcmp(pdata->cs_part_number,mem_device->memory_module_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             printk("error ___ memory type is duplicated \n");
             goto duplicated_faild;
       }
       else
       {
           mem_device = kmalloc(sizeof(struct hct_memory_device_dinfo), GFP_KERNEL);
            if(mem_device == NULL)
            {
                  printk("memory_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto duplicated_faild;
            }
            
           init_memory_device(mem_device,pdata);
           mem_device->type=isUsed;
           
           
           list_add(&mem_device->hct_list, &hct_devices->dev_list);
           list_add(&mem_device->memory_list, &mem_type_info->hct_dinfo);
       }
 
     
     return 0;
duplicated_faild:
    kfree(mem_device);
malloc_faid:
    
    printk("%s: error return: %x: ---\n",__func__,reterror);
    return reterror;
    
}

int hct_lcm_device_add(LCM_DRIVER* nLcm, campatible_type isUsed)
{
    struct hct_type_info * lcm_type_info;    
    struct hct_lcm_device_idnfo * plcm_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(lcm_type_info,&hct_devices->type_list,hct_device){
             if(lcm_type_info->dev_type == ID_LCM_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           lcm_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);

           if(lcm_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
           
           if(isUsed)
             init_device_type_info(lcm_type_info, nLcm, ID_LCM_TYPE);
           else
             init_device_type_info(lcm_type_info, NULL, ID_LCM_TYPE);
       }
       else
       {
           if(isUsed &&(lcm_type_info->current_used_dinfo!=NULL))
             printk("~~~~add lcm error , duplicated current used lcm \n");
       }
     
       flag =-1;
       
       list_for_each_entry(plcm_device,&lcm_type_info->hct_dinfo,lcm_list){
              if(!strcmp(nLcm->name,plcm_device->lcm_module_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             printk("error ___ lcm type is duplicated \n");
             goto duplicated_faild;
       }
       else
       {
           plcm_device = kmalloc(sizeof(struct hct_lcm_device_idnfo), GFP_KERNEL);
            if(plcm_device == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto duplicated_faild;
            }
            
           init_lcm_device(plcm_device,nLcm);
           plcm_device->type=isUsed;
           
           
           list_add(&plcm_device->hct_list, &hct_devices->dev_list);
           list_add(&plcm_device->lcm_list,&lcm_type_info->hct_dinfo);
       }
 
     
     return 0;
duplicated_faild:
    kfree(plcm_device);
malloc_faid:
    
    printk("%s: error return: %x: ---\n",__func__,reterror);
    return reterror;
    
}


static int hct_camera_set_used(char * module_name, int pdata)
{
    struct hct_type_info * camera_type_info;    
    struct hct_camera_device_dinfo * pcamera_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(camera_type_info,&hct_devices->type_list,hct_device){
             if(camera_type_info->dev_type == ID_CAMERA_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(pcamera_device,&camera_type_info->hct_dinfo,camera_list){
           if(!strcmp(module_name,pcamera_device->camera_module_descrip))
           {
               pcamera_device->type = DEVICE_USED;
               pcamera_device->cam_type = pdata;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}

static int hct_camera_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * camera_type_info;    
    struct hct_camera_device_dinfo * pcamera_device;
    int flag = -1;

    seq_printf(se_f, "CAMERA--------------------\n");

    list_for_each_entry(camera_type_info,&hct_devices->type_list,hct_device){
            if(camera_type_info->dev_type == ID_CAMERA_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(pcamera_device,&camera_type_info->hct_dinfo,camera_list){
                seq_printf(se_f, "      %20s\t\t:   ",pcamera_device->camera_module_descrip);
                if(pcamera_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                {
                    seq_printf(se_f, "  used     \t");
                    if(pcamera_device->cam_type ==DUAL_CAMERA_MAIN_SENSOR)
                        seq_printf(se_f, "  main camera \n");
                    else if(pcamera_device->cam_type ==DUAL_CAMERA_SUB_SENSOR)
                        seq_printf(se_f, "  sub camera \n");
                    else
                        seq_printf(se_f, " camera unsupportd type \n");
                }
                
                
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
    return 0;
}

static void init_camera_device(struct hct_camera_device_dinfo *pdevice, ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT* mCamera)
{
    memset(pdevice,0, sizeof(struct hct_camera_device_dinfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->camera_list);
    strcpy(pdevice->camera_module_descrip,mCamera->drvname);
    pdevice->pdata=mCamera;
    
}


int hct_camera_device_add(ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT* mCamera, campatible_type isUsed)
{
    struct hct_type_info * camera_type_info;    
    struct hct_camera_device_dinfo * pcamera_device;
    int flag = -1;
    int reterror=0;

     list_for_each_entry(camera_type_info,&hct_devices->type_list,hct_device){
             if(camera_type_info->dev_type == ID_CAMERA_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           camera_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);
           
           if(camera_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
       
           if(isUsed)
             init_device_type_info(camera_type_info, mCamera, ID_CAMERA_TYPE);
           else
             init_device_type_info(camera_type_info, NULL, ID_CAMERA_TYPE);
       }
       else
       {
           if(isUsed &&(camera_type_info->current_used_dinfo!=NULL))
             printk("~~~~add camera error , duplicated current used lcm \n");
       }
     
       flag =-1;
       
       list_for_each_entry(pcamera_device,&camera_type_info->hct_dinfo,camera_list){
              if(!strcmp(mCamera->drvname,pcamera_device->camera_module_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             goto duplicated_faild;
       }
       else
       {
           pcamera_device = kmalloc(sizeof(struct hct_camera_device_dinfo), GFP_KERNEL);
            if(camera_type_info == NULL)
            {
                  printk("camera_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto malloc_faid;
            }
           init_camera_device(pcamera_device,mCamera);
           pcamera_device->type=isUsed;
           
           list_add(&pcamera_device->hct_list, &hct_devices->dev_list);
           list_add(&pcamera_device->camera_list,&camera_type_info->hct_dinfo);
       }
       
       return 0;
       
     duplicated_faild:
     
         kfree(pcamera_device);
     malloc_faid:
         
         printk("%s: error return: %x: ---\n",__func__,reterror);
         return reterror;
    
}


static int hct_touchpanel_set_used(char * module_name, int pdata)
{
    struct hct_type_info * touchpanel_type_info;    
    struct hct_touchpanel_device_dinfo * ptouchpanel_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(touchpanel_type_info,&hct_devices->type_list,hct_device){
             if(touchpanel_type_info->dev_type == ID_TOUCH_TYPE)
            {
                printk("touch type has find break !!\n");
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(ptouchpanel_device,&touchpanel_type_info->hct_dinfo,touch_list){
           if(!strcmp(module_name,ptouchpanel_device->touch_descrip))
           {
               ptouchpanel_device->type = DEVICE_USED;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}

static int hct_touchpanel_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * touchpanel_type_info;    
    struct hct_touchpanel_device_dinfo * ptouchpanel_device;
    int flag = -1;

    seq_printf(se_f, "TOUCHPANEL----------------\t \n");

    list_for_each_entry(touchpanel_type_info,&hct_devices->type_list,hct_device){
            if(touchpanel_type_info->dev_type == ID_TOUCH_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(ptouchpanel_device,&touchpanel_type_info->hct_dinfo,touch_list){
                seq_printf(se_f, "      %20s\t\t:   ",ptouchpanel_device->touch_descrip);
                if(ptouchpanel_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                {
                    seq_printf(se_f, "  used \t\n");
                }
                
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
    return 0;
}

static void init_touchpanel_device(struct hct_touchpanel_device_dinfo *pdevice, struct tpd_driver_t * mTouch)
{
    memset(pdevice,0, sizeof(struct hct_touchpanel_device_dinfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->touch_list);
    strcpy(pdevice->touch_descrip,mTouch->tpd_device_name);
    pdevice->pdata=mTouch;
    
}


int hct_touchpanel_device_add(struct tpd_driver_t* mTouch, campatible_type isUsed)
{
    struct hct_type_info * touchpanel_type_info;    
    struct hct_touchpanel_device_dinfo * ptouchpanel_device;
    int flag = -1;
    int reterror=0;

     list_for_each_entry(touchpanel_type_info,&hct_devices->type_list,hct_device){
             if(touchpanel_type_info->dev_type == ID_TOUCH_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           touchpanel_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);

           if(touchpanel_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
       
           if(isUsed)
             init_device_type_info(touchpanel_type_info, mTouch, ID_TOUCH_TYPE);
           else
             init_device_type_info(touchpanel_type_info, NULL, ID_TOUCH_TYPE);
       }
       else
       {
           if(isUsed &&(touchpanel_type_info->current_used_dinfo!=NULL))
             printk("~~~~add lcm error , duplicated current used lcm \n");
       }
     
       flag =-1;
       
       list_for_each_entry(ptouchpanel_device,&touchpanel_type_info->hct_dinfo,touch_list){
              if(!strcmp(mTouch->tpd_device_name,ptouchpanel_device->touch_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             goto duplicated_faild;
       }
       else
       {
           ptouchpanel_device = kmalloc(sizeof(struct hct_touchpanel_device_dinfo), GFP_KERNEL);
            if(ptouchpanel_device == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto malloc_faid;
            }
           init_touchpanel_device(ptouchpanel_device,mTouch);
           ptouchpanel_device->type=isUsed;
           
           list_add(&ptouchpanel_device->hct_list, &hct_devices->dev_list);
           list_add(&ptouchpanel_device->touch_list,&touchpanel_type_info->hct_dinfo);
       }
 
          
          return 0;
          
     duplicated_faild:
  
         kfree(ptouchpanel_device);
     malloc_faid:
         
         printk("%s: error return: %x: ---\n",__func__,reterror);
         return reterror;
    
}


static int hct_accsensor_set_used(char * module_name, int pdata)
{
    struct hct_type_info * accsensor_type_info;    
    struct hct_accsensor_device_dinfo * paccsensor_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(accsensor_type_info,&hct_devices->type_list,hct_device){
             if(accsensor_type_info->dev_type == ID_ACCSENSOR_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(paccsensor_device,&accsensor_type_info->hct_dinfo,sensor_list){
           if(!strcmp(module_name,paccsensor_device->sensor_descrip))
           {
               paccsensor_device->type = DEVICE_USED;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}

static int hct_accsensor_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * accsensor_type_info;    
    struct hct_accsensor_device_dinfo * paccsensor_device;
    int flag = -1;

    seq_printf(se_f, "ACCSENSOR-----------------\t \n");
    
//#if defined(MTK_AUTO_DETECT_ACCELEROMETER)
#if 1
    list_for_each_entry(accsensor_type_info,&hct_devices->type_list,hct_device){
            if(accsensor_type_info->dev_type == ID_ACCSENSOR_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(paccsensor_device,&accsensor_type_info->hct_dinfo,sensor_list){
                seq_printf(se_f, "      %20s\t\t:   ",paccsensor_device->sensor_descrip);
                if(paccsensor_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                {
                    seq_printf(se_f, "  used \t\n");
                }
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
#else
    seq_printf(se_f, " warring :MTK_AUTO_DETECT_ACCELEROMETER need set to yes to display  \n");          
#endif
    return 0;
}

static void init_accsensor_device(struct hct_accsensor_device_dinfo *pdevice, struct acc_init_info * maccsensor)
{
    memset(pdevice,0, sizeof(struct hct_accsensor_device_dinfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->sensor_list);
    strcpy(pdevice->sensor_descrip,maccsensor->name);
    pdevice->pdata=maccsensor;
    
}


int hct_accsensor_device_add(struct acc_init_info* maccsensor, campatible_type isUsed)
{
    struct hct_type_info * accsensor_type_info;    
    struct hct_accsensor_device_dinfo * paccsensor_device;
    int flag = -1;
    int reterror=0;

     list_for_each_entry(accsensor_type_info,&hct_devices->type_list,hct_device){
             if(accsensor_type_info->dev_type == ID_ACCSENSOR_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           accsensor_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);

           if(accsensor_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
       
           if(isUsed)
             init_device_type_info(accsensor_type_info, maccsensor, ID_ACCSENSOR_TYPE);
           else
             init_device_type_info(accsensor_type_info, NULL, ID_ACCSENSOR_TYPE);
       }
       else
       {
           if(isUsed &&(accsensor_type_info->current_used_dinfo!=NULL))
             printk("~~~~add accsensor error , duplicated current used lcm \n");
       }
     
       flag =-1;
       
       list_for_each_entry(paccsensor_device,&accsensor_type_info->hct_dinfo,sensor_list){
              if(!strcmp(maccsensor->name,paccsensor_device->sensor_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             goto duplicated_faild;
       }
       else
       {
           paccsensor_device = kmalloc(sizeof(struct hct_accsensor_device_dinfo), GFP_KERNEL);
            if(paccsensor_device == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto malloc_faid;
            }
           init_accsensor_device(paccsensor_device,maccsensor);
           paccsensor_device->type=isUsed;
           
           list_add(&paccsensor_device->hct_list, &hct_devices->dev_list);
           list_add(&paccsensor_device->sensor_list,&accsensor_type_info->hct_dinfo);
       }
 
          
          return 0;
          
     duplicated_faild:
     
         kfree(paccsensor_device);
     malloc_faid:
         
         printk("%s: error return: %x: ---\n",__func__,reterror);
         return reterror;
    
}

//************************zhou
static int hct_alsps_set_used(char * module_name, int pdata)
{
    struct hct_type_info * alsps_type_info;    
    struct hct_alsps_device_dinfo * palsps_device;
    int flag = -1;
    int reterror=0;
    

     list_for_each_entry(alsps_type_info,&hct_devices->type_list,hct_device){
             if(alsps_type_info->dev_type == ID_ALSPS_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }
     
     if(flag == -1)  // this mean type is new
     {
        reterror = -1;
        goto error_notype;
     }

    flag =-1;

    list_for_each_entry(palsps_device,&alsps_type_info->hct_dinfo,sensor_list){
           if(!strcmp(module_name,palsps_device->sensor_descrip))
           {
               palsps_device->type = DEVICE_USED;
               flag =1;  // this mean device is ok
               break;
           }
       }
    
    if(flag == 1)
        return 0;
    
error_notype:
    return reterror;
    
}

static int hct_alsps_info_print(struct seq_file *se_f)
{
    
    struct hct_type_info * alsps_type_info;    
    struct hct_alsps_device_dinfo * palsps_device;
    int flag = -1;

    seq_printf(se_f, "ALSPS---------------------\t \n");
    
//#if defined(MTK_AUTO_DETECT_ACCELEROMETER)
#if 1
    list_for_each_entry(alsps_type_info,&hct_devices->type_list,hct_device){
            if(alsps_type_info->dev_type == ID_ALSPS_TYPE)
           {
               flag =1;  // this mean type is ok
               break;
           }
       }
    if(flag == 1)
    {
              list_for_each_entry(palsps_device,&alsps_type_info->hct_dinfo,sensor_list){
                seq_printf(se_f, "      %20s\t\t:   ",palsps_device->sensor_descrip);
                if(palsps_device->type == DEVICE_SUPPORTED)
                   seq_printf(se_f, "  supported \n");
                else
                {
                    seq_printf(se_f, "  used \t\n");
                }
                }
    }
    else
    {
        seq_printf(se_f, "        \t:   NONE    \n");          
    }
#else
    seq_printf(se_f, " warring :MTK_AUTO_DETECT_ACCELEROMETER need set to yes to display  \n");          
#endif
    return 0;
}

static void init_alsps_device(struct hct_alsps_device_dinfo *pdevice, struct alsps_init_info * malsps)
{
    memset(pdevice,0, sizeof(struct hct_alsps_device_dinfo));
    INIT_LIST_HEAD(&pdevice->hct_list);
    INIT_LIST_HEAD(&pdevice->sensor_list);
    strcpy(pdevice->sensor_descrip,malsps->name);
    pdevice->pdata=malsps;
    
}


int hct_alsps_device_add(struct alsps_init_info* malsps, campatible_type isUsed)
{
    struct hct_type_info * alsps_type_info;    
    struct hct_alsps_device_dinfo * palsps_device;
    int flag = -1;
    int reterror=0;

     list_for_each_entry(alsps_type_info,&hct_devices->type_list,hct_device){
             if(alsps_type_info->dev_type == ID_ALSPS_TYPE)
            {
                flag =1;  // this mean type is ok
                break;
            }
        }

       if(flag == -1)  // this mean type is new
       {
           alsps_type_info = kmalloc(sizeof(struct hct_type_info), GFP_KERNEL);

           if(alsps_type_info == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -1;
                  goto malloc_faid;
            }
       
           if(isUsed)
             init_device_type_info(alsps_type_info, malsps, ID_ALSPS_TYPE);
           else
             init_device_type_info(alsps_type_info, NULL, ID_ALSPS_TYPE);
       }
       else
       {
           if(isUsed &&(alsps_type_info->current_used_dinfo!=NULL))
             printk("~~~~add lcm error , duplicated current used lcm \n");
       }
     
       flag =-1;
       
       list_for_each_entry(palsps_device,&alsps_type_info->hct_dinfo,sensor_list){
              if(!strcmp(malsps->name,palsps_device->sensor_descrip))
              {
                  flag =1;  // this mean device is ok
                  break;
              }
          }
       if(flag ==1)
       {
             goto duplicated_faild;
       }
       else
       {
           palsps_device = kmalloc(sizeof(struct hct_alsps_device_dinfo), GFP_KERNEL);
            if(palsps_device == NULL)
            {
                  printk("lcm_alloc type info failed ~~~~ \n");
                  reterror = -2;
                  goto malloc_faid;
            }
           init_alsps_device(palsps_device,malsps);
           palsps_device->type=isUsed;
           
           list_add(&palsps_device->hct_list, &hct_devices->dev_list);
           list_add(&palsps_device->sensor_list,&alsps_type_info->hct_dinfo);
       }
 
          
          return 0;
          
     duplicated_faild:
    
         kfree(palsps_device);
     malloc_faid:
         
         printk("%s: error return: %x: ---\n",__func__,reterror);
         return reterror;
    
}
//************88zh
static int hct_set_device_used(int dev_type, char * module_name, int pdata)
{
    struct hct_type_info * type_info;    
    int ret_val = 0;
    
    list_for_each_entry(type_info,&hct_devices->type_list,hct_device){
        if(type_info->dev_type == dev_type)
        {
            if(type_info->set_used!=NULL)
                type_info->set_used(module_name,pdata);
        }
    }
    
    return ret_val;
}

int hct_set_touch_device_used(char * module_name, int pdata)
{
    int ret_val = 0;
    ret_val = hct_set_device_used(ID_TOUCH_TYPE,module_name,pdata);
    return ret_val;
}


int hct_set_camera_device_used(char * module_name, int pdata)
{
    int ret_val = 0;
    ret_val = hct_set_device_used(ID_CAMERA_TYPE,module_name,pdata);
    return ret_val;
}

int hct_set_accsensor_device_used(char * module_name, int pdata)
{
    int ret_val = 0;
    ret_val = hct_set_device_used(ID_ACCSENSOR_TYPE,module_name,pdata);
    return ret_val;
}
int hct_set_alsps_device_used(char * module_name, int pdata)
{
    int ret_val = 0;
    ret_val = hct_set_device_used(ID_ALSPS_TYPE,module_name,pdata);
    return ret_val;
}

int hct_device_dump(struct seq_file *m)
{
    
    struct hct_type_info * type_info;    
    seq_printf(m," \t\tDEVICE DUMP--begin\n");
    
    list_for_each_entry(type_info,&hct_devices->type_list,hct_device){
        if(type_info->type_print == NULL)
        {
            printk(" error !!! this type [%d] has no dump fun \n",type_info->dev_type);
        }
        else
            type_info->type_print(m);
    }
    seq_printf(m," \t\tDEVICE DUMP--end\n");
    
        return 0;
}

extern LCM_DRIVER* lcm_driver_list[];
extern unsigned int lcm_count;
extern char mtkfb_lcm_name[];
extern int  proc_hctinfo_init(void);
extern ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT kdSensorList[];
extern const LCM_DRIVER  *lcm_drv;
char * lcm_name;


    
static int hct_devices_probe(struct platform_device *pdev) 
{
        int temp;//index;
        int err =0;
        
        err = proc_hctinfo_init();
        if(err<0)
            goto proc_error;
        
        hct_devices = kmalloc(sizeof(struct hct_devices_info), GFP_KERNEL);
        
        if(hct_devices == NULL)
        {
             printk("%s: error probe becase of mem\n",__func__);
             return -1;
        }
        
        INIT_LIST_HEAD(&hct_devices->type_list);
        INIT_LIST_HEAD(&hct_devices->dev_list);
        mutex_init(&hct_devices->de_mutex);
        
        
        //lcm device_add init begin
        if(lcm_count ==1)
        {
            hct_lcm_device_add(lcm_driver_list[0], DEVICE_USED);
        }
        else
        {
        
          lcm_name=mtkfb_find_lcm_driver();

          
            for(temp= 0;temp< lcm_count;temp++)
            {
                //if(lcm_drv == lcm_driver_list[temp])
               
              if( (strcmp(lcm_name,lcm_driver_list[temp]->name))==0)
              {  
                    printk("zhou===LCM_DEVICE_SUPPORTED,lcm_name=%s\n",lcm_name);
                    hct_lcm_device_add(lcm_driver_list[temp], DEVICE_USED);
               }else{
                    printk("zhou===LCM_DEVICE_SUPPORTED,lcm_name=%s\n",lcm_driver_list[temp]->name);
                    hct_lcm_device_add(lcm_driver_list[temp], DEVICE_SUPPORTED);
                    
              }
           }   
        }
         //lcm device_add init end
         
         
         //cam sensor device_add init begin
       for(temp=0;;temp++)
       {
           if(kdSensorList[temp].SensorInit!=NULL)
           {
               hct_camera_device_add(&kdSensorList[temp], DEVICE_SUPPORTED);
           }
           else
            break;
       }
        //camera sensor device_add init end
 
        //memory sensor device_add init start       
      memset(memory_temp,0,(sizeof(struct memory_temp_info)*10)); 
      
       for(temp=0;temp<10;temp++)
       {    
          #if defined(CS_PART_NUMBER0)
              if(temp==0)
              {
                 strcpy(memory_temp[temp].cs_part_number, CS_PART_NUMBER0);
                 memory_temp[temp].support_num++;
                 hct_memory_device_add(&memory_temp[temp], DEVICE_SUPPORTED);
              }
          #endif
          #if defined(CS_PART_NUMBER1)
              if(temp==1)
              {
                 strcpy(memory_temp[temp].cs_part_number, CS_PART_NUMBER1);
                 memory_temp[temp].support_num++;
                 hct_memory_device_add(&memory_temp[temp], DEVICE_SUPPORTED);
              }          
          #endif    
          #if defined(CS_PART_NUMBER2)
              if(temp==2)
              {
                 strcpy(memory_temp[temp].cs_part_number, CS_PART_NUMBER2);
                 memory_temp[temp].support_num++;
                 hct_memory_device_add(&memory_temp[temp], DEVICE_SUPPORTED);
              } 
          #endif
          #if defined(CS_PART_NUMBER3)
              if(temp==3)
              {
                 strcpy(memory_temp[temp].cs_part_number, CS_PART_NUMBER3);
                 memory_temp[temp].support_num++;
                 hct_memory_device_add(&memory_temp[temp], DEVICE_SUPPORTED);
              }               
          #endif
          #if defined(CS_PART_NUMBER4)
              if(temp==4)
              {
                 strcpy(memory_temp[temp].cs_part_number, CS_PART_NUMBER4);
                 memory_temp[temp].support_num++;
                 hct_memory_device_add(&memory_temp[temp], DEVICE_SUPPORTED);
              } 
          #endif
          #if defined(CS_PART_NUMBER5)
              if(temp==5)
              {
                 strcpy(memory_temp[temp].cs_part_number, CS_PART_NUMBER5);
                 memory_temp[temp].support_num++;
                 hct_memory_device_add(&memory_temp[temp], DEVICE_SUPPORTED);
              } 
          #endif
          #if defined(CS_PART_NUMBER6)
              if(temp==6)
              {
                 strcpy(memory_temp[temp].cs_part_number, CS_PART_NUMBER6);
                 memory_temp[temp].support_num++;
                 hct_memory_device_add(&memory_temp[temp], DEVICE_SUPPORTED);
              } 
          #endif
          #if defined(CS_PART_NUMBER7)
              if(temp==7)
              {
                 strcpy(memory_temp[temp].cs_part_number, CS_PART_NUMBER7);
                 memory_temp[temp].support_num++;
                 hct_memory_device_add(&memory_temp[temp], DEVICE_SUPPORTED);
              } 
          #endif    
          #if defined(CS_PART_NUMBER8)
              if(temp==8)
              {
                 strcpy(memory_temp[temp].cs_part_number, CS_PART_NUMBER8);
                 memory_temp[temp].support_num++;
                 hct_memory_device_add(&memory_temp[temp], DEVICE_SUPPORTED);
              } 
          #endif   
          #if defined(CS_PART_NUMBER9)
              if(temp==9)
              {
                 strcpy(memory_temp[temp].cs_part_number, CS_PART_NUMBER9);
                 memory_temp[temp].support_num++;
                 hct_memory_device_add(&memory_temp[temp], DEVICE_SUPPORTED);
              }                                          
          #endif                                                        
      }
       
      //memory sensor device_add init end 

	return 0;
 proc_error:
       return err;
}
/*----------------------------------------------------------------------------*/
static int hct_devices_remove(struct platform_device *pdev)
{
	return 0;
}


/*----------------------------------------------------------------------------*/
static struct platform_driver hct_devices_driver = {
	.probe      = hct_devices_probe,
	.remove     = hct_devices_remove,    
	.driver     = {
		.name  = "hct_devices",
		//.owner = THIS_MODULE,
	}
};

static struct platform_device hct_dev = {
	.name		  = "hct_devices",
	.id		  = -1,
};


/*----------------------------------------------------------------------------*/
static int __init hct_devices_init(void)
{
    int retval = 0;
    
    retval = platform_device_register(&hct_dev);
    if (retval != 0){
        return retval;
    }

    if(platform_driver_register(&hct_devices_driver))
    {
    	printk("failed to register driver");
    	return -ENODEV;
    }
    
    return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit hct_devices_exit(void)
{
	platform_driver_unregister(&hct_devices_driver);
}
/*----------------------------------------------------------------------------*/
module_init(hct_devices_init);
module_exit(hct_devices_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Jay Zhou");
MODULE_DESCRIPTION("HCT DEVICE INFO");
MODULE_LICENSE("GPL");




