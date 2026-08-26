// Inject.jsb.c - GM hook for 大侠闯天下
// Hook JSEvaluateScript to inject GM debug panel

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