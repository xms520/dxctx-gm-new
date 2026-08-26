#!/bin/bash
# Local build script for DXCT GM dylib
# Requires: macOS + Xcode Command Line Tools

set -e

echo "=== DXCT GM Builder ==="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check Xcode
if ! command -v xcrun &> /dev/null; then
    echo -e "${RED}ERROR: Xcode Command Line Tools not found${NC}"
    exit 1
fi

# Create build directory
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

echo -e "${GREEN}Building arm64 dylib...${NC}"

# Compile
echo "Compiling fishhook.c..."
clang -arch arm64 -framework Foundation -framework JavaScriptCore \
    -c src/fishhook.c -o "$BUILD_DIR/fishhook.o" \
    -I src -O2 -fPIC

echo "Compiling Inject.jsb.c..."
clang -arch arm64 -framework Foundation -framework JavaScriptCore \
    -c src/Inject.jsb.c -o "$BUILD_DIR/Inject.jsb.o" \
    -I src -O2 -fPIC \
    -DJS_EXPORT=

echo "Linking dylib..."
clang -arch arm64 -shared \
    "$BUILD_DIR/fishhook.o" "$BUILD_DIR/Inject.jsb.o" \
    -o "$BUILD_DIR/dxctx_gm.dylib" \
    -framework Foundation \
    -framework JavaScriptCore \
    -Wl,-install_name,@rpath/dxctx_gm.dylib

# Verify
echo ""
echo -e "${GREEN}Build complete!${NC}"
file "$BUILD_DIR/dxctx_gm.dylib"
ls -lh "$BUILD_DIR/dxctx_gm.dylib"

echo ""
echo "Usage:"
echo "  1. Inject dxctx_gm.dylib into the game IPA"
echo "  2. Set environment variable: DXCT_ENABLE=1"
echo "  3. Optional: DXCT_JS_FILE=/path/to/gm_template.js"
echo ""
echo "Log: grep DXCTGM system.log or check Documents/dxct_gm.log"
