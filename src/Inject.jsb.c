// Inject.jsb.c - GM hook for 大侠闯天下
#include <JavaScriptCore/JavaScriptCore.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fishhook.h"

static struct rebinding rebindings[] = {
  {"JSEvaluateScript", (void*)my_JSEvaluateScript, (void**)&orig_JSEvaluateScript},
};

static JSContextRef (*orig_JSEvaluateScript)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *);

void dylib_init() {
    char *enable = getenv("DXCT_ENABLE");
    if (!enable || strcmp(enable, "1") != 0) return;
    
    FILE *log = fopen("/var/mobile/Library/Logs/dxct_debug.log", "w");
    if (log) {
        fprintf(log, "[DXCT] dylib loaded, injecting...\n");
        fclose(log);
    }
    
    rebind_symbols(rebindings, sizeof(rebindings)/sizeof(rebindings[0]));
}

void dylib_fini() {}

static JSContextRef my_JSEvaluateScript(JSContextRef ctx, JSStringRef script, JSObjectRef thisObject, JSStringRef sourceURL, int lineNumber, JSStringRef *exception) {
    const char *jsPath = getenv("DXCT_JS_FILE");
    if (jsPath && ctx) {
        FILE *log = fopen("/var/mobile/Library/Logs/dxct_debug.log", "a");
        if (log) {
            fprintf(log, "[DXCT] JSEvaluateScript called, injecting GM\n");
            fclose(log);
        }
        
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
                
                JSStringRef scriptStr = JSStringCreateWithUTF8CString(content);
                if (scriptStr) {
                    orig_JSEvaluateScript(ctx, scriptStr, thisObject, NULL, 0, exception);
                    JSStringRelease(scriptStr);
                }
                
                free(content);
            } else {
                fclose(f);
            }
        }
    }
    
    return orig_JSEvaluateScript(ctx, script, thisObject, sourceURL, lineNumber, exception);
}
