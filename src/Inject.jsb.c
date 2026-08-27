// Inject.jsb.c - GM hook for 大侠闯天下 (Cocos2d-x JSB)
// Features: 秒杀 (One-hit Kill), 无敌 (God Mode)
// Env: DXCT_ENABLE=1, DXCT_JS_FILE=/path/to/gm_script.js

#include <JavaScriptCore/JavaScriptCore.h>
#include <UIKit/UIKit.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mach/mach.h>
#include <sys/mman.h>

#define LOG_TAG "[DXCT-GM]"

// ========== Global State ==========
static JSContextRef gJSContext = NULL;
static JSGlobalContextRef gJSGlobal = NULL;
static FILE *gLog = NULL;
static pthread_mutex_t gCtxLock = PTHREAD_MUTEX_INITIALIZER;
static volatile int gInitDone = 0;

// GM flags
static volatile int gOneHitKill = 0;
static volatile int gGodMode  = 0;

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

// ========== GM Script (embedded) ==========
// The actual GM logic is in gm_script.js - we inject it when JSContext is ready
static const char *GM_SCRIPT_PATH_ENV = "DXCT_JS_FILE";

// ========== Native GM Overlay (UIScene/Aliases) ==========
static UIWindow *gGMWindow = NULL;
static UILabel  *gGMLable   = NULL;
static UISwitch *gKillSwitch = NULL;
static UISwitch *gGodSwitch  = NULL;
static UIButton *gCloseBtn   = nil;

static void createGMOverlay(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (gGMWindow) return;
        
        UIWindow *window = [[UIWindow alloc] initWithFrame:CGRectMake(0, 0, 200, 220)];
        window.windowLevel = UIWindowLevelAlert + 100;
        window.backgroundColor = [UIColor clearColor];
        window.userInteractionEnabled = YES;
        
        // Semi-transparent background
        UIView *bg = [[UIView alloc] initWithFrame:CGRectMake(10, 80, 180, 140)];
        bg.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.75];
        bg.layer.cornerRadius = 12;
        [window addSubview:bg];
        
        // Title label
        UILabel *title = [[UILabel alloc] initWithFrame:CGRectMake(10, 85, 180, 30)];
        title.text = @"大侠GM";
        title.textColor = [UIColor whiteColor];
        title.font = [UIFont boldSystemFontOfSize:16];
        title.textAlignment = NSTextAlignmentCenter;
        [window addSubview:title];
        
        // 秒杀 switch
        UILabel *killLabel = [[UILabel alloc] initWithFrame:CGRectMake(20, 120, 100, 30)];
        killLabel.text = @"🗡️ 秒杀";
        killLabel.textColor = [UIColor systemYellowColor];
        killLabel.font = [UIFont systemFontOfSize:15];
        [window addSubview:killLabel];
        
        UISwitch *killSwitch = [[UISwitch alloc] initWithFrame:CGRectMake(130, 122, 50, 30)];
        killSwitch.on = NO;
        [killSwitch addTarget:nil action:@selector(setOnHitKill:) animated:YES];
        [window addSubview:killSwitch];
        
        // 无敌 switch
        UILabel *godLabel = [[UILabel alloc] initWithFrame:CGRectMake(20, 155, 100, 30)];
        godLabel.text = @"🛡️ 无敌";
        godLabel.textColor = [UIColor systemGreenColor];
        godLabel.font = [UIFont systemFontOfSize:15];
        [window addSubview:godLabel];
        
        UISwitch *godSwitch = [[UISwitch alloc] initWithFrame:CGRectMake(130, 157, 50, 30)];
        godSwitch.on = NO;
        [window addSubview:godSwitch];
        
        // Status label
        UILabel *status = [[UILabel alloc] initWithFrame:CGRectMake(10, 195, 180, 25)];
        status.text = @"状态: 关闭";
        status.textColor = [UIColor lightGrayColor];
        status.font = [UIFont systemFontOfSize:11];
        status.textAlignment = NSTextAlignmentCenter;
        [window addSubview:status];
        
        gGMWindow = window;
        [window makeKeyAndVisible];
        dxct_log("Native GM overlay created");
    });
}

// ========== JS Context Capture via JSEvaluateScript hook ==========
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
            
            // Try to inject GM script
            char *jsPath = getenv(GM_SCRIPT_PATH_ENV);
            if (jsPath) {
                dxct_log("Injecting GM from: %s", jsPath);
                FILE *f = fopen(jsPath, "r");
                if (f) {
                    fseek(f, 0, SEEK_END);
                    long sz = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    char *buf = malloc(sz + 1);
                    if (buf) {
                        fread(buf, 1, sz, f);
                        buf[sz] = '\0';
                        fclose(f);
                        
                        JSStringRef jsStr = JSStringCreateWithUTF8CString(buf);
                        if (jsStr) {
                            JSValueRef exc = NULL;
                            JSValueRef result = JSEvaluateScript(ctx, jsStr, NULL, NULL, 1, &exc);
                            if (exc) {
                                JSStringRef excStr = JSValueToStringCopy(ctx, exc, NULL);
                                const char *msg = JSStringGetUTF8CString(excStr);
                                dxct_log("GM inject error: %s", msg ? msg : "unknown");
                                if (msg) JSStringRelease(excStr);
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
            }
            
            // Also embed a minimal GM script as fallback
            if (!jsPath) {
                dxct_log("No DXCT_JS_FILE set, using embedded GM");
                const char *embedded = 
                    "(function(){\n"
                    "  window.GM = {\n"
                    "    oneHitKill: false,\n"
                    "    godMode: false,\n"
                    "    enableOneHitKill: function(){ this.oneHitKill=true; console.log('[GM] 秒杀ON'); },\n"
                    "    enableGodMode:   function(){ this.godMode=true;  console.log('[GM] 无敌ON'); },\n"
                    "    disableAll:      function(){ this.oneHitKill=false; this.godMode=false; console.log('[GM] 全部关闭'); }\n"
                    "  };\n"
                    "})();\n";
                JSStringRef es = JSStringCreateWithUTF8CString(embedded);
                if (es) {
                    JSValueRef exc = NULL;
                    JSEvaluateScript(ctx, es, NULL, NULL, 1, &exc);
                    JSStringRelease(es);
                    dxct_log("Embedded GM injected");
                }
            }
        }
        pthread_mutex_unlock(&gCtxLock);
    }
    
    if (orig_JSEvaluateScript) {
        return orig_JSEvaluateScript(ctx, script, thisObject, sourceURL, lineNumber, exception);
    }
    return NULL;
}

// ========== Symbol Hook via fishhook-style rebinding ==========
struct rebindings_entry {
    void *symbol;
    void *replacement;
    void **original;
};

static struct rebindings_entry _rebindings[1];
static unsigned int _nrebindings;

static int rebind_symbols(struct rebindings_entry *rebindings, unsigned int nargs) {
    Dl_info info;
    if (dladdr((void *)hooked_JSEvaluateScript, &info) == 0) return -1;
    
    uintptr_t start = (uintptr_t)info.dli_fbase;
    Dl_info symInfo;
    void *handle = dlopen("JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
    if (!handle) {
        handle = dlopen("/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
    }
    if (!handle) {
        dxct_log("dlopen JavaScriptCore failed: %s", dlerror());
        return -1;
    }
    
    void *sym = dlsym(handle, "JSEvaluateScript");
    dlclose(handle);
    if (!sym) {
        dxct_log("dlsym JSEvaluateScript failed");
        return -1;
    }
    
    // Make the page writable
    mach_vm_address_t addr = (mach_vm_address_t)sym;
    mach_vm_size_t size = sizeof(void *);
    vm_prot_t old_protection;
    if (mach_vm_protect(mach_task_self(), addr, size, FALSE, VM_PROT_READ|VM_PROT_WRITE) != KERN_SUCCESS) {
        dxct_log("mach_vm_protect failed");
        return -1;
    }
    
    // Save original and install replacement
    *(void **)rebindings->original = (void *)addr;
    *(void **)addr = (void *)rebindings->replacement;
    
    // Restore original protections
    mach_vm_protect(mach_task_self(), addr, size, FALSE, VM_PROT_READ|VM_PROT_EXECUTE);
    
    dxct_log("Symbol rebinding installed: JSEvaluateScript -> hooked_JSEvaluateScript");
    return 0;
}

// ========== dylib entry point ==========
__attribute__((constructor))
static void dxct_gm_init(void) {
    // Check enable flag
    const char *enable = getenv("DXCT_ENABLE");
    if (!enable || strcmp(enable, "1") != 0) {
        return;
    }
    
    dxct_log("dxctx_gm.dylib loading... (DXCT_ENABLE=1)");
    
    // Setup rebindings
    _rebindings[0].symbol   = (void *)hooked_JSEvaluateScript;
    _rebindings[0].replacement = (void *)hooked_JSEvaluateScript;
    _rebindings[0].original  = (void **)&orig_JSEvaluateScript;
    _nrebindings = 1;
    
    if (rebind_symbols(_rebindings, _nrebindings) == 0) {
        dxct_log("Hook installed successfully!");
    } else {
        dxct_log("Hook installation FAILED, falling back to dlsym method");
        // Fallback: try dlsym directly
        void *jc = dlopen("/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
        if (jc) {
            orig_JSEvaluateScript = (JSEvaluateScriptFn)dlsym(jc, "JSEvaluateScript");
            dxct_log("Found JSEvaluateScript via dlsym: %p", orig_JSEvaluateScript);
        }
    }
    
    // Create native overlay after a delay
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(5.0 * NSEC_PER_SEC)), 
                   dispatch_get_main_queue(), ^{
        createGMOverlay();
    });
}

__attribute__((destructor))
static void dxct_gm_fini(void) {
    if (gJSContext) {
        CFRelease(gJSContext);
        gJSContext = NULL;
    }
    if (gLog) {
        fclose(gLog);
        gLog = NULL;
    }
    dxct_log("dylib unloaded");
}

// ========== Getter functions for JS bridge ==========
// These can be called from JavaScript via reflection or direct access
extern int get_one_hit_kill(void) { return gOneHitKill; }
extern int get_god_mode(void)   { return gGodMode; }
extern void set_one_hit_kill(int v) { gOneHitKill = v; dxct_log("oneHitKill=%d", v); }
extern void set_god_mode(int v)   { gGodMode = v;   dxct_log("godMode=%d", v); }
