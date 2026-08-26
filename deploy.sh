#!/bin/bash
# Deploy script - copy dylib and JS to IPA documents folder
# Usage: ./deploy.sh <path/to/game.ipa>

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <path/to/game.ipa>"
    exit 1
fi

IPA="$1"
BUILD_DIR="build"

echo "=== DXCT GM Deploy ==="
echo ""

# Extract IPA
echo "Extracting IPA..."
UNZIP_DIR=$(mktemp -d)
unzip -q "$IPA" -d "$UNZIP_DIR"

# Find the app bundle
APP_BUNDLE=$(find "$UNZIP_DIR" -name "*.app" -type d | head -1)
if [ -z "$APP_BUNDLE" ]; then
    echo "ERROR: No .app bundle found in IPA"
    exit 1
fi

echo "App bundle: $APP_BUNDLE"

# Copy dylib to app bundle
echo "Copying dylib..."
mkdir -p "$APP_BUNDLE/Frameworks"
cp "$BUILD_DIR/dxctx_gm.dylib" "$APP_BUNDLE/Frameworks/"

# Copy JS file if exists
if [ -f "$BUILD_DIR/gm_template.js" ]; then
    cp "$BUILD_DIR/gm_template.js" "$APP_BUNDLE/"
    echo "JS file copied"
fi

echo ""
echo "Deploy complete!"
echo "App bundle: $APP_BUNDLE"
echo ""
echo "Next steps:"
echo "  1. Re-sign the IPA (TrollStore/AppSyncUnified)"
echo "  2. Inject dxctx_gm.dylib at runtime"
echo "  3. Set DXCT_ENABLE=1 in environment"
echo ""
echo "Temp dir: $UNZIP_DIR"
