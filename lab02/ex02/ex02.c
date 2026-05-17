#include <linux/module.h>
#include <linux/slab.h>


struct stack_node {
    int value;
    struct stack_node *next;  
};

static struct stack_node *stack_top = NULL;

static struct kmem_cache *stack_node_cachep;

static void stack_node_constructor(void *arg)
{
    struct stack_node *node = (struct stack_node *)arg;
    node->value = 0;
    node->next  = NULL;
}

static int push(int value)
{
    struct stack_node *node;

    node = (struct stack_node *)kmem_cache_alloc(stack_node_cachep,
                                                  GFP_KERNEL);
    if (IS_ERR(node)) {
        pr_alert("stack_mod: kmem_cache_alloc failed: %ld\n",
                 PTR_ERR(node));
        return -ENOMEM;
    }

    node->value = value;
    node->next  = stack_top;   // link to previous top             
    stack_top   = node;        // update top-of-stack                 

    pr_notice("stack_mod: pushed %d\n", value);
    return 0;
}


static int pop(int *out)
{
    struct stack_node *node;

    if (!stack_top) {
        pr_warn("stack_mod: pop on empty stack\n");
        return -1;
    }

    node      = stack_top;
    stack_top = node->next;   // move top down by one level            
    *out      = node->value;

    kmem_cache_free(stack_node_cachep, node);
    return 0;
}


static int __init stack_mod_init(void)
{
    int i, rc;
    int values[] = {10, 20, 30, 40, 50};
    int n = ARRAY_SIZE(values);

    stack_node_cachep = kmem_cache_create(
        "stack_node_cache",          // in /proc/slabinfo 
        sizeof(struct stack_node),   // object size                    
        0,                           // offset of first object in slab 
        SLAB_HWCACHE_ALIGN | SLAB_POISON | SLAB_RED_ZONE,
        stack_node_constructor);     // per-object constructor         

    if (IS_ERR(stack_node_cachep)) {
        pr_alert("stack_mod: kmem_cache_create failed: %ld\n",
                 PTR_ERR(stack_node_cachep));
        return -ENOMEM;
    }
    pr_notice("stack_mod: slab cache created\n");

    for (i = 0; i < n; i++) {
        rc = push(values[i]);
        if (rc) {
            int tmp;
            while (stack_top)
                pop(&tmp);
            kmem_cache_destroy(stack_node_cachep);
            return rc;
        }
    }

    pr_notice("stack_mod: stack built with %d elements (top = %d)\n",
              n, stack_top ? stack_top->value : -1);
    return 0;
}


static void __exit stack_mod_exit(void)
{
    int value;

    pr_notice("stack_mod: popping all elements (LIFO order):\n");

    while (stack_top) {
        if (pop(&value) == 0)
            pr_notice("stack_mod:   popped %d\n", value);
    }

    pr_notice("stack_mod: stack is now empty\n");

    if (stack_node_cachep)
        kmem_cache_destroy(stack_node_cachep);

    pr_notice("stack_mod: slab cache destroyed, module unloaded\n");
}


module_init(stack_mod_init);
module_exit(stack_mod_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("M&M");
MODULE_DESCRIPTION("Ex02");
MODULE_VERSION("1.0");