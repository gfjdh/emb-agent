#!/bin/bash

# 飞腾温湿度监测系统部署脚本
# 用法: ./deploy.sh [目标板IP]

set -e

# 默认配置
TARGET_IP=${1:-192.168.1.100}
TARGET_USER="root"
PROJECT_NAME="temperature_humidity_monitor"
LOCAL_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TARGET_DIR="/root/projects/${PROJECT_NAME}"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查依赖
check_dependencies() {
    print_info "检查依赖..."
    
    local missing_deps=()
    
    # 检查ssh
    if ! command -v ssh &> /dev/null; then
        missing_deps+=("ssh")
    fi
    
    # 检查scp
    if ! command -v scp &> /dev/null; then
        missing_deps+=("scp")
    fi
    
    # 检查make
    if ! command -v make &> /dev/null; then
        missing_deps+=("make")
    fi
    
    # 检查交叉编译器
    if ! command -v aarch64-linux-gnu-gcc &> /dev/null; then
        print_warn "未找到aarch64-linux-gnu-gcc，请确保交叉编译工具链已安装"
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "缺少依赖: ${missing_deps[*]}"
        return 1
    fi
    
    print_info "所有依赖检查通过"
    return 0
}

# 检查目标板连接
check_target_connection() {
    print_info "检查目标板连接: ${TARGET_USER}@${TARGET_IP}"
    
    if ! ssh -o ConnectTimeout=5 ${TARGET_USER}@${TARGET_IP} "echo '连接测试成功'" &> /dev/null; then
        print_error "无法连接到目标板 ${TARGET_IP}"
        print_error "请检查:"
        print_error "  1. 目标板IP地址是否正确"
        print_error "  2. 网络连接是否正常"
        print_error "  3. SSH服务是否运行"
        print_error "  4. 防火墙设置"
        return 1
    fi
    
    print_info "目标板连接成功"
    return 0
}

# 编译项目
compile_project() {
    print_info "编译项目..."
    
    cd "${LOCAL_DIR}"
    
    if ! make clean all; then
        print_error "编译失败"
        return 1
    fi
    
    print_info "编译成功"
    return 0
}

# 准备目标板目录
prepare_target_dir() {
    print_info "准备目标板目录: ${TARGET_DIR}"
    
    ssh ${TARGET_USER}@${TARGET_IP} "
        # 创建项目目录
        mkdir -p ${TARGET_DIR}
        mkdir -p ${TARGET_DIR}/bin
        mkdir -p ${TARGET_DIR}/config
        mkdir -p ${TARGET_DIR}/logs
        
        # 创建日志目录
        mkdir -p /var/log/temp_humidity
        
        echo '目标板目录准备完成'
    "
    
    return 0
}

# 部署文件
deploy_files() {
    print_info "部署文件到目标板..."
    
    # 部署可执行文件
    if [ -f "${LOCAL_DIR}/bin/temp_humidity_monitor" ]; then
        scp "${LOCAL_DIR}/bin/temp_humidity_monitor" \
            ${TARGET_USER}@${TARGET_IP}:${TARGET_DIR}/bin/
        print_info "部署应用程序完成"
    else
        print_error "未找到可执行文件"
        return 1
    fi
    
    # 部署配置文件
    if [ -d "${LOCAL_DIR}/config" ]; then
        scp -r "${LOCAL_DIR}/config/"* \
            ${TARGET_USER}@${TARGET_IP}:${TARGET_DIR}/config/
        print_info "部署配置文件完成"
    fi
    
    # 部署脚本文件
    if [ -d "${LOCAL_DIR}/scripts" ]; then
        scp -r "${LOCAL_DIR}/scripts/"* \
            ${TARGET_USER}@${TARGET_IP}:${TARGET_DIR}/
        print_info "部署脚本文件完成"
    fi
    
    # 设置执行权限
    ssh ${TARGET_USER}@${TARGET_IP} "
        chmod +x ${TARGET_DIR}/bin/temp_humidity_monitor
        chmod +x ${TARGET_DIR}/*.sh 2>/dev/null || true
    "
    
    return 0
}

# 安装系统服务
install_service() {
    print_info "安装系统服务..."
    
    # 复制service文件
    if [ -f "${LOCAL_DIR}/config/systemd/temp_humidity_monitor.service" ]; then
        scp "${LOCAL_DIR}/config/systemd/temp_humidity_monitor.service" \
            ${TARGET_USER}@${TARGET_IP}:/etc/systemd/system/
    else
        print_warn "未找到service文件，跳过服务安装"
        return 0
    fi
    
    # 重新加载systemd并启用服务
    ssh ${TARGET_USER}@${TARGET_IP} "
        systemctl daemon-reload
        systemctl enable temp_humidity_monitor.service
    "
    
    print_info "系统服务安装完成"
    print_info "使用以下命令管理服务:"
    print_info "  systemctl start temp_humidity_monitor.service"
    print_info "  systemctl stop temp_humidity_monitor.service"
    print_info "  systemctl status temp_humidity_monitor.service"
    
    return 0
}

# 测试部署
test_deployment() {
    print_info "测试部署..."
    
    # 测试程序是否能运行
    if ssh ${TARGET_USER}@${TARGET_IP} "${TARGET_DIR}/bin/temp_humidity_monitor --help" &> /dev/null; then
        print_info "程序测试通过"
    else
        print_error "程序测试失败"
        return 1
    fi
    
    # 检查依赖库
    ssh ${TARGET_USER}@${TARGET_IP} "
        echo '检查动态库依赖...'
        ldd ${TARGET_DIR}/bin/temp_humidity_monitor || true
    "
    
    return 0
}

# 显示部署信息
show_deployment_info() {
    print_info "部署完成！"
    echo ""
    echo "================================================"
    echo "部署信息:"
    echo "  目标板: ${TARGET_USER}@${TARGET_IP}"
    echo "  项目目录: ${TARGET_DIR}"
    echo "  日志文件: /var/log/temp_humidity.log"
    echo ""
    echo "使用说明:"
    echo "  1. 手动运行:"
    echo "     ssh ${TARGET_USER}@${TARGET_IP}"
    echo "     cd ${TARGET_DIR}"
    echo "     ./bin/temp_humidity_monitor --sensor dht11 --pin 17"
    echo ""
    echo "  2. 使用系统服务:"
    echo "     systemctl start temp_humidity_monitor.service"
    echo "     systemctl status temp_humidity_monitor.service"
    echo ""
    echo "  3. 查看日志:"
    echo "     tail -f /var/log/temp_humidity.log"
    echo "     journalctl -f -u temp_humidity_monitor"
    echo "================================================"
}

# 主函数
main() {
    print_info "开始部署飞腾温湿度监测系统"
    print_info "目标板: ${TARGET_IP}"
    print_info "本地目录: ${LOCAL_DIR}"
    
    # 检查依赖
    if ! check_dependencies; then
        exit 1
    fi
    
    # 检查连接
    if ! check_target_connection; then
        exit 1
    fi
    
    # 编译项目
    if ! compile_project; then
        exit 1
    fi
    
    # 准备目标目录
    if ! prepare_target_dir; then
        exit 1
    fi
    
    # 部署文件
    if ! deploy_files; then
        exit 1
    fi
    
    # 安装服务（可选）
    read -p "是否安装系统服务？(y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        if ! install_service; then
            print_warn "服务安装失败，但部署继续"
        fi
    fi
    
    # 测试部署
    if ! test_deployment; then
        print_warn "部署测试失败，请手动检查"
    fi
    
    # 显示部署信息
    show_deployment_info
    
    print_info "部署完成！"
}

# 运行主函数
main "$@"