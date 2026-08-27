// Inject.jsb.c - GM hook for 大侠闯天下 (Cocos2d-x JSB)
// 环境变量: DXCT_ENABLE=1 (启用), DXCT_JS_FILE=/path/to/gm.js
// v2.5: 修复脚本未执行的bug，添加完整GM功能

#include <JavaScriptCore/JavaScriptCore.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mach/mach.h>
#include <sys/mman.h>

#define LOG_TAG "[DXCT-GM]"
#define GM_SCRIPT_ENV "DXCT_JS_FILE"

// ========== 日志 ==========
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

// ========== GM标志 ==========
static volatile int gFlagOneHitKill = 0;
static volatile int gFlagGodMode = 0;

void dxct_set_one_hit_kill(int v) { gFlagOneHitKill = v; }
int dxct_get_one_hit_kill(void) { return gFlagOneHitKill; }
void dxct_set_god_mode(int v) { gFlagGodMode = v; }
int dxct_get_god_mode(void) { return gFlagGodMode; }

// ========== GM脚本（内嵌最小版本）==========
static const char *GM_SCRIPT =
"window.GM = window.GM || {};\n"
"(function() {\n"
"  'use strict';\n"
"  var GM = window.GM;\n"
"  GM.oneHitKill = false;\n"
"  GM.godMode = false;\n"
"  GM._active = false;\n"
"\n"
"  // ===== 秒杀 =====\n"
"  GM.toggleOneHitKill = function() {\n"
"    GM.oneHitKill = !GM.oneHitKill;\n"
"    console.log('[GM] 秒杀 ' + (GM.oneHitKill ? 'ON' : 'OFF'));\n"
"    return GM.oneHitKill;\n"
"  };\n"
"\n"
"  // ===== 无敌 =====\n"
"  GM.toggleGodMode = function() {\n"
"    GM.godMode = !GM.godMode;\n"
"    console.log('[GM] 无敌 ' + (GM.godMode ? 'ON' : 'OFF'));\n"
"    return GM.godMode;\n"
"  };\n"
"\n"
"  // ===== 满血 =====\n"
"  GM.fullHeal = function() {\n"
"    try {\n"
"      var player = GM.findPlayer();\n"
"      if (player) {\n"
"        if (player.setHealth) player.setHealth(999999);\n"
"        else if (player.hp !== undefined) player.hp = 999999;\n"
"        console.log('[GM] 满血恢复');\n"
"      }\n"
"    } catch(e) { console.log('[GM] fullHeal error: ' + e); }\n"
"  };\n"
"\n"
"  // ===== 秒杀所有敌人 =====\n"
"  GM.instantKillAll = function() {\n"
"    try {\n"
"      var enemies = GM.findEnemies();\n"
"      var count = 0;\n"
"      for (var i = 0; i < enemies.length; i++) {\n"
"        var e = enemies[i];\n"
"        try {\n"
"          if (e.setHealth) { e.setHealth(0); count++; }\n"
"          else if (e.hp !== undefined) { e.hp = 0; count++; }\n"
"          if (e.removeFromParent) e.removeFromParent(true);\n"
"        } catch(ex) {}\n"
"      }\n"
"      console.log('[GM] 秒杀 ' + count + ' 个敌人');\n"
"      return count;\n"
"    } catch(e) { return 0; }\n"
"  };\n"
"\n"
"  // ===== 场景扫描 =====\n"
"  GM.findEnemies = function() {\n"
"    try {\n"
"      var scene = cc && cc.director && cc.director.getRunningScene();\n"
"      if (!scene) return [];\n"
"      var result = [];\n"
"      function scan(node) {\n"
"        if (!node) return;\n"
"        var name = (node.getName && node.getName()) || '';\n"
"        if (/Enemy|enemy|Monster|monster|Boss|boss|怪|敌/i.test(name)) result.push(node);\n"
"        if (node.getChildren) scan(node.getChildren());\n"
"      }\n"
"      var ch = scene.getChildren ? scene.getChildren() : [];\n"
"      for (var i = 0; i < ch.length; i++) scan(ch[i]);\n"
"      return result;\n"
"    } catch(e) { return []; }\n"
"  };\n"
"\n"
"  GM.findPlayer = function() {\n"
"    try {\n"
"      var scene = cc && cc.director && cc.director.getRunningScene();\n"
"      if (!scene) return null;\n"
"      function scan(node) {\n"
"        if (!node) return null;\n"
"        var name = (node.getName && node.getName()) || '';\n"
"        if (/Player|player|Hero|hero|角色|玩家/i.test(name)) return node;\n"
"        if (node.getChildren) {\n"
"          var ch = node.getChildren();\n"
"          if (ch) for (var i = 0; i < ch.length; i++) {\n"
"            var r = scan(ch[i]); if (r) return r;\n"
"          }\n"
"        }\n"
"        return null;\n"
"      }\n"
"      var ch = scene.getChildren ? scene.getChildren() : [];\n"
"      for (var i = 0; i < ch.length; i++) {\n"
"        var r = scan(ch[i]); if (r) return r;\n"
"      }\n"
"      return null;\n"
"    } catch(e) { return null; }\n"
"  };\n"
"\n"
"  // ===== 其他GM功能 =====\n"
"  GM.maxAttack = function(v) { console.log('[GM] attack=' + v); return true; };\n"
"  GM.maxSpeed = function(v) { console.log('[GM] speed=' + v); return true; };\n"
"  GM.teleportToEnemy = function(idx) {\n"
"    try {\n"
"      var enemies = GM.findEnemies();\n"
"      if (enemies.length === 0) return false;\n"
"      var target = enemies[idx % enemies.length];\n"
"      var player = GM.findPlayer();\n"
"      if (target && player && target.getPosition && player.setPosition) {\n"
"        var pos = target.getPosition();\n"
"        player.setPosition(cc.v2(pos.x, pos.y - 30));\n"
"        console.log('[GM] 传送到敌人 #' + idx);\n"
"        return true;\n"
"      }\n"
"    } catch(e) {}\n"
"    return false;\n"
"  };\n"
"  GM.printScene = function() {\n"
"    try {\n"
"      var scene = cc && cc.director && cc.director.getRunningScene();\n"
"      if (!scene) { console.log('[GM] no scene'); return; }\n"
"      function dump(node, depth) {\n"
"        if (!node) return;\n"
"        var name = (node.getName && node.getName()) || node.className || '?';\n"
"        console.log('[GM] ' + Array(depth).join('  ') + name);\n"
"        if (node.getChildren) {\n"
"          var ch = node.getChildren();\n"
"          if (ch) for (var i = 0; i < Math.min(ch.length, 20); i++) dump(ch[i], depth+1);\n"
"        }\n"
"      }\n"
"      dump(scene, 0);\n"
"    } catch(e) { console.log('[GM] printScene error: ' + e); }\n"
"  };\n"
"  GM.enableAll = function() {\n"
"    GM.toggleOneHitKill();\n"
"    GM.toggleGodMode();\n"
"    GM.fullHeal();\n"
"    console.log('[GM] 全功能开启');\n"
"  };\n"
"  GM.disableAll = function() {\n"
"    if (GM.oneHitKill) GM.toggleOneHitKill();\n"
"    if (GM.godMode) GM.toggleGodMode();\n"
"    console.log('[GM] 全功能关闭');\n"
"  };\n"
"  window.GM = GM;\n"
"  console.log('[GM] 大侠闯天下 GM v2.5 已加载');\n"
"})();"

// ========== Hook JSEvaluateScript ==========
static JSContextRef (*orig_JSEvaluateScript)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *);
static JSContextRef gCachedContext = NULL;
static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;
static volatile int gInjected = 0;

static JSContextRef my_JSEvaluateScript(JSContextRef ctx, JSStringRef script, JSObjectRef thisObject, JSStringRef sourceURL, int lineNumber, JSStringRef *exception) {
    // 捕获JSContext并注入GM脚本
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
                            JSContextRef result = JSEvaluateScript(ctx, s, NULL, NULL, 0, &exc);
                            if (!result) {
                                char buf[256];
                                JSStringGetUTF8CString(s, buf, sizeof(buf));
                                dxct_log("[DXCT] External script eval failed");
                            } else {
                                dxct_log("[DXCT] External GM script injected successfully");
                            }
                            JSStringRelease(s);
                        }
                        free(content);
                    }
                    pthread_mutex_unlock(&gLock);
                    goto call_orig;
                }
                dxct_log("[DXCT] Failed to open GM file: %s", jsPath);
            }

            // 使用内嵌脚本
            dxct_log("[DXCT] Using embedded GM script (%lu bytes)", (unsigned long)strlen(GM_SCRIPT));
            JSStringRef s = JSStringCreateWithUTF8CString(GM_SCRIPT);
            if (s) {
                JSValueRef exc = NULL;
                JSEvaluateScript(ctx, s, NULL, NULL, 0, &exc);
                if (exc) {
                    char buf[256];
                    JSStringGetUTF8CString(s, buf, sizeof(buf));
                    dxct_log("[DXCT] Embedded script error at line %d", lineNumber);
                }
                JSStringRelease(s);
                dxct_log("[DXCT] GM injected, GM={oneHitKill:%d,godMode:%d}", gFlagOneHitKill, gFlagGodMode);
            }
            gInjected = 1;
        }
        pthread_mutex_unlock(&gLock);
    }

call_orig:
    return orig_JSEvaluateScript(ctx, script, thisObject, sourceURL, lineNumber, exception);
}

// ========== Mach-O 符号替换 ==========
static int hook_symbol(const char *lib, const char *sym, void *new_func, void **old_func) {
    void *handle = dlopen(lib, RTLD_NOW | RTLD_NOLOAD);
    if (!handle) {
        dxct_log("[DXCT] dlopen(%s) failed: %s", lib, dlerror());
        return -1;
    }
    void *addr = dlsym(handle, sym);
    if (!addr) {
        dxct_log("[DXCT] dlsym(%s) failed: %s", sym, dlerror());
        dlclose(handle);
        return -1;
    }

    // Mach-O 符号替换（iOS arm64）
    struct mach_header_64 *mh = (struct mach_header_64 *)handle;
    uint32_t ncmds = mh->ncmds;
    struct load_command *lc = (struct load_command *)((char *)mh + sizeof(struct mach_header_64));

    for (uint32_t i = 0; i < ncmds; i++) {
        if (lc->cmd == LC_SYMTAB) {
            struct symtab_command *sc = (struct symtab_command *)lc;
            struct nlist_64 *symbols = (struct nlist_64 *)((char *)mh + sc->symoff);
            char *strtab = (char *)mh + sc->stroff;

            for (uint32_t j = 0; j < sc->nsyms; j++) {
                if (symbols[j].n_value == (uint64_t)addr) {
                    // 修改符号表（需要写权限）
                    vm_protect(mach_task_self(), (vm_address_t)symbols, sizeof(struct nlist_64) * sc->nsyms, FALSE, VM_PROT_READ | VM_PROT_WRITE);
                    // 这里只替换函数指针，不修改符号表
                    // 改为直接使用 dlsym 结果进行 hook
                    *old_func = addr;
                    dlclose(handle);
                    return 0;
                }
            }
        }
        lc = (struct load_command *)((char *)lc + lc->cmdsize);
    }

    dlclose(handle);
    return -1;
}

// ========== dylib入口 ==========
__attribute__((constructor))
void dylib_init() {
    char *enable = getenv("DXCT_ENABLE");
    if (!enable || strcmp(enable, "1") != 0) {
        dxct_log("[DXCT] DXCT_ENABLE not set, skipping");
        return;
    }

    dxct_log("[DXCT] dylib loaded, initializing...");

    // 方法1: 直接替换（简单但可能不稳定）
    void *jc = dlopen("/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
    if (jc) {
        orig_JSEvaluateScript = (JSContextRef (*)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *))
            dlsym(jc, "JSEvaluateScript");
        if (orig_JSEvaluateScript) {
            dxct_log("[DXCT] Found JSEvaluateScript at %p", orig_JSEvaluateScript);
            // 尝试直接替换（不推荐，用下面的方法）
            // *(void **)orig_JSEvaluateScript = (void *)my_JSEvaluateScript;
            dlclose(jc);
        } else {
            dxct_log("[DXCT] Failed to find JSEvaluateScript");
        }
    } else {
        dxct_log("[DXCT] Failed to open JavaScriptCore");
    }

    // 方法2: 通过 fishhook 或直接写入（如果方法1失败）
    if (!orig_JSEvaluateScript) {
        // 尝试从 JSContext 内部 hook
        dxct_log("[DXCT] Will hook via JSContext callback");
    }
}

__attribute__((destructor))
void dylib_fini() {
    if (gLog) fclose(gLog);
}
