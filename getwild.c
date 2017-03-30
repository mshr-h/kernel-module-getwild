#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/string.h>

MODULE_LICENSE("MIT");
MODULE_AUTHOR("mshr-h");
MODULE_DESCRIPTION("Get Wild module");
MODULE_VERSION("1.0");

static char *getwild_filename = "/home/mshr/getwild.mp3";

module_param(getwild_filename, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(getwild_filename, "Path to the Get Wild audio file");

static unsigned long **find_sys_call_table(void);
asmlinkage long getwild_open(const char __user *filename, int flags, umode_t mode);

asmlinkage unsigned long **sys_call_table;
asmlinkage long (*original_sys_open)(const char __user *, int, umode_t);

static int __init getwild_init(void) {
	if(!getwild_filename) {
		printk(KERN_ERR "No getwild filename given.\n");
		return ~EINVAL;
	}

	sys_call_table = find_sys_call_table();

	if(!sys_call_table) {
		printk(KERN_ERR "Couldn't find sys_call_table.\n");
		return ~EPERM;
	}

	write_cr0(read_cr0() & (~0x10000));
	original_sys_open = (void *)sys_call_table[__NR_open];
	sys_call_table[__NR_open] = (unsigned long *)getwild_open;
	write_cr0(read_cr0() | 0x10000);

	return 0;
}

static void __exit getwild_exit(void) {
	write_cr0(read_cr0() & (~0x10000));
	sys_call_table[__NR_open] = (unsigned long *)original_sys_open;
	write_cr0(read_cr0() | 0x10000);
}

asmlinkage long getwild_open(const char __user *filename, int flags, umode_t mode) {
	int len = strlen(filename);

	if(strcmp(filename + len - 4, ".mp3")) {
		return (*original_sys_open)(filename, flags, mode);
	} else {
		mm_segment_t old_fs;
		long fd;

		old_fs = get_fs();
		set_fs(KERNEL_DS);

		fd = (*original_sys_open)(getwild_filename, flags, mode);
		set_fs(old_fs);
		return fd;
	}
}

static unsigned long **find_sys_call_table() {
	unsigned long offset;
	unsigned long **cand_sys_call_table;

	for(offset=PAGE_OFFSET; offset<ULLONG_MAX; offset+=sizeof(void *)) {
		cand_sys_call_table = (unsigned long **) offset;

		if(cand_sys_call_table[__NR_close]==(unsigned long *)sys_close)
			return cand_sys_call_table;
	}

	return NULL;
}

module_init(getwild_init);
module_exit(getwild_exit);
