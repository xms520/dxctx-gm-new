#!/bin/bash
# 部署脚本 - 大侠闯天下 GM Hook
# 将 dylib 和 GM 脚本部署到设备

set -e

echo "=== DXCT GM Hook Deployer ==="

# 检查参数
if [ $# -lt 1 ]; then
    echo "Usage: $0 <action>"
    echo "Actions:"
    echo "  deploy    - 部署到设备"
    echo "  diagnose  - 运行诊断"
    echo "  log       - 查看日志"
    echo "  help      - 显示帮助"
    exit 1
fi

ACTION=$1

case $ACTION in
    deploy)
        echo "[DXCT] Deploying GM hook..."
        
        # 检查dylib
        if [ ! -f "dxctx_gm.dylib" ]; then
            echo "[DXCT] Error: dxctx_gm.dylib not found"
            echo "[DXCT] Please compile first: ./build.sh"
            exit 1
        fi
        
        # 检查GM脚本
        if [ ! -f "src/gm_template.js" ]; then
            echo "[DXCT] Error: src/gm_template.js not found"
            exit 1
        fi
        
        echo "[DXCT] Files ready:"
        echo "  - dxctx_gm.dylib"
        echo "  - src/gm_template.js"
        echo ""
        echo "[DXCT] Deployment steps:"
        echo "  1. 使用全能签/TrollStore注入 dxctx_gm.dylib"
        echo "  2. 将 src/gm_template.js 复制到游戏沙盒 Documents 目录"
        echo "  3. 设置环境变量 DXCT_ENABLE=1"
        echo "  4. 启动游戏"
        echo "  5. 查看日志: log stream --predicate 'eventMessage contains \"DXCT\"'"
        ;;
        
    diagnose)
        echo "[DXCT] Running diagnostics..."
        
        # 检查诊断脚本
        if [ ! -f "src/diagnose.js" ]; then
            echo "[DXCT] Error: src/diagnose.js not found"
            exit 1
        fi
        
        echo "[DXCT] Diagnostic script: src/diagnose.js"
        echo ""
        echo "[DXCT] Deployment steps for diagnosis:"
        echo "  1. 将 src/diagnose.js 复制到游戏沙盒 Documents 目录"
        echo "  2. 设置环境变量:"
        echo "     export DXCT_ENABLE=1"
        echo "     export DXCT_JS_FILE=/var/mobile/Documents/diagnose.js"
        echo "  3. 启动游戏"
        echo "  4. 查看日志:"
        echo "     cat /var/mobile/Library/Logs/dxct_gm.log"
        echo "     cat /var/mobile/Library/Logs/dxct_diag.json"
        ;;
        
    log)
        echo "[DXCT] Viewing logs..."
        
        LOG_FILE="/var/mobile/Library/Logs/dxct_gm.log"
        
        if [ -f "$LOG_FILE" ]; then
            echo "--- DXCT GM Log ---"
            cat "$LOG_FILE"
        else
            echo "[DXCT] Log file not found: $LOG_FILE"
            echo "[DXCT] Try: log stream --predicate 'eventMessage contains \"DXCT\"'"
        fi
        ;;
        
    help|*)
        echo "Usage: $0 <action>"
        echo ""
        echo "Actions:"
        echo "  deploy    - 显示部署指南"
        echo "  diagnose  - 显示诊断步骤"
        echo "  log       - 查看日志"
        echo "  help      - 显示此帮助"
        ;;
esac

echo ""
echo "=== Done ==="
