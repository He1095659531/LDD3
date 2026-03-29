#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/capability.h> 	/* capable */
#include <linux/kernel.h> 		/* printk() */
#include <linux/uaccess.h> 		/* copy_*_user/get_user */
#include <linux/types.h> 		/* size_t */
#include <linux/fcntl.h> 		/* O_ACCMODE */
#include <linux/fs.h> 			/* file_operations */
#include <linux/cdev.h> 		/* cdev */
#include <linux/errno.h> 		/* errno */
#include "scull_main.h"

static dev_t dev_num;
static struct scull_dev *scull_devs;
static struct class *scull_class;
static int scull_major = SCULL_MAJOR;
static int scull_minor = 0;

/* 两个全局变量专门用来给ioctl用例使用的 */
static int g_scull_quantum = SCULL_QUANTUM;
static int g_scull_qset = SCULL_QSET;

struct scull_dev *get_scull_devs(void)
{
	return scull_devs;
}

static int scull_trim(struct scull_dev *dev)
{
	struct scull_qset *next = NULL;
	struct scull_qset *dptr = NULL;
	int qset = dev->qset;
	int i;

	for (dptr = dev->data; dptr; dptr = next) {
		if (dptr->data) {
			for (i = 0; i < qset; i++) {
				kfree(dptr->data[i]);
			}
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}

	dev->size = 0;
	dev->qset = SCULL_QSET;
	dev->quantum = SCULL_QUANTUM;
	dev->data = NULL;

	return SCULL_OK;
}

static struct scull_qset *scull_follow(struct scull_dev *dev, int item)
{
	struct scull_qset *qs = dev->data;

	if (!qs) {
		qs = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		if (!qs) {
			return NULL;
		}
		memset(qs, 0, sizeof(struct scull_qset));
	}

	/* 遍历链表，到最终链表项 */
	while (item--) {
		if (!qs->next) {
			qs->next =
				kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
			if (!qs->next)
				return NULL;
			memset(qs->next, 0, sizeof(struct scull_qset));
		}
		qs = qs->next;
	}

	return qs;
}

static ssize_t scull_read(struct file *filp, char __user *buf, size_t count,
			  loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr = NULL;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset; /* 量子集大小 */
	int item; /* 链表项 */
	int rest; /* 在当前链表项中的偏移 */
	int s_pos; /* 数组项 */
	int q_pos; /* 在当前数组项中的偏移 */
	ssize_t ret = SCULL_OK;

	if (down_interruptible(&dev->sem))  
    	return -ERESTARTSYS;

	if (*f_pos >= dev->size) {
		goto out;
	}

	if (*f_pos + count > dev->size) {
		count = dev->size - *f_pos;
	}

	/* 在量子集中寻找链表项、qset索引以及偏移量 */
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	/* 沿该链表前行，直到正确的位置 */
	dptr = scull_follow(dev, item);

	if (dptr == NULL || !dptr->data || !dptr->data[s_pos]) {
		goto out;
	}

	if (count > quantum - q_pos) {
		/* 每次只处理一个数据量子 */
		count = quantum - q_pos;
	}

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
		ret = -EFAULT;
		goto out;
	}

	*f_pos += count;
	ret = count;

out:
	up(&dev->sem);
	return ret;
}

static ssize_t scull_write(struct file *filp, const char __user *buf,
			   size_t count, loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr = NULL;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset; /* 量子集大小 */
	int item; /* 链表项 */
	int rest; /* 在当前链表项中的偏移 */
	int s_pos; /* 数组项 */
	int q_pos; /* 在当前数组项中的偏移 */
	ssize_t ret = -ENOMEM;

	if (down_interruptible(&dev->sem))  
    	return -ERESTARTSYS;  

	/* 在量子集中寻找链表项、qset索引以及偏移量 */
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);
	if (dptr == NULL) {
		goto out;
	}

	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if (!dptr->data) {
			goto out;
		}
	}

	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (!dptr->data[s_pos]) {
			goto out;
		}
	}

	if (count > quantum - q_pos) {
		/* 每次只处理一个数据量子 */
		count = quantum - q_pos;
	}

	if (copy_from_user(dptr->data[s_pos] + q_pos, buf, count)) {
		ret = -EFAULT;
		goto out;
	}

	*f_pos += count;
	ret = count;

	if (dev->size < *f_pos) {
		dev->size = *f_pos;
	}
out:
	up(&dev->sem);
	return ret;
}

static int scull_open(struct inode *inode, struct file *filep)
{
	struct scull_dev *dev;

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filep->private_data = dev;

	if ((filep->f_flags & O_ACCMODE) == O_WRONLY) {
		// 以写方式打开设备文件时，清空设备中的数据
		if (down_interruptible(&dev->sem))
    		return -ERESTARTSYS;
		scull_trim(dev);
		up(&dev->sem);
	}

	return SCULL_OK;
}

static int scull_release(struct inode *inode, struct file *filep)
{
	return SCULL_OK;
}

static long scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
    int tmp;

	/* 抽取类型和编号位字段，进行校验 */
	if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC || _IOC_NR(cmd) > SCULL_IOC_MAXNR)
		return -ENOTTY;

	/* LDD3中的VERIFY_WRITE等宏早已在linux 5.0+版本被剔除，因此这里用更通用的方法 */
	switch (cmd) {
	case SCULL_IOCRESET:
		g_scull_quantum = SCULL_QUANTUM;
		g_scull_qset = SCULL_QSET;
		break;

	case SCULL_IOCSQUANTUM: /* Set: arg指向参数值 */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		ret = get_user(g_scull_quantum, (int __user *)arg);
		break;

	case SCULL_IOCTQUANTUM: /* Tell: arg本身就是参数值 */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		g_scull_quantum = arg;
		break;

	case SCULL_IOCGQUANTUM: /* Get: arg是指向结构的指针 */
		ret = put_user(g_scull_quantum, (int __user *)arg);
		break;

	case SCULL_IOCQQUANTUM: /* Query: 返回结果 */
		return g_scull_quantum;

	case SCULL_IOCXQUANTUM: /* eXchange: 将arg作为指针使用 */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = g_scull_quantum;
		ret = get_user(g_scull_quantum, (int __user *)arg);
		if (ret == 0)
			ret = put_user(tmp, (int __user *)arg);
		break;

	case SCULL_IOCHQUANTUM: /* sHift: 和Tell + Query类似 */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = g_scull_quantum;
		g_scull_quantum = arg;
		return tmp;

	case SCULL_IOCSQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		ret = get_user(g_scull_qset, (int __user *)arg);
		break;

	case SCULL_IOCTQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		g_scull_qset = arg;
		break;

	case SCULL_IOCGQSET:
		ret = put_user(g_scull_qset, (int __user *)arg);
		break;

	case SCULL_IOCQQSET:
		return g_scull_qset;

	case SCULL_IOCXQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = g_scull_qset;
		ret = get_user(g_scull_qset, (int __user *)arg);
		if (ret == 0)
			ret = put_user(tmp, (int __user *)arg);
		break;

	case SCULL_IOCHQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = g_scull_qset;
		g_scull_qset = arg;
		return tmp;

	default: /* 冗余，前面已经根据SCULL_IOC_MAXNR校验过 */
		return -ENOTTY;
	}
	return ret;
}

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.read = scull_read,
	.write = scull_write,
	.open = scull_open,
	.release = scull_release,
	.unlocked_ioctl = scull_ioctl,
};

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
	int ret;

	dev_t tmp_dev_num = MKDEV(MAJOR(dev_num), index);

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_fops;

	ret = cdev_add(&dev->cdev, tmp_dev_num, 1);
	if (ret != 0) {
		PDEBUG("Failed to add cdev");
		return;
	}

	device_create(scull_class, NULL, tmp_dev_num, NULL, "scull%d", index);

	return;
}

static int __init scull_init(void)
{
	int ret;

	if (scull_major) {
		dev_num = MKDEV(scull_major, scull_minor);
		ret = register_chrdev_region(dev_num, SCULL_DEV_SIZE,
					     "scull_dev");
	} else {
		ret = alloc_chrdev_region(&dev_num, 0, SCULL_DEV_SIZE,
					  "scull_dev");
		scull_major = MAJOR(dev_num);
	}

	if (ret != 0) {
		PDEBUG("Failed to set chrdev region.\n");
		return ret;
	}

	scull_class = class_create("scull_class");
	if (scull_class == NULL) {
		PDEBUG("Failed to create class");
		unregister_chrdev_region(dev_num, SCULL_DEV_SIZE);
		return SCULL_ERR;
	}

	scull_devs = (struct scull_dev *)kmalloc(
		SCULL_DEV_SIZE * sizeof(struct scull_dev), GFP_KERNEL);
	if (scull_devs == NULL) {
		PDEBUG("Failed to kmalloc");
		class_destroy(scull_class);
		unregister_chrdev_region(dev_num, SCULL_DEV_SIZE);
		return -ENOMEM;
	}

	memset(scull_devs, 0, SCULL_DEV_SIZE * sizeof(struct scull_dev));

	for (int i = 0; i < SCULL_DEV_SIZE; i++) {
		scull_devs[i].quantum = SCULL_QUANTUM;
		scull_devs[i].qset = SCULL_QSET;
		sema_init(&scull_devs[i].sem, 1);
		scull_setup_cdev(&scull_devs[i], i);
	}

	PDEBUG("Scull dev module load success.\n");
	return SCULL_OK;
}

static void __exit scull_exit(void)
{
	for (int i = 0; i < SCULL_DEV_SIZE; i++) {
		device_destroy(scull_class, MKDEV(MAJOR(dev_num), i));
		cdev_del(&scull_devs[i].cdev);
	}
	class_destroy(scull_class);
	unregister_chrdev_region(dev_num, SCULL_DEV_SIZE);
	kfree(scull_devs);

	PDEBUG("Goodbye, scull dev module unloaded success.\n");
	return;
}
module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("colaman");
MODULE_DESCRIPTION("LDD3 example for unit 3");