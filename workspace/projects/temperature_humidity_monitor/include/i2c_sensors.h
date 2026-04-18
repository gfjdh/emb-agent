/**
 * @file i2c_sensors.h
 * @brief I2C传感器通用接口
 * @author Embedded System Team
 * @date 2024-01-15
 * 
 * 支持多种I2C接口的传感器，包括BMP280、BH1750等
 */

#ifndef I2C_SENSORS_H
#define I2C_SENSORS_H

#include "sensor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// I2C传感器类型
typedef enum {
    I2C_SENSOR_BMP280 = 0,      /**< BMP280气压温度传感器 */
    I2C_SENSOR_BH1750,          /**< BH1750光照传感器 */
    I2C_SENSOR_SHT30,           /**< SHT30温湿度传感器 */
    I2C_SENSOR_MAX
} i2c_sensor_type_t;

// I2C配置结构体
typedef struct {
    uint8_t bus_number;         /**< I2C总线号 */
    uint8_t device_address;     /**< 设备地址 */
    uint32_t timeout_ms;        /**< 超时时间（毫秒） */
    uint32_t clock_speed;       /**< I2C时钟速度（Hz） */
} i2c_config_t;

// I2C传感器配置
typedef struct {
    i2c_sensor_type_t type;     /**< I2C传感器类型 */
    i2c_config_t i2c_config;    /**< I2C配置 */
    void *sensor_config;        /**< 传感器特定配置 */
} i2c_sensor_config_t;

// I2C传感器操作接口
typedef struct {
    /** 初始化I2C传感器 */
    sensor_status_t (*init)(i2c_sensor_config_t *config);
    
    /** 读取I2C传感器数据 */
    sensor_status_t (*read)(sensor_data_t *data);
    
    /** 释放I2C传感器资源 */
    void (*cleanup)(void);
    
    /** 获取I2C传感器信息 */
    const char* (*get_info)(void);
    
    /** I2C传感器校准 */
    sensor_status_t (*calibrate)(float *params, int param_count);
} i2c_sensor_ops_t;

/**
 * @brief 初始化I2C总线
 * 
 * @param bus_number I2C总线号
 * @param clock_speed 时钟速度（Hz）
 * @return int 成功返回文件描述符，失败返回-1
 */
int i2c_init_bus(uint8_t bus_number, uint32_t clock_speed);

/**
 * @brief 关闭I2C总线
 * 
 * @param fd I2C文件描述符
 */
void i2c_close_bus(int fd);

/**
 * @brief I2C写一个字节
 * 
 * @param fd I2C文件描述符
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @param value 要写入的值
 * @return int 成功返回0，失败返回-1
 */
int i2c_write_byte(int fd, uint8_t addr, uint8_t reg, uint8_t value);

/**
 * @brief I2C读一个字节
 * 
 * @param fd I2C文件描述符
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @param value 输出值指针
 * @return int 成功返回0，失败返回-1
 */
int i2c_read_byte(int fd, uint8_t addr, uint8_t reg, uint8_t *value);

/**
 * @brief I2C读多个字节
 * 
 * @param fd I2C文件描述符
 * @param addr 设备地址
 * @param reg 起始寄存器地址
 * @param buffer 输出缓冲区
 * @param length 要读取的字节数
 * @return int 成功返回0，失败返回-1
 */
int i2c_read_bytes(int fd, uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length);

/**
 * @brief I2C写多个字节
 * 
 * @param fd I2C文件描述符
 * @param addr 设备地址
 * @param reg 起始寄存器地址
 * @param buffer 数据缓冲区
 * @param length 要写入的字节数
 * @return int 成功返回0，失败返回-1
 */
int i2c_write_bytes(int fd, uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length);

/**
 * @brief I2C读16位数据
 * 
 * @param fd I2C文件描述符
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @param value 输出值指针
 * @return int 成功返回0，失败返回-1
 */
int i2c_read_word(int fd, uint8_t addr, uint8_t reg, uint16_t *value);

/**
 * @brief I2C写16位数据
 * 
 * @param fd I2C文件描述符
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @param value 要写入的值
 * @return int 成功返回0，失败返回-1
 */
int i2c_write_word(int fd, uint8_t addr, uint8_t reg, uint16_t value);

// BMP280传感器相关
#define BMP280_DEFAULT_ADDRESS 0x76     /**< BMP280默认I2C地址 */
#define BMP280_ALT_ADDRESS 0x77         /**< BMP280备用I2C地址 */

// BMP280寄存器定义
#define BMP280_REG_ID 0xD0
#define BMP280_REG_RESET 0xE0
#define BMP280_REG_STATUS 0xF3
#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_CONFIG 0xF5
#define BMP280_REG_PRESS_MSB 0xF7
#define BMP280_REG_TEMP_MSB 0xFA

// BMP280校准寄存器
#define BMP280_REG_CALIB00 0x88
#define BMP280_REG_CALIB25 0xA1

// BMP280操作模式
typedef enum {
    BMP280_MODE_SLEEP = 0,
    BMP280_MODE_FORCED = 1,
    BMP280_MODE_NORMAL = 3
} bmp280_mode_t;

// BMP280过采样设置
typedef enum {
    BMP280_OSRS_SKIP = 0,
    BMP280_OSRS_1 = 1,
    BMP280_OSRS_2 = 2,
    BMP280_OSRS_4 = 3,
    BMP280_OSRS_8 = 4,
    BMP280_OSRS_16 = 5
} bmp280_oversampling_t;

// BMP280滤波器设置
typedef enum {
    BMP280_FILTER_OFF = 0,
    BMP280_FILTER_2 = 1,
    BMP280_FILTER_4 = 2,
    BMP280_FILTER_8 = 3,
    BMP280_FILTER_16 = 4
} bmp280_filter_t;

// BMP280配置结构体
typedef struct {
    bmp280_mode_t mode;                 /**< 操作模式 */
    bmp280_oversampling_t temp_osrs;    /**< 温度过采样 */
    bmp280_oversampling_t press_osrs;   /**< 气压过采样 */
    bmp280_filter_t filter;             /**< 滤波器设置 */
    uint8_t standby_time;               /**< 待机时间 */
} bmp280_config_t;

// BMP280校准数据
typedef struct {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    int32_t t_fine;
} bmp280_calib_data_t;

// BMP280操作接口
extern const i2c_sensor_ops_t bmp280_ops;

/**
 * @brief 初始化BMP280传感器
 * 
 * @param config I2C传感器配置
 * @return sensor_status_t 初始化状态
 */
sensor_status_t bmp280_init(i2c_sensor_config_t *config);

/**
 * @brief 读取BMP280传感器数据
 * 
 * @param data 输出数据指针
 * @return sensor_status_t 读取状态
 */
sensor_status_t bmp280_read(sensor_data_t *data);

/**
 * @brief 获取BMP280传感器信息
 * 
 * @return const char* 传感器信息字符串
 */
const char* bmp280_get_info(void);

// BH1750传感器相关
#define BH1750_DEFAULT_ADDRESS 0x23     /**< BH1750默认I2C地址 */
#define BH1750_ALT_ADDRESS 0x5C         /**< BH1750备用I2C地址 */

// BH1750指令定义
#define BH1750_POWER_DOWN 0x00
#define BH1750_POWER_ON 0x01
#define BH1750_RESET 0x07
#define BH1750_CONTINUOUS_H_RES_MODE 0x10
#define BH1750_CONTINUOUS_H_RES_MODE2 0x11
#define BH1750_CONTINUOUS_L_RES_MODE 0x13
#define BH1750_ONE_TIME_H_RES_MODE 0x20
#define BH1750_ONE_TIME_H_RES_MODE2 0x21
#define BH1750_ONE_TIME_L_RES_MODE 0x23

// BH1750测量模式
typedef enum {
    BH1750_MODE_CONT_HIGH = 0,          /**< 连续高分辨率模式 */
    BH1750_MODE_CONT_HIGH2,             /**< 连续高分辨率模式2 */
    BH1750_MODE_CONT_LOW,               /**< 连续低分辨率模式 */
    BH1750_MODE_ONE_HIGH,               /**< 单次高分辨率模式 */
    BH1750_MODE_ONE_HIGH2,              /**< 单次高分辨率模式2 */
    BH1750_MODE_ONE_LOW                 /**< 单次低分辨率模式 */
} bh1750_mode_t;

// BH1750配置结构体
typedef struct {
    bh1750_mode_t mode;                 /**< 测量模式 */
    uint8_t measurement_time;           /**< 测量时间 */
} bh1750_config_t;

// BH1750操作接口
extern const i2c_sensor_ops_t bh1750_ops;

/**
 * @brief 初始化BH1750传感器
 * 
 * @param config I2C传感器配置
 * @return sensor_status_t 初始化状态
 */
sensor_status_t bh1750_init(i2c_sensor_config_t *config);

/**
 * @brief 读取BH1750传感器数据
 * 
 * @param data 输出数据指针
 * @return sensor_status_t 读取状态
 */
sensor_status_t bh1750_read(sensor_data_t *data);

/**
 * @brief 获取BH1750传感器信息
 * 
 * @return const char* 传感器信息字符串
 */
const char* bh1750_get_info(void);

/**
 * @brief 创建I2C传感器配置
 * 
 * @param type I2C传感器类型
 * @param bus_number I2C总线号
 * @param address 设备地址
 * @return i2c_sensor_config_t I2C传感器配置
 */
i2c_sensor_config_t i2c_sensor_create_config(i2c_sensor_type_t type, 
                                            uint8_t bus_number, 
                                            uint8_t address);

/**
 * @brief 获取I2C传感器类型字符串
 * 
 * @param type I2C传感器类型
 * @return const char* 类型字符串
 */
const char* i2c_sensor_type_to_string(i2c_sensor_type_t type);

/**
 * @brief 获取I2C传感器默认地址
 * 
 * @param type I2C传感器类型
 * @return uint8_t 默认I2C地址
 */
uint8_t i2c_sensor_get_default_address(i2c_sensor_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* I2C_SENSORS_H */