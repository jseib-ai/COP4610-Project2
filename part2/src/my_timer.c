// my_timer.c - kernel module that exposes the current and elapsed time in /proc/timer
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>

#define PROC_NAME "timer"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Prints the current time and the time elapsed since the last read");

static struct proc_dir_entry *timer_entry;
static struct timespec64 last_time;
static bool has_last_time;
static DEFINE_MUTEX(timer_lock);

static int timer_show(struct seq_file *m, void *v)
{
	struct timespec64 now, elapsed;

	ktime_get_real_ts64(&now);
	seq_printf(m, "current time: %lld.%09ld\n", (long long)now.tv_sec, now.tv_nsec);

	mutex_lock(&timer_lock);
	if (has_last_time) 
	{
		elapsed = timespec64_sub(now, last_time);
		seq_printf(m, "elapsed time: %lld.%09ld\n",
			   (long long)elapsed.tv_sec, elapsed.tv_nsec);
	}
	last_time = now;
	has_last_time = true;
	mutex_unlock(&timer_lock);

	return 0;
}

static int timer_open(struct inode *inode, struct file *file)
{
	return single_open(file, timer_show, NULL);
}

static const struct proc_ops timer_proc_ops = 
{
	.proc_open    = timer_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int __init timer_init(void)
{
	has_last_time = false;
	timer_entry = proc_create(PROC_NAME, 0444, NULL, &timer_proc_ops);
	if (!timer_entry)
		return -ENOMEM;
	return 0;
}

static void __exit timer_exit(void)
{
	proc_remove(timer_entry);
}

module_init(timer_init);
module_exit(timer_exit);
