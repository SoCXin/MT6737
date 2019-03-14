#ifndef  __FINGER_H__
#define __FINGER_H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/wakelock.h>
#include <linux/input.h>

#include "config.h"

#if __SUNWAVE_SCREEN_LOCK_EN

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>

#else

#include <linux/fb.h>
#include <linux/notifier.h>
#endif

#endif

extern int sunwave_debug_level;
#define  sw_dbg(fmt, args...)     do{if(sunwave_debug_level){  printk("sunwave: " _DRV_TAG_ " %d " fmt " \n",__LINE__, ##args);}}while(0)
#define  sw_info(fmt, args...)    do{printk("sunwave: " _DRV_TAG_ " %d " fmt " \n",__LINE__, ##args);}while(0)
#define  sw_err(fmt, args...)     do{printk(KERN_ERR"sunwave: error: " _DRV_TAG_ " %d " fmt " \n",__LINE__, ##args);}while(0)

struct finger {
    int                              ver;
    int                              attribute;
    unsigned long                    gpio_rst;
    unsigned long                    gpio_irq;
    int                             write_then_read;//1 use write_then_read
    int                             (*irq_hander)(struct spi_device** spi);
    int                             (*irq_request)(struct spi_device**    spi);
    void                            (*irq_free)(struct spi_device**   spi);
    int                             (*init)(struct spi_device**    spi);
    int                             (*reset)(struct spi_device**  spi);
    int                             (*exit)(struct spi_device**    spi);
    int                             (*speed)(struct spi_device**    spi, unsigned int speed);
};
typedef struct finger finger_t;

/***************************************************************
*attribute
*0x00000001       is tee
*0x00000002       spi fifo mode
*
***************************************************************/
#define DEVICE_ATTRIBUTE_NONE           0x00000000
#define DEVICE_ATTRIBUTE_IS_TEE         0x00000001
#define DEVICE_ATTRIBUTE_SPI_FIFO       0x00000002


struct sunwave_fp_platform_data {
    int irq_gpio_number;
    int reset_gpio_number;
    const char* vdd_name;
};
struct sunwave_sensor {
    dev_t                   devt;
    spinlock_t              spi_lock;
    struct spi_device*      spi;
    struct list_head        device_entry;
    struct mutex            buf_lock;
    struct work_struct      irq_work;
    struct wake_lock        wakelock;
    unsigned                users;
    u8*                     buffer;
    unsigned int            standby_irq;
    struct input_dev*       input; //report power key event
    finger_t*               finger;

#if __SUNWAVE_SCREEN_LOCK_EN

#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend;
#else
    struct notifier_block notifier;
#endif

#endif //__SUNWAVE_SCREEN_LOCK_EN

};

typedef struct sunwave_sensor   sunwave_sensor_t;


static inline  sunwave_sensor_t*  spi_to_sunwave(struct spi_device** spidev)
{
    return  container_of(spidev,    sunwave_sensor_t, spi);;
}

extern void  sunwavecorp_register_finger(sunwave_sensor_t*);
extern void sunwavecorp_register_platform(void);
extern int create_input_device(sunwave_sensor_t* sunwave);
extern void release_input_device(sunwave_sensor_t* sunwave);
extern void  sunwave_key_report(sunwave_sensor_t* sunwave, int value, int down_up);
extern void  sunwave_wakeupSys(sunwave_sensor_t* sunwave);
extern int  sunwave_irq_request(sunwave_sensor_t* sunwave);
extern void  sunwave_irq_free(sunwave_sensor_t* sunwave);

extern irqreturn_t sunwave_irq_hander_default_process(int irq, void* dev);
extern void sunwave_irq_hander_default_process_2(void);

extern sunwave_sensor_t* get_current_sunwave(void);
#endif
