/**
 * @file sensor_manager.c
 * @brief 传感器管理器实现
 * @author Embedded System Team
 * @date 2024-01-15
 * 
 * 管理多个传感器的初始化、数据采集和资源释放
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "sensor_types.h"
#include "dht11.h"
#include "i2c_sensors.h"

// 传感器管理器私有数据结构
typedef struct {
    sensor_instance_t *sensors;         /**< 传感器实例数组 */
    uint8_t sensor_count;               /**< 传感器数量 */
    uint8_t max_sensors;                /**< 最大传感器数量 */
    pthread_mutex_t mutex;              /**< 互斥锁 */
    uint32_t total_reads;               /**< 总读取次数 */
    uint32_t total_errors;              /**< 总错误次数 */
} sensor_manager_t;

static sensor_manager_t manager = {0};

// DHT11操作接口实现
static const sensor_ops_t dht11_ops = {
    .init = dht11_init,
    .read = dht11_read,
    .cleanup = dht11_cleanup,
    .get_info = dht11_get_info,
    .calibrate = dht11_calibrate
};

// DHT22操作接口（与DHT11相同）
static const sensor_ops_t dht22_ops = {
    .init = dht11_init,      // 使用相同的初始化函数
    .read = dht11_read,      // 使用相同的读取函数
    .cleanup = dht11_cleanup,
    .get_info = dht11_get_info,
    .calibrate = dht11_calibrate
};

/**
 * @brief 初始化传感器管理器
 * 
 * @param max_sensors 最大传感器数量
 * @return int 成功返回0，失败返回-1
 */
int sensor_manager_init(uint8_t max_sensors)
{
    if (max_sensors == 0 || max_sensors > MAX_SENSORS) {
        fprintf(stderr, "无效的最大传感器数量: %d\n", max_sensors);
        return -1;
    }
    
    // 分配传感器数组内存
    manager.sensors = calloc(max_sensors, sizeof(sensor_instance_t));
    if (!manager.sensors) {
        fprintf(stderr, "无法分配传感器内存\n");
        return -1;
    }
    
    manager.max_sensors = max_sensors;
    manager.sensor_count = 0;
    manager.total_reads = 0;
    manager.total_errors = 0;
    
    // 初始化互斥锁
    if (pthread_mutex_init(&manager.mutex, NULL) != 0) {
        fprintf(stderr, "无法初始化互斥锁\n");
        free(manager.sensors);
        manager.sensors = NULL;
        return -1;
    }
    
    printf("传感器管理器初始化完成，最大传感器数: %d\n", max_sensors);
    return 0;
}

/**
 * @brief 清理传感器管理器资源
 */
void sensor_manager_cleanup(void)
{
    pthread_mutex_lock(&manager.mutex);
    
    // 清理所有传感器
    for (int i = 0; i < manager.sensor_count; i++) {
        sensor_instance_t *sensor = &manager.sensors[i];
        if (sensor->is_initialized && sensor->ops.cleanup) {
            sensor->ops.cleanup();
        }
    }
    
    // 释放传感器数组
    if (manager.sensors) {
        free(manager.sensors);
        manager.sensors = NULL;
    }
    
    manager.sensor_count = 0;
    manager.max_sensors = 0;
    
    pthread_mutex_unlock(&manager.mutex);
    
    // 销毁互斥锁
    pthread_mutex_destroy(&manager.mutex);
    
    printf("传感器管理器资源已清理\n");
}

/**
 * @brief 添加传感器
 * 
 * @param type 传感器类型
 * @param config 传感器配置
 * @return int 成功返回传感器ID，失败返回-1
 */
int sensor_manager_add_sensor(sensor_type_t type, sensor_config_t *config)
{
    if (!config) {
        fprintf(stderr, "传感器配置为空\n");
        return -1;
    }
    
    pthread_mutex_lock(&manager.mutex);
    
    if (manager.sensor_count >= manager.max_sensors) {
        fprintf(stderr, "已达到最大传感器数量: %d\n", manager.max_sensors);
        pthread_mutex_unlock(&manager.mutex);
        return -1;
    }
    
    // 查找空闲位置
    int sensor_id = -1;
    for (int i = 0; i < manager.max_sensors; i++) {
        if (!manager.sensors[i].is_initialized) {
            sensor_id = i;
            break;
        }
    }
    
    if (sensor_id == -1) {
        fprintf(stderr, "未找到空闲传感器位置\n");
        pthread_mutex_unlock(&manager.mutex);
        return -1;
    }
    
    // 设置传感器实例
    sensor_instance_t *sensor = &manager.sensors[sensor_id];
    memset(sensor, 0, sizeof(sensor_instance_t));
    
    // 复制配置
    memcpy(&sensor->config, config, sizeof(sensor_config_t));
    
    // 设置操作接口
    switch (type) {
        case SENSOR_TYPE_DHT11:
            sensor->ops = dht11_ops;
            break;
            
        case SENSOR_TYPE_DHT22:
            sensor->ops = dht22_ops;
            break;
            
        case SENSOR_TYPE_BMP280:
            // 这里可以设置BMP280操作接口
            // sensor->ops = bmp280_ops;
            break;
            
        case SENSOR_TYPE_BH1750:
            // 这里可以设置BH1750操作接口
            // sensor->ops = bh1750_ops;
            break;
            
        default:
            fprintf(stderr, "不支持的传感器类型: %d\n", type);
            pthread_mutex_unlock(&manager.mutex);
            return -1;
    }
    
    // 初始化传感器
    if (sensor->ops.init) {
        sensor_status_t status = sensor->ops.init(config);
        if (status != SENSOR_STATUS_OK) {
            fprintf(stderr, "传感器初始化失败: %s\n", sensor_status_to_string(status));
            pthread_mutex_unlock(&manager.mutex);
            return -1;
        }
    }
    
    sensor->is_initialized = 1;
    sensor->read_count = 0;
    sensor->error_count = 0;
    
    manager.sensor_count++;
    
    pthread_mutex_unlock(&manager.mutex);
    
    printf("传感器添加成功，ID: %d，类型: %s\n", 
           sensor_id, sensor_type_to_string(type));
    
    return sensor_id;
}

/**
 * @brief 移除传感器
 * 
 * @param sensor_id 传感器ID
 * @return int 成功返回0，失败返回-1
 */
int sensor_manager_remove_sensor(int sensor_id)
{
    if (sensor_id < 0 || sensor_id >= manager.max_sensors) {
        fprintf(stderr, "无效的传感器ID: %d\n", sensor_id);
        return -1;
    }
    
    pthread_mutex_lock(&manager.mutex);
    
    sensor_instance_t *sensor = &manager.sensors[sensor_id];
    if (!sensor->is_initialized) {
        fprintf(stderr, "传感器 %d 未初始化\n", sensor_id);
        pthread_mutex_unlock(&manager.mutex);
        return -1;
    }
    
    // 清理传感器资源
    if (sensor->ops.cleanup) {
        sensor->ops.cleanup();
    }
    
    // 重置传感器实例
    memset(sensor, 0, sizeof(sensor_instance_t));
    manager.sensor_count--;
    
    pthread_mutex_unlock(&manager.mutex);
    
    printf("传感器 %d 已移除\n", sensor_id);
    return 0;
}

/**
 * @brief 读取所有传感器数据
 * 
 * @param data_array 输出数据数组
 * @param max_data 最大数据数量
 * @return int 成功读取的传感器数量
 */
int sensor_manager_read_all(sensor_data_t *data_array, int max_data)
{
    if (!data_array || max_data <= 0) {
        fprintf(stderr, "无效的输出参数\n");
        return 0;
    }
    
    pthread_mutex_lock(&manager.mutex);
    
    int read_count = 0;
    
    for (int i = 0; i < manager.max_sensors && read_count < max_data; i++) {
        sensor_instance_t *sensor = &manager.sensors[i];
        
        if (!sensor->is_initialized || !sensor->ops.read) {
            continue;
        }
        
        sensor_data_t *data = &data_array[read_count];
        memset(data, 0, sizeof(sensor_data_t));
        
        // 设置传感器ID和类型
        data->sensor_id = i;
        data->sensor_type = sensor->config.type;
        data->timestamp = time(NULL);
        
        // 读取传感器数据
        sensor_status_t status = sensor->ops.read(data);
        data->status = status;
        
        // 更新统计信息
        sensor->read_count++;
        manager.total_reads++;
        
        if (status != SENSOR_STATUS_OK) {
            sensor->error_count++;
            manager.total_errors++;
            
            // 设置默认值
            data->temperature = 0.0f;
            data->humidity = 0.0f;
        }
        
        // 保存最后一次读取的数据
        memcpy(&sensor->last_data, data, sizeof(sensor_data_t));
        
        read_count++;
    }
    
    pthread_mutex_unlock(&manager.mutex);
    
    return read_count;
}

/**
 * @brief 读取指定传感器数据
 * 
 * @param sensor_id 传感器ID
 * @param data 输出数据指针
 * @return sensor_status_t 读取状态
 */
sensor_status_t sensor_manager_read_sensor(int sensor_id, sensor_data_t *data)
{
    if (sensor_id < 0 || sensor_id >= manager.max_sensors || !data) {
        return SENSOR_STATUS_ERROR;
    }
    
    pthread_mutex_lock(&manager.mutex);
    
    sensor_instance_t *sensor = &manager.sensors[sensor_id];
    if (!sensor->is_initialized || !sensor->ops.read) {
        pthread_mutex_unlock(&manager.mutex);
        return SENSOR_STATUS_ERROR;
    }
    
    // 设置基本数据
    memset(data, 0, sizeof(sensor_data_t));
    data->sensor_id = sensor_id;
    data->sensor_type = sensor->config.type;
    data->timestamp = time(NULL);
    
    // 读取传感器数据
    sensor_status_t status = sensor->ops.read(data);
    data->status = status;
    
    // 更新统计信息
    sensor->read_count++;
    manager.total_reads++;
    
    if (status != SENSOR_STATUS_OK) {
        sensor->error_count++;
        manager.total_errors++;
    } else {
        // 保存最后一次读取的数据
        memcpy(&sensor->last_data, data, sizeof(sensor_data_t));
    }
    
    pthread_mutex_unlock(&manager.mutex);
    
    return status;
}

/**
 * @brief 获取传感器信息
 * 
 * @param sensor_id 传感器ID
 * @return const char* 传感器信息字符串，失败返回NULL
 */
const char* sensor_manager_get_sensor_info(int sensor_id)
{
    if (sensor_id < 0 || sensor_id >= manager.max_sensors) {
        return NULL;
    }
    
    pthread_mutex_lock(&manager.mutex);
    
    sensor_instance_t *sensor = &manager.sensors[sensor_id];
    if (!sensor->is_initialized || !sensor->ops.get_info) {
        pthread_mutex_unlock(&manager.mutex);
        return NULL;
    }
    
    const char *info = sensor->ops.get_info();
    
    pthread_mutex_unlock(&manager.mutex);
    
    return info;
}

/**
 * @brief 获取传感器统计信息
 * 
 * @param sensor_id 传感器ID
 * @param reads 输出读取次数指针
 * @param errors 输出错误次数指针
 * @return int 成功返回0，失败返回-1
 */
int sensor_manager_get_stats(int sensor_id, uint32_t *reads, uint32_t *errors)
{
    if (sensor_id < 0 || sensor_id >= manager.max_sensors) {
        return -1;
    }
    
    pthread_mutex_lock(&manager.mutex);
    
    sensor_instance_t *sensor = &manager.sensors[sensor_id];
    if (!sensor->is_initialized) {
        pthread_mutex_unlock(&manager.mutex);
        return -1;
    }
    
    if (reads) {
        *reads = sensor->read_count;
    }
    
    if (errors) {
        *errors = sensor->error_count;
    }
    
    pthread_mutex_unlock(&manager.mutex);
    
    return 0;
}

/**
 * @brief 获取管理器统计信息
 * 
 * @param total_reads 输出总读取次数指针
 * @param total_errors 输出总错误次数指针
 * @param active_sensors 输出活动传感器数量指针
 */
void sensor_manager_get_manager_stats(uint32_t *total_reads, 
                                     uint32_t *total_errors, 
                                     uint8_t *active_sensors)
{
    pthread_mutex_lock(&manager.mutex);
    
    if (total_reads) {
        *total_reads = manager.total_reads;
    }
    
    if (total_errors) {
        *total_errors = manager.total_errors;
    }
    
    if (active_sensors) {
        *active_sensors = manager.sensor_count;
    }
    
    pthread_mutex_unlock(&manager.mutex);
}

/**
 * @brief 获取传感器最后一次读取的数据
 * 
 * @param sensor_id 传感器ID
 * @param data 输出数据指针
 * @return int 成功返回0，失败返回-1
 */
int sensor_manager_get_last_data(int sensor_id, sensor_data_t *data)
{
    if (sensor_id < 0 || sensor_id >= manager.max_sensors || !data) {
        return -1;
    }
    
    pthread_mutex_lock(&manager.mutex);
    
    sensor_instance_t *sensor = &manager.sensors[sensor_id];
    if (!sensor->is_initialized) {
        pthread_mutex_unlock(&manager.mutex);
        return -1;
    }
    
    memcpy(data, &sensor->last_data, sizeof(sensor_data_t));
    
    pthread_mutex_unlock(&manager.mutex);
    
    return 0;
}

/**
 * @brief 获取所有活动传感器的ID
 * 
 * @param id_array 输出ID数组
 * @param max_ids 最大ID数量
 * @return int 活动传感器数量
 */
int sensor_manager_get_active_ids(int *id_array, int max_ids)
{
    if (!id_array || max_ids <= 0) {
        return 0;
    }
    
    pthread_mutex_lock(&manager.mutex);
    
    int count = 0;
    
    for (int i = 0; i < manager.max_sensors && count < max_ids; i++) {
        if (manager.sensors[i].is_initialized) {
            id_array[count] = i;
            count++;
        }
    }
    
    pthread_mutex_unlock(&manager.mutex);
    
    return count;
}

/**
 * @brief 重置传感器统计信息
 * 
 * @param sensor_id 传感器ID（-1表示重置所有传感器）
 * @return int 成功返回0，失败返回-1
 */
int sensor_manager_reset_stats(int sensor_id)
{
    pthread_mutex_lock(&manager.mutex);
    
    if (sensor_id == -1) {
        // 重置所有传感器
        for (int i = 0; i < manager.max_sensors; i++) {
            sensor_instance_t *sensor = &manager.sensors[i];
            if (sensor->is_initialized) {
                sensor->read_count = 0;
                sensor->error_count = 0;
            }
        }
        
        manager.total_reads = 0;
        manager.total_errors = 0;
    } else if (sensor_id >= 0 && sensor_id < manager.max_sensors) {
        // 重置指定传感器
        sensor_instance_t *sensor = &manager.sensors[sensor_id];
        if (sensor->is_initialized) {
            uint32_t old_reads = sensor->read_count;
            uint32_t old_errors = sensor->error_count;
            
            sensor->read_count = 0;
            sensor->error_count = 0;
            
            // 更新管理器统计
            manager.total_reads -= old_reads;
            manager.total_errors -= old_errors;
        } else {
            pthread_mutex_unlock(&manager.mutex);
            return -1;
        }
    } else {
        pthread_mutex_unlock(&manager.mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&manager.mutex);
    
    printf("传感器统计信息已重置%s\n", 
           sensor_id == -1 ? "（所有传感器）" : "");
    
    return 0;
}