# Build Guide

## GitHub Actions 编译

推送代码到 GitHub 后，Actions 会自动编译 arm64 dylib。

### 步骤

1. 创建 GitHub 仓库
2. 推送代码
3. Actions 编译完成后下载 artifact
4. 使用全能签注入 dylib

### 编译命令

```yaml
clang -arch arm64 -framework Foundation -framework JavaScriptCore \
    -c src/Inject.jsb.c -o build/Inject.jsb.o \
    -I src -O2 -fPIC

clang -arch arm64 -shared \
    build/fishhook.o build/Inject.jsb.o \
    -o build/dxctx_gm.dylib \
    -framework Foundation \
    -framework JavaScriptCore
```

## 本地编译 (macOS)

```bash
./build.sh
```

## 部署到游戏

1. 解压 IPA
2. 复制 `dxctx_gm.dylib` 到 App 的 Frameworks 目录
3. 重签 IPA
4. 运行游戏
5. 设置环境变量 `DXCT_ENABLE=1`

## 使用 GM

启动游戏后，按 F1-F9 使用 GM 功能。

## 日志

```bash
# 系统日志
log show --predicate 'eventMessage contains "DXCTGM"' --last 1h

# 应用日志
cat /var/mobile/Containers/Data/Application/*/Documents/dxct_gm.log
```
