#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/printk.h>

/* Memory lowpower private header file */
#include "../internal.h"

/*
 * config - data collection, trigger LPDMA (4->2)
 */
static int mtkdcs_config(int times, get_range_t func)
{
	pr_debug("%s:+\n", __func__);
	/* Trigger LPDMA (4 -> 2) */
	pr_debug("%s:-\n", __func__);
	return 0;
}

/*
 * enable - Notify PowerMCU to turn off high channels
 */
static int mtkdcs_enable(void)
{
	pr_debug("%s:+\n", __func__);
	/* Turn off high channels */
	pr_debug("%s:-\n", __func__);
	return 0;
}

/*
 * disable - Notify PowerMCU to turn on high channels (May needless)
 */
static int mtkdcs_disable(void)
{
	pr_debug("%s:+\n", __func__);
	/* Turn on high channels */
	pr_debug("%s:-\n", __func__);
	return 0;
}

/*
 * restore - trigger LPDMA (2->4)
 */
static int mtkdcs_restore(void)
{
	pr_debug("%s:+\n", __func__);
	/* Trigger LPDMA (2 -> 4) */
	pr_debug("%s:-\n", __func__);
	return 0;
}

static struct memory_lowpower_operation mtkdcs_handler = {
	.level = MLP_LEVEL_DCS,
	.config = mtkdcs_config,
	.enable = mtkdcs_enable,
	.disable = mtkdcs_disable,
	.restore = mtkdcs_restore,
};

static int __init mtkdcs_init(void)
{
	pr_debug("%s ++\n", __func__);
	/* Register feature operations */
	register_memory_lowpower_operation(&mtkdcs_handler);
	pr_debug("%s --\n", __func__);
	return 0;
}

static void __exit mtkdcs_exit(void)
{
	unregister_memory_lowpower_operation(&mtkdcs_handler);
}

late_initcall(mtkdcs_init);
module_exit(mtkdcs_exit);
