#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>   
#include <linux/sched/mm.h>      
#include <linux/mm.h>             
#include <linux/fdtable.h>       
#include <linux/rcupdate.h>       
#include <linux/fs.h>           
#include <linux/utsname.h>  


static char *target_name = "userspace_app";
module_param(target_name, charp, 0444);
MODULE_PARM_DESC(target_name, "Name of the user-space process to inspect (default: userspace_app)");

static const char *decode_state(unsigned int state)
{
    if (state == TASK_RUNNING)        return "TASK_RUNNING";
    if (state & TASK_INTERRUPTIBLE)   return "TASK_INTERRUPTIBLE";
    if (state & TASK_UNINTERRUPTIBLE) return "TASK_UNINTERRUPTIBLE";
    if (state & __TASK_STOPPED)       return "TASK_STOPPED";
    if (state & __TASK_TRACED)        return "TASK_TRACED";
    if (state & TASK_DEAD)            return "TASK_DEAD / EXIT_ZOMBIE";
    return "UNKNOWN";
}

static void print_task_info(struct task_struct *t)
{
    struct mm_struct *mm;

    pr_info("Process descriptor info for: %-14s\n", t->comm);
    /* --- Identity --- */
    pr_info("comm (name)   : %s\n",        t->comm);
    pr_info("PID           : %d\n",         task_pid_nr(t));
    pr_info("TGID          : %d\n",         task_tgid_nr(t));
    pr_info("PPID          : %d\n",         task_ppid_nr(t));

    /* --- Credentials --- */
    pr_info("UID (real)    : %u\n",
            from_kuid(&init_user_ns, t->cred->uid));
    pr_info("GID (real)    : %u\n",
            from_kgid(&init_user_ns, t->cred->gid));
    pr_info("EUID          : %u\n",
            from_kuid(&init_user_ns, t->cred->euid));

    /* --- State --- */
    pr_info("State (raw)   : %u  (%s)\n",
            (unsigned int)t->state, decode_state(t->state));

    /* --- Scheduling --- */
    pr_info("Policy        : %u  (%s)\n",  t->policy,
            t->policy == SCHED_NORMAL  ? "SCHED_NORMAL"  :
            t->policy == SCHED_FIFO    ? "SCHED_FIFO"    :
            t->policy == SCHED_RR      ? "SCHED_RR"      :
            t->policy == SCHED_BATCH   ? "SCHED_BATCH"   :
            t->policy == SCHED_IDLE    ? "SCHED_IDLE"    : "OTHER");
    pr_info("Static prio   : %d\n",        t->static_prio);
    pr_info("Normal prio   : %d\n",        t->normal_prio);
    pr_info("RT priority   : %u\n",        t->rt_priority);
    pr_info("On CPU        : %d\n",        task_cpu(t));

    /* --- Timing --- */
    pr_info("utime (ns)    : %llu\n",
            (unsigned long long)t->utime);
    pr_info("stime (ns)    : %llu\n",
            (unsigned long long)t->stime);
    pr_info("start_time(ns): %llu\n",
            (unsigned long long)t->start_time);

    /* --- Signals --- */
    pr_info("Pending sigs  : %lu\n",
            t->pending.signal.sig[0]);

    /* --- Memory (requires get_task_mm to be safe) --- */
    mm = get_task_mm(t);
    if (mm) {
        pr_info("Total VM pages: %lu  (%lu kB)\n",
                mm->total_vm,
                mm->total_vm << (PAGE_SHIFT - 10));
        pr_info("RSS (pages)   : %ld\n",
                get_mm_rss(mm));
        pr_info("Code  [%016lx – %016lx]\n",
                mm->start_code, mm->end_code);
        pr_info("Data  [%016lx – %016lx]\n",
                mm->start_data, mm->end_data);
        pr_info("Stack start   : %016lx\n",
                mm->start_stack);
        mmput(mm);
    } else {
        pr_info("Memory map    : (kernel thread or already exited)\n");
    }

    /* --- Open file count (approximate) --- */
    {
        struct files_struct *files = t->files;
        if (files) {
            struct fdtable *fdt;
            rcu_read_lock();
            fdt = rcu_dereference(files->fdt);
            pr_info("Max open fds  : %u\n", fdt->max_fds);
            rcu_read_unlock();
        }
    }

    /* --- Flags (selected bits) --- */
    pr_info("Flags         : 0x%08x\n",   t->flags);
    pr_info("   PF_KTHREAD  : %s\n",
            (t->flags & PF_KTHREAD) ? "yes" : "no");
    pr_info("   PF_EXITING  : %s\n",
            (t->flags & PF_EXITING) ? "yes" : "no");
}

static int __init procinfo_init(void)
{
    struct task_struct *p;
    int found = 0;

    pr_info("procinfo: searching for process named \"%s\"...\n", target_name);

    rcu_read_lock();
    for_each_process(p) {
        if (strncmp(p->comm, target_name, TASK_COMM_LEN) == 0) {
            found++;
            print_task_info(p);
        }
    }
    rcu_read_unlock();

    if (!found)
        pr_warn("procinfo: no process named \"%s\" found. "
                "Is the user-space app running?\n", target_name);
    else
        pr_info("procinfo: found %d matching process(es).\n", found);

    return 0;
}

static void __exit procinfo_exit(void)
{
    pr_info("procinfo: module removed.\n");
}

module_init(procinfo_init);
module_exit(procinfo_exit);

MODULE_AUTHOR("M&M");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ex01");
MODULE_VERSION("1.0");