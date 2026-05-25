#include <linux/module.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/random.h>

struct example_struct {
    int            data; 
    struct rb_node node;
};

static struct rb_root root = RB_ROOT;

static struct example_struct *find_node(struct rb_root *r, int number)
{
    struct rb_node *n = r->rb_node;
    while (n) {
        struct example_struct *cur = rb_entry(n, struct example_struct, node);
        if (number < cur->data)      n = n->rb_left;
        else if (number > cur->data) n = n->rb_right;
        else                         return cur;
    }
    return NULL;
}

static bool insert_node(struct rb_root *r, struct example_struct *new_node)
{
    struct rb_node **link   = &r->rb_node;
    struct rb_node  *parent = NULL;

    while (*link) {
        struct example_struct *cur = rb_entry(*link, struct example_struct, node);
        parent = *link;
        if (new_node->data < cur->data)      link = &(*link)->rb_left;
        else if (new_node->data > cur->data) link = &(*link)->rb_right;
        else return false;   /* key already present */
    }
    rb_link_node(&new_node->node, parent, link);
    rb_insert_color(&new_node->node, r);
    return true;
}

static struct example_struct *make_node(int value)
{
    struct example_struct *n =
        kmalloc(sizeof(struct example_struct), GFP_KERNEL);
    if (!n) return NULL;
    n->data = value;
    return n;
}

static void print_tree_ascending(const char *label)
{
    struct rb_node *n;
    pr_info("--- %s (non-descending) ---\n", label);
    for (n = rb_first(&root); n; n = rb_next(n)) {
        struct example_struct *e = rb_entry(n, struct example_struct, node);
        pr_info("  %d\n", e->data);
    }
}

static void print_tree_descending(const char *label)
{
    struct rb_node *n;
    pr_info("--- %s (non-ascending) ---\n", label);
    for (n = rb_last(&root); n; n = rb_prev(n)) {
        struct example_struct *e = rb_entry(n, struct example_struct, node);
        pr_info("  %d\n", e->data);
    }
}

static int __init rbtreemod_init(void)
{
    int i;
    struct example_struct *node;

    pr_info("Extended RB-Tree Module Loaded\n");

    pr_info("Inserting fixed values 0-9...\n");
    for (i = 0; i < 10; i++) {
        node = make_node(i);
        if (!node) return -ENOMEM;
        if (!insert_node(&root, node)) {
            pr_info("  Skipped duplicate key %d\n", i);
            kfree(node);
        } else {
            pr_info("  Inserted %d\n", i);
        }
    }

    pr_info("Inserting 20 random values in range [0,49]...\n");
    for (i = 0; i < 20; i++) {
        u8 rnd;
        get_random_bytes(&rnd, sizeof(rnd));
        rnd %= 50;
        node = make_node((int)rnd);
        if (!node) return -ENOMEM;
        if (!insert_node(&root, node)) {
            pr_info("  Skipped duplicate random key %d\n", (int)rnd);
            kfree(node);
        } else {
            pr_info("  Inserted random %d\n", (int)rnd);
        }
    }

    print_tree_ascending("All values after insertions");

    print_tree_descending("Same values reversed");


    pr_info("Demonstrating rb_replace_node() on key=5\n");
    {
        struct example_struct *old_node = find_node(&root, 5);
        if (!old_node) {
            pr_info("  Key 5 not in tree – skipping replacement demo\n");
        } else {
            struct example_struct *new_5 = make_node(5);
            if (!new_5) return -ENOMEM;

            pr_info("  Old node address: %px  data=%d\n",
                    old_node, old_node->data);

            rb_replace_node(&old_node->node, &new_5->node, &root);
            kfree(old_node);   // old node is now detached – free it 

            pr_info("  New node address: %px  data=%d\n",
                    new_5, new_5->data);
            pr_info("  Tree order intact? Lookup key=5: %s\n",
                    find_node(&root, 5) ? "YES" : "NO");
        }
    }

    print_tree_ascending("Final tree after rb_replace_node");

    return 0;
}

static void __exit rbtreemod_exit(void)
{
    struct rb_node *n;

    pr_info("--- Removing all nodes ---\n");
    // rb_first always returns the leftmost node; erase it and repeat 
    while ((n = rb_first(&root))) {
        struct example_struct *e = rb_entry(n, struct example_struct, node);
        pr_info("  Erasing %d\n", e->data);
        rb_erase(n, &root);
        kfree(e);
    }
    pr_info("=== Extended RB-Tree Module Unloaded ===\n");
}

module_init(rbtreemod_init);
module_exit(rbtreemod_exit);

MODULE_AUTHOR("M&M");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ex08");
MODULE_VERSION("1.0");