#include <asm/uaccess.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/pagewalk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tekassh1");
MODULE_DESCRIPTION("Kernel module to read Page Table data by PID.");
MODULE_VERSION("1.00");

#define DEVICE_NAME "page_table_interface"

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int major_num;
static int device_open_count = 0;

struct dentry *kmodule_dir;
struct dentry *input_file;
struct dentry *output_file;

pid_t pid;

static struct file_operations file_ops = {
    .read = device_read, .write = device_write, .open = device_open, .release = device_release};

static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t *offset) { return 0; }

static void getPageTable(pid_t pid) {
    struct task_struct *my_task;
    my_task = pid_task(find_vpid(pid), PIDTYPE_PID);

    struct mm_struct* mm = my_task->mm;
    mm->mmap_base;


    return;
}

static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset) {
    size_t buffer_size = 32;

    char kernel_buffer[buffer_size];
    if (len > buffer_size) {
        len = buffer_size;
        pr_warn(KERN_WARNING "User tries to read too much data!\n");
    }

    if (copy_from_user(kernel_buffer, buffer, len)) {
        pr_err("Failed to copy data from user space\n");
        return -EFAULT;
    }

    kernel_buffer[len] = '\0';

    pid = simple_strtoll(kernel_buffer, NULL, 10);

    if (pid < 0) {
        pr_err("Failed to convert PID from string to number\n");
        return -EINVAL;
    }

    return len;
}

static int device_open(struct inode *inode, struct file *file) {
    if (device_open_count) {
        return -EBUSY;
    }
    device_open_count++;
    try_module_get(THIS_MODULE);
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    device_open_count--;
    module_put(THIS_MODULE);
    return 0;
}

static int __init kernel_module_init(void) {
    kmodule_dir = debugfs_create_dir(DEVICE_NAME, NULL);
    if (!kmodule_dir) {
        printk(KERN_ALERT "Failed to create debugfs directory\n");
        return -ENOMEM;
    }
    printk(KERN_INFO "Debugfs module dir created!");

    input_file = debugfs_create_file("input", 0644, kmodule_dir, NULL, &file_ops);
    if (!input_file) {
        printk(KERN_ALERT "Failed to create debugfs file\n");
        debugfs_remove(kmodule_dir);
        return -ENOMEM;
    }
    printk(KERN_INFO "Debugfs module input file created!");

    output_file = debugfs_create_file("output", 0644, kmodule_dir, NULL, &file_ops);
    if (!output_file) {
        printk(KERN_ALERT "Failed to create debugfs file\n");
        debugfs_remove(kmodule_dir);
        return -ENOMEM;
    }
    printk(KERN_INFO "Debugfs module output file created!");

    printk(KERN_INFO "Debugfs kernel module successfully registered!");

    return 0;
}

static void __exit kernel_module_exit(void) {
    // debugfs_remove(kmodule_dir);
    // debugfs_remove(input_file);
    // debugfs_remove(output_file);

    printk(KERN_INFO "Debugfs kernel module successfully removed!");
}

module_init(kernel_module_init);
module_exit(kernel_module_exit);