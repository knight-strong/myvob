#
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.

EXTRA_CFLAGS += -Wno-declaration-after-statement

ifneq ($(KERNELRELEASE),)
    obj-m := gpio_sw.o idr410_camera_regulators.o idr410_gps_rfkill.o idr410_lm95235.o earphone.o  sun7i-ir.o 
    
    # i2ctest.o ttywd.o wdfake.o screen.o watchdog.o rtc-pcf8563.o 
else
  KERNELDIR ?= /srv/share/TVBOXA20/lichee/linux-3.4
  PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif

