#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>

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

#define SCULL_DEVICE "/dev/scull0"
 
int main(int argc, char *argv[])
{
    int fd = 0;
    int quantum = 8000;
    int quantum_old = 0;
    int qset = 2000;
    int qset_old = 0;
    
    fd = open(SCULL_DEVICE, O_RDWR);
    if(fd < 0)
    {
        perror("open /dev/scull0");
        return 0;
    }
 
    printf("SCULL_IOCSQUANTUM: quantum = %d\n", quantum);
    ioctl(fd, SCULL_IOCSQUANTUM, &quantum);
    quantum -= 500;
    printf("SCULL_IOCTQUANTUM: quantum = %d\n", quantum);    
    ioctl(fd, SCULL_IOCTQUANTUM, quantum);
    
    ioctl(fd, SCULL_IOCGQUANTUM, &quantum);
    printf("SCULL_IOCGQUANTUM: quantum = %d\n", quantum);    
    quantum = ioctl(fd, SCULL_IOCQQUANTUM);
    printf("SCULL_IOCQQUANTUM: quantum = %d\n", quantum);    
 
    quantum -= 500;
    quantum_old = ioctl(fd, SCULL_IOCHQUANTUM, quantum);
    printf("SCULL_IOCHQUANTUM: quantum = %d, quantum_old = %d\n", quantum, quantum_old);    
    quantum -= 500;
    printf("SCULL_IOCXQUANTUM: quantum = %d\n", quantum);
    ioctl(fd, SCULL_IOCXQUANTUM, &quantum);
    printf("SCULL_IOCXQUANTUM: old quantum = %d\n", quantum);
 
    printf("SCULL_IOCSQSET: qset = %d\n", qset);
    ioctl(fd, SCULL_IOCSQSET, &qset);
    qset += 500;
    printf("SCULL_IOCTQSET: qset = %d\n", qset);
    ioctl(fd, SCULL_IOCTQSET, qset);
 
    ioctl(fd, SCULL_IOCGQSET, &qset);
    printf("SCULL_IOCGQSET: qset = %d\n", qset);
    qset = ioctl(fd, SCULL_IOCQQSET);
    printf("SCULL_IOCQQSET: qset = %d\n", qset);
 
    qset += 500;
    qset_old = ioctl(fd, SCULL_IOCHQSET, qset);
    printf("SCULL_IOCHQSET: qset = %d, qset_old = %d\n", qset, qset_old);    
    qset += 500;
    printf("SCULL_IOCXQSET: qset = %d\n", qset);        
    ioctl(fd, SCULL_IOCXQSET, &qset);
    printf("SCULL_IOCHQSET: old qset = %d\n", qset);
 
    return 0;
}