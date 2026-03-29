#ifndef SCULL_DEV_H
#define SCULL_DEV_H

#include <linux/cdev.h>
#include <linux/ioctl.h>
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

/* scull_dev设备使用k作为ioctl的type项 */
#define SCULL_IOC_MAGIC 'k'

/* 
 * S表示通过指针“设置（Set）”
 * T表示直接用参数值“通知（Tell）”
 * G表示“获取（Get）”：通过设置指针来应答
 * Q表示“查询（Query）”：通过返回值应答
 * X表示“交换（Exchange）”：原地交换G和S
 * H表示“切换（Shift）”：原地交互T和Q
 */
#define SCULL_IOCRESET      _IO(SCULL_IOC_MAGIC,    0)
#define SCULL_IOCSQUANTUM   _IOW(SCULL_IOC_MAGIC,   1, int)
#define SCULL_IOCSQSET      _IOW(SCULL_IOC_MAGIC,   2, int)
#define SCULL_IOCTQUANTUM   _IO(SCULL_IOC_MAGIC,    3)
#define SCULL_IOCTQSET      _IO(SCULL_IOC_MAGIC,    4)
#define SCULL_IOCGQUANTUM   _IOR(SCULL_IOC_MAGIC,   5, int)
#define SCULL_IOCGQSET      _IOR(SCULL_IOC_MAGIC,   6, int)
#define SCULL_IOCQQUANTUM   _IO(SCULL_IOC_MAGIC,    7)
#define SCULL_IOCQQSET      _IO(SCULL_IOC_MAGIC,    8)
#define SCULL_IOCXQUANTUM    _IOWR(SCULL_IOC_MAGIC,  9, int)
#define SCULL_IOCXQSET      _IOWR(SCULL_IOC_MAGIC,  10, int)
#define SCULL_IOCHQUANTUM   _IO(SCULL_IOC_MAGIC,    11)
#define SCULL_IOCHQSET      _IO(SCULL_IOC_MAGIC,    12)

/* 当前scull_dev最大支持15个命令 */
#define SCULL_IOC_MAXNR 14


struct scull_qset {
    void **data;             
    struct scull_qset *next;
};

struct scull_dev {
    struct scull_qset *data; // 指向第一个量子集的指针
    int quantum;             // 当前量子的大小
    int qset;                // 当前数组的大小
    unsigned long size;      // 保存在其中的数据总量
    struct semaphore sem;
    struct cdev cdev;
};

struct scull_dev *get_scull_devs(void);

#endif /* SCULL_DEV_H */