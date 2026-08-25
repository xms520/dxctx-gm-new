// dxctx_gm.c - 大侠闯天下 GM调试注入
// 注入机制: 通过fishhook hook JSEvaluateScript
// 使用方式: DXCT_ENABLE=1 DXCT_JS_FILE=/path/to/gm.js

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mach-o/dyld.h>
#include <JavaScriptCore/JavaScriptCore.h>
#include "fishhook.h"

#define LOG_TAG "[DXCTGM] "
#define LOG(fmt, ...) printf(LOG_TAG fmt "\n", ##__VA_ARGS__)

// Hook state
static JSContextRef (*orig_JSEvaluateScript)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSValueRef*) = NULL;
static int g_injected = 0;
static char g_js_file_path[512] = {0};

// Read file content
static char* read_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, size, f);
    buf[n] = '\0';
    fclose(f);
    if (out_size) *out_size = n;
    return buf;
}

// Hook: JSEvaluateScript
static JSContextRef dxctx_JSEvaluateScript_hook(
    JSContextRef ctx,
    JSStringRef script,
    JSObjectRef object,
    JSStringRef source_url,
    int starting_line,
    JSValueRef* exception) {
    
    // First call: inject GM script
    if (!g_injected) {
        g_injected = 1;
        
        // Get global object
        JSObjectRef globals = JSContextGetGlobalObject(ctx);
        
        // Try to load custom JS file
        if (g_js_file_path[0]) {
            size_t size = 0;
            char* js_content = read_file(g_js_file_path, &size);
            if (js_content) {
                JSStringRef js_str = JSStringCreateWithUTF8CString(js_content);
                JSValueRef exc = NULL;
                JSContextEva la teScript(ctx, js_str, NULL, NULL, 0, &exc);
                JSStringRelease(js_str);
                free(js_content);
                
                if (exc && !JSValueIsUndefined(ctx, exc)) {
                    JSStringRef exc_str = JSValueToStringCopy(ctx, exc, &exc);
                    const char* msg = JSStringGetUTF8CString(exc_str);
                    LOG("Failed to load GM script: %s", msg ? msg : "unknown");
                    if (msg) free((void*)msg);
                    JSStringRelease(exc_str);
                } else {
                    LOG("GM script loaded from: %s", g_js_file_path);
                }
            } else {
                LOG("Failed to open GM script: %s", g_js_file_path);
            }
        }
    }
    
    // Call original function
    if (orig_JSEvaluateScript) {
        return orig_JSEvaluateScript(ctx, script, object, source_url, starting_line, exception);
    }
    return NULL;
}

// Constructor
__attribute__((constructor))
static void dxctx_gm_init() {
    // Check if injection is enabled
    const char* enable = getenv("DXCT_ENABLE");
    if (!enable || strcmp(enable, "1") != 0) {
        LOG("DXCT GM injection disabled (set DXCT_ENABLE=1)");
        return;
    }
    
    // Get custom JS file path
    const char* js_file = getenv("DXCT_JS_FILE");
    if (js_file) {
        strncpy(g_js_file_path, js_file, sizeof(g_js_file_path) - 1);
        LOG("Using GM script: %s", g_js_file_path);
    } else {
        // Default path in game sandbox
        strncpy(g_js_file_path, "/var/mobile/Library/MobileDevice/Provisioning Applications/com.tencent.xin/Documents/dxctx_gm.js", sizeof(g_js_file_path) - 1);
        LOG("Using default GM script path: %s", g_js_file_path);
    }
    
    // Rebind JSEvaluateScript using fishhook
    struct rebinding rebindings[] = {
        {"JSEvaluateScript", (void*)dxctx_JSEvaluateScript_hook, (void**)&orig_JSEvaluateScript},
        {NULL, NULL, NULL}
    };
    
    int result = rebind_symbols(rebindings, sizeof(rebindings)/sizeof(rebindings[0]) - 1);
    if (result == 0) {
        LOG("JSEvaluateScript hook installed successfully!");
    } else {
        LOG("Failed to install hook (result=%d)", result);
    }
}
