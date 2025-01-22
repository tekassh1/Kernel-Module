#include <asm/uaccess.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/hugetlb_inline.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/pagewalk.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h> 

#define DEVICE_NAME "page_table_interface"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tekassh1");
MODULE_DESCRIPTION("Kernel module to read Page Table data by PID.");
MODULE_VERSION("1.00");

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int major_num;
static int device_open_count = 0;

struct dentry *kmodule_dir;
struct dentry *interface;

pid_t pid;

static char *kernel_buffer;
static size_t buffer_size = 4000000;
static size_t buffer_offset = 0;

static struct file_operations file_ops = {
    .read = device_read, 
    .write = device_write, 
    .open = device_open, 
    .release = device_release
};

void print_pte_flags(unsigned long flags) {

    // Declared in linux/mm.h
    const char *flag_names[] = {
        "VM_READ", "VM_WRITE", "VM_EXEC", "VM_SHARED", "VM_LOCKED", "VM_IO", "VM_PFNMAP", 
        "VM_SEQ_READ", "VM_RAND_READ", "VM_HUGEPAGE"
    };
    
    unsigned long flag_values[] = {
        VM_READ, VM_WRITE, VM_EXEC, VM_SHARED, VM_LOCKED, VM_IO, VM_PFNMAP, 
        VM_SEQ_READ, VM_RAND_READ, VM_HUGEPAGE
    };

    int i;

    int written = snprintf(kernel_buffer + buffer_offset, buffer_size - buffer_offset, "\t\t\t\t|%-8s", "PTE");
    if (written < 0 || written >= buffer_size - buffer_offset) return;
    buffer_offset += written;

    for (i = 0; i < sizeof(flag_values) / sizeof(flag_values[0]); ++i) {
        written = snprintf(kernel_buffer + buffer_offset, buffer_size - buffer_offset, "|%-12s", flag_names[i]);
        if (written < 0 || written >= buffer_size - buffer_offset) return;
        buffer_offset += written;
    }
    written = snprintf(kernel_buffer + buffer_offset, buffer_size - buffer_offset, "|\n");
    if (written < 0 || written >= buffer_size - buffer_offset) return;
    buffer_offset += written;

    written = snprintf(kernel_buffer + buffer_offset, buffer_size - buffer_offset, "\t\t\t\t|--------");
    if (written < 0 || written >= buffer_size - buffer_offset) return;
    buffer_offset += written;

    for (i = 0; i < sizeof(flag_values) / sizeof(flag_values[0]); ++i) {
        written = snprintf(kernel_buffer + buffer_offset, buffer_size - buffer_offset, "|------------");
        if (written < 0 || written >= buffer_size - buffer_offset) return;
        buffer_offset += written;
    }
    written = snprintf(kernel_buffer + buffer_offset, buffer_size - buffer_offset, "|\n");
    if (written < 0 || written >= buffer_size - buffer_offset) return;
    buffer_offset += written;

    written = snprintf(kernel_buffer + buffer_offset, buffer_size - buffer_offset, "\t\t\t\t|%-8s", "");
    if (written < 0 || written >= buffer_size - buffer_offset) return;
    buffer_offset += written;

    for (i = 0; i < sizeof(flag_values) / sizeof(flag_values[0]); ++i) {
        written = snprintf(kernel_buffer + buffer_offset, buffer_size - buffer_offset, "|%-12d", flags & flag_values[i] ? 1 : 0);
        if (written < 0 || written >= buffer_size - buffer_offset) return;
        buffer_offset += written;
    }
    written = snprintf(kernel_buffer + buffer_offset, buffer_size - buffer_offset, "|\n");
    if (written < 0 || written >= buffer_size - buffer_offset) return;
    buffer_offset += written;
}

static int print_table_params(const char *format, unsigned long arg1, unsigned long arg2) {
    int wr;

    if (arg2 != -1) {
        wr = snprintf(kernel_buffer + buffer_offset, buffer_size - buffer_offset, format, arg1, arg2);
    } else {
        wr = snprintf(kernel_buffer + buffer_offset, buffer_size - buffer_offset, format, arg1);
    }

    if (wr < 0 || wr >= buffer_size - buffer_offset) {
        return -1;
    }

    buffer_offset += wr;
    return 0;
}

static void walk_page_table(pid_t pid) {

    struct task_struct *my_task;
    my_task = pid_task(find_vpid(pid), PIDTYPE_PID);

    struct mm_struct *mm = my_task->mm;
    struct vm_area_struct *mmap = mm->mmap;

    unsigned long addr;

    pgd_t *pgd;
    pte_t *ptep, pte;
    p4d_t* p4d;
    pud_t *pud;
    pmd_t *pmd;

    struct page *page;
    int i;

    for (i = 0; i < mm->map_count; i++) {
        addr = mmap->vm_start;

        pgd = pgd_offset(mm, addr);
        if (pgd_none(*pgd) || pgd_bad(*pgd)) {
            mmap = mmap->vm_next;
            continue;
        }
        
        if (print_table_params("|-- PGD: addr = 0x%lx\n", (unsigned long)(pgd->pgd), -1) < 0) return;

        p4d = p4d_offset(pgd, addr);
        if (p4d_none(*p4d) || p4d_bad(*p4d)) {
            mmap = mmap->vm_next;
            continue;
        }
        
        if (print_table_params("  |-- P4D: addr = 0x%lx\n", (unsigned long)(p4d->p4d), -1) < 0) return;

        pud = pud_offset(p4d, addr);
        if (pud_none(*pud) || pud_bad(*pud)) {
            mmap = mmap->vm_next;
            continue;
        }

        if (print_table_params("      |-- PUD: addr = 0x%lx\n", (unsigned long)(pud->pud), -1) < 0) return;

        pmd = pmd_offset(pud, addr);
        if (pmd_none(*pmd) || pmd_bad(*pmd)) {
            mmap = mmap->vm_next;
            continue;
        }

        if (print_table_params("          |-- PMD: addr = 0x%lx\n", (unsigned long)(pmd->pmd), -1) < 0) return;

        ptep = pte_offset_map(pmd, addr);
        if (!ptep) {
            mmap = mmap->vm_next;
            continue;
        }

        pte = *ptep;
        page = pte_page(pte);
        if (!page) {
            mmap = mmap->vm_next;
            continue;
        }

        if (print_table_params("              |-- PTE: addr = 0x%lx, phys = 0x%lx\n", (unsigned long)(pte.pte), page_to_pfn(page) * PAGE_SIZE) < 0) return;

        print_pte_flags(mmap->vm_flags);

        mmap = mmap->vm_next;
    }
    
    kernel_buffer[buffer_offset] = '\0';
    buffer_offset++;

    return;
}

static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset) {
    kernel_buffer[0] = '\0';
    buffer_offset = 0;

    size_t buff_size = 32;

    char buff[buff_size];
    if (len > buff_size) {
        len = buff_size;
        pr_warn(KERN_WARNING "User tries to read too much data!\n");
    }

    if (copy_from_user(buff, buffer, len)) {
        pr_err("Failed to copy data from user space\n");
        return -EFAULT;
    }

    buff[len] = '\0';

    pid = simple_strtoll(buff, NULL, 10);

    if (pid < 0) {
        pr_err("Failed to convert PID from string to number\n");
        return -EINVAL;
    }

    printk(KERN_INFO "Requested PID: %d\n entered!", pid);
    walk_page_table(pid);

    return len;
}

static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t *offset) {    
    ssize_t bytes_to_read;

    if (*offset >= buffer_size)
        return 0;

    bytes_to_read = min(len, buffer_size - *offset);

    if (copy_to_user(buffer, kernel_buffer + *offset, bytes_to_read)) {
        return -EFAULT;
    }

    *offset += bytes_to_read;

    return bytes_to_read;
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

    kernel_buffer = kmalloc(buffer_size, GFP_KERNEL);
    if (!kernel_buffer) {
        printk(KERN_ALERT "Failed to create kernel buffer!\n");
        return -ENOMEM;
    }

    kmodule_dir = debugfs_create_dir(DEVICE_NAME, NULL);
    if (!kmodule_dir) {
        printk(KERN_ALERT "Failed to create debugfs directory!\n");
        return -ENOMEM;
    }
    printk(KERN_INFO "Debugfs module dir created!");

    interface = debugfs_create_file("interface", 0644, kmodule_dir, NULL, &file_ops);
    if (!interface) {
        printk(KERN_ALERT "Failed to create debugfs file!\n");
        debugfs_remove(kmodule_dir);
        return -ENOMEM;
    }
    printk(KERN_INFO "Debugfs module interface file created.");

    printk(KERN_INFO "Debugfs kernel module successfully registered.");

    return 0;
}

static void __exit kernel_module_exit(void) {
    debugfs_remove_recursive(kmodule_dir);
    printk(KERN_INFO "Debugfs kernel module successfully removed!");
}

module_init(kernel_module_init);
module_exit(kernel_module_exit);