#include <linux/module.h>
#include <linux/hrtimer.h>

static struct hrtimer timer;
static ktime_t delay;

static enum hrtimer_restart hrtimer_function(struct hrtimer *hrtimer)
{
    // u64 overruns;
    // pr_info("The timer is active!\n");
    // overruns = hrtimer_forward_now(hrtimer,delay);
    // pr_info("The overruns number since last activation: %llu.\n", overruns);

    pr_info("Task4: timer fired running only once\n");
    // Return HRTIMER_NORESTART so the timer is NOT re-scheduled 
    return HRTIMER_NORESTART;
}

static int __init hrtimer_module_init(void)
{
    delay = ktime_set(1, 0);
    hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    timer.function = hrtimer_function;
    hrtimer_start(&timer, delay, HRTIMER_MODE_REL);
    pr_info("Task4: module loaded timer scheduled for 1 second.\n");
    return 0;
}

static void __exit hrtimer_module_exit(void)
{
    if (!hrtimer_cancel(&timer))
        pr_alert("Task4: the timer was not active already fired\n");
    else
        pr_info("Task4: timer canceled before it fired\n");
    pr_info("Task4: module unloaded\n");
}

module_init(hrtimer_module_init);
module_exit(hrtimer_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("M&M");
MODULE_DESCRIPTION("ex04");
MODULE_VERSION("1.0");