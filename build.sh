#!/bin/bash
# 编译脚本 - dxctx_gm.dylib
# 需要 macOS + Xcode

set -e

echo "=== DXCT GM Hook Compiler ==="

# 获取 SDK 路径
SDK=$(xcrun --sdk iphoneos --show-sdk-path)
MIN_VER=$(xcrun --sdk iphoneos --show-sdk-version)

echo "SDK: $SDK"
echo "Min Version: $MIN_VER"

# 编译
clang \
  -arch arm64 \
  -isysroot "$SDK" \
  -iframework "$SDK/System/Library/Frameworks" \
  -iframework "$SDK/Library/Frameworks" \
  -I . \
  -O2 \
  -Wall \
  -framework JavaScriptCore \
  -dynamiclib \
  -o dxctx_gm.dylib \
  fishhook.c \
  Inject.jsb.c \
  -Wl,-install_name,@rpath/dxctx_gm.dylib \
  -mios-version-min=$MIN_VER

echo "=== Compilation Complete ==="
echo "Output: dxctx_gm.dylib"

# 验证
echo ""
echo "=== Verification ==="
file dxctx_gm.dylib
lipo -info dxctx_gm.dylib
nm -U dxctx_gm.dylib | grep "T " | head -20

echo ""
echo "=== Done ==="
