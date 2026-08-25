# 编译指南 - dxctx_gm.dylib

## 方法一：GitHub Actions (推荐，无需本地环境)

### 步骤

1. **创建GitHub仓库**
   - 访问 https://github.com/new 创建新仓库
   - 名称建议: `dxctx-gm`

2. **上传代码**
   ```bash
   # 在电脑终端执行
   cd /var/minis/workspace/dxctx_gm
   git init
   git add .
   git commit -m "Initial commit"
   git remote add origin https://github.com/你的用户名/dxctx-gm.git
   git push -u origin main
   ```

   或者手动上传：
   - 进入仓库页面 → Actions → 提交文件
   - 上传以下文件:
     - `src/fishhook.c`
     - `src/fishhook.h`
     - `src/Inject.jsb.c`
     - `src/gm_template.js`
     - `.github/workflows/build.yml`

3. **触发编译**
   - 进入 Actions 标签页
   - 点击 "Build DXCT GM Hook" → "Run workflow"
   - 等待约2-3分钟

4. **下载dylib**
   - Actions完成后，点击运行记录
   - 在 "Artifacts" 部分下载 `dxctx_gm.zip`
   - 解压得到 `dxctx_gm.dylib`

---

## 方法二：本地macOS编译 (需要Xcode)

### 前置条件
- macOS 10.15+
- Xcode 12+
- Command Line Tools: `xcode-select --install`

### 步骤

1. **复制文件到Mac**
   ```bash
   # 通过AirDrop、iCloud或文件共享拷贝以下文件:
   /var/minis/workspace/dxctx_gm/src/fishhook.c
   /var/minis/workspace/dxctx_gm/src/fishhook.h
   /var/minis/workspace/dxctx_gm/src/Inject.jsb.c
   /var/minis/workspace/dxctx_gm/src/gm_template.js
   ```

2. **打开终端，编译**
   ```bash
   cd ~/Desktop  # 或文件所在目录
   ```

   复制以下命令执行:
   ```bash
   SDK=$(xcrun --sdk iphoneos --show-sdk-path)
   MIN_VER=$(xcrun --sdk iphoneos --show-sdk-version)
   
   clang \
     -arch arm64 \
     -isysroot "$SDK" \
     -iframework "$SDK/System/Library/Frameworks" \
     -iframework "$SDK/Library/Frameworks" \
     -I . \
     -O2 \
     -Wall \
     -framework JavaScriptCore \
     -dynamiclib \
     -o dxctx_gm.dylib \
     fishhook.c \
     Inject.jsb.c \
     -Wl,-install_name,@rpath/dxctx_gm.dylib \
     -mios-version-min=$MIN_VER
   ```

3. **验证编译结果**
   ```bash
   file dxctx_gm.dylib
   lipo -info dxctx_gm.dylib
   nm -U dxctx_gm.dylib | grep "T " | head -20
   ```

   预期输出:
   ```
   dxctx_gm.dylib: current ar archive (universal)
   archives:
     Mach-O 64-bit dynamically linked shared object arm64
     Mach-O 64-bit dynamically linked shared object arm64
   ```

---

## 方法三：使用在线macOS构建服务

如果本地没有Mac，可以使用:

1. **MacStadium** - 提供云端Mac机器
2. **AWS EC2 Mac** - 按小时租用
3. **GitHub Actions** - 免费，推荐

---

## 常见问题

### Q: 编译报错 "unsupported option '-arch'"
**A**: 确保使用Xcode的clang，而不是系统clang
```bash
xcrun clang -arch arm64 ...
```

### Q: JavaScriptCore.framework找不到
**A**: 添加正确的framework路径
```bash
-iframework "$SDK/System/Library/Frameworks"
```

### Q: 符号重绑定失败
**A**: fishhook需要在dylib加载前调用，确保构造函数正确执行

---

## 编译后部署

1. **注入dylib**
   - 使用全能签或TrollStore
   - 注入 `dxctx_gm.dylib`
   - 重签IPA

2. **部署GM脚本**
   ```bash
   # 将gm_template.js放入游戏沙盒
   cp gm_template.js /path/to/Game/Sandbox/Documents/dxctx_gm.js
   ```

3. **设置环境变量**
   ```bash
   # 在LaunchAgent或注入时设置
   export DXCT_ENABLE=1
   export DXCT_JS_FILE=/var/mobile/Documents/dxctx_gm.js
   ```

4. **启动游戏**
   - 查看日志: `log stream --predicate 'subsystem contains "DXCTGM"'`
   - 预期输出:
     ```
     [DXCTGM] DXCT GM Hook loading...
     [DXCTGM] JSEvaluateScript hook installed successfully!
     [DXCTGM] GM script injected successfully!
     ```
