# Build Status
- Local build: ❌ (requires macOS + Xcode)
- GitHub Actions: ✅ (automated)

# Files
- `src/fishhook.c` - Facebook fishhook implementation
- `src/fishhook.h` - fishhook header
- `src/Inject.jsb.c` - Main injection logic
- `src/gm_template.js` - GM debug panel JavaScript
- `build.sh` - Local build script
- `.github/workflows/build.yml` - GitHub Actions workflow

# Build Commands

## Local (macOS)
```bash
./build.sh
```

## GitHub Actions
Push to repository, Actions will auto-build.

## Manual Build
```bash
SDK=$(xcrun --sdk iphoneos --show-sdk-path)
MIN_VER=$(xcrun --sdk iphoneos --show-sdk-version)

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
```

# Usage
1. Inject `dxctx_gm.dylib` into game using 全能签/TrollStore
2. Place `gm_template.js` in game's Documents folder
3. Set `DXCT_ENABLE=1` environment variable
4. Launch game, check logs for `[DXCTGM]` prefix

# GM Features
| Key | Function |
|-----|----------|
| F1 | One Hit Kill |
| F2 | God Mode |
| F3 | Infinite HP |
| F4 | Speed Hack |
| F5 | Auto Win |
| F6 | Battle One Hit |
| F7 | Skip Story |
| F8 | Dump Player |
| F9 | Dump Global |
