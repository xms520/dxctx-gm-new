# dxctx_gm 编译问题排查

## 问题
GitHub Actions 编译失败: `Error: Process completed with exit code 1`

## 可能原因
1. fishhook.c 实现不完整
2. JavaScriptCore.framework 路径问题
3. 符号重绑定签名不匹配

## 解决方案

### 方案1: 检查Actions日志
访问: https://github.com/xms520/dxctx-gm/actions

### 方案2: 本地编译
如果你有Mac和Xcode:
```bash
# 在Mac上执行
SDK=$(xcrun --sdk iphoneos --show-sdk-path)
clang \
  -arch arm64 \
  -isysroot "$SDK" \
  -iframework "$SDK/System/Library/Frameworks" \
  -I src \
  -O2 \
  -framework JavaScriptCore \
  -dynamiclib \
  -o dxctx_gm.dylib \
  src/fishhook.c \
  src/Inject.jsb.c
```

### 方案3: 使用替代注入方式
如果fishhook编译失败，可以:
1. 使用 CydiaSubstrate 替代 fishhook
2. 直接修改二进制文件
3. 使用 Frida 动态注入

## 当前状态
- 代码已推送: https://github.com/xms520/dxctx-gm
- Actions 运行中: https://github.com/xms520/dxctx-gm/actions
