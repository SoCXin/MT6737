
#include <hwmsensor.h>
#include <hwmsen_dev.h>
#include <sensors_io.h>
#include <hwmsen_helper.h>

#include <accel.h>
#include <alsps.h>
#include <mag.h>
#include <gyroscope.h>

//#include <asm/io.h>
//#include <cust_eint.h>
//#include <cust_alsps.h>

#include <linux/list.h>
#include <linux/seq_file.h>


//#include <mach/mt_typedefs.h>
//#include <mach/mt_gpio.h>
//#include <mach/mt_pm_ldo.h>

//#include "cust_gpio_usage.h"
#include "lcm_drv.h"
#include "kd_imgsensor_define.h"
#include "tpd.h"

#define ID_MEMORY_TYPE         1
#define ID_LCM_TYPE           (ID_MEMORY_TYPE+1)
#define ID_CAMERA_TYPE        (ID_LCM_TYPE+1)
#define ID_TOUCH_TYPE         (ID_CAMERA_TYPE+1)
#define ID_ACCSENSOR_TYPE     (ID_TOUCH_TYPE+1)
#define ID_ALSPS_TYPE         (ID_ACCSENSOR_TYPE+1)

typedef enum 
{ 
    DEVICE_SUPPORTED = 0,        
    DEVICE_USED = 1,
}campatible_type;

extern char* mtkfb_find_lcm_driver(void);

extern int hct_set_touch_device_used(char * module_name, int pdata);
extern int hct_set_camera_device_used(char * module_name, int pdata);
extern int hct_set_accsensor_device_used(char * module_name, int pdata);
extern int hct_set_alsps_device_used(char * module_name, int pdata);

extern int hct_touchpanel_device_add(struct tpd_driver_t* mTouch, campatible_type isUsed);
extern int hct_camera_device_add(ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT* mCamera, campatible_type isUsed);
extern int hct_accsensor_device_add(struct acc_init_info* maccsensor, campatible_type isUsed);
extern int hct_alsps_device_add(struct alsps_init_info* malsps, campatible_type isUsed);
