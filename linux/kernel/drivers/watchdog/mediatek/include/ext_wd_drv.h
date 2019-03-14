#ifndef __EXT_WD_DRV_H
#define __EXT_WD_DRV_H
#include <linux/types.h>
#include <linux/kthread.h> /*define NR_CPUS*/
#include <mach/wd_api.h>


/* direct api */
int mtk_wdt_request_mode_set(int mark_bit, WD_REQ_MODE mode);
int mtk_wdt_request_en_set(int mark_bit, WD_REQ_CTL en);
void wdt_arch_reset(char mode);
void mtk_wdt_restart(enum wd_restart_type type);
void mtk_wdt_mode_config(bool dual_mode_en, bool irq, bool ext_en, bool ext_pol, bool wdt_en);
void mtk_wdt_set_time_out_value(unsigned int value);
int mtk_wdt_confirm_hwreboot(void);
int mtk_wdt_enable(enum wk_wdt_en en);
void mtk_wd_resume(void);
void mtk_wd_suspend(void);
void wdt_dump_reg(void);
/* used for extend request */
/* 0x10000000 for ddr reseved mode */
/* 0x20000000 for 75 irq or reboot mode */
int mtk_wdt_swsysret_config(int bit, int set_value);
int mtk_rgu_dram_reserved(int enable);

/* direct api */
int mpcore_wk_wdt_config(int reserved, int reserved2, int timeout_val);
int mpcore_wdt_restart(WD_RES_TYPE type);
int local_wdt_enable(enum wk_wdt_en en);
/* used for extend request */
int mtk_local_wdt_misc_config(int bit, int set_value, int *reserved);
void mpcore_wk_wdt_stop(void);

#if NR_CPUS == 1
#define nr_cpu_ids		1
#else
extern int nr_cpu_ids;
#endif

#define __ENABLE_WDT_SYSFS__
#ifdef __ENABLE_WDT_SYSFS__
#include <linux/proc_fs.h>
#include <linux/sysfs.h>
/*---------------------------------------------------------------------------*/
/*define sysfs entry for configuring debug level and sysrq*/
ssize_t mtk_rgu_attr_show(struct kobject *kobj, struct attribute *attr, char *buffer);
ssize_t mtk_rgu_attr_store(struct kobject *kobj, struct attribute *attr, const char *buffer,
			   size_t size);
ssize_t mtk_rgu_pause_wdt_show(struct kobject *kobj, char *page);
ssize_t mtk_rgu_pause_wdt_store(struct kobject *kobj, const char *page, size_t size);
#endif

/* end */
#endif
