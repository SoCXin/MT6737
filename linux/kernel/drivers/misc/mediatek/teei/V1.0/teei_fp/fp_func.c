#include<linux/kernel.h>
#include <linux/platform_device.h>
#include<linux/module.h>
#include<linux/types.h>
#include<linux/fs.h>
#include<linux/errno.h>
#include<linux/mm.h>
#include<linux/sched.h>
#include<linux/init.h>
#include<linux/cdev.h>
#include<asm/io.h>
#include<asm/uaccess.h>
#include <asm/cacheflush.h>
#include<linux/semaphore.h>
#include<linux/slab.h>
#include "../tz_driver/teei_id.h"
#include "../tz_driver/tz_service.h"
#include "../tz_driver/nt_smc_call.h"

#include "teei_fp.h"

#define FP_SIZE	0x80000
#define CMD_MEM_CLEAR	_IO(0x775A777E, 0x1)
#define CMD_FP_CMD      _IO(0x775A777E, 0x2)
#define FP_MAJOR	254
#define SHMEM_ENABLE    0
#define SHMEM_DISABLE   1
#define DEV_NAME "teei_fp"
#define FP_DRIVER_ID 100
static int fp_major = FP_MAJOR;
static struct class *driver_class;
static dev_t devno;
struct semaphore fp_api_lock;
struct fp_dev {
	struct cdev cdev;
	unsigned char mem[FP_SIZE];
	struct semaphore sem;
};

struct semaphore daulOS_rd_sem;
struct semaphore daulOS_wr_sem;
EXPORT_SYMBOL_GPL(daulOS_rd_sem);
EXPORT_SYMBOL_GPL(daulOS_wr_sem);
/*#define FP_DEBUG*/
extern char *fp_buff_addr;
/*extern unsigned int daulOS_shmem_flags;*/

struct fp_dev *fp_devp;

int fp_open(struct inode *inode, struct file *filp)
{
#ifdef FP_DEBUG
	printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!say hello  from fp!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif
	filp->private_data = fp_devp;

	return 0;
}

int fp_release(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static int fp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	down(&fp_api_lock);
	unsigned int args_len = 0;
	unsigned int fp_cid = 0xFF;
	unsigned int fp_fid = 0xFF;
#ifdef FP_DEBUG
	printk("##################################\n");
	printk("fp ioctl received received cmd is: %x arg is %x\n", cmd, (unsigned int)arg);
	printk("CMD_MEM_CLEAR is: %x CMD_FP_CMD is %x \n", CMD_MEM_CLEAR, CMD_FP_CMD);
#endif
	switch (cmd) {
	case CMD_MEM_CLEAR:
		printk(KERN_INFO "CMD MEM CLEAR. \n");
		break;
	case CMD_FP_CMD:
		/*TODO compute args length*/
		/*[11-15] is the length of data*/
		args_len = *((unsigned int *)(arg + 12));
		/*[0-3] is cmd id*/
		fp_cid = *((unsigned int *)(arg));
		/*[4-7] is fuction id*/
		fp_fid = *((unsigned int *)(arg + 4));
#ifdef FP_DEBUG
		printk("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		printk("invoke fp cmd CMD_FP_CMD: arg's address is %x, args's length %d\n", (unsigned int)arg, args_len);
		printk("invoke fp cmd fp_cid is %d fp_fid is %d \n", fp_cid, fp_fid);
#endif
		if (!fp_buff_addr) {
			printk("fp_buiff_addr is invalid!. \n");
			up(&fp_api_lock);
			return -EFAULT;
		}
		memset((void *)fp_buff_addr, 0, args_len + 16);
		if (copy_from_user((void *)fp_buff_addr, (void *)arg,
				args_len + 16)) {
			printk(KERN_INFO "copy from user failed. \n");
			up(&fp_api_lock);
			return -EFAULT;
		}

		Flush_Dcache_By_Area((unsigned long)fp_buff_addr,
				fp_buff_addr + FP_SIZE);
		/*send command data to TEEI*/
		send_fp_command(FP_DRIVER_ID);
#ifdef FP_DEBUG
		printk("back from TEEI try copy share mem to user \n");
		printk("result in share memory %d  \n", *((unsigned int *)fp_buff_addr));
		printk("[%s][%d] fp_buff_addr 88 - 91 = %d\n", __func__, args_len, *((unsigned int *)(fp_buff_addr + 88)));
#endif

		if (copy_to_user((void *)arg, (unsigned long)fp_buff_addr,
				args_len + 16)) {
			printk("copy from user failed. \n");
			up(&fp_api_lock);
			return -EFAULT;
		}
#ifdef FP_DEBUG
		printk("result after copy %d  \n", *((unsigned int *)arg));
		printk("invoke fp cmd end. \n");
#endif
		break;
	default:
		up(&fp_api_lock);
		return -EINVAL;
	}
	up(&fp_api_lock);
	return 0;
}

static ssize_t fp_read(struct file *filp, char __user *buf,
		size_t size, loff_t *ppos)
{
	int ret = 0;
	return ret;
}

static ssize_t fp_write(struct file *filp, const char __user *buf,
		size_t size, loff_t *ppos)
{
	return 0;
}

static loff_t fp_llseek(struct file *filp, loff_t offset, int orig)
{
	return 0;
}
static const struct file_operations fp_fops = {
	.owner = THIS_MODULE,
	.llseek = fp_llseek,
	.read = fp_read,
	.write = fp_write,
	.unlocked_ioctl = fp_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = fp_ioctl,
#endif
	.open = fp_open,
	.release = fp_release
};

static void fp_setup_cdev(struct fp_dev *dev, int index)
{
	int err = 0;
	int devno = MKDEV(fp_major, index);

	cdev_init(&dev->cdev, &fp_fops);
	dev->cdev.owner = fp_fops.owner;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
	printk(KERN_NOTICE "Error %d adding fp %d.\n", err, index);
}
}

int fp_init(void)
{
int result = 0;
struct device *class_dev = NULL;
devno = MKDEV(fp_major, 0);

result = alloc_chrdev_region(&devno, 0, 1, DEV_NAME);
fp_major = MAJOR(devno);
sema_init(&(fp_api_lock), 1);
if (result < 0) {
	return result;
}
driver_class = NULL;
driver_class = class_create(THIS_MODULE, DEV_NAME);
if (IS_ERR(driver_class)) {
	result = -ENOMEM;
	printk("class_create failed %d.\n", result);
	goto unregister_chrdev_region;
}

class_dev = device_create(driver_class, NULL, devno, NULL, DEV_NAME);
if (!class_dev) {
	result = -ENOMEM;
	printk("class_device_create failed %d.\n", result);
	goto class_destroy;
}
fp_devp = NULL;
fp_devp = kmalloc(sizeof(struct fp_dev), GFP_KERNEL);
if (fp_devp == NULL) {
	result = -ENOMEM;
	goto class_device_destroy;
}
memset(fp_devp, 0, sizeof(struct fp_dev));
fp_setup_cdev(fp_devp, 0);
sema_init(&fp_devp->sem, 1);
sema_init(&daulOS_rd_sem, 0);
sema_init(&daulOS_wr_sem, 0);

printk("[%s][%d]create the teei_fp device node successfully!\n", __func__,
		__LINE__);
goto return_fn;

class_device_destroy: device_destroy(driver_class, devno);
class_destroy: class_destroy(driver_class);
unregister_chrdev_region: unregister_chrdev_region(devno, 1);
return_fn: return result;
}

void fp_exit(void)
{
device_destroy(driver_class, devno);
class_destroy(driver_class);
cdev_del(&fp_devp->cdev);
kfree(fp_devp);
unregister_chrdev_region(MKDEV(fp_major, 0), 1);
}

MODULE_AUTHOR("Microtrust");
MODULE_LICENSE("Dual BSD/GPL");

module_param(fp_major, int, S_IRUGO);

module_init(fp_init);
module_exit(fp_exit);
