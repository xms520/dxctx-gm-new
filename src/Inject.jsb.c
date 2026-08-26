#include <dlfcn.h>
#include <UIKit/UIKit.h>
#include <stdio.h>
#include <mach-o/dyld.h>

static void* orig_JSEvaluateScript = NULL;

typedef JSValueRef (*JSEvaluateScriptFunc)(JSContextRef, JSStringRef, JSSourceURLRef, int, JSValueRef*, unsigned*);

static JSValueRef dxct_JSEvaluateScript(JSContextRef ctx, JSStringRef script, JSSourceURLRef url, int* exceptionLine, JSValueRef* exception, unsigned* sourceLength) {
    static int initialized = 0;
    if (!initialized) {
        initialized = 1;
        
        dispatch_async(dispatch_get_main_queue(), ^{
            UIWindow* window = [[UIWindow alloc] initWithFrame:CGRectMake(0, 0, 320, 56)];
            window.windowLevel = UIWindowLevelAlert + 100;
            window.backgroundColor = [UIColor clearColor];
            
            UIButton* button = [UIButton buttonWithType:UIButtonTypeSystem];
            [button setFrame:CGRectMake(0, 0, 56, 56)];
            [button setBackgroundColor:[UIColor redColor]];
            [button.layer setCornerRadius:28.0];
            [button.layer setMasksToBounds:YES];
            [button.layer setBorderWidth:2.0];
            [button.layer setBorderColor:[[UIColor whiteColor] CGColor]];
            [button setTitle:@"GM" forState:UIControlStateNormal];
            [[button titleLabel] setFont:[UIFont systemFontOfSize:16 weight:UIFontWeightBold]];
            [[button titleLabel] setTextColor:[UIColor whiteColor]];
            
            [button addTarget:nil action:@selector(touchUpInside) forControlEvents:UIControlEventTouchUpInside];
            
            [window addSubview:button];
            [window makeKeyAndVisible];
            
            // Also add to main window
            UIApplication* app = [UIApplication sharedApplication];
            UIWindow* main = app.keyWindow ? app.keyWindow : app.windows[0];
            if (main) {
                UIView* container = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 60, 60)];
                container.userInteractionEnabled = YES;
                [main addSubview:container];
                [container addSubview:button];
                [button removeFromSuperview];
                [container addSubview:button];
                [button setFrame:CGRectMake(0, 0, 60, 60)];
            }
        });
    }
    return ((JSEvaluateScriptFunc)orig_JSEvaluateScript)(ctx, script, url, exceptionLine, exception, sourceLength);
}

__attribute__((constructor))
static void dxct_init() {
    orig_JSEvaluateScript = dlsym(RTLD_DEFAULT, "JSEvaluateScript");
    if (orig_JSEvaluateScript) {
        struct rebinding rebindings[] = {
            {"JSEvaluateScript", dxct_JSEvaluateScript, &orig_JSEvaluateScript},
            {NULL, NULL, NULL}
        };
        rebind_symbols(rebindings, 1);
    }
    
    FILE* f = fopen("/var/mobile/Library/Logs/dxct_gm.log", "w");
    if (f) {
        fprintf(f, "[DXCT] GM initialized\n");
        fclose(f);
    }
}
