/*
 * scull.h -- definitions for the char module
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

#ifndef _SCULL_H_
#define _SCULL_H_

#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/cdev.h>

/*
** Debugging macros
*/

#undef PDEBUG
#ifdef SCULL_DEBUG
#  ifdef __KERNEL__ /* kernel space */
#    define PDEBUG(fmt, args...) printk(KERN_DEBUG "scull: " fmt, ## args)
#  else /* userspace */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* no debugging; nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing; a placeholder */

#ifndef SCULL_MAJOR
#define SCULL_MAJOR 0
#endif

#ifndef SCULL_NR_DEVS
#define SCULL_NR_DEVS 4
#endif

#ifndef SCULL_P_NR_DEVS
#define SCULL_P_NR_DEVS 4
#endif

/*
** Bare device is a variable-length region of memory
** Uses a linked list of indirect blocks
**
** "scull_dev->data" points to an array of pointers
** each pointer refers to a memory area of SCULL_QUANTUM bytes
** the array (quantum->set) is SCULL_QSET long
*/
#ifndef SCULL_QUANTUM
#define SCULL_QUANTUM 4000
#endif

#ifndef SCULL_QSET
#define SCULL_QSET 1000
#endif

/*
** Pipe device - a simple circular buffer
 */
#ifndef SCULL_P_BUFFER
#define SCULL_P_BUFFER 4000
#endif

/* Scull quantum sets */
struct scull_qset {
    void **data;
    struct scull_qset *next;
};

struct scull_dev {
    struct scull_qset *data; /* Pointer to first quantum set */
    int quantum; /* the current quantum size */
    int qset; /* the current array size */
    unsigned long size; /* amount of data stored */
    unsigned int access_key; /* used by sculluid and scullpriv */
    struct mutex lock; /* mutual exclusion */
    struct cdev cdev; /* char device structure */
};

/*
** Configurable parameters
*/

extern int scull_major;
extern int scull_nr_devs;
extern int scull_quantum;
extern int scull_qset;

extern int scull_p_buffer;

/*
** Prototypes for shared functions
 */

int  scull_p_init(dev_t dev);
void scull_p_cleanup(void);


int scull_trim(struct scull_dev *dev);
ssize_t scull_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos);
ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,
                   loff_t *f_pos);
long scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
loff_t scull_llseek(struct file *filp, loff_t off, int whence);

/*
** Ioctl definitions
 */

#define SCULL_IOC_MAGIC 'j'
#define SCULL_IOCRESET _IO(SCULL_IOC_MAGIC, 0)

/*
** S - "set" through a ptr
** T - "tell" directly with the argument value
** G - "get" reply by setting through a pointer
** Q - "query" response is on the return value
** X - "exchange" switch G and S atomically
** H - "shift" switch T and Q atomically
 */

#define SCULL_IOCSQUANTUM _IOW(SCULL_IOC_MAGIC,  1, int)
#define SCULL_IOCSQSET    _IOW(SCULL_IOC_MAGIC,  2, int)
#define SCULL_IOCTQUANTUM _IO(SCULL_IOC_MAGIC,   3)
#define SCULL_IOCTQSET    _IO(SCULL_IOC_MAGIC,   4)
#define SCULL_IOCGQUANTUM _IOR(SCULL_IOC_MAGIC,  5, int)
#define SCULL_IOCGQSET    _IOR(SCULL_IOC_MAGIC,  6, int)
#define SCULL_IOCQQUANTUM _IO(SCULL_IOC_MAGIC,   7)
#define SCULL_IOCQQSET    _IO(SCULL_IOC_MAGIC,   8)
#define SCULL_IOCXQUANTUM _IOWR(SCULL_IOC_MAGIC, 9, int)
#define SCULL_IOCXQSET    _IOWR(SCULL_IOC_MAGIC,10, int)
#define SCULL_IOCHQUANTUM _IO(SCULL_IOC_MAGIC,  11)
#define SCULL_IOCHQSET    _IO(SCULL_IOC_MAGIC,  12)

#define SCULL_P_IOCTSIZE  _IO(SCULL_IOC_MAGIC,  13)
#define SCULL_P_IOCQSIZE  _IO(SCULL_IOC_MAGIC,  14)

#define SCULL_IOC_MAXNR 14

#endif // _SCULL_H_
