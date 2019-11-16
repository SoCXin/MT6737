
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include "mt_ppm_internal.h"


static enum ppm_power_state ppm_perfserv_get_power_state_cb(enum ppm_power_state cur_state);
static void ppm_perfserv_update_limit_cb(enum ppm_power_state new_state);
static void ppm_perfserv_status_change_cb(bool enable);
static void ppm_perfserv_mode_change_cb(enum ppm_mode mode);

/* other members will init by ppm_main */
static struct ppm_policy_data perfserv_policy = {
	.name			= __stringify(PPM_POLICY_PERF_SERV),
	.lock			= __MUTEX_INITIALIZER(perfserv_policy.lock),
	.policy			= PPM_POLICY_PERF_SERV,
	.priority		= PPM_POLICY_PRIO_PERFORMANCE_BASE,
	.get_power_state_cb	= ppm_perfserv_get_power_state_cb,
	.update_limit_cb	= ppm_perfserv_update_limit_cb,
	.status_change_cb	= ppm_perfserv_status_change_cb,
	.mode_change_cb		= ppm_perfserv_mode_change_cb,
};

struct ppm_perfserv_data {
	int min_freq;
	int min_core_num;
	int max_available_freq;
} perfserv_data = {
	.min_freq = -1,
	.min_core_num = -1,
	.max_available_freq = -1,
};

/* MUST in lock */
bool ppm_perfserv_is_policy_active(void)
{
	if (!perfserv_policy.req.perf_idx
		&& perfserv_data.min_freq == -1
		&& perfserv_data.min_core_num == -1
		&& perfserv_data.max_available_freq == -1)
		return false;
	else
		return true;
}

static enum ppm_power_state ppm_perfserv_get_power_state_cb(enum ppm_power_state cur_state)
{
	/* TODO: check min freq / max available freq */
	if (perfserv_policy.req.perf_idx)
		return ppm_hica_get_state_by_perf_idx(cur_state, perfserv_policy.req.perf_idx);
	else
		return PPM_POWER_STATE_NONE;
}

static void ppm_perfserv_update_limit_cb(enum ppm_power_state new_state)
{
	unsigned int index, i;
	struct ppm_power_tbl_data power_table = ppm_get_power_table();

	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: perfserv policy update limit for new state = %s\n",
		__func__, ppm_get_power_state_name(new_state));

	/* TODO: add min freq/core check?? */
	if (perfserv_policy.req.perf_idx) {
		ppm_hica_set_default_limit_by_state(new_state, &perfserv_policy);

		/* get limit according to perf idx */
		index = ppm_hica_get_table_idx_by_perf(new_state, perfserv_policy.req.perf_idx);
		if (index != -1) {
			for (i = 0; i < perfserv_policy.req.cluster_num; i++) {
				perfserv_policy.req.limit[i].min_cpu_core =
					power_table.power_tbl[index].cluster_cfg[i].core_num;
				perfserv_policy.req.limit[i].min_cpufreq_idx =
					power_table.power_tbl[index].cluster_cfg[i].opp_lv;

				/* error check */
				if (perfserv_policy.req.limit[i].min_cpufreq_idx == -1)
					perfserv_policy.req.limit[i].max_cpufreq_idx = -1;
			}
		} else
			ppm_dbg("@%s: index not found!", __func__);
	}

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_perfserv_status_change_cb(bool enable)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: perfserv policy status changed to %d\n", __func__, enable);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_perfserv_mode_change_cb(enum ppm_mode mode)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: ppm mode changed to %d\n", __func__, mode);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static int ppm_perfserv_perf_idx_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "perf idx = %d\n", perfserv_policy.req.perf_idx);

	return 0;
}

static ssize_t ppm_perfserv_perf_idx_proc_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)
{
	unsigned int perf_idx;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &perf_idx)) {
		ppm_lock(&perfserv_policy.lock);

		if (!perfserv_policy.is_enabled) {
			ppm_err("@%s: perfserv policy is not enabled!\n", __func__);
			ppm_unlock(&perfserv_policy.lock);
			goto out;
		}

		perfserv_policy.req.perf_idx = perf_idx;
		perfserv_policy.is_activated = ppm_perfserv_is_policy_active();

		ppm_unlock(&perfserv_policy.lock);
		ppm_task_wakeup();
	} else
		ppm_err("@%s: Invalid input!\n", __func__);

out:
	free_page((unsigned long)buf);
	return count;
}

static int ppm_perfserv_min_perf_idx_proc_show(struct seq_file *m, void *v)
{
	struct ppm_power_tbl_data power_table = ppm_get_power_table();
	unsigned int size = power_table.nr_power_tbl;

	seq_printf(m, "%d\n", power_table.power_tbl[size - 1].perf_idx);

	return 0;
}

static int ppm_perfserv_max_perf_idx_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", ppm_get_power_table().power_tbl[0].perf_idx);

	return 0;
}

PROC_FOPS_RW(perfserv_perf_idx);
PROC_FOPS_RO(perfserv_min_perf_idx);
PROC_FOPS_RO(perfserv_max_perf_idx);

static int __init ppm_perfserv_policy_init(void)
{
	int i, ret = 0;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(perfserv_perf_idx),
		PROC_ENTRY(perfserv_min_perf_idx),
		PROC_ENTRY(perfserv_max_perf_idx),
	};

	FUNC_ENTER(FUNC_LV_POLICY);

	/* create procfs */
	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, policy_dir, entries[i].fops)) {
			ppm_err("%s(), create /proc/ppm/policy/%s failed\n", __func__, entries[i].name);
			ret = -EINVAL;
			goto out;
		}
	}

	if (ppm_main_register_policy(&perfserv_policy)) {
		ppm_err("@%s: perfserv policy register failed\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	ppm_info("@%s: register %s done!\n", __func__, perfserv_policy.name);

out:
	FUNC_EXIT(FUNC_LV_POLICY);

	return ret;
}

static void __exit ppm_perfserv_policy_exit(void)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_main_unregister_policy(&perfserv_policy);

	FUNC_EXIT(FUNC_LV_POLICY);
}

module_init(ppm_perfserv_policy_init);
module_exit(ppm_perfserv_policy_exit);

