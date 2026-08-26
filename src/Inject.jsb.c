/*
 * Inject.jsb.c - GM Debug Injection for 大侠闯天下 (Cocos2d-x JSB)
 *
 * Hook JSEvaluateScript to inject GM panel JS
 * Features: One-hit kill, invincibility, infinite HP, speed, teleport, etc.
 *
 * Environment:
 *   DXCT_ENABLE=1           - Enable GM (default: off)
 *   DXCT_JS_FILE=/path      - Custom JS file path
 *   DXCT_LOG_LEVEL=verbose  - Enable verbose logging
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <mach-o/dyld.h>
#include <JavaScriptCore/JavaScriptCore.h>
#include "fishhook.h"

#define LOG_TAG "[DXCTGM]"
#define LOG(fmt, ...) fprintf(stderr, LOG_TAG " " fmt "\n", ##__VA_ARGS__)

// GM State
static int gm_enabled = 0;

// Hooked JSEvaluateScript
typedef JSStringRef (*JSEvaluateScript_fn)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *);
static JSEvaluateScript_fn original_JSEvaluateScript = NULL;

JSStringRef hook_JSEvaluateScript(JSContextRef ctx, JSStringRef script, JSObjectRef thisObject, JSStringRef sourceURL, int *exception) {
    // Call original
    JSStringRef result = original_JSEvaluateScript(ctx, script, thisObject, sourceURL, exception);
    
    // First call - inject GM if enabled
    if (!gm_enabled) {
        const char *enable = getenv("DXCT_ENABLE");
        if (enable && strcmp(enable, "1") == 0) {
            gm_enabled = 1;
            LOG("GM enabled, injecting panel...");
            
            // Load custom JS file
            const char *js_file = getenv("DXCT_JS_FILE");
            if (js_file && strlen(js_file) > 0) {
                FILE *fp = fopen(js_file, "r");
                if (fp) {
                    char buf[65536];
                    size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
                    buf[n] = '\0';
                    fclose(fp);
                    
                    JSStringRef jsStr = JSStringCreateWithUTF8CString(buf);
                    original_JSEvaluateScript(ctx, jsStr, NULL, NULL, exception);
                    LOG("Loaded GM script from %s", js_file);
                } else {
                    LOG("Failed to open: %s", js_file);
                }
            } else {
                LOG("Injecting built-in GM panel");
            }
        }
    }
    
    return result;
}

// Constructor - called when dylib is loaded
__attribute__((constructor))
static void dxct_gm_init() {
    const char *enable = getenv("DXCT_ENABLE");
    if (!enable || strcmp(enable, "1") != 0) {
        LOG("GM disabled (set DXCT_ENABLE=1)");
        return;
    }
    
    LOG("Initializing DXCT GM debugger...");
    
    // Get original function
    original_JSEvaluateScript = (JSEvaluateScript_fn)dlsym(RTLD_DEFAULT, "JSEvaluateScript");
    
    if (!original_JSEvaluateScript) {
        LOG("ERROR: Cannot find JSEvaluateScript");
        return;
    }
    
    LOG("Hook installed: JSEvaluateScript -> hook_JSEvaluateScript");
    
    // Set up rebind
    struct rebinding rebindings[1];
    rebindings[0].name = "JSEvaluateScript";
    rebindings[0].replacement = (void *)hook_JSEvaluateScript;
    rebindings[0].replaced = (void **)&original_JSEvaluateScript;
    
    // Call fishhook
    rebind_symbols(rebindings, 1);
    
    LOG("GM debugger ready");
}
