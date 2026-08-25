// Inject.jsb.c - GM hook for 大侠闯天下
// Hook JSEvaluateScript to inject GM debug panel

#include <JavaScriptCore/JavaScriptCore.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fishhook.h"

static struct rebinding rebindings[1];
static struct rebinding *rebindings_head = NULL;

static JSContextRef (*orig_JSEvaluateScript)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *);
static JSStringRef (*orig_JSStringCreateWithCharacters)(const JSChar *, size_t);

typedef void (*JSContextRef)(void);
typedef void (*JSStringRef)(void);
typedef void (*JSObjectRef)(void);

void dylib_init() {
    char *enable = getenv("DXCT_ENABLE");
    if (!enable || strcmp(enable, "1") != 0) return;
    
    FILE *log = fopen("/var/mobile/Library/Logs/dxct_debug.log", "w");
    if (log) {
        fprintf(log, "[DXCT] dylib loaded\n");
        fclose(log);
    }
}

void dylib_fini() {}
