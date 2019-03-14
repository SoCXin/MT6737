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

#include <linux/spi/spi.h>
//#include <linux/spi/sunwave_dev.h>

#include <asm/uaccess.h>
#include "sunwavecorp.h"

void sunwavecorp_register_finger(sunwave_sensor_t*   sunwave)
{
    extern void mt6797_dts_register_finger(sunwave_sensor_t* d);
    mt6797_dts_register_finger(sunwave);
}
EXPORT_SYMBOL_GPL(sunwavecorp_register_finger);


/**
 *if you need register platform resources   use this fuction
 */
void sunwavecorp_register_platform(void)
{
}
EXPORT_SYMBOL_GPL(sunwavecorp_register_platform);