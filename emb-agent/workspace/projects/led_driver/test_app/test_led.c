/*
 * LED Driver Test Application
 * 
 * Tests the LED driver functionality via character device and sysfs.
 *
 * Compile: gcc -o test_led test_led.c
 * Run: ./test_led
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEV_PATH     "/dev/led_driver"
#define SYSFS_STATE  "/sys/class/led/led_driver/led_state"
#define SYSFS_BLINK  "/sys/class/led/led_driver/blink_enable"
#define SYSFS_DELAY  "/sys/class/led/led_driver/blink_delay"

void print_usage(const char *prog)
{
    printf("Usage: %s [command]\n", prog);
    printf("Commands:\n");
    printf("  on      - Turn LED on\n");
    printf("  off     - Turn LED off\n");
    printf("  blink   - Start blinking\n");
    printf("  stop    - Stop blinking\n");
    printf("  status  - Read LED status\n");
    printf("  delay N - Set blink delay (ms)\n");
    printf("  test    - Run automated test\n");
}

int write_sysfs(const char *path, const char *value)
{
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("Failed to open sysfs");
        return -1;
    }
    fprintf(fp, "%s", value);
    fclose(fp);
    return 0;
}

int read_sysfs(const char *path, char *buf, size_t size)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror("Failed to open sysfs");
        return -1;
    }
    if (fgets(buf, size, fp) == NULL) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

int led_on(void)
{
    int fd = open(DEV_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }
    write(fd, "1", 1);
    close(fd);
    printf("LED turned ON\n");
    return 0;
}

int led_off(void)
{
    int fd = open(DEV_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }
    write(fd, "0", 1);
    close(fd);
    printf("LED turned OFF\n");
    return 0;
}

int led_blink(void)
{
    int fd = open(DEV_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }
    write(fd, "b", 1);
    close(fd);
    printf("LED blinking started\n");
    return 0;
}

int led_stop(void)
{
    int fd = open(DEV_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }
    write(fd, "s", 1);
    close(fd);
    printf("LED blinking stopped\n");
    return 0;
}

int led_status(void)
{
    char buf[256];
    int fd = open(DEV_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }
    memset(buf, 0, sizeof(buf));
    read(fd, buf, sizeof(buf) - 1);
    close(fd);
    printf("LED Status: %s", buf);
    return 0;
}

int set_delay(int ms)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", ms);
    if (write_sysfs(SYSFS_DELAY, buf) < 0) {
        return -1;
    }
    printf("Blink delay set to %d ms\n", ms);
    return 0;
}

int run_test(void)
{
    printf("\n=== LED Driver Test Suite ===\n\n");

    /* Test 1: Turn LED on */
    printf("Test 1: Turn LED ON\n");
    if (led_on() < 0) {
        printf("FAILED\n");
        return -1;
    }
    sleep(1);

    /* Test 2: Turn LED off */
    printf("Test 2: Turn LED OFF\n");
    if (led_off() < 0) {
        printf("FAILED\n");
        return -1;
    }
    sleep(1);

    /* Test 3: Set blink delay */
    printf("Test 3: Set blink delay to 500ms\n");
    if (set_delay(500) < 0) {
        printf("FAILED\n");
        return -1;
    }

    /* Test 4: Start blinking */
    printf("Test 4: Start blinking\n");
    if (led_blink() < 0) {
        printf("FAILED\n");
        return -1;
    }
    printf("LED should be blinking for 3 seconds...\n");
    sleep(3);

    /* Test 5: Stop blinking */
    printf("Test 5: Stop blinking\n");
    if (led_stop() < 0) {
        printf("FAILED\n");
        return -1;
    }

    /* Test 6: Read status */
    printf("Test 6: Read status\n");
    if (led_status() < 0) {
        printf("FAILED\n");
        return -1;
    }

    printf("\n=== All tests completed ===\n");
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "on") == 0) {
        return led_on();
    } else if (strcmp(argv[1], "off") == 0) {
        return led_off();
    } else if (strcmp(argv[1], "blink") == 0) {
        return led_blink();
    } else if (strcmp(argv[1], "stop") == 0) {
        return led_stop();
    } else if (strcmp(argv[1], "status") == 0) {
        return led_status();
    } else if (strcmp(argv[1], "delay") == 0) {
        if (argc < 3) {
            printf("Error: delay requires milliseconds argument\n");
            return 1;
        }
        return set_delay(atoi(argv[2]));
    } else if (strcmp(argv[1], "test") == 0) {
        return run_test();
    } else {
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
