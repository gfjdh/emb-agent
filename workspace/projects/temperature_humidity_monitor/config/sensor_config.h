/**
 * @file sensor_config.h
 * @brief 传感器系统配置
 * @author Embedded System Team
 * @date 2024-01-15
 * 
 * 温湿度监测系统传感器配置参数
 */

#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <stdint.h>

// 系统配置
#define SYSTEM_NAME         "飞腾温湿度监测系统"
#define SYSTEM_VERSION      "1.0.0"
#define SYSTEM_AUTHOR       "Embedded System Team"

// 采样配置
#define DEFAULT_SAMPLE_INTERVAL     2000    // 默认采样间隔（毫秒）
#define MIN_SAMPLE_INTERVAL         1000    // 最小采样间隔（毫秒）
#define MAX_SAMPLE_INTERVAL         60000   // 最大采样间隔（毫秒）

#define DEFAULT_LOG_INTERVAL        3600    // 默认日志记录间隔（秒）
#define MIN_LOG_INTERVAL            60      // 最小日志记录间隔（秒）
#define MAX_LOG_INTERVAL            86400   // 最大日志记录间隔（秒）

// 数据存储配置
#define MAX_HISTORY_DAYS            30      // 最大历史数据保存天数
#define DATA_FILE_PREFIX            "sensor_data"
#define DATA_FILE_SUFFIX            ".csv"
#define BACKUP_FILE_SUFFIX          ".bak"

// 报警阈值配置（默认值）
#define DEFAULT_TEMP_HIGH           35.0f   // 温度高阈值（摄氏度）
#define DEFAULT_TEMP_LOW            10.0f   // 温度低阈值（摄氏度）
#define DEFAULT_HUMIDITY_HIGH       80.0f   // 湿度高阈值（百分比）
#define DEFAULT_HUMIDITY_LOW        20.0f   // 湿度低阈值（百分比）
#define DEFAULT_PRESSURE_HIGH       1100.0f // 气压高阈值（hPa）
#define DEFAULT_PRESSURE_LOW        900.0f  // 气压低阈值（hPa）
#define DEFAULT_LIGHT_HIGH          10000.0f // 光照高阈值（lux）
#define DEFAULT_LIGHT_LOW           10.0f   // 光照低阈值（lux）

// 传感器精度配置
#define TEMPERATURE_PRECISION       1       // 温度精度（小数点后位数）
#define HUMIDITY_PRECISION          1       // 湿度精度（小数点后位数）
#define PRESSURE_PRECISION          2       // 气压精度（小数点后位数）
#define LIGHT_PRECISION             0       // 光照精度（小数点后位数）

// 数据验证配置
#define MIN_TEMPERATURE             -40.0f  // 最小有效温度
#define MAX_TEMPERATURE             85.0f   // 最大有效温度
#define MIN_HUMIDITY                0.0f    // 最小有效湿度
#define MAX_HUMIDITY                100.0f  // 最大有效湿度
#define MIN_PRESSURE                300.0f  // 最小有效气压
#define MAX_PRESSURE                1100.0f // 最大有效气压
#define MIN_LIGHT                   0.0f    // 最小有效光照
#define MAX_LIGHT                   65535.0f // 最大有效光照

// 传感器类型配置
typedef enum {
    SENSOR_ENABLE_DHT11     = 0x01, // 启用DHT11传感器
    SENSOR_ENABLE_DHT22     = 0x02, // 启用DHT22传感器
    SENSOR_ENABLE_BMP280    = 0x04, // 启用BMP280传感器
    SENSOR_ENABLE_BH1750    = 0x08, // 启用BH1750传感器
    SENSOR_ENABLE_ALL       = 0x0F  // 启用所有传感器
} sensor_enable_mask_t;

// 系统模式配置
typedef enum {
    SYSTEM_MODE_NORMAL      = 0,    // 正常模式
    SYSTEM_MODE_CALIBRATION = 1,    // 校准模式
    SYSTEM_MODE_TEST        = 2,    // 测试模式
    SYSTEM_MODE_MAINTENANCE = 3     // 维护模式
} system_mode_t;

// 数据输出格式
typedef enum {
    OUTPUT_FORMAT_JSON      = 0,    // JSON格式
    OUTPUT_FORMAT_CSV       = 1,    // CSV格式
    OUTPUT_FORMAT_XML       = 2,    // XML格式
    OUTPUT_FORMAT_PLAIN     = 3     // 纯文本格式
} output_format_t;

// 网络配置
#define NETWORK_ENABLED             1       // 网络功能使能
#define NETWORK_PORT                8080    // Web服务器端口
#define NETWORK_TIMEOUT             30      // 网络超时（秒）
#define NETWORK_MAX_CLIENTS         5       // 最大客户端连接数

// Web界面配置
#define WEB_ENABLED                 1       // Web界面使能
#define WEB_REFRESH_INTERVAL        5000    // Web页面刷新间隔（毫秒）
#define WEB_HISTORY_POINTS          100     // 历史数据点数

// 数据库配置
#define DATABASE_ENABLED            0       // 数据库功能使能
#define DATABASE_PATH               "/var/lib/temp_humidity.db"
#define DATABASE_BACKUP_INTERVAL    86400   // 数据库备份间隔（秒）

// 系统日志配置
#define LOG_LEVEL_INFO              0       // 信息级别
#define LOG_LEVEL_WARNING           1       // 警告级别
#define LOG_LEVEL_ERROR             2       // 错误级别
#define LOG_LEVEL_DEBUG             3       // 调试级别

#define DEFAULT_LOG_LEVEL           LOG_LEVEL_INFO
#define LOG_FILE_MAX_SIZE           10485760 // 日志文件最大大小（10MB）
#define LOG_FILE_MAX_COUNT          10      // 日志文件最大数量

// 电源管理配置
#define POWER_SAVE_ENABLED          1       // 节能模式使能
#define POWER_SAVE_TIMEOUT          300     // 进入节能模式超时（秒）
#define POWER_WAKEUP_INTERVAL       60      // 节能模式唤醒间隔（秒）

// 系统配置结构体
typedef struct {
    // 系统信息
    char system_name[64];
    char system_version[16];
    system_mode_t system_mode;
    
    // 采样配置
    uint32_t sample_interval_ms;
    uint32_t log_interval_sec;
    
    // 传感器使能
    sensor_enable_mask_t enabled_sensors;
    
    // 报警阈值
    struct {
        float temperature_high;
        float temperature_low;
        float humidity_high;
        float humidity_low;
        float pressure_high;
        float pressure_low;
        float light_high;
        float light_low;
        uint8_t enabled;
    } alarm_threshold;
    
    // 数据输出
    output_format_t output_format;
    char output_file[256];
    
    // 网络配置
    struct {
        uint8_t enabled;
        uint16_t port;
        uint32_t timeout_sec;
        char hostname[64];
    } network;
    
    // Web配置
    struct {
        uint8_t enabled;
        uint32_t refresh_interval_ms;
        uint16_t history_points;
    } web;
    
    // 日志配置
    struct {
        uint8_t level;
        char file[256];
        uint32_t max_size;
        uint16_t max_count;
    } logging;
    
    // 电源管理
    struct {
        uint8_t enabled;
        uint32_t timeout_sec;
        uint32_t wakeup_interval_sec;
    } power_management;
    
    // 校准数据
    struct {
        float temperature_offset;
        float humidity_offset;
        float pressure_offset;
        float light_offset;
    } calibration;
    
    // 系统状态
    uint8_t daemon_mode;
    uint8_t verbose_mode;
    uint8_t auto_start;
} system_config_t;

// 默认系统配置
extern const system_config_t default_system_config;

// 函数声明

/**
 * @brief 加载系统配置
 * 
 * @param config_file 配置文件路径
 * @param config 输出配置指针
 * @return int 成功返回0，失败返回-1
 */
int load_system_config(const char *config_file, system_config_t *config);

/**
 * @brief 保存系统配置
 * 
 * @param config_file 配置文件路径
 * @param config 配置指针
 * @return int 成功返回0，失败返回-1
 */
int save_system_config(const char *config_file, const system_config_t *config);

/**
 * @brief 验证配置参数
 * 
 * @param config 配置指针
 * @return int 有效返回0，无效返回-1
 */
int validate_system_config(const system_config_t *config);

/**
 * @brief 重置为默认配置
 * 
 * @param config 配置指针
 */
void reset_to_default_config(system_config_t *config);

/**
 * @brief 打印配置信息
 * 
 * @param config 配置指针
 */
void print_system_config(const system_config_t *config);

/**
 * @brief 获取配置参数描述
 * 
 * @param param 参数名
 * @return const char* 参数描述
 */
const char* get_config_param_description(const char *param);

/**
 * @brief 更新配置参数
 * 
 * @param config 配置指针
 * @param param 参数名
 * @param value 参数值
 * @return int 成功返回0，失败返回-1
 */
int update_config_param(system_config_t *config, const char *param, const char *value);

/**
 * @brief 导出配置到文件
 * 
 * @param config 配置指针
 * @param filename 文件名
 * @param format 格式（json, ini, xml）
 * @return int 成功返回0，失败返回-1
 */
int export_config(const system_config_t *config, const char *filename, const char *format);

/**
 * @brief 导入配置从文件
 * 
 * @param config 配置指针
 * @param filename 文件名
 * @param format 格式（json, ini, xml）
 * @return int 成功返回0，失败返回-1
 */
int import_config(system_config_t *config, const char *filename, const char *format);

#endif /* SENSOR_CONFIG_H */