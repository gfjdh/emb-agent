"""Knowledge base for embedded development."""

import os
from pathlib import Path
from typing import Any


class KnowledgeBase:
    """Simple vector store for knowledge retrieval."""

    def __init__(self, workspace: Path, top_k: int = 5):
        self.workspace = workspace
        self.top_k = top_k
        self.knowledge_dir = workspace / "knowledge"
        self._index: list[dict[str, Any]] = []
        self._load_knowledge()

    def _load_knowledge(self) -> None:
        """Load knowledge files into memory."""
        if not self.knowledge_dir.exists():
            self.knowledge_dir.mkdir(parents=True, exist_ok=True)

        kb_files = list(self.knowledge_dir.glob("*.md"))
        if not kb_files:
            self._create_default_knowledge()
            return

        for kb_file in kb_files:
            content = kb_file.read_text(encoding="utf-8")
            title = kb_file.stem.replace("_", " ").replace("-", " ")
            self._index.append({
                "title": title,
                "content": content,
                "file": str(kb_file),
            })

    def _create_default_knowledge(self) -> None:
        """Create default knowledge base files for Phytium development."""
        phytium_gpio = """# Phytium GPIO Driver Development

## Overview
Phytium processors feature GPIO controllers for general-purpose input/output operations.

## Key APIs
- `gpio_request()` - Request GPIO pin
- `gpio_free()` - Release GPIO pin
- `gpio_direction_input()` - Set pin as input
- `gpio_direction_output()` - Set pin as output
- `gpio_get_value()` - Read pin value
- `gpio_set_value()` - Write pin value

## LED Driver Example
```c
#include <linux/gpio.h>
#include <linux/module.h>

static int led_pin = 52;

static int __init led_init(void) {
    int ret;
    ret = gpio_request(led_pin, "led_gpio");
    if (ret) return ret;
    gpio_direction_output(led_pin, 0);
    gpio_set_value(led_pin, 1);
    return 0;
}

static void __exit led_exit(void) {
    gpio_set_value(led_pin, 0);
    gpio_free(led_pin);
}

module_init(led_init);
module_exit(led_exit);
```

## Device Tree Binding
```dts
leds {
    compatible = "gpio-leds";
    led1 {
        gpios = <&gpio 52 GPIO_ACTIVE_HIGH>;
        default-state = "off";
    };
};
```
"""
        linux_kernel_driver = """# Linux Kernel Driver Development

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
    printk(KERN_INFO "Driver initialized\\n");
    return 0;
}

static void __exit driver_exit(void) {
    printk(KERN_INFO "Driver removed\\n");
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
\t$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
\t$(MAKE) -C $(KDIR) M=$(PWD) clean
```

## Compilation Steps
1. Install kernel headers: `apt install linux-headers-$(uname -r)`
2. Run `make` to compile
3. Load module: `insmod driver.ko`
4. Check dmesg: `dmesg | tail`
5. Unload module: `rmmod driver`
"""

        phytium_platform = """# Phytium Platform Development

## Processor Families
- Phytium D2000 - Desktop processor
- Phytium S2500 - Server processor
- Phytium FT-2000 - Embedded processor

## Boot Process
1. BootROM (internal)
2. U-Boot (or other bootloader)
3. Linux Kernel
4. Root Filesystem

## Buildroot Integration
```bash
# Configure for Phytium
make CROSS_COMPILE=aarch64-linux-gnu- phytium_defconfig
# Customize
make menuconfig
# Build
make
```

## Cross-Compilation
```bash
export CROSS_COMPILE=aarch64-linux-gnu-
export ARCH=arm64
# Build kernel
make defconfig
make -j$(nproc)
```

## Development Board Setup
1. Set boot switches to SD/TFTP mode
2. Connect serial console (115200 8N1)
3. Configure network for TFTP boot
"""

        self._index = [
            {"title": "Phytium GPIO Driver Development", "content": phytium_gpio, "file": "phytium_gpio.md"},
            {"title": "Linux Kernel Driver Development", "content": linux_kernel_driver, "file": "linux_kernel_driver.md"},
            {"title": "Phytium Platform Development", "content": phytium_platform, "file": "phytium_platform.md"},
        ]

        for item in self._index:
            kb_file = self.knowledge_dir / item["file"]
            kb_file.write_text(item["content"], encoding="utf-8")

    def retrieve(self, query: str, top_k: int | None = None) -> list[dict[str, Any]]:
        """Retrieve most relevant knowledge chunks for a query."""
        k = top_k or self.top_k

        query_lower = query.lower()
        query_words = set(query_lower.split())

        scored = []
        for item in self._index:
            content_lower = item["content"].lower()
            score = 0

            for word in query_words:
                if word in content_lower:
                    score += 1
                if word in item["title"].lower():
                    score += 2

            if score > 0:
                scored.append((score, item))

        scored.sort(key=lambda x: x[0], reverse=True)
        return [item for _, item in scored[:k]]

    def add_knowledge(self, title: str, content: str, source: str = "user") -> None:
        """Add new knowledge to the base."""
        safe_title = title.lower().replace(" ", "_").replace("/", "_")[:50]
        filename = f"{safe_title}.md"
        kb_file = self.knowledge_dir / filename

        item = {
            "title": title,
            "content": content,
            "file": filename,
            "source": source,
        }

        kb_file.write_text(content, encoding="utf-8")

        existing = [i for i in self._index if i["file"] == filename]
        if existing:
            self._index.remove(existing[0])
        self._index.append(item)
