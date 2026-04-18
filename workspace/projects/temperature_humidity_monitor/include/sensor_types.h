/**
 * @file sensor_types.h
 * @brief 传感器类型定义和数据结构
 * @author Embedded System Team
 * @date 2024-01-15
 * 
 * 定义温湿度监测系统中使用的传感器类型、数据结构和接口
 */

#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 传感器类型枚举
 */
typedef enum {
    SENSOR_TYPE_DHT11 = 0,      /**< DHT11温湿度传感器 */
    SENSOR_TYPE_DHT22,          /**< DHT22/AM2302温湿度传感器 */
    SENSOR_TYPE_BMP280,         /**< BMP280气压温度传感器 */
    SENSOR_TYPE_BH1750,         /**< BH1750光照传感器 */
    SENSOR_TYPE_DS18B20,        /**< DS18B20温度传感器 */
    SENSOR_TYPE_MAX             /**< 传感器类型数量 */
} sensor_type_t;

/**
 * @brief 传感器状态枚举
 */
typedef enum {
    SENSOR_STATUS_OK = 0,       /**< 传感器正常 */
    SENSOR_STATUS_ERROR,        /**< 传感器错误 */
    SENSOR_STATUS_TIMEOUT,      /**< 传感器超时 */
    SENSOR_STATUS_CHECKSUM_ERROR, /**< 校验和错误 */
    SENSOR_STATUS_NOT_CONNECTED, /**< 传感器未连接 */
    SENSOR_STATUS_INIT_FAILED   /**< 初始化失败 */
} sensor_status_t;

/**
 * @brief 传感器配置结构体
 */
typedef struct {
    sensor_type_t type;         /**< 传感器类型 */
    uint8_t gpio_pin;           /**< GPIO引脚号（GPIO传感器使用） */
    uint8_t i2c_address;        /**< I2C地址（I2C传感器使用） */
    uint8_t i2c_bus;            /**< I2C总线号 */
    uint32_t sample_interval;   /**< 采样间隔（毫秒） */
    char name[32];              /**< 传感器名称 */
    void *private_data;         /**< 私有数据指针 */
} sensor_config_t;

/**
 * @brief 传感器数据结构体
 */
typedef struct {
    time_t timestamp;           /**< 时间戳 */
    float temperature;          /**< 温度（摄氏度） */
    float humidity;             /**< 湿度（百分比） */
    float pressure;             /**< 气压（hPa，可选） */
    float light;                /**< 光照强度（lux，可选） */
    sensor_type_t sensor_type;  /**< 传感器类型 */
    sensor_status_t status;     /**< 传感器状态 */
    uint8_t sensor_id;          /**< 传感器ID */
} sensor_data_t;

/**
 * @brief 传感器操作接口结构体
 */
typedef struct {
    /** 初始化传感器 */
    sensor_status_t (*init)(sensor_config_t *config);
    
    /** 读取传感器数据 */
    sensor_status_t (*read)(sensor_data_t *data);
    
    /** 释放传感器资源 */
    void (*cleanup)(void);
    
    /** 获取传感器信息 */
    const char* (*get_info)(void);
    
    /** 传感器校准（可选） */
    sensor_status_t (*calibrate)(float *params, int param_count);
} sensor_ops_t;

/**
 * @brief 传感器实例结构体
 */
typedef struct {
    sensor_config_t config;     /**< 传感器配置 */
    sensor_ops_t ops;           /**< 传感器操作接口 */
    sensor_data_t last_data;    /**< 最后一次读取的数据 */
    uint32_t read_count;        /**< 读取次数 */
    uint32_t error_count;       /**< 错误次数 */
    uint8_t is_initialized;     /**< 初始化标志 */
} sensor_instance_t;

/**
 * @brief 报警阈值结构体
 */
typedef struct {
    float temp_high;            /**< 温度高阈值 */
    float temp_low;             /**< 温度低阈值 */
    float humidity_high;        /**< 湿度高阈值 */
    float humidity_low;         /**< 湿度低阈值 */
    uint8_t enabled;            /**< 报警使能标志 */
} alarm_threshold_t;

/**
 * @brief 系统配置结构体
 */
typedef struct {
    uint8_t sensor_count;               /**< 传感器数量 */
    sensor_instance_t *sensors;         /**< 传感器实例数组 */
    alarm_threshold_t alarm_threshold;  /**< 报警阈值 */
    uint32_t log_interval;              /**< 日志记录间隔（秒） */
    char log_file[256];                 /**< 日志文件路径 */
    uint8_t daemon_mode;                /**< 守护进程模式标志 */
    uint8_t verbose;                    /**< 详细输出标志 */
} system_config_t;

// 常量定义
#define MAX_SENSORS 8                    /**< 最大传感器数量 */
#define DEFAULT_SAMPLE_INTERVAL 2000     /**< 默认采样间隔（毫秒） */
#define DEFAULT_LOG_INTERVAL 3600        /**< 默认日志间隔（秒） */

// 温度单位转换
#define CELSIUS_TO_FAHRENHEIT(c) ((c) * 9.0 / 5.0 + 32.0)
#define FAHRENHEIT_TO_CELSIUS(f) (((f) - 32.0) * 5.0 / 9.0)

// 错误码定义
#define SENSOR_SUCCESS 0
#define SENSOR_ERROR_INIT -1
#define SENSOR_ERROR_READ -2
#define SENSOR_ERROR_PARAM -3
#define SENSOR_ERROR_MEMORY -4
#define SENSOR_ERROR_IO -5

// 函数声明

/**
 * @brief 创建传感器实例
 * 
 * @param type 传感器类型
 * @param config 传感器配置
 * @return sensor_instance_t* 传感器实例指针，失败返回NULL
 */
sensor_instance_t* sensor_create(sensor_type_t type, sensor_config_t *config);

/**
 * @brief 销毁传感器实例
 * 
 * @param sensor 传感器实例指针
 */
void sensor_destroy(sensor_instance_t *sensor);

/**
 * @brief 读取传感器数据
 * 
 * @param sensor 传感器实例
 * @param data 输出数据指针
 * @return sensor_status_t 读取状态
 */
sensor_status_t sensor_read_data(sensor_instance_t *sensor, sensor_data_t *data);

/**
 * @brief 获取传感器状态字符串
 * 
 * @param status 传感器状态
 * @return const char* 状态字符串
 */
const char* sensor_status_to_string(sensor_status_t status);

/**
 * @brief 获取传感器类型字符串
 * 
 * @param type 传感器类型
 * @return const char* 类型字符串
 */
const char* sensor_type_to_string(sensor_type_t type);

/**
 * @brief 检查是否需要报警
 * 
 * @param data 传感器数据
 * @param threshold 报警阈值
 * @return int 报警标志位（bit0:温度高, bit1:温度低, bit2:湿度高, bit3:湿度低）
 */
int check_alarm(const sensor_data_t *data, const alarm_threshold_t *threshold);

/**
 * @brief 格式化传感器数据为JSON字符串
 * 
 * @param data 传感器数据
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return int 写入的字符数，失败返回-1
 */
int format_sensor_data_json(const sensor_data_t *data, char *buffer, size_t size);

/**
 * @brief 格式化传感器数据为CSV字符串
 * 
 * @param data 传感器数据
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return int 写入的字符数，失败返回-1
 */
int format_sensor_data_csv(const sensor_data_t *data, char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_TYPES_H */