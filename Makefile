# Makefile - dxctx_gm for iOS arm64
# Build: make SDK_PATH=<xcode sdk path>
# Or use GitHub Actions

SDK_PATH ?= $(shell xcrun --sdk iphoneos --show-sdk-path 2>/dev/null)
CLANG    ?= clang

ARCH     = arm64
CFLAGS   = -arch $(ARCH) -isysroot "$(SDK_PATH)" \
           -iframework "$(SDK_PATH)/System/Library/Frameworks" \
           -iframework "$(SDK_PATH)/Library/Frameworks" \
           -Isrc -O2 -mios-version-min=15.0 -Wall

LDFLAGS  = -framework JavaScriptCore \
           -dynamiclib -Wl,-install_name,@rpath/dxctx_gm.dylib

SRCS     = src/Inject.jsb.c
OUT      = dxctx_gm.dylib

.PHONY: all clean

all: $(OUT)

$(OUT): $(SRCS)
	@echo "=== Building dxctx_gm.dylib ==="
	$(CLANG) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS)
	@echo "=== OK ==="
	@file $@ && lipo -info $@

clean:
	rm -f $(OUT)
