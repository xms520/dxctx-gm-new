# Makefile - dxctx_gm (带 GM 调试面板) for iOS arm64
# Build: make SDK_PATH=<xcode sdk path>
# Or use GitHub Actions

SDK_PATH ?= $(shell xcrun --sdk iphoneos --show-sdk-path 2>/dev/null)
CLANG    ?= clang

ARCH     = arm64
CFLAGS   = -arch $(ARCH) -isysroot "$(SDK_PATH)" \
           -iframework "$(SDK_PATH)/System/Library/Frameworks" \
           -iframework "$(SDK_PATH)/Library/Frameworks" \
           -Isrc -O2 -mios-version-min=15.0 -Wall

# Objective-C 需要 -fobjc-arc 与 -ObjC, 链接 UIKit/Foundation
OBJC_FLAGS = -fobjc-arc -ObjC

LDFLAGS  = -framework JavaScriptCore \
           -framework UIKit \
           -framework Foundation \
           -framework QuartzCore \
           -dynamiclib -Wl,-install_name,@rpath/dxctx_gm.dylib

SRCS_C   = src/Inject.jsb.c src/fishhook.c
SRCS_M   = src/Overlay.m
OUT      = dxctx_gm.dylib

.PHONY: all clean

all: $(OUT)

$(OUT): $(SRCS_C) $(SRCS_M)
	@echo "=== Building dxctx_gm.dylib (v3.0 GM Panel) ==="
	$(CLANG) $(CFLAGS) $(OBJC_FLAGS) $(LDFLAGS) -o $@ $(SRCS_C) $(SRCS_M)
	@echo "=== OK ==="
	@file $@ && lipo -info $@

clean:
	rm -f $(OUT)