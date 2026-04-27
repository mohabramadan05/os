#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/list.h>

// node that stores one char
struct char_node {
    char ch;
    struct list_head list;
};

// head of the doubly linked list
static LIST_HEAD(file_list);

// procfs entries
static struct proc_dir_entry *directory_entry_pointer, *file_entry_pointer;
static char *directory_name = "procfs_test";
static char *file_name      = "procfs_file";

static void free_list(void)
{
    struct char_node *node, *tmp;

    list_for_each_entry_safe(node, tmp, &file_list, list) {
        list_del(&node->list);
        kfree(node);
    }
}

static ssize_t procfsmod_read(struct file *file,
                              char __user *user_buf,
                              size_t count,
                              loff_t *pos)
{
    char *temp;
    int i = 0;
    struct char_node *node;

    if (*pos > 0)
        return 0;

    temp = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!temp)
        return -ENOMEM;

    list_for_each_entry(node, &file_list, list) {
        if (i >= PAGE_SIZE - 1)
            break;

        temp[i++] = node->ch;
    }

    temp[i] = '\0';

    if (copy_to_user(user_buf, temp, i)) {
        kfree(temp);
        return -EFAULT;
    }

    *pos = i;

    kfree(temp);
    return i;
}

static ssize_t procfsmod_write(struct file *file,
                               const char __user *user_buf,
                               size_t count,
                               loff_t *pos)
{
    size_t length = count;
    size_t i;
    char *tmp_buf;
    struct char_node *node;

    if (length > PAGE_SIZE - 1)
        length = PAGE_SIZE - 1;

    // allocate temporary buffer
    tmp_buf = kmalloc(length + 1, GFP_KERNEL);
    if (!tmp_buf)
        return -ENOMEM;

    // copy from user
    if (copy_from_user(tmp_buf, user_buf, length)) {
        kfree(tmp_buf);
        return -EFAULT;
    }

    tmp_buf[length] = '\0';

    // remove old data
    free_list();

    // create node for each character
    for (i = 0; i < length && tmp_buf[i] != '\0'; i++) {

        node = kmalloc(sizeof(struct char_node), GFP_KERNEL);
        if (!node) {
            kfree(tmp_buf);
            return i ? (ssize_t)i : -ENOMEM;
        }

        node->ch = tmp_buf[i];
        INIT_LIST_HEAD(&node->list);

        // add to doubly linked list (tail)
        list_add_tail(&node->list, &file_list);
    }

    kfree(tmp_buf);

    return length;
}

static const struct file_operations procfsmod_fops = {
    .owner  = THIS_MODULE,
    .read   = procfsmod_read,
    .write  = procfsmod_write,
    .llseek = default_llseek,
};

static int __init procfsmod_init(void)
{
    directory_entry_pointer = proc_mkdir(directory_name, NULL);
    if (!directory_entry_pointer) {
        pr_alert("Error creating directory\n");
        return -ENOMEM;
    }

    file_entry_pointer = proc_create(file_name, 0666,
                                     directory_entry_pointer,
                                     &procfsmod_fops);
    if (!file_entry_pointer) {
        pr_alert("Error creating file\n");
        proc_remove(directory_entry_pointer);
        return -ENOMEM;
    }

    pr_info("File created\n");

    return 0;
}

static void __exit procfsmod_exit(void)
{
    if (file_entry_pointer)
        proc_remove(file_entry_pointer);

    if (directory_entry_pointer)
        proc_remove(directory_entry_pointer);

    // free memory
    free_list();

    pr_info("Module unloaded\n");
}



module_init(procfsmod_init);
module_exit(procfsmod_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("M&M");
MODULE_DESCRIPTION("ex03");
MODULE_VERSION("1.0");