#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/wait.h>

enum thread_index
{
    WAKING_THREAD,
    SIMPLE_THREAD
};

static struct threads_structure
{
    struct task_struct *thread[2];
} threads;

static wait_queue_head_t wait_queue;
static bool condition;

static int simple_thread(void *data)
{
    DEFINE_WAIT(wait);
    for (;;)
    {
        add_wait_queue(&wait_queue, &wait);
        while (!condition)
        {
            prepare_to_wait(&wait_queue, &wait, TASK_INTERRUPTIBLE);
            if (kthread_should_stop())
                return 0;
            printk(KERN_INFO "[simple_thread]: awake\n");
            schedule();
        }
        condition = false;
        finish_wait(&wait_queue, &wait);
    }
}

static int waking_thread(void *data)
{
    for (;;)
    {
        if (kthread_should_stop())
            return 0;
        set_current_state(TASK_INTERRUPTIBLE);
        if (schedule_timeout(1 * HZ))
            printk(KERN_INFO "Signal received!\n");
        condition = true;
        wake_up(&wait_queue);
    }
}

static int __init threads_init(void)
{
    init_waitqueue_head(&wait_queue);
    // threads.thread[SIMPLE_THREAD] = kthread_run(simple_thread, NULL, "simple_thread");
    // threads.thread[WAKING_THREAD] = kthread_run(waking_thread, NULL, "waking_thread");
    threads.thread[SIMPLE_THREAD] = kthread_create(simple_thread, NULL, "simple_thread");
    threads.thread[WAKING_THREAD] = kthread_create(waking_thread, NULL, "waking_thread");

    if (!IS_ERR(threads.thread[SIMPLE_THREAD]))
        wake_up_process(threads.thread[SIMPLE_THREAD]);
        
    if (!IS_ERR(threads.thread[WAKING_THREAD]))
        wake_up_process(threads.thread[WAKING_THREAD]);

    return 0;
}

static void __exit threads_exit(void)
{
    kthread_stop(threads.thread[WAKING_THREAD]);
    kthread_stop(threads.thread[SIMPLE_THREAD]);
}

module_init(threads_init);
module_exit(threads_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ex04");
MODULE_AUTHOR("M&M");
MODULE_VERSION("1.0");