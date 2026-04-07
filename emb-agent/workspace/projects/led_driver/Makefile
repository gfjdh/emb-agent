# LED Driver Makefile for Phytium Board
# Target: Phytium ARM64 processor

ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
KDIR ?= /lib/modules/$(shell uname -r)/build

obj-m += led_driver.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules_install

# Build for Phytium development board (cross-compile)
phytium:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

.PHONY: all clean install phytium
