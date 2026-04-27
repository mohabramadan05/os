#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/sched.h>

struct number_node {
    struct list_head list;
    int value;
};

static LIST_HEAD(number_list);

static DEFINE_MUTEX(list_mutex);

static DECLARE_COMPLETION(item_ready);

enum thread_index { PRODUCER, CONSUMER };
static struct task_struct *threads[2];

static int produced_value = 0;


static int producer_thread(void *data)
{
    struct number_node *node;

    pr_info("producer: thread started\n");

    while (!kthread_should_stop()) {

        node = kmalloc(sizeof(*node), GFP_KERNEL);
        if (!node) {
            pr_alert("producer: kmalloc failed — skipping iteration\n");
            schedule();
            continue;
        }

        INIT_LIST_HEAD(&node->list);
        node->value = produced_value++;

        // lock the list, append the node at the tail, unlock.
        mutex_lock(&list_mutex);
        list_add_tail(&node->list, &number_list);
        mutex_unlock(&list_mutex);

        pr_info("producer: produced value: %d\n", node->value);

        // notify the consumer that one new item is available.
        complete(&item_ready);

        // let the consumer use cpu
        schedule();
    }

    pr_info("producer: thread stopping\n");
    return 0;
}



static int consumer_thread(void *data)
{
    struct number_node *node;

    pr_info("consumer: thread started\n");

    for (;;) {
        // wait until producer calls complete()
        wait_for_completion(&item_ready);

        // check stop condition
        if (kthread_should_stop())
            break;

        // remove the first (oldest) item from the list.
        mutex_lock(&list_mutex);
        if (!list_empty(&number_list)) {
            node = list_first_entry(&number_list,
                                    struct number_node,
                                    list);
            list_del(&node->list);  // unlink from the list
        } else {
            // Should not happen if completion counts are correct
            node = NULL;
        }
        mutex_unlock(&list_mutex);

        if (node) {
            pr_info("consumer: consumed value: %d\n", node->value);
            kfree(node);   // release the node memory
        }
    }

    pr_info("consumer: thread stopping\n");
    return 0;
}

static int __init prod_cons_init(void)
{
    pr_info("prod_cons: Module loaded\n");

    threads[CONSUMER] = kthread_run(consumer_thread, NULL, "consumer_thread");
    if (IS_ERR(threads[CONSUMER])) {
        pr_alert("prod_cons: Failed to create consumer thread\n");
        return PTR_ERR(threads[CONSUMER]);
    }

    threads[PRODUCER] = kthread_run(producer_thread, NULL, "producer_thread");
    if (IS_ERR(threads[PRODUCER])) {
        pr_alert("prod_cons: Failed to create producer thread\n");
        kthread_stop(threads[CONSUMER]);
        return PTR_ERR(threads[PRODUCER]);
    }

    return 0;
}

static void __exit prod_cons_exit(void)
{
    struct number_node *node, *tmp;

    kthread_stop(threads[PRODUCER]);
    complete(&item_ready);          // unblock the consumer if waiting
    kthread_stop(threads[CONSUMER]);

    // free any remaining items that were produced but not yet consumed.
    mutex_lock(&list_mutex);
    list_for_each_entry_safe(node, tmp, &number_list, list) {
        pr_info("prod_cons: cleanup: freeing unconsumed value %d\n",
                node->value);
        list_del(&node->list);
        kfree(node);
    }
    mutex_unlock(&list_mutex);

    pr_info("prod_cons: Module unloaded\n");
}


module_init(prod_cons_init);
module_exit(prod_cons_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ex02");
MODULE_AUTHOR("M&M");
MODULE_VERSION("1.0");