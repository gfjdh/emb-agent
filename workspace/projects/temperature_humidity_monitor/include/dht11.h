/**
 * @file dht11.h
 * @brief DHT11温湿度传感器驱动接口
 * @author Embedded System Team
 * @date 2024-01-15
 * 
 * DHT11数字温湿度传感器驱动，使用单总线协议
 * 温度范围: 0-50°C ±2°C
 * 湿度范围: 20-90%RH ±5%RH
 */

#ifndef DHT11_H
#define DHT11_H

#include "sensor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// DHT11特定配置
typedef struct {
    uint8_t gpio_pin;           /**< GPIO引脚号 */
    uint32_t timeout_us;        /**< 超时时间（微秒） */
    uint8_t pull_up_resistor;   /**< 上拉电阻使能 */
} dht11_config_t;

// DHT11私有数据结构
typedef struct {
    dht11_config_t config;      /**< DHT11配置 */
    uint32_t last_read_time;    /**< 最后读取时间 */
    uint8_t checksum_errors;    /**< 校验和错误计数 */
    uint8_t timeout_errors;     /**< 超时错误计数 */
} dht11_private_t;

/**
 * @brief DHT11传感器操作接口
 */
extern const sensor_ops_t dht11_ops;

/**
 * @brief 初始化DHT11传感器
 * 
 * @param config 传感器配置
 * @return sensor_status_t 初始化状态
 */
sensor_status_t dht11_init(sensor_config_t *config);

/**
 * @brief 读取DHT11传感器数据
 * 
 * @param data 输出数据指针
 * @return sensor_status_t 读取状态
 */
sensor_status_t dht11_read(sensor_data_t *data);

/**
 * @brief 清理DHT11传感器资源
 */
void dht11_cleanup(void);

/**
 * @brief 获取DHT11传感器信息
 * 
 * @return const char* 传感器信息字符串
 */
const char* dht11_get_info(void);

/**
 * @brief DHT11传感器校准（DHT11不支持软件校准）
 * 
 * @param params 参数数组（未使用）
 * @param param_count 参数数量（未使用）
 * @return sensor_status_t 总是返回SENSOR_STATUS_OK
 */
sensor_status_t dht11_calibrate(float *params, int param_count);

/**
 * @brief 创建DHT11传感器配置
 * 
 * @param gpio_pin GPIO引脚号
 * @return sensor_config_t DHT11传感器配置
 */
sensor_config_t dht11_create_config(uint8_t gpio_pin);

/**
 * @brief 验证DHT11数据校验和
 * 
 * @param data 接收到的数据数组（5字节）
 * @return uint8_t 校验和正确返回1，否则返回0
 */
uint8_t dht11_verify_checksum(uint8_t data[5]);

/**
 * @brief 解析DHT11原始数据
 * 
 * @param raw_data 原始数据数组（5字节）
 * @param temperature 输出温度值（摄氏度）
 * @param humidity 输出湿度值（百分比）
 * @return sensor_status_t 解析状态
 */
sensor_status_t dht11_parse_data(uint8_t raw_data[5], float *temperature, float *humidity);

/**
 * @brief 发送开始信号到DHT11
 * 
 * @param gpio_pin GPIO引脚号
 * @return sensor_status_t 发送状态
 */
sensor_status_t dht11_send_start_signal(uint8_t gpio_pin);

/**
 * @brief 等待DHT11响应
 * 
 * @param gpio_pin GPIO引脚号
 * @param timeout_us 超时时间（微秒）
 * @return sensor_status_t 响应状态
 */
sensor_status_t dht11_wait_response(uint8_t gpio_pin, uint32_t timeout_us);

/**
 * @brief 读取DHT11数据位
 * 
 * @param gpio_pin GPIO引脚号
 * @param timeout_us 超时时间（微秒）
 * @return int 数据位值（0或1），超时返回-1
 */
int dht11_read_bit(uint8_t gpio_pin, uint32_t timeout_us);

/**
 * @brief 读取DHT11一个字节
 * 
 * @param gpio_pin GPIO引脚号
 * @param timeout_us 超时时间（微秒）
 * @param byte 输出字节值
 * @return sensor_status_t 读取状态
 */
sensor_status_t dht11_read_byte(uint8_t gpio_pin, uint32_t timeout_us, uint8_t *byte);

/**
 * @brief 读取DHT11所有数据（5字节）
 * 
 * @param gpio_pin GPIO引脚号
 * @param timeout_us 超时时间（微秒）
 * @param data 输出数据数组（5字节）
 * @return sensor_status_t 读取状态
 */
sensor_status_t dht11_read_all_data(uint8_t gpio_pin, uint32_t timeout_us, uint8_t data[5]);

// DHT11常量定义
#define DHT11_MIN_INTERVAL_MS 2000      /**< DHT11最小读取间隔（毫秒） */
#define DHT11_TIMEOUT_US 100            /**< DHT11超时时间（微秒） */
#define DHT11_DATA_BITS 40              /**< DHT11数据位数 */
#define DHT11_DATA_BYTES 5              /**< DHT11数据字节数 */

// DHT11引脚状态
#define DHT11_PIN_HIGH 1
#define DHT11_PIN_LOW 0

// DHT11错误码
#define DHT11_ERROR_TIMEOUT -1
#define DHT11_ERROR_CHECKSUM -2
#define DHT11_ERROR_COMM -3

/**
 * @brief 设置GPIO引脚方向
 * 
 * @param gpio_pin GPIO引脚号
 * @param direction 方向（0:输入, 1:输出）
 * @return int 成功返回0，失败返回-1
 */
int dht11_set_gpio_direction(uint8_t gpio_pin, uint8_t direction);

/**
 * @brief 设置GPIO引脚电平
 * 
 * @param gpio_pin GPIO引脚号
 * @param value 电平值（0:低电平, 1:高电平）
 * @return int 成功返回0，失败返回-1
 */
int dht11_set_gpio_value(uint8_t gpio_pin, uint8_t value);

/**
 * @brief 读取GPIO引脚电平
 * 
 * @param gpio_pin GPIO引脚号
 * @return int 电平值（0:低电平, 1:高电平），失败返回-1
 */
int dht11_get_gpio_value(uint8_t gpio_pin);

/**
 * @brief 微秒级延迟
 * 
 * @param microseconds 延迟微秒数
 */
void dht11_delay_us(uint32_t microseconds);

/**
 * @brief 毫秒级延迟
 * 
 * @param milliseconds 延迟毫秒数
 */
void dht11_delay_ms(uint32_t milliseconds);

#ifdef __cplusplus
}
#endif

#endif /* DHT11_H */