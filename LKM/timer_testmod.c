/* LOADABLE KERNEL MODULE TEMPLATE */

#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<asm/param.h>
#include<linux/jiffies.h>

static int hertz = 0;
static unsigned long jstart = 0;
static unsigned long jend = 0;
static unsigned long jlu = 0;

/* function to be called when loading the module */
int init_testmod(void)
{
	hertz = HZ;
	jlu = jiffies;
	jstart = jlu;

	printk(KERN_INFO "Loading Kernel Module : timer_testmod\n");

	printk(KERN_INFO "HZ            = %10d \t(hz)\n", HZ);
	printk(KERN_INFO "START-JIFFIES = %10lu \t( #)\n", jlu);
	return 0;
}

/* function to be called when removing the module */
void exit_testmod(void)
{
	jlu = jiffies;
	jend = jlu;

	printk(KERN_INFO "END-JIFFIES   = %10lu \t( #)\n", jlu);
	printk(KERN_INFO "TIME-ELAPSED  = %10lu \t(ms)\n", ((jend - jstart) * 1000) / hertz);

	printk(KERN_INFO "Removing Kernel Module : timer_testmod\n");
	return ;
}

/* macros to register the module entry and exit points with the kernel */
module_init(init_testmod);
module_exit(exit_testmod);

/* including details is standard practice in developing kernel module */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("AV");
