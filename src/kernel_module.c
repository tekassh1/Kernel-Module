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
struct dentry *input_file;
struct dentry *output_file;

pid_t pid;

static struct file_operations file_ops = {
    .read = device_read, .write = device_write, .open = device_open, .release = device_release};

static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t *offset) { return 0; }

void print_flag_table(unsigned long flags) {

    const char *flag_names[] = {
        "VM_READ", "VM_WRITE", "VM_EXEC", "VM_SHARED", "VM_LOCKED", "VM_IO", "VM_PFNMAP", 
        "VM_SEQ_READ", "VM_RAND_READ", "VM_HUGEPAGE"
    };
    
    unsigned long flag_values[] = {
        VM_READ, VM_WRITE, VM_EXEC, VM_SHARED, VM_LOCKED, VM_IO, VM_PFNMAP, 
        VM_SEQ_READ, VM_RAND_READ, VM_HUGEPAGE
    };

    int i;
    char buffer[2048];

    snprintf(buffer, sizeof(buffer), "\t\t\t\t|%-8s", "PTE");
    for (i = 0; i < sizeof(flag_values) / sizeof(flag_values[0]); ++i) {
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "|%-12s", flag_names[i]);
    }
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "|\n");

    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "\t\t\t\t|--------");
    for (i = 0; i < sizeof(flag_values) / sizeof(flag_values[0]); ++i) {
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "|------------");
    }
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "|\n");

    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "\t\t\t\t|%-8s", "");
    for (i = 0; i < sizeof(flag_values) / sizeof(flag_values[0]); ++i) {
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "|%-12d", flags & flag_values[i] ? 1 : 0); 
    }
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "|\n");

    printk(KERN_INFO "%s", buffer);
    return;
}

static void walk_page_table(pid_t pid) {
    printk(KERN_INFO "walk_page_table entered!");

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
        if (!pgd_none(*pgd) && !pgd_bad(*pgd)) {
            printk(KERN_INFO "|-- PGD: addr = 0x%lx\n", (unsigned long)(pgd->pgd));

            p4d = p4d_offset(pgd, addr);
            if (!p4d_none(*p4d) && !p4d_bad(*p4d)) {
                printk(KERN_INFO "  |-- P4D: addr = 0x%lx\n", (unsigned long)(p4d->p4d));

                pud = pud_offset(p4d, addr);
                if (!pud_none(*pud) && !pud_bad(*pud)) {
                    printk(KERN_INFO "      |-- PUD: addr = 0x%lx\n", (unsigned long)(pud->pud));

                    pmd = pmd_offset(pud, addr);
                    if (!pmd_none(*pmd) && !pmd_bad(*pmd)) {
                        printk(KERN_INFO "          |-- PMD: addr = 0x%lx\n", (unsigned long)(pmd->pmd));

                        ptep = pte_offset_map(pmd, addr);
                        if (ptep) {
                            pte = *ptep;

                            page = pte_page(pte);
                            if (page) {
                                printk(KERN_INFO "              |-- PTE: addr = 0x%lx, phys = 0x%lx\n", (unsigned long)(pte.pte), page_to_pfn(page) * PAGE_SIZE);
                                print_flag_table(mmap->vm_flags);
                            }
                        }
                    }
                }
            }
        }
        mmap = mmap->vm_next;
    }

    return;
}

static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset) {
    printk(KERN_INFO "device_write entered!");
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

    printk(KERN_INFO "Requested PID: %d\n entered!", pid);
    walk_page_table(pid);

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