# 飞腾温湿度监测系统 Makefile
# 适用于飞腾ARM开发板

# 交叉编译工具链配置
CROSS_COMPILE ?= aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip

# 目标板IP地址（部署时使用）
TARGET_IP ?= 192.168.1.100
TARGET_USER ?= root
TARGET_PATH ?= /root/projects/temperature_humidity_monitor

# 编译选项
CFLAGS = -Wall -Wextra -O2 -g
CFLAGS += -I./include -I./drivers
LDFLAGS = -lpthread -lm -lgpiod

# 内核模块编译选项（如果需要）
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
MODULE_CFLAGS = -D__KERNEL__ -DMODULE -I$(KERNEL_DIR)/include

# 目录定义
SRC_DIR = src
DRIVER_DIR = drivers
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin
LIB_DIR = lib
CONFIG_DIR = config
SCRIPTS_DIR = scripts

# 源文件
APP_SRCS = $(SRC_DIR)/main.c \
           $(SRC_DIR)/sensor_manager.c \
           $(SRC_DIR)/data_logger.c

DRIVER_SRCS = $(DRIVER_DIR)/dht11_driver.c \
              $(DRIVER_DIR)/dht22_driver.c \
              $(DRIVER_DIR)/i2c_sensors.c

# 目标文件
APP_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(APP_SRCS))
DRIVER_OBJS = $(patsubst $(DRIVER_DIR)/%.c,$(BUILD_DIR)/%.o,$(DRIVER_SRCS))

# 最终目标
TARGET_APP = $(BIN_DIR)/temp_humidity_monitor
TARGET_DRIVERS = $(BIN_DIR)/dht11_driver.ko $(BIN_DIR)/dht22_driver.ko

# 默认目标
all: prepare $(TARGET_APP) drivers

# 准备目录结构
prepare:
	@echo "创建目录结构..."
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) $(LIB_DIR)
	@mkdir -p $(BUILD_DIR)/src $(BUILD_DIR)/drivers

# 编译应用程序
$(TARGET_APP): $(APP_OBJS)
	@echo "链接应用程序: $@"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "应用程序编译完成"

# 编译用户空间对象文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "编译: $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(DRIVER_DIR)/%.c
	@echo "编译驱动: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# 内核模块编译（如果需要）
drivers: prepare_kernel
	@echo "编译内核模块..."
	@if [ -d "$(KERNEL_DIR)" ]; then \
		$(MAKE) -C $(KERNEL_DIR) M=$(PWD)/$(DRIVER_DIR) modules; \
		cp $(DRIVER_DIR)/*.ko $(BIN_DIR)/ 2>/dev/null || true; \
	else \
		echo "警告: 内核头文件未找到，跳过内核模块编译"; \
	fi

prepare_kernel:
	@if [ ! -d "$(KERNEL_DIR)" ]; then \
		echo "警告: 内核目录 $(KERNEL_DIR) 不存在"; \
		echo "请设置 KERNEL_DIR 环境变量或安装内核头文件"; \
	fi

# 清理
clean:
	@echo "清理构建文件..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)/*.ko
	rm -f $(TARGET_APP)
	@if [ -d "$(KERNEL_DIR)" ]; then \
		$(MAKE) -C $(KERNEL_DIR) M=$(PWD)/$(DRIVER_DIR) clean; \
	fi

distclean: clean
	@echo "完全清理..."
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(LIB_DIR)

# 部署到目标板
deploy: all
	@echo "部署到目标板 $(TARGET_IP)..."
	@if [ -z "$(TARGET_IP)" ]; then \
		echo "错误: 请设置 TARGET_IP 环境变量"; \
		exit 1; \
	fi
	@echo "创建目标目录..."
	ssh $(TARGET_USER)@$(TARGET_IP) "mkdir -p $(TARGET_PATH)"
	@echo "复制文件..."
	scp -r $(BIN_DIR)/* $(TARGET_USER)@$(TARGET_IP):$(TARGET_PATH)/
	scp -r $(CONFIG_DIR)/* $(TARGET_USER)@$(TARGET_IP):$(TARGET_PATH)/ 2>/dev/null || true
	scp -r $(SCRIPTS_DIR)/* $(TARGET_USER)@$(TARGET_IP):$(TARGET_PATH)/ 2>/dev/null || true
	@echo "设置执行权限..."
	ssh $(TARGET_USER)@$(TARGET_IP) "chmod +x $(TARGET_PATH)/* 2>/dev/null || true"
	@echo "部署完成"

# 在目标板上运行测试
test-target: deploy
	@echo "在目标板上运行测试..."
	ssh $(TARGET_USER)@$(TARGET_IP) "cd $(TARGET_PATH) && ./temp_humidity_monitor --test"

# 本地测试
test: $(TARGET_APP)
	@echo "运行本地测试..."
	./$(TARGET_APP) --test

# 安装系统服务（在目标板上）
install-service:
	@echo "安装系统服务..."
	@if [ -z "$(TARGET_IP)" ]; then \
		echo "错误: 请设置 TARGET_IP 环境变量"; \
		exit 1; \
	fi
	scp $(CONFIG_DIR)/systemd/temp_humidity_monitor.service $(TARGET_USER)@$(TARGET_IP):/etc/systemd/system/
	ssh $(TARGET_USER)@$(TARGET_IP) "systemctl daemon-reload"
	ssh $(TARGET_USER)@$(TARGET_IP) "systemctl enable temp_humidity_monitor.service"
	@echo "服务安装完成，使用 systemctl start temp_humidity_monitor 启动"

# 生成文档
docs:
	@echo "生成文档..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile 2>/dev/null || echo "请创建 Doxyfile 配置文件"; \
	else \
		echo "警告: doxygen 未安装，跳过文档生成"; \
	fi

# 静态代码分析
analyze:
	@echo "运行静态代码分析..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --suppress=missingIncludeSystem $(SRC_DIR) $(DRIVER_DIR) 2>/dev/null; \
	else \
		echo "警告: cppcheck 未安装"; \
	fi

# 帮助信息
help:
	@echo "飞腾温湿度监测系统构建工具"
	@echo ""
	@echo "可用目标:"
	@echo "  all             编译所有目标（默认）"
	@echo "  clean           清理构建文件"
	@echo "  distclean       完全清理"
	@echo "  deploy          部署到目标板（需要设置 TARGET_IP）"
	@echo "  test            本地测试"
	@echo "  test-target     在目标板上测试"
	@echo "  install-service 安装系统服务"
	@echo "  docs            生成文档"
	@echo "  analyze         静态代码分析"
	@echo "  help            显示此帮助信息"
	@echo ""
	@echo "环境变量:"
	@echo "  CROSS_COMPILE   交叉编译工具链前缀（默认: aarch64-linux-gnu-）"
	@echo "  TARGET_IP       目标板IP地址（默认: 192.168.1.100）"
	@echo "  TARGET_USER     目标板用户名（默认: root）"
	@echo "  TARGET_PATH     目标板部署路径（默认: /root/projects/temperature_humidity_monitor）"
	@echo "  KERNEL_DIR      内核头文件目录"
	@echo ""
	@echo "使用示例:"
	@echo "  make all                         # 编译所有"
	@echo "  make deploy TARGET_IP=192.168.1.50  # 部署到指定IP"
	@echo "  make clean                       # 清理"

.PHONY: all prepare clean distclean deploy test test-target install-service docs analyze help drivers prepare_kernel