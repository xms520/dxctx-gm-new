# 大侠闯天下 GM Hook

基于 fishhook 的 Cocos2d-x JSB GM注入工具

## 快速开始

### 编译

```bash
# macOS本地编译
./build.sh

# 或使用 GitHub Actions
# 推送代码到 GitHub，Actions 会自动编译
```

### 部署

```bash
# 查看部署指南
./deploy.sh deploy

# 运行诊断
./deploy.sh diagnose
```

### 环境变量

```bash
export DXCT_ENABLE=1                    # 启用注入
export DXCT_JS_FILE=/path/to/gm.js      # 自定义GM脚本（可选）
```

## 文件结构

```
dxctx_gm/
├── src/
│   ├── Inject.jsb.c      # 主注入逻辑
│   ├── fishhook.c        # fishhook 符号重绑定
│   ├── fishhook.h        # fishhook 头文件
│   ├── gm_template.js    # GM调试面板
│   └── diagnose.js       # 环境诊断脚本
├── build.sh              # 编译脚本
├── deploy.sh             # 部署脚本
├── DIAGNOSIS.md          # 诊断指南
└── .github/workflows/
    └── build.yml         # GitHub Actions
```

## GM功能

- F1: 一刀秒杀
- F2: 无敌模式
- F3: 无限血量
- F4: 速度加速
- F5: 一键通关
- F6: 战斗秒杀
- F7: 跳过剧情
- F8: 玩家信息Dump
- F9: 全局对象Dump

## 日志

- 系统日志: `log stream --predicate 'eventMessage contains "DXCT"'`
- 文件日志: `/var/mobile/Library/Logs/dxct_gm.log`
- 诊断日志: `/var/mobile/Library/Logs/dxct_diag.json`

## 问题排查

参见 [DIAGNOSIS.md](DIAGNOSIS.md)
Build fix Wed Aug 26 10:31:48 LCL 2026
