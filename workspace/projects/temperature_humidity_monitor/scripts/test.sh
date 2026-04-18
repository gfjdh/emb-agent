#!/bin/bash

# 飞腾温湿度监测系统测试脚本
# 用法: ./test.sh [测试类型]

set -e

# 默认配置
TEST_TYPE=${1:-all}
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
BIN_DIR="${PROJECT_DIR}/bin"

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

# 检查编译
test_compile() {
    print_info "测试编译..."
    
    cd "${PROJECT_DIR}"
    
    # 清理
    if ! make clean; then
        print_error "清理失败"
        return 1
    fi
    
    # 编译
    if ! make all; then
        print_error "编译失败"
        return 1
    fi
    
    # 检查生成的文件
    if [ ! -f "${BIN_DIR}/temp_humidity_monitor" ]; then
        print_error "未生成可执行文件"
        return 1
    fi
    
    print_info "编译测试通过"
    return 0
}

# 测试程序基本功能
test_basic_function() {
    print_info "测试程序基本功能..."
    
    local program="${BIN_DIR}/temp_humidity_monitor"
    
    # 测试帮助信息
    if ! "${program}" --help 2>&1 | grep -q "飞腾温湿度监测系统"; then
        print_error "帮助信息测试失败"
        return 1
    fi
    
    # 测试版本信息（如果有）
    if "${program}" --version 2>&1 | grep -q "version"; then
        print_info "版本信息测试通过"
    fi
    
    # 测试无效参数
    if "${program}" --invalid-param 2>&1 | grep -q "未知选项"; then
        print_info "无效参数处理测试通过"
    fi
    
    print_info "基本功能测试通过"
    return 0
}

# 测试模拟模式
test_simulation() {
    print_info "测试模拟模式..."
    
    local program="${BIN_DIR}/temp_humidity_monitor"
    
    # 创建测试目录
    local test_dir="/tmp/temp_humidity_test"
    rm -rf "${test_dir}"
    mkdir -p "${test_dir}"
    
    # 运行模拟测试（短时间运行）
    timeout 5 "${program}" --verbose --log "${test_dir}/test.log" &
    local pid=$!
    
    # 等待程序启动
    sleep 2
    
    # 检查进程是否运行
    if ! kill -0 "${pid}" 2>/dev/null; then
        print_error "模拟测试进程异常退出"
        return 1
    fi
    
    # 停止进程
    kill -TERM "${pid}" 2>/dev/null
    wait "${pid}" 2>/dev/null
    
    # 检查日志文件
    if [ -f "${test_dir}/test.log" ]; then
        local line_count=$(wc -l < "${test_dir}/test.log")
        if [ "${line_count}" -gt 0 ]; then
            print_info "日志文件测试通过（${line_count}行）"
        else
            print_warn "日志文件为空"
        fi
    else
        print_warn "未生成日志文件"
    fi
    
    # 清理
    rm -rf "${test_dir}"
    
    print_info "模拟模式测试通过"
    return 0
}

# 测试代码质量
test_code_quality() {
    print_info "测试代码质量..."
    
    cd "${PROJECT_DIR}"
    
    # 检查代码格式（如果安装了clang-format）
    if command -v clang-format &> /dev/null; then
        print_info "检查代码格式..."
        find src drivers include -name "*.c" -o -name "*.h" | while read file; do
            if ! clang-format --style=file --dry-run -Werror "${file}" 2>/dev/null; then
                print_warn "代码格式问题: ${file}"
            fi
        done
    else
        print_warn "clang-format未安装，跳过代码格式检查"
    fi
    
    # 静态代码分析（如果安装了cppcheck）
    if command -v cppcheck &> /dev/null; then
        print_info "运行静态代码分析..."
        cppcheck --enable=all --suppress=missingIncludeSystem \
                 --suppress=unusedFunction \
                 src/ drivers/ include/ 2>&1 | grep -E "(error|warning)" || true
    else
        print_warn "cppcheck未安装，跳过静态代码分析"
    fi
    
    # 检查头文件依赖
    print_info "检查头文件依赖..."
    if make -n 2>&1 | grep -q "gcc.*-I"; then
        print_info "头文件包含路径检查通过"
    fi
    
    print_info "代码质量测试完成"
    return 0
}

# 测试内存泄漏（如果安装了valgrind）
test_memory_leak() {
    print_info "测试内存泄漏..."
    
    if ! command -v valgrind &> /dev/null; then
        print_warn "valgrind未安装，跳过内存泄漏测试"
        return 0
    fi
    
    local program="${BIN_DIR}/temp_humidity_monitor"
    local test_dir="/tmp/valgrind_test"
    
    mkdir -p "${test_dir}"
    
    # 运行valgrind测试
    if timeout 10 valgrind --leak-check=full --show-leak-kinds=all \
        --track-origins=yes --error-exitcode=1 \
        "${program}" --test 2>&1 > "${test_dir}/valgrind.log"; then
        print_info "内存泄漏测试通过"
    else
        print_error "内存泄漏测试失败"
        print_debug "查看详细日志: ${test_dir}/valgrind.log"
        return 1
    fi
    
    # 检查valgrind输出
    if grep -q "ERROR SUMMARY: 0 errors" "${test_dir}/valgrind.log"; then
        print_info "无内存错误"
    else
        print_warn "发现内存问题，查看详细日志"
    fi
    
    rm -rf "${test_dir}"
    return 0
}

# 测试构建系统
test_build_system() {
    print_info "测试构建系统..."
    
    cd "${PROJECT_DIR}"
    
    # 测试clean目标
    if ! make clean; then
        print_error "clean目标测试失败"
        return 1
    fi
    
    # 检查是否清理干净
    if [ -d "${BUILD_DIR}" ] && [ "$(ls -A ${BUILD_DIR} 2>/dev/null)" ]; then
        print_warn "build目录未完全清理"
    fi
    
    # 测试重新构建
    if ! make all; then
        print_error "重新构建测试失败"
        return 1
    fi
    
    # 测试增量构建
    touch src/main.c
    if ! make all; then
        print_error "增量构建测试失败"
        return 1
    fi
    
    print_info "构建系统测试通过"
    return 0
}

# 运行所有测试
run_all_tests() {
    local failed_tests=()
    
    print_info "开始运行所有测试..."
    
    # 测试编译
    if ! test_compile; then
        failed_tests+=("编译测试")
    fi
    
    # 测试构建系统
    if ! test_build_system; then
        failed_tests+=("构建系统测试")
    fi
    
    # 测试基本功能
    if ! test_basic_function; then
        failed_tests+=("基本功能测试")
    fi
    
    # 测试模拟模式
    if ! test_simulation; then
        failed_tests+=("模拟模式测试")
    fi
    
    # 测试代码质量
    if ! test_code_quality; then
        failed_tests+=("代码质量测试")
    fi
    
    # 测试内存泄漏
    if ! test_memory_leak; then
        failed_tests+=("内存泄漏测试")
    fi
    
    # 输出测试结果
    echo ""
    echo "================================================"
    print_info "测试完成"
    
    if [ ${#failed_tests[@]} -eq 0 ]; then
        print_info "所有测试通过！"
        return 0
    else
        print_error "以下测试失败:"
        for test in "${failed_tests[@]}"; do
            print_error "  - ${test}"
        done
        return 1
    fi
}

# 显示帮助信息
show_help() {
    echo "飞腾温湿度监测系统测试脚本"
    echo "用法: $0 [测试类型]"
    echo ""
    echo "测试类型:"
    echo "  all             运行所有测试（默认）"
    echo "  compile         仅测试编译"
    echo "  basic           仅测试基本功能"
    echo "  simulation      仅测试模拟模式"
    echo "  quality         仅测试代码质量"
    echo "  memory          仅测试内存泄漏"
    echo "  build           仅测试构建系统"
    echo "  help            显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0               # 运行所有测试"
    echo "  $0 compile       # 仅测试编译"
    echo "  $0 basic         # 仅测试基本功能"
}

# 主函数
main() {
    case "${TEST_TYPE}" in
        all)
            run_all_tests
            ;;
        compile)
            test_compile
            ;;
        basic)
            test_basic_function
            ;;
        simulation)
            test_simulation
            ;;
        quality)
            test_code_quality
            ;;
        memory)
            test_memory_leak
            ;;
        build)
            test_build_system
            ;;
        help|--help|-h)
            show_help
            return 0
            ;;
        *)
            print_error "未知的测试类型: ${TEST_TYPE}"
            show_help
            return 1
            ;;
    esac
    
    return $?
}

# 运行主函数
main "$@"