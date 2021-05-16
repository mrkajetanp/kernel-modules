/*
 * access.c -- files with access control on open
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
 * $Id: scull.h,v 1.15 2004/11/04 17:51:18 rubini Exp $
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>
#include <linux/tty.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/cred.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/seq_file.h>

#include "scull.h"

static dev_t scull_a_firstdev; /* start of the range */

/*************************************************************************
 * Single-open device
 */

static struct scull_dev scull_s_device;
static atomic_t scull_s_available = ATOMIC_INIT(1);

static int scull_s_open(struct inode *inode, struct file *filp)
{
    /* TODO: implement */
    return 0;
}

static int scull_s_release(struct inode *inode, struct file *filp)
{
    /* TODO: implement */
    return 0;
}

struct file_operations scull_sngl_fops = {
    .owner = THIS_MODULE,
    .llseek = scull_llseek,
    .read = scull_read,
    .write = scull_write,
    .unlocked_ioctl = scull_ioctl,
    .open = scull_s_open,
    .release = scull_s_release,
};


/*************************************************************************
 * Uid device
 */


/*************************************************************************
 * Blocking-open based on uid
 */


/*************************************************************************
 * The 'cloned' private device
 */


/*************************************************************************
 * init and clenup functions
 */

static struct scull_adev_info {
    char *name;
    struct scull_dev *sculldev;
    struct file_operations *fops;
} scull_access_devs[] = {
    { "scullsingle", &scull_s_device, &scull_sngl_fops },
};

#define SCULL_N_ADEVS 1

/*
 * set up a single device
 */
static void scull_access_setup(dev_t devno, struct scull_adev_info *devinfo)
{
    /* TODO: implement */
}

int scull_access_init(dev_t firstdev)
{
    int result, i;

    /* get the number space */
    result = register_chrdev_region(firstdev, SCULL_N_ADEVS, "sculla");
    if (result < 0) {
        printk(KERN_WARNING "sculla: device number registration failed\n");
        return 0;
    }
    scull_a_firstdev = firstdev;

    /* set up each device */
    for (i = 0; i < SCULL_N_ADEVS; i++)
        scull_access_setup(firstdev+i, scull_access_devs+i);
    return SCULL_N_ADEVS;
}

void scull_access_cleanup(void)
{
    /* TODO: implement */
    return;
}





