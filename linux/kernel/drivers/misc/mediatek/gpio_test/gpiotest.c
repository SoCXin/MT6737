#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

struct class *iotest_class;
struct device *iotest_dev;
#define PLATFORM_DRIVER_NAME "gpiotest"

struct device_node	*iotest_of_node; 
int iotest_gpio_2;
int iotest_gpio_3; 
int iotest_gpio_4;
int iotest_gpio_5;
int iotest_gpio_6;
int iotest_gpio_8;
int iotest_gpio_9;
int iotest_gpio_11;
int iotest_gpio_12;
int iotest_gpio_53;
int iotest_gpio_54;
int iotest_gpio_57;
int iotest_gpio_58;
int iotest_gpio_59;
int iotest_gpio_60;
int iotest_gpio_71;
int iotest_gpio_76;
int iotest_gpio_77;
int iotest_gpio_85;
int iotest_gpio_86;



/* test interface */
static ssize_t runyee_gpiotest_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    int enable = 0;
	
    if(buf != NULL && size != 0)
    {
        enable = (int)simple_strtoul(buf, NULL, 0);
    }
	
    if(enable)
    {
		gpio_request(iotest_gpio_2, "iotest_gpio_2"); 
		gpio_direction_output(iotest_gpio_2,1); 
		gpio_request(iotest_gpio_3, "iotest_gpio_3"); 
		gpio_direction_output(iotest_gpio_3,1);
		gpio_request(iotest_gpio_4, "iotest_gpio_4"); 
		gpio_direction_output(iotest_gpio_4,1); 
		gpio_request(iotest_gpio_5, "iotest_gpio_5"); 
		gpio_direction_output(iotest_gpio_5,1); 
		gpio_request(iotest_gpio_6, "iotest_gpio_6"); 
		gpio_direction_output(iotest_gpio_6,1); 
		gpio_request(iotest_gpio_8, "iotest_gpio_8"); 
		gpio_direction_output(iotest_gpio_8,1); 
		gpio_request(iotest_gpio_9, "iotest_gpio_9"); 
		gpio_direction_output(iotest_gpio_9,1);	
		gpio_request(iotest_gpio_11, "iotest_gpio_11"); 
		gpio_direction_output(iotest_gpio_11,1); 
		gpio_request(iotest_gpio_12, "iotest_gpio_12"); 
		gpio_direction_output(iotest_gpio_12,1);	
		gpio_request(iotest_gpio_53, "iotest_gpio_53"); 
		gpio_direction_output(iotest_gpio_53,1);	
		gpio_request(iotest_gpio_54, "iotest_gpio_54"); 
		gpio_direction_output(iotest_gpio_54,1);	
		gpio_request(iotest_gpio_57, "iotest_gpio_57"); 
		gpio_direction_output(iotest_gpio_57,1);
		gpio_request(iotest_gpio_58, "iotest_gpio_58"); 
		gpio_direction_output(iotest_gpio_58,1);	
		gpio_request(iotest_gpio_59, "iotest_gpio_59"); 
		gpio_direction_output(iotest_gpio_59,1);	
		gpio_request(iotest_gpio_60, "iotest_gpio_60"); 
		gpio_direction_output(iotest_gpio_60,1);	
		gpio_request(iotest_gpio_71, "iotest_gpio_71"); 
		gpio_direction_output(iotest_gpio_71,1);	
		gpio_request(iotest_gpio_76, "iotest_gpio_76"); 
		gpio_direction_output(iotest_gpio_76,1);		
		gpio_request(iotest_gpio_77, "iotest_gpio_77"); 
		gpio_direction_output(iotest_gpio_77,1);
		gpio_request(iotest_gpio_85, "iotest_gpio_85"); 
		gpio_direction_output(iotest_gpio_85,1);
		gpio_request(iotest_gpio_86, "iotest_gpio_86"); 
		gpio_direction_output(iotest_gpio_86,1);
    }
    else
    {
		gpio_request(iotest_gpio_2, "iotest_gpio_2"); 
		gpio_direction_output(iotest_gpio_2,0); 
		gpio_request(iotest_gpio_3, "iotest_gpio_3"); 
		gpio_direction_output(iotest_gpio_3,0);
		gpio_request(iotest_gpio_4, "iotest_gpio_4"); 
		gpio_direction_output(iotest_gpio_4,0); 
		gpio_request(iotest_gpio_5, "iotest_gpio_5"); 
		gpio_direction_output(iotest_gpio_5,0); 
		gpio_request(iotest_gpio_6, "iotest_gpio_6"); 
		gpio_direction_output(iotest_gpio_6,0); 
		gpio_request(iotest_gpio_8, "iotest_gpio_8"); 
		gpio_direction_output(iotest_gpio_8,0); 
		gpio_request(iotest_gpio_9, "iotest_gpio_9"); 
		gpio_direction_output(iotest_gpio_9,0);	
		gpio_request(iotest_gpio_11, "iotest_gpio_11"); 
		gpio_direction_output(iotest_gpio_11,0); 
		gpio_request(iotest_gpio_12, "iotest_gpio_12"); 
		gpio_direction_output(iotest_gpio_12,0);	
		gpio_request(iotest_gpio_53, "iotest_gpio_53"); 
		gpio_direction_output(iotest_gpio_53,0);	
		gpio_request(iotest_gpio_54, "iotest_gpio_54"); 
		gpio_direction_output(iotest_gpio_54,0);	
		gpio_request(iotest_gpio_57, "iotest_gpio_57"); 
		gpio_direction_output(iotest_gpio_57,0);
		gpio_request(iotest_gpio_58, "iotest_gpio_58"); 
		gpio_direction_output(iotest_gpio_58,0);	
		gpio_request(iotest_gpio_59, "iotest_gpio_59"); 
		gpio_direction_output(iotest_gpio_59,0);	
		gpio_request(iotest_gpio_60, "iotest_gpio_60"); 
		gpio_direction_output(iotest_gpio_60,0);	
		gpio_request(iotest_gpio_71, "iotest_gpio_71"); 
		gpio_direction_output(iotest_gpio_71,0);	
		gpio_request(iotest_gpio_76, "iotest_gpio_76"); 
		gpio_direction_output(iotest_gpio_76,0);		
		gpio_request(iotest_gpio_77, "iotest_gpio_77"); 
		gpio_direction_output(iotest_gpio_77,0);
		gpio_request(iotest_gpio_85, "iotest_gpio_85"); 
		gpio_direction_output(iotest_gpio_85,0);
		gpio_request(iotest_gpio_86, "iotest_gpio_86"); 
		gpio_direction_output(iotest_gpio_86,0);
    }
	
    return size;
}
static DEVICE_ATTR(runyee_gpiotest, 0644, NULL, runyee_gpiotest_store);


/* platform structure */
/**************************get hdmi dts pams***************************/
void iotest_get_dts(struct platform_device *pdev)
{
	// dts read
	iotest_of_node = of_find_compatible_node(NULL, NULL, "mediatek,gpiotest");

	iotest_gpio_2=of_get_named_gpio(iotest_of_node, "iotest_gpio_2", 0);
	iotest_gpio_3=of_get_named_gpio(iotest_of_node, "iotest_gpio_3", 0);
	iotest_gpio_4=of_get_named_gpio(iotest_of_node, "iotest_gpio_4", 0);
	iotest_gpio_5=of_get_named_gpio(iotest_of_node, "iotest_gpio_5", 0);
	iotest_gpio_6=of_get_named_gpio(iotest_of_node, "iotest_gpio_6", 0);
	iotest_gpio_8=of_get_named_gpio(iotest_of_node, "iotest_gpio_8", 0);
	iotest_gpio_9=of_get_named_gpio(iotest_of_node, "iotest_gpio_9", 0);
	iotest_gpio_11=of_get_named_gpio(iotest_of_node, "iotest_gpio_11", 0);
	iotest_gpio_12=of_get_named_gpio(iotest_of_node, "iotest_gpio_12", 0);
	iotest_gpio_53=of_get_named_gpio(iotest_of_node, "iotest_gpio_53", 0);
	iotest_gpio_54=of_get_named_gpio(iotest_of_node, "iotest_gpio_54", 0);
	iotest_gpio_57=of_get_named_gpio(iotest_of_node, "iotest_gpio_57", 0);
	iotest_gpio_58=of_get_named_gpio(iotest_of_node, "iotest_gpio_58", 0);
	iotest_gpio_59=of_get_named_gpio(iotest_of_node, "iotest_gpio_59", 0);
	iotest_gpio_60=of_get_named_gpio(iotest_of_node, "iotest_gpio_60", 0);
	iotest_gpio_71=of_get_named_gpio(iotest_of_node, "iotest_gpio_71", 0);
	iotest_gpio_76=of_get_named_gpio(iotest_of_node, "iotest_gpio_76", 0);
	iotest_gpio_77=of_get_named_gpio(iotest_of_node, "iotest_gpio_77", 0);
	iotest_gpio_85=of_get_named_gpio(iotest_of_node, "iotest_gpio_85", 0);
	iotest_gpio_86=of_get_named_gpio(iotest_of_node, "iotest_gpio_86", 0);

	//default set low
	gpio_request(iotest_gpio_2, "iotest_gpio_2"); 
	gpio_direction_output(iotest_gpio_2,0);	
	gpio_request(iotest_gpio_3, "iotest_gpio_3"); 
	gpio_direction_output(iotest_gpio_3,0);
	gpio_request(iotest_gpio_4, "iotest_gpio_4"); 
	gpio_direction_output(iotest_gpio_4,0);	
	gpio_request(iotest_gpio_5, "iotest_gpio_5"); 
	gpio_direction_output(iotest_gpio_5,0);	
	gpio_request(iotest_gpio_6, "iotest_gpio_6"); 
	gpio_direction_output(iotest_gpio_6,0);	
	gpio_request(iotest_gpio_8, "iotest_gpio_8"); 
	gpio_direction_output(iotest_gpio_8,0);	
	gpio_request(iotest_gpio_9, "iotest_gpio_9"); 
	gpio_direction_output(iotest_gpio_9,0);	
	gpio_request(iotest_gpio_11, "iotest_gpio_11"); 
	gpio_direction_output(iotest_gpio_11,0);	
	gpio_request(iotest_gpio_12, "iotest_gpio_12"); 
	gpio_direction_output(iotest_gpio_12,0);	
	gpio_request(iotest_gpio_53, "iotest_gpio_53"); 
	gpio_direction_output(iotest_gpio_53,0);	
	gpio_request(iotest_gpio_54, "iotest_gpio_54"); 
	gpio_direction_output(iotest_gpio_54,0);	
	gpio_request(iotest_gpio_57, "iotest_gpio_57"); 
	gpio_direction_output(iotest_gpio_57,0);
	gpio_request(iotest_gpio_58, "iotest_gpio_58"); 
	gpio_direction_output(iotest_gpio_58,0);	
	gpio_request(iotest_gpio_59, "iotest_gpio_59"); 
	gpio_direction_output(iotest_gpio_59,0);	
	gpio_request(iotest_gpio_60, "iotest_gpio_60"); 
	gpio_direction_output(iotest_gpio_60,0);	
	gpio_request(iotest_gpio_71, "iotest_gpio_71"); 
	gpio_direction_output(iotest_gpio_71,0);	
	gpio_request(iotest_gpio_76, "iotest_gpio_76"); 
	gpio_direction_output(iotest_gpio_76,0);		
	gpio_request(iotest_gpio_77, "iotest_gpio_77"); 
	gpio_direction_output(iotest_gpio_77,0);
	gpio_request(iotest_gpio_85, "iotest_gpio_85"); 
	gpio_direction_output(iotest_gpio_85,0);
	gpio_request(iotest_gpio_86, "iotest_gpio_86"); 
	gpio_direction_output(iotest_gpio_86,0);

}

static int platform_iotest_probe(struct platform_device *pdev)
{
	printk(" iotest_probe in!\n ");	

	iotest_get_dts(pdev);
	return 0;
}

static int platform_iotest_remove(struct platform_device *pdev)
{
	return 0;
}

static int platform_iotest_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int platform_iotest_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver g_iotest_Driver = {
	.probe = platform_iotest_probe,
	.remove = platform_iotest_remove,
	.suspend = platform_iotest_suspend,
	.resume = platform_iotest_resume,
	.driver = {
		   .name = PLATFORM_DRIVER_NAME,
		   .owner = THIS_MODULE,
		   }
};

static struct platform_device g_iotest_device = {
	.name = PLATFORM_DRIVER_NAME,
	.id = 0,
	.dev = {}
};

static int __init platform_iotest_init(void)
{
#if 1
	iotest_class = class_create(THIS_MODULE, "gpiotest");
	iotest_dev = device_create(iotest_class,NULL, 0, NULL,  "gpiotest");
    device_create_file(iotest_dev, &dev_attr_runyee_gpiotest);		

	if (platform_device_register(&g_iotest_device)) {
		printk("failed to register iotest device\n");
		return -1;
	}

	if (platform_driver_register(&g_iotest_Driver)) {
		printk("failed to register iotest driver\n");
		return -1;
	}
#endif
	return 0;
}

static void __exit platform_iotest_exit(void)
{
	platform_driver_unregister(&g_iotest_Driver);
}

module_init(platform_iotest_init);
module_exit(platform_iotest_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("board iotest driver");
MODULE_AUTHOR("jst<aren.jiang@runyee.com.cn>");
