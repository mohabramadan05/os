#include <linux/module.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/string.h>

#define MAX_STRINGS 16

static char *strings[MAX_STRINGS];
static int   strings_count = 0;
module_param_array(strings, charp, &strings_count, 0444);
MODULE_PARM_DESC(strings, "Comma-separated strings to store in the RB-tree");

struct str_node {
    int             key;    // integer key: insertion order
    char           *str;    // heap-allocated string         
    struct rb_node  node;   // embedded RB-tree linkage       
};

static struct rb_root root = RB_ROOT;

static bool rb_insert(struct rb_root *tree, struct str_node *new_node)
{
    struct rb_node **link = &tree->rb_node;
    struct rb_node  *parent = NULL;

    while (*link) {
        struct str_node *cur = rb_entry(*link, struct str_node, node);
        parent = *link;
        if (new_node->key < cur->key)
            link = &(*link)->rb_left;
        else if (new_node->key > cur->key)
            link = &(*link)->rb_right;
        else
            return false;   // duplicate key 
    }
    rb_link_node(&new_node->node, parent, link);
    rb_insert_color(&new_node->node, tree);
    return true;
}

static struct str_node *rb_find(struct rb_root *tree, int key)
{
    struct rb_node *n = tree->rb_node;
    while (n) {
        struct str_node *cur = rb_entry(n, struct str_node, node);
        if (key < cur->key)
            n = n->rb_left;
        else if (key > cur->key)
            n = n->rb_right;
        else
            return cur;
    }
    return NULL;
}

static void rb_free_all(struct rb_root *tree)
{
    struct rb_node *n;
    while ((n = rb_first(tree))) {
        struct str_node *cur = rb_entry(n, struct str_node, node);
        rb_erase(n, tree);
        kfree(cur->str);
        kfree(cur);
    }
}

static int __init rbstrmod_init(void)
{
    int i;
    const char *src_strings[MAX_STRINGS];
    int src_count;

    pr_info("RB-Tree String Module Loaded\n");

    if (strings_count == 0) {
        const char *demo[] = {
            "Hello", "Linux", "Kernel", "Red-Black", "Tree", "Exercise 5"
        };
        src_count = ARRAY_SIZE(demo);
        for (i = 0; i < src_count; i++)
            src_strings[i] = demo[i];
        pr_info("(no parameters given – using built-in demo strings)\n");
    } else {
        src_count = strings_count;
        for (i = 0; i < src_count; i++)
            src_strings[i] = strings[i];
    }

    // Insert strings into the RB-tree with keys 0, 1, 2, … 
    for (i = 0; i < src_count; i++) {
        struct str_node *sn = kmalloc(sizeof(*sn), GFP_KERNEL);
        if (!sn)
            return -ENOMEM;
        sn->key = i;
        sn->str = kstrdup(src_strings[i], GFP_KERNEL);
        if (!sn->str) {
            kfree(sn);
            return -ENOMEM;
        }
        if (!rb_insert(&root, sn)) {
            pr_err("rbstr: duplicate key %d – this should not happen\n", i);
            kfree(sn->str);
            kfree(sn);
        } else {
            pr_info("INSERT key=%d : \"%s\" (len=%zu)\n",
                    i, sn->str, strlen(sn->str));
        }
    }

    pr_info("In-order traversal\n");
    {
        struct rb_node *n;
        for (n = rb_first(&root); n; n = rb_next(n)) {
            struct str_node *sn = rb_entry(n, struct str_node, node);
            pr_info("key=%d val=\"%s\" len=%zu\n",
                    sn->key, sn->str, strlen(sn->str));
        }
    }

    pr_info("Lookup key=2\n");
    {
        struct str_node *found = rb_find(&root, 2);
        if (found)
            pr_info("Found: key=%d val=\"%s\"\n", found->key, found->str);
        else
            pr_info("Key 2 not found\n");
    }

    return 0;
}

static void __exit rbstrmod_exit(void)
{
    pr_info("--- Removing all nodes ---\n");
    rb_free_all(&root);
    pr_info("=== RB-Tree String Module Unloaded ===\n");
}

module_init(rbstrmod_init);
module_exit(rbstrmod_exit);

MODULE_AUTHOR("M&M");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ex05");
MODULE_VERSION("1.0");