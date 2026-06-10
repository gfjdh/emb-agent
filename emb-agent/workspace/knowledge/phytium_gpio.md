# Phytium GPIO Driver Development

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
