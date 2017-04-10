/*
 * drivers/cpufreq/cpufreq_interactive.c
 *
 * Copyright (C) 2010 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Mike Chan (mike@android.com)
 *
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/ipa.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/rwsem.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/tick.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/slab.h>
<<<<<<< HEAD
#include <linux/pm_qos.h>
#include <linux/state_notifier.h>
static struct notifier_block interactive_state_notif;

#ifdef CONFIG_ARM_EXYNOS_MP_CPUFREQ
#include <mach/cpufreq.h>
#endif
#include "cpu_load_metric.h"
=======
#include <linux/kernel_stat.h>
#include <linux/display_state.h>
#include <asm/cputime.h>
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor

#define CREATE_TRACE_POINTS
#include <trace/events/cpufreq_interactive.h>

<<<<<<< HEAD
struct cpufreq_interactive_cpuinfo {
	struct timer_list cpu_timer;
	struct timer_list cpu_slack_timer;
	spinlock_t load_lock; /* protects the next 4 fields */
	u64 time_in_idle;
	u64 time_in_idle_timestamp;
	u64 cputime_speedadj;
	u64 cputime_speedadj_timestamp;
=======
struct cpufreq_interactive_policyinfo {
	struct timer_list policy_timer;
	struct timer_list policy_slack_timer;
	spinlock_t load_lock; /* protects load tracking stat */
	u64 last_evaluated_jiffy;
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
	struct cpufreq_policy *policy;
	struct cpufreq_frequency_table *freq_table;
	spinlock_t target_freq_lock; /*protects target freq */
	unsigned int target_freq;
	unsigned int floor_freq;
<<<<<<< HEAD
	u64 pol_floor_val_time; /* policy floor_validate_time */
	u64 loc_floor_val_time; /* per-cpu floor_validate_time */
	u64 pol_hispeed_val_time; /* policy hispeed_validate_time */
	u64 loc_hispeed_val_time; /* per-cpu hispeed_validate_time */
	u64 floor_validate_time;
	u64 hispeed_validate_time;
=======
	unsigned int min_freq;
	u64 floor_validate_time;
	u64 hispeed_validate_time;
	u64 max_freq_hyst_start_time;
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
	struct rw_semaphore enable_sem;
	int governor_enabled;
<<<<<<< HEAD
=======
	struct cpufreq_interactive_tunables *cached_tunables;
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
};

/* Protected by per-policy load_lock */
struct cpufreq_interactive_cpuinfo {
	u64 time_in_idle;
	u64 time_in_idle_timestamp;
	u64 cputime_speedadj;
	u64 cputime_speedadj_timestamp;
	unsigned int loadadjfreq;
};

static DEFINE_PER_CPU(struct cpufreq_interactive_policyinfo *, polinfo);
static DEFINE_PER_CPU(struct cpufreq_interactive_cpuinfo, cpuinfo);

#define TASK_NAME_LEN 15
struct task_struct *speedchange_task;
static cpumask_t speedchange_cpumask;
static spinlock_t speedchange_cpumask_lock;
static struct mutex gov_lock;

static bool suspended = false;

/* Target load.  Lower values result in higher CPU speeds. */
#define DEFAULT_TARGET_LOAD 90
static unsigned int default_target_loads[] = {DEFAULT_TARGET_LOAD};

#define DOWN_LOW_LOAD_THRESHOLD 6

#define DEFAULT_TIMER_RATE (20 * USEC_PER_MSEC)
#define SCREEN_OFF_TIMER_RATE ((unsigned long)(60 * USEC_PER_MSEC))
#define DEFAULT_ABOVE_HISPEED_DELAY DEFAULT_TIMER_RATE
static unsigned int default_above_hispeed_delay[] = {
	DEFAULT_ABOVE_HISPEED_DELAY };

#ifdef CONFIG_MODE_AUTO_CHANGE
extern struct cpumask hmp_fast_cpu_mask;
struct cpufreq_loadinfo {
	unsigned int load;
	unsigned int freq;
	u64 timestamp;
};
static bool hmp_boost;

#define MULTI_MODE  2
#define SINGLE_MODE 1
#define NO_MODE     0
#define DEFAULT_MULTI_ENTER_TIME (4 * DEFAULT_TIMER_RATE)
#define DEFAULT_MULTI_EXIT_TIME (16 * DEFAULT_TIMER_RATE)
#define DEFAULT_SINGLE_ENTER_TIME (8 * DEFAULT_TIMER_RATE)
#define DEFAULT_SINGLE_EXIT_TIME (4 * DEFAULT_TIMER_RATE)
#define DEFAULT_SINGLE_CLUSTER0_MIN_FREQ 0
#define DEFAULT_MULTI_CLUSTER0_MIN_FREQ 0

static DEFINE_PER_CPU(struct cpufreq_loadinfo, loadinfo);
static DEFINE_PER_CPU(unsigned int, cpu_util);

static struct pm_qos_request cluster0_min_freq_qos;
static void mode_auto_change_minlock(struct work_struct *work);
static struct workqueue_struct *mode_auto_change_minlock_wq;
static struct work_struct mode_auto_change_minlock_work;
static unsigned int cluster0_min_freq=0;

#define set_qos(req, pm_qos_class, value) { \
	if (value) { \
		if (pm_qos_request_active(req)) \
			pm_qos_update_request(req, value); \
		else \
			pm_qos_add_request(req, pm_qos_class, value); \
	} \
	else \
		if (pm_qos_request_active(req)) \
			pm_qos_remove_request(req); \
}
#endif /* CONFIG_MODE_AUTO_CHANGE */

#define DEFAULT_SCREEN_OFF_MAX 1555200
static unsigned long screen_off_max = DEFAULT_SCREEN_OFF_MAX;

struct cpufreq_interactive_tunables {
	int usage_count;
	/* Hi speed to bump to from lo speed when load burst (default max) */
	unsigned int hispeed_freq;
	/* Go to hi speed when CPU load at or above this value. */
#define DEFAULT_GO_HISPEED_LOAD 99
	unsigned long go_hispeed_load;
	/* Target load. Lower values result in higher CPU speeds. */
	spinlock_t target_loads_lock;
	unsigned int *target_loads;
	int ntarget_loads;
	/*
	 * The minimum amount of time to spend at a frequency before we can ramp
	 * down.
	 */
#define DEFAULT_MIN_SAMPLE_TIME (80 * USEC_PER_MSEC)
	unsigned long min_sample_time;
	/*
	 * The sample rate of the timer used to increase frequency
	 */
	unsigned long timer_rate;
	unsigned long prev_timer_rate;
	/*
	 * Wait this long before raising speed above hispeed, by default a
	 * single timer interval.
	 */
	spinlock_t above_hispeed_delay_lock;
	unsigned int *above_hispeed_delay;
	int nabove_hispeed_delay;
	/* Non-zero means indefinite speed boost active */
	int boost_val;
	/* Duration of a boot pulse in usecs */
	int boostpulse_duration_val;
	/* End time of boost pulse in ktime converted to usecs */
	u64 boostpulse_endtime;
	bool boosted;
	/*
	 * Max additional time to wait in idle, beyond timer_rate, at speeds
	 * above minimum before wakeup to reduce speed, or -1 if unnecessary.
	 */
#define DEFAULT_TIMER_SLACK (4 * DEFAULT_TIMER_RATE)
	int timer_slack_val;
	bool io_is_busy;


<<<<<<< HEAD
	/* handle for get cpufreq_policy */
	unsigned int *policy;
#ifdef CONFIG_MODE_AUTO_CHANGE
	spinlock_t mode_lock;
	unsigned int mode;
	unsigned int enforced_mode;
	u64 mode_check_timestamp;

	unsigned long multi_enter_time;
	unsigned long time_in_multi_enter;
	unsigned int multi_enter_load;

	unsigned long multi_exit_time;
	unsigned long time_in_multi_exit;
	unsigned int multi_exit_load;

	unsigned long single_enter_time;
	unsigned long time_in_single_enter;
	unsigned int single_enter_load;

	unsigned long single_exit_time;
	unsigned long time_in_single_exit;
	unsigned int single_exit_load;

	spinlock_t param_index_lock;
	unsigned int param_index;
	unsigned int cur_param_index;

	unsigned int single_cluster0_min_freq;
	unsigned int multi_cluster0_min_freq;

#define MAX_PARAM_SET 4 /* ((MULTI_MODE | SINGLE_MODE | NO_MODE) + 1) */
	unsigned int hispeed_freq_set[MAX_PARAM_SET];
	unsigned long go_hispeed_load_set[MAX_PARAM_SET];
	unsigned int *target_loads_set[MAX_PARAM_SET];
	int ntarget_loads_set[MAX_PARAM_SET];
	unsigned long min_sample_time_set[MAX_PARAM_SET];
	unsigned long timer_rate_set[MAX_PARAM_SET];
	unsigned int *above_hispeed_delay_set[MAX_PARAM_SET];
	int nabove_hispeed_delay_set[MAX_PARAM_SET];
	unsigned int sampling_down_factor_set[MAX_PARAM_SET];

	unsigned int sampling_down_factor;
#endif
=======
	/*
	 * Whether to align timer windows across all CPUs. When
	 * use_sched_load is true, this flag is ignored and windows
	 * will always be aligned.
	 */
	bool align_windows;

	/*
	 * Stay at max freq for at least max_freq_hysteresis before dropping
	 * frequency.
	 */
	unsigned int max_freq_hysteresis;

	/* Ignore hispeed_freq and above_hispeed_delay for notification */
	bool ignore_hispeed_on_notif;

	/* Ignore min_sample_time for notification */
	bool fast_ramp_down;
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
};

/* For cases where we have single governor instance for system */
static struct cpufreq_interactive_tunables *common_tunables;
<<<<<<< HEAD
static struct cpufreq_interactive_tunables *tuned_parameters[NR_CPUS] = {NULL, };

static struct attribute_group *get_sysfs_attr(void);

static void cpufreq_interactive_timer_resched(
	struct cpufreq_interactive_cpuinfo *pcpu)
{
	struct cpufreq_interactive_tunables *tunables =
		pcpu->policy->governor_data;
	unsigned long expires;
=======
static struct cpufreq_interactive_tunables *cached_common_tunables;

static struct attribute_group *get_sysfs_attr(void);

/* Round to starting jiffy of next evaluation window */
static u64 round_to_nw_start(u64 jif,
			     struct cpufreq_interactive_tunables *tunables)
{
	unsigned long step = usecs_to_jiffies(tunables->timer_rate);
	u64 ret;

	if (tunables->use_sched_load || tunables->align_windows) {
		do_div(jif, step);
		ret = (jif + 1) * step;
	} else {
		ret = jiffies + usecs_to_jiffies(tunables->timer_rate);
	}

	return ret;
}

static inline int set_window_helper(
			struct cpufreq_interactive_tunables *tunables)
{
	return sched_set_window(round_to_nw_start(get_jiffies_64(), tunables),
			 usecs_to_jiffies(tunables->timer_rate));
}

static void cpufreq_interactive_timer_resched(unsigned long cpu,
					      bool slack_only)
{
	struct cpufreq_interactive_policyinfo *ppol = per_cpu(polinfo, cpu);
	struct cpufreq_interactive_cpuinfo *pcpu;
	struct cpufreq_interactive_tunables *tunables =
		ppol->policy->governor_data;
	u64 expires;
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
	unsigned long flags;
	int i;

<<<<<<< HEAD
	if (!speedchange_task)
		return;

	spin_lock_irqsave(&pcpu->load_lock, flags);
	pcpu->time_in_idle =
		get_cpu_idle_time(smp_processor_id(),
				  &pcpu->time_in_idle_timestamp,
				  tunables->io_is_busy);
	pcpu->cputime_speedadj = 0;
	pcpu->cputime_speedadj_timestamp = pcpu->time_in_idle_timestamp;
	expires = jiffies + usecs_to_jiffies(tunables->timer_rate);
	mod_timer_pinned(&pcpu->cpu_timer, expires);

	if (!suspended && tunables->timer_slack_val >= 0 &&
	    pcpu->target_freq > pcpu->policy->min) {
		expires += usecs_to_jiffies(tunables->timer_slack_val);
		mod_timer_pinned(&pcpu->cpu_slack_timer, expires);
=======
	spin_lock_irqsave(&ppol->load_lock, flags);
	expires = round_to_nw_start(ppol->last_evaluated_jiffy, tunables);
	if (!slack_only) {
		for_each_cpu(i, ppol->policy->cpus) {
			pcpu = &per_cpu(cpuinfo, i);
			pcpu->time_in_idle = get_cpu_idle_time(i,
						&pcpu->time_in_idle_timestamp,
						tunables->io_is_busy);
			pcpu->cputime_speedadj = 0;
			pcpu->cputime_speedadj_timestamp =
						pcpu->time_in_idle_timestamp;
		}
		del_timer(&ppol->policy_timer);
		ppol->policy_timer.expires = expires;
		add_timer(&ppol->policy_timer);
	}

	if (tunables->timer_slack_val >= 0 &&
	    ppol->target_freq > ppol->policy->min) {
		expires += usecs_to_jiffies(tunables->timer_slack_val);
		del_timer(&ppol->policy_slack_timer);
		ppol->policy_slack_timer.expires = expires;
		add_timer(&ppol->policy_slack_timer);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
	}

	spin_unlock_irqrestore(&ppol->load_lock, flags);
}

/* The caller shall take enable_sem write semaphore to avoid any timer race.
 * The policy_timer and policy_slack_timer must be deactivated when calling
 * this function.
 */
static void cpufreq_interactive_timer_start(
	struct cpufreq_interactive_tunables *tunables, int cpu)
{
<<<<<<< HEAD
	struct cpufreq_interactive_cpuinfo *pcpu = &per_cpu(cpuinfo, cpu);
	unsigned long expires = jiffies +
		usecs_to_jiffies(tunables->timer_rate);
=======
	struct cpufreq_interactive_policyinfo *ppol = per_cpu(polinfo, cpu);
	struct cpufreq_interactive_cpuinfo *pcpu;
	u64 expires = round_to_nw_start(ppol->last_evaluated_jiffy, tunables);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
	unsigned long flags;
	int i;

<<<<<<< HEAD
	if (!speedchange_task)
		return;

	pcpu->cpu_timer.expires = expires;
	add_timer_on(&pcpu->cpu_timer, cpu);
	if (!suspended && tunables->timer_slack_val >= 0 &&
	    pcpu->target_freq > pcpu->policy->min) {
=======
	spin_lock_irqsave(&ppol->load_lock, flags);
	ppol->policy_timer.expires = expires;
	add_timer(&ppol->policy_timer);
	if (tunables->timer_slack_val >= 0 &&
	    ppol->target_freq > ppol->policy->min) {
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
		expires += usecs_to_jiffies(tunables->timer_slack_val);
		ppol->policy_slack_timer.expires = expires;
		add_timer(&ppol->policy_slack_timer);
	}

<<<<<<< HEAD
	spin_lock_irqsave(&pcpu->load_lock, flags);
	pcpu->time_in_idle =
		get_cpu_idle_time(cpu, &pcpu->time_in_idle_timestamp,
				  tunables->io_is_busy);
	pcpu->cputime_speedadj = 0;
	pcpu->cputime_speedadj_timestamp = pcpu->time_in_idle_timestamp;
	spin_unlock_irqrestore(&pcpu->load_lock, flags);
=======
	for_each_cpu(i, ppol->policy->cpus) {
		pcpu = &per_cpu(cpuinfo, i);
		pcpu->time_in_idle =
			get_cpu_idle_time(i, &pcpu->time_in_idle_timestamp,
					  tunables->io_is_busy);
		pcpu->cputime_speedadj = 0;
		pcpu->cputime_speedadj_timestamp = pcpu->time_in_idle_timestamp;
	}
	spin_unlock_irqrestore(&ppol->load_lock, flags);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
}

static unsigned int freq_to_above_hispeed_delay(
	struct cpufreq_interactive_tunables *tunables,
	unsigned int freq)
{
	int i;
	unsigned int ret;
	unsigned long flags;

	spin_lock_irqsave(&tunables->above_hispeed_delay_lock, flags);

	for (i = 0; i < tunables->nabove_hispeed_delay - 1 &&
			freq >= tunables->above_hispeed_delay[i+1]; i += 2)
		;

	ret = tunables->above_hispeed_delay[i];
	spin_unlock_irqrestore(&tunables->above_hispeed_delay_lock, flags);
	return ret;
}

static unsigned int freq_to_targetload(
	struct cpufreq_interactive_tunables *tunables, unsigned int freq)
{
	int i;
	unsigned int ret;
	unsigned long flags;

	spin_lock_irqsave(&tunables->target_loads_lock, flags);

	for (i = 0; i < tunables->ntarget_loads - 1 &&
		    freq >= tunables->target_loads[i+1]; i += 2)
		;

	ret = tunables->target_loads[i];
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);
	return ret;
}

/*
 * If increasing frequencies never map to a lower target load then
 * choose_freq() will find the minimum frequency that does not exceed its
 * target load given the current load.
 */
static unsigned int choose_freq(struct cpufreq_interactive_policyinfo *pcpu,
		unsigned int loadadjfreq)
{
	unsigned int freq = pcpu->policy->cur;
	unsigned int prevfreq, freqmin, freqmax;
	unsigned int tl;
	int index;

	freqmin = 0;
	freqmax = UINT_MAX;

	do {
		prevfreq = freq;
		tl = freq_to_targetload(pcpu->policy->governor_data, freq);

		/*
		 * Find the lowest frequency where the computed load is less
		 * than or equal to the target load.
		 */

		if (cpufreq_frequency_table_target(
			    pcpu->policy, pcpu->freq_table, loadadjfreq / tl,
			    CPUFREQ_RELATION_L, &index))
			break;
		freq = pcpu->freq_table[index].frequency;

		if (freq > prevfreq) {
			/* The previous frequency is too low. */
			freqmin = prevfreq;

			if (freq >= freqmax) {
				/*
				 * Find the highest frequency that is less
				 * than freqmax.
				 */
				if (cpufreq_frequency_table_target(
					    pcpu->policy, pcpu->freq_table,
					    freqmax - 1, CPUFREQ_RELATION_H,
					    &index))
					break;
				freq = pcpu->freq_table[index].frequency;

				if (freq == freqmin) {
					/*
					 * The first frequency below freqmax
					 * has already been found to be too
					 * low.  freqmax is the lowest speed
					 * we found that is fast enough.
					 */
					freq = freqmax;
					break;
				}
			}
		} else if (freq < prevfreq) {
			/* The previous frequency is high enough. */
			freqmax = prevfreq;

			if (freq <= freqmin) {
				/*
				 * Find the lowest frequency that is higher
				 * than freqmin.
				 */
				if (cpufreq_frequency_table_target(
					    pcpu->policy, pcpu->freq_table,
					    freqmin + 1, CPUFREQ_RELATION_L,
					    &index))
					break;
				freq = pcpu->freq_table[index].frequency;

				/*
				 * If freqmax is the first frequency above
				 * freqmin then we have already found that
				 * this speed is fast enough.
				 */
				if (freq == freqmax)
					break;
			}
		}

		/* If same frequency chosen as previous then done. */
	} while (freq != prevfreq);

	return freq;
}

static u64 update_load(int cpu)
{
	struct cpufreq_interactive_policyinfo *ppol = per_cpu(polinfo, cpu);
	struct cpufreq_interactive_cpuinfo *pcpu = &per_cpu(cpuinfo, cpu);
	struct cpufreq_interactive_tunables *tunables =
		ppol->policy->governor_data;
	u64 now;
	u64 now_idle;
	u64 delta_idle;
	u64 delta_time;
	u64 active_time;
#ifdef CONFIG_MODE_AUTO_CHANGE
	unsigned int cur_load = 0;
	struct cpufreq_loadinfo *cur_loadinfo = &per_cpu(loadinfo, cpu);
#endif
	now_idle = get_cpu_idle_time(cpu, &now, tunables->io_is_busy);
	delta_idle = (now_idle - pcpu->time_in_idle);
	delta_time = (now - pcpu->time_in_idle_timestamp);

	if (delta_time <= delta_idle)
		active_time = 0;
	else
		active_time = delta_time - delta_idle;

	pcpu->cputime_speedadj += active_time * ppol->policy->cur;

	update_cpu_metric(cpu, now, delta_idle, delta_time, pcpu->policy);

	pcpu->time_in_idle = now_idle;
	pcpu->time_in_idle_timestamp = now;
#ifdef CONFIG_MODE_AUTO_CHANGE
	cur_load = (unsigned int)(active_time * 100) / delta_time;
	per_cpu(cpu_util, cpu) = cur_load;

	cur_loadinfo->load = (cur_load * pcpu->policy->cur) /
                    pcpu->policy->cpuinfo.max_freq;
	cur_loadinfo->freq = pcpu->policy->cur;
	cur_loadinfo->timestamp = now;
#endif
	return now;
}

<<<<<<< HEAD
#ifdef CONFIG_MODE_AUTO_CHANGE
static unsigned int check_mode(int cpu, unsigned int cur_mode, u64 now)
{
	int i;
	unsigned int ret=cur_mode, total_load=0, max_single_load=0;
	struct cpufreq_loadinfo *cur_loadinfo;
	struct cpufreq_interactive_cpuinfo *pcpu = &per_cpu(cpuinfo, cpu);
	struct cpufreq_interactive_tunables *tunables =
		pcpu->policy->governor_data;

	if (now - tunables->mode_check_timestamp < tunables->timer_rate - USEC_PER_MSEC)
		return ret;

	if (now - tunables->mode_check_timestamp > tunables->timer_rate + USEC_PER_MSEC)
		tunables->mode_check_timestamp = now - tunables->timer_rate;

	if(cpumask_test_cpu(cpu, &hmp_fast_cpu_mask)) {
		for_each_cpu_mask(i, hmp_fast_cpu_mask) {
			cur_loadinfo = &per_cpu(loadinfo, i);
			if (now - cur_loadinfo->timestamp <= tunables->timer_rate + USEC_PER_MSEC) {
				total_load += cur_loadinfo->load;
				if (cur_loadinfo->load > max_single_load)
					max_single_load = cur_loadinfo->load;
			}
		}
	}
	else
		return ret;

	if (!(cur_mode & SINGLE_MODE)) {
		if (max_single_load >= tunables->single_enter_load)
			tunables->time_in_single_enter += now - tunables->mode_check_timestamp;
		else
			tunables->time_in_single_enter = 0;

		if (tunables->time_in_single_enter >= tunables->single_enter_time)
			ret |= SINGLE_MODE;
	}

	if (!(cur_mode & MULTI_MODE)) {
		if (total_load >= tunables->multi_enter_load)
			tunables->time_in_multi_enter += now - tunables->mode_check_timestamp;
		else
			tunables->time_in_multi_enter = 0;

		if (tunables->time_in_multi_enter >= tunables->multi_enter_time)
			ret |= MULTI_MODE;
	}

	if (cur_mode & SINGLE_MODE) {
		if (max_single_load < tunables->single_exit_load)
			tunables->time_in_single_exit += now - tunables->mode_check_timestamp;
		else
			tunables->time_in_single_exit = 0;

		if (tunables->time_in_single_exit >= tunables->single_exit_time)
			ret &= ~SINGLE_MODE;
	}

	if (cur_mode & MULTI_MODE) {
		if (total_load < tunables->multi_exit_load)
			tunables->time_in_multi_exit += now - tunables->mode_check_timestamp;
		else
			tunables->time_in_multi_exit = 0;

		if (tunables->time_in_multi_exit >= tunables->multi_exit_time)
			ret &= ~MULTI_MODE;
	}

	trace_cpufreq_interactive_mode(cpu, total_load,
		tunables->time_in_single_enter, tunables->time_in_multi_enter,
		tunables->time_in_single_exit, tunables->time_in_multi_exit, ret);

	if (tunables->time_in_single_enter >= tunables->single_enter_time)
		tunables->time_in_single_enter = 0;
	if (tunables->time_in_multi_enter >= tunables->multi_enter_time)
		tunables->time_in_multi_enter = 0;
	if (tunables->time_in_single_exit >= tunables->single_exit_time)
		tunables->time_in_single_exit = 0;
	if (tunables->time_in_multi_exit >= tunables->multi_exit_time)
		tunables->time_in_multi_exit = 0;
	tunables->mode_check_timestamp = now;

	return ret;
}

static void set_new_param_set(unsigned int index,
			struct cpufreq_interactive_tunables * tunables)
{
	unsigned long flags;

	tunables->hispeed_freq = tunables->hispeed_freq_set[index];
	tunables->go_hispeed_load = tunables->go_hispeed_load_set[index];
	tunables->min_sample_time = tunables->min_sample_time_set[index];
	tunables->timer_rate = tunables->timer_rate_set[index];

	spin_lock_irqsave(&tunables->target_loads_lock, flags);
	tunables->target_loads = tunables->target_loads_set[index];
	tunables->ntarget_loads = tunables->ntarget_loads_set[index];
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);

	spin_lock_irqsave(&tunables->above_hispeed_delay_lock, flags);
	tunables->above_hispeed_delay =
		tunables->above_hispeed_delay_set[index];
	tunables->nabove_hispeed_delay =
		tunables->nabove_hispeed_delay_set[index];
	spin_unlock_irqrestore(&tunables->above_hispeed_delay_lock, flags);

	tunables->cur_param_index = index;
}

static void enter_mode(struct cpufreq_interactive_tunables * tunables)
{
	set_new_param_set(tunables->mode, tunables);
	if(tunables->mode & SINGLE_MODE)
		cluster0_min_freq = tunables->single_cluster0_min_freq;
	if(tunables->mode & MULTI_MODE)
		cluster0_min_freq = tunables->multi_cluster0_min_freq;
	if(!hmp_boost) {
		pr_debug("%s mp boost on", __func__);
		(void)set_hmp_boost(1);
		hmp_boost = true;
	}

	queue_work(mode_auto_change_minlock_wq, &mode_auto_change_minlock_work);
}

static void exit_mode(struct cpufreq_interactive_tunables * tunables)
{
	set_new_param_set(0, tunables);
	cluster0_min_freq = 0;

	if(hmp_boost) {
		pr_debug("%s mp boost off", __func__);
		(void)set_hmp_boost(0);
		hmp_boost = false;
	}

	queue_work(mode_auto_change_minlock_wq, &mode_auto_change_minlock_work);
}
#endif

static void cpufreq_interactive_timer(unsigned long data)
=======
static void __cpufreq_interactive_timer(unsigned long data, bool is_notif)
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
{
	u64 now;
	unsigned int delta_time;
	u64 cputime_speedadj;
	int cpu_load;
	struct cpufreq_interactive_policyinfo *ppol = per_cpu(polinfo, data);
	struct cpufreq_interactive_tunables *tunables =
		ppol->policy->governor_data;
	struct cpufreq_interactive_cpuinfo *pcpu;
	unsigned int new_freq;
	unsigned int loadadjfreq = 0, tmploadadjfreq;
	unsigned int index;
	unsigned long flags;
<<<<<<< HEAD
	u64 max_fvtime;
#ifdef CONFIG_MODE_AUTO_CHANGE
	unsigned int new_mode;
#endif
	if (!down_read_trylock(&pcpu->enable_sem))
		return;
	if (!pcpu->governor_enabled)
		goto exit;		
	if (pcpu->policy->min == pcpu->policy->max)
		goto rearm;

	spin_lock_irqsave(&pcpu->load_lock, flags);
	now = update_load(data);
	delta_time = (unsigned int)(now - pcpu->cputime_speedadj_timestamp);
	cputime_speedadj = pcpu->cputime_speedadj;
	spin_unlock_irqrestore(&pcpu->load_lock, flags);

	if (WARN_ON_ONCE(!delta_time))
		goto rearm;
#ifdef CONFIG_MODE_AUTO_CHANGE
	spin_lock_irqsave(&tunables->mode_lock, flags);
	if (tunables->enforced_mode)
		new_mode = tunables->enforced_mode;
	else
		new_mode = check_mode(data, tunables->mode, now);

	if (new_mode != tunables->mode) {
		tunables->mode = new_mode;
		if (new_mode & MULTI_MODE || new_mode & SINGLE_MODE)
			enter_mode(tunables);
		else
			exit_mode(tunables);
	}
	spin_unlock_irqrestore(&tunables->mode_lock, flags);
#endif
	spin_lock_irqsave(&pcpu->target_freq_lock, flags);
	do_div(cputime_speedadj, delta_time);
	loadadjfreq = (unsigned int)cputime_speedadj * 100;
	
/*
	 * The current cpu freq can be a little inaccurate for calculating cpu
	 * load since we could have changed frqeuency since the last time we set
	 * it due to an updated cpufreq policy. This shouldn't happen very often
	 * though.
	 */
	 
	cpu_load = loadadjfreq / pcpu->policy->cur;
	tunables->boosted = tunables->boost_val || now < tunables->boostpulse_endtime;


	if (cpu_load >= tunables->go_hispeed_load&& !suspended || tunables->boosted) {
		if (pcpu->policy->cur < tunables->hispeed_freq) {
			new_freq = tunables->hispeed_freq;
=======
	unsigned long max_cpu;
	int i;
	struct cpufreq_govinfo govinfo;
	bool skip_hispeed_logic, skip_min_sample_time;
	bool policy_max_fast_restore = false;
	bool display_on = is_display_on();
	unsigned int this_hispeed_freq;

	if (!down_read_trylock(&ppol->enable_sem))
		return;
	if (!ppol->governor_enabled)
		goto exit;

	now = ktime_to_us(ktime_get());
	spin_lock_irqsave(&ppol->load_lock, flags);
	ppol->last_evaluated_jiffy = get_jiffies_64();

	if (display_on
		&& tunables->timer_rate != tunables->prev_timer_rate)
		tunables->timer_rate = tunables->prev_timer_rate;
	else if (!display_on
		&& tunables->timer_rate != SCREEN_OFF_TIMER_RATE) {
		tunables->prev_timer_rate = tunables->timer_rate;
		tunables->timer_rate
			= max(tunables->timer_rate,
				SCREEN_OFF_TIMER_RATE);
	}

	max_cpu = cpumask_first(ppol->policy->cpus);
	for_each_cpu(i, ppol->policy->cpus) {
		pcpu = &per_cpu(cpuinfo, i);
		if (tunables->use_sched_load) {
			cputime_speedadj = (u64)sched_get_busy(i) *
					ppol->policy->cpuinfo.max_freq;
			do_div(cputime_speedadj, tunables->timer_rate);
		} else {
			now = update_load(i);
			delta_time = (unsigned int)
				(now - pcpu->cputime_speedadj_timestamp);
			if (WARN_ON_ONCE(!delta_time))
				continue;
			cputime_speedadj = pcpu->cputime_speedadj;
			do_div(cputime_speedadj, delta_time);
		}
		tmploadadjfreq = (unsigned int)cputime_speedadj * 100;
		pcpu->loadadjfreq = tmploadadjfreq;
		trace_cpufreq_interactive_cpuload(i, tmploadadjfreq /
						  ppol->target_freq);

		if (tmploadadjfreq > loadadjfreq) {
			loadadjfreq = tmploadadjfreq;
			max_cpu = i;
		}
	}
	spin_unlock_irqrestore(&ppol->load_lock, flags);

	/*
	 * Send govinfo notification.
	 * Govinfo notification could potentially wake up another thread
	 * managed by its clients. Thread wakeups might trigger a load
	 * change callback that executes this function again. Therefore
	 * no spinlock could be held when sending the notification.
	 */
	for_each_cpu(i, ppol->policy->cpus) {
		pcpu = &per_cpu(cpuinfo, i);
		govinfo.cpu = i;
		govinfo.load = pcpu->loadadjfreq / ppol->policy->max;
		govinfo.sampling_rate_us = tunables->timer_rate;
		atomic_notifier_call_chain(&cpufreq_govinfo_notifier_list,
					   CPUFREQ_LOAD_CHANGE, &govinfo);
	}

	spin_lock_irqsave(&ppol->target_freq_lock, flags);
	cpu_load = loadadjfreq / ppol->target_freq;
	tunables->boosted = tunables->boost_val || now < tunables->boostpulse_endtime;
	this_hispeed_freq = max(tunables->hispeed_freq, ppol->policy->min);
	this_hispeed_freq = min(this_hispeed_freq, ppol->policy->max);

	skip_hispeed_logic = tunables->ignore_hispeed_on_notif && is_notif;
	skip_min_sample_time = tunables->fast_ramp_down && is_notif;
	if (now - ppol->max_freq_hyst_start_time <
	    tunables->max_freq_hysteresis &&
	    cpu_load >= tunables->go_hispeed_load &&
	    ppol->target_freq < ppol->policy->max) {
		skip_hispeed_logic = true;
		skip_min_sample_time = true;
		policy_max_fast_restore = true;
	}

	if (policy_max_fast_restore) {
		new_freq = ppol->policy->max;
	} else if (skip_hispeed_logic) {
		new_freq = choose_freq(ppol, loadadjfreq);
	} else if (cpu_load >= tunables->go_hispeed_load || tunables->boosted) {
		if (ppol->target_freq < this_hispeed_freq) {
			new_freq = this_hispeed_freq;
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
		} else {
			new_freq = choose_freq(ppol, loadadjfreq);

			if (new_freq < this_hispeed_freq)
				new_freq = this_hispeed_freq;
		}
	} else if (cpu_load <= DOWN_LOW_LOAD_THRESHOLD) {
		new_freq = pcpu->policy->cpuinfo.min_freq;
	} else {
<<<<<<< HEAD
		new_freq = choose_freq(pcpu, loadadjfreq);
		if (new_freq > tunables->hispeed_freq &&
				pcpu->policy->cur < tunables->hispeed_freq)
			new_freq = tunables->hispeed_freq;
	}

	if (cpufreq_frequency_table_target(pcpu->policy, pcpu->freq_table,
=======
		new_freq = choose_freq(ppol, loadadjfreq);
		if (new_freq > this_hispeed_freq &&
				ppol->policy->cur < this_hispeed_freq)
			new_freq = this_hispeed_freq;
	}

	if (now - ppol->max_freq_hyst_start_time <
	    tunables->max_freq_hysteresis)
		new_freq = max(this_hispeed_freq, new_freq);

	if (!skip_hispeed_logic &&
	    ppol->target_freq >= this_hispeed_freq &&
	    new_freq > ppol->target_freq &&
	    now - ppol->hispeed_validate_time <
	    freq_to_above_hispeed_delay(tunables, ppol->target_freq)) {
		trace_cpufreq_interactive_notyet(
			max_cpu, cpu_load, ppol->target_freq,
			ppol->policy->cur, new_freq);
		spin_unlock_irqrestore(&ppol->target_freq_lock, flags);
		goto rearm;
	}

	ppol->hispeed_validate_time = now;

	if (cpufreq_frequency_table_target(ppol->policy, ppol->freq_table,
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
					   new_freq, CPUFREQ_RELATION_L,
					   &index)) {
		spin_unlock_irqrestore(&ppol->target_freq_lock, flags);
		goto rearm;
	}

<<<<<<< HEAD
	new_freq = pcpu->freq_table[index].frequency;

	if (pcpu->policy->cur >= tunables->hispeed_freq &&
	    new_freq > pcpu->policy->cur &&
	    now - pcpu->pol_hispeed_val_time <
	    freq_to_above_hispeed_delay(tunables, pcpu->policy->cur)) {
		trace_cpufreq_interactive_notyet(
			data, cpu_load, pcpu->target_freq,
			pcpu->policy->cur, new_freq);
		spin_unlock_irqrestore(&pcpu->target_freq_lock, flags);
		goto target_update;
	}
=======
	new_freq = ppol->freq_table[index].frequency;
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor

	pcpu->loc_hispeed_val_time = now;

	/*
	 * Do not scale below floor_freq unless we have been at or above the
	 * floor frequency for the minimum sample time since last validated.
	 */
<<<<<<< HEAD
	max_fvtime = max(pcpu->pol_floor_val_time, pcpu->loc_floor_val_time);
	if (new_freq < pcpu->floor_freq &&
	    pcpu->target_freq >= pcpu->policy->cur) {
		if (now - max_fvtime < tunables->min_sample_time) {
=======
	if (!skip_min_sample_time && new_freq < ppol->floor_freq) {
		if (now - ppol->floor_validate_time <
				tunables->min_sample_time) {
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
			trace_cpufreq_interactive_notyet(
				max_cpu, cpu_load, ppol->target_freq,
				ppol->policy->cur, new_freq);
			spin_unlock_irqrestore(&ppol->target_freq_lock, flags);
			goto rearm;
		}
	}

	/*
	 * Update the timestamp for checking whether speed has been held at
	 * or above the selected frequency for a minimum of min_sample_time,
	 * if not boosted to hispeed_freq.  If boosted to hispeed_freq then we
	 * allow the speed to drop as soon as the boostpulse duration expires
	 * (or the indefinite boost is turned off). If policy->max is restored
	 * for max_freq_hysteresis, don't extend the timestamp. Otherwise, it
	 * could incorrectly extended the duration of max_freq_hysteresis by
	 * min_sample_time.
	 */

<<<<<<< HEAD
	if (!tunables->boosted || new_freq > tunables->hispeed_freq) {
		pcpu->floor_freq = new_freq;
		if (pcpu->target_freq >= pcpu->policy->cur ||
		    new_freq >= pcpu->policy->cur)
			pcpu->loc_floor_val_time = now;
	}

	if (pcpu->target_freq == new_freq &&
			pcpu->target_freq <= pcpu->policy->cur) {
		trace_cpufreq_interactive_already(
			data, cpu_load, pcpu->target_freq,
			pcpu->policy->cur, new_freq);
		spin_unlock_irqrestore(&pcpu->target_freq_lock, flags);
=======
	if ((!tunables->boosted || new_freq > this_hispeed_freq)
	    && !policy_max_fast_restore) {
		ppol->floor_freq = new_freq;
		ppol->floor_validate_time = now;
	}

	if (new_freq == ppol->policy->max && !policy_max_fast_restore)
		ppol->max_freq_hyst_start_time = now;

	if (ppol->target_freq == new_freq &&
			ppol->target_freq <= ppol->policy->cur) {
		trace_cpufreq_interactive_already(
			max_cpu, cpu_load, ppol->target_freq,
			ppol->policy->cur, new_freq);
		spin_unlock_irqrestore(&ppol->target_freq_lock, flags);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
		goto rearm;
	}

	trace_cpufreq_interactive_target(max_cpu, cpu_load, ppol->target_freq,
					 ppol->policy->cur, new_freq);

	ppol->target_freq = new_freq;
	spin_unlock_irqrestore(&ppol->target_freq_lock, flags);
	spin_lock_irqsave(&speedchange_cpumask_lock, flags);
	cpumask_set_cpu(max_cpu, &speedchange_cpumask);
	spin_unlock_irqrestore(&speedchange_cpumask_lock, flags);
	wake_up_process(speedchange_task);
<<<<<<< HEAD

	goto rearm;

target_update:
	pcpu->target_freq = pcpu->policy->cur;

#ifdef CONFIG_MODE_AUTO_CHANGE
		goto rearm;
#else
		goto exit;
#endif

rearm:
	if (!timer_pending(&pcpu->cpu_timer))
		cpufreq_interactive_timer_resched(pcpu);
=======

rearm:
	if (!timer_pending(&ppol->policy_timer))
		cpufreq_interactive_timer_resched(data, false);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor

exit:
	up_read(&ppol->enable_sem);
	return;
}

<<<<<<< HEAD
static void cpufreq_interactive_idle_end(void)
{
	struct cpufreq_interactive_cpuinfo *pcpu =
		&per_cpu(cpuinfo, smp_processor_id());

	if (!down_read_trylock(&pcpu->enable_sem))
		return;
	if (!pcpu->governor_enabled) {
		up_read(&pcpu->enable_sem);
		return;
	}

	/* Arm the timer for 1-2 ticks later if not already. */
	if (!timer_pending(&pcpu->cpu_timer)) {
		cpufreq_interactive_timer_resched(pcpu);
	} else if (time_after_eq(jiffies, pcpu->cpu_timer.expires)) {
		del_timer(&pcpu->cpu_timer);
		del_timer(&pcpu->cpu_slack_timer);
		cpufreq_interactive_timer(smp_processor_id());
	}

	up_read(&pcpu->enable_sem);
=======
static void cpufreq_interactive_timer(unsigned long data)
{
	__cpufreq_interactive_timer(data, false);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
}

static int cpufreq_interactive_speedchange_task(void *data)
{
	unsigned int cpu;
	cpumask_t tmp_mask;
	unsigned long flags;
	struct cpufreq_interactive_policyinfo *ppol;

	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);
		spin_lock_irqsave(&speedchange_cpumask_lock, flags);

		if (cpumask_empty(&speedchange_cpumask)) {
			spin_unlock_irqrestore(&speedchange_cpumask_lock,
					       flags);
			schedule();

			if (kthread_should_stop())
				break;

			spin_lock_irqsave(&speedchange_cpumask_lock, flags);
		}

		set_current_state(TASK_RUNNING);
		tmp_mask = speedchange_cpumask;
		cpumask_clear(&speedchange_cpumask);

		spin_unlock_irqrestore(&speedchange_cpumask_lock, flags);

		for_each_cpu(cpu, &tmp_mask) {
<<<<<<< HEAD
			unsigned int j;
			unsigned int max_freq = 0;
			struct cpufreq_interactive_cpuinfo *pjcpu;
			u64 hvt = ~0ULL, fvt = 0;

			pcpu = &per_cpu(cpuinfo, cpu);
			if (!down_read_trylock(&pcpu->enable_sem))
=======
			ppol = per_cpu(polinfo, cpu);
			if (!down_read_trylock(&ppol->enable_sem))
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
				continue;
			if (!ppol->governor_enabled) {
				up_read(&ppol->enable_sem);
				continue;
			}

<<<<<<< HEAD
			for_each_cpu(j, pcpu->policy->cpus) {
				pjcpu = &per_cpu(cpuinfo, j);

				fvt = max(fvt, pjcpu->loc_floor_val_time);
				if (pjcpu->target_freq > max_freq) {
					max_freq = pjcpu->target_freq;
					hvt = pjcpu->loc_hispeed_val_time;
				} else if (pjcpu->target_freq == max_freq) {
					hvt = min(hvt, pjcpu->loc_hispeed_val_time);
				}
			}
			for_each_cpu(j, pcpu->policy->cpus) {
				pjcpu = &per_cpu(cpuinfo, j);
				pjcpu->pol_floor_val_time = fvt;
			}

			if (unlikely(!suspended))
					if (max_freq > screen_off_max) max_freq = screen_off_max;

			if (max_freq != pcpu->policy->cur) {
				__cpufreq_driver_target(pcpu->policy,
							max_freq,
							CPUFREQ_RELATION_H);
				for_each_cpu(j, pcpu->policy->cpus) {
					pjcpu = &per_cpu(cpuinfo, j);
					pjcpu->pol_hispeed_val_time = hvt;
				}
			}

#if defined(CONFIG_CPU_THERMAL_IPA)
			ipa_cpufreq_requested(pcpu->policy, max_freq);
#endif
=======
			if (ppol->target_freq != ppol->policy->cur)
				__cpufreq_driver_target(ppol->policy,
							ppol->target_freq,
							CPUFREQ_RELATION_H);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
			trace_cpufreq_interactive_setspeed(cpu,
						     ppol->target_freq,
						     ppol->policy->cur);
			up_read(&ppol->enable_sem);
		}
	}

	return 0;
}

static void cpufreq_interactive_boost(struct cpufreq_interactive_tunables *tunables)
{
	int i;
	int anyboost = 0;
	unsigned long flags[2];
<<<<<<< HEAD
	struct cpufreq_interactive_cpuinfo *pcpu;
	struct cpumask boost_mask;
	struct cpufreq_policy *policy = container_of(tunables->policy,
						struct cpufreq_policy, policy);
=======
	struct cpufreq_interactive_policyinfo *ppol;
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor

	tunables->boosted = true;

	spin_lock_irqsave(&speedchange_cpumask_lock, flags[0]);

<<<<<<< HEAD
	if (have_governor_per_policy())
		cpumask_copy(&boost_mask, policy->cpus);
	else
		cpumask_copy(&boost_mask, cpu_online_mask);

	for_each_cpu(i, &boost_mask) {
		pcpu = &per_cpu(cpuinfo, i);
		if (tunables != pcpu->policy->governor_data)
=======
	for_each_online_cpu(i) {
		ppol = per_cpu(polinfo, i);
		if (!ppol || tunables != ppol->policy->governor_data)
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
			continue;

		spin_lock_irqsave(&ppol->target_freq_lock, flags[1]);
		if (ppol->target_freq < tunables->hispeed_freq) {
			ppol->target_freq = tunables->hispeed_freq;
			cpumask_set_cpu(i, &speedchange_cpumask);
<<<<<<< HEAD
			pcpu->pol_hispeed_val_time =
				ktime_to_us(ktime_get());
			anyboost = 1;
		}
		spin_unlock_irqrestore(&pcpu->target_freq_lock, flags[1]);
=======
			ppol->hispeed_validate_time =
				ktime_to_us(ktime_get());
			anyboost = 1;
		}

		/*
		 * Set floor freq and (re)start timer for when last
		 * validated.
		 */

		ppol->floor_freq = tunables->hispeed_freq;
		ppol->floor_validate_time = ktime_to_us(ktime_get());
		spin_unlock_irqrestore(&ppol->target_freq_lock, flags[1]);
		break;
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
	}

	spin_unlock_irqrestore(&speedchange_cpumask_lock, flags[0]);

<<<<<<< HEAD

	if (anyboost && speedchange_task)
		wake_up_process(speedchange_task);
=======
	if (anyboost)
		wake_up_process(speedchange_task);
}

static int load_change_callback(struct notifier_block *nb, unsigned long val,
				void *data)
{
	unsigned long cpu = (unsigned long) data;
	struct cpufreq_interactive_policyinfo *ppol = per_cpu(polinfo, cpu);
	struct cpufreq_interactive_tunables *tunables;

	if (speedchange_task == current)
		return 0;
	if (!ppol || ppol->reject_notification)
		return 0;

	if (!down_read_trylock(&ppol->enable_sem))
		return 0;
	if (!ppol->governor_enabled) {
		up_read(&ppol->enable_sem);
		return 0;
	}
	tunables = ppol->policy->governor_data;
	if (!tunables->use_sched_load || !tunables->use_migration_notif) {
		up_read(&ppol->enable_sem);
		return 0;
	}

	trace_cpufreq_interactive_load_change(cpu);
	del_timer(&ppol->policy_timer);
	del_timer(&ppol->policy_slack_timer);
	__cpufreq_interactive_timer(cpu, true);

	up_read(&ppol->enable_sem);
	return 0;
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
}

static int cpufreq_interactive_notifier(
	struct notifier_block *nb, unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = data;
	struct cpufreq_interactive_policyinfo *ppol;
	int cpu;
	unsigned long flags;

	if (val == CPUFREQ_PRECHANGE) {
<<<<<<< HEAD
		pcpu = &per_cpu(cpuinfo, freq->cpu);
		if (!down_read_trylock(&pcpu->enable_sem))
=======
		ppol = per_cpu(polinfo, freq->cpu);
		if (!ppol)
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
			return 0;
		if (!down_read_trylock(&ppol->enable_sem))
			return 0;
		if (!ppol->governor_enabled) {
			up_read(&ppol->enable_sem);
			return 0;
		}

		if (cpumask_first(ppol->policy->cpus) != freq->cpu) {
			up_read(&ppol->enable_sem);
			return 0;
		}
		spin_lock_irqsave(&ppol->load_lock, flags);
		for_each_cpu(cpu, ppol->policy->cpus)
			update_load(cpu);
		spin_unlock_irqrestore(&ppol->load_lock, flags);

		up_read(&ppol->enable_sem);
	}
	return 0;
}

static struct notifier_block cpufreq_notifier_block = {
	.notifier_call = cpufreq_interactive_notifier,
};

static unsigned int *get_tokenized_data(const char *buf, int *num_tokens)
{
	const char *cp;
	int i;
	int ntokens = 1;
	unsigned int *tokenized_data;
	int err = -EINVAL;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if (!(ntokens & 0x1))
		goto err;

	tokenized_data = kmalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!tokenized_data) {
		err = -ENOMEM;
		goto err;
	}

	cp = buf;
	i = 0;
	while (i < ntokens) {
		if (sscanf(cp, "%u", &tokenized_data[i++]) != 1)
			goto err_kfree;

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != ntokens)
		goto err_kfree;

	*num_tokens = ntokens;
	return tokenized_data;

err_kfree:
	kfree(tokenized_data);
err:
	return ERR_PTR(err);
}

static ssize_t show_target_loads(
	struct cpufreq_interactive_tunables *tunables,
	char *buf)
{
	int i;
	ssize_t ret = 0;
	unsigned long flags;
#ifdef CONFIG_MODE_AUTO_CHANGE
	unsigned long flags_idx;
#endif

	spin_lock_irqsave(&tunables->target_loads_lock, flags);
#ifdef CONFIG_MODE_AUTO_CHANGE
	spin_lock_irqsave(&tunables->param_index_lock, flags_idx);
	for (i = 0; i < tunables->ntarget_loads_set[tunables->param_index]; i++)
		ret += sprintf(buf + ret, "%u%s",
			tunables->target_loads_set[tunables->param_index][i],
			i & 0x1 ? ":" : " ");
	spin_unlock_irqrestore(&tunables->param_index_lock, flags_idx);
#else
	for (i = 0; i < tunables->ntarget_loads; i++)
		ret += sprintf(buf + ret, "%u%s", tunables->target_loads[i],
			       i & 0x1 ? ":" : " ");
#endif
	sprintf(buf + ret - 1, "\n");
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);
	return ret;
}

static ssize_t store_target_loads(
	struct cpufreq_interactive_tunables *tunables,
	const char *buf, size_t count)
{
	int ntokens;
	unsigned int *new_target_loads = NULL;
	unsigned long flags;
#ifdef CONFIG_MODE_AUTO_CHANGE
	unsigned long flags_idx;
#endif

	new_target_loads = get_tokenized_data(buf, &ntokens);
	if (IS_ERR(new_target_loads))
		return PTR_RET(new_target_loads);

	spin_lock_irqsave(&tunables->target_loads_lock, flags);
#ifdef CONFIG_MODE_AUTO_CHANGE
	spin_lock_irqsave(&tunables->param_index_lock, flags_idx);
	if (tunables->target_loads_set[tunables->param_index] != default_target_loads)
		kfree(tunables->target_loads_set[tunables->param_index]);
	tunables->target_loads_set[tunables->param_index] = new_target_loads;
	tunables->ntarget_loads_set[tunables->param_index] = ntokens;
	if (tunables->cur_param_index == tunables->param_index) {
		tunables->target_loads = new_target_loads;
		tunables->ntarget_loads = ntokens;
	}
	spin_unlock_irqrestore(&tunables->param_index_lock, flags_idx);
#else
	if (tunables->target_loads != default_target_loads)
		kfree(tunables->target_loads);
	tunables->target_loads = new_target_loads;
	tunables->ntarget_loads = ntokens;
#endif
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);

	return count;
}

static ssize_t show_above_hispeed_delay(
	struct cpufreq_interactive_tunables *tunables, char *buf)
{
	int i;
	ssize_t ret = 0;
	unsigned long flags;
#ifdef CONFIG_MODE_AUTO_CHANGE
	unsigned long flags_idx;
#endif

	spin_lock_irqsave(&tunables->above_hispeed_delay_lock, flags);
#ifdef CONFIG_MODE_AUTO_CHANGE
	spin_lock_irqsave(&tunables->param_index_lock, flags_idx);
	for (i = 0; i < tunables->nabove_hispeed_delay_set[tunables->param_index]; i++)
		ret += sprintf(buf + ret, "%u%s",
			tunables->above_hispeed_delay_set[tunables->param_index][i],
			i & 0x1 ? ":" : " ");
	spin_unlock_irqrestore(&tunables->param_index_lock, flags_idx);
#else
	for (i = 0; i < tunables->nabove_hispeed_delay; i++)
		ret += sprintf(buf + ret, "%u%s",
			       tunables->above_hispeed_delay[i],
			       i & 0x1 ? ":" : " ");
#endif
	sprintf(buf + ret - 1, "\n");
	spin_unlock_irqrestore(&tunables->above_hispeed_delay_lock, flags);
	return ret;
}

static ssize_t store_above_hispeed_delay(
	struct cpufreq_interactive_tunables *tunables,
	const char *buf, size_t count)
{
	int ntokens, i;
	unsigned int *new_above_hispeed_delay = NULL;
	unsigned long flags;
#ifdef CONFIG_MODE_AUTO_CHANGE
	unsigned long flags_idx;
#endif

	new_above_hispeed_delay = get_tokenized_data(buf, &ntokens);
	if (IS_ERR(new_above_hispeed_delay))
		return PTR_RET(new_above_hispeed_delay);
		
	/* Make sure frequencies are in ascending order. */
	for (i = 3; i < ntokens; i += 2) {
		if (new_above_hispeed_delay[i] <=
		    new_above_hispeed_delay[i - 2]) {
			kfree(new_above_hispeed_delay);
			return -EINVAL;
		}
	}

	spin_lock_irqsave(&tunables->above_hispeed_delay_lock, flags);
#ifdef CONFIG_MODE_AUTO_CHANGE
	spin_lock_irqsave(&tunables->param_index_lock, flags_idx);
	if (tunables->above_hispeed_delay_set[tunables->param_index] != default_above_hispeed_delay)
		kfree(tunables->above_hispeed_delay_set[tunables->param_index]);
	tunables->above_hispeed_delay_set[tunables->param_index] = new_above_hispeed_delay;
	tunables->nabove_hispeed_delay_set[tunables->param_index] = ntokens;
	if (tunables->cur_param_index == tunables->param_index) {
		tunables->above_hispeed_delay = new_above_hispeed_delay;
		tunables->nabove_hispeed_delay = ntokens;
	}
	spin_unlock_irqrestore(&tunables->param_index_lock, flags_idx);
#else
	if (tunables->above_hispeed_delay != default_above_hispeed_delay)
		kfree(tunables->above_hispeed_delay);
	tunables->above_hispeed_delay = new_above_hispeed_delay;
	tunables->nabove_hispeed_delay = ntokens;
#endif
	spin_unlock_irqrestore(&tunables->above_hispeed_delay_lock, flags);
	return count;

}

static ssize_t show_hispeed_freq(struct cpufreq_interactive_tunables *tunables,
		char *buf)
{
#ifdef CONFIG_MODE_AUTO_CHANGE
	return sprintf(buf, "%u\n", tunables->hispeed_freq_set[tunables->param_index]);
#else
	return sprintf(buf, "%u\n", tunables->hispeed_freq);
#endif
}

static ssize_t store_hispeed_freq(struct cpufreq_interactive_tunables *tunables,
		const char *buf, size_t count)
{
	int ret;
	long unsigned int val;
#ifdef CONFIG_MODE_AUTO_CHANGE
	unsigned long flags_idx;
#endif
	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
#ifdef CONFIG_MODE_AUTO_CHANGE
	spin_lock_irqsave(&tunables->param_index_lock, flags_idx);
	tunables->hispeed_freq_set[tunables->param_index] = val;
	if (tunables->cur_param_index == tunables->param_index)
	tunables->hispeed_freq = val;
	spin_unlock_irqrestore(&tunables->param_index_lock, flags_idx);
#else
	tunables->hispeed_freq = val;
#endif
	return count;
}

<<<<<<< HEAD
=======
#define show_store_one(file_name)					\
static ssize_t show_##file_name(					\
	struct cpufreq_interactive_tunables *tunables, char *buf)	\
{									\
	return snprintf(buf, PAGE_SIZE, "%u\n", tunables->file_name);	\
}									\
static ssize_t store_##file_name(					\
		struct cpufreq_interactive_tunables *tunables,		\
		const char *buf, size_t count)				\
{									\
	int ret;							\
	long unsigned int val;						\
									\
	ret = kstrtoul(buf, 0, &val);				\
	if (ret < 0)							\
		return ret;						\
	tunables->file_name = val;					\
	return count;							\
}
show_store_one(max_freq_hysteresis);
show_store_one(align_windows);
show_store_one(ignore_hispeed_on_notif);
show_store_one(fast_ramp_down);

>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
static ssize_t show_go_hispeed_load(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
#ifdef CONFIG_MODE_AUTO_CHANGE
	return sprintf(buf, "%lu\n", tunables->go_hispeed_load_set[tunables->param_index]);
#else
	return sprintf(buf, "%lu\n", tunables->go_hispeed_load);
#endif
}

static ssize_t store_go_hispeed_load(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	unsigned long val;
#ifdef CONFIG_MODE_AUTO_CHANGE
	unsigned long flags_idx;
#endif
	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
#ifdef CONFIG_MODE_AUTO_CHANGE
	spin_lock_irqsave(&tunables->param_index_lock, flags_idx);
	tunables->go_hispeed_load_set[tunables->param_index] = val;
	if (tunables->cur_param_index == tunables->param_index)
	tunables->go_hispeed_load = val;
	spin_unlock_irqrestore(&tunables->param_index_lock, flags_idx);
#else
	tunables->go_hispeed_load = val;
#endif
	return count;
}

static ssize_t show_min_sample_time(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
#ifdef CONFIG_MODE_AUTO_CHANGE
	return sprintf(buf, "%lu\n", tunables->min_sample_time_set[tunables->param_index]);
#else
	return sprintf(buf, "%lu\n", tunables->min_sample_time);
#endif
}

static ssize_t store_min_sample_time(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	unsigned long val;
#ifdef CONFIG_MODE_AUTO_CHANGE
	unsigned long flags_idx;
#endif
	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
#ifdef CONFIG_MODE_AUTO_CHANGE
	spin_lock_irqsave(&tunables->param_index_lock, flags_idx);
	tunables->min_sample_time_set[tunables->param_index] = val;
	if (tunables->cur_param_index == tunables->param_index)
		tunables->min_sample_time = val;
	spin_unlock_irqrestore(&tunables->param_index_lock, flags_idx);
#else
	tunables->min_sample_time = val;
#endif
	return count;
}

static ssize_t show_timer_rate(struct cpufreq_interactive_tunables *tunables,
		char *buf)
{
#ifdef CONFIG_MODE_AUTO_CHANGE
	return sprintf(buf, "%lu\n", tunables->timer_rate_set[tunables->param_index]);
#else
	return sprintf(buf, "%lu\n", tunables->timer_rate);
#endif
}

static ssize_t store_timer_rate(struct cpufreq_interactive_tunables *tunables,
		const char *buf, size_t count)
{
	int ret;
        unsigned long val, val_round;
#ifdef CONFIG_MODE_AUTO_CHANGE
	unsigned long flags_idx;
#endif
	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	val_round = jiffies_to_usecs(usecs_to_jiffies(val));
	if (val != val_round)
		pr_warn("timer_rate not aligned to jiffy. Rounded up to %lu\n",
			val_round);
<<<<<<< HEAD

#ifdef CONFIG_MODE_AUTO_CHANGE
	spin_lock_irqsave(&tunables->param_index_lock, flags_idx);
	tunables->timer_rate_set[tunables->param_index] = val;
	if (tunables->cur_param_index == tunables->param_index)
		tunables->timer_rate = val;
	spin_unlock_irqrestore(&tunables->param_index_lock, flags_idx);
#else
	tunables->timer_rate = val_round;
#endif
=======
	tunables->timer_rate = val_round;
	tunables->prev_timer_rate = val_round;

	if (!tunables->use_sched_load)
		return count;

	for_each_possible_cpu(cpu) {
		if (!per_cpu(polinfo, cpu))
			continue;
		t = per_cpu(polinfo, cpu)->cached_tunables;
		if (t && t->use_sched_load) {
			t->timer_rate = val_round;
			t->prev_timer_rate = val_round;
		}
	}
	set_window_helper(tunables);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor

	return count;
}

static ssize_t show_timer_slack(struct cpufreq_interactive_tunables *tunables,
		char *buf)
{
	return sprintf(buf, "%d\n", tunables->timer_slack_val);
}

static ssize_t store_timer_slack(struct cpufreq_interactive_tunables *tunables,
		const char *buf, size_t count)
{
	int ret;
	unsigned long val;

	ret = kstrtol(buf, 10, &val);
	if (ret < 0)
		return ret;

	tunables->timer_slack_val = val;
	return count;
}

static ssize_t show_boost(struct cpufreq_interactive_tunables *tunables,
			  char *buf)
{
	return sprintf(buf, "%d\n", tunables->boost_val);
}

static ssize_t store_boost(struct cpufreq_interactive_tunables *tunables,
			   const char *buf, size_t count)
{
	/*int ret;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
  
	tunables->boost_val = val;

	if (tunables->boost_val) {
		trace_cpufreq_interactive_boost("on");
<<<<<<< HEAD
                if (!tunables->boosted)
                        cpufreq_interactive_boost(tunables);
=======
		if (!tunables->boosted)
			cpufreq_interactive_boost(tunables);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
	} else {
		tunables->boostpulse_endtime = ktime_to_us(ktime_get());
		trace_cpufreq_interactive_unboost("off");
	}
*/
	return count;
}

static ssize_t store_boostpulse(struct cpufreq_interactive_tunables *tunables,
				const char *buf, size_t count)
{
	int ret;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	tunables->boostpulse_endtime = ktime_to_us(ktime_get()) +
		tunables->boostpulse_duration_val;
	trace_cpufreq_interactive_boost("pulse");
<<<<<<< HEAD
        if (!tunables->boosted)
                cpufreq_interactive_boost(tunables);
=======
	if (!tunables->boosted)
		cpufreq_interactive_boost(tunables);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
	return count;
}

static ssize_t show_boostpulse_duration(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%d\n", tunables->boostpulse_duration_val);
}

static ssize_t store_boostpulse_duration(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	tunables->boostpulse_duration_val = val;
 	return count;
}

static ssize_t show_screen_off_maxfreq(struct cpufreq_interactive_tunables *tunables,
                char *buf)
{
	return sprintf(buf, "%lu\n", screen_off_max);
}

static ssize_t store_screen_off_maxfreq(struct cpufreq_interactive_tunables *tunables,
                const char *buf, size_t count)
{
	int ret;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0) return ret;
	if (val < 384000) screen_off_max = DEFAULT_SCREEN_OFF_MAX;
	else screen_off_max = val;
	return count;
}

static ssize_t show_io_is_busy(struct cpufreq_interactive_tunables *tunables,
		char *buf)
{
	return sprintf(buf, "%u\n", tunables->io_is_busy);
}

static ssize_t store_io_is_busy(struct cpufreq_interactive_tunables *tunables,
		const char *buf, size_t count)
{
	int ret;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	tunables->io_is_busy = val;
	return count;
}

#ifdef CONFIG_MODE_AUTO_CHANGE
static ssize_t show_mode(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%u\n", tunables->mode);
}

<<<<<<< HEAD
static ssize_t store_mode(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;
=======
	for_each_possible_cpu(cpu) {
		if (!per_cpu(polinfo, cpu))
			continue;
		t = per_cpu(polinfo, cpu)->cached_tunables;
		if (t && t->use_sched_load)
			t->io_is_busy = val;
	}
	sched_set_io_is_busy(val);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	val &= MULTI_MODE | SINGLE_MODE | NO_MODE;
	tunables->mode = val;
	return count;
}

static ssize_t show_enforced_mode(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%u\n", tunables->enforced_mode);
}

static ssize_t store_enforced_mode(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;

<<<<<<< HEAD
	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
=======
	set_window_count++;
	if (set_window_count > 1) {
		for_each_possible_cpu(j) {
			if (!per_cpu(polinfo, j))
				continue;
			t = per_cpu(polinfo, j)->cached_tunables;
			if (t && t->use_sched_load) {
				tunables->timer_rate = t->timer_rate;
				tunables->io_is_busy = t->io_is_busy;
				break;
			}
		}
	} else {
		rc = set_window_helper(tunables);
		if (rc) {
			pr_err("%s: Failed to set sched window\n", __func__);
			set_window_count--;
			goto out;
		}
		sched_set_io_is_busy(tunables->io_is_busy);
	}
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor

	val &= MULTI_MODE | SINGLE_MODE | NO_MODE;
	tunables->enforced_mode = val;
	return count;
}

static ssize_t show_param_index(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%u\n", tunables->param_index);
}

static ssize_t store_param_index(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;
	unsigned long flags;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	val &= MULTI_MODE | SINGLE_MODE | NO_MODE;

	spin_lock_irqsave(&tunables->param_index_lock, flags);
	tunables->param_index = val;
	spin_unlock_irqrestore(&tunables->param_index_lock, flags);
	return count;
}

static ssize_t show_multi_enter_load(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%u\n", tunables->multi_enter_load);
}

static ssize_t store_multi_enter_load(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
			return ret;

	tunables->multi_enter_load = val;
	return count;
}

static ssize_t show_multi_exit_load(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%u\n", tunables->multi_exit_load);
}

static ssize_t store_multi_exit_load(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
			return ret;

	tunables->multi_exit_load = val;
	return count;
}

static ssize_t show_single_enter_load(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%u\n", tunables->single_enter_load);
}

static ssize_t store_single_enter_load(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
			return ret;

	tunables->single_enter_load = val;
	return count;
}

static ssize_t show_single_exit_load(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%u\n", tunables->single_exit_load);
}

static ssize_t store_single_exit_load(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
			return ret;

	tunables->single_exit_load = val;
	return count;
}

static ssize_t show_multi_enter_time(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%lu\n", tunables->multi_enter_time);
}

static ssize_t store_multi_enter_time(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
			return ret;

	tunables->multi_enter_time = val;
	return count;
}

static ssize_t show_multi_exit_time(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%lu\n", tunables->multi_exit_time);
}

static ssize_t store_multi_exit_time(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
			return ret;

	tunables->multi_exit_time = val;
	return count;
}

static ssize_t show_single_enter_time(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%lu\n", tunables->single_enter_time);
}

static ssize_t store_single_enter_time(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
			return ret;

	tunables->single_enter_time = val;
	return count;
}

static ssize_t show_single_exit_time(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%lu\n", tunables->single_exit_time);
}

static ssize_t store_single_exit_time(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
			return ret;

	tunables->single_exit_time = val;
	return count;
}

static ssize_t show_cpu_util(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	int i;
	u64 now;
	struct cpufreq_interactive_cpuinfo *pcpu;
	struct cpufreq_policy *policy = container_of(tunables->policy,
		struct cpufreq_policy, policy);
	ssize_t ret = 0;

	for_each_cpu_mask(i, policy->related_cpus[0]) {
		if (cpu_online(i)) {
			pcpu = &per_cpu(cpuinfo, i);
			get_cpu_idle_time(i, &now, tunables->io_is_busy);

			if (now - pcpu->time_in_idle_timestamp <= tunables->timer_rate)
				ret += sprintf(buf + ret, "%3u ", per_cpu(cpu_util, i));
			else
				ret += sprintf(buf + ret, "%3s ", (pcpu->target_freq == pcpu->policy->max) ? "H_I" : "L_I");
		} else
			ret += sprintf(buf + ret, "OFF ");
	}

	--ret;
	ret += sprintf(buf + ret, "\n");
	return ret;
}

static ssize_t show_single_cluster0_min_freq(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%u\n", tunables->single_cluster0_min_freq);
}

static ssize_t store_single_cluster0_min_freq(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
			return ret;

	tunables->single_cluster0_min_freq = val;
	return count;
}

static ssize_t show_multi_cluster0_min_freq(struct cpufreq_interactive_tunables
		*tunables, char *buf)
{
	return sprintf(buf, "%u\n", tunables->multi_cluster0_min_freq);
}

static ssize_t store_multi_cluster0_min_freq(struct cpufreq_interactive_tunables
		*tunables, const char *buf, size_t count)
{
	int ret;
	long unsigned int val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
			return ret;

	tunables->multi_cluster0_min_freq = val;
	return count;
}
#endif
/*
 * Create show/store routines
 * - sys: One governor instance for complete SYSTEM
 * - pol: One governor instance per struct cpufreq_policy
 */
#define show_gov_pol_sys(file_name)					\
static ssize_t show_##file_name##_gov_sys				\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	return show_##file_name(common_tunables, buf);			\
}									\
									\
static ssize_t show_##file_name##_gov_pol				\
(struct cpufreq_policy *policy, char *buf)				\
{									\
	return show_##file_name(policy->governor_data, buf);		\
}

#define store_gov_pol_sys(file_name)					\
static ssize_t store_##file_name##_gov_sys				\
(struct kobject *kobj, struct attribute *attr, const char *buf,		\
	size_t count)							\
{									\
	return store_##file_name(common_tunables, buf, count);		\
}									\
									\
static ssize_t store_##file_name##_gov_pol				\
(struct cpufreq_policy *policy, const char *buf, size_t count)		\
{									\
	return store_##file_name(policy->governor_data, buf, count);	\
}

#define show_store_gov_pol_sys(file_name)				\
show_gov_pol_sys(file_name);						\
store_gov_pol_sys(file_name)

show_store_gov_pol_sys(target_loads);
show_store_gov_pol_sys(above_hispeed_delay);
show_store_gov_pol_sys(hispeed_freq);
show_store_gov_pol_sys(go_hispeed_load);
show_store_gov_pol_sys(min_sample_time);
show_store_gov_pol_sys(timer_rate);
show_store_gov_pol_sys(timer_slack);
show_store_gov_pol_sys(boost);
store_gov_pol_sys(boostpulse);
show_store_gov_pol_sys(boostpulse_duration);
show_store_gov_pol_sys(io_is_busy);
<<<<<<< HEAD
show_store_gov_pol_sys(screen_off_maxfreq);

#ifdef CONFIG_MODE_AUTO_CHANGE
show_store_gov_pol_sys(mode);
show_store_gov_pol_sys(enforced_mode);
show_store_gov_pol_sys(param_index);
show_store_gov_pol_sys(multi_enter_load);
show_store_gov_pol_sys(multi_exit_load);
show_store_gov_pol_sys(single_enter_load);
show_store_gov_pol_sys(single_exit_load);
show_store_gov_pol_sys(multi_enter_time);
show_store_gov_pol_sys(multi_exit_time);
show_store_gov_pol_sys(single_enter_time);
show_store_gov_pol_sys(single_exit_time);
show_store_gov_pol_sys(single_cluster0_min_freq);
show_store_gov_pol_sys(multi_cluster0_min_freq);
show_gov_pol_sys(cpu_util);
#endif
=======
show_store_gov_pol_sys(use_sched_load);
show_store_gov_pol_sys(use_migration_notif);
show_store_gov_pol_sys(max_freq_hysteresis);
show_store_gov_pol_sys(align_windows);
show_store_gov_pol_sys(ignore_hispeed_on_notif);
show_store_gov_pol_sys(fast_ramp_down);

>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor
#define gov_sys_attr_rw(_name)						\
static struct global_attr _name##_gov_sys =				\
__ATTR(_name, 0660, show_##_name##_gov_sys, store_##_name##_gov_sys)

#define gov_pol_attr_rw(_name)						\
static struct freq_attr _name##_gov_pol =				\
__ATTR(_name, 0660, show_##_name##_gov_pol, store_##_name##_gov_pol)

#define gov_sys_pol_attr_rw(_name)					\
	gov_sys_attr_rw(_name);						\
	gov_pol_attr_rw(_name)

gov_sys_pol_attr_rw(target_loads);
gov_sys_pol_attr_rw(above_hispeed_delay);
gov_sys_pol_attr_rw(hispeed_freq);
gov_sys_pol_attr_rw(go_hispeed_load);
gov_sys_pol_attr_rw(min_sample_time);
gov_sys_pol_attr_rw(timer_rate);
gov_sys_pol_attr_rw(timer_slack);
gov_sys_pol_attr_rw(boost);
gov_sys_pol_attr_rw(boostpulse_duration);
gov_sys_pol_attr_rw(io_is_busy);
<<<<<<< HEAD
gov_sys_pol_attr_rw(screen_off_maxfreq);
#ifdef CONFIG_MODE_AUTO_CHANGE
gov_sys_pol_attr_rw(mode);
gov_sys_pol_attr_rw(enforced_mode);
gov_sys_pol_attr_rw(param_index);
gov_sys_pol_attr_rw(multi_enter_load);
gov_sys_pol_attr_rw(multi_exit_load);
gov_sys_pol_attr_rw(single_enter_load);
gov_sys_pol_attr_rw(single_exit_load);
gov_sys_pol_attr_rw(multi_enter_time);
gov_sys_pol_attr_rw(multi_exit_time);
gov_sys_pol_attr_rw(single_enter_time);
gov_sys_pol_attr_rw(single_exit_time);
gov_sys_pol_attr_rw(single_cluster0_min_freq);
gov_sys_pol_attr_rw(multi_cluster0_min_freq);
#endif
=======
gov_sys_pol_attr_rw(use_sched_load);
gov_sys_pol_attr_rw(use_migration_notif);
gov_sys_pol_attr_rw(max_freq_hysteresis);
gov_sys_pol_attr_rw(align_windows);
gov_sys_pol_attr_rw(ignore_hispeed_on_notif);
gov_sys_pol_attr_rw(fast_ramp_down);
>>>>>>> 8b166a3... Updates and Optimizations to Interactive Governor

static struct global_attr boostpulse_gov_sys =
	__ATTR(boostpulse, 0200, NULL, store_boostpulse_gov_sys);

static struct freq_attr boostpulse_gov_pol =
	__ATTR(boostpulse, 0200, NULL, store_boostpulse_gov_pol);
#ifdef CONFIG_MODE_AUTO_CHANGE
static struct global_attr cpu_util_gov_sys =
	__ATTR(cpu_util, 0444, show_cpu_util_gov_sys, NULL);

static struct freq_attr cpu_util_gov_pol =
	__ATTR(cpu_util, 0444, show_cpu_util_gov_pol, NULL);
#endif

/* One Governor instance for entire system */
static struct attribute *interactive_attributes_gov_sys[] = {
	&target_loads_gov_sys.attr,
	&above_hispeed_delay_gov_sys.attr,
	&hispeed_freq_gov_sys.attr,
	&go_hispeed_load_gov_sys.attr,
	&min_sample_time_gov_sys.attr,
	&timer_rate_gov_sys.attr,
	&timer_slack_gov_sys.attr,
	&boost_gov_sys.attr,
	&boostpulse_gov_sys.attr,
	&boostpulse_duration_gov_sys.attr,
	&io_is_busy_gov_sys.attr,
	&use_sched_load_gov_sys.attr,
	&use_migration_notif_gov_sys.attr,
	&max_freq_hysteresis_gov_sys.attr,
	&align_windows_gov_sys.attr,
	&ignore_hispeed_on_notif_gov_sys.attr,
	&fast_ramp_down_gov_sys.attr,
	NULL,
};

static struct attribute_group interactive_attr_group_gov_sys = {
	.attrs = interactive_attributes_gov_sys,
	.name = "interactive",
};

/* Per policy governor instance */
static struct attribute *interactive_attributes_gov_pol[] = {
	&target_loads_gov_pol.attr,
	&above_hispeed_delay_gov_pol.attr,
	&hispeed_freq_gov_pol.attr,
	&go_hispeed_load_gov_pol.attr,
	&min_sample_time_gov_pol.attr,
	&timer_rate_gov_pol.attr,
	&timer_slack_gov_pol.attr,
	&boost_gov_pol.attr,
	&boostpulse_gov_pol.attr,
	&boostpulse_duration_gov_pol.attr,
	&io_is_busy_gov_pol.attr,
	&use_sched_load_gov_pol.attr,
	&use_migration_notif_gov_pol.attr,
	&max_freq_hysteresis_gov_pol.attr,
	&align_windows_gov_pol.attr,
	&ignore_hispeed_on_notif_gov_pol.attr,
	&fast_ramp_down_gov_pol.attr,
	NULL,
};

static struct attribute_group interactive_attr_group_gov_pol = {
	.attrs = interactive_attributes_gov_pol,
	.name = "interactive",
};

static struct attribute_group *get_sysfs_attr(void)
{
	if (have_governor_per_policy())
		return &interactive_attr_group_gov_pol;
	else
		return &interactive_attr_group_gov_sys;
}

static void cpufreq_interactive_nop_timer(unsigned long data)
{
}

static struct cpufreq_interactive_tunables *alloc_tunable(
					struct cpufreq_policy *policy)
{
	struct cpufreq_interactive_tunables *tunables;

	tunables = kzalloc(sizeof(*tunables), GFP_KERNEL);
	if (!tunables)
		return ERR_PTR(-ENOMEM);

	tunables->above_hispeed_delay = default_above_hispeed_delay;
	tunables->nabove_hispeed_delay =
		ARRAY_SIZE(default_above_hispeed_delay);
	tunables->go_hispeed_load = DEFAULT_GO_HISPEED_LOAD;
	tunables->target_loads = default_target_loads;
	tunables->ntarget_loads = ARRAY_SIZE(default_target_loads);
	tunables->min_sample_time = DEFAULT_MIN_SAMPLE_TIME;
	tunables->timer_rate = DEFAULT_TIMER_RATE;
	tunables->prev_timer_rate = DEFAULT_TIMER_RATE;
	tunables->boostpulse_duration_val = DEFAULT_MIN_SAMPLE_TIME;
	tunables->timer_slack_val = DEFAULT_TIMER_SLACK;

	spin_lock_init(&tunables->target_loads_lock);
	spin_lock_init(&tunables->above_hispeed_delay_lock);

	return tunables;
}

static struct cpufreq_interactive_policyinfo *get_policyinfo(
					struct cpufreq_policy *policy)
{
	struct cpufreq_interactive_policyinfo *ppol =
				per_cpu(polinfo, policy->cpu);
	int i;

	/* polinfo already allocated for policy, return */
	if (ppol)
		return ppol;

	ppol = kzalloc(sizeof(*ppol), GFP_KERNEL);
	if (!ppol)
		return ERR_PTR(-ENOMEM);

	init_timer_deferrable(&ppol->policy_timer);
	ppol->policy_timer.function = cpufreq_interactive_timer;
	init_timer(&ppol->policy_slack_timer);
	ppol->policy_slack_timer.function = cpufreq_interactive_nop_timer;
	spin_lock_init(&ppol->load_lock);
	spin_lock_init(&ppol->target_freq_lock);
	init_rwsem(&ppol->enable_sem);

	for_each_cpu(i, policy->related_cpus)
		per_cpu(polinfo, i) = ppol;
	return ppol;
}

/* This function is not multithread-safe. */
static void free_policyinfo(int cpu)
{
	struct cpufreq_interactive_policyinfo *ppol = per_cpu(polinfo, cpu);
	int j;

	if (!ppol)
		return;

	for_each_possible_cpu(j)
		if (per_cpu(polinfo, j) == ppol)
			per_cpu(polinfo, cpu) = NULL;
	kfree(ppol->cached_tunables);
	kfree(ppol);
}

static struct cpufreq_interactive_tunables *get_tunables(
				struct cpufreq_interactive_policyinfo *ppol)
{
	if (have_governor_per_policy())
		return ppol->cached_tunables;
	else
		return cached_common_tunables;
}
#endif

static int cpufreq_governor_interactive(struct cpufreq_policy *policy,
		unsigned int event)
{
	int rc;
	struct cpufreq_interactive_policyinfo *ppol;
	struct cpufreq_frequency_table *freq_table;
	struct cpufreq_interactive_tunables *tunables;
	unsigned long flags;
	unsigned int anyboost;

	if (have_governor_per_policy())
		tunables = policy->governor_data;
	else
		tunables = common_tunables;

	WARN_ON(!tunables && (event != CPUFREQ_GOV_POLICY_INIT));

	switch (event) {
	case CPUFREQ_GOV_POLICY_INIT:
		ppol = get_policyinfo(policy);
		if (IS_ERR(ppol))
			return PTR_ERR(ppol);

		if (have_governor_per_policy()) {
			WARN_ON(tunables);
		} else if (tunables) {
			tunables->usage_count++;
			policy->governor_data = tunables;
			return 0;
		}

		tunables = get_tunables(ppol);
		if (!tunables) {
			pr_err("%s: POLICY_INIT: kzalloc failed\n", __func__);
			return -ENOMEM;
		}

		if (!tuned_parameters[policy->cpu]) {
			tunables->above_hispeed_delay = default_above_hispeed_delay;
			tunables->nabove_hispeed_delay =
				ARRAY_SIZE(default_above_hispeed_delay);
			tunables->go_hispeed_load = DEFAULT_GO_HISPEED_LOAD;
			tunables->target_loads = default_target_loads;
			tunables->ntarget_loads = ARRAY_SIZE(default_target_loads);
			tunables->min_sample_time = DEFAULT_MIN_SAMPLE_TIME;
			tunables->timer_rate = DEFAULT_TIMER_RATE;
			tunables->boostpulse_duration_val = DEFAULT_MIN_SAMPLE_TIME;
			tunables->timer_slack_val = DEFAULT_TIMER_SLACK;
#ifdef CONFIG_MODE_AUTO_CHANGE
			tunables->multi_enter_time = DEFAULT_MULTI_ENTER_TIME;
			tunables->multi_enter_load = 4 * DEFAULT_TARGET_LOAD;
			tunables->multi_exit_time = DEFAULT_MULTI_EXIT_TIME;
			tunables->multi_exit_load = 4 * DEFAULT_TARGET_LOAD;
			tunables->single_enter_time = DEFAULT_SINGLE_ENTER_TIME;
			tunables->single_enter_load = DEFAULT_TARGET_LOAD;
			tunables->single_exit_time = DEFAULT_SINGLE_EXIT_TIME;
			tunables->single_exit_load = DEFAULT_TARGET_LOAD;
			tunables->single_cluster0_min_freq = DEFAULT_SINGLE_CLUSTER0_MIN_FREQ;
			tunables->multi_cluster0_min_freq = DEFAULT_MULTI_CLUSTER0_MIN_FREQ;

			cpufreq_param_set_init(tunables);
#endif
		} else {
			memcpy(tunables, tuned_parameters[policy->cpu], sizeof(*tunables));
			kfree(tuned_parameters[policy->cpu]);
		}
		tunables->usage_count = 1;

		/* update handle for get cpufreq_policy */
		tunables->policy = &policy->policy;

		spin_lock_init(&tunables->target_loads_lock);
		spin_lock_init(&tunables->above_hispeed_delay_lock);
#ifdef CONFIG_MODE_AUTO_CHANGE
		spin_lock_init(&tunables->mode_lock);
		spin_lock_init(&tunables->param_index_lock);
#endif

		policy->governor_data = tunables;
		if (!have_governor_per_policy())
			common_tunables = tunables;

		rc = sysfs_create_group(get_governor_parent_kobj(policy),
				get_sysfs_attr());
		if (rc) {
			kfree(tunables);
			policy->governor_data = NULL;
			if (!have_governor_per_policy())
				common_tunables = NULL;
			return rc;
		}

		if (!policy->governor->initialized)
			cpufreq_register_notifier(&cpufreq_notifier_block,
					CPUFREQ_TRANSITION_NOTIFIER);

		if (tunables->use_sched_load)
			cpufreq_interactive_enable_sched_input(tunables);

		if (have_governor_per_policy())
			ppol->cached_tunables = tunables;
		else
			cached_common_tunables = tunables;
		break;

	case CPUFREQ_GOV_POLICY_EXIT:
		if (!--tunables->usage_count) {
			if (policy->governor->initialized == 1)
				cpufreq_unregister_notifier(&cpufreq_notifier_block,
						CPUFREQ_TRANSITION_NOTIFIER);

			sysfs_remove_group(get_governor_parent_kobj(policy),
					get_sysfs_attr());

			tuned_parameters[policy->cpu] = kzalloc(sizeof(*tunables), GFP_KERNEL);
			if (!tuned_parameters[policy->cpu]) {
				pr_err("%s: POLICY_EXIT: kzalloc failed\n", __func__);
				return -ENOMEM;
			}
			memcpy(tuned_parameters[policy->cpu], tunables, sizeof(*tunables));
			kfree(tunables);
			common_tunables = NULL;
		}

		policy->governor_data = NULL;
		break;

	case CPUFREQ_GOV_START:
		mutex_lock(&gov_lock);

		freq_table = cpufreq_frequency_get_table(policy->cpu);

		ppol = per_cpu(polinfo, policy->cpu);
		ppol->policy = policy;
		ppol->target_freq = policy->cur;
		ppol->freq_table = freq_table;
		ppol->floor_freq = ppol->target_freq;
		ppol->floor_validate_time = ktime_to_us(ktime_get());
		ppol->hispeed_validate_time = ppol->floor_validate_time;
		ppol->min_freq = policy->min;
		ppol->reject_notification = true;
		down_write(&ppol->enable_sem);
		del_timer_sync(&ppol->policy_timer);
		del_timer_sync(&ppol->policy_slack_timer);
		ppol->policy_timer.data = policy->cpu;
		ppol->last_evaluated_jiffy = get_jiffies_64();
		cpufreq_interactive_timer_start(tunables, policy->cpu);
		ppol->governor_enabled = 1;
		up_write(&ppol->enable_sem);
		ppol->reject_notification = false;

		mutex_unlock(&gov_lock);
		break;

	case CPUFREQ_GOV_STOP:
		mutex_lock(&gov_lock);

		ppol = per_cpu(polinfo, policy->cpu);
		ppol->reject_notification = true;
		down_write(&ppol->enable_sem);
		ppol->governor_enabled = 0;
		ppol->target_freq = 0;
		del_timer_sync(&ppol->policy_timer);
		del_timer_sync(&ppol->policy_slack_timer);
		up_write(&ppol->enable_sem);
		ppol->reject_notification = false;

		mutex_unlock(&gov_lock);
		break;

	case CPUFREQ_GOV_LIMITS:
		__cpufreq_driver_target(policy,
				policy->cur, CPUFREQ_RELATION_L);

		ppol = per_cpu(polinfo, policy->cpu);

		down_read(&ppol->enable_sem);
		if (ppol->governor_enabled) {
			spin_lock_irqsave(&ppol->target_freq_lock, flags);
			if (policy->max < ppol->target_freq) {
				ppol->target_freq = policy->max;
			} else if (policy->min > ppol->target_freq) {
				ppol->target_freq = policy->min;
				anyboost = 1;
			}
			spin_unlock_irqrestore(&ppol->target_freq_lock, flags);

			if (policy->min < ppol->min_freq)
				cpufreq_interactive_timer_resched(policy->cpu,
								  true);
			ppol->min_freq = policy->min;
		}

		up_read(&ppol->enable_sem);

		if (anyboost) {
			u64 now = ktime_to_us(ktime_get());

			ppol->hispeed_validate_time = now;
			ppol->floor_freq = policy->min;
			ppol->floor_validate_time = now;
		}

		break;
	}
	return 0;
}

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_INTERACTIVE
static
#endif
struct cpufreq_governor cpufreq_gov_interactive = {
	.name = "interactive",
	.governor = cpufreq_governor_interactive,
	.max_transition_latency = 10000000,
	.owner = THIS_MODULE,
};

static int __init cpufreq_interactive_init(void)
{
	struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };

	spin_lock_init(&speedchange_cpumask_lock);
	mutex_init(&gov_lock);

	speedchange_task =
		kthread_create(cpufreq_interactive_speedchange_task, NULL,
				"cfinteractive");
	if (IS_ERR(speedchange_task))
		return PTR_ERR(speedchange_task);

	kthread_bind(speedchange_task, 0);

#ifdef CONFIG_ARCH_EXYNOS
	pm_qos_add_notifier(PM_QOS_CLUSTER1_FREQ_MIN, &cpufreq_interactive_cluster1_min_qos_notifier);
	pm_qos_add_notifier(PM_QOS_CLUSTER1_FREQ_MAX, &cpufreq_interactive_cluster1_max_qos_notifier);
#ifdef CONFIG_ARM_EXYNOS_MP_CPUFREQ
	pm_qos_add_notifier(PM_QOS_CLUSTER0_FREQ_MIN, &cpufreq_interactive_cluster0_min_qos_notifier);
	pm_qos_add_notifier(PM_QOS_CLUSTER0_FREQ_MAX, &cpufreq_interactive_cluster0_max_qos_notifier);
#endif
#endif
	/* NB: wake up so the thread does not look hung to the freezer */
	wake_up_process(speedchange_task);

#ifdef CONFIG_MODE_AUTO_CHANGE
	mode_auto_change_minlock_wq = alloc_workqueue("mode_auto_change_minlock_wq", WQ_HIGHPRI, 0);
	if(!mode_auto_change_minlock_wq)
		pr_info("mode auto change minlock workqueue init error\n");
	INIT_WORK(&mode_auto_change_minlock_work, mode_auto_change_minlock);
#endif
	return cpufreq_register_governor(&cpufreq_gov_interactive);
}

#ifdef CONFIG_MODE_AUTO_CHANGE
static void mode_auto_change_minlock(struct work_struct *work)
{
	set_qos(&cluster0_min_freq_qos, PM_QOS_CLUSTER0_FREQ_MIN, cluster0_min_freq);
}
#endif

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_INTERACTIVE
fs_initcall(cpufreq_interactive_init);
#else
module_init(cpufreq_interactive_init);
#endif

static void __exit cpufreq_interactive_exit(void)
{
	int cpu;

	cpufreq_unregister_governor(&cpufreq_gov_interactive);
	kthread_stop(speedchange_task);
	put_task_struct(speedchange_task);

	for_each_possible_cpu(cpu)
		free_policyinfo(cpu);
}

module_exit(cpufreq_interactive_exit);

MODULE_AUTHOR("Mike Chan <mike@android.com>");
MODULE_DESCRIPTION("'cpufreq_interactive' - A cpufreq governor for "
	"Latency sensitive workloads");
MODULE_LICENSE("GPL");

