# 飞腾开发板室内温湿度监测系统

## 项目简介
这是一个基于飞腾ARM处理器的嵌入式温湿度监测系统，用于实时监测室内环境参数。系统支持多种传感器，包括DHT11/DHT22温湿度传感器和I2C接口的传感器。

## 功能特性
- ✅ 实时温湿度监测
- ✅ 多传感器支持（DHT11, DHT22, I2C传感器）
- ✅ 数据记录和存储
- ✅ 命令行界面显示
- ✅ 可配置的采样频率
- ✅ 阈值报警功能
- ⏳ Web界面（可选）
- ⏳ 移动端应用（可选）

## 硬件要求
### 必需硬件
1. 飞腾开发板（如FT-2000/4开发板）
2. DHT11或DHT22温湿度传感器
3. 杜邦线若干

### 可选硬件
1. BMP280气压传感器（I2C）
2. BH1750光照传感器（I2C）
3. OLED显示屏（I2C/SPI）
4. 蜂鸣器（用于报警）

## 软件依赖
- Linux内核 4.x 或更高版本
- GCC ARM交叉编译工具链
- make工具
- libgpiod库（可选）
- Python 3（用于Web界面，可选）

## 快速开始

### 1. 克隆项目
```bash
git clone <repository-url>
cd temperature_humidity_monitor
```

### 2. 配置硬件连接
连接DHT11/DHT22传感器到飞腾开发板：
- VCC -> 3.3V
- GND -> GND
- DATA -> GPIO17（可配置）

### 3. 编译项目
```bash
make all
```

### 4. 部署到开发板
```bash
make deploy TARGET_IP=192.168.1.100
```

### 5. 运行程序
在开发板上执行：
```bash
cd /root/projects/temperature_humidity_monitor
./bin/temp_humidity_monitor
```

## 项目结构
```
.
├── Makefile              # 主构建文件
├── PROJECT_PLAN.md       # 项目计划
├── README.md            # 本文档
├── src/                 # 用户空间应用
│   ├── main.c          # 主程序
│   ├── sensor_manager.c # 传感器管理
│   ├── data_logger.c   # 数据记录
│   └── web_interface.c # Web界面（可选）
├── drivers/            # 内核驱动
│   ├── dht11_driver.c  # DHT11驱动
│   ├── dht22_driver.c  # DHT22驱动
│   └── i2c_sensors.c   # I2C传感器驱动
├── include/            # 头文件
│   ├── sensor_types.h
│   ├── dht11.h
│   └── i2c_sensors.h
├── config/             # 配置文件
│   ├── systemd/       # systemd服务文件
│   └── web/           # Web配置文件
└── scripts/           # 脚本文件
    ├── deploy.sh      # 部署脚本
    ├── test.sh        # 测试脚本
    └── setup.sh       # 安装脚本
```

## 配置说明

### GPIO引脚配置
编辑 `config/gpio_config.h` 修改GPIO引脚：
```c
#define DHT11_GPIO_PIN 17    // DHT11数据引脚
#define DHT22_GPIO_PIN 18    // DHT22数据引脚
#define LED_GPIO_PIN   23    // 状态指示灯
```

### 采样频率配置
在 `config/sensor_config.h` 中配置：
```c
#define SAMPLE_INTERVAL 60   // 采样间隔（秒）
#define LOG_INTERVAL   3600  // 日志记录间隔（秒）
```

## 使用示例

### 基本使用
```bash
# 启动监测程序
./temp_humidity_monitor --sensor dht11 --pin 17

# 查看实时数据
./temp_humidity_monitor --monitor

# 查看历史数据
./temp_humidity_monitor --history --days 7
```

### 命令行参数
```
--sensor <type>     传感器类型 (dht11, dht22, bmp280)
--pin <num>         GPIO引脚号
--interval <sec>    采样间隔（秒）
--log <path>        日志文件路径
--daemon            以守护进程模式运行
--verbose           详细输出模式
```

## 数据格式
程序输出的数据格式为JSON：
```json
{
  "timestamp": "2024-01-15T10:30:00Z",
  "temperature": 25.3,
  "humidity": 45.2,
  "sensor": "dht22",
  "unit": {
    "temperature": "celsius",
    "humidity": "percent"
  }
}
```

## 故障排除

### 常见问题
1. **传感器无响应**
   - 检查电源连接（3.3V）
   - 检查GPIO引脚配置
   - 检查传感器型号是否匹配

2. **编译错误**
   - 确认交叉编译工具链已安装
   - 检查内核头文件路径

3. **权限问题**
   - 确保程序有GPIO访问权限
   - 可能需要root权限运行

### 调试模式
```bash
# 启用调试输出
./temp_humidity_monitor --verbose --debug

# 查看系统日志
journalctl -f -u temp_humidity_monitor
```

## 扩展开发

### 添加新传感器
1. 在 `drivers/` 目录创建新驱动
2. 在 `include/sensor_types.h` 添加传感器类型
3. 在 `src/sensor_manager.c` 注册新传感器
4. 更新Makefile编译新驱动

### 自定义输出格式
修改 `src/data_logger.c` 中的输出函数，支持CSV、XML等格式。

## 性能指标
- 采样精度：温度±0.5°C，湿度±2%RH（DHT22）
- 响应时间：< 2秒
- 内存占用：< 10MB
- CPU使用率：< 5%（空闲时）

## 许可证
本项目采用 MIT 许可证。详见 LICENSE 文件。

## 贡献指南
欢迎提交Issue和Pull Request。请确保代码符合项目编码规范。

## 联系方式
- 项目维护者：[Your Name]
- 邮箱：[your.email@example.com]
- 问题反馈：[GitHub Issues]

## 更新日志
### v1.0.0 (2024-01-15)
- 初始版本发布
- 支持DHT11/DHT22传感器
- 基础数据记录功能
- 命令行界面