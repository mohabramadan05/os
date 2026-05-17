#include <linux/module.h>
#include <linux/workqueue.h>

#define REPEAT_DELAY_SEC  2
#define REPEAT_DELAY      (REPEAT_DELAY_SEC * HZ)

static void repeat_delayed_handler(struct work_struct *work);

static struct workqueue_struct *repeat_queue;


static DECLARE_DELAYED_WORK(repeat_dwork, repeat_delayed_handler);


static atomic_t exec_count = ATOMIC_INIT(0);


static void repeat_delayed_handler(struct work_struct *work)
{
    int count = atomic_inc_return(&exec_count);

    pr_notice("repeat_wq_mod: delayed work executed (run #%d)\n", count);

    if (!queue_delayed_work(repeat_queue, &repeat_dwork, REPEAT_DELAY))
        pr_warn("repeat_wq_mod: work was already queued (unexpected)\n");
}

static int __init repeat_wq_mod_init(void)
{
    repeat_queue = create_singlethread_workqueue("repeat_wq");
    if (IS_ERR(repeat_queue)) {
        pr_alert("repeat_wq_mod: failed to create work queue: %ld\n",
                 PTR_ERR(repeat_queue));
        return -ENOMEM;
    }

    pr_notice("repeat_wq_mod: work queue created, "
              "scheduling first run in %d second(s)\n", REPEAT_DELAY_SEC);

    /* Schedule the first (and all subsequent) executions. */
    if (!queue_delayed_work(repeat_queue, &repeat_dwork, REPEAT_DELAY))
        pr_warn("repeat_wq_mod: first schedule returned false (unexpected)\n");

    return 0;
}

static void __exit repeat_wq_mod_exit(void)
{
    if (cancel_delayed_work_sync(&repeat_dwork))
        pr_notice("repeat_wq_mod: pending delayed work cancelled\n");
    else
        pr_notice("repeat_wq_mod: no pending work at exit time\n");

    destroy_workqueue(repeat_queue);

    pr_notice("repeat_wq_mod: unloaded after %d execution(s)\n",
              atomic_read(&exec_count));
}


module_init(repeat_wq_mod_init);
module_exit(repeat_wq_mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("M&M");
MODULE_DESCRIPTION("Ex05");
MODULE_VERSION("1.0");