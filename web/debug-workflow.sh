#!/bin/bash

set -e  # 遇到错误时退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_VUE="$SCRIPT_DIR/src/views/root/Root.vue"
ROOT_VUE_BACKUP="$SCRIPT_DIR/src/views/root/Root.vue.backup"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
QT_EXECUTABLE="$PROJECT_ROOT/build/src/uos-ai-assistant"

# 三种模式的 Root.vue 内容
PROD_MODE_CONTENT='<script setup>
import initialize from "./prodmode";
initialize();
</script>
'

MOCK_MODE_CONTENT='<script setup>
import initialize from "./mockmode";
initialize();
</script>
'

DEVFRONT_MODE_CONTENT='<script setup>
import initialize from "./devfront";
initialize();
</script>
'

# 日志函数
log_info() {
    echo -e "${CYAN}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_step() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

# 检查是否在运行中
cleanup_manual() {
    log_info "正在清理进程..."
    stop_qt_app
    stop_dev_server
    stop_mock_server
    restore_root_vue
}

# 注意：自动退出时不清理，让服务器继续运行
# 用户可以手动执行 clean 命令清理
# trap cleanup_manual EXIT INT TERM

# 备份原始文件
backup_root_vue() {
    if [ ! -f "$ROOT_VUE_BACKUP" ]; then
        log_info "备份原始 Root.vue 文件..."
        cp "$ROOT_VUE" "$ROOT_VUE_BACKUP"
        log_success "备份完成: $ROOT_VUE_BACKUP"
    else
        log_warning "备份文件已存在，跳过备份"
    fi
}

# 恢复原始文件
restore_root_vue() {
    if [ -f "$ROOT_VUE_BACKUP" ]; then
        log_info "恢复原始 Root.vue 文件..."
        cp "$ROOT_VUE_BACKUP" "$ROOT_VUE"
        log_success "恢复完成"
    else
        log_warning "备份文件不存在，无法恢复"
    fi
}

# 设置指定的模式
set_mode() {
    local mode="$1"
    log_info "设置模式为: $mode"

    case "$mode" in
        prod)
            echo -e "$PROD_MODE_CONTENT" > "$ROOT_VUE"
            ;;
        mock)
            echo -e "$MOCK_MODE_CONTENT" > "$ROOT_VUE"
            ;;
        devfront)
            echo -e "$DEVFRONT_MODE_CONTENT" > "$ROOT_VUE"
            ;;
        *)
            log_error "未知模式: $mode"
            exit 1
            ;;
    esac

    log_success "模式设置完成"
}

# 编译前端代码
build_frontend() {
    log_info "编译前端代码..."
    cd "$SCRIPT_DIR"

    npm run build
    sed -i 's/ nomodule/ /g' dist/index.html
    log_success "编译成功"
}

# 编译 CMake 项目
build_cmake() {
    log_info "编译 CMake 项目..."
    cd "$PROJECT_ROOT"

    # 检查 build 目录是否存在
    if [ ! -d "build" ]; then
        log_info "创建 build 目录..."
        mkdir -p build
        cd build
        log_info "运行 CMake 配置..."
        cmake .. || {
            log_error "CMake 配置失败"
            exit 1
        }
    else
        cd build
    fi

    log_info "编译项目..."
    make -j$(nproc) || {
        log_error "编译失败"
        exit 1
    }

    log_success "CMake 项目编译完成"
}

# 启动 Mock 服务器
start_mock_server() {
    log_info "启动 Mock 服务器..."
    cd "$SCRIPT_DIR"

    # 检查是否已经在运行
    if pgrep -f "node.*websocket-server.js" > /dev/null; then
        log_warning "Mock 服务器已在运行，跳过启动"
        return
    fi

    # 在后台启动 mock 服务器
    nohup npm run mock > mock-server.log 2>&1 &
    MOCK_PID=$!
    echo $MOCK_PID > mock-server.pid
    log_success "Mock 服务器已启动 (PID: $MOCK_PID)"

    # 等待服务器启动
    log_info "等待 Mock 服务器启动..."
    sleep 2
}

# 停止 Mock 服务器
stop_mock_server() {
    if [ -f "$SCRIPT_DIR/mock-server.pid" ]; then
        MOCK_PID=$(cat "$SCRIPT_DIR/mock-server.pid")
        if ps -p $MOCK_PID > /dev/null; then
            log_info "停止 Mock 服务器 (PID: $MOCK_PID)..."
            kill $MOCK_PID 2>/dev/null || true
            rm "$SCRIPT_DIR/mock-server.pid"
        fi
    fi
    pkill -f "node.*websocket-server.js" > /dev/null 2>&1 || true
}

# 启动 Qt 应用程序
start_qt_app() {
    log_info "启动 Qt 应用程序..."
    log_info "可执行文件: $QT_EXECUTABLE"

    if [ ! -f "$QT_EXECUTABLE" ]; then
        log_error "Qt 可执行文件不存在: $QT_EXECUTABLE"
        log_info "请先编译 Qt 项目"
        exit 1
    fi

    # 先强制终止所有已运行的 uos-ai-assistant 进程
    log_info "停止现有的 uos-ai-assistant 进程..."
    killall -9 uos-ai-assistant 2>/dev/null || true
    sleep 1

    # 在后台启动 Qt 应用，带上 --chat 参数
    nohup "$QT_EXECUTABLE" --chat > qt-app.log 2>&1 &
    # nohup "$QT_EXECUTABLE" > qt-app.log 2>&1 &
    QT_PID=$!
    echo $QT_PID > qt-app.pid
    log_success "Qt 应用已启动 (PID: $QT_PID)"

    # 等待应用启动
    log_info "等待 Qt 应用启动..."
    sleep 10

    # 调用 D-Bus 启动聊天界面
    # log_info "调用 D-Bus 启动聊天界面..."
    # gdbus call --session \
    #     --dest com.deepin.copilot \
    #     --object-path /com/deepin/copilot \
    #     --method com.deepin.copilot.launchChatPage || {
    #     log_warning "D-Bus 调用失败，可能应用尚未完全启动"
    # }

    log_success "聊天界面已启动"
}

# 停止 Qt 应用程序
stop_qt_app() {
    if [ -f "$PROJECT_ROOT/qt-app.pid" ]; then
        QT_PID=$(cat "$PROJECT_ROOT/qt-app.pid")
        if ps -p $QT_PID > /dev/null; then
            log_info "停止 Qt 应用 (PID: $QT_PID)..."
            kill $QT_PID 2>/dev/null || true
            rm "$PROJECT_ROOT/qt-app.pid"
        fi
    fi
    # 尝试通过进程名查找
    pkill -f "uos-ai-assistant" > /dev/null 2>&1 || true
}

# 启动前端调试服务器
start_dev_server() {
    log_info "启动前端调试服务器..."
    cd "$SCRIPT_DIR"

    # 检查是否已经在运行
    if pgrep -f "vite.*dev" > /dev/null; then
        log_warning "前端调试服务器已在运行，跳过启动"
        return
    fi

    # 在后台启动 dev 服务器
    nohup npm run dev > dev-server.log 2>&1 &
    DEV_PID=$!
    echo $DEV_PID > dev-server.pid
    log_success "前端调试服务器已启动 (PID: $DEV_PID)"

    # 等待服务器启动并提取URL
    log_info "等待前端调试服务器启动..."
    sleep 3

    # 从日志中提取 URL
    DEV_URL=$(grep -o 'http://[^[:space:]]*' "$SCRIPT_DIR/dev-server.log" | head -1)
    if [ -n "$DEV_URL" ]; then
        log_success "前端调试服务器已就绪"
        echo -e "${BLUE}========================================${NC}"
        echo -e "${CYAN}请在浏览器中打开以下链接进行调试:${NC}"
        echo -e "${GREEN}$DEV_URL${NC}"
        echo -e "${BLUE}========================================${NC}"
    else
        log_warning "未能自动获取调试URL，请查看 dev-server.log"
    fi
}

# 停止前端调试服务器
stop_dev_server() {
    if [ -f "$SCRIPT_DIR/dev-server.pid" ]; then
        DEV_PID=$(cat "$SCRIPT_DIR/dev-server.pid")
        if ps -p $DEV_PID > /dev/null; then
            log_info "停止前端调试服务器 (PID: $DEV_PID)..."
            kill $DEV_PID 2>/dev/null || true
            rm "$SCRIPT_DIR/dev-server.pid"
        fi
    fi
    pkill -f "vite.*dev" > /dev/null 2>&1 || true
}

# 步骤1: 启动服务器
step1_start_server() {
    log_step "步骤 1: 启动服务器"

    # 1.1 打开 probemode
    log_info "1.1 打开 probemode，屏蔽其他两个"
    set_mode prod

    # 1.2 编译前端代码
    log_info "1.2 编译前端代码"
    build_frontend

    # 1.3 npm run mock
    log_info "1.3 启动 Mock 服务器"
    start_mock_server

    log_success "步骤 1 完成"
}

# 步骤2: 修改前端代码为 mock 模式
step2_mock_mode() {
    log_step "步骤 2: 修改前端代码为 mock 模式"

    # 2.1 打开 mockmode
    log_info "2.1 打开 mockmode，屏蔽其他两个"
    set_mode mock

    # 2.2 编译前端代码
    log_info "2.2 编译前端代码"
    build_frontend

    # 2.3 编译 CMake 项目
    log_info "2.3 编译 CMake 项目"
    build_cmake

    # 2.4 启动 Qt 应用
    log_info "2.4 启动 Qt 应用程序"
    start_qt_app

    log_success "步骤 2 完成"
    echo ""
    log_info "自动继续到步骤 3..."
    sleep 1
}

# 步骤3: 修改前端代码为调试模式
step3_debug_mode() {
    log_step "步骤 3: 修改前端代码为调试模式"

    # 注意: 不停止 Qt 应用，让它持续运行

    # 3.1 打开 devfront
    log_info "3.1 打开 devfront，屏蔽其他两个"
    set_mode devfront

    # 3.2 编译前端代码
    log_info "3.2 编译前端代码"
    build_frontend

    # 3.3 npm run dev
    log_info "3.3 启动前端调试服务器"
    start_dev_server

    log_success "步骤 3 完成"
    log_success "所有调试服务已就绪"
    echo ""
    echo "后台调试服务正在运行："
    echo "  - Mock 服务器 (PID: $(cat mock-server.pid 2>/dev/null || echo 'N/A'))"
    echo "  - Qt 应用 (PID: $(cat "$PROJECT_ROOT/qt-app.pid" 2>/dev/null || echo 'N/A'))"
    echo "  - 前端调试服务器 (PID: $(cat dev-server.pid 2>/dev/null || echo 'N/A'))"
    echo ""
    echo "日志文件："
    echo "  - Mock 服务器: mock-server.log"
    echo "  - Qt 应用: $(basename "$PROJECT_ROOT")/qt-app.log"
    echo "  - 前端调试服务器: dev-server.log"
    echo ""
    echo -e "${GREEN}调试服务器链接：${CYAN}"
    grep -o 'http://[^[:space:]]*' "$SCRIPT_DIR/dev-server.log" | head -1 || echo "请查看 dev-server.log"
    echo -e "${NC}"
    echo ""
    echo "调试完成后，执行以下命令清理所有服务并恢复配置："
    echo -e "${GREEN}./debug-workflow.sh clean${NC}"
    echo ""
}

# 完整调试流程
full_debug_workflow() {
    log_info "开始完整调试流程..."

    # 备份原始文件
    backup_root_vue

    # 步骤1: 启动服务器
    step1_start_server

    # 步骤2: 修改前端代码为 mock 模式
    step2_mock_mode

    # 步骤3: 修改前端代码为调试模式
    step3_debug_mode

    # 等待用户按Ctrl+C
    wait_forever
}

# 永久等待
wait_forever() {
    while true; do
        sleep 1
    done
}

# 帮助信息
show_help() {
    cat << EOF
UOS AI 调试流程脚本

用法: $0 [命令] [参数]

完整调试流程:
    workflow         执行完整调试流程（推荐）
                     步骤1: 启动 Mock 服务器
                     步骤2: 编译前端 + CMake 并启动 Qt 应用和聊天界面
                     步骤3: 启动前端调试服务器
                     调试完成后执行: $0 clean

单独执行步骤:
    step1            执行步骤 1（启动 Mock 服务器）
    step2            执行步骤 2（编译前端和 CMake，启动 Qt 应用）
    step3            执行步骤 3（启动前端调试服务器）

其他命令:
    backup           备份原始 Root.vue 文件
    restore          恢复原始 Root.vue 文件
    set [mode]       设置指定模式 (prod/mock/devfront)
    build            编译前端代码
    build-cmake      编译 CMake 项目
    start-mock       启动 Mock 服务器
    stop-mock        停止 Mock 服务器
    start-qt         启动 Qt 应用
    stop-qt          停止 Qt 应用
    start-dev        启动前端调试服务器
    stop-dev         停止前端调试服务器
    clean            清理所有进程和日志并恢复配置
    help             显示帮助信息

使用示例:
    $0 workflow       # 执行完整调试流程
    $0 step2          # 只执行步骤 2
    $0 build-cmake    # 只编译 CMake 项目
    $0 clean          # 清理所有服务并恢复配置

注意:
    - 脚本会在 Root.vue.backup 中备份原始文件
    - 所有后台服务会在后台运行，日志保存到 .log 文件
    - 步骤3不会停止 Qt 应用，所有服务持续运行
    - 调试完成后必须执行 clean 命令清理并恢复配置
    - Qt 可执行文件路径: $QT_EXECUTABLE
EOF
}

# 主函数
main() {
    local command="${1:-help}"

    case "$command" in
        workflow)
            full_debug_workflow
            ;;
        step1)
            backup_root_vue
            step1_start_server
            log_success "Mock 服务器已启动，可以继续步骤 2"
            ;;
        step2)
            backup_root_vue
            step2_mock_mode
            ;;
        step3)
            backup_root_vue
            step3_debug_mode
            log_warning "前端调试服务器已启动，按 Ctrl+C 结束"
            wait
            ;;
        start-mock)
            backup_root_vue
            start_mock_server
            log_warning "按 Ctrl+C 停止 Mock 服务器"
            wait
            ;;
        stop-mock)
            stop_mock_server
            restore_root_vue
            ;;
        start-qt)
            build_cmake
            backup_root_vue
            start_qt_app
            log_warning "按 Ctrl+C 停止 Qt 应用"
            wait
            ;;
        stop-qt)
            stop_qt_app
            restore_root_vue
            ;;
        start-dev)
            backup_root_vue
            start_dev_server
            log_warning "按 Ctrl+C 停止前端调试服务器"
            wait
            ;;
        stop-dev)
            stop_dev_server
            restore_root_vue
            ;;
        backup)
            backup_root_vue
            ;;
        restore)
            restore_root_vue
            ;;
        build)
            build_frontend
            ;;
        build-cmake)
            build_cmake
            ;;
        clean)
            stop_qt_app
            stop_dev_server
            stop_mock_server
            restore_root_vue
            rm -f "$SCRIPT_DIR/mock-server.log" "$SCRIPT_DIR/dev-server.log" "$PROJECT_ROOT/qt-app.log"
            rm -f "$SCRIPT_DIR/mock-server.pid" "$SCRIPT_DIR/dev-server.pid" "$PROJECT_ROOT/qt-app.pid"
            log_success "清理完成"
            ;;
        set)
            local mode="${2:-}"
            if [ -z "$mode" ]; then
                log_error "请指定模式 (prod/mock/devfront)"
                exit 1
            fi
            backup_root_vue
            set_mode "$mode"
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            log_error "未知命令: $command"
            show_help
            exit 1
            ;;
    esac
}

# 执行主函数
main "$@"
