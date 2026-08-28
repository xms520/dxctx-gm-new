//
//  gm_overlay.h - GM 调试面板 (大侠闯天下)
//  Native UI Overlay 接口声明
//  Overlay.m 提供实现, Inject.jsb.c 调用启动
//

#ifndef GM_OVERLAY_H
#define GM_OVERLAY_H

// 在 JS 上下文上执行一段 JS 表达式 (由 Inject.jsb.c 实现)
// 供 Overlay 按钮点击时调用 GM 对象的 JS 方法
// 如: dxct_run_js("GM.toggleOneHitKill()")
int dxct_run_js(const char *jsExpression);

// 返回当前 JS 环境是否已就绪 (JSContext 已捕获)
int dxct_js_ready(void);

// 在主线程创建并显示 GM 悬浮调试面板
// 应仅在 JSContext 已捕获、GM 对象注入完成后调用
void dxct_show_overlay(void);

// 在 JS 侧执行并返回 bool 值表达式的值 (如 GM 状态)
// 用于按钮状态同步; 返回 -1 表示未就绪
int dxct_eval_bool(const char *jsExpression);

#endif /* GM_OVERLAY_H */