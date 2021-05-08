#ifndef _SCULL_H_
#define _SCULL_H_

#include <linux/ioctl.h>

#ifndef SCULL_MAJOR
#define SCULL_MAJOR 0
#endif

#ifndef SCULL_NR_DEVS
#define SCULL_NR_DEVS 4
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
/* extern int scull_major; */
/* extern int scull_minor; */
/* extern int scull_nr_devs; */


#endif // _SCULL_H_
