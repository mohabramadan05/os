#include <linux/module.h>
#include <linux/kthread.h>       
#include <linux/mutex.h>         
#include <linux/atomic.h>       
#include <linux/kfifo.h>         
#include <linux/sched.h>         
#include <linux/delay.h>       


#define FIFO_SIZE   16
#define PRODUCE_MS  500   

static DEFINE_KFIFO(number_fifo, int, FIFO_SIZE);

static DEFINE_MUTEX(fifo_mutex);

static atomic_t item_count = ATOMIC_INIT(0);

enum thread_index { PRODUCER, CONSUMER, WAKING };
static struct task_struct *threads[3];

static int next_value = 0;

static int producer_thread(void *data)
{
    int value;
    int pushed;

    pr_info("producer: thread started (fifo size = %d)\n", FIFO_SIZE);

    while (!kthread_should_stop()) {

        value = next_value;

        mutex_lock(&fifo_mutex);

        if (kfifo_is_full(&number_fifo)) {
            pr_warn("producer: fifo FULL — dropping value %d\n", value);
            mutex_unlock(&fifo_mutex);
        } else {
            pushed = kfifo_in(&number_fifo, &value, 1);
            mutex_unlock(&fifo_mutex);

            if (pushed == 1) {
                atomic_inc(&item_count);
                next_value++;
                pr_info("producer: pushed value: %d  (items in fifo: %d)\n",
                        value, atomic_read(&item_count));
            }
        }

        msleep(PRODUCE_MS);
    }

    pr_info("producer: thread stopping\n");
    return 0;
}



static int consumer_thread(void *data)
{
    int value;
    int popped;

    pr_info("consumer: thread started\n");

    while (!kthread_should_stop()) {
        
        if (atomic_read(&item_count) == 0) {
            schedule();
            continue;
        }

        mutex_lock(&fifo_mutex);
        popped = kfifo_out(&number_fifo, &value, 1);
        mutex_unlock(&fifo_mutex);

        if (popped == 1) {
            atomic_dec(&item_count);
            pr_info("consumer: popped value: %d  (items remaining: %d)\n",
                    value, atomic_read(&item_count));
        }

        schedule();
    }

    pr_info("consumer: thread stopping\n");
    return 0;
}


static int waking_thread(void *data)
{
    pr_info("waking: thread started\n");

    for (;;) {
        if (kthread_should_stop())
            break;

        set_current_state(TASK_INTERRUPTIBLE);
        if (schedule_timeout(HZ))
            pr_info("waking: signal received\n");

        pr_info("waking: queue depth: %d / %d\n",
                atomic_read(&item_count), FIFO_SIZE);
    }

    pr_info("waking: thread stopping\n");
    return 0;
}


static int __init prod_cons_init(void)
{
    pr_info("ex5: Module loaded\n");

    threads[CONSUMER] = kthread_run(consumer_thread, NULL, "consumer_thread");
    if (IS_ERR(threads[CONSUMER])) {
        pr_alert("ex5: Failed to create consumer thread\n");
        return PTR_ERR(threads[CONSUMER]);
    }

    threads[PRODUCER] = kthread_run(producer_thread, NULL, "producer_thread");
    if (IS_ERR(threads[PRODUCER])) {
        pr_alert("ex5: Failed to create producer thread\n");
        kthread_stop(threads[CONSUMER]);
        return PTR_ERR(threads[PRODUCER]);
    }

    threads[WAKING] = kthread_run(waking_thread, NULL, "waking_thread");
    if (IS_ERR(threads[WAKING])) {
        pr_alert("ex5: Failed to create waking thread\n");
        kthread_stop(threads[PRODUCER]);
        kthread_stop(threads[CONSUMER]);
        return PTR_ERR(threads[WAKING]);
    }

    return 0;
}


static void __exit prod_cons_exit(void)
{
    int value;

    kthread_stop(threads[WAKING]);
    kthread_stop(threads[PRODUCER]);
    kthread_stop(threads[CONSUMER]);

    mutex_lock(&fifo_mutex);
    while (kfifo_out(&number_fifo, &value, 1) == 1)
        pr_info("ex5 cleanup: discarding unconsumed value %d\n", value);
    mutex_unlock(&fifo_mutex);

    pr_info("ex5: Module unloaded\n");
}


module_init(prod_cons_init);
module_exit(prod_cons_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ex05");
MODULE_AUTHOR("M&M");
MODULE_VERSION("1.0");