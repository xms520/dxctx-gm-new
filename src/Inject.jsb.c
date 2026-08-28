// Inject.jsb.c - GM hook for 大侠闯天下 (Cocos2d-x JSB)
// 环境变量: DXCT_ENABLE=1 (启用), DXCT_JS_FILE=/path/to/gm.js
// v2.7: 修复所有编译错误，简化代码

#include <JavaScriptCore/JavaScriptCore.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "gm_overlay.h"
#include "fishhook.h"
#include <JavaScriptCore/JSStringRef.h>

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

// GM脚本注入标志
static volatile int gFlagOneHitKill = 0;
static volatile int gFlagGodMode = 0;
void dxct_set_one_hit_kill(int v) { gFlagOneHitKill = v; }
int dxct_get_one_hit_kill(void) { return gFlagOneHitKill; }
void dxct_set_god_mode(int v) { gFlagGodMode = v; }
int dxct_get_god_mode(void) { return gFlagGodMode; }

// JSEvaluateScript hook
static JSValueRef (*orig_JSEvaluateScript)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSValueRef *);
static JSContextRef gCachedContext = NULL;
static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;
static volatile int gInjected = 0;

// ===== 桥接: 供 Overlay.m 调用, 在缓存的 JSContext 上执行 JS =====
// 返回 1 表示已执行, 0 表示 JS 上下文未就绪
int dxct_run_js(const char *jsExpression) {
    if (!jsExpression || !gCachedContext || !gInjected) {
        return 0;
    }
    JSContextRef ctx = gCachedContext;
    JSStringRef s = JSStringCreateWithUTF8CString(jsExpression);
    if (!s) return 0;
    JSValueRef exc = NULL;
    orig_JSEvaluateScript(ctx, s, NULL, NULL, 0, &exc);
    if (exc) dxct_log("run_js error: %s", jsExpression);
    JSStringRelease(s);
    return 1;
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
    JSValueRef v = orig_JSEvaluateScript(ctx, s, NULL, NULL, 0, &exc);
    int r = -1;
    if (v && !exc && JSValueIsBoolean(ctx, v)) {
        r = (int)JSValueToBoolean(ctx, v);
    }
    if (exc) dxct_log("eval_bool error");
    JSStringRelease(s);
    return r;
}
// ===== 桥接结束 =====

// GM脚本 - 使用属性防止优化器移除
static const char *gm_script_parts[] __attribute__((used)) = {
    "window.GM = window.GM || {};\n(function() {\n",
    "  'use strict';\n",
    "  var GM = window.GM;\n",
    "  GM.oneHitKill = false;\n",
    "  GM.godMode = false;\n",
    "  GM._active = false;\n",
    "\n",
    "  GM.toggleOneHitKill = function() {\n",
    "    GM.oneHitKill = !GM.oneHitKill;\n",
    "    console.log('[GM] 秒杀 ' + (GM.oneHitKill ? 'ON' : 'OFF'));\n",
    "    return GM.oneHitKill;\n",
    "  };\n",
    "\n",
    "  GM.toggleGodMode = function() {\n",
    "    GM.godMode = !GM.godMode;\n",
    "    console.log('[GM] 无敌 ' + (GM.godMode ? 'ON' : 'OFF'));\n",
    "    return GM.godMode;\n",
    "  };\n",
    "\n",
    "  GM.fullHeal = function() {\n",
    "    try {\n",
    "      var player = GM.findPlayer();\n",
    "      if (player) {\n",
    "        if (player.setHealth) player.setHealth(999999);\n",
    "        else if (player.hp !== undefined) player.hp = 999999;\n",
    "        console.log('[GM] 满血恢复');\n",
    "      }\n",
    "    } catch(e) { console.log('[GM] fullHeal error: ' + e); }\n",
    "  };\n",
    "\n",
    "  GM.instantKillAll = function() {\n",
    "    try {\n",
    "      var enemies = GM.findEnemies();\n",
    "      var count = 0;\n",
    "      for (var i = 0; i < enemies.length; i++) {\n",
    "        var e = enemies[i];\n",
    "        try {\n",
    "          if (e.setHealth) { e.setHealth(0); count++; }\n",
    "          else if (e.hp !== undefined) { e.hp = 0; count++; }\n",
    "          if (e.removeFromParent) e.removeFromParent(true);\n",
    "        } catch(ex) {}\n",
    "      }\n",
    "      console.log('[GM] 秒杀 ' + count + ' 个敌人');\n",
    "      return count;\n",
    "    } catch(e) { return 0; }\n",
    "  };\n",
    "\n",
    "  GM.findEnemies = function() {\n",
    "    try {\n",
    "      var scene = cc && cc.director && cc.director.getRunningScene();\n",
    "      if (!scene) return [];\n",
    "      var result = [];\n",
    "      function scan(node) {\n",
    "        if (!node) return;\n",
    "        var name = (node.getName && node.getName()) || '';\n",
    "        if (/Enemy|enemy|Monster|monster|Boss|boss|怪|敌/i.test(name)) result.push(node);\n",
    "        if (node.getChildren) scan(node.getChildren());\n",
    "      }\n",
    "      var ch = scene.getChildren ? scene.getChildren() : [];\n",
    "      for (var i = 0; i < ch.length; i++) scan(ch[i]);\n",
    "      return result;\n",
    "    } catch(e) { return []; }\n",
    "  };\n",
    "\n",
    "  GM.findPlayer = function() {\n",
    "    try {\n",
    "      var scene = cc && cc.director && cc.director.getRunningScene();\n",
    "      if (!scene) return null;\n",
    "      function scan(node) {\n",
    "        if (!node) return null;\n",
    "        var name = (node.getName && node.getName()) || '';\n",
    "        if (/Player|player|Hero|hero|角色|玩家/i.test(name)) return node;\n",
    "        if (node.getChildren) {\n",
    "          var ch = node.getChildren();\n",
    "          if (ch) for (var i = 0; i < ch.length; i++) {\n",
    "            var r = scan(ch[i]); if (r) return r;\n",
    "          }\n",
    "        }\n",
    "        return null;\n",
    "      }\n",
    "      var ch = scene.getChildren ? scene.getChildren() : [];\n",
    "      for (var i = 0; i < ch.length; i++) {\n",
    "        var r = scan(ch[i]); if (r) return r;\n",
    "      }\n",
    "      return null;\n",
    "    } catch(e) { return null; }\n",
    "  };\n",
    "\n",
    "  GM.maxAttack = function(v) { console.log('[GM] attack=' + v); return true; };\n",
    "  GM.maxSpeed = function(v) { console.log('[GM] speed=' + v); return true; };\n",
    "  GM.teleportToEnemy = function(idx) {\n",
    "    try {\n",
    "      var enemies = GM.findEnemies();\n",
    "      if (enemies.length === 0) return false;\n",
    "      var target = enemies[idx % enemies.length];\n",
    "      var player = GM.findPlayer();\n",
    "      if (target && player && target.getPosition && player.setPosition) {\n",
    "        var pos = target.getPosition();\n",
    "        player.setPosition(cc.v2(pos.x, pos.y - 30));\n",
    "        console.log('[GM] 传送到敌人 #' + idx);\n",
    "        return true;\n",
    "      }\n",
    "    } catch(e) {}\n",
    "    return false;\n",
    "  };\n",
    "  GM.printScene = function() {\n",
    "    try {\n",
    "      var scene = cc && cc.director && cc.director.getRunningScene();\n",
    "      if (!scene) { console.log('[GM] no scene'); return; }\n",
    "      function dump(node, depth) {\n",
    "        if (!node) return;\n",
    "        var name = (node.getName && node.getName()) || node.className || '?';\n",
    "        console.log('[GM] ' + Array(depth).join('  ') + name);\n",
    "        if (node.getChildren) {\n",
    "          var ch = node.getChildren();\n",
    "          if (ch) for (var i = 0; i < Math.min(ch.length, 20); i++) dump(ch[i], depth+1);\n",
    "        }\n",
    "      }\n",
    "      dump(scene, 0);\n",
    "    } catch(e) { console.log('[GM] printScene error: ' + e); }\n",
    "  };\n",
    "  GM.enableAll = function() {\n",
    "    GM.toggleOneHitKill();\n",
    "    GM.toggleGodMode();\n",
    "    GM.fullHeal();\n",
    "    console.log('[GM] 全功能开启');\n",
    "  };\n",
    "  GM.disableAll = function() {\n",
    "    if (GM.oneHitKill) GM.toggleOneHitKill();\n",
    "    if (GM.godMode) GM.toggleGodMode();\n",
    "    console.log('[GM] 全功能关闭');\n",
    "  };\n",
    "  window.GM = GM;\n",
    "  console.log('[GM] 大侠闯天下 GM v2.7 已加载');\n",
    "})();\n",
    NULL
};

static JSValueRef my_JSEvaluateScript(JSContextRef ctx, JSStringRef script, JSObjectRef thisObject, JSStringRef sourceURL, int lineNumber, JSValueRef *exception) {
    if (!gCachedContext && ctx) {
        pthread_mutex_lock(&gLock);
        if (!gCachedContext) {
            gCachedContext = ctx;
            dxct_log("[DXCT] JSContext captured, injecting GM...");

            // 优先从外部文件加载
            char *jsPath = getenv(GM_SCRIPT_ENV);
            if (jsPath) {
                dxct_log("[DXCT] Loading from: %s", jsPath);
                FILE *f = fopen(jsPath, "r");
                if (f) {
                    fseek(f, 0, SEEK_END);
                    long sz = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    char *content = malloc((size_t)sz + 1);
                    if (content) {
                        fread(content, 1, (size_t)sz, f);
                        content[(size_t)sz] = '\0';
                        fclose(f);
                        JSStringRef s = JSStringCreateWithUTF8CString(content);
                        if (s) {
                            JSValueRef exc = NULL;
                            orig_JSEvaluateScript(ctx, s, NULL, NULL, 0, &exc);
                            if (exc) dxct_log("[DXCT] External script error");
                            JSStringRelease(s);
                        }
                        free(content);
                    }
                    dxct_log("[DXCT] External script evaluated");
                    gInjected = 1;
                    dxct_show_overlay();
                    pthread_mutex_unlock(&gLock);
                    goto call_orig;
                }
                dxct_log("[DXCT] Failed to open GM file: %s", jsPath);
            }

            // 使用内嵌脚本
            dxct_log("[DXCT] Using embedded GM script");
            // 拼接脚本
            size_t totalLen = 0;
            for (int i = 0; gm_script_parts[i]; i++) totalLen += strlen(gm_script_parts[i]);
            char *fullScript = malloc(totalLen + 1);
            if (fullScript) {
                char *p = fullScript;
                for (int i = 0; gm_script_parts[i]; i++) {
                    strcpy(p, gm_script_parts[i]);
                    p += strlen(gm_script_parts[i]);
                }
                *p = '\0';
                JSStringRef s = JSStringCreateWithUTF8CString(fullScript);
                if (s) {
                    JSValueRef exc = NULL;
                    orig_JSEvaluateScript(ctx, s, NULL, NULL, 0, &exc);
                    if (exc) dxct_log("[DXCT] Embedded script error at line %d", lineNumber);
                    JSStringRelease(s);
                }
                free(fullScript);
            }
            dxct_log("[DXCT] GM injected, oneHitKill:%d godMode:%d", gFlagOneHitKill, gFlagGodMode);
            gInjected = 1;
            // 在主线程显示 GM 调试面板
            dxct_show_overlay();
        }
        pthread_mutex_unlock(&gLock);
    }

call_orig:
    return orig_JSEvaluateScript(ctx, script, thisObject, sourceURL, lineNumber, exception);
}

__attribute__((constructor))
void dylib_init() {
    // v3.1: 默认开启注入, 无需 DXCT_ENABLE=1
    // 仅当显式设置 DXCT_ENABLE=0 时才跳过 (便于特殊场景禁用)
    char *disable = getenv("DXCT_ENABLE");
    if (disable && strcmp(disable, "0") == 0) {
        dxct_log("[DXCT] DXCT_ENABLE=0, skipping");
        return;
    }

    dxct_log("[DXCT] dylib loaded, initializing...");
    // Show a load diagnostic independently of the JS engine hook.
    dxct_show_overlay();

    void *jc = dlopen("/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
    if (jc) {
        orig_JSEvaluateScript = (JSValueRef (*)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSValueRef *))
            dlsym(jc, "JSEvaluateScript");
        if (orig_JSEvaluateScript) {
            dxct_log("[DXCT] Found JSEvaluateScript at %p", orig_JSEvaluateScript);
            struct rebinding binding = {
                "JSEvaluateScript",
                (void *)my_JSEvaluateScript,
                (void **)&orig_JSEvaluateScript
            };
            int rc = rebind_symbols(&binding, 1);
            dxct_log("[DXCT] JSEvaluateScript hook installed, rc=%d", rc);
        } else {
            dxct_log("[DXCT] Failed to find JSEvaluateScript");
        }
        dlclose(jc);
    } else {
        dxct_log("[DXCT] Failed to open JavaScriptCore");
    }
}

__attribute__((destructor))
void dylib_fini() {
    if (gLog) fclose(gLog);
}
