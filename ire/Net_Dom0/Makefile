obj-m := xen-chrback.o 

xen-chrback-y := dom0.o data.o xen_queue.o

KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules
clean:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) clean
