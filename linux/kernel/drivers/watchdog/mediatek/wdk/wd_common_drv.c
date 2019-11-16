#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/rtc.h>
#include <linux/cpu.h>
#include <mt-plat/aee.h>
#include <ext_wd_drv.h>

#include <mach/wd_api.h>
#include <linux/seq_file.h>

/*#include <mach/mtk_rtc.h>*/

/*************************************************************************
 * Feature configure region
 *************************************************************************/
#define __ENABLE_WDT_SYSFS__
#define __ENABLE_WDT_AT_INIT__

/* ------------------------------------------------------------------------ */
#define PFX "wdk: "
#define DEBUG_WDK 0
#if DEBUG_WDK
#define dbgmsg(msg...) pr_debug(PFX msg)
#else
#define dbgmsg(...)
#endif
#define msg(msg...) pr_info(PFX msg)
#define warnmsg(msg...) pr_warn(PFX msg)
#define errmsg(msg...) pr_err(PFX msg)

#define MIN_KICK_INTERVAL	 1
#define MAX_KICK_INTERVAL	30
#define	MRDUMP_SYSRESETB	0
#define	MRDUMP_EINTRST		1
#define PROC_WK "wdk"
#define	PROC_MRDUMP_RST	"mrdump_rst"

static int kwdt_thread(void *arg);
static int start_kicker(void);

static int g_kicker_init;
static int debug_sleep;

static DEFINE_SPINLOCK(lock);

#define CPU_NR (nr_cpu_ids)
struct task_struct *wk_tsk[16] = { 0 };	/* max cpu 16 */

static unsigned long kick_bit;


enum ext_wdt_mode g_wk_wdt_mode = WDT_DUAL_MODE;
static struct wd_api *g_wd_api;
static int g_kinterval = -1;
static int g_timeout = -1;
static int g_need_config;
static int wdt_start;
static int g_enable = 1;

static unsigned int lasthpg_act;
static unsigned int lasthpg_cpu;
static unsigned long long lasthpg_t;


static char cmd_buf[256];


static int wk_proc_cmd_read(struct seq_file *s, void *v)
{
	seq_printf(s, "mode interval timeout enable\n%-4d %-9d %-8d %-7d\n", g_wk_wdt_mode,
		   g_kinterval, g_timeout, g_enable);
	return 0;
}

static int wk_proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, wk_proc_cmd_read, NULL);
}

static ssize_t wk_proc_cmd_write(struct file *file, const char *buf, size_t count, loff_t *data)
{
	int ret;
	int timeout;
	int mode;
	int kinterval;
	int en;			/* enable or disable ext wdt 1<-->enable 0<-->disable */
	struct wd_api *my_wd_api = NULL;

	ret = get_wd_api(&my_wd_api);
	if (ret)
		pr_debug("get public api error in wd common driver %d", ret);

	if (count == 0)
		return -1;

	if (count > 255)
		count = 255;

	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)
		return -1;

	cmd_buf[count] = '\0';

	pr_debug("Write %s\n", cmd_buf);

	ret = sscanf(cmd_buf, "%d %d %d %d %d", &mode, &kinterval, &timeout, &debug_sleep, &en);

	pr_debug("[WDK] mode=%d interval=%d timeout=%d enable =%d\n", mode, kinterval, timeout, en);

	if (timeout < kinterval) {
		pr_err("The interval(%d) value should be smaller than timeout value(%d)\n",
		       kinterval, timeout);
		return -1;
	}

	if ((timeout < MIN_KICK_INTERVAL) || (timeout > MAX_KICK_INTERVAL)) {
		pr_err("The timeout(%d) is invalid (%d - %d)\n", kinterval, MIN_KICK_INTERVAL,
		       MAX_KICK_INTERVAL);
		return -1;
	}

	if ((kinterval < MIN_KICK_INTERVAL) || (kinterval > MAX_KICK_INTERVAL)) {
		pr_err("The interval(%d) is invalid (%d - %d)\n", kinterval, MIN_KICK_INTERVAL,
		       MAX_KICK_INTERVAL);
		return -1;
	}

	if (!((mode == WDT_IRQ_ONLY_MODE) ||
	      (mode == WDT_HW_REBOOT_ONLY_MODE) || (mode == WDT_DUAL_MODE))) {
		pr_err("Tha watchdog kicker wdt mode is not correct %d\n", mode);
		return -1;
	}

	if (1 == en) {
		mtk_wdt_enable(WK_WDT_EN);
#ifdef CONFIG_LOCAL_WDT
		local_wdt_enable(WK_WDT_EN);
		pr_debug("[WDK] enable local wdt\n");
#endif
		pr_debug("[WDK] enable wdt\n");
	}
	if (0 == en) {
		mtk_wdt_enable(WK_WDT_DIS);
#ifdef CONFIG_LOCAL_WDT
		local_wdt_enable(WK_WDT_DIS);
		pr_debug("[WDK] disable local wdt\n");
#endif
		pr_debug("[WDK] disable wdt\n");
	}

	spin_lock(&lock);

	g_enable = en;
	g_kinterval = kinterval;

	g_wk_wdt_mode = mode;
	if (1 == mode) {
		/* irq mode only useful to 75 */
		mtk_wdt_swsysret_config(0x20000000, 1);
		pr_debug("[WDK] use irq mod\n");
	} else if (0 == mode) {
		/* reboot mode only useful to 75 */
		mtk_wdt_swsysret_config(0x20000000, 0);
		pr_debug("[WDK] use reboot mod\n");
	} else if (2 == mode)
		my_wd_api->wd_set_mode(WDT_IRQ_ONLY_MODE);
	else
		pr_debug("[WDK] mode err\n");

	g_timeout = timeout;
	if (mode != 2)
		g_need_config = 1;

	spin_unlock(&lock);

	return count;
}

static int mrdump_proc_cmd_read(struct seq_file *s, void *v)
{
	return 0;
}

static int mrdump_proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, mrdump_proc_cmd_read, NULL);
}

static ssize_t mrdump_proc_cmd_write(struct file *file, const char *buf, size_t count,
				     loff_t *data)
{
	int ret = 0;
	int mrdump_rst_source;
	int en, mode;		/* enable or disable ext wdt 1<-->enable 0<-->disable */
	char mrdump_cmd_buf[256];
	struct wd_api *my_wd_api = NULL;

	ret = get_wd_api(&my_wd_api);
	if (ret)
		pr_debug("get public api error in wd common driver %d", ret);

	if (count == 0)
		return -1;

	if (count > 255)
		count = 255;

	ret = copy_from_user(mrdump_cmd_buf, buf, count);
	if (ret < 0)
		return -1;

	mrdump_cmd_buf[count] = '\0';

	dbgmsg("Write %s\n", mrdump_cmd_buf);

	ret = sscanf(mrdump_cmd_buf, "%d %d %d", &mrdump_rst_source, &mode, &en);
	if (ret != 3)
		pr_debug("%s: expect 3 numbers\n", __func__);

	pr_debug("[MRDUMP] rst_source=%d mode=%d enable=%d\n", mrdump_rst_source, mode, en);

	if (1 < mrdump_rst_source) {
		errmsg("The mrdump_rst_source(%d) value should be smaller than 2\n",
		       mrdump_rst_source);
		return -1;
	}

	if (1 < mode) {
		errmsg("The mrdump_rst_mode(%d) value should be smaller than 2\n", mode);
		return -1;
	}

	spin_lock(&lock);
	if (mrdump_rst_source == MRDUMP_SYSRESETB) {
		ret = my_wd_api->wd_debug_key_sysrst_config(en, mode);
	} else if (mrdump_rst_source == MRDUMP_EINTRST) {
		ret = my_wd_api->wd_debug_key_eint_config(en, mode);
	} else {
		pr_debug("[MRDUMP] invalid mrdump_rst_source\n");
		ret = -1;
	}
	spin_unlock(&lock);

	if (ret == 0)
		pr_debug("[MRDUMP] MRDUMP External success\n");
	else
		pr_debug("[MRDUMP] MRDUMP External key not support!\n");

	return count;
}

static int start_kicker_thread_with_default_setting(void)
{
	int ret = 0;

	spin_lock(&lock);

	g_kinterval = 20;	/* default interval: 20s */

	g_need_config = 0;	/* Note, we DO NOT want to call configure function */

	wdt_start = 1;		/* Start once only */
	spin_unlock(&lock);
	start_kicker();

	pr_debug("[WDK] fwq start_kicker_thread_with_default_setting done\n");
	return ret;
}

static unsigned int cpus_kick_bit;
void wk_start_kick_cpu(int cpu)
{
	if (IS_ERR(wk_tsk[cpu])) {
		pr_debug("[wdk]wk_task[%d] is NULL\n", cpu);
	} else {
		kthread_bind(wk_tsk[cpu], cpu);
//		pr_debug("[wdk]bind thread[%d] to cpu[%d]\n", wk_tsk[cpu]->pid, cpu);
		wake_up_process(wk_tsk[cpu]);
	}
}

void kicker_cpu_bind(int cpu)
{
	if (IS_ERR(wk_tsk[cpu]))
		pr_debug("[wdk]wk_task[%d] is NULL\n", cpu);
	else {
		/* kthread_bind(wk_tsk[cpu], cpu); */
		WARN_ON_ONCE(set_cpus_allowed_ptr(wk_tsk[cpu], cpumask_of(cpu)) < 0);

		wake_up_process(wk_tsk[cpu]);
//		pr_debug("[wdk]bind kicker thread[%d] to cpu[%d]\n", wk_tsk[cpu]->pid, cpu);
	}
}

void wk_cpu_update_bit_flag(int cpu, int plug_status)
{
	if (1 == plug_status) {	/* plug on */
		spin_lock(&lock);
		cpus_kick_bit |= (1 << cpu);
		kick_bit = 0;
		lasthpg_cpu = cpu;
		lasthpg_act = plug_status;
		lasthpg_t = sched_clock();
		spin_unlock(&lock);
	}
	if (0 == plug_status) {	/* plug off */
		spin_lock(&lock);
		cpus_kick_bit &= (~(1 << cpu));
		kick_bit = 0;
		lasthpg_cpu = cpu;
		lasthpg_act = plug_status;
		lasthpg_t = sched_clock();
		spin_unlock(&lock);
	}
}

unsigned int wk_check_kick_bit(void)
{
	return cpus_kick_bit;
}

static const struct file_operations wk_proc_cmd_fops = {
	.owner = THIS_MODULE,
	.open = wk_proc_cmd_open,
	.read = seq_read,
	.write = wk_proc_cmd_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations mrdump_rst_proc_cmd_fops = {
	.owner = THIS_MODULE,
	.open = mrdump_proc_cmd_open,
	.read = seq_read,
	.write = mrdump_proc_cmd_write,
	.llseek = seq_lseek,
	.release = single_release,
};

int wk_proc_init(void)
{

	struct proc_dir_entry *de = proc_create(PROC_WK, 0660, NULL, &wk_proc_cmd_fops);

	if (!de)
		pr_debug("[wk_proc_init]: create /proc/wdk failed\n");

	de = proc_create(PROC_MRDUMP_RST, 0660, NULL, &mrdump_rst_proc_cmd_fops);
	if (!de)
		pr_debug("[wk_proc_init]: create /proc/mrdump_rst failed\n");

	pr_debug("[WDK] Initialize proc\n");

/* de->read_proc = wk_proc_cmd_read; */
/* de->write_proc = wk_proc_cmd_write; */

	return 0;
}


void wk_proc_exit(void)
{

	remove_proc_entry(PROC_WK, NULL);

}

static int kwdt_thread(void *arg)
{

	struct sched_param param = {.sched_priority = 99 };
	struct rtc_time tm;
	struct timeval tv = { 0 };
	/* android time */
	struct rtc_time tm_android;
	struct timeval tv_android = { 0 };
	int cpu = 0;
	int local_bit = 0, loc_need_config = 0, loc_timeout = 0;
	struct wd_api *loc_wk_wdt = NULL;

	sched_setscheduler(current, SCHED_FIFO, &param);
	set_current_state(TASK_INTERRUPTIBLE);

	for (;;) {

		if (kthread_should_stop()) {
			pr_err("[WDK] kthread_should_stop do !!\n");
			break;
		}
		spin_lock(&lock);
		cpu = smp_processor_id();
		loc_wk_wdt = g_wd_api;
		loc_need_config = g_need_config;
		loc_timeout = g_timeout;
		spin_unlock(&lock);
		/* printk("fwq loc_wk_wdt(%x),loc_wk_wdt->ready(%d)\n",loc_wk_wdt ,loc_wk_wdt->ready); */
		if (loc_wk_wdt && loc_wk_wdt->ready && g_enable) {
			if (loc_need_config) {
				/* daul  mode */
				loc_wk_wdt->wd_config(WDT_DUAL_MODE, loc_timeout);
				spin_lock(&lock);
				g_need_config = 0;
				spin_unlock(&lock);
			}
			/* pr_debug("[WDK]  cpu-task=%d, current_pid=%d\n",  wk_tsk[cpu]->pid,  current->pid); */
			/*to avoid wk_tsk[cpu] had not created out */
			if (wk_tsk[cpu] != 0) {
				if (wk_tsk[cpu]->pid == current->pid) {
					/* only process WDT info if thread-x is on cpu-x */
					spin_lock(&lock);
					local_bit = kick_bit;
				/*	printk_deferred("[WDK], local_bit:0x%x, cpu:%d,RT[%lld]\n",
							local_bit, cpu, sched_clock()); */
					if ((local_bit & (1 << cpu)) == 0) {
						/* printk("[WDK]: set  WDT kick_bit\n"); */
						local_bit |= (1 << cpu);
						/* aee_rr_rec_wdk_kick_jiffies(jiffies); */
					}  /*
					printk_deferred
					    ("[WDK], local_bit:0x%x, cpu:%d, check bit0x:%x, lasthpg_cpu:%d, lasthpg_act:%d, lasthpg_t:%lld, RT[%lld]\n",
					     local_bit, cpu, wk_check_kick_bit(), lasthpg_cpu, lasthpg_act, lasthpg_t, sched_clock());  */
					if (local_bit == wk_check_kick_bit()) {
/*						printk_deferred("[WDK]: kick Ex WDT,RT[%lld]\n",
								sched_clock());
								*/
						mtk_wdt_restart(WD_TYPE_NORMAL);	/* for KICK external wdt */
						local_bit = 0;
					}
					kick_bit = local_bit;
					spin_unlock(&lock);

#ifdef CONFIG_LOCAL_WDT
					printk_deferred("[WDK]: cpu:%d, kick local wdt,RT[%lld]\n",
							cpu, sched_clock());
					/* kick local wdt */
					mpcore_wdt_restart(WD_TYPE_NORMAL);
#endif
				}
			}
		} else if (0 == g_enable) {
			pr_debug("WDK stop to kick\n");
		} else {
			pr_err("No watch dog driver is hooked\n");
			BUG();
		}

		/*to avoid wk_tsk[cpu] had not created out */
		if (wk_tsk[cpu] != 0) {
			if (wk_tsk[cpu]->pid == current->pid) {
#if (DEBUG_WDK == 1)
				msleep_interruptible(debug_sleep * 1000);
				pr_debug("WD kicker woke up %d\n", debug_sleep);
#endif
				do_gettimeofday(&tv);
				tv_android = tv;
				rtc_time_to_tm(tv.tv_sec, &tm);
				tv_android.tv_sec -= sys_tz.tz_minuteswest * 60;
				rtc_time_to_tm(tv_android.tv_sec, &tm_android);
			/*	printk_deferred
				    ("[thread:%d][RT:%lld] %d-%02d-%02d %02d:%02d:%02d.%u UTC;"
				     "android time %d-%02d-%02d %02d:%02d:%02d.%03d\n",
				     current->pid, sched_clock(), tm.tm_year + 1900, tm.tm_mon + 1,
				     tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
				     (unsigned int)tv.tv_usec, tm_android.tm_year + 1900,
				     tm_android.tm_mon + 1, tm_android.tm_mday, tm_android.tm_hour,
				     tm_android.tm_min, tm_android.tm_sec,
				     (unsigned int)tv_android.tv_usec);  */
			}
		}

		msleep_interruptible((g_kinterval) * 1000);

#ifdef CONFIG_MTK_AEE_POWERKEY_HANG_DETECT
		if ((cpu == 0) && (wk_tsk[cpu]->pid == current->pid)) {	/* only effect at cpu0 */
			if (aee_kernel_wdt_kick_api(g_kinterval) == WDT_PWK_HANG_FORCE_HWT) {
				printk_deferred("power key trigger HWT\n");
				cpus_kick_bit = 0xFFFF;	/* Try to force to HWT */
			}
		}
#endif
	}
	pr_debug("[WDK] WDT kicker thread stop, cpu:%d, pid:%d\n", cpu, current->pid);
	return 0;
}

static int start_kicker(void)
{

	int i;

	wk_cpu_update_bit_flag(0, 1);
	for (i = 0; i < CPU_NR; i++) {
		wk_tsk[i] = kthread_create(kwdt_thread, (void *)(unsigned long)i, "wdtk-%d", i);
		if (IS_ERR(wk_tsk[i])) {
			int ret = PTR_ERR(wk_tsk[i]);

			wk_tsk[i] = NULL;
			pr_alert("[WDK]kthread_create failed, wdtk-%d\n", i);
			return ret;
		}
		/* wk_cpu_update_bit_flag(i,1); */
		wk_start_kick_cpu(i);
	}
	g_kicker_init = 1;
	pr_alert("[WDK] WDT start kicker done CPU_NR=%d\n", CPU_NR);
	return 0;
}

unsigned int get_check_bit(void)
{
	return wk_check_kick_bit();
}

unsigned int get_kick_bit(void)
{
	return kick_bit;
}


/******************************************************************************
 * SYSFS support
******************************************************************************/
#ifdef __ENABLE_WDT_SYSFS__
/*---------------------------------------------------------------------------*/
/*define sysfs entry for configuring debug level and sysrq*/
const struct sysfs_ops mtk_rgu_sysfs_ops = {
	.show = mtk_rgu_attr_show,
	.store = mtk_rgu_attr_store,
};

/*---------------------------------------------------------------------------*/
struct mtk_rgu_sys_entry {
	struct attribute attr;
	 ssize_t (*show)(struct kobject *kobj, char *page);
	 ssize_t (*store)(struct kobject *kobj, const char *page, size_t size);
};
/*---------------------------------------------------------------------------*/
static struct mtk_rgu_sys_entry pause_wdt_entry = {
	{.name = "pause", .mode = S_IRUGO | S_IWUSR},
	mtk_rgu_pause_wdt_show,
	mtk_rgu_pause_wdt_store,
};

/*---------------------------------------------------------------------------*/
struct attribute *mtk_rgu_attributes[] = {
	&pause_wdt_entry.attr,
	NULL,
};

/*---------------------------------------------------------------------------*/
struct kobj_type mtk_rgu_ktype = {
	.sysfs_ops = &mtk_rgu_sysfs_ops,
	.default_attrs = mtk_rgu_attributes,
};

/*---------------------------------------------------------------------------*/
static struct mtk_rgu_sysobj {
	struct kobject kobj;
} rgu_sysobj;
/*---------------------------------------------------------------------------*/
int mtk_rgu_sysfs(void)
{
	struct mtk_rgu_sysobj *obj = &rgu_sysobj;

	memset(&obj->kobj, 0x00, sizeof(obj->kobj));

	obj->kobj.parent = kernel_kobj;
	if (kobject_init_and_add(&obj->kobj, &mtk_rgu_ktype, NULL, "mtk_rgu")) {
		kobject_put(&obj->kobj);
		return -ENOMEM;
	}
	kobject_uevent(&obj->kobj, KOBJ_ADD);

	return 0;
}

/*---------------------------------------------------------------------------*/
ssize_t mtk_rgu_attr_show(struct kobject *kobj, struct attribute *attr, char *buffer)
{
	struct mtk_rgu_sys_entry *entry = container_of(attr, struct mtk_rgu_sys_entry, attr);

	return entry->show(kobj, buffer);
}

/*---------------------------------------------------------------------------*/
ssize_t mtk_rgu_attr_store(struct kobject *kobj, struct attribute *attr, const char *buffer,
			   size_t size)
{
	struct mtk_rgu_sys_entry *entry = container_of(attr, struct mtk_rgu_sys_entry, attr);

	return entry->store(kobj, buffer, size);
}

/*---------------------------------------------------------------------------*/
ssize_t mtk_rgu_pause_wdt_show(struct kobject *kobj, char *buffer)
{
	int remain = PAGE_SIZE;
	int len = 0;
	char *ptr = buffer;

	ptr += len;
	remain -= len;

	return (PAGE_SIZE - remain);
}

/*---------------------------------------------------------------------------*/
ssize_t mtk_rgu_pause_wdt_store(struct kobject *kobj, const char *buffer, size_t size)
{
	char pause_wdt;
	int pause_wdt_b;
	int res = sscanf(buffer, "%s", &pause_wdt);

	pause_wdt_b = pause_wdt;

	if (res != 1) {
		pr_err("%s: expect 1 numbers\n", __func__);
	} else {
		/* For real case, pause wdt if get value is not zero. Suspend and resume may enable wdt again */
		if (pause_wdt_b)
			mtk_wdt_enable(WK_WDT_DIS);
	}
	return size;
}

/*---------------------------------------------------------------------------*/
#endif /*__ENABLE_WDT_SYSFS__*/
/*---------------------------------------------------------------------------*/

static int __cpuinit wk_cpu_callback(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	int hotcpu = (unsigned long)hcpu;

	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
/* watchdog_prepare_cpu(hotcpu); */
		wk_cpu_update_bit_flag(hotcpu, 1);
		break;
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:

		/* wk_cpu_update_bit_flag(hotcpu, 1); */
		if (1 == g_kicker_init)
			kicker_cpu_bind(hotcpu);

		mtk_wdt_restart(WD_TYPE_NORMAL);	/* for KICK external wdt */
#ifdef CONFIG_LOCAL_WDT
//		pr_debug("[WDK]cpu %d plug on kick local wdt\n", hotcpu);
		/* kick local wdt */
		mpcore_wdt_restart(WD_TYPE_NORMAL);
#endif

		/* pr_alert("[WDK]cpu %d plug on kick wdt\n", hotcpu); */
		break;
#ifdef CONFIG_HOTPLUG_CPU
#ifdef CONFIG_LOCAL_WDT
		/* must kick local wdt in per cpu */
	case CPU_DYING:
		/* fall-through */
#endif
	case CPU_UP_CANCELED:
		/* fall-through */
	case CPU_UP_CANCELED_FROZEN:
		/* fall-through */
	case CPU_DEAD:
		/* fall-through */
	case CPU_DEAD_FROZEN:
		mtk_wdt_restart(WD_TYPE_NORMAL);	/* for KICK external wdt */
#ifdef CONFIG_LOCAL_WDT
		pr_debug("[WDK]cpu %d plug off kick local wdt\n", hotcpu);
		/* kick local wdt */
		/* mpcore_wdt_restart(WD_TYPE_NORMAL); */
		/* disable local watchdog */
		mpcore_wk_wdt_stop();
#endif
		wk_cpu_update_bit_flag(hotcpu, 0);
		/* pr_alert("[WDK]cpu %d plug off, kick wdt\n", hotcpu); */
		break;
#endif				/* CONFIG_HOTPLUG_CPU */
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block cpu_nfb __cpuinitdata = {
	.notifier_call = wk_cpu_callback,
	.priority = 6
};


static int __init init_wk(void)
{
	int res = 0;
	int i = 0;
	/* init api */
	wd_api_init();
	/*  */
	res = get_wd_api(&g_wd_api);
	if (res)
		pr_err("get public api error in wd common driver %d", res);

#ifdef __ENABLE_WDT_SYSFS__
	mtk_rgu_sysfs();
#endif

	cpu_hotplug_disable();

#ifdef __ENABLE_WDT_AT_INIT__

	start_kicker_thread_with_default_setting();

#endif

	wk_proc_init();
	register_cpu_notifier(&cpu_nfb);

	for (i = 0; i < CPU_NR; i++) {
		if (cpu_online(i)) {
			wk_cpu_update_bit_flag(i, 1);
			pr_debug("[WDK]init cpu online %d\n", i);
		} else {
			wk_cpu_update_bit_flag(i, 0);
			pr_debug("[WDK]init cpu offline %d\n", i);
		}
	}
	mtk_wdt_restart(WD_TYPE_NORMAL);	/* for KICK external wdt */
	cpu_hotplug_enable();
	pr_alert("[WDK]init_wk done late_initcall cpus_kick_bit=0x%x -----\n", cpus_kick_bit);

	return 0;
}

static void __exit exit_wk(void)
{
	wk_proc_exit();
	kthread_stop((struct task_struct *)wk_tsk);
}

static int __init init_wk_check_bit(void)
{
	int i = 0;

	pr_debug("[WDK]arch init check_bit=0x%x+++++\n", cpus_kick_bit);
	for (i = 0; i < CPU_NR; i++)
		wk_cpu_update_bit_flag(i, 1);

	pr_debug("[WDK]arch init check_bit=0x%x-----\n", cpus_kick_bit);
	return 0;
}

/*********************************************************************************/
late_initcall(init_wk);
arch_initcall(init_wk_check_bit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mediatek inc.");
MODULE_DESCRIPTION("The watchdog kicker");
