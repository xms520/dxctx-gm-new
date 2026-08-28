# 大侠闯天下 GM Hook v3.0 (秒杀+无敌+悬浮调试面板)

基于 JavaScriptCore JSB 注入的 Cocos2d-x GM 工具，带 **原生 UIKit 悬浮调试面板**。
注入后游戏屏幕上出现可拖动的半透明面板，点按钮即可实时开关功能。

## 🎮 GM 悬浮面板功能

| 按钮 | 功能 | 说明 |
|------|------|------|
| 🗡️ 秒杀 | One-Hit Kill | 开关，开启后秒杀所有敌人（hook 伤害函数） |
| 🛡️ 无敌 | God Mode | 开关，开启后免疫伤害（hook 受伤函数） |
| ❤️ 满血恢复 | Full Heal | 玩家 HP 立即回满 |
| ⚡ 无限攻击 | Infinite Attack | 开关，攻击力设为 99999 |
| 🌀 快速攻击 | Fast Attack | 攻速加速到 0.05 |
| 🔍 场景调试 | Dump Objects | 打印场景节点信息 |

- 面板为**半透明悬浮窗**，可拖动位置
- 点击**标题栏**可展开/收起面板
- 开关型按钮（秒杀/无敌/攻击）点击后**变绿+ON ✅**表示已开启

## 快速开始

### 云编译（GitHub Actions）
```bash
# 推送代码到 GitHub，Actions 自动交叉编译 dylib
git push origin main
```
构建产物 `dxctx_gm.dylib` 在 Actions 的 artifact 中下载。

### 本地编译（需 macOS + Xcode）
```bash
./build.sh
```

## 注入方式
1. 下载 `dxctx_gm.dylib`（arm64, iOS 15.0+）
2. 使用 **TrollStore / 全能签** 注入到已安装的「大侠闯天下」游戏
3. **无需设置环境变量**（v3.1 默认开启）—— dylib 加载即自动注入并显示 GM 面板
4. 进入游戏后屏幕左上角出现 GM 面板

> 若需禁用: 设置 `DXCT_ENABLE=0` 环境变量则跳过注入

## 环境变量
```bash
export DXCT_ENABLE=1                    # 启用注入（必须）
export DXCT_JS_FILE=/path/to/gm.js      # 自定义GM脚本（可选）
```

## 文件结构
```
dxctx_gm/
├── src/
│   ├── Inject.jsb.c      # 主注入逻辑 + JS 桥接
│   ├── Overlay.m         # 原生 UIKit 悬浮调试面板
│   ├── gm_overlay.h      # 面板桥接接口
│   ├── gm_script.js      # GM 秒杀/无敌核心逻辑 (JS)
│   ├── fishhook.c        # fishhook 符号重绑定
│   └── ...
├── build.sh              # 编译脚本
├── Makefile              # 构建
└── .github/workflows/
    └── build.yml         # GitHub Actions 云编译
```

## 日志
- 文件日志: `/var/mobile/Library/Logs/dxct_gm.log`

v3.0 (GM Panel) — 2026-08-28