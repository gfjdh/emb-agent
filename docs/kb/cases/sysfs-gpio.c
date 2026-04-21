/*
本例程使用sysfs的方式来操作GPIO，以飞腾派GPIO2_10为例，GPIO2_10的编号为464+10=474。在GPIO2_10（第7个）引脚上接LED正极，LED负极接GND，可点亮LED.
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define GPIO_EXPORT "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define led1 474
//导出GPIO函数
void exportPin(int pin) {
    char pinStr[4];
    sprintf(pinStr, "%d", pin);

    int fd = open(GPIO_EXPORT, O_WRONLY);
    if (fd == -1) {
        printf("Failed to open GPIO export file.\n");
        exit(1);
    }

    write(fd, pinStr, strlen(pinStr));
    close(fd);
}
//释放GPIO函数
void unexportPin(int pin) {
    char pinStr[4];
    sprintf(pinStr, "%d", pin);

    int fd = open(GPIO_UNEXPORT, O_WRONLY);
    if (fd == -1) {
        printf("Failed to open GPIO unexport file.\n");
        exit(1);
    }

    write(fd, pinStr, strlen(pinStr));
    close(fd);
}
//设置GPIO方向函数
void setPinDirection(int pin, const char *direction) {
    char pinDir[100];
    sprintf(pinDir, "/sys/class/gpio/gpio%d/direction", pin);

    int fd = open(pinDir, O_WRONLY);
    if (fd == -1) {
        printf("Failed to open pin %d\n", pin);
        exit(1);
    }

    write(fd, direction, strlen(direction));
    close(fd);
}
//设置GPIO输出值函数
void setPinValue(int pin, int value) {
    char pinValue[100];
    sprintf(pinValue, "/sys/class/gpio/gpio%d/value", pin);

    int fd = open(pinValue, O_WRONLY);
    if (fd == -1) {
        printf("Failed to open pin %d\n", pin);
        exit(1);
    }

    char strValue[2];
    sprintf(strValue, "%d", value);
    write(fd, strValue, strlen(strValue));
    close(fd);
}
//读取GPIO输入值函数
int readPinValue(int pin) {
    char pinValue[100];
    sprintf(pinValue, "/sys/class/gpio/gpio%d/value", pin);

    int fd = open(pinValue, O_RDONLY);
    if (fd == -1) {
        printf("Failed to open pin %d\n", pin);
        exit(1);
    }

    char value;
    read(fd, &value, sizeof(value));
    close(fd);

    return (value == '1') ? 1 : 0;
}
//主程序
int main() {
    int gpioPin = led1;   //导出GPIO2_10脚
    int pinValue;
    exportPin(gpioPin);   // 导出GPIO引脚        
    setPinDirection(gpioPin, "out");// 设置GPIO引脚方向为输出 
    
    setPinValue(gpioPin, 1);// 设置GPIO引脚输出高电平
    pinValue = readPinValue(gpioPin); // 读取GPIO引脚的状态
    printf("GPIO Pin %d value: %d\n", gpioPin, pinValue);
    sleep(3); //延时3秒   
    setPinValue(gpioPin, 0);// 设置GPIO引脚输出低电平 
      
    pinValue = readPinValue(gpioPin);// 读取GPIO引脚的状态
    printf("GPIO Pin %d value: %d\n", gpioPin, pinValue);  
    unexportPin(gpioPin);// 取消导出GPIO引脚

    return 0;
}

