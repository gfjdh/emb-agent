/**
 * @file main.c
 * @brief 温湿度监测系统主程序
 * @author Embedded System Team
 * @date 2024-01-15
 * 
 * 飞腾开发板温湿度监测系统主程序，支持多传感器数据采集和显示
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "sensor_types.h"
#include "dht11.h"
#include "i2c_sensors.h"

// 全局变量
static volatile int running = 1;
static system_config_t sys_config = {0};
static FILE *log_file = NULL;

/**
 * @brief 信号处理函数
 * 
 * @param sig 信号编号
 */
static void signal_handler(int sig)
{
    printf("\n接收到信号 %d，正在关闭程序...\n", sig);
    running = 0;
}

/**
 * @brief 初始化系统配置
 * 
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return int 成功返回0，失败返回-1
 */
static int init_config(int argc, char *argv[])
{
    // 设置默认配置
    memset(&sys_config, 0, sizeof(system_config_t));
    
    sys_config.sensor_count = 0;
    sys_config.sensors = NULL;
    
    // 设置默认报警阈值
    sys_config.alarm_threshold.temp_high = 35.0f;
    sys_config.alarm_threshold.temp_low = 10.0f;
    sys_config.alarm_threshold.humidity_high = 80.0f;
    sys_config.alarm_threshold.humidity_low = 20.0f;
    sys_config.alarm_threshold.enabled = 1;
    
    sys_config.log_interval = DEFAULT_LOG_INTERVAL;
    strncpy(sys_config.log_file, "/var/log/temp_humidity.log", sizeof(sys_config.log_file) - 1);
    sys_config.daemon_mode = 0;
    sys_config.verbose = 0;
    
    return 0;
}

/**
 * @brief 解析命令行参数
 * 
 * @param argc 参数个数
 * @param argv 参数数组
 * @return int 成功返回0，失败返回-1
 */
static int parse_arguments(int argc, char *argv[])
{
    static struct option long_options[] = {
        {"sensor", required_argument, 0, 's'},
        {"pin", required_argument, 0, 'p'},
        {"interval", required_argument, 0, 'i'},
        {"log", required_argument, 0, 'l'},
        {"daemon", no_argument, 0, 'd'},
        {"verbose", no_argument, 0, 'v'},
        {"test", no_argument, 0, 't'},
        {"help", no_argument, 0, 'h'},
        {"monitor", no_argument, 0, 'm'},
        {"history", optional_argument, 0, 'H'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "s:p:i:l:dvthmH::", long_options, &option_index)) != -1) {
        switch (opt) {
            case 's':
                printf("传感器类型: %s\n", optarg);
                // 这里可以解析传感器类型并添加到配置中
                break;
                
            case 'p':
                printf("GPIO引脚: %s\n", optarg);
                break;
                
            case 'i':
                sys_config.log_interval = atoi(optarg);
                printf("采样间隔: %d秒\n", sys_config.log_interval);
                break;
                
            case 'l':
                strncpy(sys_config.log_file, optarg, sizeof(sys_config.log_file) - 1);
                printf("日志文件: %s\n", sys_config.log_file);
                break;
                
            case 'd':
                sys_config.daemon_mode = 1;
                printf("守护进程模式\n");
                break;
                
            case 'v':
                sys_config.verbose = 1;
                printf("详细输出模式\n");
                break;
                
            case 't':
                printf("测试模式\n");
                // 这里可以添加测试代码
                return 1;
                
            case 'm':
                printf("监控模式\n");
                // 这里可以添加监控代码
                break;
                
            case 'H':
                if (optarg) {
                    printf("查看历史数据，天数: %s\n", optarg);
                } else {
                    printf("查看历史数据\n");
                }
                // 这里可以添加历史数据查看代码
                return 1;
                
            case 'h':
                print_help();
                return 1;
                
            default:
                fprintf(stderr, "未知选项\n");
                print_help();
                return -1;
        }
    }
    
    return 0;
}

/**
 * @brief 打印帮助信息
 */
static void print_help(void)
{
    printf("飞腾温湿度监测系统\n");
    printf("用法: temp_humidity_monitor [选项]\n\n");
    printf("选项:\n");
    printf("  -s, --sensor TYPE     传感器类型 (dht11, dht22, bmp280, bh1750)\n");
    printf("  -p, --pin NUM         GPIO引脚号\n");
    printf("  -i, --interval SEC    采样间隔（秒，默认: 60）\n");
    printf("  -l, --log FILE        日志文件路径（默认: /var/log/temp_humidity.log）\n");
    printf("  -d, --daemon          以守护进程模式运行\n");
    printf("  -v, --verbose         详细输出模式\n");
    printf("  -t, --test            运行测试\n");
    printf("  -m, --monitor         实时监控模式\n");
    printf("  -H, --history [DAYS]  查看历史数据（可选天数）\n");
    printf("  -h, --help            显示此帮助信息\n\n");
    printf("示例:\n");
    printf("  temp_humidity_monitor --sensor dht11 --pin 17 --interval 30\n");
    printf("  temp_humidity_monitor --daemon --log /tmp/sensor.log\n");
    printf("  temp_humidity_monitor --monitor\n");
}

/**
 * @brief 初始化日志系统
 * 
 * @return int 成功返回0，失败返回-1
 */
static int init_logging(void)
{
    // 创建日志目录（如果不存在）
    char log_dir[256];
    strncpy(log_dir, sys_config.log_file, sizeof(log_dir) - 1);
    
    char *last_slash = strrchr(log_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (mkdir(log_dir, 0755) == -1 && errno != EEXIST) {
            fprintf(stderr, "无法创建日志目录 %s: %s\n", log_dir, strerror(errno));
            return -1;
        }
    }
    
    // 打开日志文件
    log_file = fopen(sys_config.log_file, "a");
    if (!log_file) {
        fprintf(stderr, "无法打开日志文件 %s: %s\n", sys_config.log_file, strerror(errno));
        return -1;
    }
    
    // 设置文件缓冲区
    setbuf(log_file, NULL);
    
    printf("日志系统初始化完成，日志文件: %s\n", sys_config.log_file);
    return 0;
}

/**
 * @brief 初始化传感器
 * 
 * @return int 成功返回0，失败返回-1
 */
static int init_sensors(void)
{
    // 这里可以添加传感器初始化代码
    // 例如：创建DHT11传感器实例
    
    printf("传感器初始化完成\n");
    return 0;
}

/**
 * @brief 记录传感器数据到日志
 * 
 * @param data 传感器数据
 */
static void log_sensor_data(const sensor_data_t *data)
{
    if (!log_file || !data) {
        return;
    }
    
    char time_str[64];
    struct tm *tm_info;
    
    tm_info = localtime(&data->timestamp);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(log_file, "%s,%.1f,%.1f,%s,%s\n",
            time_str,
            data->temperature,
            data->humidity,
            sensor_type_to_string(data->sensor_type),
            sensor_status_to_string(data->status));
}

/**
 * @brief 显示传感器数据
 * 
 * @param data 传感器数据
 */
static void display_sensor_data(const sensor_data_t *data)
{
    char time_str[64];
    struct tm *tm_info;
    
    tm_info = localtime(&data->timestamp);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] ", time_str);
    printf("温度: %.1f°C, ", data->temperature);
    printf("湿度: %.1f%%, ", data->humidity);
    
    if (data->pressure > 0) {
        printf("气压: %.1fhPa, ", data->pressure);
    }
    
    if (data->light > 0) {
        printf("光照: %.1flux, ", data->light);
    }
    
    printf("传感器: %s, ", sensor_type_to_string(data->sensor_type));
    printf("状态: %s\n", sensor_status_to_string(data->status));
    
    // 检查报警
    if (sys_config.alarm_threshold.enabled) {
        int alarm_flags = check_alarm(data, &sys_config.alarm_threshold);
        if (alarm_flags) {
            printf("警告: ");
            if (alarm_flags & 0x01) printf("温度过高 ");
            if (alarm_flags & 0x02) printf("温度过低 ");
            if (alarm_flags & 0x04) printf("湿度过高 ");
            if (alarm_flags & 0x08) printf("湿度过低 ");
            printf("\n");
        }
    }
}

/**
 * @brief 主循环
 */
static void main_loop(void)
{
    time_t last_log_time = 0;
    sensor_data_t sensor_data;
    
    printf("开始主循环，按Ctrl+C退出\n");
    
    while (running) {
        time_t current_time = time(NULL);
        
        // 模拟读取传感器数据（实际项目中这里会调用真实的传感器读取函数）
        memset(&sensor_data, 0, sizeof(sensor_data_t));
        sensor_data.timestamp = current_time;
        sensor_data.temperature = 25.0f + (rand() % 100) / 10.0f; // 模拟温度数据
        sensor_data.humidity = 50.0f + (rand() % 60) / 10.0f;    // 模拟湿度数据
        sensor_data.sensor_type = SENSOR_TYPE_DHT22;
        sensor_data.status = SENSOR_STATUS_OK;
        
        // 显示数据
        display_sensor_data(&sensor_data);
        
        // 记录数据到日志
        if (current_time - last_log_time >= sys_config.log_interval) {
            log_sensor_data(&sensor_data);
            last_log_time = current_time;
            
            if (sys_config.verbose) {
                printf("数据已记录到日志\n");
            }
        }
        
        // 等待下一次采样
        sleep(1);
    }
}

/**
 * @brief 清理资源
 */
static void cleanup(void)
{
    printf("正在清理资源...\n");
    
    // 关闭日志文件
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    
    // 释放传感器资源
    if (sys_config.sensors) {
        free(sys_config.sensors);
        sys_config.sensors = NULL;
    }
    
    printf("资源清理完成\n");
}

/**
 * @brief 守护进程模式
 */
static void daemonize(void)
{
    pid_t pid = fork();
    
    if (pid < 0) {
        fprintf(stderr, "创建守护进程失败: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        // 父进程退出
        exit(EXIT_SUCCESS);
    }
    
    // 子进程继续执行
    
    // 创建新会话
    if (setsid() < 0) {
        fprintf(stderr, "创建新会话失败: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    // 改变工作目录
    if (chdir("/") < 0) {
        fprintf(stderr, "改变工作目录失败: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    // 重定向标准输入输出
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    
    printf("守护进程已启动，PID: %d\n", getpid());
}

/**
 * @brief 主函数
 * 
 * @param argc 参数个数
 * @param argv 参数数组
 * @return int 程序退出码
 */
int main(int argc, char *argv[])
{
    printf("飞腾温湿度监测系统 v1.0\n");
    printf("=======================\n\n");
    
    // 初始化配置
    if (init_config(argc, argv) < 0) {
        fprintf(stderr, "配置初始化失败\n");
        return EXIT_FAILURE;
    }
    
    // 解析命令行参数
    int parse_result = parse_arguments(argc, argv);
    if (parse_result < 0) {
        return EXIT_FAILURE;
    } else if (parse_result > 0) {
        // 测试模式或帮助模式，直接退出
        return EXIT_SUCCESS;
    }
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 守护进程模式
    if (sys_config.daemon_mode) {
        daemonize();
    }
    
    // 初始化日志系统
    if (init_logging() < 0) {
        fprintf(stderr, "日志系统初始化失败\n");
        return EXIT_FAILURE;
    }
    
    // 初始化传感器
    if (init_sensors() < 0) {
        fprintf(stderr, "传感器初始化失败\n");
        cleanup();
        return EXIT_FAILURE;
    }
    
    // 运行主循环
    main_loop();
    
    // 清理资源
    cleanup();
    
    printf("程序正常退出\n");
    return EXIT_SUCCESS;
}