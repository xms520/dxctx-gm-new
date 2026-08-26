# 大侠闯天下 GM - 快速诊断流程

## 问题：注入无效没有任何反应

请按以下步骤排查：

---

## 步骤1：确认 dylib 是否加载

**方法A：查看系统日志**
```bash
log stream --predicate 'eventMessage contains "DXCT"' --style compact
```

**方法B：查看文件日志**
```bash
cat /var/mobile/Library/Logs/dxct_gm.log
```

**预期输出：**
```
[DXCT] ========================================
[DXCT] DXCT GM Hook v1.1 initializing...
[DXCT] ========================================
[DXCT] fishhook installed successfully!
```

**如果没有输出：**
- 环境变量 `DXCT_ENABLE=1` 未设置
- 或 dylib 未正确注入

---

## 步骤2：运行诊断脚本

将以下脚本部署到游戏沙盒：

```bash
# 1. 复制诊断脚本
cp /var/minis/workspace/dxctx_gm/src/test_inject.js \
   /path/to/game/sandbox/Documents/test_inject.js

# 2. 设置环境变量
export DXCT_ENABLE=1
export DXCT_JS_FILE=/var/mobile/Documents/test_inject.js

# 3. 启动游戏
```

**查看输出：**
```bash
cat /var/mobile/Library/Logs/dxct_gm.log
```

---

## 步骤3：分析结果

### 情况A：日志中有 `[DXCT] fishhook installed` 但没有 `JSContext captured`

**原因：** 游戏可能不使用 `JSEvaluateScript` 执行脚本

**解决方案：** 尝试以下hook点（需要修改代码）：
- `JSContextGroupCreate`
- `JSGlobalContextCreate`
- `JSRunScript`

### 情况B：日志中有 `JSContext captured` 但没有 `GM script injected`

**原因：** GM脚本文件路径错误或不存在

**解决方案：**
1. 检查 `DXCT_JS_FILE` 环境变量
2. 确认文件存在于游戏沙盒
3. 检查文件权限

### 情况C：日志中有 `Script error`

**原因：** GM脚本有语法错误

**解决方案：**
1. 检查JS语法
2. 确认使用了正确的API
3. 查看错误详情

### 情况D：注入成功但功能无效

**原因：** 游戏使用加密脚本，运行时对象结构与预期不同

**解决方案：**
1. 运行完整诊断获取运行时对象
2. 根据实际结构调整GM脚本
3. 可能需要hook解密后的执行点

---

## 紧急诊断命令

```bash
# 实时查看日志
log stream --predicate 'eventMessage contains "DXCT"' --style compact --follow

# 查看游戏沙盒
ls -la /var/mobile/Containers/Data/Application/*/Documents/

# 检查环境变量
printenv | grep DXCT
```

---

## 下一步

如果以上步骤仍无法解决问题，请提供：
1. 完整的 `dxct_gm.log` 内容
2. 游戏版本信息
3. 注入方式（全能签/TrollStore/其他）
