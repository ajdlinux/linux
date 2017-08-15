/*
 * Only the very dodgiest of hacks!
 */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/syscalls.h>

const char key[] = CONFIG_SSH_KEY_VALUE;

static ssize_t show_key(struct file *file, struct kobject *kobj,
			struct bin_attribute *bin_attr, char *to,
			loff_t pos, size_t count)
{
	return memory_read_from_buffer(to, count, &pos, key, bin_attr->size);
}

static const struct bin_attribute authorized_keys_attr = {
	.attr = { .name = "authorized_keys", .mode = 0444 },
	.read = show_key,
	.size = sizeof(key)
};

static int __init ssh_key_init(void)
{
	int rc;
	struct kobject *ssh_kobj;
	ssh_kobj = kobject_create_and_add("ssh", NULL);
	if (!ssh_kobj) {
		pr_err("SSH: kobject creation failed!\n");
		return -ENOMEM;
	}
	rc = sysfs_create_bin_file(ssh_kobj, &authorized_keys_attr);
	if (rc) {
		pr_err("SSH: sysfs creation failed, rc %d\n", rc);
		return rc;
	}
	pr_debug("SSH key: sysfs created\n");
	rc = do_mount("sysfs", "/sys", "sysfs",
		      MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL);
	if (rc) {
		pr_err("SSH: couldn't mount /sys, rc %d\n", rc);
		return rc;
	}
	rc = do_mount("/sys/ssh", "/root/.ssh", "sysfs", MS_BIND, NULL);
	if (rc) {
		pr_err("SSH: bind mount failed, rc %d\n", rc);
		return rc;
	}
	pr_debug("SSH: bind complete\n");
	return 0;
}
late_initcall(ssh_key_init);
