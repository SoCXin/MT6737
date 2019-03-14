/*++

 File Name:    fpsensor_main.c
 Author:       zmtian
 Date :        11,23,2015
 Version:      1.0[.revision]
 History :
     Change logs.
 --*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/spi/spi.h>
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

#include <linux/of.h>
#include "fpsensor.h"
#include "fpsensor_common.h"
#include "fpsensor_regs.h"
#include "fpsensor_input.h"
#include "fpsensor_capture.h"
#include "fpsensor_platform.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("zmtian");
MODULE_DESCRIPTION("FPSENSOR touch sensor driver.");



/* -------------------------------------------------------------------- */
/* fpsensor sensor commands and registers                */
/* -------------------------------------------------------------------- */
typedef enum
{
    FPSENSOR_ERROR_REG_BIT_FIFO_UNDERFLOW = 1 << 0
} fpsensor_error_reg_t;



/* -------------------------------------------------------------------- */
/* global variables                         */
/* -------------------------------------------------------------------- */
static int fpsensor_device_count;


/* -------------------------------------------------------------------- */
/* fpsensor data types                           */
/* -------------------------------------------------------------------- */
struct fpsensor_attribute
{
    struct device_attribute attr;
    size_t offset;
};

enum
{
    FPSENSOR_WORKER_IDLE_MODE = 0,
    FPSENSOR_WORKER_CAPTURE_MODE,
    FPSENSOR_WORKER_INPUT_MODE,
    FPSENSOR_WORKER_EXIT
};


/* -------------------------------------------------------------------- */
/* fpsensor driver constants                     */
/* -------------------------------------------------------------------- */
#define FPSENSOR_CLASS_NAME                      "fpsensor"
#define FPSENSOR_WORKER_THREAD_NAME              "fpsensor_worker"


#define FPSENSOR_BASE_IO_OFFSET                   0
#define FPSENSOR_IOCTL_MAGIC_NO                   0xFC
#define FPSENSOR_IOCTL_BASE(x)                    FPSENSOR_BASE_IO_OFFSET+x

#define FPSENSOR_IOCTL_WRITE_ADC_GAIN            _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(20), unsigned int)
#define FPSENSOR_IOCTL_READ_ADC_GAIN             _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(21), unsigned int)
#define FPSENSOR_IOCTL_WRITE_ADC_SHIFT           _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(22), unsigned int)
#define FPSENSOR_IOCTL_READ_ADC_SHIFT            _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(23), unsigned int)
#define FPSENSOR_IOCTL_WRITE_PXL_CTRL            _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(24), unsigned int)
#define FPSENSOR_IOCTL_READ_PXL_CTRL             _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(25), unsigned int)
#define FPSENSOR_IOCTL_WRITE_ET1                 _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(26), unsigned int)
#define FPSENSOR_IOCTL_READ_ET1                  _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(27), unsigned int)
#define FPSENSOR_IOCTL_SET_MUX                   _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(28), unsigned int)
#define FPSENSOR_IOCTL_SET_CAPTURE_COUNT         _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(29), unsigned int)
#define FPSENSOR_IOCTL_SET_CAPTURE_MODE          _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(30), unsigned int)
#define FPSENSOR_IOCTL_SET_ROW_START             _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(31), unsigned int)
#define FPSENSOR_IOCTL_SET_ROW_COUNT             _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(32), unsigned int)
#define FPSENSOR_IOCTL_SET_COL_START             _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(33), unsigned int)
#define FPSENSOR_IOCTL_SET_COL_GROUPS            _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(34), unsigned int)
#define FPSENSOR_IOCTL_GET_CAPTURE_STATE         _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(35), unsigned int)
#define FPSENSOR_IOCTL_WRITE_TINY_CAPTURE        _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(36), unsigned int)
#define FPSENSOR_IOCTL_READ_TINY_CAPTURE         _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(37), unsigned int)
#define FPSENSOR_IOCTL_GET_FINGER_STATUS         _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(38), unsigned int)
#define FPSENSOR_IOCTL_WAKEUP_SYSTEM             _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(39), unsigned int)
#define FPSENSOR_IOCTL_GESTURE_KEY               _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(40), unsigned int)
#define FPSENSOR_IOCTL_SET_UP_THRESHOLD          _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(41), unsigned int)
#define FPSENSOR_IOCTL_SET_DOWN_THRESHOLD        _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(42), unsigned int)
#define FPSENSOR_IOCTL_SET_DETECT_THRESHOLD      _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(43), unsigned int)
#define FPSENSOR_IOCTL_GET_HWID                  _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(44), unsigned int)
#define FPSENSOR_IOCTL_GET_REVISION_ID           _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(45), unsigned int)
#define FPSENSOR_IOCTL_SELFTEST                  _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(46), unsigned int)
#define FPSENSOR_IOCTL_GET_SENSOR_ID             _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(47), unsigned int)
#define FPSENSOR_IOCTL_GET_ZONE_VALUES           _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(48), u8[12])
#define FPSENSOR_IOCTL_SET_SLEEP_DECT            _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(49), unsigned int)
#define FPSENSOR_IOCTL_RW_REG                    _IOWR(FPSENSOR_IOCTL_MAGIC_NO, FPSENSOR_IOCTL_BASE(50), u8[32])


/* -------------------------------------------------------------------- */
/* function prototypes                          */
/* -------------------------------------------------------------------- */
static int __init fpsensor_init(void);

static void __exit fpsensor_exit(void);

static int fpsensor_probe(struct spi_device *spi);

static int fpsensor_remove(struct spi_device *spi);

static int fpsensor_open(struct inode *inode, struct file *file);

static ssize_t fpsensor_write(struct file *file, const char *buff,
                              size_t count, loff_t *ppos);

static ssize_t fpsensor_read(struct file *file, char *buff,
                             size_t count, loff_t *ppos);

static int fpsensor_release(struct inode *inode, struct file *file);

static unsigned int fpsensor_poll(struct file *file, poll_table *wait);

static long fpsensor_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);

static long fpsensor_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static int fpsensor_cleanup(fpsensor_data_t *fpsensor, struct spi_device *spidev);

static int fpsensor_worker_init(fpsensor_data_t *fpsensor);

static int fpsensor_worker_destroy(fpsensor_data_t *fpsensor);

static int fpsensor_create_class(fpsensor_data_t *fpsensor);

static int fpsensor_create_device(fpsensor_data_t *fpsensor);

static int fpsensor_manage_sysfs(fpsensor_data_t *fpsensor,
                                 struct spi_device *spi, bool create);

static ssize_t fpsensor_show_attr_setup(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf);

static ssize_t fpsensor_store_attr_setup(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf,
                                         size_t count);

static ssize_t fpsensor_show_attr_diag(struct device *dev,
                                       struct device_attribute *attr,
                                       char *buf);

static ssize_t fpsensor_store_attr_diag(struct device *dev,
                                        struct device_attribute *attr,
                                        const char *buf,
                                        size_t count);

static u8 fpsensor_selftest_short(fpsensor_data_t *fpsensor);

static int fpsensor_start_capture(fpsensor_data_t *fpsensor);

static int fpsensor_new_job(fpsensor_data_t *fpsensor, int new_job);

static int fpsensor_worker_goto_idle(fpsensor_data_t *fpsensor);

static int fpsensor_worker_function(void *_fpsensor);

static int fpsensor_start_input(fpsensor_data_t *fpsensor);

/* -------------------------------------------------------------------- */
/* External interface                           */
/* -------------------------------------------------------------------- */
module_init(fpsensor_init);
module_exit(fpsensor_exit);


#if FPSENSOR_SPI_BUS_DYNAMIC
static struct spi_board_info spi_board_devs[] __initdata =
{
    [0] = {
        .modalias = FPSENSOR_DEV_NAME,
        .bus_num = 0,
        .chip_select = 0,
        .mode = SPI_MODE_0,
        .controller_data = &fpsensor_spi_conf_mt65xx, //&spi_conf
    },
};
#endif

struct spi_device_id fpsensor_spi_id_table = {FPSENSOR_DEV_NAME, 0};

static struct spi_driver fpsensor_driver =
{
    .driver = {
        .name   = FPSENSOR_DEV_NAME,
        //.bus    = &spi_bus_type,
        .owner  = THIS_MODULE,


    },
    .id_table = &fpsensor_spi_id_table,
    .probe  = fpsensor_probe,
    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
    .remove = __devexit_p(fpsensor_remove),
    #else
    .remove = fpsensor_remove,
    #endif
};

static const struct file_operations fpsensor_fops =
{
    .owner          = THIS_MODULE,
    .open           = fpsensor_open,
    .write          = fpsensor_write,
    .read           = fpsensor_read,
    .release        = fpsensor_release,
    .poll           = fpsensor_poll,
    .unlocked_ioctl = fpsensor_ioctl,
    .compat_ioctl   = fpsensor_compat_ioctl,
};


/* -------------------------------------------------------------------- */
/* devfs                                */
/* -------------------------------------------------------------------- */
#define FPSENSOR_ATTR(__grp, __field, __mode)                \
    {                                   \
        .attr = __ATTR(__field, (__mode),               \
                       fpsensor_show_attr_##__grp,                  \
                       fpsensor_store_attr_##__grp),                    \
                .offset = offsetof(struct fpsensor_##__grp, __field)     \
    }

#define FPSENSOR_DEV_ATTR(_grp, _field, _mode)               \
    struct fpsensor_attribute fpsensor_attr_##_field =            \
                                                                  FPSENSOR_ATTR(_grp, _field, (_mode))

//#define DEVFS_SETUP_MODE (S_IWUSR|S_IWGRP|S_IWOTH|S_IRUSR|S_IRGRP|S_IROTH)
#define DEVFS_SETUP_MODE (0644)
static FPSENSOR_DEV_ATTR(setup, adc_gain,        DEVFS_SETUP_MODE);
static FPSENSOR_DEV_ATTR(setup, adc_shift,       DEVFS_SETUP_MODE);
static FPSENSOR_DEV_ATTR(setup, capture_mode,        DEVFS_SETUP_MODE);
static FPSENSOR_DEV_ATTR(setup, capture_count,       DEVFS_SETUP_MODE);
static FPSENSOR_DEV_ATTR(setup, capture_settings_mux,    DEVFS_SETUP_MODE);
static FPSENSOR_DEV_ATTR(setup, pxl_ctrl,        DEVFS_SETUP_MODE);
static FPSENSOR_DEV_ATTR(setup, capture_row_start,   DEVFS_SETUP_MODE);
static FPSENSOR_DEV_ATTR(setup, capture_row_count,   DEVFS_SETUP_MODE);
static FPSENSOR_DEV_ATTR(setup, capture_col_start,   DEVFS_SETUP_MODE);
static FPSENSOR_DEV_ATTR(setup, capture_col_groups,  DEVFS_SETUP_MODE);
static FPSENSOR_DEV_ATTR(setup, adc_et1,  DEVFS_SETUP_MODE);
static FPSENSOR_DEV_ATTR(setup, tiny_capture_enable, DEVFS_SETUP_MODE);

static struct attribute *fpsensor_setup_attrs[] =
{
    &fpsensor_attr_adc_gain.attr.attr,
    &fpsensor_attr_adc_shift.attr.attr,
    &fpsensor_attr_capture_mode.attr.attr,
    &fpsensor_attr_capture_count.attr.attr,
    &fpsensor_attr_capture_settings_mux.attr.attr,
    &fpsensor_attr_pxl_ctrl.attr.attr,
    &fpsensor_attr_capture_row_start.attr.attr,
    &fpsensor_attr_capture_row_count.attr.attr,
    &fpsensor_attr_capture_col_start.attr.attr,
    &fpsensor_attr_capture_col_groups.attr.attr,
    &fpsensor_attr_adc_et1.attr.attr,
    &fpsensor_attr_tiny_capture_enable.attr.attr,//liuxn
    NULL
};

static const struct attribute_group fpsensor_setup_attr_group =
{
    .attrs = fpsensor_setup_attrs,
    .name = "setup"
};

//#define DEVFS_DIAG_MODE_RO (S_IRUSR|S_IRGRP|S_IROTH)
#define DEVFS_DIAG_MODE_RO (0644)
//#define DEVFS_DIAG_MODE_RW (S_IWUSR|S_IWGRP|S_IWOTH|S_IRUSR|S_IRGRP|S_IROTH)
#define DEVFS_DIAG_MODE_RW (0644)
static FPSENSOR_DEV_ATTR(diag, chip_id,      DEVFS_DIAG_MODE_RO);
static FPSENSOR_DEV_ATTR(diag, selftest,     DEVFS_DIAG_MODE_RO);
static FPSENSOR_DEV_ATTR(diag, spi_register, DEVFS_DIAG_MODE_RW);
static FPSENSOR_DEV_ATTR(diag, spi_regsize,  DEVFS_DIAG_MODE_RO);
static FPSENSOR_DEV_ATTR(diag, spi_data ,    DEVFS_DIAG_MODE_RW);
static FPSENSOR_DEV_ATTR(diag, last_capture_time, DEVFS_DIAG_MODE_RO);
static FPSENSOR_DEV_ATTR(diag, finger_present_status, DEVFS_DIAG_MODE_RO);
static FPSENSOR_DEV_ATTR(diag, wakeup_system,  DEVFS_DIAG_MODE_RW);
static FPSENSOR_DEV_ATTR(diag, gesture_key,  DEVFS_DIAG_MODE_RW);


static struct attribute *fpsensor_diag_attrs[] =
{
    &fpsensor_attr_chip_id.attr.attr,
    &fpsensor_attr_selftest.attr.attr,
    &fpsensor_attr_spi_register.attr.attr,
    &fpsensor_attr_spi_regsize.attr.attr,
    &fpsensor_attr_spi_data.attr.attr,
    &fpsensor_attr_last_capture_time.attr.attr,
    &fpsensor_attr_finger_present_status.attr.attr,
    &fpsensor_attr_wakeup_system.attr.attr,
    &fpsensor_attr_gesture_key.attr.attr,
    NULL
};

static const struct attribute_group fpsensor_diag_attr_group =
{
    .attrs = fpsensor_diag_attrs,
    .name = "diag"
};


/* -------------------------------------------------------------------- */
/* SPI debug interface, prototypes                  */
/* -------------------------------------------------------------------- */
static int fpsensor_spi_debug_select(fpsensor_data_t *fpsensor,
                                     fpsensor_reg_t reg);

static int fpsensor_spi_debug_value_write(fpsensor_data_t *fpsensor, u64 data);

static int fpsensor_spi_debug_buffer_write(fpsensor_data_t *fpsensor,
                                           const char *data,
                                           size_t count);

static int fpsensor_spi_debug_value_read(fpsensor_data_t *fpsensor,
                                         u64 *data);

static int fpsensor_spi_debug_buffer_read(fpsensor_data_t *fpsensor,
                                          u8 *data,
                                          size_t max_count);

static void fpsensor_spi_debug_buffer_to_hex_string(char *string,
                                                    u8 *buffer,
                                                    size_t bytes);

static int fpsensor_spi_debug_hex_string_to_buffer(u8 *buffer,
                                                   size_t buf_size,
                                                   const char *string,
                                                   size_t chars);

/* -------------------------------------------------------------------- */
/* function definitions                         */
/* -------------------------------------------------------------------- */
static int __init fpsensor_init(void)
{
    int status = 0;
    fpsensor_printk( " %s\n", __func__);
    #if FPSENSOR_SPI_BUS_DYNAMIC
    spi_register_board_info(spi_board_devs, ARRAY_SIZE(spi_board_devs));
    #endif
    status = spi_register_driver(&fpsensor_driver);
    if (status < 0)
    {
        fpsensor_error("Failed to register SPI driver.\n");
    }
    fpsensor_printk("fpsensor_init end\n");

    return status;
}


/* -------------------------------------------------------------------- */
static void __exit fpsensor_exit(void)
{
    fpsensor_printk( "%s\n", __func__);
    spi_unregister_driver(&fpsensor_driver);
}


/* -------------------------------------------------------------------- */
static int fpsensor_probe(struct spi_device *spi)
{
    struct fpsensor_platform_data *fpsensor_pdata;
    struct fpsensor_platform_data pdata_of;
    struct device *dev = &spi->dev;
    int error = 0;
    fpsensor_data_t *fpsensor = NULL;
    size_t buffer_size;

    plat_power(1);

    fpsensor = kzalloc(sizeof(*fpsensor), GFP_KERNEL);
    if (!fpsensor)
    {
        fpsensor_error("failed to allocate memory for struct fpsensor_data\n");

        return -ENOMEM;
    }
    fpsensor_printk("%s, 201600705 -- 3,0x%p\n", __func__, fpsensor);
    fpsensor_data_init(fpsensor);

    buffer_size = fpsensor_calc_huge_buffer_minsize(fpsensor);
    error = fpsensor_manage_huge_buffer(fpsensor, buffer_size);
    if (error)
    {
        goto err;
    }

    spi_set_drvdata(spi, fpsensor);
    fpsensor->spi = spi;
    fpsensor->spi_freq_khz = 1000u;

    fpsensor->reset_gpio = -EINVAL;
    fpsensor->irq_gpio   = -EINVAL;
    fpsensor->cs_gpio    = -EINVAL;

    fpsensor->irq        = -EINVAL;
    fpsensor->use_regulator_for_bezel = 0;
    fpsensor->without_bezel = 0;
    fpsensor->fpsensor_init_done = 0;

    init_waitqueue_head(&fpsensor->wq_irq_return);

    error = fpsensor_init_capture(fpsensor);
    if (error)
    {
        goto err;
    }

    fpsensor_pdata = spi->dev.platform_data;

    if (!fpsensor_pdata)
    {
        error = fpsensor_get_of_pdata(dev, &pdata_of);
        fpsensor_pdata = &pdata_of;

        if (error)
        {
            goto err;
        }
    }

    // dts read
    spi->dev.of_node = of_find_compatible_node(NULL, NULL, "mediatek,sunwave-finger");

    fpsensor->pinctrl1 = devm_pinctrl_get(&spi->dev);
    if (IS_ERR(fpsensor->pinctrl1))
    {
        error = PTR_ERR(fpsensor->pinctrl1);
        fpsensor_error("fpsensor Cannot find fp pinctrl1.\n");
        goto err;
    }
    fpsensor_spidev_dts_init(fpsensor);
    // end dts read

    error = fpsensor_param_init(fpsensor, fpsensor_pdata);
    if (error)
    {
        goto err;
    }

    error = fpsensor_supply_init(fpsensor);
    if (error)
    {
        goto err;
    }

    error = fpsensor_reset_init(fpsensor, fpsensor_pdata);
    if (error)
    {
        goto err;
    }

    error = fpsensor_irq_init(fpsensor, fpsensor_pdata);
    if (error)
    {
        goto err;
    }

    error = fpsensor_spi_setup(fpsensor, fpsensor_pdata);
    if (error)
    {
        goto err;
    }

    error = fpsensor_reset(fpsensor);
    if (error)
    {
        goto err;
    }

    fpsensor->fpsensor_init_done = 1;

    error = fpsensor_check_hw_id(fpsensor);
    if (error)
    {
        goto err;
    }

    #if FPSENSOR_FIX_FREQ == 0
    fpsensor->spi_freq_khz = fpsensor->chip.spi_max_khz;
    #else
    fpsensor->spi_freq_khz = FPSENSOR_FIX_FREQ;
    #endif
    fpsensor_trace("Req. SPI frequency : %d kHz.\n",
                   fpsensor->spi_freq_khz);

    buffer_size = fpsensor_calc_huge_buffer_minsize(fpsensor);
    error = fpsensor_manage_huge_buffer(fpsensor, buffer_size);
    if (error)
    {
        goto err;
    }

    error = fpsensor_setup_defaults(fpsensor);
    if (error)
    {
        goto err;
    }

    error = fpsensor_create_class(fpsensor);
    if (error)
    {
        goto err;
    }

    error = fpsensor_create_device(fpsensor);
    if (error)
    {
        goto err;
    }

    sema_init(&fpsensor->mutex, 0);

    error = fpsensor_manage_sysfs(fpsensor, spi, true);
    if (error)
    {
        goto err;
    }

    cdev_init(&fpsensor->cdev, &fpsensor_fops);
    fpsensor->cdev.owner = THIS_MODULE;

    error = cdev_add(&fpsensor->cdev, fpsensor->devno, 1);
    if (error)
    {
        fpsensor_error( "cdev_add failed.\n");
        goto err_chrdev;
    }

    error = fpsensor_worker_init(fpsensor);
    if (error)
    {
        goto err_cdev;
    }

    error = fpsensor_calc_finger_detect_threshold_min(fpsensor);
    if (error < 0)
    {
        goto err_cdev;
    }

    error = fpsensor_set_finger_detect_threshold(fpsensor, error);
    if (error < 0)
    {
        goto err_cdev;
    }


    error = fpsensor_input_init(fpsensor);
    if (error)
    {
        goto err_cdev;
    }

    /*
        error = fpsensor_start_input(fpsensor);
        if (error)
            goto err_cdev;
    */

    fpsensor_sleep(fpsensor, true);

    up(&fpsensor->mutex);
    return 0;

err_cdev:
    cdev_del(&fpsensor->cdev);

err_chrdev:
    unregister_chrdev_region(fpsensor->devno, 1);

    fpsensor_manage_sysfs(fpsensor, spi, false);

err:
    fpsensor_cleanup(fpsensor, spi);
    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_remove(struct spi_device *spi)
{
    fpsensor_data_t *fpsensor = spi_get_drvdata(spi);

    printk("%s\n", __func__);

    fpsensor_manage_sysfs(fpsensor, spi, false);

    fpsensor_sleep(fpsensor, true);

    cdev_del(&fpsensor->cdev);

    unregister_chrdev_region(fpsensor->devno, 1);

    fpsensor_cleanup(fpsensor, spi);

    return 0;
}



/* -------------------------------------------------------------------- */
static int fpsensor_open(struct inode *inode, struct file *file)

{
    fpsensor_data_t *fpsensor;

    fpsensor_trace( "%s\n", __func__);

    fpsensor = container_of(inode->i_cdev, fpsensor_data_t, cdev);

    if (down_interruptible(&fpsensor->mutex))
    {
        return -ERESTARTSYS;
    }

    file->private_data = fpsensor;

    up(&fpsensor->mutex);

    return 0;
}


/* -------------------------------------------------------------------- */
static ssize_t fpsensor_write(struct file *file, const char *buff,
                              size_t count, loff_t *ppos)
{
//    fpsensor_trace( "%s\n", __func__);

    return -ENOTTY;
}


/* -------------------------------------------------------------------- */
static ssize_t fpsensor_read(struct file *file, char *buff,
                             size_t count, loff_t *ppos)
{
    fpsensor_data_t *fpsensor = file->private_data;
    int error = 0;
    u32 max_data;
    u32 avail_data;
//    fpsensor_trace( "%s, count: %d\n", __func__, count);

    if (down_interruptible(&fpsensor->mutex))
    {
        return -ERESTARTSYS;
    }

    if (fpsensor->capture.available_bytes > 0)
    {

        goto copy_data;
    }
    else
    {

        if (fpsensor->capture.read_pending_eof)
        {
            fpsensor->capture.read_pending_eof = false;
            error = 0;
            goto out;
        }

        if (file->f_flags & O_NONBLOCK)
        {
            if (fpsensor_capture_check_ready(fpsensor))
            {
                error = fpsensor_start_capture(fpsensor);
                if (error)
                {
                    goto out;
                }
            }
            error = -EWOULDBLOCK;
            goto out;

        }
        else
        {
            error = fpsensor_start_capture(fpsensor);
            if (error)
            {
                goto out;
            }
        }
    }

    error = wait_event_interruptible(
                fpsensor->capture.wq_data_avail,
                (fpsensor->capture.available_bytes > 0));

    if (error)
    {
        goto out;
    }
    if (fpsensor->capture.last_error != 0)
    {
        error = fpsensor->capture.last_error;
        goto out;
    }

copy_data:
    avail_data = fpsensor->capture.available_bytes;
    max_data = (count > avail_data) ? avail_data : count;

    if (max_data)
    {
        error = copy_to_user(buff,
                             &fpsensor->huge_buffer[fpsensor->capture.read_offset],
                             max_data);

        if (error)
        {
            goto out;
        }

        fpsensor->capture.read_offset += max_data;
        fpsensor->capture.available_bytes -= max_data;

        error = max_data;

        if (fpsensor->capture.available_bytes == 0)
        {
            fpsensor->capture.read_pending_eof = true;
        }
    }
out:
    up(&fpsensor->mutex);

    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_release(struct inode *inode, struct file *file)
{
    fpsensor_data_t *fpsensor = file->private_data;
    int status = 0;

    fpsensor_trace( "%s\n", __func__);

    if (down_interruptible(&fpsensor->mutex))
    {
        return -ERESTARTSYS;
    }

    fpsensor_worker_goto_idle(fpsensor);

    fpsensor_sleep(fpsensor, true);

    up(&fpsensor->mutex);

    return status;
}


/* -------------------------------------------------------------------- */
static unsigned int fpsensor_poll(struct file *file, poll_table *wait)
{
    fpsensor_data_t *fpsensor = file->private_data;
    unsigned int ret = 0;
    fpsensor_capture_mode_t mode = fpsensor->setup.capture_mode;
    bool blocking_op;
//    fpsensor_trace("%s, mode: %d\n", __func__, mode);

    if (down_interruptible(&fpsensor->mutex))
    {
        return -ERESTARTSYS;
    }

    if (fpsensor->capture.available_bytes > 0)
    {
        ret |= (POLLIN | POLLRDNORM);
    }
    else if (fpsensor->capture.read_pending_eof)
    {
        ret |= POLLHUP;
    }
    else   /* available_bytes == 0 && !pending_eof */
    {

        blocking_op =
            (mode == FPSENSOR_MODE_WAIT_AND_CAPTURE) ? true : false;

        switch (fpsensor->capture.state)
        {

            case FPSENSOR_CAPTURE_STATE_IDLE:
                if (!blocking_op)
                {
                    ret |= POLLIN;
                }
                break;

            case FPSENSOR_CAPTURE_STATE_STARTED:
            case FPSENSOR_CAPTURE_STATE_PENDING:
            case FPSENSOR_CAPTURE_STATE_WRITE_SETTINGS:
            case FPSENSOR_CAPTURE_STATE_WAIT_FOR_FINGER_DOWN:
            case FPSENSOR_CAPTURE_STATE_ACQUIRE:
            case FPSENSOR_CAPTURE_STATE_FETCH:
            case FPSENSOR_CAPTURE_STATE_WAIT_FOR_FINGER_UP:
            case FPSENSOR_CAPTURE_STATE_COMPLETED:
                //ret |= POLLIN;

                poll_wait(file, &fpsensor->capture.wq_data_avail, wait);

                if (fpsensor->capture.available_bytes > 0)
                {
                    ret |= POLLRDNORM;
                }
                else if (blocking_op)
                {
                    ret = 0;
                }

                break;

            case FPSENSOR_CAPTURE_STATE_FAILED:
                if (!blocking_op)
                {
                    ret |= POLLIN;
                }
                break;

            default:
                fpsensor_error("%s unknown state\n", __func__);
                break;
        }
    }

    up(&fpsensor->mutex);

    return ret;
}

static long fpsensor_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return fpsensor_ioctl(filp, cmd, (unsigned long)(arg));
}

static long fpsensor_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    int error = 0;
    int mux;
    unsigned int user_regval;
    fpsensor_data_t *fpsensor = file->private_data;
    int key;
    int downUp;
    bool sync;
    u8 zone_value[12];
    int column_groups = fpsensor->chip.pixel_columns / fpsensor->chip.adc_group_size;

    fpsensor_trace("%s, cmd: %d\n", __func__, (u8)cmd);
//    if (down_interruptible(&fpsensor->mutex))
//        return -ERESTARTSYS;
    mux = fpsensor->setup.capture_settings_mux;
    switch (cmd)
    {
        case FPSENSOR_IOCTL_WRITE_ADC_GAIN:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            fpsensor->setup.adc_gain[mux] = (u8)user_regval;
            fpsensor_trace( "%s, mux = %d, adc_gain = 0x%x\n", __func__, mux, fpsensor->setup.adc_gain[mux]);
            break;
        case FPSENSOR_IOCTL_READ_ADC_GAIN:

            break;
        case FPSENSOR_IOCTL_WRITE_ADC_SHIFT:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            fpsensor->setup.adc_shift[mux] = (u8)user_regval;
            fpsensor_trace( "%s, mux = %d, adc_shift = 0x%x\n", __func__, mux, fpsensor->setup.adc_shift[mux]);
            break;
        case FPSENSOR_IOCTL_READ_ADC_SHIFT:

            break;
        case FPSENSOR_IOCTL_WRITE_PXL_CTRL:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 2) )
            {
                error = -EFAULT;
                break;
            }
            fpsensor->setup.pxl_ctrl[mux] = (u16)user_regval;
            fpsensor_trace( "%s, mux = %d, pxl_ctrl = 0x%x\n", __func__, mux, fpsensor->setup.pxl_ctrl[mux]);
            break;
        case FPSENSOR_IOCTL_READ_PXL_CTRL:

            break;

        case FPSENSOR_IOCTL_WRITE_ET1:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            fpsensor->setup.adc_et1[mux] = (u8)user_regval;
            fpsensor_trace( "%s, mux = %d, adc_et1 = 0x%x\n", __func__, mux, fpsensor->setup.adc_et1[mux]);
            break;
        case FPSENSOR_IOCTL_READ_ET1:

            break;


        case FPSENSOR_IOCTL_SET_MUX:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            if (fpsensor_check_in_range_u64((u8)user_regval, 0, (FPSENSOR_BUFFER_MAX_IMAGES)))
            {
                fpsensor->setup.capture_settings_mux = (u8)user_regval;
                fpsensor_trace( "%s, mux = %d, capture_settings_mux = 0x%x\n", __func__, mux,
                                fpsensor->setup.capture_settings_mux);
            }
            else
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_SET_CAPTURE_COUNT:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            if (fpsensor_check_in_range_u64((u8)user_regval, 1, FPSENSOR_BUFFER_MAX_IMAGES))
            {

                fpsensor->setup.capture_count = (u8)user_regval;
                fpsensor_trace( "%s, mux = %d, capture_count = 0x%x\n", __func__, mux,
                                fpsensor->setup.capture_count);
            }
            else
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_SET_CAPTURE_MODE:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            fpsensor->setup.capture_mode = (u8)user_regval;
            fpsensor_trace( "%s, mux = %d, capture_mode = 0x%x\n", __func__, mux, fpsensor->setup.capture_mode);
            break;
        case FPSENSOR_IOCTL_SET_ROW_START:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            if (fpsensor_check_in_range_u64((u8)user_regval, 0, (fpsensor->chip.pixel_rows - 1)))
            {

                fpsensor->setup.capture_row_start = (u8)user_regval;
                fpsensor_trace( "%s, mux = %d, capture_row_start = 0x%x\n", __func__, mux,
                                fpsensor->setup.capture_row_start);
            }
            else
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_SET_ROW_COUNT:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            if (fpsensor_check_in_range_u64((u8)user_regval, 1, fpsensor->chip.pixel_rows))
            {

                fpsensor->setup.capture_row_count = (u8)user_regval;
                fpsensor_trace( "%s, mux = %d, capture_row_count = 0x%x\n", __func__, mux,
                                fpsensor->setup.capture_row_count);
            }
            else
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_SET_COL_START:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            if (fpsensor_check_in_range_u64((u8)user_regval, 0, (column_groups - 1)))
            {

                fpsensor->setup.capture_col_start = (u8)user_regval;
                fpsensor_trace( "%s, mux = %d, capture_col_start = 0x%x\n", __func__, mux,
                                fpsensor->setup.capture_col_start);
            }
            else
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_SET_COL_GROUPS:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            if (fpsensor_check_in_range_u64((u8)user_regval, 1, column_groups))
            {

                fpsensor->setup.capture_col_groups = (u8)user_regval;
                fpsensor_trace( "%s, mux = %d, capture_col_groups = 0x%x\n", __func__, mux,
                                fpsensor->setup.capture_col_groups);
            }
            else
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_GET_CAPTURE_STATE:

            user_regval = fpsensor->capture.state;
            fpsensor_trace( "FPSENSOR_IOCTL_GET_CAPTURE_STATE = %d\n", (u8)user_regval);
            if (copy_to_user((void __user *)arg, &user_regval, 1) != 0)
            {
                error = -EFAULT;
            }
            break;

        case FPSENSOR_IOCTL_WRITE_TINY_CAPTURE:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            fpsensor->setup.tiny_capture_enable = user_regval & 0x1;
            fpsensor_trace( "%s, mux = %d, tiny_capture_enable = 0x%x\n", __func__, mux,
                            fpsensor->setup.tiny_capture_enable);
            break;
        case FPSENSOR_IOCTL_GET_FINGER_STATUS:

            error = fpsensor_get_finger_present_status(fpsensor);
            if (error >= 0)
            {
                user_regval = (int)error;
                error = 0;
            }

            fpsensor_trace( "FPSENSOR_IOCTL_GET_FINGER_STATUS = 0x%x\n", (u16)user_regval);
            if (copy_to_user((void __user *)arg, &user_regval, 2) != 0)
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_WAKEUP_SYSTEM:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            error = fpsensor_input_wakeup(fpsensor, (fpsensor_wakeup_level_t)user_regval);
            if (error)
            {
                return error;
            }
            fpsensor_trace( "FPSENSOR_IOCTL_WAKEUP_SYSTEM,  = %d\n", (fpsensor_wakeup_level_t)user_regval);

            break;
        case FPSENSOR_IOCTL_GESTURE_KEY:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 2) )
            {
                error = -EFAULT;
                break;
            }
            key = user_regval & (0xff);
            downUp = (user_regval >> 8) & 0x0f;
            sync = (user_regval >> 12) & 0x0f;
            fpsensor_input_key(fpsensor, key, downUp, sync);
            fpsensor_trace( "FPSENSOR_IOCTL_GESTURE_KEY = 0x%x\n", (u16)user_regval);

            break;
        case FPSENSOR_IOCTL_SET_UP_THRESHOLD:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            fpsensor->setup.capture_finger_up_threshold = (u8)user_regval;
            fpsensor_trace( "%s, mux = %d, capture_finger_up_threshold = 0x%x\n", __func__, mux,
                            fpsensor->setup.capture_finger_up_threshold);
            break;

        case FPSENSOR_IOCTL_SET_DOWN_THRESHOLD:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            fpsensor->setup.capture_finger_down_threshold = (u8)user_regval;
            fpsensor_trace( "%s, mux = %d, capture_finger_down_threshold = 0x%x\n", __func__, mux,
                            fpsensor->setup.capture_finger_down_threshold);
            break;
        case FPSENSOR_IOCTL_SET_DETECT_THRESHOLD:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            fpsensor->setup.finger_detect_threshold = (u8)user_regval;
            fpsensor_trace( "%s, mux = %d, finger_detect_threshold = 0x%x\n", __func__, mux,
                            fpsensor->setup.finger_detect_threshold);
            break;
        case FPSENSOR_IOCTL_GET_HWID:
            user_regval = fpsensor->chip.hwid;
            fpsensor_trace( "FPSENSOR_IOCTL_GET_HWID = 0x%x\n", (u16)user_regval);
            if (copy_to_user((void __user *)arg, &user_regval, 2) != 0)
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_GET_REVISION_ID:
            user_regval = fpsensor->chip.revision;
            fpsensor_trace( "FPSENSOR_IOCTL_GET_REVISION_ID = 0x%x\n", (u16)user_regval);
            if (copy_to_user((void __user *)arg, &user_regval, 2) != 0)
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_GET_SENSOR_ID:
            user_regval = fpsensor->chip.sensorid;
            fpsensor_trace( "FPSENSOR_IOCTL_GET_REVISION_ID = 0x%x\n", (u16)user_regval);
            if (copy_to_user((void __user *)arg, &user_regval, 2) != 0)
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_SELFTEST:
            user_regval = fpsensor_selftest_short(fpsensor);
            fpsensor_trace( "FPSENSOR_IOCTL_SELFTEST = 0x%x\n", (u16)user_regval);
            if (copy_to_user((void __user *)arg, &user_regval, 2) != 0)
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_GET_ZONE_VALUES:
            fpsensor_get_finger_status_value(fpsensor , zone_value);
            fpsensor_trace( "FPSENSOR_IOCTL_GET_ZONE_VALUES[0] = 0x%x\n", (u16)zone_value[6]);
            if (copy_to_user((void __user *)arg, zone_value, 12) != 0)
            {
                error = -EFAULT;
            }
            break;
        case FPSENSOR_IOCTL_SET_SLEEP_DECT:
            if (copy_from_user(&user_regval, (unsigned int __user *)arg, 1) )
            {
                error = -EFAULT;
                break;
            }
            fpsensor->setup.sleep_dect = (u8)user_regval;
            fpsensor_trace( "%s, mux = %d, sleep_dect = 0x%x\n", __func__, mux, fpsensor->setup.sleep_dect);
            break;
        default:
            //error = -ENOTTY;
            break;
    }

//    up(&fpsensor->mutex);
    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_cleanup(fpsensor_data_t *fpsensor, struct spi_device *spidev)
{
    fpsensor_trace( "%s\n", __func__);

    plat_power(0);

    fpsensor_worker_destroy(fpsensor);

    if (!IS_ERR_OR_NULL(fpsensor->device))
    {
        device_destroy(fpsensor->class, fpsensor->devno);
    }

    class_destroy(fpsensor->class);

    fpsensor_irq_disable(fpsensor);
    fpsensor_irq_free(fpsensor);

    if (fpsensor_gpio_valid(fpsensor->reset_gpio))
    {
        fpsensor_gpio_free(fpsensor->reset_gpio);
    }

    #if FPSENSOR_MANUAL_CS
    if (fpsensor_gpio_valid(fpsensor->cs_gpio))
    {
        fpsensor_gpio_free(fpsensor->cs_gpio);
    }
    #endif

    fpsensor_manage_huge_buffer(fpsensor, 0);

    fpsensor_input_destroy(fpsensor);

    kfree(fpsensor);

    spi_set_drvdata(spidev, NULL);

    // spi_unregister_driver(&fpsensor_driver);

    return 0;
}





/* -------------------------------------------------------------------- */
static int fpsensor_worker_init(fpsensor_data_t *fpsensor)
{
    int error = 0;

    fpsensor_trace("%s\n", __func__);

    init_waitqueue_head(&fpsensor->worker.wq_wait_job);
    sema_init(&fpsensor->worker.sem_idle, 0);

    fpsensor->worker.req_mode = FPSENSOR_WORKER_IDLE_MODE;

    fpsensor->worker.thread = kthread_run(fpsensor_worker_function,
                                          fpsensor, "%s",
                                          FPSENSOR_WORKER_THREAD_NAME);

    if (IS_ERR(fpsensor->worker.thread))
    {
        fpsensor_error( "kthread_run failed.\n");
        error = (int)PTR_ERR(fpsensor->worker.thread);
    }

    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_worker_destroy(fpsensor_data_t *fpsensor)
{
    int error = 0;

    fpsensor_trace("%s\n", __func__);

    if (fpsensor->worker.thread)
    {
        fpsensor_worker_goto_idle(fpsensor);

        fpsensor->worker.req_mode = FPSENSOR_WORKER_EXIT;
        wake_up_interruptible(&fpsensor->worker.wq_wait_job);
        kthread_stop(fpsensor->worker.thread);
    }

    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_create_class(fpsensor_data_t *fpsensor)
{
    int error = 0;

    fpsensor_trace( "%s\n", __func__);

    fpsensor->class = class_create(THIS_MODULE, FPSENSOR_CLASS_NAME);

    if (IS_ERR(fpsensor->class))
    {
        fpsensor_error( "failed to create class.\n");
        error = PTR_ERR(fpsensor->class);
    }

    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_create_device(fpsensor_data_t *fpsensor)
{
    int error = 0;

    fpsensor_trace( "%s\n", __func__);

    if (FPSENSOR_MAJOR > 0)
    {
        fpsensor->devno = MKDEV(FPSENSOR_MAJOR, fpsensor_device_count++);

        error = register_chrdev_region(fpsensor->devno,
                                       1,
                                       FPSENSOR_DEV_NAME);
    }
    else
    {
        error = alloc_chrdev_region(&fpsensor->devno,
                                    fpsensor_device_count++,
                                    1,
                                    FPSENSOR_DEV_NAME);
    }

    if (error < 0)
    {
        fpsensor_error(
            "%s: FAILED %d.\n", __func__, error);
        goto out;

    }
    else
    {
        fpsensor_trace( "%s: major=%d, minor=%d\n",
                        __func__,
                        MAJOR(fpsensor->devno),
                        MINOR(fpsensor->devno));
    }

    fpsensor->device = device_create(fpsensor->class, NULL, fpsensor->devno,
                                     NULL, "%s", FPSENSOR_DEV_NAME);

    if (IS_ERR(fpsensor->device))
    {
        fpsensor_error( "device_create failed.\n");
        error = PTR_ERR(fpsensor->device);
    }
out:
    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_manage_sysfs(fpsensor_data_t *fpsensor,
                                 struct spi_device *spi, bool create)
{
    int error = 0;

    if (create)
    {
        fpsensor_trace( "%s create\n", __func__);

        error = sysfs_create_group(&spi->dev.kobj,
                                   &fpsensor_setup_attr_group);

        if (error)
        {
            fpsensor_error(
                "sysf_create_group failed.\n");
            return error;
        }

        error = sysfs_create_group(&spi->dev.kobj,
                                   &fpsensor_diag_attr_group);

        if (error)
        {
            sysfs_remove_group(&spi->dev.kobj,
                               &fpsensor_setup_attr_group);

            fpsensor_error(
                "sysf_create_group failed.\n");

            return error;
        }
    }
    else
    {
        fpsensor_trace( "%s remove\n", __func__);

        sysfs_remove_group(&spi->dev.kobj, &fpsensor_setup_attr_group);
        sysfs_remove_group(&spi->dev.kobj, &fpsensor_diag_attr_group);
    }

    return error;
}


/* -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
static ssize_t fpsensor_show_attr_setup(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf)
{
    fpsensor_data_t *fpsensor = dev_get_drvdata(dev);
    struct fpsensor_attribute *fpsensor_attr;
    int val = -1;
    int mux;
//    fpsensor_trace("%s\n", __func__);

    fpsensor_attr = container_of(attr, struct fpsensor_attribute, attr);

    mux = fpsensor->setup.capture_settings_mux;

    if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, adc_gain))
    {
        val = fpsensor->setup.adc_gain[mux];
    }

    else if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, adc_shift))
    {
        val = fpsensor->setup.adc_shift[mux];
    }

    else if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, pxl_ctrl))
    {
        val = fpsensor->setup.pxl_ctrl[mux];
    }

    else if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, adc_et1))
    {
        val = fpsensor->setup.adc_et1[mux];
    }


    else if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, capture_mode))
    {
        val = fpsensor->setup.capture_mode;
    }
    else if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, tiny_capture_enable))
    {
        val = fpsensor->setup.tiny_capture_enable;
    }

    else if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, capture_count))
    {
        val = fpsensor->setup.capture_count;
    }

    else if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, capture_settings_mux))
    {
        val = fpsensor->setup.capture_settings_mux;
    }

    else if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, capture_row_start))
    {
        val = fpsensor->setup.capture_row_start;
    }

    else if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, capture_row_count))
    {
        val = fpsensor->setup.capture_row_count;
    }

    else if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, capture_col_start))
    {
        val = fpsensor->setup.capture_col_start;
    }

    else if (fpsensor_attr->offset == offsetof(fpsensor_setup_t, capture_col_groups))
    {
        val = fpsensor->setup.capture_col_groups;
    }

    if (val >= 0)
    {
        return scnprintf(buf, PAGE_SIZE, "%i\n", val);
    }

    return -ENOENT;
}


/* -------------------------------------------------------------------- */
static ssize_t fpsensor_store_attr_setup(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf,
                                         size_t count)
{
    fpsensor_data_t *fpsensor = dev_get_drvdata(dev);
    u64 val;
    int error = kstrtou64(buf, 0, &val);
    int mux;
    int column_groups = fpsensor->chip.pixel_columns / fpsensor->chip.adc_group_size;

    struct fpsensor_attribute *fpsensor_attr;
    fpsensor_attr = container_of(attr, struct fpsensor_attribute, attr);

    mux = fpsensor->setup.capture_settings_mux;

    if (!error)
    {
        if (fpsensor_attr->offset ==
            offsetof(fpsensor_setup_t, adc_gain))
        {

            fpsensor->setup.adc_gain[mux] = (u8)val;
            fpsensor_trace( "%s, mux = %d, adc_gain = 0x%x\n", __func__, mux, (u8)val);

        }
        else if (fpsensor_attr->offset ==
                 offsetof(fpsensor_setup_t, adc_shift))
        {


            fpsensor->setup.adc_shift[mux] = (u8)val;
            fpsensor_trace( "%s, mux = %d, adc_shift = 0x%x\n", __func__, mux, (u8)val);

        }
        else if (fpsensor_attr->offset ==
                 offsetof(fpsensor_setup_t, pxl_ctrl))
        {


            fpsensor->setup.pxl_ctrl[mux] = (u16)val;
            fpsensor_trace( "%s, mux = %d, pxl_ctrl = 0x%x\n", __func__, mux, (u16)val);

        }
        else if (fpsensor_attr->offset ==
                 offsetof(fpsensor_setup_t, adc_et1))
        {

            fpsensor->setup.adc_et1[mux] = (u8)val;
            fpsensor_trace( "%s, mux = %d, adc_et1 = 0x%x\n", __func__, mux, (u8)val);


        }
        else if (fpsensor_attr->offset ==
                 offsetof(fpsensor_setup_t, capture_mode))
        {


            fpsensor->setup.capture_mode = (fpsensor_capture_mode_t)val;
//            fpsensor_trace( "%s, mux = %d, capture_mode = 0x%x\n", __func__, mux, (fpsensor_capture_mode_t)val);

        }
        else if (fpsensor_attr->offset ==
                 offsetof(fpsensor_setup_t, tiny_capture_enable))
        {


            fpsensor->setup.tiny_capture_enable = (bool)val;
            //            fpsensor_trace( "%s, mux = %d, tiny_capture_enable = 0x%x\n", __func__, mux, (u8)val);
        }
        else if (fpsensor_attr->offset ==
                 offsetof(fpsensor_setup_t, capture_count))
        {


            if (fpsensor_check_in_range_u64
                (val, 1, FPSENSOR_BUFFER_MAX_IMAGES))
            {

                fpsensor->setup.capture_count = (u8)val;
//                fpsensor_trace( "%s, mux = %d, capture_count = 0x%x\n", __func__, mux, (u8)val);
            }
            else
            {
                return -EINVAL;
            }

        }
        else if (fpsensor_attr->offset ==
                 offsetof(fpsensor_setup_t, capture_settings_mux))
        {


            if (fpsensor_check_in_range_u64
                (val, 0, (FPSENSOR_BUFFER_MAX_IMAGES - 1)))
            {

                fpsensor->setup.capture_settings_mux = (u8)val;
//                fpsensor_trace( "%s, mux = %d, capture_settings_mux = 0x%x\n", __func__, mux, (u8)val);
            }
            else
            {
                return -EINVAL;
            }

        }
        else if (fpsensor_attr->offset ==
                 offsetof(fpsensor_setup_t, capture_row_start))
        {


            if (fpsensor_check_in_range_u64
                (val, 0, (fpsensor->chip.pixel_rows - 1)))
            {

                fpsensor->setup.capture_row_start = (u8)val;
//                fpsensor_trace( "%s, mux = %d, capture_row_start = 0x%x\n", __func__, mux, (u8)val);
            }
            else
            {
                return -EINVAL;
            }

        }
        else if (fpsensor_attr->offset ==
                 offsetof(fpsensor_setup_t, capture_row_count))
        {


            if (fpsensor_check_in_range_u64
                (val, 1, fpsensor->chip.pixel_rows))
            {

                fpsensor->setup.capture_row_count = (u8)val;
//                fpsensor_trace( "%s, mux = %d, capture_row_count = 0x%x\n", __func__, mux, (u8)val);
            }
            else
            {
                return -EINVAL;
            }

        }
        else if (fpsensor_attr->offset ==
                 offsetof(fpsensor_setup_t, capture_col_start))
        {


            if (fpsensor_check_in_range_u64
                (val, 0, (column_groups - 1)))
            {

                fpsensor->setup.capture_col_start = (u8)val;
//                fpsensor_trace( "%s, mux = %d, capture_col_start = 0x%x\n", __func__, mux, (u8)val);
            }
            else
            {
                return -EINVAL;
            }


        }
        else if (fpsensor_attr->offset ==
                 offsetof(fpsensor_setup_t, capture_col_groups))
        {


            if (fpsensor_check_in_range_u64
                (val, 1, column_groups))
            {

                fpsensor->setup.capture_col_groups = (u8)val;
//                fpsensor_trace( "%s, mux = %d, capture_col_groups = 0x%x\n", __func__, mux, (u8)val);
            }
            else
            {
                return -EINVAL;
            }
        }
        else
        {
            return -ENOENT;
        }

        return strnlen(buf, count);
    }
    return error;
}


/* -------------------------------------------------------------------- */
static ssize_t fpsensor_show_attr_diag(struct device *dev,
                                       struct device_attribute *attr,
                                       char *buf)
{
    fpsensor_data_t *fpsensor;
    struct fpsensor_attribute *fpsensor_attr;
    u64 val = 0;
    int error = 0;
    bool is_buffer = false;
    u8 u8_buffer[FPSENSOR_REG_MAX_SIZE];
    char hex_string[sizeof("0x") + (FPSENSOR_REG_MAX_SIZE * 2)];
//    fpsensor_trace( "%s\n", __func__);

    fpsensor = dev_get_drvdata(dev);

    fpsensor_attr = container_of(attr, struct fpsensor_attribute, attr);

    switch (fpsensor_attr->offset)
    {
        case offsetof(fpsensor_diag_t, chip_id):
            return scnprintf(buf,
                             PAGE_SIZE,
                             "%s rev.%d\n",
                             fpsensor_hw_id_text(fpsensor),
                             fpsensor->chip.revision);
            break;
        case offsetof(fpsensor_diag_t, selftest):
            val = (u64)fpsensor_selftest_short(fpsensor);
            break;
        case offsetof(fpsensor_diag_t, spi_register):
            val = (int)fpsensor->diag.spi_register;
            break;
        case offsetof(fpsensor_diag_t, spi_regsize):
            val = (int)fpsensor->diag.spi_regsize;
            break;
        case offsetof(fpsensor_diag_t, spi_data):
            is_buffer = (fpsensor->diag.spi_regsize > sizeof(val));

            if (!is_buffer)
            {
                error = fpsensor_spi_debug_value_read(fpsensor, &val);
            }
            else
            {
                error = fpsensor_spi_debug_buffer_read(fpsensor,
                                                       u8_buffer,
                                                       sizeof(u8_buffer));
            }
            break;
        case offsetof(fpsensor_diag_t, last_capture_time):
            val = (int)fpsensor->diag.last_capture_time;
            break;
        case offsetof(fpsensor_diag_t, finger_present_status):
            error = fpsensor_get_finger_present_status(fpsensor);
            if (error >= 0)
            {
                val = (int)error;
                error = 0;
            }
            break;
    }

    if (error >= 0 && !is_buffer)
    {

        printk("======== is_buffer == 0, 0x%llx\n", val);
        return scnprintf(buf,
                         PAGE_SIZE,
                         "%lu\n",
                         (long unsigned int)val);


    }

    if (error >= 0 && is_buffer)
    {

        printk("======== is_buffer == 1\n");
        fpsensor_spi_debug_buffer_to_hex_string(hex_string,
                                                u8_buffer,
                                                fpsensor->diag.spi_regsize);

        return scnprintf(buf, PAGE_SIZE, "%s\n", hex_string);
    }

    return error;
}


/* -------------------------------------------------------------------- */
static ssize_t fpsensor_store_attr_diag(struct device *dev,
                                        struct device_attribute *attr,
                                        const char *buf,
                                        size_t count)
{
    fpsensor_data_t *fpsensor = dev_get_drvdata(dev);
    u64 val;
    int error = 0;
    int key;
    int status;
    int downUp;
    bool sync;

    struct fpsensor_attribute *fpsensor_attr;
    fpsensor_attr = container_of(attr, struct fpsensor_attribute, attr);
//    fpsensor_trace( "%s\n", __func__);

    if (fpsensor_attr->offset == offsetof(fpsensor_diag_t, spi_register))
    {
        error = kstrtou64(buf, 0, &val);

        if (!error)
        {
            error = fpsensor_spi_debug_select(fpsensor,
                                              (fpsensor_reg_t)val);
        }
    }
    else if (fpsensor_attr->offset == offsetof(fpsensor_diag_t, spi_data))
    {

        if (fpsensor->diag.spi_regsize <= sizeof(val))
        {
            error = kstrtou64(buf, 0, &val);

            if (!error)
                error = fpsensor_spi_debug_value_write(fpsensor,
                                                       val);
        }
        else
        {
            error = fpsensor_spi_debug_buffer_write(fpsensor,
                                                    buf,
                                                    count);
        }

    }
    else if (fpsensor_attr->offset ==
             offsetof(fpsensor_diag_t, wakeup_system))
    {

        error = kstrtou64(buf, 0, &val);
        if (!error)
        {
//            fpsensor_trace( "%s, wakeup_system = 0x%x\n", __func__, (fpsensor_wakeup_level_t)val);
            status = fpsensor_input_wakeup(fpsensor, (fpsensor_wakeup_level_t)val);
            if (status)
            {
                return status;
            }
        }
    }
    else if (fpsensor_attr->offset ==
             offsetof(fpsensor_diag_t, gesture_key))
    {

        error = kstrtou64(buf, 0, &val);
        if (!error)
        {
            fpsensor_trace( "%s, gesture_key = 0x%llx\n", __func__, val);
            key = val & (0xff);
            downUp = (val >> 8) & 0x0f;
            sync = (val >> 12) & 0x0f;
            fpsensor_input_key(fpsensor, key, downUp, sync);
        }
    }
    else
    {
        error = -EPERM;
    }

    return (error < 0) ? error : strnlen(buf, count);
}


/* -------------------------------------------------------------------- */
static u8 fpsensor_selftest_short(fpsensor_data_t *fpsensor)
{
    const char *id_str = "selftest,";
    int error = 0;

    bool resume_input = false;
//    fpsensor_trace( "%s\n", __func__);
    if (fpsensor->input.enabled)
    {
        resume_input = true;
        fpsensor_worker_goto_idle(fpsensor);
    }

    fpsensor->diag.selftest = 0;

    error = fpsensor_wake_up(fpsensor);

    if (error)
    {
        fpsensor_error("%s wake up fail on entry.\n", id_str);
        goto out;
    }

    error = fpsensor_reset(fpsensor);

    if (error)
    {
        fpsensor_error("%s reset fail on entry.\n", id_str);
        goto out;
    }

    error = fpsensor_check_hw_id(fpsensor);

    if (error)
    {
        goto out;
    }

    error = fpsensor_cmd(fpsensor, FPSENSOR_CMD_CAPTURE_IMAGE,
                         false);//FPSENSOR_IRQ_REG_BIT_FIFO_NEW_DATA

    if (error < 0)
    {
        fpsensor_error("%s capture command failed.\n", id_str);
        goto out;
    }

    udelay(100);
    fpsensor_trace( "%s, delay 1000 us\n", __func__);

    error = fpsensor_gpio_read(fpsensor->irq_gpio) ? 0 : -EIO;

    if (error)
    {
        fpsensor_error("%s IRQ not HIGH after capture.\n", id_str);
        goto out;
    }

    error = fpsensor_wait_for_irq(fpsensor, FPSENSOR_DEFAULT_IRQ_TIMEOUT_MS);

    if (error)
    {
        fpsensor_error("%s IRQ-wait after capture failed.\n", id_str);
        goto out;
    }

    error = fpsensor_read_irq(fpsensor, true);

    if (error < 0)
    {
        fpsensor_error("%s IRQ clear fail\n", id_str);
        goto out;
    }
    else
    {
        error = 0;
    }

    error = (fpsensor_gpio_read(fpsensor->irq_gpio) == 0) ? 0 : -EIO;

    if (error)
    {
        fpsensor_error("%s IRQ not LOW after clear.\n", id_str);
        goto out;
    }

    error = fpsensor_reset(fpsensor);

    if (error)
    {
        fpsensor_error("%s reset fail on exit.\n", id_str);
        goto out;
    }

    error = fpsensor_read_status_reg(fpsensor);

    if (error != FPSENSOR_STATUS_REG_RESET_VALUE)
    {
        fpsensor_error("%s status check fail on exit.\n", id_str);
        goto out;
    }

    error = 0;

out:
    fpsensor->diag.selftest = (error == 0) ? 1 : 0;

    fpsensor_trace( "%s %s\n", id_str,
                    (fpsensor->diag.selftest) ? "PASS" : "FAIL");

    if (resume_input && fpsensor->diag.selftest)
    {
        fpsensor_start_input(fpsensor);
    }

    return fpsensor->diag.selftest;
};


/* -------------------------------------------------------------------- */
static int fpsensor_start_capture(fpsensor_data_t *fpsensor)
{
    fpsensor_capture_mode_t mode = fpsensor->setup.capture_mode;
    int error = 0;

    fpsensor_trace( "%s mode= %d\n", __func__, mode);

    /* Mode check (and pre-conditions if required) ? */
    switch (mode)
    {
        case FPSENSOR_MODE_WAIT_AND_CAPTURE:
        case FPSENSOR_MODE_SINGLE_CAPTURE:
        case FPSENSOR_MODE_CHECKERBOARD_TEST_NORM:
        case FPSENSOR_MODE_CHECKERBOARD_TEST_INV:
        case FPSENSOR_MODE_BOARD_TEST_ONE:
        case FPSENSOR_MODE_BOARD_TEST_ZERO:
        case FPSENSOR_MODE_WAIT_FINGER_DOWN:
        case FPSENSOR_MODE_WAIT_FINGER_UP:
        case FPSENSOR_MODE_SINGLE_CAPTURE_CAL:
        case FPSENSOR_MODE_CAPTURE_AND_WAIT_FINGER_UP:
        case FPSENSOR_MODE_GESTURE:
            break;

        case FPSENSOR_MODE_IDLE:
        default:
            error = -EINVAL;
            break;
    }

    fpsensor->capture.current_mode = (error >= 0) ? mode : FPSENSOR_MODE_IDLE;

    fpsensor->capture.state = FPSENSOR_CAPTURE_STATE_STARTED;
    fpsensor->capture.available_bytes  = 0;
    fpsensor->capture.read_offset = 0;
    fpsensor->capture.read_pending_eof = false;

    fpsensor_new_job(fpsensor, FPSENSOR_WORKER_CAPTURE_MODE);

    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_worker_goto_idle(fpsensor_data_t *fpsensor)
{
    const int wait_idle_us = 100;

    if (down_trylock(&fpsensor->worker.sem_idle))
    {
        fpsensor_trace( "%s, stop_request\n", __func__);

        fpsensor->worker.stop_request = true;
        fpsensor->worker.req_mode = FPSENSOR_WORKER_IDLE_MODE;

        while (down_trylock(&fpsensor->worker.sem_idle))
        {

            fpsensor->worker.stop_request = true;
            fpsensor->worker.req_mode = FPSENSOR_WORKER_IDLE_MODE;

            //usleep(wait_idle_us);
            usleep_range(wait_idle_us, wait_idle_us + 50);

        }
        fpsensor_trace( "%s, is idle\n", __func__);
        up(&fpsensor->worker.sem_idle);

    }
    else
    {
        fpsensor_trace( "%s, already idle\n", __func__);
        up(&fpsensor->worker.sem_idle);
    }

    return 0;
}


/* -------------------------------------------------------------------- */
static int fpsensor_new_job(fpsensor_data_t *fpsensor, int new_job)
{
    fpsensor_trace( "%s %d\n", __func__, new_job);

    fpsensor_worker_goto_idle(fpsensor);

    fpsensor->worker.req_mode = new_job;
    fpsensor->worker.stop_request = false;

    wake_up_interruptible(&fpsensor->worker.wq_wait_job);

    return 0;
}


/* -------------------------------------------------------------------- */
static int fpsensor_worker_function(void *_fpsensor)
{
    fpsensor_data_t *fpsensor = _fpsensor;

    while (!kthread_should_stop())
    {

        up(&fpsensor->worker.sem_idle);

        wait_event_interruptible(fpsensor->worker.wq_wait_job,
                                 fpsensor->worker.req_mode != FPSENSOR_WORKER_IDLE_MODE);

        down(&fpsensor->worker.sem_idle);

        switch (fpsensor->worker.req_mode)
        {
            case FPSENSOR_WORKER_CAPTURE_MODE:
                fpsensor->capture.state = FPSENSOR_CAPTURE_STATE_PENDING;
                fpsensor_capture_task(fpsensor);
                break;

            case FPSENSOR_WORKER_INPUT_MODE:
                if (fpsensor_capture_deferred_task(fpsensor) != -EINTR)
                {
                    fpsensor_input_enable(fpsensor, true);
                    fpsensor_input_task(fpsensor);
                }
                break;
            case FPSENSOR_WORKER_IDLE_MODE:
            case FPSENSOR_WORKER_EXIT:
            default:
                break;
        }

        if (fpsensor->worker.req_mode != FPSENSOR_WORKER_EXIT)
        {
            fpsensor->worker.req_mode = FPSENSOR_WORKER_IDLE_MODE;
        }
    }

    return 0;
}


/* -------------------------------------------------------------------- */
/* SPI debug interface, implementation                  */
/* -------------------------------------------------------------------- */
static int fpsensor_spi_debug_select(fpsensor_data_t *fpsensor, fpsensor_reg_t reg)
{
    u8 size = FPSENSOR_REG_SIZE(reg);
    fpsensor_trace( "%s\n", __func__);

    if (size)
    {
        fpsensor->diag.spi_register = reg;
        fpsensor->diag.spi_regsize  = size;

        fpsensor_trace( "%s : selected %d (%d byte(s))\n",
                        __func__
                        , fpsensor->diag.spi_register
                        , fpsensor->diag.spi_regsize);
        return 0;
    }
    else
    {
        fpsensor_trace("%s : reg %d not available\n", __func__, reg);

        return -ENOENT;
    }
}


/* -------------------------------------------------------------------- */
static int fpsensor_spi_debug_value_write(fpsensor_data_t *fpsensor, u64 data)
{
    int error = 0;
    fpsensor_reg_access_t reg;

    fpsensor_trace( "%s\n", __func__);

    FPSENSOR_MK_REG_WRITE_BYTES(reg,
                                fpsensor->diag.spi_register,
                                fpsensor->diag.spi_regsize,
                                (u8 *)&data);

    error = fpsensor_reg_access(fpsensor, &reg);

    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_spi_debug_buffer_write(fpsensor_data_t *fpsensor,
                                           const char *data, size_t count)
{
    int error = 0;
    fpsensor_reg_access_t reg;
    u8 u8_buffer[FPSENSOR_REG_MAX_SIZE];

    fpsensor_trace( "%s\n", __func__);

    error = fpsensor_spi_debug_hex_string_to_buffer(u8_buffer,
                                                    sizeof(u8_buffer),
                                                    data,
                                                    count);

    if (error < 0)
    {
        return error;
    }

    FPSENSOR_MK_REG_WRITE_BYTES(reg,
                                fpsensor->diag.spi_register,
                                fpsensor->diag.spi_regsize,
                                u8_buffer);

    error = fpsensor_reg_access(fpsensor, &reg);

    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_spi_debug_value_read(fpsensor_data_t *fpsensor, u64 *data)
{
    int error = 0;
    fpsensor_reg_access_t reg;

    fpsensor_trace( "%s\n", __func__);

    *data = 0;

    FPSENSOR_MK_REG_READ_BYTES(reg,
                               fpsensor->diag.spi_register,
                               fpsensor->diag.spi_regsize,
                               (u8 *)data);

    error = fpsensor_reg_access(fpsensor, &reg);

    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_spi_debug_buffer_read(fpsensor_data_t *fpsensor,
                                          u8 *data, size_t max_count)
{
    int error = 0;
    fpsensor_reg_access_t reg;
    fpsensor_trace( "%s\n", __func__);

    if (max_count < fpsensor->diag.spi_regsize)
    {
        return -ENOMEM;
    }

    FPSENSOR_MK_REG_READ_BYTES(reg,
                               fpsensor->diag.spi_register,
                               fpsensor->diag.spi_regsize,
                               data);

    error = fpsensor_reg_access(fpsensor, &reg);

    return error;
}


/* -------------------------------------------------------------------- */
static void fpsensor_spi_debug_buffer_to_hex_string(char *string,
                                                    u8 *buffer,
                                                    size_t bytes)
{
    int count = bytes;
    int pos = 0;
    int src = (target_little_endian) ? (bytes - 1) : 0;
    u8 v1, v2;
    fpsensor_trace("%s\n", __func__);

    string[pos++] = '0';
    string[pos++] = 'x';

    while (count)
    {
        v1 = buffer[src] >> 4;
        v2 = buffer[src] & 0x0f;

        string[pos++] = (v1 >= 0x0a) ? ('a' - 0x0a + v1) : ('0' + v1);
        string[pos++] = (v2 >= 0x0a) ? ('a' - 0x0a + v2) : ('0' + v2);

        src += (target_little_endian) ? -1 : 1;

        count--;
    }

    string[pos] = '\0';
}


/* -------------------------------------------------------------------- */
static u8 fpsensor_char_to_u8(char in_char)
{
    if ((in_char >= 'A') && (in_char <= 'F'))
    {
        return (u8)(in_char - 'A' + 0xa);
    }

    if ((in_char >= 'a') && (in_char <= 'f'))
    {
        return (u8)(in_char - 'a' + 0xa);
    }

    if ((in_char >= '0') && (in_char <= '9'))
    {
        return (u8)(in_char - '0');
    }

    return 0;
}


/* -------------------------------------------------------------------- */
static int fpsensor_spi_debug_hex_string_to_buffer(u8 *buffer,
                                                   size_t buf_size,
                                                   const char *string,
                                                   size_t chars)
{
    int bytes = 0;
    int count;
    int dst = (target_little_endian) ? 0 : (buf_size - 1);
    int pos;
    u8 v1, v2;
    fpsensor_trace( "%s\n", __func__);

    if (string[1] != 'x' && string[1] != 'X')
    {
        return -EINVAL;
    }

    if (string[0] != '0')
    {
        return -EINVAL;
    }

    if (chars < sizeof("0x1"))
    {
        return -EINVAL;
    }

    count = buf_size;
    while (count)
    {
        buffer[--count] = 0;
    }

    count = chars - sizeof("0x");

    bytes = ((count % 2) == 0) ? (count / 2) : (count / 2) + 1;

    if (bytes > buf_size)
    {
        return -EINVAL;
    }

    pos = chars - 2;

    while (pos >= 2)
    {
        v1 = fpsensor_char_to_u8(string[pos--]);
        v2 = (pos >= 2) ? fpsensor_char_to_u8(string[pos--]) : 0;

        buffer[dst] = (v2 << 4) | v1;

        dst += (target_little_endian) ? 1 : -1;
    }
    return bytes;
}


/* -------------------------------------------------------------------- */
static int fpsensor_start_input(fpsensor_data_t *fpsensor)
{
    return fpsensor_new_job(fpsensor, FPSENSOR_WORKER_INPUT_MODE);
}


/* -------------------------------------------------------------------- */


