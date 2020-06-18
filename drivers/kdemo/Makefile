#
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.

EXTRA_CFLAGS += -Wno-declaration-after-statement

obj-m := dumy.o tdev.o

dumy-objs := testuevent.o
tdev-objs := testdev.o

ifneq ($(KERNELRELEASE),)
else
    KERNELDIR ?= /usr/src/linux-headers-5.3.0-53-generic
    PWD := $(shell pwd)
    modules := $(patsubst %.o, %.ko, $(obj-m))

install: build
	echo "do nothing"

build:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

uninstall:
	echo "TODO:..."

endif

