#!/bin/bash
set -e

echo "=== dxctx_gm 本地编译测试 ==="
echo ""

# 检查Xcode
if ! command -v xcodebuild &> /dev/null; then
    echo "错误: 未找到Xcode，请在macOS上运行"
    exit 1
fi

# 获取SDK路径
SDK=$(xcrun --sdk iphoneos --show-sdk-path 2>/dev/null)
if [ -z "$SDK" ]; then
    echo "错误: 无法找到iOS SDK"
    exit 1
fi

MIN_VER=$(xcrun --sdk iphoneos --show-sdk-version)
echo "SDK: $SDK"
echo "最低版本: $MIN_VER"
echo ""

# 检查源文件
if [ ! -f "src/fishhook.c" ]; then
    echo "错误: 未找到 src/fishhook.c"
    exit 1
fi

if [ ! -f "src/Inject.jsb.c" ]; then
    echo "错误: 未找到 src/Inject.jsb.c"
    exit 1
fi

echo "开始编译..."
echo ""

# 编译
clang \
    -arch arm64 \
    -isysroot "$SDK" \
    -iframework "$SDK/System/Library/Frameworks" \
    -iframework "$SDK/Library/Frameworks" \
    -I src \
    -O2 \
    -Wall \
    -framework JavaScriptCore \
    -dynamiclib \
    -o dxctx_gm.dylib \
    src/fishhook.c \
    src/Inject.jsb.c \
    -Wl,-install_name,@rpath/dxctx_gm.dylib \
    -mios-version-min=$MIN_VER

echo "✅ 编译成功!"
echo ""

# 验证
echo "二进制信息:"
file dxctx_gm.dylib
echo ""
lipo -info dxctx_gm.dylib
echo ""
echo "导出符号:"
nm -U dxctx_gm.dylib | grep " T " | grep -E "(init|hook|rebind)" | head -10
echo ""
echo "文件大小: $(ls -lh dxctx_gm.dylib | awk '{print $5}')"
echo ""
echo "下一步: 将 dxctx_gm.dylib 复制到iOS设备使用"
