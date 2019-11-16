#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/fb.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/timer.h> 
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#include "fy6203.h"

static int breath_mode=0;

 int fy6203_breath_ready(void)
{
	//first,set low
	pinctrl_select_state(fy6203ctrl, en_set_low);
	udelay(500);
	//set high
	//printk("[**]fy6203_en high \n");
	pinctrl_select_state(fy6203ctrl, en_set_high);
	udelay(10);
	//printk("[**]fy6203_breath_ready \n");

	return 0;
}
 int fy6203_en_pulse_count(int count)
{
	int i=0;

  if(count>15)
     count=8; //count max value limit
     
	for(i=0;i<count;i++)
	{
	pinctrl_select_state(fy6203ctrl, en_set_low);
		udelay(5);
	pinctrl_select_state(fy6203ctrl, en_set_high);
		udelay(2);
	}
	//printk("[**]fy6203_en_pulse_count %d \n",i);
	
	return 0;
}

 int fy6203_breath_poweroff(void)
{
	pinctrl_select_state(fy6203ctrl, en_set_low);
	udelay(500);
	//printk("[**]fy6203_power_off \n");

	return 0;
}
int breathled_enable_ext(int mode)
{
	  if(mode==0)
	  {
		   fy6203_breath_poweroff();
	  }else{
	  
		   fy6203_breath_ready();
		   fy6203_en_pulse_count(mode); 
		   printk("[**]fy6203 enable pulse \n");
	  }  
    return mode;
}
EXPORT_SYMBOL(breathled_enable_ext);

struct class *breathled_class;
struct device *breathled_dev;

static ssize_t breathled_enable_show(struct device *dev, struct device_attribute *attr, char *buf) 
{           
     return scnprintf(buf, PAGE_SIZE, "%d\n", breath_mode);
}

static ssize_t breathled_enable_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    int mode = 0;
    if(buf != NULL && size != 0)
    {
        mode = (int)simple_strtoul(buf, NULL, 0);
    }
    breath_mode=mode;
#if 1  
	printk("[breathled]fy6203 mode change!! mode=%d\n", mode);
	  if(mode==0)
	  {
		   fy6203_breath_poweroff();
	  }else{
	  
		   fy6203_breath_ready();
		   fy6203_en_pulse_count(mode); 
		   //printk("[**]fy6203 enable pulse \n");
	  }
#endif	    
    return size;
}

static DEVICE_ATTR(breathled_enable, 0644, breathled_enable_show, breathled_enable_store);
static int create_node_file(void)
{

	breathled_class = class_create(THIS_MODULE, "breathled");
	breathled_dev = device_create(breathled_class,NULL, 0, NULL,  "breathled");
    device_create_file(breathled_dev, &dev_attr_breathled_enable);
	return 0;

}

static int FRAM_probe(struct platform_device *pdev)
{
	int ret = 0;

    printk(" fy6203 FRAM_probe in!\n ");
    // dts read
    pdev->dev.of_node = of_find_compatible_node(NULL, NULL, "mediatek,breathled");

	  fy6203ctrl = devm_pinctrl_get(&pdev->dev);
                
	en_set_low = pinctrl_lookup_state(fy6203ctrl, "en_set_0");
	if (IS_ERR(en_set_low)) {
		ret = PTR_ERR(en_set_low);
		pr_debug("%s : pinctrl err, fy6203 en_set_0 \n", __func__);
	}	                 
                 
	en_set_high = pinctrl_lookup_state(fy6203ctrl, "en_set_1");
	if (IS_ERR(en_set_high)) {
		ret = PTR_ERR(en_set_high);
		pr_debug("%s : pinctrl err, fy6203 en_set_1 \n", __func__);
	}	                              


   create_node_file();

	return ret;
}

static int FRAM_remove(struct platform_device *pdev)
{
	return 0;
}

static int FRAM_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int FRAM_resume(struct platform_device *pdev)
{
	return 0;
}

/* platform structure */
static struct platform_driver fy6203_Driver = {
	.probe = FRAM_probe,
	.remove = FRAM_remove,
	.suspend = FRAM_suspend,
	.resume = FRAM_resume,
	.driver = {
		   .name = PLATFORM_DRIVER_NAME,
		   .owner = THIS_MODULE,
		   }
};

static struct platform_device fy6203_device = {
	.name = PLATFORM_DRIVER_NAME,
	.id = 0,
	.dev = {}
};

static int __init fy6203_i2C_init(void)
{
	
	if (platform_device_register(&fy6203_device)) {
		printk("failed to register fy6203 driver\n"); 
		return -ENODEV;
	}

	if (platform_driver_register(&fy6203_Driver)) {
		printk("Failed to register fy6203 driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit fy6203_i2C_exit(void)
{

	platform_driver_unregister(&fy6203_Driver);
}

module_init(fy6203_i2C_init);
module_exit(fy6203_i2C_exit);

MODULE_DESCRIPTION("MTK MT6735 breath fy6203 driver");
MODULE_AUTHOR("zhoucheng <cheng.zhou@runyee.com.cn>");
MODULE_LICENSE("GPL");
