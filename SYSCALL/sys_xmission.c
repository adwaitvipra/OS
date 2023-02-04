#include<linux/kernel.h>
#include<linux/syscalls.h>
#include<asm/param.h>

SYSCALL_DEFINE0(xmission)
{
	printk(KERN_INFO "xmission: invoked...\n");
	printk(KERN_INFO "xmission: timer interrupt frequency is %d hertz\n", HZ);
	printk(KERN_INFO "xmission: returning...\n");
	return 0;
}
