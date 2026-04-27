#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/sched.h>

static unsigned long bit_word = 0;

static struct task_struct *demo_thread;

static void print_word(const char *label)
{
    int i;
    char buf[65];
    buf[64] = '\0';

    for (i = 63; i >= 0; i--)
        buf[63 - i] = test_bit(i, &bit_word) ? '1' : '0';

    pr_info("atomic_bits %s | word = 0b%s (decimal: %lu)\n",
            label, buf, bit_word);
}

static int bit_demo_thread(void *data)
{
    int old_value;
    pr_info("atomic_bits start demo operations:\n");

    bit_word = 0;
    print_word("initial state all zeros");
    set_bit(0, &bit_word);      // (LSB)
    print_word("after set_bit(0)");
    set_bit(3, &bit_word);
    print_word("after set_bit(3)");
    set_bit(7, &bit_word);
    print_word("after set_bit(7)");
    set_bit(63, &bit_word);     // (MSB on 64-bit)
    print_word("after set_bit(63)");

    clear_bit(3, &bit_word);    // was 1 > 0
    print_word("after clear_bit(3)");
    clear_bit(5, &bit_word);    // already 0 — no effect
    print_word("after clear_bit(5) [was 0]");

    change_bit(0, &bit_word);   // 1 >  0
    print_word("after change_bit(0) [1->0]");
    change_bit(0, &bit_word);   // 0 >  1
    print_word("after change_bit(0) [0->1]");
    change_bit(10, &bit_word);  // 0 >  1
    print_word("after change_bit(10) [0->1]");

    old_value = test_and_set_bit(0, &bit_word);   // bit 0 was 1
    pr_info("atomic_bits test_and_set_bit(0): old value was %d\n", old_value);
    print_word("after test_and_set_bit(0)");

    old_value = test_and_set_bit(1, &bit_word);   // bit 1 was 0
    pr_info("atomic_bits test_and_set_bit(1): old value was %d\n", old_value);
    print_word("after test_and_set_bit(1) [0->1]");

    old_value = test_and_clear_bit(7, &bit_word);  // bit 7 was 1
    pr_info("atomic_bits test_and_clear_bit(7): old value was %d\n", old_value);
    print_word("after test_and_clear_bit(7) [1->0]");

    old_value = test_and_clear_bit(4, &bit_word);  // bit 4 was 0
    pr_info("atomic_bits test_and_clear_bit(4): old value was %d\n", old_value);
    print_word("after test_and_clear_bit(4) [0->0]");

    old_value = test_and_change_bit(63, &bit_word); // bit 63 was 1
    pr_info("atomic_bits test_and_change_bit(63): old value was %d\n", old_value);
    print_word("after test_and_change_bit(63)[1->0]");

    old_value = test_and_change_bit(63, &bit_word); // bit 63 is now 0
    pr_info("atomic_bits test_and_change_bit(63): old value was %d\n", old_value);
    print_word("after test_and_change_bit(63)[0->1]");

    pr_info("atomic_bits test_bit(0)  = %d (expected 1)\n",
            (int)test_bit(0,  &bit_word));
    pr_info("atomic_bits test_bit(4)  = %d (expected 0)\n",
            (int)test_bit(4,  &bit_word));
    pr_info("atomic_bits test_bit(63) = %d (expected 1)\n",
            (int)test_bit(63, &bit_word));

    print_word("final state");
    pr_info("atomic_bits end demo operations:\n");
    return 0;
}


static int __init atomic_bits_init(void)
{
    pr_info("atomic_bits module loaded\n");

    demo_thread = kthread_run(bit_demo_thread, NULL, "bit_demo_thread");
    if (IS_ERR(demo_thread)) {
        pr_alert("atomic_bits failed to create thread: %ld\n",
                 PTR_ERR(demo_thread));
        return PTR_ERR(demo_thread);
    }

    return 0;
}

static void __exit atomic_bits_exit(void)
{
    if (demo_thread)
        kthread_stop(demo_thread);

    pr_info("atomic_bits Module unloaded.\n");
}

module_init(atomic_bits_init);
module_exit(atomic_bits_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ex01");
MODULE_AUTHOR("M&M");
MODULE_VERSION("1.0");