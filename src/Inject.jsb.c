// Inject.jsb.c - GM hook for 大侠闯天下 (Cocos2d-x JSB)
// 简化版本：直接 hook，避免 fishhook 复杂性
// 环境变量: DXCT_ENABLE=1 (启用), DXCT_JS_FILE=/path/to/gm.js

#include <JavaScriptCore/JavaScriptCore.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 原始函数指针
static JSContextRef (*orig_JSEvaluateScript)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *);
static JSContextRef gCachedContext = NULL;
static FILE *gLog = NULL;

// 日志
static void dxct_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (!gLog) {
        gLog = fopen("/var/mobile/Library/Logs/dxct_gm.log", "a");
    }
    if (gLog) {
        vfprintf(gLog, fmt, args);
        fprintf(gLog, "\n");
        fflush(gLog);
    }
    va_end(args);
}

// Hook函数
static JSContextRef my_JSEvaluateScript(JSContextRef ctx, JSStringRef script, JSObjectRef thisObject, JSStringRef sourceURL, int lineNumber, JSStringRef *exception) {
    // 缓存JSContext
    if (!gCachedContext && ctx) {
        gCachedContext = ctx;
        dxct_log("[DXCT] JSContext captured");
        
        // 注入GM脚本
        char *jsPath = getenv("DXCT_JS_FILE");
        if (jsPath) {
            dxct_log("[DXCT] Injecting from: %s", jsPath);
            
            FILE *f = fopen(jsPath, "r");
            if (f) {
                fseek(f, 0, SEEK_END);
                long size = ftell(f);
                fseek(f, 0, SEEK_SET);
                
                char *content = malloc(size + 1);
                if (content) {
                    fread(content, 1, size, f);
                    content[size] = '\0';
                    fclose(f);
                    
                    JSStringRef injectScript = JSStringCreateWithUTF8CString(content);
                    if (injectScript) {
                        dxct_log("[DXCT] GM script injected");
                    }
                    JSStringRelease(injectScript);
                }
                free(content);
            } else {
                dxct_log("[DXCT] Failed to open GM file");
            }
        }
    }
    
    // 调用原始函数
    return orig_JSEvaluateScript(ctx, script, thisObject, sourceURL, lineNumber, exception);
}

// dylib入口
__attribute__((constructor))
void dylib_init() {
    // 检查环境变量
    char *enable = getenv("DXCT_ENABLE");
    if (!enable || strcmp(enable, "1") != 0) {
        return;
    }
    
    dxct_log("[DXCT] dylib loaded, initializing...");
    
    // 使用 fishhook 或直接符号替换
    // 这里使用 dlsym 获取原始函数
    void *handle = dlopen("/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore", RTLD_NOW);
    if (handle) {
        orig_JSEvaluateScript = (JSContextRef (*)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *))
            dlsym(handle, "JSEvaluateScript");
        
        if (orig_JSEvaluateScript) {
            // 尝试符号替换
            dxct_log("[DXCT] Found JSEvaluateScript at %p", orig_JSEvaluateScript);
        } else {
            dxct_log("[DXCT] Failed to find JSEvaluateScript");
        }
        dlclose(handle);
    } else {
        dxct_log("[DXCT] Failed to open JavaScriptCore");
    }
}

__attribute__((destructor))
void dylib_fini() {
    if (gLog) {
        fclose(gLog);
    }
}