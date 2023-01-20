/* LOADABLE KERNEL MODULE TEMPLATE */
/*
 * creating additional entries in /proc fs
 * /proc is the pseudo file system in kernel mem.
 */
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/proc_fs.h>
#include<asm/uaccess.h>

#define BUF_SIZE 512
#define PROC_NAME "truth"

ssize_t proc_read(struct file *file, char __user *usr_buf,
		size_t count, loff_t *pos);
/*
static struct file_operations file_ops = {
	.owner = THIS_MODULE,
	.read = proc_read,
};
*/
static struct proc_ops proc_opsx = {
	.proc_read = proc_read,
};
/* function to be called when loading the module */
int proc_init(void)
{
	/* create /proc/PROC_NAME entry */
	proc_create(PROC_NAME, 0666, NULL, &proc_opsx);

	return 0;
}

/* function to be called when removing the module */
void proc_exit(void)
{
	/* remove /proc/PROC_NAME entry */
	remove_proc_entry(PROC_NAME, NULL);

	return ;
}

/* function to be called when /proc/PROC_NAME is read by user */
ssize_t proc_read(struct file *file, char __user *usr_buf,
		size_t count, loff_t *pos)
{
	int ret_val;
	char buf[BUF_SIZE];
	static int completed = 0;

	ret_val = 0;
	
	if(completed)
	{
		completed = 0;
		return 0;
	}
	completed = 1;

	ret_val = sprintf(buf, "knowledge is truth, ignorance is death!\n");
	
	/* copy kernel space buffer to user space buffer */
	copy_to_user(usr_buf, buf, ret_val);

	return ret_val;
}

/* macros to register the module entry and exit points with the kernel */
module_init(proc_init);
module_exit(proc_exit);

/* including details is standard practice in developing kernel module */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("AV");
