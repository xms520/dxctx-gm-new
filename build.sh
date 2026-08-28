#!/bin/bash
# 编译脚本 - dxctx_gm.dylib v3 (GM 调试面板)
# 需要 macOS + Xcode
set -e

echo "=== DXCT GM Hook Compiler (v3 Panel) ==="

SDK=$(xcrun --sdk iphoneos --show-sdk-path)
MIN_VER=$(xcrun --sdk iphoneos --show-sdk-version)

echo "SDK: $SDK"
echo "Min Version: $MIN_VER"

clang \
  -arch arm64 \
  -isysroot "$SDK" \
  -iframework "$SDK/System/Library/Frameworks" \
  -iframework "$SDK/Library/Frameworks" \
  -I . \
  -O2 \
  -Wall \
  -fobjc-arc \
  -ObjC \
  -framework JavaScriptCore \
  -framework UIKit \
  -framework Foundation \
  -framework QuartzCore \
  -dynamiclib \
  -o dxctx_gm.dylib \
  src/Inject.jsb.c \
  src/Overlay.m \
  -Wl,-install_name,@rpath/dxctx_gm.dylib \
  -mios-version-min=$MIN_VER

echo "=== Compilation Complete ==="
echo "Output: dxctx_gm.dylib"

echo ""
echo "=== Verification ==="
file dxctx_gm.dylib
lipo -info dxctx_gm.dylib
nm -U dxctx_gm.dylib | grep "T " | head -20

echo ""
echo "=== Done ==="