
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#else
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

#include "mt_ppm_internal.h"


static enum ppm_power_state ppm_lcmoff_get_power_state_cb(enum ppm_power_state cur_state);
static void ppm_lcmoff_update_limit_cb(enum ppm_power_state new_state);
static void ppm_lcmoff_status_change_cb(bool enable);
static void ppm_lcmoff_mode_change_cb(enum ppm_mode mode);

/* other members will init by ppm_main */
static struct ppm_policy_data lcmoff_policy = {
	.name			= __stringify(PPM_POLICY_LCM_OFF),
	.lock			= __MUTEX_INITIALIZER(lcmoff_policy.lock),
	.policy			= PPM_POLICY_LCM_OFF,
	.priority		= PPM_POLICY_PRIO_USER_SPECIFY_BASE,
	.get_power_state_cb	= ppm_lcmoff_get_power_state_cb,
	.update_limit_cb	= ppm_lcmoff_update_limit_cb,
	.status_change_cb	= ppm_lcmoff_status_change_cb,
	.mode_change_cb		= ppm_lcmoff_mode_change_cb,
};

static enum ppm_power_state ppm_lcmoff_get_power_state_cb(enum ppm_power_state cur_state)
{
	return cur_state;
}

static void ppm_lcmoff_update_limit_cb(enum ppm_power_state new_state)
{
	unsigned int i;

	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: lcmoff policy update limit for new state = %s\n",
		__func__, ppm_get_power_state_name(new_state));

	ppm_hica_set_default_limit_by_state(new_state, &lcmoff_policy);

	for (i = 0; i < lcmoff_policy.req.cluster_num; i++) {
		if (lcmoff_policy.req.limit[i].min_cpufreq_idx != -1) {
			int idx = ppm_main_freq_to_idx(i, get_cluster_lcmoff_min_freq(i), CPUFREQ_RELATION_L);

			lcmoff_policy.req.limit[i].min_cpufreq_idx =
				MIN(lcmoff_policy.req.limit[i].min_cpufreq_idx, idx);
		}
	}

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_lcmoff_status_change_cb(bool enable)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: lcmoff policy status changed to %d\n", __func__, enable);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_lcmoff_mode_change_cb(enum ppm_mode mode)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: ppm mode changed to %d\n", __func__, mode);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_lcmoff_switch(int onoff)
{
	unsigned int i;

	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_info("@%s: onoff = %d\n", __func__, onoff);

	ppm_lock(&lcmoff_policy.lock);

	/* onoff = 0: LCM OFF */
	/* others: LCM ON */
	if (onoff) {
		/* deactivate lcmoff policy */
		if (lcmoff_policy.is_activated) {
			lcmoff_policy.is_activated = false;
			for (i = 0; i < lcmoff_policy.req.cluster_num; i++) {
				lcmoff_policy.req.limit[i].min_cpufreq_idx = get_cluster_min_cpufreq_idx(i);
				lcmoff_policy.req.limit[i].max_cpufreq_idx = get_cluster_max_cpufreq_idx(i);
				lcmoff_policy.req.limit[i].min_cpu_core = get_cluster_min_cpu_core(i);
				lcmoff_policy.req.limit[i].max_cpu_core = get_cluster_max_cpu_core(i);
			}
			ppm_unlock(&lcmoff_policy.lock);
			ppm_task_wakeup();
		} else
			ppm_unlock(&lcmoff_policy.lock);
	} else {
		/* activate lcmoff policy */
		if (lcmoff_policy.is_enabled) {
			lcmoff_policy.is_activated = true;
			ppm_unlock(&lcmoff_policy.lock);
			ppm_task_wakeup();
		} else
			ppm_unlock(&lcmoff_policy.lock);
	}

	FUNC_EXIT(FUNC_LV_POLICY);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ppm_lcmoff_early_suspend(struct early_suspend *h)
{
	FUNC_ENTER(FUNC_LV_POLICY);
	ppm_lcmoff_switch(0);
	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_lcmoff_late_resume(struct early_suspend *h)
{
	FUNC_ENTER(FUNC_LV_POLICY);
	ppm_lcmoff_switch(1);
	FUNC_EXIT(FUNC_LV_POLICY);
}

static struct early_suspend ppm_lcmoff_es_handler = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 200,
	.suspend = ppm_lcmoff_early_suspend,
	.resume = ppm_lcmoff_late_resume,
};
#else
static int ppm_lcmoff_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int blank;

	FUNC_ENTER(FUNC_LV_POLICY);

	blank = *(int *)evdata->data;
	ppm_ver("@%s: blank = %d, event = %lu\n", __func__, blank, event);

	/* skip if it's not a blank event */
	if (event != FB_EVENT_BLANK)
		return 0;

	switch (blank) {
	/* LCM ON */
	case FB_BLANK_UNBLANK:
		ppm_lcmoff_switch(1);
		break;
	/* LCM OFF */
	case FB_BLANK_POWERDOWN:
		ppm_lcmoff_switch(0);
		break;
	default:
		break;
	}

	FUNC_EXIT(FUNC_LV_POLICY);

	return 0;
}

static struct notifier_block ppm_lcmoff_fb_notifier = {
	.notifier_call = ppm_lcmoff_fb_notifier_callback,
};
#endif

static int __init ppm_lcmoff_policy_init(void)
{
	int ret = 0;

	FUNC_ENTER(FUNC_LV_POLICY);

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&ppm_lcmoff_es_handler);
#else
	if (fb_register_client(&ppm_lcmoff_fb_notifier)) {
		ppm_err("@%s: lcmoff policy register FB client failed!\n", __func__);
		ret = -EINVAL;
		goto out;
	}
#endif

	if (ppm_main_register_policy(&lcmoff_policy)) {
		ppm_err("@%s: lcmoff policy register failed\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	ppm_info("@%s: register %s done!\n", __func__, lcmoff_policy.name);

out:
	FUNC_EXIT(FUNC_LV_POLICY);

	return ret;
}

static void __exit ppm_lcmoff_policy_exit(void)
{
	FUNC_ENTER(FUNC_LV_POLICY);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ppm_lcmoff_es_handler);
#else
	fb_unregister_client(&ppm_lcmoff_fb_notifier);
#endif

	ppm_main_unregister_policy(&lcmoff_policy);

	FUNC_EXIT(FUNC_LV_POLICY);
}
#ifdef CONFIG_HAS_EARLYSUSPEND
module_init(ppm_lcmoff_policy_init);
#else
/* Cannot init before FB driver */
late_initcall(ppm_lcmoff_policy_init);
#endif
module_exit(ppm_lcmoff_policy_exit);

