# 大侠闯天下 GM 调试器 (DXCT GM Debugger)

基于 fishhook 的 Cocos2d-x JSB 注入方案，实现 GM 调试功能。

## 功能

| 快捷键 | 功能 |
|--------|------|
| F1 | 一刀秒杀 |
| F2 | 无敌模式 |
| F3 | 无限血量 |
| F4 | 设置血量/攻击 |
| F5 | 加速移动 |
| F6 | 传送 |
| F7 | 添加物品 |
| F8 | 通关 |
| F9 | 显示/隐藏面板 |

## 环境变量

```bash
DXCT_ENABLE=1           # 启用 GM (默认关闭)
DXCT_JS_FILE=/path      # 自定义 JS 文件路径
DXCT_LOG_LEVEL=verbose  # 详细日志
```

## 日志

- 系统日志：`grep DXCTGM system.log`
- 沙盒日志：`Documents/dxct_gm.log`

## 编译

### GitHub Actions (推荐)

1. 推送到 GitHub 仓库
2. Actions 自动编译 arm64 dylib
3. 下载 artifact

### 本地编译 (需 macOS + Xcode)

```bash
./build.sh
```

## 部署

1. 用全能签/TrollStore 注入 dylib
2. 重签 IPA
3. 运行游戏，设置 `DXCT_ENABLE=1`

## 架构

- Cocos2d-x JSB + JavaScriptCore
- fishhook hook `JSEvaluateScript`
- 首次执行时注入 GM 面板 JS

## 文件结构

```
dxctx_gm/
├── src/
│   ├── fishhook.c/h     # fishhook 符号重绑定
│   ├── Inject.jsb.c     # 主注入逻辑
│   └── gm_template.js   # GM 面板 UI (JS)
├── .github/workflows/
│   └── build.yml        # GitHub Actions 编译
├── build.sh             # 本地编译脚本
├── deploy.sh            # 部署脚本
└── README.md
```

## 注意事项

- 游戏 JS 脚本加密，GM 通过 hook JSEvaluateScript 在运行时注入
- 需要全能签/TrollStore 权限注入 dylib
- 日志搜索 `[DXCTGM]` 前缀

## 仓库

https://github.com/xms520/dxctx-gm-new
