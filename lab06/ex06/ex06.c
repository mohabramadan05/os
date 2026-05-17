#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>     
#include <linux/interrupt.h>   
#include <linux/slab.h>        
#include <linux/proc_fs.h>     
#include <linux/seq_file.h>    
#include <linux/atomic.h>      
#include <linux/string.h>      

#define NORMAL_COUNT  3
#define HI_COUNT      2
#define PROC_NAME     "tasklet_stats"

struct tasklet_stats {
    atomic_t created;    // objects allocated from the slab cache     
    atomic_t scheduled;  // times tasklet_schedule/hi_schedule called 
    atomic_t executed;   // times the handler actually ran            
    atomic_t waiting;    // = scheduled - executed  (live counter)    
};

static struct tasklet_stats normal_stats;
static struct tasklet_stats hi_stats;

struct tasklet_entry {
    struct tasklet_struct ts;  // first becuase handler gets *ts cast  
    unsigned int id;           
    int is_hi;                 
};

static struct kmem_cache *tasklet_cachep;
static struct tasklet_entry *normal_entries[NORMAL_COUNT];
static struct tasklet_entry *hi_entries[HI_COUNT];

static void tasklet_entry_ctor(void *arg)
{
    memset(arg, 0, sizeof(struct tasklet_entry));
}

static void normal_tasklet_handler(unsigned long data)
{
    struct tasklet_entry *entry = (struct tasklet_entry *)data;

    atomic_inc(&normal_stats.executed);
    atomic_dec(&normal_stats.waiting);

    pr_notice("tasklet_stats_mod: NORMAL tasklet #%u executed\n", entry->id);
}

static void hi_tasklet_handler(unsigned long data)
{
    struct tasklet_entry *entry = (struct tasklet_entry *)data;

    atomic_inc(&hi_stats.executed);
    atomic_dec(&hi_stats.waiting);

    pr_notice("tasklet_stats_mod: HIGH tasklet #%u executed\n", entry->id);
}

static struct tasklet_entry *alloc_and_schedule(unsigned int id, int is_hi)
{
    struct tasklet_entry *entry;
    struct tasklet_stats *stats    = is_hi ? &hi_stats : &normal_stats;
    void (*handler)(unsigned long) = is_hi ? hi_tasklet_handler
                                           : normal_tasklet_handler;

    entry = (struct tasklet_entry *)kmem_cache_alloc(tasklet_cachep,
                                                      GFP_KERNEL);
    if (!entry) {
        pr_alert("tasklet_stats_mod: kmem_cache_alloc failed\n");
        return ERR_PTR(-ENOMEM);
    }

    entry->id    = id;
    entry->is_hi = is_hi;

    tasklet_init(&entry->ts, handler, (unsigned long)entry);
    atomic_inc(&stats->created);

    if (is_hi)
        tasklet_hi_schedule(&entry->ts);
    else
        tasklet_schedule(&entry->ts);

    atomic_inc(&stats->scheduled);
    atomic_inc(&stats->waiting);

    return entry;
}


static int tasklet_stats_show(struct seq_file *sf, void *v)
{
    seq_printf(sf, "%-20s %10s %10s %10s %10s\n",
               "Priority", "Created", "Scheduled", "Executed", "Waiting");

    seq_printf(sf, "%-20s %10d %10d %10d %10d\n",
               "NORMAL",
               atomic_read(&normal_stats.created),
               atomic_read(&normal_stats.scheduled),
               atomic_read(&normal_stats.executed),
               atomic_read(&normal_stats.waiting));

    seq_printf(sf, "%-20s %10d %10d %10d %10d\n",
               "HIGH",
               atomic_read(&hi_stats.created),
               atomic_read(&hi_stats.scheduled),
               atomic_read(&hi_stats.executed),
               atomic_read(&hi_stats.waiting));
    return 0;
}

static int tasklet_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, tasklet_stats_show, NULL);
}


static const struct file_operations tasklet_stats_fops = {
    .owner   = THIS_MODULE,
    .open    = tasklet_stats_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static struct proc_dir_entry *proc_entry;

static int __init tasklet_stats_mod_init(void)
{
    int i;
    struct tasklet_entry *entry;

    atomic_set(&normal_stats.created,   0);
    atomic_set(&normal_stats.scheduled, 0);
    atomic_set(&normal_stats.executed,  0);
    atomic_set(&normal_stats.waiting,   0);

    atomic_set(&hi_stats.created,   0);
    atomic_set(&hi_stats.scheduled, 0);
    atomic_set(&hi_stats.executed,  0);
    atomic_set(&hi_stats.waiting,   0);

    // create the slab cache
    tasklet_cachep = kmem_cache_create(
        "tasklet_entry_cache",
        sizeof(struct tasklet_entry),
        0,
        SLAB_HWCACHE_ALIGN | SLAB_POISON | SLAB_RED_ZONE,
        tasklet_entry_ctor);

    if (IS_ERR(tasklet_cachep)) {
        pr_alert("tasklet_stats_mod: kmem_cache_create failed: %ld\n",
                 PTR_ERR(tasklet_cachep));
        return -ENOMEM;
    }
    pr_notice("tasklet_stats_mod: slab cache created\n");

    proc_entry = proc_create(PROC_NAME, 0444, NULL, &tasklet_stats_fops);
    if (!proc_entry) {
        pr_alert("tasklet_stats_mod: proc_create() failed\n");
        kmem_cache_destroy(tasklet_cachep);
        return -ENOMEM;
    }
    pr_notice("tasklet_stats_mod: /proc/%s created\n", PROC_NAME);

    for (i = 0; i < NORMAL_COUNT; i++) {
        entry = alloc_and_schedule(i, 0 /* normal */);
        if (IS_ERR(entry)) {
            int j;
            for (j = 0; j < i; j++) {
                tasklet_kill(&normal_entries[j]->ts);
                kmem_cache_free(tasklet_cachep, normal_entries[j]);
            }
            proc_remove(proc_entry);
            kmem_cache_destroy(tasklet_cachep);
            return PTR_ERR(entry);
        }
        normal_entries[i] = entry;
        pr_notice("tasklet_stats_mod: scheduled NORMAL #%d\n", i);
    }

    for (i = 0; i < HI_COUNT; i++) {
        entry = alloc_and_schedule(i, 1 /* high */);
        if (IS_ERR(entry)) {
            int j;
            for (j = 0; j < NORMAL_COUNT; j++) {
                tasklet_kill(&normal_entries[j]->ts);
                kmem_cache_free(tasklet_cachep, normal_entries[j]);
            }
            for (j = 0; j < i; j++) {
                tasklet_kill(&hi_entries[j]->ts);
                kmem_cache_free(tasklet_cachep, hi_entries[j]);
            }
            proc_remove(proc_entry);
            kmem_cache_destroy(tasklet_cachep);
            return PTR_ERR(entry);
        }
        hi_entries[i] = entry;
        pr_notice("tasklet_stats_mod: scheduled HIGH #%d\n", i);
    }

    pr_notice("tasklet_stats_mod: loaded – cat /proc/%s for stats\n",
              PROC_NAME);
    return 0;
}

static void __exit tasklet_stats_mod_exit(void)
{
    int i;

    pr_notice("tasklet_stats_mod: unloading...\n");

    for (i = 0; i < NORMAL_COUNT; i++) {
        tasklet_kill(&normal_entries[i]->ts);
        kmem_cache_free(tasklet_cachep, normal_entries[i]);
    }
    for (i = 0; i < HI_COUNT; i++) {
        tasklet_kill(&hi_entries[i]->ts);
        kmem_cache_free(tasklet_cachep, hi_entries[i]);
    }

    proc_remove(proc_entry);
    pr_notice("tasklet_stats_mod: /proc/%s removed\n", PROC_NAME);

    kmem_cache_destroy(tasklet_cachep);
    pr_notice("tasklet_stats_mod: slab cache destroyed\n");

    pr_notice("tasklet_stats_mod: final stats:\n");
    pr_notice("  NORMAL – created=%d scheduled=%d executed=%d waiting=%d\n",
              atomic_read(&normal_stats.created),
              atomic_read(&normal_stats.scheduled),
              atomic_read(&normal_stats.executed),
              atomic_read(&normal_stats.waiting));
    pr_notice("  HIGH   – created=%d scheduled=%d executed=%d waiting=%d\n",
              atomic_read(&hi_stats.created),
              atomic_read(&hi_stats.scheduled),
              atomic_read(&hi_stats.executed),
              atomic_read(&hi_stats.waiting));
}

module_init(tasklet_stats_mod_init);
module_exit(tasklet_stats_mod_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("M&M");
MODULE_DESCRIPTION("Ex06");
MODULE_VERSION("1.0");