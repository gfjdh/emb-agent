/**
 * @file data_logger.c
 * @brief 数据记录器实现
 * @author Embedded System Team
 * @date 2024-01-15
 * 
 * 负责传感器数据的记录、存储和检索功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>

#include "sensor_types.h"

// 数据记录器配置
typedef struct {
    char log_dir[256];          /**< 日志目录 */
    char data_dir[256];         /**< 数据目录 */
    uint32_t max_files;         /**< 最大文件数 */
    uint64_t max_size;          /**< 最大文件大小（字节） */
    uint8_t rotation_enabled;   /**< 日志轮转使能 */
    uint8_t compression_enabled; /**< 压缩使能 */
} logger_config_t;

static logger_config_t config = {0};
static FILE *current_log_file = NULL;
static char current_log_path[512] = {0};
static uint64_t current_file_size = 0;

/**
 * @brief 初始化数据记录器
 * 
 * @param log_dir 日志目录
 * @param data_dir 数据目录
 * @return int 成功返回0，失败返回-1
 */
int data_logger_init(const char *log_dir, const char *data_dir)
{
    if (!log_dir || !data_dir) {
        fprintf(stderr, "日志目录或数据目录为空\n");
        return -1;
    }
    
    // 设置默认配置
    memset(&config, 0, sizeof(logger_config_t));
    strncpy(config.log_dir, log_dir, sizeof(config.log_dir) - 1);
    strncpy(config.data_dir, data_dir, sizeof(config.data_dir) - 1);
    config.max_files = 10;
    config.max_size = 10 * 1024 * 1024; // 10MB
    config.rotation_enabled = 1;
    config.compression_enabled = 0;
    
    // 创建目录
    if (mkdir(config.log_dir, 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "无法创建日志目录 %s: %s\n", 
                config.log_dir, strerror(errno));
        return -1;
    }
    
    if (mkdir(config.data_dir, 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "无法创建数据目录 %s: %s\n", 
                config.data_dir, strerror(errno));
        return -1;
    }
    
    printf("数据记录器初始化完成\n");
    printf("日志目录: %s\n", config.log_dir);
    printf("数据目录: %s\n", config.data_dir);
    
    return 0;
}

/**
 * @brief 清理数据记录器资源
 */
void data_logger_cleanup(void)
{
    if (current_log_file) {
        fclose(current_log_file);
        current_log_file = NULL;
    }
    
    memset(current_log_path, 0, sizeof(current_log_path));
    current_file_size = 0;
    
    printf("数据记录器资源已清理\n");
}

/**
 * @brief 获取当前日志文件路径
 * 
 * @return const char* 当前日志文件路径
 */
static const char* get_current_log_path(void)
{
    if (current_log_path[0] == '\0') {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        
        snprintf(current_log_path, sizeof(current_log_path),
                "%s/sensor_data_%04d%02d%02d.csv",
                config.log_dir,
                tm_info->tm_year + 1900,
                tm_info->tm_mon + 1,
                tm_info->tm_mday);
    }
    
    return current_log_path;
}

/**
 * @brief 打开或创建日志文件
 * 
 * @return FILE* 文件指针，失败返回NULL
 */
static FILE* open_log_file(void)
{
    if (current_log_file) {
        return current_log_file;
    }
    
    const char *log_path = get_current_log_path();
    
    // 检查文件是否存在
    struct stat st;
    if (stat(log_path, &st) == 0) {
        current_file_size = st.st_size;
    } else {
        current_file_size = 0;
    }
    
    // 打开文件（追加模式）
    current_log_file = fopen(log_path, "a");
    if (!current_log_file) {
        fprintf(stderr, "无法打开日志文件 %s: %s\n", 
                log_path, strerror(errno));
        return NULL;
    }
    
    // 如果是新文件，写入CSV头
    if (current_file_size == 0) {
        fprintf(current_log_file, "timestamp,temperature,humidity,pressure,light,sensor_type,status\n");
        fflush(current_log_file);
        current_file_size = ftell(current_log_file);
    }
    
    printf("日志文件已打开: %s (当前大小: %lu bytes)\n", 
           log_path, current_file_size);
    
    return current_log_file;
}

/**
 * @brief 检查是否需要日志轮转
 * 
 * @return int 需要轮转返回1，否则返回0
 */
static int need_rotation(void)
{
    if (!config.rotation_enabled) {
        return 0;
    }
    
    // 检查文件大小
    if (current_file_size >= config.max_size) {
        return 1;
    }
    
    // 检查日期（如果日期变化，也需要新文件）
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char new_path[512];
    snprintf(new_path, sizeof(new_path),
            "%s/sensor_data_%04d%02d%02d.csv",
            config.log_dir,
            tm_info->tm_year + 1900,
            tm_info->tm_mon + 1,
            tm_info->tm_mday);
    
    if (strcmp(new_path, current_log_path) != 0) {
        return 1;
    }
    
    return 0;
}

/**
 * @brief 执行日志轮转
 */
static void rotate_log_file(void)
{
    if (!current_log_file) {
        return;
    }
    
    // 关闭当前文件
    fclose(current_log_file);
    current_log_file = NULL;
    
    // 重置当前文件路径
    memset(current_log_path, 0, sizeof(current_log_path));
    current_file_size = 0;
    
    // 清理旧文件
    cleanup_old_files();
    
    printf("日志文件已轮转\n");
}

/**
 * @brief 清理旧日志文件
 */
static void cleanup_old_files(void)
{
    DIR *dir = opendir(config.log_dir);
    if (!dir) {
        return;
    }
    
    struct dirent *entry;
    char **files = NULL;
    int file_count = 0;
    int max_files = 100;
    
    // 分配文件列表内存
    files = calloc(max_files, sizeof(char*));
    if (!files) {
        closedir(dir);
        return;
    }
    
    // 读取所有CSV文件
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, "sensor_data_") && 
            strstr(entry->d_name, ".csv")) {
            
            if (file_count >= max_files) {
                max_files *= 2;
                files = realloc(files, max_files * sizeof(char*));
                if (!files) {
                    break;
                }
            }
            
            files[file_count] = strdup(entry->d_name);
            if (files[file_count]) {
                file_count++;
            }
        }
    }
    
    closedir(dir);
    
    // 如果文件数量超过限制，删除最旧的文件
    if (file_count > config.max_files) {
        // 按文件名排序（假设文件名包含日期）
        for (int i = 0; i < file_count - 1; i++) {
            for (int j = i + 1; j < file_count; j++) {
                if (strcmp(files[i], files[j]) > 0) {
                    char *temp = files[i];
                    files[i] = files[j];
                    files[j] = temp;
                }
            }
        }
        
        // 删除最旧的文件
        for (int i = 0; i < file_count - config.max_files; i++) {
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s/%s", 
                    config.log_dir, files[i]);
            
            if (unlink(file_path) == 0) {
                printf("删除旧日志文件: %s\n", file_path);
            }
            
            free(files[i]);
        }
    }
    
    // 释放剩余的文件名
    for (int i = file_count - config.max_files > 0 ? 
         file_count - config.max_files : 0; i < file_count; i++) {
        free(files[i]);
    }
    
    free(files);
}

/**
 * @brief 记录传感器数据
 * 
 * @param data 传感器数据
 * @return int 成功返回0，失败返回-1
 */
int data_logger_log(const sensor_data_t *data)
{
    if (!data) {
        fprintf(stderr, "传感器数据为空\n");
        return -1;
    }
    
    // 检查是否需要轮转
    if (need_rotation()) {
        rotate_log_file();
    }
    
    // 打开日志文件
    FILE *log_file = open_log_file();
    if (!log_file) {
        return -1;
    }
    
    // 格式化时间戳
    char time_str[64];
    struct tm *tm_info = localtime(&data->timestamp);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // 写入CSV格式数据
    int written = fprintf(log_file, "%s,%.2f,%.2f,%.2f,%.2f,%s,%s\n",
                         time_str,
                         data->temperature,
                         data->humidity,
                         data->pressure,
                         data->light,
                         sensor_type_to_string(data->sensor_type),
                         sensor_status_to_string(data->status));
    
    if (written < 0) {
        fprintf(stderr, "写入日志文件失败\n");
        return -1;
    }
    
    fflush(log_file);
    
    // 更新文件大小
    current_file_size = ftell(log_file);
    
    return 0;
}

/**
 * @brief 批量记录传感器数据
 * 
 * @param data_array 传感器数据数组
 * @param count 数据数量
 * @return int 成功记录的数据数量
 */
int data_logger_log_batch(const sensor_data_t *data_array, int count)
{
    if (!data_array || count <= 0) {
        return 0;
    }
    
    int success_count = 0;
    
    for (int i = 0; i < count; i++) {
        if (data_logger_log(&data_array[i]) == 0) {
            success_count++;
        }
    }
    
    return success_count;
}

/**
 * @brief 读取历史数据
 * 
 * @param start_time 开始时间
 * @param end_time 结束时间
 * @param buffer 输出缓冲区
 * @param max_records 最大记录数
 * @return int 读取的记录数量
 */
int data_logger_read_history(time_t start_time, time_t end_time,
                            sensor_data_t *buffer, int max_records)
{
    if (!buffer || max_records <= 0) {
        return 0;
    }
    
    // 这里可以实现从日志文件读取历史数据的功能
    // 由于时间关系，这里只返回0
    
    printf("历史数据读取功能暂未实现\n");
    return 0;
}

/**
 * @brief 导出数据到文件
 * 
 * @param start_time 开始时间
 * @param end_time 结束时间
 * @param output_file 输出文件路径
 * @param format 输出格式（csv, json, xml）
 * @return int 成功返回0，失败返回-1
 */
int data_logger_export_data(time_t start_time, time_t end_time,
                           const char *output_file, const char *format)
{
    if (!output_file || !format) {
        fprintf(stderr, "输出文件或格式为空\n");
        return -1;
    }
    
    printf("数据导出功能暂未实现\n");
    printf("参数: 开始时间=%ld, 结束时间=%ld, 输出文件=%s, 格式=%s\n",
           start_time, end_time, output_file, format);
    
    return -1;
}

/**
 * @brief 获取日志文件统计信息
 * 
 * @param total_files 输出总文件数
 * @param total_size 输出总大小（字节）
 * @param oldest_file 输出最旧文件时间
 * @param newest_file 输出最新文件时间
 * @return int 成功返回0，失败返回-1
 */
int data_logger_get_stats(int *total_files, uint64_t *total_size,
                         time_t *oldest_file, time_t *newest_file)
{
    DIR *dir = opendir(config.log_dir);
    if (!dir) {
        fprintf(stderr, "无法打开日志目录: %s\n", config.log_dir);
        return -1;
    }
    
    struct dirent *entry;
    int file_count = 0;
    uint64_t size_total = 0;
    time_t oldest = 0;
    time_t newest = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, "sensor_data_") && 
            strstr(entry->d_name, ".csv")) {
            
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s/%s", 
                    config.log_dir, entry->d_name);
            
            struct stat st;
            if (stat(file_path, &st) == 0) {
                file_count++;
                size_total += st.st_size;
                
                if (oldest == 0 || st.st_mtime < oldest) {
                    oldest = st.st_mtime;
                }
                
                if (newest == 0 || st.st_mtime > newest) {
                    newest = st.st_mtime;
                }
            }
        }
    }
    
    closedir(dir);
    
    if (total_files) {
        *total_files = file_count;
    }
    
    if (total_size) {
        *total_size = size_total;
    }
    
    if (oldest_file) {
        *oldest_file = oldest;
    }
    
    if (newest_file) {
        *newest_file = newest;
    }
    
    return 0;
}

/**
 * @brief 设置记录器配置
 * 
 * @param new_config 新配置
 * @return int 成功返回0，失败返回-1
 */
int data_logger_set_config(const logger_config_t *new_config)
{
    if (!new_config) {
        return -1;
    }
    
    memcpy(&config, new_config, sizeof(logger_config_t));
    
    printf("记录器配置已更新\n");
    printf("最大文件数: %u\n", config.max_files);
    printf("最大文件大小: %lu bytes\n", config.max_size);
    printf("日志轮转: %s\n", config.rotation_enabled ? "启用" : "禁用");
    printf("压缩: %s\n", config.compression_enabled ? "启用" : "禁用");
    
    return 0;
}

/**
 * @brief 获取记录器配置
 * 
 * @return const logger_config_t* 配置指针
 */
const logger_config_t* data_logger_get_config(void)
{
    return &config;
}

/**
 * @brief 强制刷新日志文件
 * 
 * @return int 成功返回0，失败返回-1
 */
int data_logger_flush(void)
{
    if (current_log_file) {
        if (fflush(current_log_file) == 0) {
            return 0;
        }
    }
    
    return -1;
}

/**
 * @brief 备份日志文件
 * 
 * @param backup_dir 备份目录
 * @return int 成功返回0，失败返回-1
 */
int data_logger_backup(const char *backup_dir)
{
    if (!backup_dir) {
        fprintf(stderr, "备份目录为空\n");
        return -1;
    }
    
    // 创建备份目录
    if (mkdir(backup_dir, 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "无法创建备份目录 %s: %s\n", 
                backup_dir, strerror(errno));
        return -1;
    }
    
    printf("日志备份功能暂未完全实现\n");
    printf("备份目录: %s\n", backup_dir);
    
    return 0;
}

/**
 * @brief 压缩日志文件
 * 
 * @return int 成功返回0，失败返回-1
 */
int data_logger_compress(void)
{
    if (!config.compression_enabled) {
        printf("压缩功能未启用\n");
        return -1;
    }
    
    printf("日志压缩功能暂未实现\n");
    return -1;
}