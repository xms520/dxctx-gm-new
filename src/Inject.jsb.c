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
#include <fcntl.h>
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

// Create GM panel using UIKit
void create_gm_panel() {
    if (gm_window) return;
    
    // Get key window
    UIWindow *keyWindow = [UIApplication sharedApplication].keyWindow;
    if (!keyWindow) {
        // Fallback to main screen
        UIScreen *screen = [UIScreen mainScreen];
        CGRect bounds = screen.bounds;
        keyWindow = [[UIWindow alloc] initWithFrame:bounds];
        keyWindow.windowLevel = UIWindowLevelAlert + 100;
        [keyWindow makeKeyAndVisible];
    }
    
    // Create overlay window
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    gm_window = [[UIWindow alloc] initWithFrame:screenBounds];
    gm_window.windowLevel = UIWindowLevelAlert + 200;
    gm_window.backgroundColor = [UIColor clearColor];
    gm_window.userInteractionEnabled = YES;
    
    [gm_window makeKeyAndVisible];
    
    // Create floating button
    CGFloat btnX = screenBounds.size.width - 70;
    CGFloat btnY = screenBounds.size.height - 120;
    UIButton *gmBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    [gmBtn setFrame:CGRectMake(btnX, btnY, 56, 56)];
    [gmBtn setBackgroundColor:[UIColor colorWithRed:0.93 green:0.24 blue:0.14 alpha:1.0]];
    [gmBtn setTitle:@"GM" forState:UIControlStateNormal];
    [gmBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    gmBtn.titleLabel.font = [UIFont boldSystemFontOfSize:16];
    gmBtn.layer.cornerRadius = 28;
    gmBtn.layer.borderWidth = 2;
    gmBtn.layer.borderColor = [[UIColor whiteColor] colorWithAlphaComponent:0.5].CGColor;
    [gmBtn addTarget:self action:@selector(gmButtonTapped:) forControlEvents:UIControlEventTouchUpInside];
    [gm_window addSubview:gmBtn];
    
    // Create panel view
    CGFloat panelWidth = 300;
    CGFloat panelHeight = 500;
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
    titleLabel.text = @"🎮 GM 调试器";
    titleLabel.textAlignment = NSTextAlignmentCenter;
    titleLabel.textColor = [UIColor whiteColor];
    titleLabel.font = [UIFont boldSystemFontOfSize:18];
    [panel addSubview:titleLabel];
    
    // Close button
    UIButton *closeBtn = [UIButton buttonWithType:UIButtonTypeCustom];
    [closeBtn setFrame:CGRectMake(panelWidth - 40, 5, 30, 30)];
    [closeBtn setBackgroundColor:[UIColor colorWithRed:0.8 green:0.2 blue:0.2 alpha:1.0]];
    [closeBtn setTitle:@"✕" forState:UIControlStateNormal];
    [closeBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    closeBtn.titleLabel.font = [UIFont boldSystemFontOfSize:14];
    [closeBtn.layer setCornerRadius:15];
    [closeBtn addTarget:self action:@selector(closePanel) forControlEvents:UIControlEventTouchUpInside];
    [panel addSubview:closeBtn];
    
    // GM Options
    NSArray *options = @[@"⚔️ 一刀秒杀", @"🛡️ 无敌模式", @"❤️ 无限血量", @"📊 设置数值", @"⚡ 加速移动", @"📍 传送", @"🎁 添加物品", @"🏆 通关"];
    CGFloat startY = 50;
    CGFloat rowHeight = 45;
    
    for (int i = 0; i < options.count; i++) {
        NSString *title = options[i];
        
        // Button
        UIButton *optBtn = [UIButton buttonWithType:UIButtonTypeCustom];
        [optBtn setFrame:CGRectMake(15, startY + i * rowHeight, panelWidth - 30, 40)];
        [optBtn setBackgroundColor:[UIColor colorWithRed:0.15 green:0.15 blue:0.25 alpha:1.0]];
        [optBtn setTitle:title forState:UIControlStateNormal];
        [optBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        optBtn.titleLabel.font = [UIFont systemFontOfSize:14];
        optBtn.layer.cornerRadius = 8;
        [optBtn.layer setBorderColor:[[UIColor colorWithRed:0.3 green:0.3 blue:0.5 alpha:1.0] CGColor]];
        [optBtn.layer setBorderWidth:1];
        
        // Tag for identification
        optBtn.tag = 2000 + i;
        
        [panel addSubview:optBtn];
    }
    
    // Add to window
    [gm_window addSubview:panel];
    
    LOG("GM panel created using UIKit");
}

// Handle GM button tap
void gmButtonTapped(id sender) {
    UIView *panel = [gm_window viewWithTag:1000];
    if (panel) {
        panel_visible = !panel_visible;
        panel.hidden = !panel_visible;
        LOG("Panel %s", panel_visible ? "opened" : "closed");
    }
}

// Handle close button
void closePanel(id sender) {
    UIView *panel = [gm_window viewWithTag:1000];
    if (panel) {
        panel_visible = NO;
        panel.hidden = YES;
        LOG("Panel closed");
    }
}

// Hooked JSEvaluateScript - for injecting JS if needed
typedef JSStringRef (*JSEvaluateScript_fn)(JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSStringRef *);
static JSEvaluateScript_fn original_JSEvaluateScript = NULL;

JSStringRef hook_JSEvaluateScript(JSContextRef ctx, JSStringRef script, JSObjectRef thisObject, JSStringRef sourceURL, int *exception) {
    JSStringRef result = original_JSEvaluateScript(ctx, script, thisObject, sourceURL, exception);
    
    // First call - create GM panel
    if (!gm_enabled) {
        gm_enabled = 1;
        LOG("GM enabled, creating UIKit panel...");
        create_gm_panel();
    }
    
    return result;
}

// Constructor - called when dylib is loaded
__attribute__((constructor))
static void dxct_gm_init() {
    LOG("Initializing DXCT GM debugger...");
    
    // Try to get JSEvaluateScript for hooking
    original_JSEvaluateScript = (JSEvaluateScript_fn)dlsym(RTLD_DEFAULT, "JSEvaluateScript");
    
    if (original_JSEvaluateScript) {
        LOG("Hook installed: JSEvaluateScript -> hook_JSEvaluateScript");
        
        // Set up rebind
        struct rebinding rebindings[1];
        rebindings[0].name = "JSEvaluateScript";
        rebindings[0].replacement = (void *)hook_JSEvaluateScript;
        rebindings[0].replaced = (void **)&original_JSEvaluateScript;
        
        rebind_symbols(rebindings, 1);
    } else {
        LOG("JSEvaluateScript not found, will create panel on main thread");
        // Fallback: create panel on next run loop
        dispatch_async(dispatch_get_main_queue(), ^{
            create_gm_panel();
        });
    }
    
    // Also try to create panel immediately (fallback)
    dispatch_async(dispatch_get_main_queue(), ^{
        create_gm_panel();
    });
    
    LOG("GM debugger ready");
}
