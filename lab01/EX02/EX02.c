#include <linux/module.h>
#include <linux/string.h>

static int __init EX02_init(void)
{
    char dest[64];
    char src[]  = "Hello";
    char cat[]  = ", kernel world!";
    char s1[] = "Linux";
    char s2[] = "Linux";
    char s3[] = "Unix";
    int  result;

    // Copy src to dest and return the number of chars copied and return -E2BIG if there is error
    result = strscpy(dest, src, sizeof(dest));
    printk(KERN_ALERT "strscpy: copied \"%s\", result=%d\n", dest, result);

    // add cat to dest and checks if the total lenght is less than the dest size and return the lenght after adding
    result = strlcat(dest, cat, sizeof(dest));
    printk(KERN_ALERT "strlcat: result \"%s\", result=%d\n", dest, result);

    // compare 2 strings  till the lenght parameters and return the differnece between them 
    result = strncmp(s1, s2, sizeof(s1));
    printk(KERN_ALERT "strncmp(\"%s\",\"%s\"): %d (expect 0)\n",
           s1, s2, result);

    result = strncmp(s1, s3, sizeof(s1));
    printk(KERN_ALERT "strncmp(\"%s\",\"%s\"): %d (expect <0)\n",
           s1, s3, result);

    return 0;
}

static void __exit EX02_exit(void)
{
    printk(KERN_ALERT "EX02: module removed\n");
}
// module properties 
module_init(EX02_init);
module_exit(EX02_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("M&M");
MODULE_DESCRIPTION("EX02");
MODULE_VERSION("1.0");