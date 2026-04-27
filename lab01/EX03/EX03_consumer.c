#include <linux/module.h>

// the symbols imported from the provider module
extern int  shared_value;
extern void open_greet(void);
extern void gpl_greet(void);

static int __init consumer_init(void)
{
    printk(KERN_ALERT "consumer: loaded.\n");

    // exported variable
    printk(KERN_ALERT "consumer: shared_value from provider: %d\n",
           shared_value);

    // Call the openly exported function (EXPORT_SYMBOL)
    open_greet();

    // Call the GPL-only exported function (EXPORT_SYMBOL_GPL)
    gpl_greet();

    return 0;
}

static void __exit consumer_exit(void)
{
    printk(KERN_ALERT "consumer: removed.\n");
}

module_init(consumer_init);
module_exit(consumer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("M&M");
MODULE_DESCRIPTION("EX03");
MODULE_VERSION("1.0");