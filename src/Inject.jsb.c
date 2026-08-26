// Inject.jsb.c - GM hook for 大侠闯天下 (Cocos2d-x JSB)
// 通过 fishhook hook JSEvaluateScript 注入 GM 脚本
// 环境变量: DXCT_ENABLE=1 (启用), DXCT_JS_FILE=/path/to/gm.js (自定义JS)

#include <JavaScriptCore/JavaScriptCore.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "fishhook.h"

// 全局状态
static JSContextRef gContext = NULL;
static int gContextCaptured = 0;
static int gGMInjected = 0;

// 原始函数指针
static JSContextRef (*orig_JSEvaluateScript)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *);

// 日志文件
static FILE *gLogFile = NULL;

static void dxct_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    if (!gLogFile) {
        gLogFile = fopen("/var/mobile/Library/Logs/dxct_gm.log", "a");
    }
    if (gLogFile) {
        vfprintf(gLogFile, fmt, args);
        fflush(gLogFile);
    }
    
    va_end(args);
}

// 读取文件
static char *read_file(const char *path) {
    if (!path) return NULL;
    
    FILE *f = fopen(path, "r");
    if (!f) {
        dxct_log("[DXCT] Failed to open: %s\n", path);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *content = (char *)malloc(size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }
    
    size_t n = fread(content, 1, size, f);
    content[n] = '\0';
    fclose(f);
    
    dxct_log("[DXCT] Read %ld bytes from %s\n", (long)n, path);
    return content;
}

// 实际注入函数
static void do_inject(JSContextRef ctx, const char *jsPath) {
    if (!ctx || !jsPath || !jsPath[0]) return;
    
    dxct_log("[DXCT] Injecting GM script...\n");
    
    char *content = read_file(jsPath);
    if (!content) {
        dxct_log("[DXCT] Failed to read GM script\n");
        return;
    }
    
    JSStringRef scriptStr = JSStringCreateWithUTF8CString(content);
    if (!scriptStr) {
        dxct_log("[DXCT] Failed to create JSString\n");
        free(content);
        return;
    }
    
    JSStringRef sourceURL = JSStringCreateWithUTF8CString("dxct_gm://main");
    JSValueRef exception = NULL;
    
    JSValueRef result = JSEvaluateScript(ctx, scriptStr, NULL, sourceURL, 0, &exception);
    
    if (exception) {
        JSStringRef excStr = JSValueToStringCopy(ctx, exception, NULL);
        if (excStr) {
            const char *excChar = JSStringGetCStringPtr(excStr, kCFStringEncodingUTF8);
            dxct_log("[DXCT] Script error: %s\n", excChar ? excChar : "unknown");
            JSStringRelease(excStr);
        }
    } else {
        dxct_log("[DXCT] GM script injected successfully!\n");
    }
    
    JSStringRelease(sourceURL);
    JSStringRelease(scriptStr);
    free(content);
}

// Hook JSEvaluateScript
static JSContextRef my_JSEvaluateScript(JSContextRef ctx, JSStringRef script, JSObjectRef thisObject, JSStringRef sourceURL, int lineNumber, JSStringRef *exception) {
    // 捕获第一个有效的 context
    if (!gContextCaptured && ctx) {
        gContext = ctx;
        gContextCaptured = 1;
        dxct_log("[DXCT] JSContext captured via JSEvaluateScript: %p\n", (void *)ctx);
        
        // 尝试注入
        const char *jsPath = getenv("DXCT_JS_FILE");
        if (jsPath && jsPath[0] != '\0') {
            do_inject(ctx, jsPath);
        } else {
            do_inject(ctx, "/var/mobile/Library/Playgrounds/dxct_gm.js");
        }
    }
    
    return orig_JSEvaluateScript(ctx, script, thisObject, sourceURL, lineNumber, exception);
}

// dylib 初始化
__attribute__((constructor))
static void dylib_init() {
    // 检查启用标志
    char *enable = getenv("DXCT_ENABLE");
    if (!enable || strcmp(enable, "1") != 0) {
        dxct_log("[DXCT] DXCT_ENABLE not set, exiting\n");
        return;
    }
    
    dxct_log("[DXCT] ========================================\n");
    dxct_log("[DXCT] DXCT GM Hook initializing...\n");
    dxct_log("[DXCT] ========================================\n");
    
    // 记录环境变量
    const char *jsFile = getenv("DXCT_JS_FILE");
    dxct_log("[DXCT] DXCT_JS_FILE=%s\n", jsFile ? jsFile : "(using default)");
    
    // 安装 fishhook
    struct rebinding rebindings[] = {
        {"JSEvaluateScript", (void *)my_JSEvaluateScript, (void **)&orig_JSEvaluateScript},
    };
    
    int result = rebind_symbols(rebindings, sizeof(rebindings) / sizeof(rebindings[0]));
    if (result == 0) {
        dxct_log("[DXCT] fishhook installed successfully!\n");
        dxct_log("[DXCT] JSEvaluateScript original: %p\n", (void *)orig_JSEvaluateScript);
    } else {
        dxct_log("[DXCT] fishhook failed: %d\n", result);
    }
    
    dxct_log("[DXCT] dylib_init complete\n");
}

__attribute__((destructor))
static void dylib_fini() {
    dxct_log("[DXCT] Unloading...\n");
    if (gLogFile) {
        fclose(gLogFile);
        gLogFile = NULL;
    }
}
