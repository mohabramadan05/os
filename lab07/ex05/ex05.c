#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/spinlock.h>


static long shared_value = 0;
static DEFINE_SPINLOCK(value_lock); // protects concurent timer access


static struct hrtimer timer1;
static struct hrtimer timer2;

static ktime_t delay1;   // 1 second  
static ktime_t delay2;   // 2 second 


static enum hrtimer_restart timer1_function(struct hrtimer *hrtimer)
{
    unsigned long flags;

    spin_lock_irqsave(&value_lock, flags);
    pr_info("Task5 Timer1: shared_value = %ld  (will add 2)\n", shared_value);
    shared_value += 2; // add 2
    spin_unlock_irqrestore(&value_lock, flags);

    hrtimer_forward_now(hrtimer, delay1);
    return HRTIMER_RESTART;
}


static enum hrtimer_restart timer2_function(struct hrtimer *hrtimer)
{
    unsigned long flags;

    spin_lock_irqsave(&value_lock, flags);
    pr_info("Task5 Timer2: shared_value = %ld  (will add 3)\n", shared_value);
    shared_value += 3; // add 3
    spin_unlock_irqrestore(&value_lock, flags);

    hrtimer_forward_now(hrtimer, delay2);
    return HRTIMER_RESTART;
}

static int __init task5_module_init(void)
{
    delay1 = ktime_set(1, 0);   // 1 second
    delay2 = ktime_set(2, 0);   // 2 second

    hrtimer_init(&timer1, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    timer1.function = timer1_function;
    hrtimer_start(&timer1, delay1, HRTIMER_MODE_REL);

    hrtimer_init(&timer2, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    timer2.function = timer2_function;
    hrtimer_start(&timer2, delay2, HRTIMER_MODE_REL);

    pr_info("Task5: Module loaded. timer1=1s (+2), timer2=2s (+3).\n");
    return 0;
}

static void __exit task5_module_exit(void)
{
    hrtimer_cancel(&timer1);
    hrtimer_cancel(&timer2);
    pr_info("Task5: Module unloaded. Final shared_value = %ld\n", shared_value);
}

module_init(task5_module_init);
module_exit(task5_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("M&M");
MODULE_DESCRIPTION("ex05");
MODULE_VERSION("1.0");