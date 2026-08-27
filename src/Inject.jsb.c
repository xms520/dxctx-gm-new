// Inject.jsb.c - GM hook core (pure C)
// JSContext capture + script injection via JSEvaluateScript hook

#include <JavaScriptCore/JavaScriptCore.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "[DXCT-GM]"
#define GM_SCRIPT_ENV "DXCT_JS_FILE"

// ========== Globals ==========
static JSContextRef gJSContext = NULL;
static FILE *gLog = NULL;
static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;

// ========== Logging ==========
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

// ========== Hook State ==========
typedef JSContextRef (*JSEvalFn)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *);
static JSEvalFn orig_JSEval = NULL;
static int gHookInstalled = 0;
static volatile int gFlagKill = 0;
static volatile int gFlagGod  = 0;

// ========== Hook Function ==========
static JSContextRef my_JSEvaluateScript(JSContextRef ctx, JSStringRef script,
                                        JSObjectRef thisObj, JSStringRef srcURL,
                                        int line, JSStringRef *exc) {
    if (!gHookInstalled && ctx) {
        gHookInstalled = 1;
        pthread_mutex_lock(&gLock);
        if (!gJSContext) {
            CFRetain(ctx);
            gJSContext = ctx;
            dxct_log("JSContext captured: %p", ctx);
            
            // Inject GM script from file
            char *jsPath = getenv(GM_SCRIPT_ENV);
            if (jsPath && strlen(jsPath) > 0) {
                dxct_log("Loading GM from: %s", jsPath);
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
                            JSValueRef result = JSEvaluateScript(ctx, jsStr, NULL, NULL, 1, &excRef);
                            if (excRef) {
                                JSStringRef es = JSValueToStringCopy(ctx, excRef, NULL);
                                if (es) {
                                    const char *m = JSStringGetUTF8CString(es);
                                    dxct_log("GM inject error: %s", m ? m : "?");
                                    if (m) JSStringRelease(es);
                                }
                            } else {
                                dxct_log("GM script injected OK");
                            }
                            JSStringRelease(jsStr);
                        }
                        free(buf);
                    }
                } else {
                    dxct_log("Cannot open: %s", jsPath);
                }
            } else {
                dxct_log("No DXCT_JS_FILE, using embedded fallback");
                const char *fb =
                    "(function(){\n"
                    "  window.GM={oneHitKill:false,godMode:false,\n"
                    "    toggleOneHitKill:function(){this.oneHitKill=!this.oneHitKill;console.log('[GM] 秒杀'+(this.oneHitKill?'ON':'OFF'));return this.oneHitKill;},\n"
                    "    toggleGodMode:function(){this.godMode=!this.godMode;console.log('[GM] 无敌'+(this.godMode?'ON':'OFF'));return this.godMode;},\n"
                    "    hookDamage:function(fn){if(!this._origDamage){this._origDamage=fn;}try{var self=this;return function(){var r=fn.apply(this,arguments);if(self.oneHitKill&&r>0)return 999999;return r;};}catch(e){}return fn;},\n"
                    "    unhookDamage:function(){try{delete this._origDamage;}catch(e){}},\n"
                    "    hookTakeDamage:function(fn){if(!this._origTD){this._origTD=fn;}try{var self=this;return function(d){if(self.godMode)return 0;return fn.call(this,d);};}catch(e){}return fn;},\n"
                    "    unhookTakeDamage:function(){try{delete this._origTD;}catch(e){}},\n"
                    "    findPlayer:function(){try{var s=cc.director.getRunningScene();if(!s)return null;var nodes=s.getChildren();for(var i=0;i<nodes.length;i++){var n=nodes[i];if(n.getName&&n.getName()==='Player')return n;}for(var i=0;i<nodes.length;i++){var n=nodes[i];if(n.getComponent&&n.getComponent(cc.Component))return n;}return null;}catch(e){}return null;},\n"
                    "    findEnemies:function(){try{var s=cc.director.getRunningScene();if(!s)return[];var result=[];var nodes=s.getChildren();for(var i=0;i<nodes.length;i++){var n=nodes[i];var nm=n.getName?n.getName():'';if(nm.indexOf('Enemy')>=0||nm.indexOf('Monster')>=0||nm.indexOf('enemy')>=0||nm.indexOf('monster')>=0){result.push(n);}}return result;}catch(e){}return[];},\n"
                    "    instantKillAll:function(){try{var enemies=this.findEnemies();for(var i=0;i<enemies.length;i++){var e=enemies[i];if(e&&!e.isDestroyed()){if(e.getNodeType===undefined){if(e.removeSelf)e.removeSelf();}else{try{e.setHealth&&e.setHealth(0);}catch(ex){}}}}console.log('[GM] 秒杀了'+enemies.length+'个敌人');}catch(e){console.log('[GM] instantKill error:'+e);}},\n"
                    "    fullHeal:function(){try{var p=this.findPlayer();if(p){p.setHealth&&p.setHealth(999999);p.setMaxHealth&&p.setMaxHealth(999999);console.log('[GM] 满血恢复');}}catch(e){}}\n"
                    "  };window.DXCT=window.GM;console.log('[GM] GM initialized');\n"
                    "})();\n";
                JSStringRef es = JSStringCreateWithUTF8CString(fb);
                if (es) {
                    JSValueRef excRef = NULL;
                    JSEvaluateScript(ctx, es, NULL, NULL, 1, &excRef);
                    JSStringRelease(es);
                    dxct_log("Embedded GM loaded");
                }
            }
        }
        pthread_mutex_unlock(&gLock);
    }
    
    if (orig_JSEval) {
        return orig_JSEval(ctx, script, thisObj, srcURL, line, exc);
    }
    return ctx;
}

// ========== Symbol Hook via Mach-O ==========
static int install_hook(void) {
    Dl_info info;
    if (dladdr((void *)my_JSEvaluateScript, &info) == 0) {
        dxct_log("dladdr failed");
        return -1;
    }
    
    void *handle = dlopen("JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
    if (!handle) handle = dlopen("/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
    if (!handle) {
        dxct_log("dlopen JC failed: %s", dlerror());
        return -1;
    }
    
    void *sym = dlsym(handle, "JSEvaluateScript");
    if (!sym) {
        dxct_log("dlsym JSEvaluateScript failed: %s", dlerror());
        dlclose(handle);
        return -1;
    }
    
    orig_JSEval = (JSEvalFn)sym;
    dxct_log("Found JSEvaluateScript at %p", sym);
    
    // Use vm_remap to replace the symbol (simpler than mach_vm_protect dance)
    // Just install via direct write to the jump table entry
    mach_vm_address_t addr = (mach_vm_address_t)sym;
    mach_vm_size_t page_size = 4096;
    vm_prot_t old_prot;
    
    kern_return_t kr = mach_vm_protect(mach_task_self(), addr, page_size, FALSE,
                                        VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
    if (kr != KERN_SUCCESS) {
        dxct_log("mach_vm_protect fail: %d", kr);
        dlclose(handle);
        return -1;
    }
    
    void **slot = (void **)addr;
    *slot = (void *)my_JSEvaluateScript;
    
    kr = mach_vm_protect(mach_task_self(), addr, page_size, FALSE,
                         VM_PROT_READ | VM_PROT_EXECUTE);
    if (kr != KERN_SUCCESS) {
        dxct_log("mach_vm_protect restore fail: %d", kr);
    }
    
    dlclose(handle);
    dxct_log("Hook installed successfully");
    return 0;
}

// ========== Constructor ==========
__attribute__((constructor))
static void dxct_init(void) {
    const char *enable = getenv("DXCT_ENABLE");
    if (!enable || strcmp(enable, "1") != 0) return;
    
    dxct_log("dxctx_gm.dylib loading... (DXCT_ENABLE=1)");
    install_hook();
}

__attribute__((destructor))
static void dxct_fini(void) {
    pthread_mutex_lock(&gLock);
    if (gJSContext) { CFRelease(gJSContext); gJSContext = NULL; }
    pthread_mutex_unlock(&gLock);
    if (gLog) { fclose(gLog); gLog = NULL; }
}

// ========== Exported API (called from Overlay.m) ==========

int dxct_get_one_hit_kill(void) { return gFlagKill; }
int dxct_get_god_mode(void)     { return gFlagGod; }

void dxct_set_one_hit_kill(int v) {
    gFlagKill = v;
    dxct_log("set oneHitKill=%d", v);
    // Push to JS
    const char *js = v
        ? "if(window.GM){try{GM.toggleOneHitKill();}catch(e){console.log('[GM] err:'+e);}}"
        : "if(window.GM){try{GM.unhookDamage();GM.oneHitKill=false;}catch(e){}}";
    dxct_eval_js(js);
}

void dxct_set_god_mode(int v) {
    gFlagGod = v;
    dxct_log("set godMode=%d", v);
    const char *js = v
        ? "if(window.GM){try{GM.toggleGodMode();}catch(e){console.log('[GM] err:'+e);}}"
        : "if(window.GM){try{GM.unhookTakeDamage();GM.godMode=false;}catch(e){}}";
    dxct_eval_js(js);
}

JSContextRef dxct_get_js_context(void) {
    pthread_mutex_lock(&gLock);
    JSContextRef ctx = gJSContext;
    if (ctx) CFRetain(ctx);
    pthread_mutex_unlock(&gLock);
    return ctx;
}

void dxct_eval_js(const char *js_code) {
    pthread_mutex_lock(&gLock);
    JSContextRef ctx = gJSContext;
    pthread_mutex_unlock(&gLock);
    if (!ctx || !js_code) return;
    
    JSStringRef jsStr = JSStringCreateWithUTF8CString(js_code);
    if (!jsStr) return;
    
    JSValueRef exc = NULL;
    JSValueRef result = JSEvaluateScript(ctx, jsStr, NULL, NULL, 1, &exc);
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
