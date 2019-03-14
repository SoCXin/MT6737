
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#include "mt_ppm_internal.h"


#define hica_dbg(fmt, args...)				\
	do {						\
		if (hica_debug_enable)			\
			ppm_info("[HICA]"fmt, ##args);	\
	} while (0)

#define PROC_FOPS_RO_HICA_SETTINGS(name, var)						\
	static int ppm_##name##_proc_show(struct seq_file *m, void *v)			\
	{										\
		struct ppm_state_transfer *p =						\
			(struct ppm_state_transfer *)m->private;			\
											\
		seq_printf(m, "%u\n", var);						\
		return 0;								\
	}										\
	PROC_FOPS_RO(name)

#define PROC_FOPS_RW_HICA_SETTINGS(name, var)						\
	static int ppm_##name##_proc_show(struct seq_file *m, void *v)			\
	{										\
		struct ppm_state_transfer *p =						\
			(struct ppm_state_transfer *)m->private;			\
											\
		seq_printf(m, "%u\n", var);						\
		return 0;								\
	}										\
	static ssize_t ppm_##name##_proc_write(struct file *file,			\
			const char __user *buffer, size_t count, loff_t *pos)		\
	{										\
		struct ppm_state_transfer *p =						\
			(struct ppm_state_transfer *)PDE_DATA(file_inode(file));	\
		unsigned int setting;							\
		char *buf = ppm_copy_from_user_for_proc(buffer, count);			\
											\
		if (!buf)								\
			return -EINVAL;							\
											\
		if (!kstrtouint(buf, 10, &setting))					\
			var = setting;							\
		else									\
			ppm_err("@%s: Bad argument(%d)!\n", __func__, setting);		\
											\
		free_page((unsigned long)buf);						\
		return count;								\
	}										\
	PROC_FOPS_RW(name)


static int hica_debug_enable;
static enum ppm_power_state fix_power_state = PPM_POWER_STATE_NONE;

static void ppm_hica_reset_data_for_state(enum ppm_power_state new_state);
static void ppm_hica_update_limit_cb(enum ppm_power_state new_state);
static void ppm_hica_status_change_cb(bool enable);
static void ppm_hica_mode_change_cb(enum ppm_mode mode);


/* other members will init by ppm_main */
static struct ppm_policy_data hica_policy = {
	.name			= __stringify(PPM_POLICY_HICA),
	.lock			= __MUTEX_INITIALIZER(hica_policy.lock),
	.policy			= PPM_POLICY_HICA,
	.priority		= PPM_POLICY_PRIO_SYSTEM_BASE,
	.get_power_state_cb	= NULL,	/* No need */
	.update_limit_cb	= ppm_hica_update_limit_cb,
	.status_change_cb	= ppm_hica_status_change_cb,
	.mode_change_cb		= ppm_hica_mode_change_cb,
};

struct ppm_hica_algo_data ppm_hica_algo_data = {
	.cur_state = PPM_POWER_STATE_4LL_L,
	.new_state = PPM_POWER_STATE_NONE,

	.ppm_cur_loads = 0,
	.ppm_cur_tlp = 0,
	.ppm_cur_nr_heavy_task = 0,
};

void mt_ppm_hica_update_algo_data(unsigned int cur_loads,
					unsigned int cur_nr_heavy_task, unsigned int cur_tlp)
{
	struct ppm_power_state_data *state_info = ppm_get_power_state_info();
	struct ppm_state_transfer_data *data;
	enum ppm_power_state cur_state;
	enum ppm_mode cur_mode;
	int i, j;

	FUNC_ENTER(FUNC_LV_HICA);

	ppm_lock(&hica_policy.lock);

	if (!hica_policy.is_enabled)
		goto end;

	ppm_hica_algo_data.ppm_cur_loads = cur_loads;
	ppm_hica_algo_data.ppm_cur_tlp = cur_tlp;
	ppm_hica_algo_data.ppm_cur_nr_heavy_task = cur_nr_heavy_task;

	cur_state = ppm_hica_algo_data.cur_state;
	cur_mode = ppm_main_info.cur_mode;

	hica_dbg("cur_loads = %d, cur_tlp = %d, cur_nr_heavy_task = %d, cur_state = %s, cur_mode = %d\n",
		cur_loads, cur_tlp, cur_nr_heavy_task, ppm_get_power_state_name(cur_state), cur_mode);

	if (!ppm_main_info.is_enabled || ppm_main_info.is_in_suspend || cur_state == PPM_POWER_STATE_NONE)
		goto end;

	/* Power state is fixed by user, skip HICA state calculation */
	if (fix_power_state != PPM_POWER_STATE_NONE)
		goto end;

	for (i = 0; i < 2; i++) {
		data = (i == 0) ? state_info[cur_state].transfer_by_perf
				: state_info[cur_state].transfer_by_pwr;

		for (j = 0; j < data->size; j++) {
			if (!data->transition_data[j].transition_rule
				|| !((1 << cur_mode) & data->transition_data[j].mode_mask))
				continue;

			if (data->transition_data[j].transition_rule(
				ppm_hica_algo_data, &data->transition_data[j])) {
				ppm_hica_algo_data.new_state = data->transition_data[j].next_state;
				hica_dbg("[%s(%d)] Need state transfer: %s --> %s\n",
					(i == 0) ? "PERF" : "PWR",
					j,
					ppm_get_power_state_name(cur_state),
					ppm_get_power_state_name(ppm_hica_algo_data.new_state)
					);
				goto end;
			} else
				hica_dbg("[%s(%d)]hold in %s state, loading_cnt = %d, freq_cnt = %d\n",
					(i == 0) ? "PERF" : "PWR",
					j,
					ppm_get_power_state_name(cur_state),
					data->transition_data[j].loading_hold_cnt,
					data->transition_data[j].freq_hold_cnt
					);
		}
	}

end:
	ppm_unlock(&hica_policy.lock);
	FUNC_EXIT(FUNC_LV_HICA);
}

bool ppm_hica_is_log_enabled(void)
{
	return (hica_debug_enable) ? true : false;
}

unsigned int ppm_hica_get_table_idx_by_perf(enum ppm_power_state state, unsigned int perf_idx)
{
	int i;
	struct ppm_power_state_data *state_info;
	const struct ppm_state_sorted_pwr_tbl_data *tbl;
	struct ppm_power_tbl_data power_table = ppm_get_power_table();

	if (state > NR_PPM_POWER_STATE || (perf_idx == -1)) {
		ppm_warn("Invalid argument: state = %d, pwr_idx = %d\n", state, perf_idx);
		return -1;
	}

	/* search whole tlp table */
	if (state == PPM_POWER_STATE_NONE) {
		for (i = 1; i < power_table.nr_power_tbl; i++) {
			if (power_table.power_tbl[i].perf_idx < perf_idx)
				return power_table.power_tbl[i-1].index;
		}
	} else {
		state_info = ppm_get_power_state_info();
		tbl = state_info[state].perf_sorted_tbl;

		/* return -1 (not found) if input is larger than max perf_idx in table */
		if (tbl->sorted_tbl[0].value < perf_idx)
			return -1;

		for (i = 1; i < tbl->size; i++) {
			if (tbl->sorted_tbl[i].value < perf_idx)
				return tbl->sorted_tbl[i-1].index;
		}
	}

	/* not found */
	return -1;
}

unsigned int ppm_hica_get_table_idx_by_pwr(enum ppm_power_state state, unsigned int pwr_idx)
{
	int i;
	struct ppm_power_state_data *state_info;
	const struct ppm_state_sorted_pwr_tbl_data *tbl;
	struct ppm_power_tbl_data power_table = ppm_get_power_table();

	if (state > NR_PPM_POWER_STATE || (pwr_idx == ~0)) {
		ppm_warn("Invalid argument: state = %d, pwr_idx = %d\n", state, pwr_idx);
		return -1;
	}

	/* search whole tlp table */
	if (state == PPM_POWER_STATE_NONE) {
		for_each_pwr_tbl_entry(i, power_table) {
			if (power_table.power_tbl[i].power_idx <= pwr_idx)
				return i;
		}
	} else {
		state_info = ppm_get_power_state_info();
		tbl = state_info[state].pwr_sorted_tbl;

		for (i = 0; i < tbl->size; i++) {
			if (tbl->sorted_tbl[i].value <= pwr_idx)
				return tbl->sorted_tbl[i].advise_index;
		}
	}

	/* not found */
	return -1;
}

void ppm_hica_set_default_limit_by_state(enum ppm_power_state state,
					struct ppm_policy_data *policy)
{
	unsigned int i;
	struct ppm_power_state_data *state_info = ppm_get_power_state_info();

	FUNC_ENTER(FUNC_LV_HICA);

	for (i = 0; i < policy->req.cluster_num; i++) {
		if (state >= PPM_POWER_STATE_NONE) {
			if (state > NR_PPM_POWER_STATE)
				ppm_err("@%s: Invalid PPM state(%d)\n", __func__, state);

			policy->req.limit[i].min_cpu_core = get_cluster_min_cpu_core(i);
			policy->req.limit[i].max_cpu_core = get_cluster_max_cpu_core(i);
			policy->req.limit[i].min_cpufreq_idx = get_cluster_min_cpufreq_idx(i);
			policy->req.limit[i].max_cpufreq_idx = get_cluster_max_cpufreq_idx(i);
		} else {
			policy->req.limit[i].min_cpu_core =
				state_info[state].cluster_limit->state_limit[i].min_cpu_core;
			policy->req.limit[i].max_cpu_core =
				state_info[state].cluster_limit->state_limit[i].max_cpu_core;
			policy->req.limit[i].min_cpufreq_idx =
				state_info[state].cluster_limit->state_limit[i].min_cpufreq_idx;
			policy->req.limit[i].max_cpufreq_idx =
				state_info[state].cluster_limit->state_limit[i].max_cpufreq_idx;
		}
	}

	FUNC_EXIT(FUNC_LV_HICA);
}

enum ppm_power_state ppm_hica_get_state_by_perf_idx(enum ppm_power_state state, unsigned int perf_idx)
{
	enum ppm_power_state new_state = state;
	unsigned int index = 0, level = 0, found = 0;

	FUNC_ENTER(FUNC_LV_HICA);

	/* Power state is fixed by user */
	if (fix_power_state != PPM_POWER_STATE_NONE)
		return fix_power_state;

	while (1) {
		index = ppm_hica_get_table_idx_by_perf(new_state, perf_idx);
		ppm_ver("@%s: index = %d\n", __func__, index);
		if (index != -1) {
			found = 1;
			break;
		}

		new_state = ppm_find_next_state(state, &level, PERFORMANCE);
		if (new_state == PPM_POWER_STATE_NONE) {
			ppm_warn("HICA state not found for idx = %d, cur_state = %d\n",
					perf_idx, state);
			break;
		}

		level++; /* to find next state */
	}

	FUNC_EXIT(FUNC_LV_HICA);

	return (found) ? new_state : PPM_POWER_STATE_NONE;
}

enum ppm_power_state ppm_hica_get_state_by_pwr_budget(enum ppm_power_state state, unsigned int budget)
{
	enum ppm_power_state new_state = state;
	unsigned int index = 0, level = 0, found = 0;

	FUNC_ENTER(FUNC_LV_HICA);

	/* Power state is fixed by user */
	if (fix_power_state != PPM_POWER_STATE_NONE)
		return fix_power_state;

	while (1) {
		index = ppm_hica_get_table_idx_by_pwr(new_state, budget);
		ppm_ver("@%s: index = %d\n", __func__, index);
		if (index != -1) {
			found = 1;
			break;
		}

		new_state = ppm_find_next_state(state, &level, LOW_POWER);
		if (new_state == PPM_POWER_STATE_NONE) {
			ppm_warn("HICA state not found for budget = %d, cur_state = %d\n",
					budget, state);
			break;
		}

		level++; /* to find next state */
	}

	FUNC_EXIT(FUNC_LV_HICA);

	return (found) ? new_state : PPM_POWER_STATE_NONE;
}

enum ppm_power_state ppm_hica_get_cur_state(void)
{
	enum ppm_power_state state;

	FUNC_ENTER(FUNC_LV_HICA);

	ppm_lock(&hica_policy.lock);

	if (!hica_policy.is_enabled) {
		state = PPM_POWER_STATE_NONE;
		goto end;
	}

	if (!hica_policy.is_activated) {
		unsigned int i;

		for (i = 0; i < hica_policy.req.cluster_num; i++) {
			hica_policy.req.limit[i].min_cpufreq_idx = get_cluster_min_cpufreq_idx(i);
			hica_policy.req.limit[i].max_cpufreq_idx = get_cluster_max_cpufreq_idx(i);
			hica_policy.req.limit[i].min_cpu_core = get_cluster_min_cpu_core(i);
			hica_policy.req.limit[i].max_cpu_core = get_cluster_max_cpu_core(i);
		}
		state = PPM_POWER_STATE_NONE;
	} else
		state = (fix_power_state != PPM_POWER_STATE_NONE)
			? fix_power_state : ppm_hica_algo_data.new_state;

end:
	ppm_unlock(&hica_policy.lock);

	FUNC_EXIT(FUNC_LV_HICA);

	return state;
}

void ppm_hica_fix_root_cluster_changed(int cluster_id)
{
	enum ppm_power_state new_state;

	FUNC_ENTER(FUNC_LV_HICA);

	new_state = ppm_change_state_with_fix_root_cluster(ppm_hica_algo_data.cur_state, cluster_id);
	if (new_state != ppm_hica_algo_data.cur_state) {
		ppm_info("@%s: ppm state change to %s due to root cluster is fixed at %d\n",
			__func__, ppm_get_power_state_name(new_state), ppm_main_info.fixed_root_cluster);
		ppm_lock(&hica_policy.lock);
		ppm_hica_algo_data.new_state = new_state;
		ppm_unlock(&hica_policy.lock);
		ppm_task_wakeup();
	}

	FUNC_EXIT(FUNC_LV_HICA);
}

static void ppm_hica_reset_data_for_state(enum ppm_power_state state)
{
	struct ppm_power_state_data *state_info = ppm_get_power_state_info();
	struct ppm_state_transfer_data *data;
	int i, j;

	FUNC_ENTER(FUNC_LV_HICA);

	for (i = 0; i < 2; i++) {
		if (i == 0)
			data = state_info[state].transfer_by_pwr;
		else
			data = state_info[state].transfer_by_perf;

		for (j = 0; j < data->size; j++) {
			data->transition_data[j].freq_hold_cnt = 0;
			data->transition_data[j].loading_hold_cnt = 0;
		}
	}

	FUNC_EXIT(FUNC_LV_HICA);
}

static void ppm_hica_update_limit_cb(enum ppm_power_state new_state)
{
	FUNC_ENTER(FUNC_LV_HICA);

	ppm_ver("@%s: hica policy update limit for new state = %s\n",
		__func__, ppm_get_power_state_name(new_state));

	ppm_hica_set_default_limit_by_state(new_state, &hica_policy);

	if (new_state >= NR_PPM_POWER_STATE)
		hica_dbg("PPM current state is NONE, skip HICA result...\n");
	else if (new_state != ppm_hica_algo_data.cur_state) {
		/* update HICA algo data for state transition */
		hica_dbg("state transfer (final): %s --> %s\n",
			ppm_get_power_state_name(ppm_hica_algo_data.cur_state),
			ppm_get_power_state_name(new_state)
			);
		ppm_hica_algo_data.cur_state = new_state;
		ppm_hica_reset_data_for_state(new_state);
	}

	FUNC_EXIT(FUNC_LV_HICA);
}

static void ppm_hica_status_change_cb(bool enable)
{
	FUNC_ENTER(FUNC_LV_HICA);

	/* HICA policy is default active if it is enabled */
	if (enable)
		hica_policy.is_activated = true;

	ppm_info("@%s: hica policy status changed to %d\n", __func__, enable);

	FUNC_EXIT(FUNC_LV_HICA);
}

static void ppm_hica_mode_change_cb(enum ppm_mode mode)
{
	FUNC_ENTER(FUNC_LV_HICA);

	ppm_info("@%s: ppm mode changed to %d\n", __func__, mode);

	FUNC_EXIT(FUNC_LV_HICA);
}

static int ppm_hica_debug_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "hica_debug_enable = %d\n", hica_debug_enable);

	return 0;
}

static ssize_t ppm_hica_debug_proc_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)
{
	unsigned int dbg;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &dbg))
		hica_debug_enable = dbg;
	else
		ppm_err("echo (1 or 0) > /proc/ppm/policy/hica_debug\n");

	free_page((unsigned long)buf);
	return count;
}

static int ppm_hica_power_state_proc_show(struct seq_file *m, void *v)
{
	struct ppm_power_state_data *state_info = ppm_get_power_state_info();
	enum ppm_power_state cur_state = (fix_power_state != PPM_POWER_STATE_NONE)
		? fix_power_state : ppm_hica_algo_data.cur_state;
	int i;

	seq_printf(m, "\nhica_state = %d (%s)\n\n", cur_state, ppm_get_power_state_name(cur_state));

	for_each_ppm_power_state(i)
		seq_printf(m, "[%d] %s\n", state_info[i].state, state_info[i].name);

	seq_puts(m, "\nNote: echo -1 to re-enable HICA algo!\n");
	return 0;
}

static ssize_t ppm_hica_power_state_proc_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)
{
	int state;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &state))
		fix_power_state = (state == -1) ? PPM_POWER_STATE_NONE : state;
	else
		ppm_err("echo (state idx) > /proc/ppm/policy/hica_power_state\n");

	free_page((unsigned long)buf);
	return count;
}

PROC_FOPS_RW(hica_debug);
PROC_FOPS_RW(hica_power_state);
PROC_FOPS_RW_HICA_SETTINGS(mode_mask, p->mode_mask);
PROC_FOPS_RW_HICA_SETTINGS(loading_delta, p->loading_delta);
PROC_FOPS_RW_HICA_SETTINGS(loading_hold_time, p->loading_hold_time);
PROC_FOPS_RO_HICA_SETTINGS(loading_hold_cnt, p->loading_hold_cnt);
PROC_FOPS_RW_HICA_SETTINGS(loading_bond, p->loading_bond);
PROC_FOPS_RW_HICA_SETTINGS(freq_hold_time, p->freq_hold_time);
PROC_FOPS_RO_HICA_SETTINGS(freq_hold_cnt, p->freq_hold_cnt);
PROC_FOPS_RW_HICA_SETTINGS(tlp_bond, p->tlp_bond);

static int __init ppm_hica_policy_init(void)
{
	int ret = 0, i, j, k;
	struct ppm_power_state_data *state_info = ppm_get_power_state_info();
	struct proc_dir_entry *hica_setting_dir = NULL;
	struct proc_dir_entry *trans_rule_dir = NULL;
	struct ppm_state_transfer_data *data;
	char str[32];

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(hica_debug),
		PROC_ENTRY(hica_power_state),
	};

	const struct pentry trans_rule_entries[] = {
		PROC_ENTRY(mode_mask),
		PROC_ENTRY(loading_delta),
		PROC_ENTRY(loading_hold_time),
		PROC_ENTRY(loading_hold_cnt),
		PROC_ENTRY(loading_bond),
		PROC_ENTRY(freq_hold_time),
		PROC_ENTRY(freq_hold_cnt),
		PROC_ENTRY(tlp_bond),
	};

	FUNC_ENTER(FUNC_LV_HICA);

	/* create procfs */
	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, policy_dir, entries[i].fops)) {
			ppm_err("%s(), create /proc/ppm/policy/%s failed\n", __func__, entries[i].name);
			ret = -EINVAL;
			goto out;
		}
	}

	hica_setting_dir = proc_mkdir("hica_settings", policy_dir);
	if (!hica_setting_dir) {
		ppm_err("fail to create /proc/ppm/policy/hica_settings dir\n");
		return -ENOMEM;
	}

	for (i = 0; i < NR_PPM_POWER_STATE * 2; i++) {
		data = (i % 2 == 0) ? state_info[i / 2].transfer_by_perf
				: state_info[i / 2].transfer_by_pwr;

		/* create dir for transfer rule */
		for (j = 0; j < data->size; j++) {
			if (!data->transition_data[j].transition_rule)
				continue;

			sprintf(str, "%s_to_%s", ppm_get_power_state_name(i / 2),
				ppm_get_power_state_name(data->transition_data[j].next_state));
			trans_rule_dir = proc_mkdir(str, hica_setting_dir);
			if (!trans_rule_dir) {
				ppm_err("fail to create /proc/ppm/policy/hica_settings/%s dir\n", str);
				return -ENOMEM;
			}

			/* create node for each parameter in trans_rule_entries */
			for (k = 0; k < ARRAY_SIZE(trans_rule_entries); k++) {
				if (!proc_create_data(trans_rule_entries[k].name,
					S_IRUGO | S_IWUSR | S_IWGRP, trans_rule_dir,
					trans_rule_entries[k].fops, &data->transition_data[j]))
					ppm_err("fail to create /proc/ppm/policy/hica_settings/%s/%s dir\n",
						str, trans_rule_entries[k].name);
			}
		}
	}

	if (ppm_main_register_policy(&hica_policy)) {
		ppm_err("@%s: hica policy register failed\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	/* default enabled */
	hica_policy.is_activated = true;

	ppm_info("@%s: register %s done!\n", __func__, hica_policy.name);

out:
	FUNC_EXIT(FUNC_LV_HICA);

	return ret;
}

static void __exit ppm_hica_policy_exit(void)
{
	FUNC_ENTER(FUNC_LV_HICA);

	ppm_main_unregister_policy(&hica_policy);

	FUNC_EXIT(FUNC_LV_HICA);
}

module_init(ppm_hica_policy_init);
module_exit(ppm_hica_policy_exit);

