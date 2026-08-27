// Overlay.m - Native GM overlay for 大侠闯天下
// Creates a floating toggle panel with 秒杀 and 无敌 switches
// Links to Inject.jsb.c via external symbol declarations

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

// Declarations from Inject.jsb.c
extern JSContextRef dxct_get_js_context(void);
extern void dxct_eval_js(const char *js_code);
extern int dxct_get_one_hit_kill(void);
extern int dxct_get_god_mode(void);
extern void dxct_set_one_hit_kill(int v);
extern void dxct_set_god_mode(int v);

// GM Overlay Controller
@interface DXCTOverlayController : NSObject
@property (nonatomic, strong) UIWindow *window;
@property (nonatomic, assign) BOOL oneHitKill;
@property (nonatomic, assign) BOOL godMode;
@end

@implementation DXCTOverlayController

- (instancetype)init {
    self = [super init];
    if (self) {
        _oneHitKill = NO;
        _godMode = NO;
    }
    return self;
}

- (void)showOverlay {
    NSLog(@"[DXCT] Creating GM overlay...");
    
    UIWindow *window = [[UIWindow alloc] initWithFrame:CGRectMake(0, 0, 220, 200)];
    window.windowLevel = UIWindowLevelAlert + 100;
    window.backgroundColor = [UIColor clearColor];
    window.userInteractionEnabled = YES;
    
    // Draggable container
    UIView *container = [[UIView alloc] initWithFrame:CGRectMake(15, 70, 190, 155)];
    container.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.80];
    container.layer.cornerRadius = 14;
    container.layer.borderWidth = 1.0;
    container.layer.borderColor = [[UIColor whiteColor] colorWithAlphaComponent:0.3].CGColor;
    
    // Drag gesture
    UIPanGestureRecognizer *panGR = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPan:)];
    [container addGestureRecognizer:panGR];
    
    // Title
    UILabel *title = [[UILabel alloc] initWithFrame:CGRectMake(0, 8, 190, 32)];
    title.text = @"🎮 大侠GM v2.0";
    title.textColor = [UIColor whiteColor];
    title.font = [UIFont boldSystemFontOfSize:14];
    title.textAlignment = NSTextAlignmentCenter;
    [container addSubview:title];
    
    // Divider
    UIView *divider = [[UIView alloc] initWithFrame:CGRectMake(10, 42, 170, 1)];
    divider.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.2];
    [container addSubview:divider];
    
    // Row 1: 秒杀
    UILabel *killLabel = [[UILabel alloc] initWithFrame:CGRectMake(15, 50, 90, 30)];
    killLabel.text = @"🗡️ 秒杀";
    killLabel.textColor = [UIColor systemYellowColor];
    killLabel.font = [UIFont systemFontOfSize:15];
    [container addSubview:killLabel];
    
    UISwitch *killSwitch = [[UISwitch alloc] initWithFrame:CGRectMake(125, 52, 55, 30)];
    killSwitch.on = NO;
    [killSwitch addTarget:self action:@selector(onKillSwitch:) forControlEvents:UIControlEventValueChanged];
    [container addSubview:killSwitch];
    self.killSwitchRef = killSwitch;
    
    // Row 2: 无敌
    UILabel *godLabel = [[UILabel alloc] initWithFrame:CGRectMake(15, 88, 90, 30)];
    godLabel.text = @"🛡️ 无敌";
    godLabel.textColor = [UIColor systemGreenColor];
    godLabel.font = [UIFont systemFontOfSize:15];
    [container addSubview:godLabel];
    
    UISwitch *godSwitch = [[UISwitch alloc] initWithFrame:CGRectMake(125, 90, 55, 30)];
    godSwitch.on = NO;
    [godSwitch addTarget:self action:@selector(onGodSwitch:) forControlEvents:UIControlEventValueChanged];
    [container addSubview:godSwitch];
    self.godSwitchRef = godSwitch;
    
    // Status label
    UILabel *statusLabel = [[UILabel alloc] initWithFrame:CGRectMake(10, 128, 170, 22)];
    statusLabel.text = @"状态: 关闭";
    statusLabel.textColor = [UIColor lightGrayColor];
    statusLabel.font = [UIFont systemFontOfSize:11];
    statusLabel.textAlignment = NSTextAlignmentCenter;
    [container addSubview:statusLabel];
    self.statusLabel = statusLabel;
    
    // Help hint
    UILabel *hint = [[UILabel alloc] initWithFrame:CGRectMake(10, 152, 170, 18)];
    hint.text = @"点击按钮切换功能";
    hint.textColor = [UIColor colorWithWhite:0.6 alpha:1.0];
    hint.font = [UIFont systemFontOfSize:10];
    hint.textAlignment = NSTextAlignmentCenter;
    [container addSubview:hint];
    
    [window addSubview:container];
    self.window = window;
    [window makeKeyAndVisible];
    
    NSLog(@"[DXCT] GM overlay created and visible");
}

- (void)onKillSwitch:(UISwitch *)sender {
    self.oneHitKill = sender.on;
    dxct_set_one_hit_kill(sender.on ? 1 : 0);
    [self updateStatus];
    [self sendToJS:@"oneHitKill"];
    NSLog(@"[DXCT] 秒杀 %@", sender.on ? @"ON" : @"OFF");
}

- (void)onGodSwitch:(UISwitch *)sender {
    self.godMode = sender.on;
    dxct_set_god_mode(sender.on ? 1 : 0);
    [self updateStatus];
    [self sendToJS:@"godMode"];
    NSLog(@"[DXCT] 无敌 %@", sender.on ? @"ON" : @"OFF");
}

- (void)updateStatus {
    if (!self.statusLabel) return;
    NSString *status;
    if (self.oneHitKill && self.godMode) {
        status = @"秒杀+无敌 ON";
    } else if (self.oneHitKill) {
        status = @"秒杀 ON";
    } else if (self.godMode) {
        status = @"无敌 ON";
    } else {
        status = @"全部关闭";
    }
    UIColor *color;
    if (self.oneHitKill && self.godMode) {
        color = [UIColor systemRedColor];
    } else if (self.oneHitKill) {
        color = [UIColor systemYellowColor];
    } else if (self.godMode) {
        color = [UIColor systemGreenColor];
    } else {
        color = [UIColor lightGrayColor];
    }
    self.statusLabel.text = status;
    self.statusLabel.textColor = color;
}

- (void)sendToJS:(NSString *)feature {
    JSContextRef ctx = dxct_get_js_context();
    if (!ctx) {
        NSLog(@"[DXCT] No JS context available yet");
        return;
    }
    
    CFStringRef jsCode;
    if ([feature isEqualToString:@"oneHitKill"]) {
        jsCode = CFStringCreateWithCString(NULL, 
            self.oneHitKill ? "if(window.GM){GM.toggleOneHitKill();}" : "if(window.GM){GM.unhookDamage();GM.oneHitKill=false;}",
            kCFStringEncodingUTF8);
    } else {
        jsCode = CFStringCreateWithCString(NULL,
            self.godMode ? "if(window.GM){GM.toggleGodMode();}" : "if(window.GM){GM.unhookTakeDamage();GM.godMode=false;}",
            kCFStringEncodingUTF8);
    }
    
    if (jsCode) {
        JSStringRef jsStr = JSStringCreateWithCFString(jsCode);
        CFRelease(jsCode);
        if (jsStr) {
            JSValueRef exc = NULL;
            JSEvaluateScript(ctx, jsStr, NULL, NULL, 1, &exc);
            JSStringRelease(jsStr);
        }
    }
}

- (void)onPan:(UIPanGestureRecognizer *)pan {
    UIView *container = pan.view;
    if (pan.state == UIGestureRecognizerStateBegan || pan.state == UIGestureRecognizerStateChanged) {
        CGPoint translation = [pan translationInView:self.window];
        CGRect frame = container.frame;
        frame.origin.x += translation.x;
        frame.origin.y += translation.y;
        // Keep within bounds
        frame.origin.x = MIN(MAX(frame.origin.x, 0), self.window.bounds.size.width - frame.size.width);
        frame.origin.y = MIN(MAX(frame.origin.y, 0), self.window.bounds.size.height - frame.size.height);
        container.frame = frame;
        [pan setTranslation:CGPointZero inView:self.window];
    }
}

@end

// Global controller
static DXCTOverlayController *gOverlayController = nil;
static dispatch_once_t onceToken;

// Constructor - called when dylib is loaded
__attribute__((constructor))
static void dxct_overlay_init(void) {
    dispatch_once(&onceToken, ^{
        NSLog(@"[DXCT] Native overlay initializing...");
        gOverlayController = [[DXCTOverlayController alloc] init];
        
        // Delay creation to let game initialize
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(4.0 * NSEC_PER_SEC)),
                      dispatch_get_main_queue(), ^{
            [gOverlayController showOverlay];
        });
    });
}
