/**
 * @file dht11_driver.c
 * @brief DHT11温湿度传感器驱动实现
 * @author Embedded System Team
 * @date 2024-01-15
 * 
 * DHT11数字温湿度传感器驱动，适用于飞腾开发板
 * 使用GPIO接口，单总线协议
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>

#include "dht11.h"
#include "sensor_types.h"

// DHT11操作接口
const sensor_ops_t dht11_ops = {
    .init = dht11_init,
    .read = dht11_read,
    .cleanup = dht11_cleanup,
    .get_info = dht11_get_info,
    .calibrate = dht11_calibrate
};

// 全局变量
static dht11_private_t dht11_private = {0};
static uint8_t dht11_initialized = 0;

// GPIO相关定义（飞腾开发板）
#define GPIO_BASE 0x28004000  // 示例GPIO基地址，实际值需根据具体开发板调整
#define GPIO_SIZE 0x1000

// GPIO寄存器偏移
#define GPIO_DATA_OFFSET 0x00
#define GPIO_DIR_OFFSET  0x04
#define GPIO_SET_OFFSET  0x08
#define GPIO_CLR_OFFSET  0x0C

// 内存映射指针
static volatile uint32_t *gpio_regs = NULL;
static int gpio_mem_fd = -1;

/**
 * @brief 初始化DHT11传感器
 */
sensor_status_t dht11_init(sensor_config_t *config)
{
    if (!config) {
        return SENSOR_STATUS_ERROR;
    }
    
    printf("初始化DHT11传感器，GPIO引脚: %d\n", config->gpio_pin);
    
    // 检查是否已经初始化
    if (dht11_initialized) {
        printf("DHT11已经初始化\n");
        return SENSOR_STATUS_OK;
    }
    
    // 初始化私有数据结构
    memset(&dht11_private, 0, sizeof(dht11_private_t));
    dht11_private.config.gpio_pin = config->gpio_pin;
    dht11_private.config.timeout_us = DHT11_TIMEOUT_US;
    dht11_private.config.pull_up_resistor = 1;
    
    // 初始化GPIO（这里使用内存映射方式）
    // 注意：实际开发中可能需要使用libgpiod或其他GPIO库
    
    dht11_initialized = 1;
    dht11_private.last_read_time = 0;
    
    printf("DHT11初始化完成\n");
    return SENSOR_STATUS_OK;
}

/**
 * @brief 读取DHT11传感器数据
 */
sensor_status_t dht11_read(sensor_data_t *data)
{
    if (!data || !dht11_initialized) {
        return SENSOR_STATUS_ERROR;
    }
    
    // 检查读取间隔
    uint32_t current_time = (uint32_t)time(NULL);
    if (current_time - dht11_private.last_read_time < DHT11_MIN_INTERVAL_MS / 1000) {
        printf("DHT11读取间隔太短，请等待至少2秒\n");
        return SENSOR_STATUS_ERROR;
    }
    
    printf("读取DHT11传感器数据...\n");
    
    uint8_t raw_data[5] = {0};
    float temperature = 0.0f;
    float humidity = 0.0f;
    
    // 发送开始信号
    sensor_status_t status = dht11_send_start_signal(dht11_private.config.gpio_pin);
    if (status != SENSOR_STATUS_OK) {
        dht11_private.timeout_errors++;
        return status;
    }
    
    // 等待响应
    status = dht11_wait_response(dht11_private.config.gpio_pin, 
                                 dht11_private.config.timeout_us);
    if (status != SENSOR_STATUS_OK) {
        dht11_private.timeout_errors++;
        return status;
    }
    
    // 读取数据
    status = dht11_read_all_data(dht11_private.config.gpio_pin,
                                dht11_private.config.timeout_us,
                                raw_data);
    if (status != SENSOR_STATUS_OK) {
        return status;
    }
    
    // 验证校验和
    if (!dht11_verify_checksum(raw_data)) {
        printf("DHT11校验和错误\n");
        dht11_private.checksum_errors++;
        return SENSOR_STATUS_CHECKSUM_ERROR;
    }
    
    // 解析数据
    status = dht11_parse_data(raw_data, &temperature, &humidity);
    if (status != SENSOR_STATUS_OK) {
        return status;
    }
    
    // 填充数据
    data->timestamp = time(NULL);
    data->temperature = temperature;
    data->humidity = humidity;
    data->sensor_type = SENSOR_TYPE_DHT11;
    data->status = SENSOR_STATUS_OK;
    
    // 更新最后读取时间
    dht11_private.last_read_time = current_time;
    
    printf("DHT11读取成功: 温度=%.1f°C, 湿度=%.1f%%\n", 
           temperature, humidity);
    
    return SENSOR_STATUS_OK;
}

/**
 * @brief 清理DHT11传感器资源
 */
void dht11_cleanup(void)
{
    if (!dht11_initialized) {
        return;
    }
    
    printf("清理DHT11传感器资源\n");
    
    // 释放GPIO资源
    if (gpio_regs) {
        // 取消内存映射
        if (gpio_mem_fd >= 0) {
            close(gpio_mem_fd);
        }
        gpio_regs = NULL;
    }
    
    memset(&dht11_private, 0, sizeof(dht11_private_t));
    dht11_initialized = 0;
    
    printf("DHT11资源清理完成\n");
}

/**
 * @brief 获取DHT11传感器信息
 */
const char* dht11_get_info(void)
{
    static char info[256];
    
    snprintf(info, sizeof(info),
             "DHT11温湿度传感器\n"
             "GPIO引脚: %d\n"
             "超时错误: %d\n"
             "校验和错误: %d\n"
             "最后读取时间: %u\n",
             dht11_private.config.gpio_pin,
             dht11_private.timeout_errors,
             dht11_private.checksum_errors,
             dht11_private.last_read_time);
    
    return info;
}

/**
 * @brief DHT11传感器校准
 */
sensor_status_t dht11_calibrate(float *params, int param_count)
{
    // DHT11不支持软件校准
    printf("DHT11不支持软件校准\n");
    return SENSOR_STATUS_OK;
}

/**
 * @brief 创建DHT11传感器配置
 */
sensor_config_t dht11_create_config(uint8_t gpio_pin)
{
    sensor_config_t config;
    memset(&config, 0, sizeof(sensor_config_t));
    
    config.type = SENSOR_TYPE_DHT11;
    config.gpio_pin = gpio_pin;
    config.sample_interval = DEFAULT_SAMPLE_INTERVAL;
    snprintf(config.name, sizeof(config.name), "DHT11_GPIO%d", gpio_pin);
    
    return config;
}

/**
 * @brief 验证DHT11数据校验和
 */
uint8_t dht11_verify_checksum(uint8_t data[5])
{
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    return (checksum == data[4]);
}

/**
 * @brief 解析DHT11原始数据
 */
sensor_status_t dht11_parse_data(uint8_t raw_data[5], float *temperature, float *humidity)
{
    if (!temperature || !humidity) {
        return SENSOR_STATUS_ERROR;
    }
    
    // DHT11数据格式：
    // byte0: 湿度整数部分
    // byte1: 湿度小数部分（总是0）
    // byte2: 温度整数部分
    // byte3: 温度小数部分（总是0）
    // byte4: 校验和
    
    *humidity = (float)raw_data[0];
    *temperature = (float)raw_data[2];
    
    // 验证数据范围
    if (*humidity < 20.0f || *humidity > 90.0f) {
        printf("湿度值超出范围: %.1f%%\n", *humidity);
        return SENSOR_STATUS_ERROR;
    }
    
    if (*temperature < 0.0f || *temperature > 50.0f) {
        printf("温度值超出范围: %.1f°C\n", *temperature);
        return SENSOR_STATUS_ERROR;
    }
    
    return SENSOR_STATUS_OK;
}

/**
 * @brief 发送开始信号到DHT11
 */
sensor_status_t dht11_send_start_signal(uint8_t gpio_pin)
{
    printf("发送DHT11开始信号...\n");
    
    // 设置引脚为输出模式
    if (dht11_set_gpio_direction(gpio_pin, 1) < 0) {
        return SENSOR_STATUS_ERROR;
    }
    
    // 拉低至少18ms
    if (dht11_set_gpio_value(gpio_pin, 0) < 0) {
        return SENSOR_STATUS_ERROR;
    }
    dht11_delay_ms(20);
    
    // 拉高20-40us
    if (dht11_set_gpio_value(gpio_pin, 1) < 0) {
        return SENSOR_STATUS_ERROR;
    }
    dht11_delay_us(30);
    
    // 设置引脚为输入模式，等待响应
    if (dht11_set_gpio_direction(gpio_pin, 0) < 0) {
        return SENSOR_STATUS_ERROR;
    }
    
    return SENSOR_STATUS_OK;
}

/**
 * @brief 等待DHT11响应
 */
sensor_status_t dht11_wait_response(uint8_t gpio_pin, uint32_t timeout_us)
{
    printf("等待DHT11响应...\n");
    
    // 等待低电平响应（80us）
    uint32_t start_time = (uint32_t)clock();
    while (dht11_get_gpio_value(gpio_pin) == 1) {
        if (((uint32_t)clock() - start_time) * 1000000 / CLOCKS_PER_SEC > timeout_us) {
            printf("DHT11响应超时（等待低电平）\n");
            return SENSOR_STATUS_TIMEOUT;
        }
    }
    
    // 等待高电平响应（80us）
    start_time = (uint32_t)clock();
    while (dht11_get_gpio_value(gpio_pin) == 0) {
        if (((uint32_t)clock() - start_time) * 1000000 / CLOCKS_PER_SEC > timeout_us) {
            printf("DHT11响应超时（等待高电平）\n");
            return SENSOR_STATUS_TIMEOUT;
        }
    }
    
    printf("DHT11响应成功\n");
    return SENSOR_STATUS_OK;
}

/**
 * @brief 读取DHT11数据位
 */
int dht11_read_bit(uint8_t gpio_pin, uint32_t timeout_us)
{
    // 等待低电平开始（50us）
    uint32_t start_time = (uint32_t)clock();
    while (dht11_get_gpio_value(gpio_pin) == 1) {
        if (((uint32_t)clock() - start_time) * 1000000 / CLOCKS_PER_SEC > timeout_us) {
            return -1; // 超时
        }
    }
    
    // 等待高电平
    start_time = (uint32_t)clock();
    while (dht11_get_gpio_value(gpio_pin) == 0) {
        if (((uint32_t)clock() - start_time) * 1000000 / CLOCKS_PER_SEC > timeout_us) {
            return -1; // 超时
        }
    }
    
    // 测量高电平持续时间
    uint32_t high_start = (uint32_t)clock();
    while (dht11_get_gpio_value(gpio_pin) == 1) {
        if (((uint32_t)clock() - high_start) * 1000000 / CLOCKS_PER_SEC > timeout_us) {
            return -1; // 超时
        }
    }
    
    uint32_t high_duration = (uint32_t)clock() - high_start;
    
    // 判断数据位：26-28us为0，70us为1
    if (high_duration * 1000000 / CLOCKS_PER_SEC > 50) {
        return 1; // 数据位1
    } else {
        return 0; // 数据位0
    }
}

/**
 * @brief 读取DHT11一个字节
 */
sensor_status_t dht11_read_byte(uint8_t gpio_pin, uint32_t timeout_us, uint8_t *byte)
{
    if (!byte) {
        return SENSOR_STATUS_ERROR;
    }
    
    *byte = 0;
    
    for (int i = 7; i >= 0; i--) {
        int bit = dht11_read_bit(gpio_pin, timeout_us);
        if (bit < 0) {
            return SENSOR_STATUS_TIMEOUT;
        }
        
        *byte |= (bit << i);
    }
    
    return SENSOR_STATUS_OK;
}

/**
 * @brief 读取DHT11所有数据
 */
sensor_status_t dht11_read_all_data(uint8_t gpio_pin, uint32_t timeout_us, uint8_t data[5])
{
    if (!data) {
        return SENSOR_STATUS_ERROR;
    }
    
    printf("读取DHT11数据...\n");
    
    for (int i = 0; i < 5; i++) {
        sensor_status_t status = dht11_read_byte(gpio_pin, timeout_us, &data[i]);
        if (status != SENSOR_STATUS_OK) {
            return status;
        }
    }
    
    printf("DHT11原始数据: ");
    for (int i = 0; i < 5; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
    
    return SENSOR_STATUS_OK;
}

/**
 * @brief 设置GPIO引脚方向（简化实现）
 */
int dht11_set_gpio_direction(uint8_t gpio_pin, uint8_t direction)
{
    // 这里使用系统调用方式设置GPIO方向
    // 实际开发中应使用libgpiod或直接操作寄存器
    
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio_pin);
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        // 如果gpio未导出，先导出
        FILE *export_fp = fopen("/sys/class/gpio/export", "w");
        if (export_fp) {
            fprintf(export_fp, "%d", gpio_pin);
            fclose(export_fp);
            usleep(100000); // 等待导出完成
        }
        
        fp = fopen(path, "w");
        if (!fp) {
            printf("无法设置GPIO%d方向\n", gpio_pin);
            return -1;
        }
    }
    
    if (direction == 0) {
        fprintf(fp, "in");
    } else {
        fprintf(fp, "out");
    }
    
    fclose(fp);
    return 0;
}

/**
 * @brief 设置GPIO引脚电平（简化实现）
 */
int dht11_set_gpio_value(uint8_t gpio_pin, uint8_t value)
{
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_pin);
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        printf("无法设置GPIO%d值\n", gpio_pin);
        return -1;
    }
    
    fprintf(fp, "%d", value);
    fclose(fp);
    return 0;
}

/**
 * @brief 读取GPIO引脚电平（简化实现）
 */
int dht11_get_gpio_value(uint8_t gpio_pin)
{
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_pin);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        printf("无法读取GPIO%d值\n", gpio_pin);
        return -1;
    }
    
    char value_str[4];
    if (fgets(value_str, sizeof(value_str), fp) == NULL) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return atoi(value_str);
}

/**
 * @brief 微秒级延迟
 */
void dht11_delay_us(uint32_t microseconds)
{
    usleep(microseconds);
}

/**
 * @brief 毫秒级延迟
 */
void dht11_delay_ms(uint32_t milliseconds)
{
    usleep(milliseconds * 1000);
}