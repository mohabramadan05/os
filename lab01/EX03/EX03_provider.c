#include <linux/module.h>

// shared variable 
int shared_value = 42;
EXPORT_SYMBOL(shared_value);

// function to any module
void open_greet(void)
{
    printk(KERN_ALERT "provider: open_greet() called, available to all\n");
}
EXPORT_SYMBOL(open_greet);

// function to GPL modules only
void gpl_greet(void)
{
    printk(KERN_ALERT "provider: gpl_greet() called, GPL modules only\n");
}
EXPORT_SYMBOL_GPL(gpl_greet);

static int __init provider_init(void)
{
    printk(KERN_ALERT "provider: loaded. shared_value=%d\n", shared_value);
    return 0;
}

static void __exit provider_exit(void)
{
    printk(KERN_ALERT "provider: removed.\n");
}

module_init(provider_init);
module_exit(provider_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("M&M");
MODULE_DESCRIPTION("EX03");
MODULE_VERSION("1.0");