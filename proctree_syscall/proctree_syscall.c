#include <linux/kernel.h>      
#include <linux/syscalls.h>    
#include <linux/sched.h>        
#include <linux/sched/signal.h>  
#include <linux/mm.h>           
#include <linux/cred.h>        
#include <linux/uidgid.h>        
#include <linux/rcupdate.h>      


static void dfs_proc_tree(struct task_struct *task, int level)
{
    struct task_struct *child;
    struct list_head *list;
    int i;
    char indent[64];

	//print limit
    if (level < 0)
        level = 0;
    if (level >= (int)sizeof(indent))
        level = (int)sizeof(indent) - 1;

    for (i = 0; i < level; i++)
        indent[i] = ' ';
    indent[i] = '\0';

   
    int prio = task->prio;
    int nice = task_nice(task);

    //owner uid
    const struct cred *cred = get_task_cred(task);
    uid_t uid = from_kuid_munged(&init_user_ns, cred->uid);
    put_cred(cred);

    //memory
    unsigned long rss_kb = 0, virt_kb = 0;
    if (task->mm) {
        rss_kb  = (get_mm_rss(task->mm) << PAGE_SHIFT) >> 10;
        virt_kb = (task->mm->total_vm  << PAGE_SHIFT) >> 10;
    }

    //runtime in ms
    u64 runtime_ms = div_u64(task->se.sum_exec_runtime, 1000000);

    printk(KERN_INFO
           "%scomm=%s pid=%d prio=%d nice=%d uid=%u rss=%lukB virt=%lukB runtime=%llums\n",
           indent,
           task->comm,
           task->pid,
           prio,
           nice,
           uid,
           rss_kb,
           virt_kb,
           runtime_ms);

    
    list_for_each(list, &task->children) {
        child = list_entry(list, struct task_struct, sibling);
        dfs_proc_tree(child, level + 2);    
    }
}


SYSCALL_DEFINE1(proctree, pid_t, pid)
{
    struct task_struct *task;

    if (pid <= 0)
        return -EINVAL;

  
    rcu_read_lock();

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        rcu_read_unlock();
        return -ESRCH;
    }

    dfs_proc_tree(task, 0);

    rcu_read_unlock();

    return 0;
}

/*


*/