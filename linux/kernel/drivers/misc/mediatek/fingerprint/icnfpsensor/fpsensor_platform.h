/*++


 File Name:    fpsensor_platform.h
 Author:       zmtian
 Date :        11,23,2015
 Version:      1.0[.revision]

 History :
     Change logs.
 --*/


#ifndef LINUX_SPI_FPSENSOR_PLATFORM_H
#define LINUX_SPI_FPSENSOR_PLATFORM_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/ioctl.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/poll.h>
#include <linux/sort.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/spi/spidev.h>
#include <linux/spi/spi.h>
#include <linux/usb.h>
#include <linux/usb/ulpi.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/power_supply.h>

//#include <cust_gpio_usage.h>

#include <asm/uaccess.h>
#include <linux/ktime.h>

#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/completion.h>
#include <mt_spi.h>
#include <mt-plat/mt_gpio.h>
//#include <mach/mt_spi.h>
//#include <mach/mt_gpio.h>
//#include <mach/mt_clkmgr.h>

#define     FPSENSOR_MANUAL_CS                  1
#define     FPSENSOR_SPI_BUS_DYNAMIC            1
#define     FPSENSOR_SOFT_RST_ENABLE            0
#define     FPSENSOR_STILL_WAIT_INVALID_IRQ     1

#define     FPSENSOR_FIX_FREQ                   6000u

#define     FPSENSOR_INT_PIN          -EINVAL//(1 | 0x80000000)//(GPIO1 | 0x80000000) /*GPIO7*/   //(GPIO4 | 0x80000000)
#define     FPSENSOR_IRQ_NUM          289//CUST_EINT_FP_NUM
#define     FPSENSOR_RST_PIN          1  // not gpio, only macro       //(63 | 0x80000000)  /*GPIO19*/   //(GPIO44 | 0x80000000)


#define     FPSENSOR_SPI_CS_PIN       2 // not gpio, only macro         //65//(65 | 0x80000000)
#define     FPSENSOR_SPI_MO_PIN       3
#define     FPSENSOR_SPI_MI_PIN       4
#define     FPSENSOR_SPI_CK_PIN       5
/*
#define SUPPLY_1V8          1800000UL
#define SUPPLY_3V3          3300000UL
#define SUPPLY_SPI_MIN      SUPPLY_1V8
#define SUPPLY_SPI_MAX      SUPPLY_1V8

#define SUPPLY_IO_MIN       SUPPLY_1V8
#define SUPPLY_IO_MAX       SUPPLY_1V8

#define SUPPLY_ANA_MIN      SUPPLY_1V8
#define SUPPLY_ANA_MAX      SUPPLY_1V8

#define SUPPLY_TX_MIN       SUPPLY_3V3
#define SUPPLY_TX_MAX       SUPPLY_3V3

#define SUPPLY_SPI_REQ_CURRENT  10U
#define SUPPLY_IO_REQ_CURRENT   6000U
#define SUPPLY_ANA_REQ_CURRENT  6000U
*/

extern struct mt_chip_conf fpsensor_spi_conf_mt65xx;

extern int fpsensor_data_init(fpsensor_data_t *fpsensor);
extern int fpsensor_get_of_pdata(struct device *dev, struct fpsensor_platform_data *pdata);
extern int fpsensor_param_init(fpsensor_data_t *fpsensor, struct fpsensor_platform_data *pdata);
extern int fpsensor_supply_init(fpsensor_data_t *fpsensor);
extern int fpsensor_regulator_configure(fpsensor_data_t *fpsensor);
extern int fpsensor_regulator_release(fpsensor_data_t *fpsensor);
extern int fpsensor_regulator_set(fpsensor_data_t *fpsensor, bool enable);
extern int fpsensor_reset_init(fpsensor_data_t *fpsensor, struct fpsensor_platform_data *pdata);
extern int fpsensor_gpio_wirte(int gpio, int value);
extern int fpsensor_gpio_read(int gpio);
extern int fpsensor_gpio_valid(int gpio);
extern int fpsensor_gpio_free(int gpio);
extern int fpsensor_irq_init(fpsensor_data_t *fpsensor, struct fpsensor_platform_data *pdata);
extern int fpsensor_irq_disable(fpsensor_data_t *fpsensor);
extern int fpsensor_irq_enable(fpsensor_data_t *fpsensor);
extern int fpsensor_irq_free(fpsensor_data_t *fpsensor);
extern int fpsensor_spi_setup(fpsensor_data_t *fpsensor, struct fpsensor_platform_data *pdata);
extern int fpsensor_reg_access(fpsensor_data_t *fpsensor, fpsensor_reg_access_t *reg_data);
extern int fpsensor_cmd(fpsensor_data_t *fpsensor, fpsensor_cmd_t cmd, u8 wait_irq_mask);
extern int fpsensor_fetch_image(fpsensor_data_t *fpsensor,
                                u8 *buffer,
                                int offset,
                                size_t image_size_bytes,
                                size_t buff_size);

extern int plat_power(int power);
extern int fpsensor_spidev_dts_init(fpsensor_data_t *fpsensor);
extern void fpsensor_gpio_output_dts(int gpio, int level);


#endif /* LINUX_SPI_FPSENSOR_PLATFORM_H */

