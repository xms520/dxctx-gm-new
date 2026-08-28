//
//  Overlay.m - GM 调试面板 (大侠闯天下 秒杀+无敌)
//  Objective-C 原生悬浮窗 UI
//
//  功能:
//     🗡️ 秒杀开关      🛡️ 无敌开关      ❤️ 满血
//     ⚡ 无限攻击       🌀 快速攻击       🔍 场景调试
//
//  通过 dxct_run_js() 调用 JS 侧 GM 对象的对应方法, 达到实时生效。
//  面板可拖动, 点击标题栏收起/展开, 有半透明背景与开关状态高亮。

#import <UIKit/UIKit.h>
#import <JavaScriptCore/JavaScriptCore.h>
#import "gm_overlay.h"

// =========================================================================
// 日志
// =========================================================================
static void olog(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    static FILE *gLog = NULL;
    if (!gLog) gLog = fopen("/var/mobile/Library/Logs/dxct_gm.log", "a");
    if (gLog) {
        fprintf(gLog, "[DXCT-UI] ");
        vfprintf(gLog, fmt, args);
        fprintf(gLog, "\n");
        fflush(gLog);
    }
    va_end(args);
}

// =========================================================================
// Overlay window forwards touches outside the panel to the game window.
// =========================================================================
@interface DXCTOverlayWindow : UIWindow
@end

@implementation DXCTOverlayWindow
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hit = [super hitTest:point withEvent:event];
    if (hit == self || hit == self.rootViewController.view) return nil;
    return hit;
}
@end

// =========================================================================
// GM Overlay ViewController (负责承载面板与按钮)
// =========================================================================
@interface GMOverlayVC : UIViewController
@property(nonatomic, strong) UIView *panelView;
@property(nonatomic, assign) BOOL expanded;
@property(nonatomic, strong) NSMutableDictionary *btnStates;
@property(nonatomic, assign) CGFloat originX;
@property(nonatomic, assign) CGFloat originY;
- (void)applyStateForCommand:(NSString *)cmd on:(BOOL)on;
@end

@implementation GMOverlayVC

- (UIButton *)makeButton:(NSString *)title frame:(CGRect)frame {
    UIButton *btn = [UIButton buttonWithType:UIButtonTypeSystem];
    if (!btn) return nil;
    btn.frame = frame;
    btn.layer.cornerRadius = 12.0;
    btn.layer.borderWidth = 1.0;
    btn.titleLabel.font = [UIFont boldSystemFontOfSize:15];
    [btn setTitle:title forState:UIControlStateNormal];
    [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [btn setBackgroundColor:[UIColor colorWithWhite:0.15 alpha:0.9]];
    btn.layer.borderColor = [UIColor colorWithWhite:0.45 alpha:1.0].CGColor;
    return btn;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    CGFloat pw = 200, ph = 300;
    _originX = 16;
    _originY = 120;
    _expanded = YES;

    UIView *panel = [[UIView alloc] initWithFrame:CGRectMake(_originX, _originY, pw, ph)];
    panel.backgroundColor = [UIColor colorWithWhite:0.08 alpha:0.72];
    panel.layer.cornerRadius = 16.0;
    panel.layer.borderWidth = 1.0;
    panel.layer.borderColor = [UIColor colorWithWhite:0.6 alpha:0.5].CGColor;
    panel.clipsToBounds = YES;
    self.panelView = panel;
    [self.view addSubview:panel];

    // 标题栏
    UIButton *title = [UIButton buttonWithType:UIButtonTypeCustom];
    title.frame = CGRectMake(0, 0, pw, 36);
    title.backgroundColor = [UIColor colorWithRed:0.12 green:0.12 blue:0.16 alpha:0.95];
    [title setTitle:@"🕹 大侠 GM v3.0" forState:UIControlStateNormal];
    [title setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    title.titleLabel.font = [UIFont boldSystemFontOfSize:15];
    [panel addSubview:title];

    // 收起/展开按钮
    UIButton *fold = [UIButton buttonWithType:UIButtonTypeSystem];
    fold.frame = CGRectMake(pw - 40, 0, 40, 36);
    [fold setTitle:@"▽" forState:UIControlStateNormal];
    [fold setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    fold.titleLabel.font = [UIFont boldSystemFontOfSize:20];
    [panel addSubview:fold];

    __weak typeof(self) weakSelf = self;
    [fold addTarget:weakSelf action:@selector(foldTapped:) forControlEvents:UIControlEventTouchUpInside];
    [title addTarget:weakSelf action:@selector(foldTapped:) forControlEvents:UIControlEventTouchUpInside];

    // 功能按钮
    int y = 44;
    int bh = 40, bw = pw - 24;

    UIButton *bKill = [self makeButton:@"🗡️ 秒杀 OFF" frame:CGRectMake(12, y, bw, bh)];
    UIButton *bGod  = [self makeButton:@"🛡️ 无敌 OFF" frame:CGRectMake(12, y+bh+8, bw, bh)];
    UIButton *bHeal = [self makeButton:@"❤️ 满血恢复" frame:CGRectMake(12, y+2*(bh+8), bw, bh)];
    UIButton *bAtk  = [self makeButton:@"⚡ 无限攻击" frame:CGRectMake(12, y+3*(bh+8), bw, bh)];
    UIButton *bSpd  = [self makeButton:@"🌀 快速攻击" frame:CGRectMake(12, y+4*(bh+8), bw, bh)];
    UIButton *bDump = [self makeButton:@"🔍 场景调试" frame:CGRectMake(12, y+5*(bh+8), bw, bh)];

    bKill.tag = 101; bGod.tag = 102; bHeal.tag = 103; bAtk.tag = 104; bSpd.tag = 105; bDump.tag = 106;

    [panel addSubview:bKill];
    [panel addSubview:bGod];
    [panel addSubview:bHeal];
    [panel addSubview:bAtk];
    [panel addSubview:bSpd];
    [panel addSubview:bDump];

    [bKill addTarget:weakSelf action:@selector(bKillTapped:) forControlEvents:UIControlEventTouchUpInside];
    [bGod  addTarget:weakSelf action:@selector(bGodTapped:)  forControlEvents:UIControlEventTouchUpInside];
    [bHeal addTarget:weakSelf action:@selector(bHealTapped:) forControlEvents:UIControlEventTouchUpInside];
    [bAtk  addTarget:weakSelf action:@selector(bAtkTapped:)  forControlEvents:UIControlEventTouchUpInside];
    [bSpd  addTarget:weakSelf action:@selector(bSpdTapped:)  forControlEvents:UIControlEventTouchUpInside];
    [bDump addTarget:weakSelf action:@selector(bDumpTapped:) forControlEvents:UIControlEventTouchUpInside];

    _btnStates = [NSMutableDictionary dictionary];
    _btnStates[@"kill"] = @NO;
    _btnStates[@"god"]  = @NO;
    _btnStates[@"atk"]  = @NO;

    olog("Overlay UI created");
}

// ---------- 按钮动作 ----------

- (void)bKillTapped:(id)sender {
    BOOL on = ![self onFor:@"kill"];
    _btnStates[@"kill"] = @(on);
    if (dxct_run_js("GM.toggleOneHitKill()")) {
        [self applyStateForCommand:@"kill" on:on];
    }
}

- (void)bGodTapped:(id)sender {
    BOOL on = ![self onFor:@"god"];
    _btnStates[@"god"] = @(on);
    if (dxct_run_js("GM.toggleGodMode()")) {
        [self applyStateForCommand:@"god" on:on];
    }
}

- (void)bHealTapped:(id)sender {
    dxct_run_js("GM.fullHeal()");
}

- (void)bAtkTapped:(id)sender {
    BOOL on = ![self onFor:@"atk"];
    _btnStates[@"atk"] = @(on);
    if (dxct_run_js(on ? "GM.infiniteAttack(99999)" : "GM.infiniteAttack(1)")) {
        [self applyStateForCommand:@"atk" on:on];
    }
}

- (void)bSpdTapped:(id)sender {
    dxct_run_js("GM.fastAttack(0.05)");
}

- (void)bDumpTapped:(id)sender {
    dxct_run_js("GM.dumpObjects()");
}

// ---------- 状态辅助 ----------

- (BOOL)onFor:(NSString *)key {
    id v = _btnStates[key];
    return v ? [v boolValue] : NO;
}

- (void)applyStateForCommand:(NSString *)cmd on:(BOOL)on {
    UIButton *btn = [self btnFor:cmd];
    if (!btn) return;
    if (on) {
        btn.backgroundColor = [UIColor colorWithRed:0.0 green:0.55 blue:0.0 alpha:0.9];
        NSString *title = [NSString stringWithFormat:@"%@ ON ✅", [self labelFor:cmd]];
        [btn setTitle:title forState:UIControlStateNormal];
    } else {
        btn.backgroundColor = [UIColor colorWithWhite:0.15 alpha:0.9];
        NSString *title = [NSString stringWithFormat:@"%@ OFF ❌", [self labelFor:cmd]];
        [btn setTitle:title forState:UIControlStateNormal];
    }
}

- (NSString *)labelFor:(NSString *)cmd {
    if ([cmd isEqual:@"kill"]) return @"🗡️ 秒杀";
    if ([cmd isEqual:@"god"])  return @"🛡️ 无敌";
    if ([cmd isEqual:@"atk"])  return @"⚡ 攻击";
    return cmd;
}

- (UIButton *)btnFor:(NSString *)cmd {
    long tag = 0;
    if ([cmd isEqual:@"kill"]) tag = 101;
    else if ([cmd isEqual:@"god"]) tag = 102;
    else if ([cmd isEqual:@"atk"]) tag = 104;
    return (UIButton *)[self.view viewWithTag:tag];
}

- (void)foldTapped:(id)sender {
    _expanded = !_expanded;
    CGRect f = _panelView.frame;
    CGFloat ph = _expanded ? 300 : 40;
    f.size.height = ph;
    [UIView animateWithDuration:0.25 animations:^{
        self.panelView.frame = f;
    }];
}

@end

// =========================================================================
// C 入口: 在主线程创建并展示 Overlay
// =========================================================================

static GMOverlayVC *gVC = nil;
static DXCTOverlayWindow *gWin = nil;
static BOOL gUIShown = NO;

void dxct_show_overlay(void) {
    if (gUIShown) {
        return;
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        if (gUIShown) return;

        @try {
            UIWindowScene *scene = nil;
            for (UIScene *candidate in [UIApplication sharedApplication].connectedScenes) {
                if ([candidate isKindOfClass:[UIWindowScene class]] &&
                    candidate.activationState == UISceneActivationStateForegroundActive) {
                    scene = (UIWindowScene *)candidate;
                    break;
                }
            }
            if (!scene) {
                for (UIScene *candidate in [UIApplication sharedApplication].connectedScenes) {
                    if ([candidate isKindOfClass:[UIWindowScene class]]) {
                        scene = (UIWindowScene *)candidate;
                        break;
                    }
                }
            }
            if (!gWin) {
                gWin = scene ? [[DXCTOverlayWindow alloc] initWithWindowScene:scene]
                             : [[DXCTOverlayWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
            }
            if (scene && gWin.windowScene != scene) {
                gWin.windowScene = scene;
            }
            gWin.frame = scene ? scene.coordinateSpace.bounds : [UIScreen mainScreen].bounds;
            if (!gVC) {
                gVC = [[GMOverlayVC alloc] initWithNibName:nil bundle:nil];
            }
            gWin.windowLevel = 1001.0;
            gWin.rootViewController = gVC;
            gWin.backgroundColor = [UIColor clearColor];
            gWin.hidden = NO;
            // Do not steal key-window status from the game's login UI.
            [gWin setHidden:NO];
            [gWin setUserInteractionEnabled:YES];
            gUIShown = YES;
            olog("Overlay shown");
        } @catch (NSException *e) {
            olog("Overlay failed: %s", [[e description] UTF8String]);
        }
    });
}