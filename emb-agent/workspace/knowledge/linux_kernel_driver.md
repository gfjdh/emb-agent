# Linux Kernel Driver Development

## Module Structure
```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author");
MODULE_DESCRIPTION("Driver Description");
MODULE_VERSION("1.0");

static int __init driver_init(void) {
    printk(KERN_INFO "Driver initialized\n");
    return 0;
}

static void __exit driver_exit(void) {
    printk(KERN_INFO "Driver removed\n");
}

module_init(driver_init);
module_exit(driver_exit);
```

## Makefile
```makefile
obj-m += driver.o
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

## Compilation Steps
1. Install kernel headers: `apt install linux-headers-$(uname -r)`
2. Run `make` to compile
3. Load module: `insmod driver.ko`
4. Check dmesg: `dmesg | tail`
5. Unload module: `rmmod driver`
