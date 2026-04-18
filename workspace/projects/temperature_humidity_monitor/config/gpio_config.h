/**
 * @file gpio_config.h
 * @brief GPIO引脚配置
 * @author Embedded System Team
 * @date 2024-01-15
 * 
 * 飞腾开发板GPIO引脚配置，用于温湿度传感器连接
 */

#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

// DHT11/DHT22传感器GPIO引脚配置
// 注意：具体引脚号需要根据实际硬件连接调整

// DHT11温湿度传感器引脚
#define DHT11_GPIO_PIN     17  // GPIO17，用于DHT11传感器

// DHT22/AM2302温湿度传感器引脚
#define DHT22_GPIO_PIN     18  // GPIO18，用于DHT22传感器

// 状态指示灯引脚
#define LED_STATUS_PIN     23  // GPIO23，系统状态指示灯
#define LED_ERROR_PIN      24  // GPIO24，错误指示灯
#define LED_DATA_PIN       25  // GPIO25，数据采集指示灯

// 报警输出引脚
#define BUZZER_PIN         26  // GPIO26，蜂鸣器报警输出
#define RELAY_PIN          27  // GPIO27，继电器控制输出

// I2C引脚（通常固定，不需要配置）
// SDA: GPIO2 (I2C1_SDA)
// SCL: GPIO3 (I2C1_SCL)

// SPI引脚（用于显示屏等）
#define SPI_CS_PIN         8   // GPIO8，SPI片选
#define SPI_MOSI_PIN       10  // GPIO10，SPI主出从入
#define SPI_MISO_PIN       9   // GPIO9，SPI主入从出
#define SPI_CLK_PIN        11  // GPIO11，SPI时钟

// UART引脚（用于调试）
#define UART_TX_PIN        14  // GPIO14，UART发送
#define UART_RX_PIN        15  // GPIO15，UART接收

// 按键输入引脚
#define BUTTON1_PIN        5   // GPIO5，按钮1
#define BUTTON2_PIN        6   // GPIO6，按钮2
#define BUTTON3_PIN        13  // GPIO13，按钮3

// 飞腾开发板特定引脚映射
// 以下是根据常见飞腾开发板的引脚映射
#ifdef PHYTIUM_FT2000
    // FT-2000/4开发板引脚
    #define DHT11_GPIO_PIN     40  // J8-7
    #define DHT22_GPIO_PIN     41  // J8-8
    #define LED_STATUS_PIN     42  // J8-9
#endif

#ifdef PHYTIUM_D2000
    // D2000开发板引脚
    #define DHT11_GPIO_PIN     48  // GPIO48
    #define DHT22_GPIO_PIN     49  // GPIO49
    #define LED_STATUS_PIN     50  // GPIO50
#endif

// GPIO方向定义
#define GPIO_DIR_INPUT     0   // 输入模式
#define GPIO_DIR_OUTPUT    1   // 输出模式

// GPIO电平定义
#define GPIO_LEVEL_LOW     0   // 低电平
#define GPIO_LEVEL_HIGH    1   // 高电平

// GPIO上拉/下拉配置
#define GPIO_PULL_NONE     0   // 无上拉/下拉
#define GPIO_PULL_UP       1   // 上拉电阻
#define GPIO_PULL_DOWN     2   // 下拉电阻

// GPIO中断触发方式
#define GPIO_IRQ_NONE      0   // 无中断
#define GPIO_IRQ_RISING    1   // 上升沿触发
#define GPIO_IRQ_FALLING   2   // 下降沿触发
#define GPIO_IRQ_BOTH      3   // 双边沿触发
#define GPIO_IRQ_HIGH      4   // 高电平触发
#define GPIO_IRQ_LOW       5   // 低电平触发

// GPIO配置结构体
typedef struct {
    uint8_t pin;            // 引脚号
    uint8_t direction;      // 方向 (GPIO_DIR_INPUT/GPIO_DIR_OUTPUT)
    uint8_t pull;           // 上拉/下拉 (GPIO_PULL_*)
    uint8_t irq_type;       // 中断类型 (GPIO_IRQ_*)
    char name[32];          // 引脚名称
} gpio_config_t;

// 默认GPIO配置
static const gpio_config_t default_gpio_configs[] = {
    // DHT11传感器
    {
        .pin = DHT11_GPIO_PIN,
        .direction = GPIO_DIR_INPUT,
        .pull = GPIO_PULL_UP,
        .irq_type = GPIO_IRQ_NONE,
        .name = "DHT11_Sensor"
    },
    
    // DHT22传感器
    {
        .pin = DHT22_GPIO_PIN,
        .direction = GPIO_DIR_INPUT,
        .pull = GPIO_PULL_UP,
        .irq_type = GPIO_IRQ_NONE,
        .name = "DHT22_Sensor"
    },
    
    // 状态指示灯
    {
        .pin = LED_STATUS_PIN,
        .direction = GPIO_DIR_OUTPUT,
        .pull = GPIO_PULL_NONE,
        .irq_type = GPIO_IRQ_NONE,
        .name = "Status_LED"
    },
    
    // 错误指示灯
    {
        .pin = LED_ERROR_PIN,
        .direction = GPIO_DIR_OUTPUT,
        .pull = GPIO_PULL_NONE,
        .irq_type = GPIO_IRQ_NONE,
        .name = "Error_LED"
    },
    
    // 数据采集指示灯
    {
        .pin = LED_DATA_PIN,
        .direction = GPIO_DIR_OUTPUT,
        .pull = GPIO_PULL_NONE,
        .irq_type = GPIO_IRQ_NONE,
        .name = "Data_LED"
    },
    
    // 蜂鸣器
    {
        .pin = BUZZER_PIN,
        .direction = GPIO_DIR_OUTPUT,
        .pull = GPIO_PULL_NONE,
        .irq_type = GPIO_IRQ_NONE,
        .name = "Buzzer"
    }
};

// GPIO配置数量
#define GPIO_CONFIG_COUNT (sizeof(default_gpio_configs) / sizeof(gpio_config_t))

// 函数声明

/**
 * @brief 初始化GPIO系统
 * 
 * @return int 成功返回0，失败返回-1
 */
int gpio_system_init(void);

/**
 * @brief 配置GPIO引脚
 * 
 * @param config GPIO配置
 * @return int 成功返回0，失败返回-1
 */
int gpio_configure(const gpio_config_t *config);

/**
 * @brief 设置GPIO引脚方向
 * 
 * @param pin 引脚号
 * @param direction 方向
 * @return int 成功返回0，失败返回-1
 */
int gpio_set_direction(uint8_t pin, uint8_t direction);

/**
 * @brief 设置GPIO引脚电平
 * 
 * @param pin 引脚号
 * @param level 电平
 * @return int 成功返回0，失败返回-1
 */
int gpio_set_level(uint8_t pin, uint8_t level);

/**
 * @brief 读取GPIO引脚电平
 * 
 * @param pin 引脚号
 * @return int 电平值，失败返回-1
 */
int gpio_get_level(uint8_t pin);

/**
 * @brief 设置GPIO上拉/下拉
 * 
 * @param pin 引脚号
 * @param pull 上拉/下拉配置
 * @return int 成功返回0，失败返回-1
 */
int gpio_set_pull(uint8_t pin, uint8_t pull);

/**
 * @brief 设置GPIO中断
 * 
 * @param pin 引脚号
 * @param irq_type 中断类型
 * @param callback 中断回调函数
 * @param arg 回调函数参数
 * @return int 成功返回0，失败返回-1
 */
int gpio_set_irq(uint8_t pin, uint8_t irq_type, 
                 void (*callback)(void *), void *arg);

/**
 * @brief 清理GPIO资源
 */
void gpio_cleanup(void);

/**
 * @brief 获取GPIO引脚名称
 * 
 * @param pin 引脚号
 * @return const char* 引脚名称，未找到返回NULL
 */
const char* gpio_get_pin_name(uint8_t pin);

/**
 * @brief 导出GPIO引脚
 * 
 * @param pin 引脚号
 * @return int 成功返回0，失败返回-1
 */
int gpio_export(uint8_t pin);

/**
 * @brief 取消导出GPIO引脚
 * 
 * @param pin 引脚号
 * @return int 成功返回0，失败返回-1
 */
int gpio_unexport(uint8_t pin);

#endif /* GPIO_CONFIG_H */