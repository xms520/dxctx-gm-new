# 大侠闯天下 GM注入 - 诊断与部署指南

## 问题：注入无效没有任何反应

### 可能原因分析

1. **dylib 未加载** - 环境变量 `DXCT_ENABLE=1` 未设置
2. **fishhook 失败** - 符号重绑定未成功
3. **JSEvaluateScript 未被调用** - 游戏可能使用其他 JS 执行函数
4. **GM 脚本执行失败** - 语法错误或 API 不兼容

---

## 诊断步骤

### 步骤1: 确认 dylib 已加载

查看系统日志：
```bash
log stream --predicate 'eventMessage contains "DXCT"' --style compact
```

或查看文件日志：
```bash
cat /var/mobile/Library/Logs/dxct_gm.log
```

**预期输出：**
```
[DXCT] ========================================
[DXCT] DXCT GM Hook v1.1 initializing...
[DXCT] ========================================
[DXCT] DXCT_JS_FILE=(using default)
[DXCT] fishhook installed successfully!
[DXCT] JSEvaluateScript original: 0x...
```

**如果没有输出：**
- 检查环境变量 `DXCT_ENABLE=1` 是否正确设置
- 确认 dylib 已正确注入并重签

---

### 步骤2: 确认 JSEvaluateScript 被调用

在日志中搜索：
```
[DXCT] JSContext captured via JSEvaluateScript
```

**如果没有找到：**
- 游戏可能不使用 `JSEvaluateScript`
- 需要尝试其他注入点（见下文）

---

### 步骤3: 运行诊断脚本

将 `diagnose.js` 部署到游戏沙盒，然后手动执行：

```bash
# 1. 将诊断脚本放入沙盒
cp diagnose.js /path/to/Game/Sandbox/Documents/diagnose.js

# 2. 设置环境变量并启动游戏
export DXCT_ENABLE=1
export DXCT_JS_FILE=/var/mobile/Documents/diagnose.js

# 3. 启动游戏
```

查看输出日志 `/var/mobile/Library/Logs/dxct_gm.log` 和 `/var/mobile/Library/Logs/dxct_diag.json`

---

## 替代注入方案

如果 `JSEvaluateScript` 没有被调用，尝试以下方案：

### 方案A: Hook JSContextGroupCreate

Cocos2d-x 可能在创建 JSContextGroup 时初始化上下文。

```c
// 在 Inject.jsb.c 中添加
static JSContextGroupRef (*orig_JSContextGroupCreate)(const JSContextObserver *);

static JSContextGroupRef my_JSContextGroupCreate(const JSContextObserver *observer) {
    JSContextGroupRef group = orig_JSContextGroupCreate(observer);
    dxct_log("[DXCT] JSContextGroup created\n");
    return group;
}
```

### 方案B: Hook JSGlobalContextCreate

```c
static JSContextRef (*origJSGlobalContextCreate)(JSContextGroupRef);

static JSContextRef myJSGlobalContextCreate(JSContextGroupRef group) {
    JSContextRef ctx = origJSGlobalContextCreate(group);
    if (ctx && !gContextCaptured) {
        gContext = ctx;
        gContextCaptured = 1;
        dxct_log("[DXCT] JSGlobalContext created: %p\n", ctx);
        // 注入 GM
    }
    return ctx;
}
```

### 方案C: 直接搜索 JavaScriptCore 符号

使用 `dlsym` 动态查找并使用函数指针：

```c
// 不使用 fishhook，直接调用原始函数
typedef JSContextRef (*JSEvalFunc)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef*);

void *handle = dlopen("/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore", RTLD_LAZY);
JSEvalFunc orig = (JSEvalFunc)dlsym(handle, "JSEvaluateScript");
```

---

## 部署清单

### 必需文件
- [ ] `dxctx_gm.dylib` - 注入库
- [ ] `gm_template.js` - GM脚本
- [ ] `diagnose.js` - 诊断脚本

### 部署位置
```
# dylib - 使用全能签/TrollStore注入
# GM脚本 - 游戏沙盒
/var/mobile/Library/MobileDevice/Provisioning Profiles/  (或沙盒Documents)
```

### 环境变量
```bash
export DXCT_ENABLE=1
export DXCT_JS_FILE=/var/mobile/Documents/dxct_gm.js
```

---

## 常见问题

### Q: 日志中没有 `[DXCT] fishhook installed`
**A:** fishhook 可能失败了。检查：
1. dylib 是否正确加载
2. 符号是否在 GOT 中

### Q: 日志中有 `JSContext captured` 但没有 `GM script injected`
**A:** GM脚本文件可能不存在或路径错误。检查：
1. `DXCT_JS_FILE` 环境变量
2. 文件权限

### Q: 日志中有 `Script error`
**A:** GM脚本有语法错误。检查：
1. JS语法是否正确
2. 是否使用了不支持的API

### Q: 注入成功但GM功能无效
**A:** 游戏可能使用加密脚本，运行时对象结构不同。需要：
1. 运行诊断脚本获取运行时对象
2. 根据实际对象结构调整GM脚本

---

## 下一步

1. **运行诊断** - 使用 `diagnose.js` 获取游戏JS环境信息
2. **分析日志** - 查看 `/var/mobile/Library/Logs/dxct_gm.log`
3. **调整注入点** - 如果 JSEvaluateScript 未被调用，尝试其他方案
4. **调试GM脚本** - 根据实际游戏对象调整GM功能
