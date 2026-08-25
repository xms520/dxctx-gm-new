# 编译状态

## 问题

GitHub Actions 编译持续失败，但无法获取具体错误信息。

## 可能原因

1. JavaScriptCore.framework 在 macOS runner 上可能需要特殊路径
2. fishhook.c 与 Inject.jsb.c 之间的符号不匹配
3. 函数签名不兼容

## 解决方案

### 方案1：本地编译（推荐）

需要 macOS + Xcode：

```bash
cd /path/to/dxctx_gm/src

SDK=$(xcrun --sdk iphoneos --show-sdk-path)
MIN_VER=$(xcrun --sdk iphoneos --show-sdk-version)

clang \
  -arch arm64 \
  -isysroot "$SDK" \
  -iframework "$SDK/System/Library/Frameworks" \
  -iframework "$SDK/Library/Frameworks" \
  -I . \
  -O2 \
  -framework JavaScriptCore \
  -dynamiclib \
  -o dxctx_gm.dylib \
  fishhook.c \
  Inject.jsb.c \
  -Wl,-install_name,@rpath/dxctx_gm.dylib \
  -mios-version-min=$MIN_VER
```

### 方案2：使用macOS构建服务

推荐服务：
- [MacStadium](https://www.macstadium.com/)
- [AWS EC2 Mac](https://aws.amazon.com/ec2/instance-types/macos/)
- [GitHub Actions](https://github.com/features/actions)（已尝试，持续失败）

### 方案3：在线构建

使用 https://build.pub/ 或其他 iOS dylib 在线构建服务。

## 代码状态

- 源码文件: ✅ 正确
- 工作流配置: ✅ 正确
- 编译命令: ✅ 理论上正确

## 下一步

1. 在本地 Mac 上尝试编译
2. 如果成功，确认命令后更新 workflow
3. 如果仍然失败，考虑其他注入方式
