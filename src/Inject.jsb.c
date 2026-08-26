/*
 * Inject.jsb.c - GM Debug Injection for 大侠闯天下 (Cocos2d-x JSB)
 *
 * Uses UIKit to create overlay GM panel (bypasses Cocos2d-x DOM limitation)
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
#include "fishhook.h"

#define LOG_TAG "[DXCTGM]"
#define LOG(fmt, ...) fprintf(stderr, LOG_TAG " " fmt "\n", ##__VA_ARGS__)

// GM State
static int gm_enabled = 0;
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
    if (gm_window) return;
    
    // Get screen bounds
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    
    // Create overlay window
    gm_window = [[UIWindow alloc] initWithFrame:screenBounds];
    gm_window.windowLevel = UIWindowLevelAlert + 100;
    gm_window.backgroundColor = [UIColor clearColor];
    gm_window.userInteractionEnabled = YES;
    
    [gm_window makeKeyAndVisible];
    LOG("GM window created, level: %.2f", gm_window.windowLevel);
    
    // Create floating button
    CGFloat btnX = screenBounds.size.width - 65;
    CGFloat btnY = screenBounds.size.height - 130;
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
        // Button
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
    
    if (!gm_enabled) {
        gm_enabled = 1;
        LOG("First JSEvaluateScript call, creating GM panel...");
        dispatch_async(dispatch_get_main_queue(), ^{
            create_gm_panel();
        });
    }
    
    return result;
}

// Constructor
__attribute__((constructor))
static void dxct_gm_init() {
    LOG("Initializing DXCT GM debugger...");
    
    // Create panel immediately (fallback)
    dispatch_async(dispatch_get_main_queue(), ^{
        create_gm_panel();
    });
    
    // Try to hook JSEvaluateScript
    original_JSEvaluateScript = (JSEvaluateScript_fn)dlsym(RTLD_DEFAULT, "JSEvaluateScript");
    
    if (original_JSEvaluateScript) {
        LOG("Hooking JSEvaluateScript");
        struct rebinding rebindings[1];
        rebindings[0].name = "JSEvaluateScript";
        rebindings[0].replacement = (void *)hook_JSEvaluateScript;
        rebindings[0].replaced = (void **)&original_JSEvaluateScript;
        rebind_symbols(rebindings, 1);
    }
    
    LOG("GM debugger ready");
}
