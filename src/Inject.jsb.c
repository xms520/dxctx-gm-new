// Inject.jsb.c - GM hook core (pure C, no ObjC)
// Handles JSContext capture and GM script injection
// Env: DXCT_ENABLE=1, DXCT_JS_FILE=/path/to/gm_script.js

#include <JavaScriptCore/JavaScriptCore.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mach/mach.h>
#include <sys/mman.h>

#define LOG_TAG "[DXCT-GM]"
#define GM_SCRIPT_ENV "DXCT_JS_FILE"

// ========== Global State ==========
static JSContextRef gJSContext = NULL;
static FILE *gLog = NULL;
static pthread_mutex_t gCtxLock = PTHREAD_MUTEX_INITIALIZER;

// ========== Logging ==========
static void dxct_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (!gLog) {
        gLog = fopen("/var/mobile/Library/Logs/dxct_gm.log", "a");
    }
    if (gLog) {
        fprintf(gLog, "%s ", LOG_TAG);
        vfprintf(gLog, fmt, args);
        fprintf(gLog, "\n");
        fflush(gLog);
        fclose(gLog);
        gLog = NULL;
    }
    va_end(args);
}

// ========== JSEvaluateScript Hook ==========
typedef JSContextRef (*JSEvaluateScriptFn)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *);
static JSEvaluateScriptFn orig_JSEvaluateScript = NULL;

static JSContextRef hooked_JSEvaluateScript(JSContextRef ctx, JSStringRef script, JSObjectRef thisObject, JSStringRef sourceURL, int lineNumber, JSStringRef *exception) {
    // Capture context on first call
    if (!gJSContext && ctx) {
        pthread_mutex_lock(&gCtxLock);
        if (!gJSContext) {
            CFRetain(ctx);
            gJSContext = ctx;
            dxct_log("JSContext captured: %p", ctx);
            
            // Inject GM script from file
            char *jsPath = getenv(GM_SCRIPT_ENV);
            if (jsPath && strlen(jsPath) > 0) {
                dxct_log("Injecting GM from: %s", jsPath);
                FILE *f = fopen(jsPath, "r");
                if (f) {
                    fseek(f, 0, SEEK_END);
                    long sz = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    char *buf = malloc((size_t)sz + 1);
                    if (buf) {
                        fread(buf, 1, (size_t)sz, f);
                        buf[sz] = '\0';
                        fclose(f);
                        
                        JSStringRef jsStr = JSStringCreateWithUTF8CString(buf);
                        if (jsStr) {
                            JSValueRef exc = NULL;
                            JSValueRef result = JSEvaluateScript(ctx, jsStr, NULL, NULL, 1, &exc);
                            if (exc) {
                                JSStringRef excStr = JSValueToStringCopy(ctx, exc, NULL);
                                if (excStr) {
                                    const char *msg = JSStringGetUTF8CString(excStr);
                                    dxct_log("GM inject error: %s", msg ? msg : "unknown");
                                    if (msg) JSStringRelease(excStr);
                                }
                            } else {
                                dxct_log("GM script injected successfully");
                            }
                            JSStringRelease(jsStr);
                        }
                        free(buf);
                    }
                } else {
                    dxct_log("Cannot open GM script: %s", jsPath);
                }
            } else {
                dxct_log("No DXCT_JS_FILE set, injecting embedded fallback GM");
                // Embedded fallback GM (minimal)
                const char *embedded =
                    "(function(){\n"
                    "  window.GM = {\n"
                    "    oneHitKill: false,\n"
                    "    godMode: false,\n"
                    "    enableOneHitKill: function(){ this.oneHitKill=true; console.log('[GM] 秒杀ON'); return true; },\n"
                    "    enableGodMode:   function(){ this.godMode=true;  console.log('[GM] 无敌ON'); return true; },\n"
                    "    disableAll:      function(){ this.oneHitKill=false; this.godMode=false; console.log('[GM] 全部关闭'); return true; }\n"
                    "  };\n"
                    "  window.DXCT = window.GM;\n"
                    "})();\n";
                JSStringRef es = JSStringCreateWithUTF8CString(embedded);
                if (es) {
                    JSValueRef exc = NULL;
                    JSEvaluateScript(ctx, es, NULL, NULL, 1, &exc);
                    JSStringRelease(es);
                    dxct_log("Embedded fallback GM injected");
                }
            }
        }
        pthread_mutex_unlock(&gCtxLock);
    }
    
    if (orig_JSEvaluateScript) {
        return orig_JSEvaluateScript(ctx, script, thisObject, sourceURL, lineNumber, exception);
    }
    return ctx;
}

// ========== Symbol Rebinding via mach_vm ==========
static int hook_symbol(const char *symbol_name, void *replacement, void **original_out) {
    Dl_info info;
    if (dladdr((void *)hooked_JSEvaluateScript, &info) == 0) {
        dxct_log("dladdr failed");
        return -1;
    }
    
    void *handle = dlopen("JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
    if (!handle) {
        handle = dlopen("/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
    }
    if (!handle) {
        dxct_log("dlopen JavaScriptCore failed: %s", dlerror());
        return -1;
    }
    
    void *sym = dlsym(handle, symbol_name);
    if (!sym) {
        dxct_log("dlsym %s failed: %s", symbol_name, dlerror());
        dlclose(handle);
        return -1;
    }
    
    // Save original
    *original_out = sym;
    dxct_log("Found %s at %p", symbol_name, sym);
    
    // Make page writable
    mach_vm_address_t addr = (mach_vm_address_t)sym;
    mach_vm_size_t page_size = 4096;
    vm_prot_t old_prot;
    kern_return_t kr = mach_vm_protect(mach_task_self(), addr, page_size, FALSE,
                                        VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
    if (kr != KERN_SUCCESS) {
        dxct_log("mach_vm_protect failed: %d", kr);
        dlclose(handle);
        return -1;
    }
    
    // Install replacement (store pointer to replacement function)
    void **slot = (void **)addr;
    *slot = replacement;
    
    // Restore execute permission
    kr = mach_vm_protect(mach_task_self(), addr, page_size, FALSE,
                         VM_PROT_READ | VM_PROT_EXECUTE);
    if (kr != KERN_SUCCESS) {
        dxct_log("mach_vm_protect restore failed: %d", kr);
    }
    
    dlclose(handle);
    dxct_log("Symbol rebinding installed for %s", symbol_name);
    return 0;
}

// ========== dylib entry point ==========
__attribute__((constructor))
static void dxct_gm_init(void) {
    const char *enable = getenv("DXCT_ENABLE");
    if (!enable || strcmp(enable, "1") != 0) {
        return;
    }
    
    dxct_log("dxctx_gm.dylib loading... (DXCT_ENABLE=1)");
    
    // Try direct symbol rebind first
    if (hook_symbol("JSEvaluateScript", (void *)hooked_JSEvaluateScript, 
                    (void **)&orig_JSEvaluateScript) == 0) {
        dxct_log("Direct symbol hook installed");
    } else {
        // Fallback: try to find via dlsym at runtime
        dxct_log("Direct hook failed, will try runtime lookup");
        void *jc = dlopen("/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
        if (jc) {
            orig_JSEvaluateScript = (JSEvaluateScriptFn)dlsym(jc, "JSEvaluateScript");
            dxct_log("Found JSEvaluateScript via dlsym: %p", orig_JSEvaluateScript);
            // Can't easily replace without Mach-O editing, but we can still capture context
            // by wrapping through JSEvaluateScript if it's called through our library
        }
    }
}

__attribute__((destructor))
static void dxct_gm_fini(void) {
    pthread_mutex_lock(&gCtxLock);
    if (gJSContext) {
        CFRelease(gJSContext);
        gJSContext = NULL;
    }
    pthread_mutex_unlock(&gCtxLock);
    
    if (gLog) {
        fclose(gLog);
        gLog = NULL;
    }
    dxct_log("dylib unloaded");
}

// ========== Exported symbols (for native overlay to call) ==========
int dxct_get_one_hit_kill(void) { return 0; }
int dxct_get_god_mode(void)   { return 0; }
void dxct_set_one_hit_kill(int v) { (void)v; }
void dxct_set_god_mode(int v)   { (void)v; }

// Get captured JSContext (for native overlay)
JSContextRef dxct_get_js_context(void) {
    pthread_mutex_lock(&gCtxLock);
    JSContextRef ctx = gJSContext;
    if (ctx) CFRetain(ctx);
    pthread_mutex_unlock(&gCtxLock);
    return ctx;
}

// Evaluate JS in captured context
void dxct_eval_js(const char *js_code) {
    pthread_mutex_lock(&gCtxLock);
    JSContextRef ctx = gJSContext;
    pthread_mutex_unlock(&gCtxLock);
    
    if (!ctx || !js_code) return;
    
    JSStringRef jsStr = JSStringCreateWithUTF8CString(js_code);
    if (!jsStr) return;
    
    JSValueRef exc = NULL;
    JSValueRef result = JSEvaluateScript(ctx, jsStr, NULL, NULL, 1, &exc);
    JSStringRelease(jsStr);
    
    if (exc) {
        JSStringRef excStr = JSValueToStringCopy(ctx, exc, NULL);
        if (excStr) {
            const char *msg = JSStringGetUTF8CString(excStr);
            dxct_log("dxct_eval_js error: %s", msg ? msg : "unknown");
            if (msg) JSStringRelease(excStr);
        }
    } else {
        dxct_log("dxct_eval_js success");
    }
}

// ========== GM flag setters (called from Overlay.m) ==========
static volatile int gFlagOneHitKill = 0;
static volatile int gFlagGodMode    = 0;

void dxct_set_one_hit_kill(int v) {
    gFlagOneHitKill = v;
    dxct_log("oneHitKill=%d", v);
}

int dxct_get_one_hit_kill(void) {
    return gFlagOneHitKill;
}

void dxct_set_god_mode(int v) {
    gFlagGodMode = v;
    dxct_log("godMode=%d", v);
}

int dxct_get_god_mode(void) {
    return gFlagGodMode;
}
