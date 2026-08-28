// Inject.jsb.c - GM hook for 大侠闯天下 (Cocos2d-x JSB)
// v3.5: 使用 ObjC Method Swizzling Hook JSContext.evaluateScript:
// 环境变量: DXCT_ENABLE=1 (启用), DXCT_JS_FILE=/path/to/gm.js

#include <JavaScriptCore/JavaScriptCore.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#import <objc/runtime.h>
#import <JavaScriptCore/JSStringRef.h>
#include "gm_overlay.h"

#define LOG_TAG "[DXCT-GM]"
#define GM_SCRIPT_ENV "DXCT_JS_FILE"

static FILE *gLog = NULL;

static void dxct_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (!gLog) gLog = fopen("/var/mobile/Library/Logs/dxct_gm.log", "a");
    if (gLog) {
        fprintf(gLog, "%s ", LOG_TAG);
        vfprintf(gLog, fmt, args);
        fprintf(gLog, "\n");
        fflush(gLog);
    }
    va_end(args);
}

// GM 状态标志
static volatile int gFlagOneHitKill = 0;
static volatile int gFlagGodMode = 0;
void dxct_set_one_hit_kill(int v) { gFlagOneHitKill = v; }
int dxct_get_one_hit_kill(void) { return gFlagOneHitKill; }
void dxct_set_god_mode(int v) { gFlagGodMode = v; }
int dxct_get_god_mode(void) { return gFlagGodMode; }

// JS 上下文缓存
static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;
static JSContextRef gCachedContext = NULL;
static volatile int gInjected = 0;

// 原始 C API 指针（备用）
static JSValueRef (*orig_C_JSEvaluateScript)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSValueRef *) = NULL;

// ===== GM 脚本常量（防止优化器移除）=====
static const char *gm_script_text __attribute__((used)) =
    "window.GM = window.GM || {};\n"
    "(function() {\n"
    "  'use strict';\n"
    "  var GM = window.GM;\n"
    "  GM.oneHitKill = false;\n"
    "  GM.godMode = false;\n"
    "  GM._active = true;\n"
    "  GM.toggleOneHitKill = function() { GM.oneHitKill = !GM.oneHitKill; console.log('[GM] 秒杀 ' + GM.oneHitKill); return GM.oneHitKill; };\n"
    "  GM.toggleGodMode = function() { GM.godMode = !GM.godMode; console.log('[GM] 无敌 ' + GM.godMode); return GM.godMode; };\n"
    "  GM.fullHeal = function() { try { var p = GM.findPlayer(); if(p) { if(p.setHealth) p.setHealth(999999); else if(p.hp!==undefined) p.hp=999999; } } catch(e){} };\n"
    "  GM.maxAttack = function(v) { console.log('[GM] attack=' + v); };\n"
    "  GM.maxSpeed = function(v) { console.log('[GM] speed=' + v); };\n"
    "  window.GM = GM;\n"
    "  console.log('[GM] 大侠闯天下 GM v3.5 已加载');\n"
    "})();\n";

// ===== ObjC JSContext 原始方法指针 =====
static JSValueRef (*orig_evaluateScriptIMP)(id, SEL, JSStringRef) = NULL;

// ===== 注入逻辑（在 Hook 之前调用）=====
static void dxct_inject_gm_js(JSContextRef ctx) {
    if (!ctx || !gCachedContext) return;

    dxct_log("[DXCT] Injecting GM script into context...");

    JSStringRef script = JSStringCreateWithUTF8CString(gm_script_text);
    if (script) {
        JSValueRef exception = NULL;
        JSEvaluateScript(ctx, script, NULL, NULL, 0, &exception);
        if (exception) {
            dxct_log("[DXCT] GM script error");
        }
        JSStringRelease(script);
    }

    gInjected = 1;
    dxct_log("[DXCT] GM injected");
    dxct_show_overlay();
}

// ===== 桥接函数: 供 Overlay.m 调用 =====

int dxct_run_js(const char *jsExpression) {
    if (!jsExpression || !gCachedContext || !gInjected) {
        return 0;
    }
    JSContextRef ctx = gCachedContext;
    JSStringRef s = JSStringCreateWithUTF8CString(jsExpression);
    if (!s) return 0;
    JSValueRef exc = NULL;
    JSValueRef v = JSEvaluateScript(ctx, s, NULL, NULL, 0, &exc);
    if (exc) dxct_log("run_js error: %s", jsExpression);
    JSStringRelease(s);
    return (v != NULL) ? 1 : 0;
}

int dxct_js_ready(void) {
    return (gCachedContext != NULL && gInjected) ? 1 : 0;
}

int dxct_eval_bool(const char *jsExpression) {
    if (!jsExpression || !gCachedContext || !gInjected) return -1;
    JSContextRef ctx = gCachedContext;
    JSStringRef s = JSStringCreateWithUTF8CString(jsExpression);
    if (!s) return -1;
    JSValueRef exc = NULL;
    JSValueRef v = JSEvaluateScript(ctx, s, NULL, NULL, 0, &exc);
    int r = -1;
    if (v && !exc && JSValueIsBoolean(ctx, v)) {
        r = (int)JSValueToBoolean(ctx, v);
    }
    if (exc) dxct_log("eval_bool error");
    JSStringRelease(s);
    return r;
}

// ===== ObjC Method Swizzle 实现 =====

// 传统 C 函数实现
static JSValueRef dxct_evaluateScript_impl(id self, SEL _cmd, JSStringRef script) {
    // 获取 JSContextRef
    JSContextRef ctx = (__bridge JSContextRef)self;

    // 第一次调用时捕获上下文并注入 GM 脚本
    if (!gCachedContext && ctx) {
        pthread_mutex_lock(&gLock);
        if (!gCachedContext) {
            gCachedContext = ctx;
            dxct_log("[DXCT] JSContext captured via ObjC evaluateScript:");
            dxct_inject_gm_js(ctx);
        }
        pthread_mutex_unlock(&gLock);
    }

    // 调用原始实现
    if (orig_evaluateScriptIMP) {
        return orig_evaluateScriptIMP(self, _cmd, script);
    }

    return NULL;
}

// ===== 构造函数：初始化 =====

__attribute__((constructor))
void dylib_init() {
    char *disable = getenv("DXCT_ENABLE");
    if (disable && strcmp(disable, "0") == 0) {
        dxct_log("[DXCT] DXCT_ENABLE=0, skipping");
        return;
    }

    dxct_log("[DXCT] dylib loaded, initializing...");

    // 先显示悬浮窗（诊断用）
    dxct_show_overlay();

    // 尝试 ObjC JSContext Method Swizzle
    Class ctxClass = objc_getClass("JSContext");
    if (ctxClass) {
        SEL evaluateScriptSel = sel_registerName("evaluateScript:");
        Method m = class_getInstanceMethod(ctxClass, evaluateScriptSel);
        if (m) {
            // 保存原始实现
            orig_evaluateScriptIMP = (JSValueRef (*)(id, SEL, JSStringRef))method_getImplementation(m);
            dxct_log("[DXCT] Original ObjC evaluateScriptIMP: %p", orig_evaluateScriptIMP);

            // 替换为新实现（使用传统 C 函数指针）
            method_setImplementation(m, (IMP)dxct_evaluateScript_impl);
            dxct_log("[DXCT] ObjC evaluateScript: swizzled successfully");
        } else {
            dxct_log("[DXCT] evaluateScript: method not found on JSContext");
        }
    } else {
        dxct_log("[DXCT] JSContext class not found");
    }

    // 备用：Fishhook C API
    void *jc = dlopen("/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
    if (jc) {
        orig_C_JSEvaluateScript = (JSValueRef (*)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSValueRef *))
            dlsym(jc, "JSEvaluateScript");
        if (orig_C_JSEvaluateScript) {
            dxct_log("[DXCT] Found C JSEvaluateScript at %p", orig_C_JSEvaluateScript);
        }
        dlclose(jc);
    }
}

__attribute__((destructor))
void dylib_fini() {
    if (gLog) fclose(gLog);
}
