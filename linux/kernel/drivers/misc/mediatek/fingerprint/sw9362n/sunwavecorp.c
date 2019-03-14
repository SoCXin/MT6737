/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *  Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

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
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>

#include <asm/uaccess.h>
#include "sunwavecorp.h"
#include "config.h"
#include <linux/wakelock.h>
#include <linux/delay.h>

#if __SUNWAVE_QUIK_WK_CPU_EN
//add for open core begin
#include <linux/cpu.h>
#include <linux/workqueue.h>
//add for open core end
#endif
#if __PALTFORM_SPREAD_EN
#include <linux/of.h>
#endif
#define __devexit
#define __devinitdata
#define __devinit
#define __devexit_p

#ifdef _DRV_TAG_
#undef _DRV_TAG_
#endif
#define _DRV_TAG_ "corp"

#include <linux/platform_device.h>

#if __PALTFORM_SPREAD_EN
#include <soc/sprd/sci.h>
#include <soc/sprd/sci_glb_regs.h>
#include <soc/sprd/adi.h>
#include <soc/sprd/adc.h>
#include <linux/regulator/consumer.h>
#endif
/*
 * This supports access to SPI devices using normal userspace I/O calls.
 * Note that while traditional UNIX/POSIX I/O semantics are half duplex,
 * and often mask message boundaries, full SPI support requires full duplex
 * transfers.  There are several kinds of internal message boundaries to
 * handle chipselect management and other protocol options.
 *
 * SPI has a character major number assigned.  We allocate minor numbers
 * dynamically using a bitmask.  You must use hotplug tools, such as udev
 * (or mdev with busybox) to create and destroy the /dev/sunwave_devB.C device
 * nodes, since there is no fixed association of minor numbers with any
 * particular SPI bus or device.
 */

#if __SUNWAVE_HW_INFO_EN

#if __SUNWAVE_PRIZE_HW_EN

#include "../../hardware_info/hardware_info.h"
extern struct hardware_info current_fingerprint_info;
static int read_id;

#else

struct hardware_info {
    char chip[16];
    char id[16];
    char vendor[16];
    char more[16];
};
static struct hardware_info current_fingerprint_info;
static int read_id;
#endif

#endif //__SUNWAVE_HW_INFO_EN
static LIST_HEAD(sunwave_device_list);
static DEFINE_MUTEX(sunwave_device_list_lock);
static sunwave_sensor_t* g_sunwave_sensor;
static int sunwave_driver_status = 0;

u8 suspend_flag = 0;
sunwave_sensor_t* get_current_sunwave(void)
{
    return g_sunwave_sensor;
}
EXPORT_SYMBOL_GPL(get_current_sunwave);

static unsigned bufsiz = 25 * 1024;
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

#if __SUNWAVE_QUIK_WK_CPU_EN
//add for open core begin
//static struct workqueue_struct *core_queue=NULL;
//static struct work_struct core_work;
struct workqueue_struct* core_queue = NULL;
struct work_struct core_work;
//add for open core end
#endif



/*
 * We can't use the standard synchronous wrappers for file I/O; we
 * need to protect against async removal of the underlying spi_device.
 */
static void sunwave_dev_complete(void* arg)
{
    complete(arg);
}

static ssize_t
sunwave_dev_sync(sunwave_sensor_t* sunwave_dev, struct spi_message* message)
{
    DECLARE_COMPLETION_ONSTACK(done);
    int status;
    message->complete = sunwave_dev_complete;
    message->context = &done;
    spin_lock_irq(&sunwave_dev->spi_lock);

    if (sunwave_dev->spi == NULL) {
        sw_err("spi device is NULL");
        status = -ESHUTDOWN;
    }
    else {
        status = spi_async(sunwave_dev->spi, message);
    }

    spin_unlock_irq(&sunwave_dev->spi_lock);

    if (status == 0) {
        wait_for_completion(&done);
        status = message->status;

        if (status == 0) {
            status = message->actual_length;
        }
        else {
            sw_err("spi async return error %d", status);
        }
    }
    else {
        sw_err("spi async error %d", status);
    }

    return status;
}

static inline ssize_t
sunwave_dev_sync_write(sunwave_sensor_t* sunwave_dev, size_t len)
{
    int ret;
    struct spi_transfer t = {
        .tx_buf     = sunwave_dev->buffer,
        .len        = len,
    };
    struct spi_message  m;
    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    ret = sunwave_dev_sync(sunwave_dev, &m);

    if (ret == 0) {
        ret = len;
    }

    return ret;
}

static inline ssize_t
sunwave_dev_sync_read(sunwave_sensor_t* sunwave_dev, size_t len)
{
    int ret;
    struct spi_transfer t = {
        .tx_buf     = sunwave_dev->buffer,
        .rx_buf     = sunwave_dev->buffer,
        .len        = len,
    };
    struct spi_message  m;
    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    ret = sunwave_dev_sync(sunwave_dev, &m);

    if (ret == 0) {
        ret = len;
    }

    return ret;
}

static inline ssize_t
sunwave_dev_wr(sunwave_sensor_t* sunwave_dev, u8* buf, u16 buflen)
{
    if (sunwave_dev->finger && sunwave_dev->finger->write_then_read == 0) {
        struct spi_transfer t = {
            .tx_buf     = buf,
            .rx_buf     = buf,
            .len        = buflen,
        };
        struct spi_message  m;
        spi_message_init(&m);
        spi_message_add_tail(&t, &m);
        return sunwave_dev_sync(sunwave_dev, &m);
    }

    //sunwave_dev->finger->write_then_read==1
    return spi_write_then_read(sunwave_dev->spi, buf, 8, buf + 8, buflen - 8);
}

static ssize_t sunwave_version_show(struct device* dev,
                                    struct device_attribute* attr, char* buf)
{
    int len = 0;
    //printk("%s.\n",SUNWAVE_VERSION);
    len += sprintf(buf, "%s\n", SUNWAVE_VERSION);
    return len;
}
int sunwave_debug_level = 0;
static ssize_t sunwave_debug_show(struct device* dev,
                                  struct device_attribute* attr, char* buf)
{
    int len = 0;
    //printk("debug_level = %d\n", sunwave_debug_level);
    len += sprintf((char*)buf, "debug_level = %d\n", sunwave_debug_level);
    return len;
}

static ssize_t sunwave_debug_store(struct device* dev,
                                   struct device_attribute* attr, const char* buf, size_t count)
{
    sscanf(buf, "%d", &sunwave_debug_level);
    //printk("Store. debug_level = %d\n", sunwave_debug_level);
    return strnlen(buf, count);
}

static ssize_t sunwave_chip_info_show(struct device* dev,
                                      struct device_attribute* attr, char* buf)
{
    int len = 0;
    len += sprintf((char*)buf, "chip   : %s\nid     : %s\nvendor : %s\nmore   : %s\n",
                   current_fingerprint_info.chip,
                   current_fingerprint_info.id,
                   current_fingerprint_info.vendor,
                   current_fingerprint_info.more);
    return len;
}
static DEVICE_ATTR(version, S_IRUGO | S_IWUSR, sunwave_version_show, NULL);
static DEVICE_ATTR(debug, S_IRUGO | S_IWUSR, sunwave_debug_show, sunwave_debug_store);
static DEVICE_ATTR(chip_info, S_IRUGO | S_IWUSR, sunwave_chip_info_show, NULL);


static struct attribute* sunwave_tools_attrs[] = {
    &dev_attr_version.attr,
    &dev_attr_debug.attr,
    &dev_attr_chip_info.attr,
    NULL
};

static const struct attribute_group sunwave_attr_group = {
    .attrs = sunwave_tools_attrs,
    .name = "sunwave"
};

/*-------------------------------------------------------------------------*/
/* Read-only message with current device setup */
static ssize_t
sunwave_dev_read(struct file* filp, char __user* buf, size_t count, loff_t* f_pos)
{
    sunwave_sensor_t*    sunwave_dev;
    ssize_t         status = 0;

    /* chipselect only toggles at start or end of operation */
    if (count > bufsiz) {
        return -EMSGSIZE;
    }

    sunwave_dev = filp->private_data;
    mutex_lock(&sunwave_dev->buf_lock);
    memset(sunwave_dev->buffer, 0, bufsiz);

    if (copy_from_user(sunwave_dev->buffer, buf, count)) {
        status = -EMSGSIZE;
        sw_err("cpoy from user");
        goto cpy_error;
    }

    if (sunwave_dev->finger && sunwave_dev->finger->write_then_read == 0) {
        status = sunwave_dev_sync_read(sunwave_dev, count);

        if (status >= 0) {
            unsigned long   missing;
            missing = copy_to_user(buf, sunwave_dev->buffer, status);
            //sw_dbg("ret %x missing = %ld status = %d", (buf[status - 2] << 8) | buf[status - 1], missing, status);

            if (missing == status) {
                status = -EFAULT;
            }
            else {
                status = status - missing;
            }
        }
    }
    else {
        status = spi_write_then_read(sunwave_dev->spi, sunwave_dev->buffer, 8, sunwave_dev->buffer + 8, count - 8);

        if (status > 0) {
            unsigned long   missing;
            missing = copy_to_user(buf + 8, sunwave_dev->buffer + 8, count - 8);

            if (missing == status) {
                status = -EFAULT;
            }
            else {
                status = status - missing;
            }
        }
    }

cpy_error:
    mutex_unlock(&sunwave_dev->buf_lock);
    return status;
}

/* Write-only message with current device setup */
static ssize_t
sunwave_dev_write(struct file* filp, const char __user* buf,
                  size_t count, loff_t* f_pos)
{
    sunwave_sensor_t*    sunwave_dev;
    ssize_t         status = 0;
    unsigned long       missing;

    /* chipselect only toggles at start or end of operation */
    if (count > bufsiz) {
        sw_err("write length lage than buff") ;
        return -EMSGSIZE;
    }

    sunwave_dev = filp->private_data;
    mutex_lock(&sunwave_dev->buf_lock);
    memset(sunwave_dev->buffer, 0, bufsiz);
    missing = copy_from_user(sunwave_dev->buffer, buf, count);

    if (missing == 0) {
        status = sunwave_dev_sync_write(sunwave_dev, count);
    }
    else {
        sw_err("copy from user") ;
        status = -EFAULT;
    }

    mutex_unlock(&sunwave_dev->buf_lock);
    return status;
}




static int sunwave_dev_message(sunwave_sensor_t* sunwave_dev,
                               struct spi_ioc_transfer* u_xfers, unsigned n_xfers)
{
    struct spi_message  msg;
    struct spi_transfer* k_xfers;
    struct spi_transfer* k_tmp;
    struct spi_ioc_transfer* u_tmp;
    unsigned        n, total;
    u8*          buf;
    int         status = -EFAULT;
    spi_message_init(&msg);
    k_xfers = kcalloc(n_xfers, sizeof(*k_tmp), GFP_KERNEL);

    if (k_xfers == NULL) {
        return -ENOMEM;
    }

    /* Construct spi_message, copying any tx data to bounce buffer.
     * We walk the array of user-provided transfers, using each one
     * to initialize a kernel version of the same transfer.
     */
    buf = sunwave_dev->buffer;
    total = 0;

    for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
         n;
         n--, k_tmp++, u_tmp++) {
        k_tmp->len = u_tmp->len;
        total += k_tmp->len;

        if (total > bufsiz) {
            status = -EMSGSIZE;
            goto done;
        }

        if (u_tmp->rx_buf) {
            k_tmp->rx_buf = buf;

            if (!access_ok(VERIFY_WRITE, (u8 __user*)
                           (uintptr_t) u_tmp->rx_buf,
                           u_tmp->len)) {
                goto done;
            }
        }

        if (u_tmp->tx_buf) {
            k_tmp->tx_buf = buf;

            if (copy_from_user(buf, (const u8 __user*)
                               (uintptr_t) u_tmp->tx_buf,
                               u_tmp->len)) {
                goto done;
            }
        }

        buf += k_tmp->len;
        k_tmp->cs_change = !!u_tmp->cs_change;
        k_tmp->bits_per_word = u_tmp->bits_per_word;
        k_tmp->delay_usecs = u_tmp->delay_usecs;
        k_tmp->speed_hz = u_tmp->speed_hz;
#ifdef VERBOSE
        dev_dbg(&sunwave_dev->spi->dev,
                "  xfer len %zd %s%s%s%dbits %u usec %uHz\n",
                u_tmp->len,
                u_tmp->rx_buf ? "rx " : "",
                u_tmp->tx_buf ? "tx " : "",
                u_tmp->cs_change ? "cs " : "",
                u_tmp->bits_per_word ? : sunwave_dev->spi->bits_per_word,
                u_tmp->delay_usecs,
                u_tmp->speed_hz ? : sunwave_dev->spi->max_speed_hz);
#endif
        spi_message_add_tail(k_tmp, &msg);
    }

    status = sunwave_dev_sync(sunwave_dev, &msg);

    if (status < 0) {
        goto done;
    }

    /* copy any rx data out of bounce buffer */
    buf = sunwave_dev->buffer;

    for (n = n_xfers, u_tmp = u_xfers; n; n--, u_tmp++) {
        if (u_tmp->rx_buf) {
            if (__copy_to_user((u8 __user*)
                               (uintptr_t) u_tmp->rx_buf, buf,
                               u_tmp->len)) {
                status = -EFAULT;
                goto done;
            }
        }

        buf += u_tmp->len;
    }

    status = total;
done:
    kfree(k_xfers);
    return status;
}

static long
sunwave_dev_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    int         err = 0;
    int         retval = 0;
    sunwave_sensor_t*    sunwave_dev;
    struct spi_device*   spi;
    u32         tmp;
    unsigned        n_ioc;
    struct spi_ioc_transfer* ioc;

    /* Check type and command number */
    if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC || _IOC_TYPE(cmd) !=  SUNWAVE_IOC_MAGIC) {
        return -ENOTTY;
    }

    /* Check access direction once here; don't repeat below.
     * IOC_DIR is from the user perspective, while access_ok is
     * from the kernel perspective; so they look reversed.
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE,
                         (void __user*)arg, _IOC_SIZE(cmd));

    if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ,
                         (void __user*)arg, _IOC_SIZE(cmd));

    if (err) {
        return -EFAULT;
    }

    /* guard against device removal before, or while,
     * we issue this ioctl.
     */
    sunwave_dev = filp->private_data;
    spin_lock_irq(&sunwave_dev->spi_lock);
    spi = spi_dev_get(sunwave_dev->spi);
    spin_unlock_irq(&sunwave_dev->spi_lock);

    if (spi == NULL) {
        return -ESHUTDOWN;
    }

    /* use the buffer lock here for triple duty:
     *  - prevent I/O (from us) so calling spi_setup() is safe;
     *  - prevent concurrent SPI_IOC_WR_* from morphing
     *    data fields while SPI_IOC_RD_* reads them;
     *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
     */
    mutex_lock(&sunwave_dev->buf_lock);

    switch (cmd) {
        /* read requests */
        case SPI_IOC_RD_MODE:
            retval = __put_user(spi->mode & SPI_MODE_MASK,
                                (__u8 __user*)arg);
            break;

        case SPI_IOC_RD_LSB_FIRST:
            retval = __put_user((spi->mode & SPI_LSB_FIRST) ?  1 : 0,
                                (__u8 __user*)arg);
            break;

        case SPI_IOC_RD_BITS_PER_WORD:
            retval = __put_user(spi->bits_per_word, (__u8 __user*)arg);
            break;

        case SPI_IOC_RD_MAX_SPEED_HZ:
            retval = __put_user(spi->max_speed_hz, (__u32 __user*)arg);
            break;

        /* write requests */
        case SPI_IOC_WR_MODE:
            retval = __get_user(tmp, (u8 __user*)arg);

            if (retval == 0) {
                u8  save = spi->mode;

                if (tmp & ~SPI_MODE_MASK) {
                    retval = -EINVAL;
                    break;
                }

                tmp |= spi->mode & ~SPI_MODE_MASK;
                spi->mode = (u8)tmp;
                retval = spi_setup(spi);

                if (retval < 0) {
                    spi->mode = save;
                }
                else {
                    dev_dbg(&spi->dev, "spi mode %02x\n", tmp);
                }
            }

            break;

        case SPI_IOC_WR_LSB_FIRST:
            retval = __get_user(tmp, (__u8 __user*)arg);

            if (retval == 0) {
                u8  save = spi->mode;

                if (tmp) {
                    spi->mode |= SPI_LSB_FIRST;
                }
                else {
                    spi->mode &= ~SPI_LSB_FIRST;
                }

                retval = spi_setup(spi);

                if (retval < 0) {
                    spi->mode = save;
                }
                else
                    dev_dbg(&spi->dev, "%csb first\n",
                            tmp ? 'l' : 'm');
            }

            break;

        case SPI_IOC_WR_BITS_PER_WORD:
            retval = __get_user(tmp, (__u8 __user*)arg);

            if (retval == 0) {
                u8  save = spi->bits_per_word;
                spi->bits_per_word = tmp;
                retval = spi_setup(spi);

                if (retval < 0) {
                    spi->bits_per_word = save;
                }
                else {
                    dev_dbg(&spi->dev, "%d bits per word\n", tmp);
                }
            }

            break;

        case SPI_IOC_WR_MAX_SPEED_HZ:
            retval = __get_user(tmp, (__u32 __user*)arg);

            if (retval == 0) {
                u32 save = spi->max_speed_hz;
                spi->max_speed_hz = tmp;

                if (sunwave_dev->finger && sunwave_dev->finger->speed) {
                    sunwave_dev->finger->speed(&sunwave_dev->spi, tmp);
                }

                retval = spi_setup(spi);

                if (retval < 0) {
                    spi->max_speed_hz = save;
                }
                else {
                    dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
                }
            }

            break;

        case SUNWAVE_IOC_SET_SENSOR_STATUS:
            break;

        case SUNWAVE_IOC_GET_SENSOR_STATUS:
            break;

        case SUNWAVE_IOC_IS_FINGER_ON:
            break;

        case SUNWAVE_WRITE_READ_DATA: {
            struct sunwave_rw_operate  operate;

            if (sunwave_dev->buffer == NULL) {
                return -ENOMEM;
            }

            if (__copy_from_user(&operate, (void __user*)arg, sizeof(struct sunwave_rw_operate))) {
                return -EFAULT;
            }

            if (operate.len > bufsiz) {
                return -EFAULT;
            }

            if (__copy_from_user(sunwave_dev->buffer, (void __user*)operate.buf,  operate.len)) {
                return -EFAULT;
            }

            retval = sunwave_dev_wr(sunwave_dev, sunwave_dev->buffer, operate.len);

            if (retval < 0) {
                return retval;
            }

            retval = __copy_to_user((__u8 __user*)operate.buf,  sunwave_dev->buffer, operate.len);

            if (retval < 0) {
                dev_dbg(&spi->dev, "SUNWAVE_FP_READ_DATA: error copying to user\n");
            }
        }
        break;

        case SUNWAVE_IOC_RST_SENSOR:
            if (sunwave_dev->finger && sunwave_dev->finger->reset) {
                sunwave_dev->finger->reset(&sunwave_dev->spi);
            }

            break;

        case SUNWAVE_SET_INTERRUPT_WAKE_STATUS:
            break;

        case SUNWAVE_KEY_REPORT: {
            char key[3];

            if (__copy_from_user(key, (void __user*)arg, 2)) {
                return -EFAULT;
            }

            sunwave_key_report(sunwave_dev, key[1], key[0] ? 1 : 0);
        }
        break;

        case SUNWAVE_WAKEUP_SYSTEM:
            sunwave_wakeupSys(sunwave_dev);
            break;

        case SUNWAVE_IOC_ATTRIBUTE:
            retval = __put_user(sunwave_dev->finger->attribute, (__u32 __user*)arg);
            break;
#if __SUNWAVE_HW_INFO_EN

        case SUNWAVE_SET_VERSION_INFO: {
            char sw_version_info[32];

            if (__copy_from_user(sw_version_info, (void __user*)arg, 32)) {
                return -EFAULT;
            }

            if (1 == sw_version_info[0]) {
                sprintf(current_fingerprint_info.chip, "%s", &sw_version_info[1]);
            }
            else if (2 == sw_version_info[0]) {
                sprintf(current_fingerprint_info.id, "0x%x %s", read_id, &sw_version_info[1]);
            }
            else if (3 == sw_version_info[0]) {
                sprintf(current_fingerprint_info.vendor, "%s", &sw_version_info[1]);
            }

            break;
        }

#endif //__SUNWAVE_HW_INFO_EN       

        default:

            /* segmented and/or full-duplex I/O request */
            if (_IOC_NR(cmd) != _IOC_NR(SPI_IOC_MESSAGE(0))
                || _IOC_DIR(cmd) != _IOC_WRITE) {
                retval = -ENOTTY;
                break;
            }

            tmp = _IOC_SIZE(cmd);

            if ((tmp % sizeof(struct spi_ioc_transfer)) != 0) {
                retval = -EINVAL;
                break;
            }

            n_ioc = tmp / sizeof(struct spi_ioc_transfer);

            if (n_ioc == 0) {
                break;
            }

            /* copy into scratch area */
            ioc = kmalloc(tmp, GFP_KERNEL);

            if (!ioc) {
                retval = -ENOMEM;
                break;
            }

            if (__copy_from_user(ioc, (void __user*)arg, tmp)) {
                kfree(ioc);
                retval = -EFAULT;
                break;
            }

            /* translate to spi_message, execute */
            retval = sunwave_dev_message(sunwave_dev, ioc, n_ioc);
            kfree(ioc);
            break;
    }

    mutex_unlock(&sunwave_dev->buf_lock);
    spi_dev_put(spi);
    return retval;
}

#ifdef CONFIG_COMPAT
static long
sunwave_dev_compat_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    return sunwave_dev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define sunwave_dev_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int sunwave_dev_open(struct inode* inode, struct file* filp)
{
    sunwave_sensor_t*    sunwave_dev;
    int         status = -ENXIO;
    mutex_lock(&sunwave_device_list_lock);
    list_for_each_entry(sunwave_dev, &sunwave_device_list, device_entry) {
        if (sunwave_dev->devt == inode->i_rdev) {
            status = 0;
            break;
        }
    }

    /* suport users: service and test */
    if (sunwave_dev->users > 1) {
        mutex_unlock(&sunwave_device_list_lock);
        pr_debug(KERN_ERR "Spi sensor: %s: Too many users\n", __func__);
        return -EPERM;
    }

    if (status == 0) {
        if (!sunwave_dev->buffer) {
            sunwave_dev->buffer = kmalloc(bufsiz, GFP_KERNEL);

            if (!sunwave_dev->buffer) {
                dev_dbg(&sunwave_dev->spi->dev, "open/ENOMEM\n");
                status = -ENOMEM;
            }
        }

        if (status == 0) {
            sunwave_dev->users++;
            filp->private_data = sunwave_dev;
            nonseekable_open(inode, filp);
        }
    }
    else {
        pr_debug("sunwave_dev: nothing for minor %d\n", iminor(inode));
    }

    mutex_unlock(&sunwave_device_list_lock);

    if (status == 0) {
        if (sunwave_dev->finger && sunwave_dev->finger->reset) {
            sunwave_dev->finger->reset(&sunwave_dev->spi);
        }
    }

    g_sunwave_sensor = sunwave_dev;
    return status;
}

static int sunwave_dev_release(struct inode* inode, struct file* filp)
{
    sunwave_sensor_t*    sunwave_dev;
    int         status = 0;
    mutex_lock(&sunwave_device_list_lock);
    sunwave_dev = filp->private_data;
    filp->private_data = NULL;
    /* last close? */
    sunwave_dev->users--;

    if (!sunwave_dev->users) {
        int     dofree;
        kfree(sunwave_dev->buffer);
        sunwave_dev->buffer = NULL;
        /* ... after we unbound from the underlying device? */
        spin_lock_irq(&sunwave_dev->spi_lock);
        dofree = (sunwave_dev->spi == NULL);
        spin_unlock_irq(&sunwave_dev->spi_lock);

        if (dofree) {
            kfree(sunwave_dev);
        }
    }

    mutex_unlock(&sunwave_device_list_lock);
    return status;
}

static const struct file_operations sunwave_dev_fops = {
    .owner =    THIS_MODULE,
    /* REVISIT switch to aio primitives, so that userspace
     * gets more complete API coverage.  It'll simplify things
     * too, except for the locking.
     */
    .write =    sunwave_dev_write,
    .read =     sunwave_dev_read,
    .unlocked_ioctl = sunwave_dev_ioctl,
    .compat_ioctl = sunwave_dev_compat_ioctl,
    .open =     sunwave_dev_open,
    .release =  sunwave_dev_release,
    .llseek =   no_llseek,
};

static struct miscdevice sunwave_misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "sunwave_fp",
    .fops = &sunwave_dev_fops,
};


/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/sunwave_devB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

//static struct class* sunwave_dev_class;

/*-------------------------------------------------------------------------*/
static int sunwave_suspend(struct spi_device* spi, pm_message_t mesg)
{
    suspend_flag = 1;
    return 0;
}

static int sunwave_resume(struct spi_device* spi)
{
    return 0;
}

#if __SUNWAVE_DETECT_ID_EN

//#define __CHECK_ESD_REG__

//add by Sea 20160505
static int sunwave_read_id_fw(sunwave_sensor_t* sunwave_dev)
{
    ssize_t  status = 0;
    unsigned char wbuff[10] = {0};
    int err_cnt = 0;
    int reg;
    int tryTime;
    sunwave_dev->buffer = kzalloc(10, GFP_KERNEL);
    sw_info("sunwave_read_id_fw");

    do {
        if (sunwave_dev->finger && sunwave_dev->finger->reset) {
            sunwave_dev->finger->reset(&sunwave_dev->spi);
        }

        err_cnt++;
        tryTime = 3;

        while (tryTime--) {
            msleep(220);
            sunwave_dev->buffer[0] = 0x1C;
            sunwave_dev->buffer[1] = 0x1C;
            sunwave_dev->buffer[2] = 0x1C;
            //wake up
            sunwave_dev_sync_write(sunwave_dev, 3);
#ifndef __CHECK_ESD_REG__
            memset(sunwave_dev->buffer, 0, 10);
            wbuff[0] = 0x58;
            wbuff[1] = 0xA7;
            wbuff[2] = 0x00;
            wbuff[3] = (0 >> 8);
            wbuff[4] =  0;
            wbuff[5] = 0x00;
            wbuff[6] = 0x02;
            wbuff[7] =  ((0xFAA0 >> 8) & 0xFF);
            wbuff[8] =  (0xFAA0 & 0xFF);
            memcpy(sunwave_dev->buffer, wbuff, 9);
            status = sunwave_dev_sync_write(sunwave_dev, 9);

            if (status <= 0) {
                kfree(sunwave_dev->buffer);
                sunwave_dev->buffer = NULL;
                return -2;
            }

            sw_info("test register 0");
#else
            sw_info("test register esd");
#endif
            memset(sunwave_dev->buffer, 0, 10);
            wbuff[0] = 0x96;
            wbuff[1] = 0x69;
            wbuff[2] = 0x00;
#ifdef __CHECK_ESD_REG__
            wbuff[3] = (0x1E >> 8) & 0xff; //ID_SENSOR_ID 0x08  ESD 0x1E
            wbuff[4] = 0x1E & 0xff;
#else
            wbuff[3] = (0 >> 8) & 0xff;
            wbuff[4] = 0 & 0xff;
#endif
            wbuff[5] = 0x00;
            wbuff[6] = 0x02;
            wbuff[7] = 0x00;
            memcpy(sunwave_dev->buffer, wbuff, 8);

            if (sunwave_dev->finger && sunwave_dev->finger->write_then_read == 0) {
                status = sunwave_dev_sync_read(sunwave_dev, 10);

                if (status <= 0) {
                    kfree(sunwave_dev->buffer);
                    sunwave_dev->buffer = NULL;
                    return -2;
                }
            }
            else {
                status = spi_write_then_read(sunwave_dev->spi, sunwave_dev->buffer, 8, sunwave_dev->buffer + 8, 2);

                if (status <= 0) {
                    kfree(sunwave_dev->buffer);
                    sunwave_dev->buffer = NULL;
                    return -2;
                }
            }

            reg = ((sunwave_dev->buffer[8] << 8) | sunwave_dev->buffer[9]);
            sw_info("*** read sunwave_fp id is %4x ***", reg);
            sw_info("err_cnt:%d, tryTime:%d", err_cnt, tryTime);
#if __SUNWAVE_HW_INFO_EN
            read_id = reg;
#endif

            if (reg == 0xFAA0) {//0xFAA0  0xFA00
                kfree(sunwave_dev->buffer);
                sunwave_dev->buffer = NULL;
                return 0;
            }
        }
    }
    while (err_cnt < 3);

    kfree(sunwave_dev->buffer);
    sunwave_dev->buffer = NULL;
    return -2;
}

//add by zsx
static int sunwave_read_id_8221(sunwave_sensor_t* sunwave_dev)
{
    ssize_t  status = 0;
    unsigned char wbuff[4] = {0};
    int err_cnt = 0;
    int reg;
    int tryTime;
    sunwave_dev->buffer = kzalloc(6, GFP_KERNEL);
    sw_info("sunwave_read_id_8221");

    do {
        if (sunwave_dev->finger && sunwave_dev->finger->reset) {
            sunwave_dev->finger->reset(&sunwave_dev->spi);
        }

        tryTime = 3;

        while (tryTime--) {
            msleep(5);
            memset(sunwave_dev->buffer, 0, 6);
            wbuff[0] = 0x60;
            wbuff[1] = 0x28;
            wbuff[2] = 0x02;
            wbuff[3] = 0x00;
            memcpy(sunwave_dev->buffer, wbuff, 4);

            if (sunwave_dev->finger && sunwave_dev->finger->write_then_read == 0) {
                status = sunwave_dev_sync_read(sunwave_dev, 6);

                if (status <= 0) {
                    kfree(sunwave_dev->buffer);
                    sunwave_dev->buffer = NULL;
                    return -2;
                }
            }
            else {
                status = spi_write_then_read(sunwave_dev->spi, sunwave_dev->buffer, 4, sunwave_dev->buffer + 4, 2);

                if (status <= 0) {
                    kfree(sunwave_dev->buffer);
                    sunwave_dev->buffer = NULL;
                    return -2;
                }
            }

            reg = sunwave_dev->buffer[4] ;
            sw_info("*** read sunwave_fp id 8221 is %x ***", reg);

            if (reg == 0x82) {
                kfree(sunwave_dev->buffer);
                sunwave_dev->buffer = NULL;
                return 0;
            }
        }

        err_cnt++;
    }
    while (err_cnt < 3);

    kfree(sunwave_dev->buffer);
    sunwave_dev->buffer = NULL;
    return -2;
}

static int sunwave_read_id_8202(sunwave_sensor_t* sunwave_dev)
{
    ssize_t  status = 0;
    unsigned char wbuff[5] = {0};
    int err_cnt = 0;
    int reg;
    int tryTime;
    sunwave_dev->buffer = kzalloc(7, GFP_KERNEL);
    sw_info("sunwave_read_id_8202");

    do {
        if (sunwave_dev->finger && sunwave_dev->finger->reset) {
            sunwave_dev->finger->reset(&sunwave_dev->spi);
        }

        tryTime = 3;

        while (tryTime--) {
            msleep(5);
            memset(sunwave_dev->buffer, 0, 7);
            wbuff[0] = 0x60;
            wbuff[1] = 0x9f;
            wbuff[2] = 0x28;
            wbuff[3] = 0x02;
            wbuff[4] = 0x00;
            memcpy(sunwave_dev->buffer, wbuff, 5);

            if (sunwave_dev->finger && sunwave_dev->finger->write_then_read == 0) {
                status = sunwave_dev_sync_read(sunwave_dev, 7);

                if (status <= 0) {
                    kfree(sunwave_dev->buffer);
                    sunwave_dev->buffer = NULL;
                    return -2;
                }
            }
            else {
                status = spi_write_then_read(sunwave_dev->spi, sunwave_dev->buffer, 5, sunwave_dev->buffer + 5, 2);

                if (status <= 0) {
                    kfree(sunwave_dev->buffer);
                    sunwave_dev->buffer = NULL;
                    return -2;
                }
            }

            reg = sunwave_dev->buffer[5] ;
            //sw_info("*** read sunwave_fp id 8202 is %x ***", reg);

            if (reg == 0x82) {
                if (sunwave_dev->buffer[6] == 0x02) {
                    sw_info("*** read sunwave_fp id 8202 is %x ***", reg);
                }
                else if (sunwave_dev->buffer[6] == 0x05) {
                    sw_info("*** read sunwave_fp id 8205 is %x ***", reg);
                }

                kfree(sunwave_dev->buffer);
                sunwave_dev->buffer = NULL;
                return 0;
            }
        }

        err_cnt++;
    }
    while (err_cnt < 3);

    kfree(sunwave_dev->buffer);
    sunwave_dev->buffer = NULL;
    return -2;
}

static int sunwave_read_id_rom(sunwave_sensor_t* sunwave_dev)
{
    ssize_t  status = 0;
    unsigned char wbuff[11] = {0};
    int err_cnt = 0;
    int reg ;
    int reg1;
    int reg2;
    int romId1;
    int romId2;
    sw_info("sunwave_read_id_rom");
    sunwave_dev->buffer = kzalloc(11, GFP_KERNEL);

    do {
        if (sunwave_dev->finger && sunwave_dev->finger->reset) {
            sunwave_dev->finger->reset(&sunwave_dev->spi);
        }

        msleep(5);
#if 1
        //------------------------------------------------------------------------------------------
        //add enter rom start. by lin 20161101
        wbuff[0] = 0x81;
        wbuff[1] = 0x7e;
        wbuff[2] = 0x00;
        wbuff[3] = 0x00;
        wbuff[4] = 0x84;
        memcpy(sunwave_dev->buffer, wbuff, 5);
        status = sunwave_dev_sync_write(sunwave_dev, 5);

        if (status <= 0) {
            kfree(sunwave_dev->buffer);
            sunwave_dev->buffer = NULL;
            return -2;
        }

        msleep(1);
        //add enter rom end. by lin 20161101
        //------------------------------------------------------------------------------------------
#endif
        //------------------------------------------------------------------------------------------
        //read rom id start
        memset(sunwave_dev->buffer, 0, 11);
        wbuff[0] = 0x81;
        wbuff[1] = 0x7e;
        wbuff[2] = 0x00;
        wbuff[3] = 0x00;
        wbuff[4] = 0x21;
        wbuff[5] = 0x00;
        wbuff[6] = 0x00;
        wbuff[7] = 0x03;
        memcpy(sunwave_dev->buffer, wbuff, 8);

        if (sunwave_dev->finger && sunwave_dev->finger->write_then_read == 0) {
            status = sunwave_dev_sync_read(sunwave_dev, 11);

            if (status <= 0) {
                kfree(sunwave_dev->buffer);
                sunwave_dev->buffer = NULL;
                return -2;
            }
        }
        else {
            status = spi_write_then_read(sunwave_dev->spi, sunwave_dev->buffer, 8, sunwave_dev->buffer + 8, 3);

            if (status <= 0) {
                kfree(sunwave_dev->buffer);
                sunwave_dev->buffer = NULL;
                return -2;
            }
        }

        reg = sunwave_dev->buffer[8];
        reg1 = sunwave_dev->buffer[9];
        reg2 = sunwave_dev->buffer[10];
        romId1 = (reg << 8)  + reg1;
        romId2 = (reg1 << 8) + reg2;
        sw_info("***read sunwave_fp id rom, romId1:%04x, romId2:%04x***", romId1, romId2);
        sw_info("***read sunwave_fp id rom, buf:%02x-%02x-%02x***", reg, reg1, reg2);
        err_cnt++;

        if ((romId1 == 0x8201) || (romId2 == 0x8201) || (romId1 == 0x8211) || (romId2 == 0x8211)) {
            kfree(sunwave_dev->buffer);
            sunwave_dev->buffer = NULL;

            if (sunwave_dev->finger && sunwave_dev->finger->reset) {
                sunwave_dev->finger->reset(&sunwave_dev->spi);
            }

            return 0;
        }

        //read rom id end
        //------------------------------------------------------------------------------------------
    }
    while (err_cnt < 6);

    kfree(sunwave_dev->buffer);
    sunwave_dev->buffer = NULL;
    return -2;
}

static int sunwave_read_vendor_id(sunwave_sensor_t* sunwave_dev)
{
    ssize_t  status = 0;
    unsigned char wbuff[2] = {0};
    int err_cnt = 0;
    int reg;
    int tryTime;
    sunwave_dev->buffer = kzalloc(6, GFP_KERNEL);
    sw_info("sunwave_read_vendor_id");

    do {
        if (sunwave_dev->finger && sunwave_dev->finger->reset) {
            sunwave_dev->finger->reset(&sunwave_dev->spi);
        }

        tryTime = 3;

        while (tryTime--) {
            msleep(5);
            memset(sunwave_dev->buffer, 0, 6);
            wbuff[0] = 0xa0;
            wbuff[1] = 0x5f;
            memcpy(sunwave_dev->buffer, wbuff, 2);

            if (sunwave_dev->finger && sunwave_dev->finger->write_then_read == 0) {
                status = sunwave_dev_sync_read(sunwave_dev, 6);

                if (status <= 0) {
                    kfree(sunwave_dev->buffer);
                    sunwave_dev->buffer = NULL;
                    return -2;
                }
            }
            else {
                status = spi_write_then_read(sunwave_dev->spi, sunwave_dev->buffer, 2, sunwave_dev->buffer + 2, 4);

                if (status <= 0) {
                    kfree(sunwave_dev->buffer);
                    sunwave_dev->buffer = NULL;
                    return -2;
                }
            }

            if (0x53 == sunwave_dev->buffer[2] && 0x75 == sunwave_dev->buffer[3] && 0x6e == sunwave_dev->buffer[4]
                && 0x57 == sunwave_dev->buffer[5]) {
                sw_info("*** read sunwave_read_vendor_id is success ***");
                kfree(sunwave_dev->buffer);
                sunwave_dev->buffer = NULL;
                return 0;
            }
        }

        err_cnt++;
    }
    while (err_cnt < 3);

    kfree(sunwave_dev->buffer);
    sunwave_dev->buffer = NULL;
    return -2;
}

static int sunwave_read_id(sunwave_sensor_t* sunwave_dev)
{
    sw_info("=======>read_id is ready<=======");

    if (sunwave_read_vendor_id(sunwave_dev) == 0) {
        sw_info("sunwave_read_vendor_id is success!");
        return 0;
    }
    else if (sunwave_read_id_8221(sunwave_dev) == 0) {
        sw_info("sunwave_read_id_8221 is success!");
        return 0;
    }
    else if (sunwave_read_id_8202(sunwave_dev) == 0) {
        sw_info("sunwave_read_id_8202 is success!");
        return 0;
    }
    else if (sunwave_read_id_rom(sunwave_dev) == 0) {
        sw_info("sunwave_read_id_rom is success!");
        return 0;
    }
    else if (sunwave_read_id_fw(sunwave_dev) == 0) {
        sw_info("sunwave_read_id_fw is success!");
        return 0;
    }
    else {
        sw_err("sunwave_read_id fail!");
        return -2;
    }
}

#endif
//end zsx

#if __SUNWAVE_SCREEN_LOCK_EN

#ifdef CONFIG_HAS_EARLYSUSPEND
static void sunwave_early_suspend(struct early_suspend* handler)
{
    char* screen[2] = { "SCREEN_STATUS=OFF", NULL };
    sunwave_sensor_t* sunwave = container_of(handler, sunwave_sensor_t, early_suspend);
    sw_info("%s enter.\n", __func__);
    kobject_uevent_env(&sunwave->spi->dev.kobj, KOBJ_CHANGE, screen);
    sw_info("%s leave.\n", __func__);
}
static void sunwave_late_resume(struct early_suspend* handler)
{
    char* screen[2] = { "SCREEN_STATUS=ON", NULL };
    sunwave_sensor_t* sunwave = container_of(handler, sunwave_sensor_t, early_suspend);
    sw_info("%s enter.\n", __func__);
    kobject_uevent_env(&sunwave->spi->dev.kobj, KOBJ_CHANGE, screen);
    sw_info("%s leave.\n", __func__);
}

#else //CONFIG_HAS_EARLYSUSPEND

static int sunwave_fb_notifier_callback(struct notifier_block* self,
                                        unsigned long event, void* data)
{
    static char screen_status[64] = {'\0'};
    char* screen_env[2] = { screen_status, NULL };
    sunwave_sensor_t* sunwave;
    struct fb_event* evdata = data;
    unsigned int blank;
    int retval = 0;
    sw_info("%s enter.\n", __func__);

    if (event != FB_EVENT_BLANK /* FB_EARLY_EVENT_BLANK */) {
        return 0;
    }

    sunwave = container_of(self, sunwave_sensor_t, notifier);
    blank = *(int*)evdata->data;
    sw_info("%s enter, blank=0x%x\n", __func__, blank);

    switch (blank) {
        case FB_BLANK_UNBLANK:
            sw_info("%s: lcd on notify\n", __func__);
            sprintf(screen_status, "SCREEN_STATUS=%s", "ON");
            kobject_uevent_env(&sunwave->spi->dev.kobj, KOBJ_CHANGE, screen_env);
            break;

        case FB_BLANK_POWERDOWN:
            sw_info("%s: lcd off notify\n", __func__);
            sprintf(screen_status, "SCREEN_STATUS=%s", "OFF");
            kobject_uevent_env(&sunwave->spi->dev.kobj, KOBJ_CHANGE, screen_env);
            break;

        default:
            sw_info("%s: other notifier, ignore\n", __func__);
            break;
    }

    sw_info("%s %s leave.\n", screen_status, __func__);
    return retval;
}
#endif //CONFIG_HAS_EARLYSUSPEND

#endif //__SUNWAVE_SCREEN_LOCK_EN

#if __SUNWAVE_QUIK_WK_CPU_EN
//add for open core begin
static void work_handler(struct work_struct* data)
{
    int cpu;
    sw_dbg("sunwave:Entry work_handler");

    for (cpu = 1 ; cpu < NR_CPUS; cpu++) {
        if (!cpu_online(cpu)) {
            cpu_up(cpu);
        }
    }
}

static void  finger_workerqueue_init(void)
{
    core_queue = create_singlethread_workqueue("sf_wk_main"); //cretae a signal thread worker queue

    if (!core_queue) {
        return;
    }

    INIT_WORK(&core_work, work_handler);
}
//add for open core end
#endif

static int __devinit sunwave_dev_probe(struct spi_device* spi)
{
    sunwave_sensor_t*    sunwave_dev;
    int         status = 0;
    int attr;
    //struct device* dev;
    sw_info("sunwave probe info:%s %s", __DATE__, __TIME__);
    sw_info("sunwave version:%s", SUNWAVE_VERSION);
    /* Allocate driver data */
    g_sunwave_sensor = NULL;
    sunwave_dev = kzalloc(sizeof(*sunwave_dev), GFP_KERNEL);

    if (!sunwave_dev) {
        sw_err("memory error");
        return -ENOMEM;
    }

    memset(sunwave_dev, 0, sizeof(sunwave_sensor_t));
#if __PALTFORM_SPREAD_EN

    if (spi->dev.of_node == NULL) {
        spi->dev.of_node = of_find_compatible_node(NULL, NULL, "sunwave,sunwave_fp");
    }

    spi->bits_per_word = 8;
#endif
    /* Initialize the driver data */
    sunwave_dev->spi = spi;
    sunwavecorp_register_finger(sunwave_dev);

    if (sunwave_dev->finger && sunwave_dev->finger->init) {
        status = sunwave_dev->finger->init(&sunwave_dev->spi);

        if (status < 0) {
            sw_err("init finger gpio %d", status) ;
            return status;
        }
    }

    create_input_device(sunwave_dev);
    spin_lock_init(&sunwave_dev->spi_lock);
    mutex_init(&sunwave_dev->buf_lock);
    INIT_LIST_HEAD(&sunwave_dev->device_entry);
    /* If we can allocate a minor number, hook up this device.
     * Reusing minors is fine so long as udev or mdev is working.
     */
    spi_set_drvdata(spi, sunwave_dev);
    //add by Sea 20160505
#if __SUNWAVE_DETECT_ID_EN
    status = sunwave_read_id(sunwave_dev);

    if (status != 0) {
        sw_err("read id error....remove register");
        list_del(&sunwave_dev->device_entry);
        release_input_device(sunwave_dev);
        spi_set_drvdata(spi, NULL);
        g_sunwave_sensor = NULL;
        kfree(sunwave_dev);
        sunwave_dev = NULL;
        return -ENODEV;
    }

#endif
#if __SUNWAVE_SCREEN_LOCK_EN
#ifdef CONFIG_HAS_EARLYSUSPEND
    sw_info("%s: register_early_suspend\n", __func__);
    sunwave_dev->early_suspend.level = (EARLY_SUSPEND_LEVEL_DISABLE_FB - 1);
    sunwave_dev->early_suspend.suspend = sunwave_early_suspend;
    sunwave_dev->early_suspend.resume = sunwave_late_resume;
    register_early_suspend(&sunwave_dev->early_suspend);
#else
    sw_info("%s: fb_register_client\n", __func__);
    sunwave_dev->notifier.notifier_call = sunwave_fb_notifier_callback;
    fb_register_client(&sunwave_dev->notifier);
#endif
#endif //__SUNWAVE_SCREEN_LOCK_EN
    //add by Sea
    device_init_wakeup(&sunwave_dev->spi->dev, 1);
    device_may_wakeup(&sunwave_dev->spi->dev);
    wake_lock_init(&sunwave_dev->wakelock, WAKE_LOCK_SUSPEND, dev_name(&sunwave_dev->spi->dev));
    status = misc_register(&sunwave_misc_dev);

    if (status < 0) {
        sw_err("misc_register error!");
        return status;
    }

    attr = sysfs_create_group(&sunwave_misc_dev.this_device->kobj, &sunwave_attr_group);

    if (attr) {
        printk("kls fail to creat file!");
        kobject_put(&sunwave_misc_dev.this_device->kobj);
        return attr;
    }

    mutex_lock(&sunwave_device_list_lock);
    sunwave_dev->devt = MKDEV(MISC_MAJOR, sunwave_misc_dev.minor);
    list_add(&sunwave_dev->device_entry, &sunwave_device_list);
    mutex_unlock(&sunwave_device_list_lock);
#if __SUNWAVE_HW_INFO_EN
    sprintf(current_fingerprint_info.chip, SUNWAVE_CHIP_DEF);
    sprintf(current_fingerprint_info.id, "0x%x", read_id);
    strcpy(current_fingerprint_info.vendor, SUNWAVE_VENDOR_DEF);
    strcpy(current_fingerprint_info.more, "fingerprint");
#endif
#if __SUNWAVE_QUIK_WK_CPU_EN
    //add for open core begin
    finger_workerqueue_init();
    //add for open core end
#endif
    status = sunwave_irq_request(sunwave_dev);

    if (status < 0) {
        sw_err("request irq error!");
        return status;
    }

    sunwave_driver_status = 1;
    return status;
}

static int __devexit sunwave_dev_remove(struct spi_device* spi)
{
    sunwave_sensor_t*    sunwave_dev = spi_get_drvdata(spi);
#if __SUNWAVE_QUIK_WK_CPU_EN
    //exit work queue begin
    destroy_workqueue(core_queue);
    //exit work queue end
#endif

    if (sunwave_dev->finger && sunwave_dev->finger->exit) {
        sunwave_dev->finger->exit(&sunwave_dev->spi);
    }

#if __PALTFORM_SPREAD_EN
    sci_glb_clr(REG_AP_APB_APB_EB, BIT_SPI0_EB);
#endif
#if __SUNWAVE_SCREEN_LOCK_EN
#ifdef CONFIG_HAS_EARLYSUSPEND

    if (sunwave_dev->early_suspend.suspend) {
        unregister_early_suspend(&sunwave_dev->early_suspend);
    }

#else
    fb_unregister_client(&sunwave_dev->notifier);
#endif
#endif //__SUNWAVE_SCREEN_LOCK_EN
    sunwave_irq_free(sunwave_dev);
    release_input_device(sunwave_dev);
    g_sunwave_sensor = NULL;
    /* make sure ops on existing fds can abort cleanly */
    spin_lock_irq(&sunwave_dev->spi_lock);
    sunwave_dev->spi = NULL;
    spi_set_drvdata(spi, NULL);
    spin_unlock_irq(&sunwave_dev->spi_lock);
    //add by sea
    wake_lock_destroy(&sunwave_dev->wakelock);
    /* prevent new opens */
    mutex_lock(&sunwave_device_list_lock);
    list_del(&sunwave_dev->device_entry);
    //device_destroy(sunwave_dev_class, sunwave_dev->devt);

    if (sunwave_dev->users == 0) {
        kfree(sunwave_dev);
    }

    mutex_unlock(&sunwave_device_list_lock);
    misc_deregister(&sunwave_misc_dev);
    return 0;
}

struct spi_device_id sunwave_id_table = {"sunwave_fp", 0};

#ifdef CONFIG_OF
#if __PALTFORM_SPREAD_EN
static struct of_device_id sunwave_of_match[] = {
    { .compatible = "mediatek,sunwave-finger", },
    {}
};
#else
/*the .compatible shoulde be same with dts compatible*/
static struct of_device_id sunwave_of_match[] = {
    { .compatible = "sunwave,sunwave_fp", },
    {}
};
#endif
MODULE_DEVICE_TABLE(of, sunwave_of_match);

#endif

static struct spi_driver sunwave_dev_spi_driver = {
    .driver = {
        .name =     "sunwave_fp",
        .owner =    THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = sunwave_of_match,
#endif
    },
    .probe =    sunwave_dev_probe,
    .remove =   __devexit_p(sunwave_dev_remove),
    .suspend = sunwave_suspend,
    .resume = sunwave_resume,
    .id_table = &sunwave_id_table,

    /* NOTE:  suspend/resume methods are not necessary here.
     * We don't do anything except pass the requests to/from
     * the underlying controller.  The refrigerator handles
     * most issues; the controller driver handles the rest.
     */
};


/*-------------------------------------------------------------------------*/

static int __init sunwave_dev_init(void)
{
    int status;
    /* Claim our 256 reserved device numbers.  Then register a class
     * that will key udev/mdev to add/remove /dev nodes.  Last, register
     * the driver which manages those device numbers.
     */
    //dev_err(NULL,     "master %s: is invalid. \n",    dev_name ( &pdev->dev ) );
    /**     for add platform resources  if you need
     *      by Jone.Chen
     *   2015.15.25
     */
    sunwave_driver_status = 0;
    sunwavecorp_register_platform();
    status = spi_register_driver(&sunwave_dev_spi_driver);
    sw_info("spi_register_driver %d", status);
    return status;
}
module_init(sunwave_dev_init);

void  sunwave_driver_unregister(void)
{
    spi_unregister_driver(&sunwave_dev_spi_driver);
}
EXPORT_SYMBOL_GPL(sunwave_driver_unregister);

int  sunwave_devices_status(void)
{
    return sunwave_driver_status;
}
EXPORT_SYMBOL_GPL(sunwave_devices_status);

static void __exit sunwave_dev_exit(void)
{
    sunwave_driver_unregister();
}
module_exit(sunwave_dev_exit);

MODULE_AUTHOR("Jone.Chen, <yuhua8688@tom.com>");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:sunwave_fp");

