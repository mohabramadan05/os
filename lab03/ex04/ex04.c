#include <linux/module.h>
#include <linux/kfifo.h>
#include <linux/slab.h>
#include <linux/string.h>

#define FIFO_SIZE   4096 
#define MAX_STRINGS 16
#define MAX_STR_LEN 256

static char *strings[MAX_STRINGS];
static int   strings_count = 0;
module_param_array(strings, charp, &strings_count, 0444);
MODULE_PARM_DESC(strings, "Comma-separated strings to enqueue");

static struct kfifo str_fifo;

static int enqueue_string(const char *s)
{
    u32 len = (u32)strlen(s);
    unsigned int written;

    written = kfifo_in(&str_fifo, &len, sizeof(len));
    if (written != sizeof(len)) {
        pr_err("fifostr: failed to write length header\n");
        return -EIO;
    }
    written = kfifo_in(&str_fifo, s, len);
    if (written != len) {
        pr_err("fifostr: failed to write string body\n");
        return -EIO;
    }
    pr_info("ENQUEUE [%u bytes]: \"%s\"\n", len, s);
    return 0;
}

static int dequeue_string(char *buf, size_t buf_size)
{
    u32 len;
    unsigned int got;

    if (kfifo_is_empty(&str_fifo))
        return -ENODATA;

    got = kfifo_out(&str_fifo, &len, sizeof(len));
    if (got != sizeof(len))
        return -EIO;

    if (len >= buf_size) {
        pr_err("fifostr: buffer too small (%u needed)\n", len + 1);
        return -ENOSPC;
    }
    got = kfifo_out(&str_fifo, buf, len);
    if (got != len)
        return -EIO;
    buf[len] = '\0';
    return (int)len;
}

static int __init fifostrmod_init(void)
{
    int i, ret;
    char buf[MAX_STR_LEN + 1];

    pr_info("FIFO String Module Loaded\n");

    ret = kfifo_alloc(&str_fifo, FIFO_SIZE, GFP_KERNEL);
    if (ret) {
        pr_err("fifostr: kfifo_alloc failed\n");
        return ret;
    }

    if (strings_count == 0) {
        const char *demo[] = {
            "Hello", "Linux", "Kernel", "FIFO", "Exercise 4"
        };
        int n = ARRAY_SIZE(demo);
        pr_info("(no parameters given – using built-in demo strings)\n");
        for (i = 0; i < n; i++)
            enqueue_string(demo[i]);
    } else {
        for (i = 0; i < strings_count; i++)
            enqueue_string(strings[i]);
    }

    pr_info("Queue used: %u bytes / %u bytes total\n",
            kfifo_len(&str_fifo), kfifo_size(&str_fifo));

    /* Peek at first element without removing it */
    {
        u32 first_len;
        if (kfifo_out_peek(&str_fifo, &first_len, sizeof(first_len))
                == sizeof(first_len))
            pr_info("Peek: first string length = %u bytes\n", first_len);
    }

    return 0;
}

static void __exit fifostrmod_exit(void)
{
    char buf[MAX_STR_LEN + 1];
    int ret;
    int count = 0;

    pr_info("Dequeuing all strings (FIFO order)\n");
    while (!kfifo_is_empty(&str_fifo)) {
        ret = dequeue_string(buf, sizeof(buf));
        if (ret < 0) {
            pr_err("fifostr: dequeue error %d\n", ret);
            break;
        }
        pr_info("DEQUEUE [%d bytes]: \"%s\"\n", ret, buf);
        count++;
    }
    pr_info("Total strings dequeued: %d\n", count);

    kfifo_free(&str_fifo);
    pr_info("=== FIFO String Module Unloaded ===\n");
}

module_init(fifostrmod_init);
module_exit(fifostrmod_exit);

MODULE_AUTHOR("M&M");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ex04");
MODULE_VERSION("1.0");