# Makefile - dxctx_gm for iOS arm64
# Build with: make SDK_PATH=<path>
# Or use GitHub Actions

SDK_PATH     ?= $(shell xcrun --sdk iphoneos --show-sdk-path 2>/dev/null)
SDK_VERSION  ?= $(shell xcrun --sdk iphoneos --show-sdk-version 2>/dev/null)
CLANG        ?= $(shell which clang 2>/dev/null || xcrun --sdk iphoneos find clang 2>/dev/null)

ARCH     = arm64
CFLAGS   = -arch $(ARCH) \
           -isysroot "$(SDK_PATH)" \
           -iframework "$(SDK_PATH)/System/Library/Frameworks" \
           -iframework "$(SDK_PATH)/Library/Frameworks" \
           -Isrc \
           -O2 \
           -mios-version-min=15.0 \
           -fobjc-arc \
           -Wall

LDFLAGS  = -framework JavaScriptCore \
           -framework UIKit \
           -framework Foundation \
           -dynamiclib \
           -Wl,-install_name,@rpath/dxctx_gm.dylib

SRCS     = src/Inject.jsb.c src/Overlay.m
OUT      = dxctx_gm.dylib

.PHONY: all clean

all: $(OUT)

$(OUT): $(SRCS)
	@echo "=== Building dxctx_gm.dylib ==="
	@echo "SDK: $(SDK_PATH) (v$(SDK_VERSION))"
	@echo "Arch: $(ARCH)"
	$(CLANG) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS)
	@echo "=== Build OK ==="
	@file $@
	@lipo -info $@ 2>/dev/null || true
	@ls -lh $@
	@echo "=== Symbols ==="
	@nm -U $@ 2>/dev/null | grep " T " | head -20

clean:
	rm -f $(OUT)
	@echo "Cleaned."
