#include<linux/sched.h>
#include<linux/kernel.h>
#include<linux/syscalls.h>

#define MAX_ANCESTORS 16384 

SYSCALL_DEFINE1(ancestry, int, pid)
{
	int retval;
	int idx, ord;
	int *ancestors = NULL;
	struct task_struct *this_task = NULL;
	retval = -1;

	printk(KERN_INFO "ancestry: invoked...\n");

	if (!(this_task = pid_task(find_vpid(pid), PIDTYPE_PID)))
		printk(KERN_INFO "ancestry: pid_task: "
				"non-existent process with pid %5d...\n", pid);
	else
	{
		idx = 0;
		ancestors = kmalloc(sizeof(int) * MAX_ANCESTORS, GFP_KERNEL); 

		if (!ancestors)
		{
			printk(KERN_INFO "ancestry: printing reverse order ancestry...\n");

			printk(KERN_INFO "ancestry: %5d\n", pid);
			while(pid && this_task)
			{
				rcu_read_lock();
				this_task = rcu_dereference(this_task->real_parent);
				pid = this_task->pid;
				rcu_read_unlock();

				printk(KERN_INFO "ancestry: %5d\n", pid);
			}

			printk(KERN_INFO "ancestry: printed ancestry successfully...\n");
		}
		else
		{
			ancestors[idx] = pid;
			while(pid && this_task)
			{
				rcu_read_lock();
				this_task = rcu_dereference(this_task->real_parent);
				pid = this_task->pid;
				rcu_read_unlock();

				ancestors[++idx] = pid;
			}

			printk(KERN_INFO "ancestry: printing ancestry from root...\n");

			for(ord = 0; idx >= 0; ord++, idx--)
				printk(KERN_INFO "ancestry: %5d)\t%5d\n", ord, ancestors[idx]);

			printk(KERN_INFO "ancestry: printed ancestry successfully...\n"); kfree(ancestors);
		}

		retval = 0;
	}

	printk(KERN_INFO "ancestry: returning...\n");
	return retval; 
}
