#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include "scull.h"

int scull_major = SCULL_MAJOR;
int scull_minor = 0;
int scull_nr_devs = SCULL_NR_DEVS;

module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_nr_devs, int, S_IRUGO);


MODULE_AUTHOR("Kajetan Puchalski");
MODULE_LICENSE("Dual BSD/GPL");

/* empty the scull device */
/* has to be called when the device semaphore is held */
int scull_trim(struct scull_dev *dev)
{
    /* TODO: implement */
    return 0;
}

/* open the device file */
int scull_open(struct inode *inode, struct file *filp)
{
    /* device informatino */
    struct scull_dev *dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev;

    /* trime the device length to 0 if opened write-only */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
        scull_trim(dev);
        mutex_unlock(&dev->lock);
    }
    return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
    return 0;
}

struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .llseek = NULL,
    .read = NULL,
    .write = NULL,
    .unlocked_ioctl = NULL,
    .open = scull_open,
    .release = scull_release,
};

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
    int err, devno = MKDEV(scull_major, scull_minor + index);
    cdev_init(&dev->cdev, &scull_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &scull_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    /* Failure */
    if (err)
        printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

void scull_cleanup_module(void)
{
}

int scull_init_module(void)
{
    int result;
    dev_t dev = 0;

    if (scull_major) {
        dev = MKDEV(scull_major, scull_minor);
        result = register_chrdev_region(dev, scull_nr_devs, "scull");
    } else {
        result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs, "scull");
        scull_major = MAJOR(dev);
    }

    if (result < 0) {
        printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
        return result;
    }

    /* fail: */
    scull_cleanup_module();
    return result;
}

module_init(scull_init_module);
module_exit(scull_cleanup_module);
