#!/bin/bash

# 飞腾温湿度监测系统设置脚本
# 用于设置开发环境和目标板环境

set -e

# 配置
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
TARGET_ARCH="aarch64-linux-gnu"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

print_debug() {
    echo -e "${BLUE}[DEBUG]${NC} $1"
}

# 显示横幅
show_banner() {
    echo ""
    echo "================================================"
    echo "  飞腾温湿度监测系统环境设置脚本"
    echo "================================================"
    echo ""
}

# 检查操作系统
check_os() {
    print_info "检查操作系统..."
    
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        print_info "操作系统: ${PRETTY_NAME}"
        
        case ${ID} in
            ubuntu|debian)
                OS_TYPE="debian"
                ;;
            centos|rhel|fedora)
                OS_TYPE="rhel"
                ;;
            arch)
                OS_TYPE="arch"
                ;;
            *)
                OS_TYPE="unknown"
                print_warn "未知的操作系统类型"
                ;;
        esac
    else
        print_warn "无法确定操作系统类型"
        OS_TYPE="unknown"
    fi
    
    return 0
}

# 安装开发工具（Ubuntu/Debian）
install_dev_tools_debian() {
    print_info "安装开发工具（Debian/Ubuntu）..."
    
    sudo apt-get update
    
    # 基本开发工具
    sudo apt-get install -y \
        build-essential \
        git \
        cmake \
        autoconf \
        automake \
        libtool \
        pkg-config
    
    # 交叉编译工具链
    sudo apt-get install -y \
        gcc-${TARGET_ARCH} \
        g++-${TARGET_ARCH}
    
    # 代码质量工具
    sudo apt-get install -y \
        cppcheck \
        valgrind \
        clang-format \
        doxygen \
        graphviz
    
    # Python工具（用于Web界面）
    sudo apt-get install -y \
        python3 \
        python3-pip \
        python3-venv
    
    print_info "开发工具安装完成"
}

# 安装开发工具（CentOS/RHEL/Fedora）
install_dev_tools_rhel() {
    print_info "安装开发工具（RHEL/CentOS/Fedora）..."
    
    # 基本开发工具
    sudo yum groupinstall -y "Development Tools"
    sudo yum install -y \
        git \
        cmake \
        autoconf \
        automake \
        libtool \
        pkgconfig
    
    # 交叉编译工具链
    sudo yum install -y \
        gcc-${TARGET_ARCH} \
        g++-${TARGET_ARCH}
    
    # 代码质量工具
    sudo yum install -y \
        cppcheck \
        valgrind \
        clang \
        doxygen \
        graphviz
    
    # Python工具
    sudo yum install -y \
        python3 \
        python3-pip
    
    print_info "开发工具安装完成"
}

# 安装开发工具（Arch Linux）
install_dev_tools_arch() {
    print_info "安装开发工具（Arch Linux）..."
    
    # 基本开发工具
    sudo pacman -S --noconfirm \
        base-devel \
        git \
        cmake \
        autoconf \
        automake \
        libtool \
        pkg-config
    
    # 交叉编译工具链
    sudo pacman -S --noconfirm \
        aarch64-linux-gnu-gcc \
        aarch64-linux-gnu-binutils
    
    # 代码质量工具
    sudo pacman -S --noconfirm \
        cppcheck \
        valgrind \
        clang \
        doxygen \
        graphviz
    
    # Python工具
    sudo pacman -S --noconfirm \
        python \
        python-pip
    
    print_info "开发工具安装完成"
}

# 安装开发工具
install_dev_tools() {
    print_info "安装开发工具..."
    
    case ${OS_TYPE} in
        debian)
            install_dev_tools_debian
            ;;
        rhel)
            install_dev_tools_rhel
            ;;
        arch)
            install_dev_tools_arch
            ;;
        *)
            print_error "不支持的操作系统类型: ${OS_TYPE}"
            print_error "请手动安装以下工具:"
            print_error "  - 交叉编译工具链 (${TARGET_ARCH}-gcc)"
            print_error "  - make, cmake, git"
            print_error "  - cppcheck, valgrind (可选)"
            return 1
            ;;
    esac
    
    return 0
}

# 检查交叉编译工具链
check_cross_compiler() {
    print_info "检查交叉编译工具链..."
    
    if command -v ${TARGET_ARCH}-gcc &> /dev/null; then
        print_info "找到交叉编译器: $(${TARGET_ARCH}-gcc --version | head -1)"
        return 0
    else
        print_error "未找到交叉编译器: ${TARGET_ARCH}-gcc"
        return 1
    fi
}

# 设置环境变量
setup_environment() {
    print_info "设置环境变量..."
    
    # 创建环境变量文件
    local env_file="${PROJECT_DIR}/.env"
    
    cat > "${env_file}" << EOF
# 飞腾温湿度监测系统环境变量
export PROJECT_DIR="${PROJECT_DIR}"
export TARGET_ARCH="${TARGET_ARCH}"
export CROSS_COMPILE="${TARGET_ARCH}-"
export PATH="\${PATH}:\${PROJECT_DIR}/scripts"

# 目标板配置
export TARGET_IP="192.168.1.100"
export TARGET_USER="root"
export TARGET_PATH="/root/projects/temperature_humidity_monitor"

# 编译选项
export CFLAGS="-O2 -g"
export LDFLAGS=""

# 提示信息
echo "环境已设置: 飞腾温湿度监测系统"
echo "项目目录: \${PROJECT_DIR}"
echo "目标架构: \${TARGET_ARCH}"
EOF
    
    # 设置权限
    chmod 644 "${env_file}"
    
    print_info "环境变量文件已创建: ${env_file}"
    print_info "使用以下命令加载环境变量:"
    print_info "  source ${env_file}"
    
    return 0
}

# 创建项目符号链接
create_symlinks() {
    print_info "创建项目符号链接..."
    
    # 创建bin目录符号链接到PATH
    local bin_link="${HOME}/.local/bin/temp_humidity_monitor"
    
    mkdir -p "${HOME}/.local/bin"
    
    if [ -f "${PROJECT_DIR}/bin/temp_humidity_monitor" ]; then
        ln -sf "${PROJECT_DIR}/bin/temp_humidity_monitor" "${bin_link}"
        print_info "创建符号链接: ${bin_link}"
    fi
    
    return 0
}

# 设置Git钩子
setup_git_hooks() {
    print_info "设置Git钩子..."
    
    local hooks_dir="${PROJECT_DIR}/.git/hooks"
    
    if [ -d "${PROJECT_DIR}/.git" ]; then
        mkdir -p "${hooks_dir}"
        
        # 创建pre-commit钩子
        cat > "${hooks_dir}/pre-commit" << 'EOF'
#!/bin/bash
# 预提交钩子：运行代码检查

echo "运行预提交检查..."

# 运行代码格式检查
if command -v clang-format &> /dev/null; then
    echo "检查代码格式..."
    find src drivers include -name "*.c" -o -name "*.h" | while read file; do
        clang-format --style=file -i "${file}"
    done
fi

# 运行静态分析
if command -v cppcheck &> /dev/null; then
    echo "运行静态代码分析..."
    cppcheck --enable=all --suppress=missingIncludeSystem \
             --suppress=unusedFunction \
             src/ drivers/ include/ 2>&1 | grep -E "(error|warning)" || true
fi

echo "预提交检查完成"
EOF
        
        chmod +x "${hooks_dir}/pre-commit"
        print_info "Git钩子设置完成"
    else
        print_warn "未找到.git目录，跳过Git钩子设置"
    fi
    
    return 0
}

# 配置目标板环境
setup_target_board() {
    print_info "配置目标板环境..."
    
    read -p "请输入目标板IP地址（默认: 192.168.1.100）: " target_ip
    target_ip=${target_ip:-192.168.1.100}
    
    read -p "请输入目标板用户名（默认: root）: " target_user
    target_user=${target_user:-root}
    
    # 测试连接
    print_info "测试目标板连接..."
    if ssh -o ConnectTimeout=5 ${target_user}@${target_ip} "echo '连接成功'" &> /dev/null; then
        print_info "目标板连接成功"
    else
        print_error "无法连接到目标板"
        print_error "请检查网络连接和SSH配置"
        return 1
    fi
    
    # 创建目标板配置
    local target_config="${PROJECT_DIR}/config/target_board.conf"
    
    cat > "${target_config}" << EOF
# 目标板配置
TARGET_IP="${target_ip}"
TARGET_USER="${target_user}"
TARGET_PATH="/root/projects/temperature_humidity_monitor"

# 传感器配置
DHT11_GPIO_PIN=17
DHT22_GPIO_PIN=18
I2C_BUS=1

# 采样配置
SAMPLE_INTERVAL=60
LOG_INTERVAL=3600
EOF
    
    print_info "目标板配置已保存: ${target_config}"
    
    # 更新环境变量
    sed -i "s/export TARGET_IP=.*/export TARGET_IP=\"${target_ip}\"/" "${PROJECT_DIR}/.env"
    sed -i "s/export TARGET_USER=.*/export TARGET_USER=\"${target_user}\"/" "${PROJECT_DIR}/.env"
    
    return 0
}

# 显示设置完成信息
show_setup_complete() {
    echo ""
    echo "================================================"
    print_info "环境设置完成！"
    echo ""
    echo "下一步操作:"
    echo "  1. 加载环境变量:"
    echo "     source ${PROJECT_DIR}/.env"
    echo ""
    echo "  2. 编译项目:"
    echo "     cd ${PROJECT_DIR}"
    echo "     make all"
    echo ""
    echo "  3. 运行测试:"
    echo "     ./scripts/test.sh"
    echo ""
    echo "  4. 部署到目标板:"
    echo "     ./scripts/deploy.sh"
    echo ""
    echo "  5. 开始开发:"
    echo "     编辑 src/ 目录下的源代码"
    echo "     查看文档: cat README.md"
    echo "================================================"
}

# 主函数
main() {
    show_banner
    
    # 检查操作系统
    check_os
    
    # 安装开发工具
    if [ "$1" != "skip-tools" ]; then
        read -p "是否安装开发工具？(y/n): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            install_dev_tools
        fi
    fi
    
    # 检查交叉编译器
    if ! check_cross_compiler; then
        print_error "交叉编译工具链未安装"
        print_error "请安装 ${TARGET_ARCH}-gcc 或修改 Makefile 中的 CROSS_COMPILE"
        return 1
    fi
    
    # 设置环境变量
    setup_environment
    
    # 创建符号链接
    create_symlinks
    
    # 设置Git钩子
    setup_git_hooks
    
    # 配置目标板
    read -p "是否配置目标板？(y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        setup_target_board
    fi
    
    # 显示完成信息
    show_setup_complete
    
    return 0
}

# 显示帮助信息
show_help() {
    echo "飞腾温湿度监测系统设置脚本"
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  skip-tools    跳过开发工具安装"
    echo "  help         显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0             # 完整设置"
    echo "  $0 skip-tools  # 跳过工具安装"
}

# 解析参数
case "$1" in
    help|--help|-h)
        show_help
        exit 0
        ;;
    *)
        main "$@"
        ;;
esac