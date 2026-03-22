obj-m := scull_dev.o
scull_dev-objs = scull_main.o scull_proc_ops.o

KDIR := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

DEBUG = y

ifeq ($(DEBUG), y)
	DEBFLAGS = -O -g -DSCULL_DEBUG
else
	DEBFLAGS = -O2
endif

CFLAGS += $(DEBFLAGS)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

