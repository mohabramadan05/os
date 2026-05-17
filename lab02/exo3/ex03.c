#include <linux/module.h>
#include <linux/slab.h>

struct dlist_node {
    int value;
    struct dlist_node *prev;  
    struct dlist_node *next;   
};


static struct dlist_node *list_head = NULL;
static struct dlist_node *list_tail = NULL;

static struct kmem_cache *dlist_node_cachep;

static void dlist_node_constructor(void *arg)
{
    struct dlist_node *node = (struct dlist_node *)arg;
    node->value = 0;
    node->prev  = NULL;
    node->next  = NULL;
}

static int list_append(int value)
{
    struct dlist_node *node;

    node = (struct dlist_node *)kmem_cache_alloc(dlist_node_cachep,
                                                  GFP_KERNEL);
    if (IS_ERR(node)) {
        pr_alert("dlist_mod: kmem_cache_alloc failed: %ld\n",
                 PTR_ERR(node));
        return -ENOMEM;
    }

    node->value = value;
    node->prev  = list_tail;  // point back at current tail 
    node->next  = NULL;       // this will be the new tail 

    if (list_tail)
        list_tail->next = node;  // old tail now points forward      
    else
        list_head = node;        // first node: also becomes the head  

    list_tail = node;            // advance tail                       

    pr_notice("dlist_mod: appended %d\n", value);
    return 0;
}


static void list_delete_all(void)
{
    struct dlist_node *cur = list_head;
    struct dlist_node *nxt;

    while (cur) {
        nxt = cur->next;
        kmem_cache_free(dlist_node_cachep, cur);
        cur = nxt;
    }

    list_head = NULL;
    list_tail = NULL;
}


static int __init dlist_mod_init(void)
{
    int i, rc;
    int values[] = {11, 22, 33, 44, 55};
    int n = ARRAY_SIZE(values);

    dlist_node_cachep = kmem_cache_create(
        "dlist_node_cache",
        sizeof(struct dlist_node),
        0,
        SLAB_HWCACHE_ALIGN | SLAB_POISON | SLAB_RED_ZONE,
        dlist_node_constructor);

    if (IS_ERR(dlist_node_cachep)) {
        pr_alert("dlist_mod: kmem_cache_create failed: %ld\n",
                 PTR_ERR(dlist_node_cachep));
        return -ENOMEM;
    }
    pr_notice("dlist_mod: slab cache created\n");

    for (i = 0; i < n; i++) {
        rc = list_append(values[i]);
        if (rc) {
            list_delete_all();
            kmem_cache_destroy(dlist_node_cachep);
            return rc;
        }
    }

    pr_notice("dlist_mod: list built with %d nodes\n", n);
    return 0;
}



static void __exit dlist_mod_exit(void)
{
    struct dlist_node *cur;

    // head > tail
    pr_notice("dlist_mod: forward traversal (head -> tail):\n");
    cur = list_head;
    while (cur) {
        pr_notice("dlist_mod:   %d\n", cur->value);
        cur = cur->next;
    }

    // tail > head
    pr_notice("dlist_mod: backward traversal (tail -> head):\n");
    cur = list_tail;
    while (cur) {
        pr_notice("dlist_mod:   %d\n", cur->value);
        cur = cur->prev;
    }

    list_delete_all();
    pr_notice("dlist_mod: all nodes freed\n");

    if (dlist_node_cachep)
        kmem_cache_destroy(dlist_node_cachep);

    pr_notice("dlist_mod: slab cache destroyed, module unloaded\n");
}

module_init(dlist_mod_init);
module_exit(dlist_mod_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("M&M");
MODULE_DESCRIPTION("Ex03");
MODULE_VERSION("1.0");