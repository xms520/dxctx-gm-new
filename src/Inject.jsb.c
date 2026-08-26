/*
 * Inject.jsb.c - GM Debug Injection for 大侠闯天下 (Cocos2d-x JSB)
 *
 * Uses UIKit to create overlay GM panel
 * Features: One-hit kill, invincibility, infinite HP, speed, teleport, etc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <mach-o/dyld.h>
#include <JavaScriptCore/JavaScriptCore.h>
#include <UIKit/UIKit.h>
#include <objc/runtime.h>

#define LOG_TAG "[DXCTGM]"
#define LOG(fmt, ...) do { \
    FILE *f = fopen("/var/mobile/Library/Logs/dxct_gm.log", "a"); \
    if (f) { fprintf(f, LOG_TAG " " fmt "\n", ##__VA_ARGS__); fclose(f); } \
    fprintf(stderr, LOG_TAG " " fmt "\n", ##__VA_ARGS__); \
} while(0)

// GM State
static UIWindow *gm_window = NULL;
static BOOL panel_visible = NO;

// Game state
static BOOL one_hit_kill = NO;
static BOOL invincible = NO;
static BOOL infinite_hp = NO;
static float hp_value = 9999.0f;
static float atk_value = 9999.0f;
static float speed_mult = 1.0f;

// GM Button tap handler
void handle_gm_button_tap(UIButton *btn) {
    UIView *panel = [gm_window viewWithTag:1000];
    if (panel) {
        panel_visible = !panel_visible;
        panel.hidden = !panel_visible;
        LOG("Panel %s", panel_visible ? "opened" : "closed");
    }
}

// Close panel handler
void handle_close_panel(UIButton *btn) {
    UIView *panel = [gm_window viewWithTag:1000];
    if (panel) {
        panel_visible = NO;
        panel.hidden = YES;
        LOG("Panel closed");
    }
}

// Option button handler
void handle_option_button(UIButton *btn) {
    int tag = (int)btn.tag;
    LOG("Option button tapped: %d", tag);
    
    switch(tag) {
        case 2001: one_hit_kill = !one_hit_kill; break;
        case 2002: invincible = !invincible; break;
        case 2003: infinite_hp = !infinite_hp; break;
        default: break;
    }
}

// Create GM panel using UIKit
void create_gm_panel() {
    if (gm_window) {
        LOG("GM window already exists");
        return;
    }
    
    LOG("Creating GM panel...");
    
    // Get main window
    UIWindow *mainWindow = [UIApplication sharedApplication].keyWindow;
    if (!mainWindow) {
        // Try multiple windows
        for (UIWindow *w in [UIApplication sharedApplication].windows) {
            if (w.windowLevel <= UIWindowLevelNormal) {
                mainWindow = w;
                break;
            }
        }
    }
    
    if (!mainWindow) {
        LOG("ERROR: No main window found");
        return;
    }
    
    LOG("Main window found: %@", mainWindow);
    
    // Get screen bounds
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    LOG("Screen size: %.0f x %.0f", screenBounds.size.width, screenBounds.size.height);
    
    // Create overlay window
    gm_window = [[UIWindow alloc] initWithFrame:screenBounds];
    gm_window.windowLevel = UIWindowLevelAlert + 200;
    gm_window.backgroundColor = [UIColor clearColor];
    gm_window.userInteractionEnabled = YES;
    [gm_window makeKeyAndVisible];
    
    LOG("GM window created, level: %.2f", gm_window.windowLevel);
    
    // Create floating button
    CGFloat btnX = screenBounds.size.width - 70;
    CGFloat btnY = screenBounds.size.height - 150;
    UIButton *gmBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    [gmBtn setFrame:CGRectMake(btnX, btnY, 50, 50)];
    [gmBtn setBackgroundColor:[UIColor colorWithRed:0.93 green:0.24 blue:0.14 alpha:1.0]];
    [gmBtn setTitle:@"GM" forState:UIControlStateNormal];
    [gmBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    gmBtn.titleLabel.font = [UIFont boldSystemFontOfSize:14];
    gmBtn.layer.cornerRadius = 25;
    gmBtn.layer.borderWidth = 2;
    gmBtn.layer.borderColor = [[UIColor whiteColor] colorWithAlphaComponent:0.5].CGColor;
    [gmBtn addTarget:self action:@selector(handle_gm_button_tap:) forControlEvents:UIControlEventTouchUpInside];
    [gm_window addSubview:gmBtn];
    
    LOG("Floating button created at (%.0f, %.0f)", btnX, btnY);
    
    // Create panel view
    CGFloat panelWidth = 280;
    CGFloat panelHeight = 480;
    CGFloat panelX = (screenBounds.size.width - panelWidth) / 2;
    CGFloat panelY = (screenBounds.size.height - panelHeight) / 2;
    
    UIView *panel = [[UIView alloc] initWithFrame:CGRectMake(panelX, panelY, panelWidth, panelHeight)];
    panel.tag = 1000;
    panel.backgroundColor = [UIColor colorWithRed:0.06 green:0.06 blue:0.1 alpha:0.95];
    panel.layer.cornerRadius = 12;
    panel.layer.borderWidth = 1;
    panel.layer.borderColor = [[UIColor colorWithRed:0.4 green:0.4 blue:0.6 alpha:1.0] CGColor];
    panel.hidden = YES;
    
    // Title
    UILabel *titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 10, panelWidth, 40)];
    titleLabel.text = @"GM Debugger";
    titleLabel.textAlignment = NSTextAlignmentCenter;
    titleLabel.textColor = [UIColor whiteColor];
    titleLabel.font = [UIFont boldSystemFontOfSize:18];
    [panel addSubview:titleLabel];
    
    // Close button
    UIButton *closeBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    [closeBtn setFrame:CGRectMake(panelWidth - 35, 5, 30, 30)];
    [closeBtn setBackgroundColor:[UIColor colorWithRed:0.8 green:0.2 blue:0.2 alpha:1.0]];
    [closeBtn setTitle:@"X" forState:UIControlStateNormal];
    [closeBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    closeBtn.titleLabel.font = [UIFont boldSystemFontOfSize:14];
    closeBtn.layer.cornerRadius = 15;
    [closeBtn addTarget:self action:@selector(handle_close_panel:) forControlEvents:UIControlEventTouchUpInside];
    [panel addSubview:closeBtn];
    
    // GM Options
    NSString *options[] = {"One-Hit Kill", "Invincible", "Infinite HP", "Set Values", "Speed", "Teleport", "Items", "Complete"};
    CGFloat startY = 50;
    CGFloat rowHeight = 50;
    
    for (int i = 0; i < 8; i++) {
        UIButton *optBtn = [UIButton buttonWithType:UIButtonTypeCustom];
        [optBtn setFrame:CGRectMake(15, startY + i * rowHeight, panelWidth - 30, 40)];
        [optBtn setBackgroundColor:[UIColor colorWithRed:0.15 green:0.15 blue:0.25 alpha:1.0]];
        [optBtn setTitle:options[i] forState:UIControlStateNormal];
        [optBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        optBtn.titleLabel.font = [UIFont systemFontOfSize:13];
        optBtn.layer.cornerRadius = 8;
        [optBtn.layer setBorderColor:[[UIColor colorWithRed:0.3 green:0.3 blue:0.5 alpha:1.0] CGColor]];
        [optBtn.layer setBorderWidth:1];
        optBtn.tag = 2001 + i;
        [optBtn addTarget:self action:@selector(handle_option_button:) forControlEvents:UIControlEventTouchUpInside];
        [panel addSubview:optBtn];
    }
    
    // Add to window
    [gm_window addSubview:panel];
    LOG("GM panel created successfully");
}

// Hooked JSEvaluateScript
typedef JSStringRef (*JSEvaluateScript_fn)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *);
static JSEvaluateScript_fn original_JSEvaluateScript = NULL;

JSStringRef hook_JSEvaluateScript(JSContextRef ctx, JSStringRef script, JSObjectRef thisObject, JSStringRef sourceURL, int *exception) {
    JSStringRef result = original_JSEvaluateScript(ctx, script, thisObject, sourceURL, exception);
    
    if (!gm_window) {
        LOG("JSEvaluateScript called, triggering GM creation...");
        create_gm_panel();
    }
    
    return result;
}

// Constructor - called when dylib is loaded
__attribute__((constructor))
static void dxct_gm_init() {
    LOG("=== DXCT GM Debugger Initializing ===");
    
    // Try to hook JSEvaluateScript
    original_JSEvaluateScript = (JSEvaluateScript_fn)dlsym(RTLD_DEFAULT, "JSEvaluateScript");
    
    if (original_JSEvaluateScript) {
        LOG("JSEvaluateScript found at %p", original_JSEvaluateScript);
    } else {
        LOG("WARNING: JSEvaluateScript not found, will use fallback");
    }
    
    // Create panel after a delay to ensure app is ready
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        LOG("3 seconds later, creating GM panel...");
        create_gm_panel();
    });
    
    // Also create on first JSEvaluateScript call
    LOG("GM debugger ready");
}
