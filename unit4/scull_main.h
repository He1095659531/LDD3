#ifndef SCULL_DEV_H
#define SCULL_DEV_H

#include <linux/cdev.h>
#include <linux/semaphore.h>

#undef PDEBUG
#ifdef SCULL_DEBUG
    #ifdef __KERNEL__
        #define PDEBUG(fmt, args...) printk(KERN_DEBUG "scull: " fmt, ## args)
    #else
        #define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
    #endif
#else
    #define PDEBUG(fmt, args...)   /* nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...)   /* nothing */

#define SCULL_DEV_SIZE 4
#define SCULL_MAJOR 0
#define SCULL_QSET 1000
#define SCULL_QUANTUM 4000
#define SCULL_OK 0
#define SCULL_ERR (-1)

struct scull_qset {
    void **data;             
    struct scull_qset *next;
};

struct scull_dev {
    struct scull_qset *data; // 指向第一个量子集的指针
    int quantum;             // 当前量子的大小
    int qset;                // 当前数组的大小
    unsigned long size;      // 保存在其中的数据总量
    struct cdev cdev;
};

struct scull_dev *get_scull_devs(void);

#endif /* SCULL_DEV_H */