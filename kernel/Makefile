KERNEL_SRC := /usr/src/linux-headers-$(shell uname -r)

obj-m := baikal_scp.o

baikal_scp-objs := \
	baikal_scp_core.o \
	baikal_scp_flash.o \
	baikal_scp_ioctl.o

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
