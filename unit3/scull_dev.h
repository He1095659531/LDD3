#ifndef SCULL_DEV_H
#define SCULL_DEV_H

#include <linux/cdev.h>
#include <linux/semaphore.h>

#define SCULL_MAJOR 0
#define SCULL_QSET 1000
#define SCULL_QUANTUM 4000
#define SCULL_OK 0
#defind SCULL_ERR (-1)

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

#endif /* SCULL_DEV_H */