# LED Blinking Driver for Phytium Board

## Overview

This is a Linux kernel driver for controlling LEDs on Phytium ARM64 development boards. The driver provides both a character device interface and sysfs interface for LED control.

## Features

- GPIO-based LED control
- Configurable blink rate via sysfs
- Character device interface for programmatic control
- Device tree support
- Thread-safe operation using atomic variables

## Hardware Requirements

- Phytium development board (D2000, S2500, FT-2000)
- LED connected to GPIO pin (default: GPIO 52)

## Build

### Prerequisites

```bash
# Install kernel headers
sudo apt install linux-headers-$(uname -r)

# For cross-compilation (Phytium ARM64)
sudo apt install gcc-aarch64-linux-gnu
```

### Compile

```bash
# Local build (x86_64 host)
make

# Cross-compile for Phytium ARM64
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- phytium
```

## Usage

### Load the Driver

```bash
# Load module
sudo insmod led_driver.ko

# Verify loaded
lsmod | grep led_driver
dmesg | tail
```

### Control via Sysfs

```bash
# LED state control
echo 1 > /sys/class/led/led_driver/led_state  # Turn ON
echo 0 > /sys/class/led/led_driver/led_state  # Turn OFF

# Blink control
echo 1 > /sys/class/led/led_driver/blink_enable  # Start blinking
echo 0 > /sys/class/led/led_driver/blink_enable  # Stop blinking

# Set blink delay (milliseconds)
echo 200 > /sys/class/led/led_driver/blink_delay

# Read current status
cat /sys/class/led/led_driver/led_state
cat /sys/class/led/led_driver/blink_delay
```

### Control via Character Device

```bash
# Commands:
# '1' - Turn LED on
# '0' - Turn LED off
# 'b' - Start blinking
# 's' - Stop blinking

# Examples
echo "1" > /dev/led_driver    # Turn on
echo "b" > /dev/led_driver    # Start blinking
echo "s" > /dev/led_driver    # Stop blinking

# Read status
cat /dev/led_driver
```

### Unload the Driver

```bash
sudo rmmod led_driver
```

## Device Tree Configuration

Add to your device tree source:

```dts
/ {
    leds {
        compatible = "phytium,led-gpio";
        led1 {
            gpios = <&gpio 52 GPIO_ACTIVE_HIGH>;
            label = "phytium:green:user";
        };
    };
};
```

## Project Structure

```
led_driver/
├── led_driver.c    # Kernel module source
├── Makefile        # Build configuration
├── README.md       # Documentation
└── test_app/       # Test application (optional)
    ├── Makefile
    └── test_led.c
```

## API Reference

### Sysfs Attributes

| Attribute | Type | Description |
|-----------|------|-------------|
| led_state | RW | LED state (0=off, 1=on) |
| blink_enable | WO | Enable/disable blinking (0/1) |
| blink_delay | RW | Blink period in milliseconds |

### Character Device Commands

| Command | Action |
|---------|--------|
| '0' | Turn LED off |
| '1' | Turn LED on |
| 'b' | Start blinking |
| 's' | Stop blinking |

## Troubleshooting

1. **Module fails to load**: Check if GPIO pin is valid and not in use
   ```bash
   cat /proc/interrupts | grep gpio
   ```

2. **Permission denied**: Ensure running with root privileges

3. **LED not blinking**: Verify physical connection of LED to GPIO pin

## License

GPL v2
