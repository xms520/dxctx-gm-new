#!/bin/bash
# dxctx_gm Deploy Script
# 部署GM脚本到游戏沙盒

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GM_JS="$SCRIPT_DIR/src/gm_template.js"
TARGET_DIR="${1:-/var/mobile/Library/MobileDevice/Provisioning Applications}"

echo "DXCT GM Deploy Script"
echo "====================="
echo ""
echo "GM JS source: $GM_JS"
echo "Target sandbox: $TARGET_DIR"
echo ""

# 检查源文件
if [ ! -f "$GM_JS" ]; then
    echo "Error: GM JS file not found: $GM_JS"
    exit 1
fi

# 提示用户
echo "Next steps:"
echo "1. Copy gm_template.js to game sandbox:"
echo "   cp $GM_JS /path/to/Game/Sandbox/Documents/dxctx_gm.js"
echo ""
echo "2. Inject dxctx_gm.dylib using 全能签 or TrollStore"
echo ""
echo "3. Set environment variables:"
echo "   export DXCT_ENABLE=1"
echo "   export DXCT_JS_FILE=/var/mobile/Documents/dxctx_gm.js"
echo ""
echo "4. Launch game and check logs:"
echo "   log stream --predicate 'subsystem contains \"DXCTGM\"'"
