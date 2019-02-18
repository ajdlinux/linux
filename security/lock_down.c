// SPDX-License-Identifier: GPL-2.0
/* Lock down the kernel
 *
 * Copyright (C) 2016 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#include <linux/security.h>
#include <linux/export.h>

static enum lockdown_level kernel_locked_down;

char *lockdown_levels[LOCKDOWN_MAX] = {"none", "integrity", "confidentiality"};

/*
 * Put the kernel into lock-down mode.
 */
static int lock_kernel_down(const char *where, enum lockdown_level level)
{
	if (kernel_locked_down >= level)
		return -EPERM;

	kernel_locked_down = level;
	pr_notice("Kernel is locked down from %s; see man kernel_lockdown.7\n",
		  where);
	return 0;
}

static int __init lockdown_param(char *level)
{
	if (!level)
		return -EINVAL;

	if (strcmp(level, "integrity") == 0)
		lock_kernel_down("command line", LOCKDOWN_INTEGRITY);
	else if (strcmp(level, "confidentiality") == 0)
		lock_kernel_down("command line", LOCKDOWN_CONFIDENTIALITY);
	else
		return -EINVAL;

	return 0;
}

early_param("lockdown", lockdown_param);

/*
 * This must be called before arch setup code in order to ensure that the
 * appropriate default can be applied without being overridden by the command
 * line option.
 */
void __init init_lockdown(void)
{
#if defined(CONFIG_LOCK_DOWN_KERNEL_FORCE_INTEGRITY)
	lock_kernel_down("Kernel configuration", LOCKDOWN_INTEGRITY);
#elif defined(CONFIG_LOCK_DOWN_KERNEL_FORCE_CONFIDENTIALITY)
	lock_kernel_down("Kernel configuration", LOCKDOWN_CONFIDENTIALITY);
#endif
}

/**
 * kernel_is_locked_down - Find out if the kernel is locked down
 * @what: Tag to use in notice generated if lockdown is in effect
 */
bool __kernel_is_locked_down(const char *what, enum lockdown_level level,
			     bool first)
{
	if ((kernel_locked_down >= level) && what && first)
		pr_notice("Lockdown: %s is restricted; see man kernel_lockdown.7\n",
			  what);
	return (kernel_locked_down >= level);
}
EXPORT_SYMBOL(__kernel_is_locked_down);

static ssize_t lockdown_read(struct file *filp, char __user *buf, size_t count,
			     loff_t *ppos)
{
	char temp[80];
	int i, offset=0;

	for (i = LOCKDOWN_NONE; i < LOCKDOWN_MAX; i++) {
		if (lockdown_levels[i]) {
			const char *label = lockdown_levels[i];

			if (kernel_locked_down == i)
				offset += sprintf(temp+offset, "[%s] ", label);
			else
				offset += sprintf(temp+offset, "%s ", label);
		}
	}

	/* Convert the last space to a newline if needed. */
	if (offset > 0)
		temp[offset-1] = '\n';

	return simple_read_from_buffer(buf, count, ppos, temp, strlen(temp));
}

static ssize_t lockdown_write(struct file *file, const char __user *buf,
			      size_t n, loff_t *ppos)
{
	char *state;
	int i, len, err = -EINVAL;

	state = memdup_user_nul(buf, n);
	if (IS_ERR(state))
		return PTR_ERR(state);

	len = strlen(state);
	if (len && state[len-1] == '\n') {
		state[len-1] = '\0';
		len--;
	}

	for (i = 0; i < LOCKDOWN_MAX; i++) {
		const char *label = lockdown_levels[i];

		if (label && !strcmp(state, label))
			err = lock_kernel_down("securityfs", i);
	}

	kfree(state);
	return err ? err : n;
}

static const struct file_operations lockdown_ops = {
	.read  = lockdown_read,
	.write = lockdown_write,
};

static int __init lockdown_secfs_init(void)
{
	struct dentry *dentry;

	dentry = securityfs_create_file("lockdown", 0600, NULL, NULL,
					&lockdown_ops);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

	return 0;
}

core_initcall(lockdown_secfs_init);
