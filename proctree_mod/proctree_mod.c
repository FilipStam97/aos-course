#include <linux/init.h>          
#include <linux/module.h>        
#include <linux/kernel.h>        
#include <linux/sched.h>        
#include <linux/sched/signal.h>  
#include <linux/mm.h>         
#include <linux/cred.h>         
#include <linux/slab.h>         
#include <linux/rcupdate.h>     
#include <linux/uidgid.h>    

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Filip");
MODULE_DESCRIPTION("Kernel module: DFS process tree for given PID");

static int pid = 1;
module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "Root PID for the DFS process tree");

static void dfs_proc_tree(struct task_struct *task, int level)
{
    struct task_struct *child;
    struct list_head *list;
    int i;
    char indent[64];

    //printing limit
    if (level < 0)
        level = 0;
    if (level >= (int)sizeof(indent))
        level = sizeof(indent) - 1;

    //for printing
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

    //time of execution
    u64 runtime_ms = div_u64(task->se.sum_exec_runtime, 1000000);

    //dmesg | tail -n 50
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


static int proctree_init(void)
{
    struct task_struct *task;

    printk(KERN_INFO "proctree_mod -> loaded, starting DFS from PID=%d\n", pid);

    //lock rcu
    rcu_read_lock();

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        printk(KERN_INFO "proctree_mod -> PID %d not found\n", pid);
        rcu_read_unlock();
        return -ESRCH;
    }


    dfs_proc_tree(task, 0);

    rcu_read_unlock();

    return 0;
}



static void  proctree_exit(void)
{
    printk(KERN_INFO "proctree_mod: unloaded\n");
}


module_init(proctree_init);
module_exit(proctree_exit);
