/*
 * main.c -- the bare scull char module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <linux/uaccess.h>

#include "scull.h"
#include "proc_ops_version.h"

#ifdef SCULL_DEBUG
#endif

/*
** Parameters which can be set at load time
*/
int scull_major = SCULL_MAJOR;
int scull_minor = 0;
int scull_nr_devs = SCULL_NR_DEVS; /* number of bare devices */
int scull_quantum = SCULL_QUANTUM;
int scull_qset = SCULL_QSET;

module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_nr_devs, int, S_IRUGO);
module_param(scull_quantum, int, S_IRUGO);
module_param(scull_qset, int, S_IRUGO);


MODULE_AUTHOR("Kajetan Puchalski");
MODULE_LICENSE("Dual BSD/GPL");

/* allocated in scull_init_module */
struct scull_dev *scull_devices;

/* empty the scull device */
/* has to be called when the device semaphore is held */
int scull_trim(struct scull_dev *dev)
{
    struct scull_qset *next, *dptr;
    int qset = dev->qset;
    int i;

    for (dptr = dev->data; dptr; dptr = next) {
        if (dptr->data) {
            for (i = 0; i < qset; i++)
                kfree(dptr->data[i]);
            kfree(dptr->data);
            dptr->data = NULL;
        }
        next = dptr->next;
        kfree(dptr);
    }
    dev->size = 0;
    dev->quantum = scull_quantum;
    dev->qset = scull_qset;
    dev->data = NULL;

    return 0;
}

/* #ifdef SCULL_DEBUG // use proc only in debug mode */

/*
** proc filesystem for debugging
*/
int scull_read_procmem(struct seq_file *s, void *v)
{
    int i, j;
    int limit = s->size - 80;

    for (i = 0; i < scull_nr_devs && s->count <= limit; i++) {
        struct scull_dev *d = &scull_devices[i];
        struct scull_qset *qs = d->data;

        if (mutex_lock_interruptible(&d->lock))
            return -ERESTARTSYS;

        seq_printf(s, "\nDevice %i: qset %i, q %i, sz %li\n",
                   i, d->qset, d->quantum, d->size);
        for (; qs && s->count <= limit; qs = qs->next) {
            seq_printf(s, " item at %p, qset at %p\n", qs, qs->data);
            if (qs->data && !qs->next)
                for (j = 0 ; j < d->qset; j++) {
                    if (qs->data[j])
                        seq_printf(s, "   % 4i: %8p\n", j, qs->data[j]);
                }
        }

        mutex_unlock(&d->lock);
    }

    return 0;
}

/*
** sequence iteration methods
** *pos = scull device number
*/
static void *scull_seq_start(struct seq_file *s, loff_t *pos)
{
    if (*pos >= scull_nr_devs)
        return NULL;
    return scull_devices + *pos;
}

static void *scull_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos)++;
    if (*pos >= scull_nr_devs)
        return NULL;
    return scull_devices + *pos;
}

static void scull_seq_stop(struct seq_file *s, void *v)
{
    /* nothing to do */
}

static int scull_seq_show(struct seq_file *s, void *v)
{
    struct scull_dev *dev = (struct scull_dev*) v;
    struct scull_qset *d;
    int i;

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
    seq_printf(s, "\nDevice %i: qset %i, q %i, sz %li\n",
               (int)(dev - scull_devices), dev->qset, dev->quantum, dev->size);
    for (d = dev->data; d; d = d->next) {
        seq_printf(s, " item at %p, qset at %p\n", d, d->data);
        if (d->data && !d->next)
            for (i = 0 ; i < dev->qset; i++) {
                if (d->data[i])
                    seq_printf(s, "   % 4i: %8p\n", i, d->data[i]);
            }
    }
    mutex_unlock(&dev->lock);

    return 0;
}

/*
** connect the sequence operators
*/

static struct seq_operations scull_seq_ops = {
    .start = scull_seq_start,
    .next = scull_seq_next,
    .stop = scull_seq_stop,
    .show = scull_seq_show
};

/*
** open method sets up the sequence operators
*/
static int scullmem_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, scull_read_procmem, NULL);
}

static int scullseq_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &scull_seq_ops);
}

/*
** Set of file operations for proc files
*/
static struct file_operations scullmem_proc_ops = {
    .owner = THIS_MODULE,
    .open = scullmem_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release
};

static struct file_operations scullseq_proc_ops = {
    .owner = THIS_MODULE,
    .open = scullseq_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release
};

/*
** Actually create/remove the /proc files
*/
static void scull_create_proc(void)
{
    proc_create_data("scullmem", 0, NULL,
                     proc_ops_wrapper(&scullmem_proc_ops, scullmem_pops), NULL);
    proc_create("scullseq", 0, NULL,
                     proc_ops_wrapper(&scullseq_proc_ops, scullmem_pops));
}

static void scull_remove_proc(void)
{
    remove_proc_entry("scullmem", NULL);
    remove_proc_entry("scullseq", NULL);
}

/* #endif // SCULL_DEBUG */


struct scull_qset *scull_follow(struct scull_dev *dev, int n)
{
    struct scull_qset *qs = dev->data;

    /* allocate the qset if needed */
    if (!qs) {
        qs = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
        if (qs == NULL)
            return NULL;
        memset(qs, 0, sizeof(scull_qset));
    }

    /* follow the list */
    while (n--) {
        if (!qs->next) {
            qs->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
            if (qs->next == NULL)
                return NULL;
            memset(qs->next, 0, sizeof(struct scull_qset));
        }
        qs = qs->next;
        continue;
    }

    return qs;
}

ssize_t scull_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos)
{
    struct scull_dev *dev = filp->private_data;
    struct scull_qset *dptr; /* first list item */
    int quantum = dev->quantum;
    int qset = dev->qset;
    int itemsize = quantum * qset; /* bytes in the list item */
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    if (*f_pos >= dev->size)
        goto out;
    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;

    /* listitem, qset index & offset in the quantum */
    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;

    /* follow the list up to the right position */
    dptr = scull_follow(dev, item);

    if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
        goto out; /* don't fill holes */

    /* read only up to the end of this quantum */
    if (count > quantum - q_pos)
        count = quantum - q_pos;

    if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

    out:
        mutex_unlock(&dev->lock);
        return retval;
}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
    struct scull_dev *dev = filp->private_data;
    struct scull_qset *dptr;
    int quantum = dev->quantum;
    int qset = dev->qset;
    int itemsize = quantum * qset;
    int item, s_pos, q_pos, rest;
    ssize_t retval = -ENOMEM; /* value used in "goto out" statements */

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    /* listitem, qset index & offset in the quantum */
    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;

    /* follow the list up to the right position */
    dptr = scull_follow(dev, item);
    if (dptr == NULL)
        goto out;
    /* allocate & initialise the array of pointers */
    if (!dptr->data) {
        dptr->data = kmalloc(qset * sizeof(char*), GFP_KERNEL);
        if (!dptr->data)
            goto out;
        memset(dptr->data, 0, qset * sizeof(char*));
    }
    /* allocate the quantum to be written to */
    if (!dptr->data[s_pos]) {
        dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
        if (!dptr->data[s_pos])
            goto out;
    }
    /* write only up to the end of this quantum */
    if (count > quantum - q_pos)
        count = quantum - q_pos;

    if (copy_from_user(dptr->data[s_pos]+q_pos, buf, count)) {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

    /* update the size */
    if (dev->size < *f_pos)
        dev->size = *f_pos;

    out:
        mutex_unlock(&dev->lock);
        return retval;
}

/* open the device file */
int scull_open(struct inode *inode, struct file *filp)
{
    /* device informatino */
    struct scull_dev *dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev;

    /* trim the device length to 0 if opened write-only */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
        scull_trim(dev);
        mutex_unlock(&dev->lock);
    }
    return 0;
}

/* release the device file */
int scull_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/*
** ioctl() implementation
 */

long scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0, tmp;
    int retval = 0;

    return retval;
}

struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .llseek = NULL,
    .read = scull_read,
    .write = scull_write,
    .unlocked_ioctl = scull_ioctl,
    .open = scull_open,
    .release = scull_release,
};

void scull_cleanup_module(void)
{
    int i;
    dev_t devno = MKDEV(scull_major, scull_minor);

    /* get rid of char dev entries */
    if (scull_devices) {
        for (i = 0; i < scull_nr_devs; i++) {
            scull_trim(scull_devices + i);
            cdev_del(&scull_devices[i].cdev);
        }
        kfree(scull_devices);
   }

    scull_remove_proc();

    /* cleanup_module never called if registering failed */
    unregister_chrdev_region(devno, scull_nr_devs);
}

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

int scull_init_module(void)
{
    int result, i;
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

    /* allocate the devices */
    scull_devices = kmalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL);
    if (!scull_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));

    /* initialise devices */
    for (i = 0 ; i < scull_nr_devs; i++) {
        scull_devices[i].quantum = scull_quantum;
        scull_devices[i].qset = scull_qset;
        mutex_init(&scull_devices[i].lock);
        scull_setup_cdev(&scull_devices[i], i);
    }

    scull_create_proc();

    return 0;

    fail:
        scull_cleanup_module();
        return result;
}

module_init(scull_init_module);
module_exit(scull_cleanup_module);
