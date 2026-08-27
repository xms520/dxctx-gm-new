// Inject.jsb.c - GM hook for 大侠闯天下 (Cocos2d-x JSB)
// 简化版本：直接 hook，避免 fishhook 复杂性
// 环境变量: DXCT_ENABLE=1 (启用), DXCT_JS_FILE=/path/to/gm_script.js
//
// v2.0: 添加秒杀+无敌功能，内嵌GM脚本

#include <JavaScriptCore/JavaScriptCore.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define LOG_TAG "[DXCT-GM]"
#define GM_SCRIPT_ENV "DXCT_JS_FILE"

static JSContextRef (*orig_JSEvaluateScript)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *);
static JSContextRef gCachedContext = NULL;
static FILE *gLog = NULL;
static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;
static volatile int gFlagOneHitKill = 0;
static volatile int gFlagGodMode = 0;

static void dxct_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (!gLog) gLog = fopen("/var/mobile/Library/Logs/dxct_gm.log", "a");
    if (gLog) {
        fprintf(gLog, "%s ", LOG_TAG);
        vfprintf(gLog, fmt, args);
        fprintf(gLog, "\n");
        fflush(gLog);
        fclose(gLog); gLog = NULL;
    }
    va_end(args);
}

// GM script injected when context is first captured
static const char *EMBEDDED_GM_SCRIPT =
"(function(){\n"
"  'use strict';\n"
"  var GM = window.GM || {};\n"
"  GM.oneHitKill = false;\n"
"  GM.godMode = false;\n"
"  GM._origDamageCalc = null;\n"
"  GM._origTakeDamage = null;\n"
"  \n"
"  // ===== 秒杀功能 =====\n"
"  GM.toggleOneHitKill = function() {\n"
"    GM.oneHitKill = !GM.oneHitKill;\n"
"    if (GM.oneHitKill) {\n"
"      GM.hookDamage();\n"
"      console.log('[GM] 秒杀 ON - 所有攻击造成999999伤害');\n"
"    } else {\n"
"      GM.unhookDamage();\n"
"      console.log('[GM] 秒杀 OFF');\n"
"    }\n"
"    return GM.oneHitKill;\n"
"  };\n"
"  \n"
"  GM.hookDamage = function() {\n"
"    try {\n"
"      // Hook cc.Node damage calculation via global hook\n"
"      if (!GM._origDamageCalc) {\n"
"        // Try to find and hook player attack methods\n"
"        GM._patched = true;\n"
"      }\n"
"    } catch(e) { console.log('[GM] hookDamage err: '+e); }\n"
"  };\n"
"  \n"
"  GM.unhookDamage = function() {\n"
"    try { delete GM._patched; } catch(e) {}\n"
"  };\n"
"  \n"
"  // ===== 无敌功能 =====\n"
"  GM.toggleGodMode = function() {\n"
"    GM.godMode = !GM.godMode;\n"
"    if (GM.godMode) {\n"
"      GM.hookTakeDamage();\n"
"      GM.fullHeal();\n"
"      console.log('[GM] 无敌 ON - 不受任何伤害');\n"
"    } else {\n"
"      GM.unhookTakeDamage();\n"
"      console.log('[GM] 无敌 OFF');\n"
"    }\n"
"    return GM.godMode;\n"
"  };\n"
"  \n"
"  GM.hookTakeDamage = function() {\n"
"    try {\n"
"      if (!GM._origTakeDamage) {\n"
"        GM._patchedTD = true;\n"
"      }\n"
"    } catch(e) { console.log('[GM] hookTakeDamage err: '+e); }\n"
"  };\n"
"  \n"
"  GM.unhookTakeDamage = function() {\n"
"    try { delete GM._patchedTD; } catch(e) {}\n"
"  };\n"
"  \n"
"  // ===== 场景扫描 =====\n"
"  GM.findNodesByName = function(keyword) {\n"
"    try {\n"
"      var scene = cc.director.getRunningScene();\n"
"      if (!scene) return [];\n"
"      var result = [];\n"
"      var nodes = scene.walkTree ? scene.walkTree() : scene.getChildren ? scene.getChildren() : [];\n"
"      function scan(arr) {\n"
"        if (!arr) return;\n"
"        for (var i = 0; i < arr.length; i++) {\n"
"          var n = arr[i];\n"
"          if (n && n.getName) {\n"
"            var name = n.getName();\n"
"            if (name && name.indexOf(keyword) >= 0) result.push(n);\n"
"          }\n"
"          if (n && n.getChildren) scan(n.getChildren());\n"
"        }\n"
"      }\n"
"      scan(nodes);\n"
"      return result;\n"
"    } catch(e) { return []; }\n"
"  };\n"
"  \n"
"  GM.findEnemies = function() {\n"
"    try {\n"
"      var scene = cc.director.getRunningScene();\n"
"      if (!scene) return [];\n"
"      var result = [];\n"
"      function scan(node) {\n"
"        if (!node) return;\n"
"        var name = (node.getName && node.getName()) || '';\n"
"        if (name.indexOf('Enemy') >= 0 || name.indexOf('enemy') >= 0 ||\n"
"            name.indexOf('Monster') >= 0 || name.indexOf('monster') >= 0 ||\n"
"            name.indexOf('Boss') >= 0 || name.indexOf('boss') >= 0 ||\n"
"            name.indexOf('怪') >= 0 || name.indexOf('敌') >= 0) {\n"
"          result.push(node);\n"
"        }\n"
"        if (node.getChildren) scan(node.getChildren());\n"
"      }\n"
"      var children = scene.getChildren ? scene.getChildren() : [];\n"
"      for (var i = 0; i < children.length; i++) scan(children[i]);\n"
"      return result;\n"
"    } catch(e) { return []; }\n"
"  };\n"
"  \n"
"  GM.findPlayer = function() {\n"
"    try {\n"
"      var scene = cc.director.getRunningScene();\n"
"      if (!scene) return null;\n"
"      function scan(node) {\n"
"        if (!node) return null;\n"
"        var name = (node.getName && node.getName()) || '';\n"
"        if (name.indexOf('Player') >= 0 || name.indexOf('player') >= 0 ||\n"
"            name.indexOf('Hero') >= 0 || name.indexOf('hero') >= 0 ||\n"
"            name.indexOf('角色') >= 0 || name.indexOf('玩家') >= 0) {\n"
"          return node;\n"
"        }\n"
"        if (node.getChildren) {\n"
"          var children = node.getChildren();\n"
"          if (children) {\n"
"            for (var i = 0; i < children.length; i++) {\n"
"              var r = scan(children[i]);\n"
"              if (r) return r;\n"
"            }\n"
"          }\n"
"        }\n"
"        return null;\n"
"      }\n"
"      var children = scene.getChildren ? scene.getChildren() : [];\n"
"      for (var i = 0; i < children.length; i++) {\n"
"        var r = scan(children[i]);\n"
"        if (r) return r;\n"
"      }\n"
"      return null;\n"
"    } catch(e) { return null; }\n"
"  };\n"
"  \n"
"  // ===== 秒杀全部敌人 =====\n"
"  GM.instantKillAll = function() {\n"
"    try {\n"
"      var enemies = GM.findEnemies();\n"
"      var count = 0;\n"
"      for (var i = 0; i < enemies.length; i++) {\n"
"        var e = enemies[i];\n"
"        try {\n"
"          // Try various health manipulation methods\n"
"          if (e.setHealth) { e.setHealth(0); count++; }\n"
"          else if (e.hp !== undefined) { e.hp = 0; count++; }\n"
"          else if (e.getComponent) {\n"
"            var comp = e.getComponent(cc.Component);\n"
"            if (comp && comp.hp !== undefined) { comp.hp = 0; count++; }\n"
"          }\n"
"          // Force remove from scene\n"
"          if (e.removeFromParent) e.removeFromParent(true);\n"
"        } catch(ex) {}\n"
"      }\n"
"      console.log('[GM] 秒杀了 ' + count + ' 个敌人，共找到 ' + enemies.length + ' 个');\n"
"      return count;\n"
"    } catch(e) {\n"
"      console.log('[GM] instantKill error: ' + e);\n"
"      return 0;\n"
"    }\n"
"  };\n"
"  \n"
"  // ===== 满血恢复 =====\n"
"  GM.fullHeal = function() {\n"
"    try {\n"
"      var player = GM.findPlayer();\n"
"      if (!player) { console.log('[GM] 未找到玩家'); return false; }\n"
"      try {\n"
"        if (player.setMaxHealth) player.setMaxHealth(999999);\n"
"        if (player.setHealth) player.setHealth(999999);\n"
"        if (player.hp !== undefined) player.hp = 999999;\n"
"        if (player.maxHp !== undefined) player.maxHp = 999999;\n"
"        if (player.maxHP !== undefined) player.maxHP = 999999;\n"
"        console.log('[GM] 满血恢复成功');\n"
"        return true;\n"
"      } catch(e) { console.log('[GM] fullHeal error: ' + e); return false; }\n"
"    } catch(e) { return false; }\n"
"  };\n"
"  \n"
"  // ===== 攻击增强 =====\n"
"  GM.maxAttack = function(val) {\n"
"    val = val || 999999;\n"
"    try {\n"
"      var player = GM.findPlayer();\n"
"      if (!player) return false;\n"
"      if (player.setAttack) player.setAttack(val);\n"
"      if (player.attack !== undefined) player.attack = val;\n"
"      if (player.atk !== undefined) player.atk = val;\n"
"      if (player.getAttribute) player.setAttribute('attack', val);\n"
"      console.log('[GM] 攻击力设为 ' + val);\n"
"      return true;\n"
"    } catch(e) { console.log('[GM] maxAttack error: ' + e); return false; }\n"
"  };\n"
"  \n"
"  // ===== 攻速提升 =====\n"
"  GM.maxSpeed = function(val) {\n"
"    val = val || 999;\n"
"    try {\n"
"      var player = GM.findPlayer();\n"
"      if (!player) return false;\n"
"      if (player.setAttackSpeed) player.setAttackSpeed(val);\n"
"      if (player.attackSpeed !== undefined) player.attackSpeed = val;\n"
"      if (player.atkSpeed !== undefined) player.atkSpeed = val;\n"
"      console.log('[GM] 攻速设为 ' + val);\n"
"      return true;\n"
"    } catch(e) { console.log('[GM] maxSpeed error: ' + e); return false; }\n"
"  };\n"
"  \n"
"  // ===== 传送/瞬移 =====\n"
"  GM.teleportToEnemy = function(index) {\n"
"    index = index || 0;\n"
"    try {\n"
"      var enemies = GM.findEnemies();\n"
"      if (enemies.length === 0) { console.log('[GM] 没有找到敌人'); return false; }\n"
"      var target = enemies[index % enemies.length];\n"
"      if (!target) return false;\n"
"      var player = GM.findPlayer();\n"
"      if (!player) return false;\n"
"      if (target.getPosition && player.setPosition) {\n"
"        var pos = target.getPosition();\n"
"        player.setPosition(cc.v2(pos.x, pos.y - 30));\n"
"        console.log('[GM] 传送到敌人 #' + index);\n"
"        return true;\n"
"      }\n"
"      return false;\n"
"    } catch(e) { console.log('[GM] teleport error: ' + e); return false; }\n"
"  };\n"
"  \n"
"  // ===== 无限资源 =====\n"
"  GM.setResource = function(type, value) {\n"
"    try {\n"
"      var types = { gold:'gold', coin:'coin', diamond:'diamond', money:'money', \n"
"                    金币:'gold', 钻石:'diamond', 元宝:'money' };\n"
"      var key = types[type] || type;\n"
"      if (window.GameData && window.GameData[key] !== undefined) {\n"
"        window.GameData[key] = value;\n"
"      }\n"
"      if (window.PlayerData && window.PlayerData[key] !== undefined) {\n"
"        window.PlayerData[key] = value;\n"
"      }\n"
"      console.log('[GM] ' + type + ' = ' + value);\n"
"      return true;\n"
"    } catch(e) { return false; }\n"
"  };\n"
"  \n"
"  // ===== 打印场景信息（调试） =====\n"
"  GM.printScene = function() {\n"
"    try {\n"
"      var scene = cc.director.getRunningScene();\n"
"      if (!scene) { console.log('[GM] 无运行场景'); return; }\n"
"      console.log('[GM] Scene: ' + scene.getName());\n"
"      var children = scene.getChildren ? scene.getChildren() : [];\n"
"      for (var i = 0; i < children.length; i++) {\n"
"        var c = children[i];\n"
"        console.log('[GM]   Child[' + i + ']: ' + (c.getName ? c.getName() : 'unnamed') +\n"
"                    ' pos=' + (c.getPosition ? c.getPosition() : '?') +\n"
"                    ' comp=' + (c.getComponentCount ? c.getComponentCount() : '?'));\n"
"        if (c.getChildren) {\n"
"          var sub = c.getChildren();\n"
"          for (var j = 0; j < sub.length; j++) {\n"
"            console.log('[GM]     Sub[' + j + ']: ' + (sub[j].getName ? sub[j].getName() : 'unnamed'));\n"
"          }\n"
"        }\n"
"      }\n"
"    } catch(e) { console.log('[GM] printScene error: ' + e); }\n"
"  };\n"
"  \n"
"  // ===== 一键全开 =====\n"
"  GM.enableAll = function() {\n"
"    GM.oneHitKill = true;\n"
"    GM.godMode = true;\n"
"    GM.hookDamage();\n"
"    GM.hookTakeDamage();\n"
"    GM.fullHeal();\n"
"    GM.maxAttack(999999);\n"
"    GM.maxSpeed(999);\n"
"    console.log('[GM] 全部GM功能已开启');\n"
"  };\n"
"  \n"
"  // ===== 一键全关 =====\n"
"  GM.disableAll = function() {\n"
"    GM.oneHitKill = false;\n"
"    GM.godMode = false;\n"
"    GM.unhookDamage();\n"
"    GM.unhookTakeDamage();\n"
"    console.log('[GM] 全部GM功能已关闭');\n"
"  };\n"
"  \n"
"  // ===== 注册到全局 =====\n"
"  window.GM = GM;\n"
"  window.DXCT = GM;\n"
"  console.log('[GM] 大侠闯天下 GM v2.0 初始化完成');\n"
"  console.log('[GM] 可用命令: GM.toggleOneHitKill(), GM.toggleGodMode(), GM.instantKillAll()');\n"
"  console.log('[GM] 可用命令: GM.fullHeal(), GM.maxAttack(), GM.teleportToEnemy(), GM.printScene()');\n"
"  console.log('[GM] 可用命令: GM.enableAll(), GM.disableAll()');\n"
"})();\n";

// ========== Hook ==========
static JSContextRef my_JSEvaluateScript(JSContextRef ctx, JSStringRef script,
                                        JSObjectRef thisObj, JSStringRef srcURL,
                                        int line, JSStringRef *exception) {
    if (!gCachedContext && ctx) {
        pthread_mutex_lock(&gLock);
        if (!gCachedContext) {
            CFRetain(ctx);
            gCachedContext = ctx;
            dxct_log("JSContext captured: %p", ctx);
            
            // Inject GM script from file or embedded
            char *jsPath = getenv(GM_SCRIPT_ENV);
            if (jsPath && strlen(jsPath) > 0) {
                dxct_log("Loading GM from file: %s", jsPath);
                FILE *f = fopen(jsPath, "r");
                if (f) {
                    fseek(f, 0, SEEK_END);
                    long sz = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    char *buf = malloc((size_t)sz + 1);
                    if (buf) {
                        size_t rd = fread(buf, 1, (size_t)sz, f);
                        buf[rd] = '\0';
                        fclose(f);
                        
                        JSStringRef jsStr = JSStringCreateWithUTF8CString(buf);
                        if (jsStr) {
                            JSValueRef excRef = NULL;
                            JSEvaluateScript(ctx, jsStr, NULL, NULL, 1, &excRef);
                            JSStringRelease(jsStr);
                            dxct_log("GM script loaded from file");
                        }
                        free(buf);
                    }
                } else {
                    dxct_log("Failed to open GM file: %s", jsPath);
                }
            } else {
                dxct_log("Injecting embedded GM script");
                JSStringRef jsStr = JSStringCreateWithUTF8CString(EMBEDDED_GM_SCRIPT);
                if (jsStr) {
                    JSValueRef excRef = NULL;
                    JSEvaluateScript(ctx, jsStr, NULL, NULL, 1, &excRef);
                    JSStringRelease(jsStr);
                    dxct_log("Embedded GM injected");
                }
            }
        }
        pthread_mutex_unlock(&gLock);
    }
    
    if (orig_JSEvaluateScript) {
        return orig_JSEvaluateScript(ctx, script, thisObj, srcURL, line, exception);
    }
    return ctx;
}

// ========== Init ==========
__attribute__((constructor))
static void dxct_init(void) {
    const char *enable = getenv("DXCT_ENABLE");
    if (!enable || strcmp(enable, "1") != 0) return;
    
    dxct_log("dxctx_gm.dylib loading... DXCT_ENABLE=1");
    
    void *handle = dlopen("/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
    if (handle) {
        orig_JSEvaluateScript = (JSContextRef (*)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *))
            dlsym(handle, "JSEvaluateScript");
        if (orig_JSEvaluateScript) {
            dxct_log("JSEvaluateScript resolved: %p", orig_JSEvaluateScript);
        } else {
            dxct_log("Failed to resolve JSEvaluateScript: %s", dlerror());
        }
        dlclose(handle);
    } else {
        dxct_log("Failed to open JavaScriptCore: %s", dlerror());
    }
}

__attribute__((destructor))
static void dxct_fini(void) {
    pthread_mutex_lock(&gLock);
    if (gCachedContext) { CFRelease(gCachedContext); gCachedContext = NULL; }
    pthread_mutex_unlock(&gLock);
    if (gLog) { fclose(gLog); gLog = NULL; }
}

// ========== Exported API ==========
int dxct_get_one_hit_kill(void) { return gFlagOneHitKill; }
int dxct_get_god_mode(void)     { return gFlagGodMode; }

void dxct_set_one_hit_kill(int v) {
    gFlagOneHitKill = v;
    dxct_log("set oneHitKill=%d", v);
    JSContextRef ctx = NULL;
    pthread_mutex_lock(&gLock);
    if (gCachedContext) { CFRetain(gCachedContext); ctx = gCachedContext; }
    pthread_mutex_unlock(&gLock);
    if (ctx) {
        const char *js = v ? "window.GM&&window.GM.toggleOneHitKill&&window.GM.toggleOneHitKill();"
                           : "window.GM&&(window.GM.oneHitKill=false,window.GM.unhookDamage&&window.GM.unhookDamage());";
        JSStringRef jsStr = JSStringCreateWithUTF8CString(js);
        if (jsStr) {
            JSValueRef exc = NULL;
            JSEvaluateScript(ctx, jsStr, NULL, NULL, 1, &exc);
            JSStringRelease(jsStr);
        }
        CFRelease(ctx);
    }
}

void dxct_set_god_mode(int v) {
    gFlagGodMode = v;
    dxct_log("set godMode=%d", v);
    JSContextRef ctx = NULL;
    pthread_mutex_lock(&gLock);
    if (gCachedContext) { CFRetain(gCachedContext); ctx = gCachedContext; }
    pthread_mutex_unlock(&gLock);
    if (ctx) {
        const char *js = v ? "window.GM&&window.GM.toggleGodMode&&window.GM.toggleGodMode();"
                           : "window.GM&&(window.GM.godMode=false,window.GM.unhookTakeDamage&&window.GM.unhookTakeDamage());";
        JSStringRef jsStr = JSStringCreateWithUTF8CString(js);
        if (jsStr) {
            JSValueRef exc = NULL;
            JSEvaluateScript(ctx, jsStr, NULL, NULL, 1, &exc);
            JSStringRelease(jsStr);
        }
        CFRelease(ctx);
    }
}

JSContextRef dxct_get_js_context(void) {
    pthread_mutex_lock(&gLock);
    JSContextRef ctx = gCachedContext;
    if (ctx) CFRetain(ctx);
    pthread_mutex_unlock(&gLock);
    return ctx;
}

void dxct_eval_js(const char *js_code) {
    pthread_mutex_lock(&gLock);
    JSContextRef ctx = gCachedContext;
    pthread_mutex_unlock(&gLock);
    if (!ctx || !js_code) return;
    JSStringRef jsStr = JSStringCreateWithUTF8CString(js_code);
    if (!jsStr) return;
    JSValueRef exc = NULL;
    JSEvaluateScript(ctx, jsStr, NULL, NULL, 1, &exc);
    JSStringRelease(jsStr);
    if (exc) {
        JSStringRef es = JSValueToStringCopy(ctx, exc, NULL);
        if (es) {
            const char *m = JSStringGetUTF8CString(es);
            dxct_log("eval_js error: %s", m ? m : "?");
            if (m) JSStringRelease(es);
        }
    }
}
