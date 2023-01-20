/* LOADABLE KERNEL MODULE TEMPLATE */

#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/module.h>

/* function to be called when loading the module */
int init_testmod(void)
{
	printk(KERN_INFO "Loading Kernel Module - test\n");
	return 0;
}

/* function to be called when removing the module */
void exit_testmod(void)
{
	printk(KERN_INFO "Removing Kernel Module - test\n");
	return ;
}

/* macros to register the module entry and exit points with the kernel */
module_init(init_testmod);
module_exit(exit_testmod);

/* including details is standard practice in developing kernel module */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("AV");
