/*
 * rtl_proc.h
 *
 *  Created on: 27 jun 2018
 *  mailto: dmitry.parhomenko@terranet.se
 */

#ifndef INCLUDE_RTL_PROC_H_
#define INCLUDE_RTL_PROC_H_

struct rtl_proc_hdl {
	char *name;
	int (*show)(struct seq_file *, void *);
	ssize_t (*write)(struct file *file, const char __user *buffer, size_t count, loff_t *pos);
};

#define RTL_PROC_HDL_ITEM(_name, _show, _write) \
	{ .name = _name, .show = _show, .write = _write }

int proc_init (void* cookie);
void proc_cleanup(void);

#endif /* INCLUDE_RTL_PROC_H_ */
