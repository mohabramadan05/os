#include <linux/module.h>   
#include <linux/uaccess.h>  // For copy_to_user / copy_from_user
#include <linux/proc_fs.h>  // For proc filesystem

// pointers for directory and file in /proc
static struct proc_dir_entry *directory_entry_pointer, *file_entry_pointer;

// names of directory and file
static char *directory_name = "procfs_test";
static char *file_name     = "procfs_file";

// buffer to store data written by user
static char   file_buffer[PAGE_SIZE];
static size_t file_data_len = 0; // length of stored data

// called when user runs "cat"
static ssize_t procfsmod_read(struct file *file,
                              char __user *user_buf,
                              size_t count,
                              loff_t *position)
{
    ssize_t bytes_to_copy;

    // file ended 
    if (*position >= file_data_len)
        return 0;

    // remaining data
    bytes_to_copy = file_data_len - *position;

    // copy the requested size
    if (bytes_to_copy > count)
        bytes_to_copy = count;

    // copy data from kernel buffer to user space
    if (copy_to_user(user_buf, file_buffer + *position, bytes_to_copy))
        return -EFAULT;

    // update read position
    *position += bytes_to_copy;

    return bytes_to_copy; // return number of bytes read
}

// called when user uses "echo"
static ssize_t procfsmod_write(struct file *file,
                               const char __user *buffer,
                               size_t count,
                               loff_t *position)
{
    size_t length = count;

    // leave space for '\0'
    if (count > PAGE_SIZE)
        length = PAGE_SIZE - 1;

    // copy data from user to kernel buffer
    if (copy_from_user(file_buffer, buffer, length))
        return -EFAULT;

    // end of string
    file_buffer[length] = '\0';

    // save length of data
    file_data_len = length;

    return length; // return number of bytes written
}

// file operations (connect read/write functions)
static const struct file_operations procfsmod_fops = {
    .owner   = THIS_MODULE,
    .read    = procfsmod_read,
    .write   = procfsmod_write,
    .llseek  = default_llseek,
};

static int __init procfsmod_init(void)
{
    // create /proc/procfs_test
    directory_entry_pointer = proc_mkdir(directory_name, NULL);

    if (IS_ERR(directory_entry_pointer)) {
        pr_alert("Error creating directory\n");
        return -1;
    }

    // create /proc/procfs_test/procfs_file
    file_entry_pointer = proc_create_data(file_name, 0666,
                                          directory_entry_pointer,
                                          &procfsmod_fops,
                                          (void *)file_buffer);

    if (IS_ERR(file_entry_pointer)) {
        pr_alert("Error creating file\n");
        proc_remove(directory_entry_pointer);
        return -1;
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

    pr_info("File removed\n");
}

module_init(procfsmod_init);
module_exit(procfsmod_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("M&M");
MODULE_DESCRIPTION("Ex02");
MODULE_VERSION("1.0");