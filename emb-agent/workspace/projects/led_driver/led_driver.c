/*
 * LED Blinking Driver for Phytium Board
 * 
 * This driver creates a GPIO-based LED driver with configurable blink rate.
 * Supports sysfs interface for userspace control.
 *
 * Copyright (C) 2026
 * License: GPL v2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/atomic.h>

#define DEVICE_NAME     "led_driver"
#define CLASS_NAME      "led"
#define DEFAULT_LED_PIN 52
#define DEFAULT_BLINK_MS 500

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phytium Developer");
MODULE_DESCRIPTION("LED Blinking Driver for Phytium Board");
MODULE_VERSION("1.0");

/* Driver private data */
struct led_data {
    int gpio_pin;
    struct timer_list blink_timer;
    atomic_t blink_state;
    atomic_t blink_enabled;
    unsigned long blink_delay;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    dev_t devno;
};

/* Global driver instance */
static struct led_data *led_data_ptr;
static struct device_node *led_node;

/* Timer callback for LED blinking */
static void led_blink_callback(struct timer_list *t)
{
    struct led_data *data = from_timer(data, t, blink_timer);
    int state;

    if (!atomic_read(&data->blink_enabled))
        return;

    /* Toggle LED state */
    state = atomic_read(&data->blink_state);
    gpio_set_value(data->gpio_pin, state);
    atomic_set(&data->blink_state, !state);

    /* Reschedule timer */
    mod_timer(&data->blink_timer, jiffies + msecs_to_jiffies(data->blink_delay));
}

/* File operations */
static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = led_data_ptr;
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf,
                         size_t count, loff_t *ppos)
{
    char cmd;
    unsigned long delay;

    if (count < 1)
        return -EINVAL;

    if (copy_from_user(&cmd, buf, 1))
        return -EFAULT;

    switch (cmd) {
    case '1':  /* Turn LED on */
        atomic_set(&led_data_ptr->blink_enabled, 0);
        del_timer_sync(&led_data_ptr->blink_timer);
        gpio_set_value(led_data_ptr->gpio_pin, 1);
        break;

    case '0':  /* Turn LED off */
        atomic_set(&led_data_ptr->blink_enabled, 0);
        del_timer_sync(&led_data_ptr->blink_timer);
        gpio_set_value(led_data_ptr->gpio_pin, 0);
        break;

    case 'b':  /* Start blinking */
        atomic_set(&led_data_ptr->blink_enabled, 1);
        atomic_set(&led_data_ptr->blink_state, 1);
        mod_timer(&led_data_ptr->blink_timer, 
                  jiffies + msecs_to_jiffies(led_data_ptr->blink_delay));
        break;

    case 's':  /* Stop blinking */
        atomic_set(&led_data_ptr->blink_enabled, 0);
        del_timer_sync(&led_data_ptr->blink_timer);
        break;

    default:
        break;
    }

    return count;
}

static ssize_t led_read(struct file *filp, char __user *buf,
                        size_t count, loff_t *ppos)
{
    char status[64];
    int len;
    int state = gpio_get_value(led_data_ptr->gpio_pin);

    len = snprintf(status, sizeof(status),
                   "LED: %s, Blink: %s, Delay: %lums\n",
                   state ? "ON" : "OFF",
                   atomic_read(&led_data_ptr->blink_enabled) ? "enabled" : "disabled",
                   led_data_ptr->blink_delay);

    if (*ppos >= len)
        return 0;

    if (count < len - *ppos)
        count = len - *ppos;

    if (copy_to_user(buf, status + *ppos, count))
        return -EFAULT;

    *ppos += count;
    return count;
}

/* Sysfs attributes */
static ssize_t blink_delay_show(struct device *dev,
                                 struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%lu\n", led_data_ptr->blink_delay);
}

static ssize_t blink_delay_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    unsigned long delay;
    if (kstrtoul(buf, 10, &delay) == 0 && delay > 0) {
        led_data_ptr->blink_delay = delay;
        return count;
    }
    return -EINVAL;
}

static DEVICE_ATTR(blink_delay, 0664, blink_delay_show, blink_delay_store);

static ssize_t led_state_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
    int state = gpio_get_value(led_data_ptr->gpio_pin);
    return snprintf(buf, PAGE_SIZE, "%d\n", state);
}

static ssize_t led_state_store(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
    unsigned long state;
    if (kstrtoul(buf, 10, &state) == 0) {
        atomic_set(&led_data_ptr->blink_enabled, 0);
        del_timer_sync(&led_data_ptr->blink_timer);
        gpio_set_value(led_data_ptr->gpio_pin, state ? 1 : 0);
        return count;
    }
    return -EINVAL;
}

static DEVICE_ATTR(led_state, 0664, led_state_show, led_state_store);

static ssize_t blink_enable_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    unsigned long enable;
    if (kstrtoul(buf, 10, &enable) == 0) {
        if (enable) {
            atomic_set(&led_data_ptr->blink_enabled, 1);
            atomic_set(&led_data_ptr->blink_state, 1);
            mod_timer(&led_data_ptr->blink_timer,
                      jiffies + msecs_to_jiffies(led_data_ptr->blink_delay));
        } else {
            atomic_set(&led_data_ptr->blink_enabled, 0);
            del_timer_sync(&led_data_ptr->blink_timer);
        }
        return count;
    }
    return -EINVAL;
}

static DEVICE_ATTR(blink_enable, 0664, NULL, blink_enable_store);

static struct attribute *led_attrs[] = {
    &dev_attr_led_state.attr,
    &dev_attr_blink_delay.attr,
    &dev_attr_blink_enable.attr,
    NULL,
};

static struct attribute_group led_attr_group = {
    .attrs = led_attrs,
};

/* File operations structure */
static const struct file_operations led_fops = {
    .owner   = THIS_MODULE,
    .open    = led_open,
    .release = led_release,
    .read    = led_read,
    .write   = led_write,
};

/* Platform driver probe */
static int led_probe(struct platform_device *pdev)
{
    struct led_data *data;
    int ret;

    printk(KERN_INFO "LED Driver: Probing device\n");

    /* Allocate driver data */
    data = devm_kzalloc(&pdev->dev, sizeof(struct led_data), GFP_KERNEL);
    if (!data) {
        dev_err(&pdev->dev, "Failed to allocate memory\n");
        return -ENOMEM;
    }

    led_data_ptr = data;

    /* Get GPIO pin from device tree or use default */
    if (led_node)
        data->gpio_pin = of_get_gpio(led_node, 0);
    else
        data->gpio_pin = DEFAULT_LED_PIN;

    if (!gpio_is_valid(data->gpio_pin)) {
        dev_err(&pdev->dev, "Invalid GPIO pin: %d\n", data->gpio_pin);
        return -ENODEV;
    }

    /* Request and configure GPIO */
    ret = gpio_request(data->gpio_pin, "led_gpio");
    if (ret) {
        dev_err(&pdev->dev, "Failed to request GPIO %d\n", data->gpio_pin);
        return ret;
    }

    ret = gpio_direction_output(data->gpio_pin, 0);
    if (ret) {
        dev_err(&pdev->dev, "Failed to set GPIO direction\n");
        gpio_free(data->gpio_pin);
        return ret;
    }

    /* Initialize blink parameters */
    atomic_set(&data->blink_state, 0);
    atomic_set(&data->blink_enabled, 0);
    data->blink_delay = DEFAULT_BLINK_MS;

    /* Initialize timer */
    timer_setup(&data->blink_timer, led_blink_callback, 0);

    /* Create character device */
    ret = alloc_chrdev_region(&data->devno, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to allocate device number\n");
        goto err_gpio;
    }

    cdev_init(&data->cdev, &led_fops);
    data->cdev.owner = THIS_MODULE;

    ret = cdev_add(&data->cdev, data->devno, 1);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to add cdev\n");
        goto err_chrdev;
    }

    /* Create device class */
    data->class = class_create(CLASS_NAME);
    if (IS_ERR(data->class)) {
        dev_err(&pdev->dev, "Failed to create class\n");
        ret = PTR_ERR(data->class);
        goto err_cdev;
    }

    /* Create device node */
    data->device = device_create(data->class, NULL, data->devno,
                                  NULL, DEVICE_NAME);
    if (IS_ERR(data->device)) {
        dev_err(&pdev->dev, "Failed to create device\n");
        ret = PTR_ERR(data->device);
        goto err_class;
    }

    /* Create sysfs attributes */
    ret = sysfs_create_group(&data->device->kobj, &led_attr_group);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to create sysfs group\n");
        goto err_device;
    }

    /* Turn LED off initially */
    gpio_set_value(data->gpio_pin, 0);

    dev_info(&pdev->dev, "LED Driver loaded successfully\n");
    dev_info(&pdev->dev, "GPIO Pin: %d, Default blink delay: %lums\n",
             data->gpio_pin, data->blink_delay);

    return 0;

err_device:
    device_destroy(data->class, data->devno);
err_class:
    class_destroy(data->class);
err_cdev:
    cdev_del(&data->cdev);
err_chrdev:
    unregister_chrdev_region(data->devno, 1);
err_gpio:
    gpio_free(data->gpio_pin);

    return ret;
}

static int led_remove(struct platform_device *pdev)
{
    struct led_data *data = led_data_ptr;

    if (!data)
        return -ENODEV;

    /* Stop blinking */
    atomic_set(&data->blink_enabled, 0);
    del_timer_sync(&data->blink_timer);

    /* Turn LED off */
    gpio_set_value(data->gpio_pin, 0);

    /* Cleanup */
    sysfs_remove_group(&data->device->kobj, &led_attr_group);
    device_destroy(data->class, data->devno);
    class_destroy(data->class);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->devno, 1);
    gpio_free(data->gpio_pin);

    dev_info(&pdev->dev, "LED Driver removed\n");

    return 0;
}

/* Device tree matching */
static const struct of_device_id led_of_match[] = {
    { .compatible = "phytium,led-gpio" },
    { .compatible = "gpio-leds" },
    { },
};
MODULE_DEVICE_TABLE(of, led_of_match);

static struct platform_driver led_platform_driver = {
    .probe  = led_probe,
    .remove = led_remove,
    .driver = {
        .name   = DEVICE_NAME,
        .of_match_table = led_of_match,
    },
};

/* Module initialization */
static int __init led_driver_init(void)
{
    int ret;

    printk(KERN_INFO "LED Driver: Initializing module\n");

    /* Register platform driver */
    ret = platform_driver_register(&led_platform_driver);
    if (ret) {
        printk(KERN_ERR "LED Driver: Failed to register platform driver\n");
        return ret;
    }

    printk(KERN_INFO "LED Driver: Module initialized\n");
    return 0;
}

/* Module exit */
static void __exit led_driver_exit(void)
{
    printk(KERN_INFO "LED Driver: Cleaning up module\n");
    platform_driver_unregister(&led_platform_driver);
}

module_init(led_driver_init);
module_exit(led_driver_exit);
