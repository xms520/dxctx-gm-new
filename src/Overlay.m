// Overlay.m - Native GM overlay for 大侠闯天下
// Creates a floating toggle panel with 秒杀 and 无敌 switches

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

// Declarations from Inject.jsb.c
extern JSContextRef dxct_get_js_context(void);
extern void dxct_eval_js(const char *js_code);

// Forward declarations
static void dxct_set_one_hit_kill(int v);
static void dxct_set_god_mode(int v);

@interface DXCTOverlayViewController : UIViewController
@property (nonatomic, strong) UIView *container;
@property (nonatomic, strong) UISwitch *killSwitch;
@property (nonatomic, strong) UISwitch *godSwitch;
@property (nonatomic, strong) UILabel *statusLabel;
@property (nonatomic, assign) BOOL oneHitKill;
@property (nonatomic, assign) BOOL godMode;
@end

@implementation DXCTOverlayViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor clearColor];
    self.oneHitKill = NO;
    self.godMode = NO;
    [self buildUI];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    // Make window topmost
    if (self.view.window) {
        self.view.window.windowLevel = UIWindowLevelAlert + 100;
    }
}

- (void)buildUI {
    // Container
    UIView *container = [[UIView alloc] initWithFrame:CGRectMake(0, 60, 220, 180)];
    container.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.85];
    container.layer.cornerRadius = 14;
    container.layer.borderWidth = 1.0;
    container.layer.borderColor = [[UIColor whiteColor] colorWithAlphaComponent:0.25].CGColor;
    container.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:container];
    self.container = container;
    
    // Drag gesture
    UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPan:)];
    [container addGestureRecognizer:pan];
    
    // Title
    UILabel *title = [[UILabel alloc] init];
    title.text = @"🎮 大侠GM v2.0";
    title.textColor = [UIColor whiteColor];
    title.font = [UIFont boldSystemFontOfSize:15];
    title.textAlignment = NSTextAlignmentCenter;
    title.translatesAutoresizingMaskIntoConstraints = NO;
    [container addSubview:title];
    [NSLayoutConstraint activateConstraints:@[
        [title.topAnchor constraintEqualToAnchor:container.topAnchor constant:12],
        [title.leadingAnchor constraintEqualToAnchor:container.leadingAnchor constant:0],
        [title.trailingAnchor constraintEqualToAnchor:container.trailingAnchor constant:0],
    ]];
    
    // Divider
    UIView *divider = [[UIView alloc] init];
    divider.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.2];
    divider.translatesAutoresizingMaskIntoConstraints = NO;
    [container addSubview:divider];
    [NSLayoutConstraint activateConstraints:@[
        [divider.topAnchor constraintEqualToAnchor:title.bottomAnchor constant:10],
        [divider.leadingAnchor constraintEqualToAnchor:container.leadingAnchor constant:15],
        [divider.trailingAnchor constraintEqualToAnchor:container.trailingAnchor constant:-15],
        [divider.heightAnchor constraintEqualToConstant:1],
    ]];
    
    // Row 1: 秒杀
    UILabel *killLabel = [[UILabel alloc] init];
    killLabel.text = @"🗡️ 秒杀";
    killLabel.textColor = [UIColor systemYellowColor];
    killLabel.font = [UIFont systemFontOfSize:15];
    killLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [container addSubview:killLabel];
    [NSLayoutConstraint activateConstraints:@[
        [killLabel.topAnchor constraintEqualToAnchor:divider.bottomAnchor constant:12],
        [killLabel.leadingAnchor constraintEqualToAnchor:container.leadingAnchor constant:20],
    ]];
    
    UISwitch *killSwitch = [[UISwitch alloc] init];
    killSwitch.on = NO;
    [killSwitch addTarget:self action:@selector(onKillSwitch:) forControlEvents:UIControlEventValueChanged];
    killSwitch.translatesAutoresizingMaskIntoConstraints = NO;
    [container addSubview:killSwitch];
    [NSLayoutConstraint activateConstraints:@[
        [killSwitch.topAnchor constraintEqualToAnchor:killLabel.topAnchor constant:0],
        [killSwitch.trailingAnchor constraintEqualToAnchor:container.trailingAnchor constant:-20],
    ]];
    self.killSwitch = killSwitch;
    
    // Row 2: 无敌
    UILabel *godLabel = [[UILabel alloc] init];
    godLabel.text = @"🛡️ 无敌";
    godLabel.textColor = [UIColor systemGreenColor];
    godLabel.font = [UIFont systemFontOfSize:15];
    godLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [container addSubview:godLabel];
    [NSLayoutConstraint activateConstraints:@[
        [godLabel.topAnchor constraintEqualToAnchor:killLabel.bottomAnchor constant:14],
        [godLabel.leadingAnchor constraintEqualToAnchor:container.leadingAnchor constant:20],
    ]];
    
    UISwitch *godSwitch = [[UISwitch alloc] init];
    godSwitch.on = NO;
    [godSwitch addTarget:self action:@selector(onGodSwitch:) forControlEvents:UIControlEventValueChanged];
    godSwitch.translatesAutoresizingMaskIntoConstraints = NO;
    [container addSubview:godSwitch];
    [NSLayoutConstraint activateConstraints:@[
        [godSwitch.topAnchor constraintEqualToAnchor:godLabel.topAnchor constant:0],
        [godSwitch.trailingAnchor constraintEqualToAnchor:container.trailingAnchor constant:-20],
    ]];
    self.godSwitch = godSwitch;
    
    // Status
    UILabel *status = [[UILabel alloc] init];
    status.text = @"状态: 关闭";
    status.textColor = [UIColor lightGrayColor];
    status.font = [UIFont systemFontOfSize:11];
    status.textAlignment = NSTextAlignmentCenter;
    status.translatesAutoresizingMaskIntoConstraints = NO;
    [container addSubview:status];
    [NSLayoutConstraint activateConstraints:@[
        [status.topAnchor constraintEqualToAnchor:godLabel.bottomAnchor constant:14],
        [status.leadingAnchor constraintEqualToAnchor:container.leadingAnchor constant:0],
        [status.trailingAnchor constraintEqualToAnchor:container.trailingAnchor constant:0],
    ]];
    self.statusLabel = status;
    
    // Hint
    UILabel *hint = [[UILabel alloc] init];
    hint.text = @"拖拽移动 • 点击切换";
    hint.textColor = [UIColor colorWithWhite:0.5 alpha:1.0];
    hint.font = [UIFont systemFontOfSize:10];
    hint.textAlignment = NSTextAlignmentCenter;
    hint.translatesAutoresizingMaskIntoConstraints = NO;
    [container addSubview:hint];
    [NSLayoutConstraint activateConstraints:@[
        [hint.topAnchor constraintEqualToAnchor:status.bottomAnchor constant:6],
        [hint.leadingAnchor constraintEqualToAnchor:container.leadingAnchor constant:0],
        [hint.trailingAnchor constraintEqualToAnchor:container.trailingAnchor constant:0],
        [hint.bottomAnchor constraintEqualToAnchor:container.bottomAnchor constant:-8],
    ]];
}

- (void)onKillSwitch:(UISwitch *)sender {
    self.oneHitKill = sender.on;
    dxct_set_one_hit_kill(sender.on ? 1 : 0);
    [self updateStatus];
    [self sendToJSWithFeature:@"oneHitKill" on:sender.on];
    NSLog(@"[DXCT] 秒杀 %@", sender.on ? @"ON" : @"OFF");
}

- (void)onGodSwitch:(UISwitch *)sender {
    self.godMode = sender.on;
    dxct_set_god_mode(sender.on ? 1 : 0);
    [self updateStatus];
    [self sendToJSWithFeature:@"godMode" on:sender.on];
    NSLog(@"[DXCT] 无敌 %@", sender.on ? @"ON" : @"OFF");
}

- (void)updateStatus {
    NSString *text;
    UIColor *color;
    if (self.oneHitKill && self.godMode) {
        text = @"🔥 秒杀+无敌 ON";
        color = [UIColor systemRedColor];
    } else if (self.oneHitKill) {
        text = @"🗡️ 秒杀 ON";
        color = [UIColor systemYellowColor];
    } else if (self.godMode) {
        text = @"🛡️ 无敌 ON";
        color = [UIColor systemGreenColor];
    } else {
        text = @"状态: 全部关闭";
        color = [UIColor lightGrayColor];
    }
    self.statusLabel.text = text;
    self.statusLabel.textColor = color;
}

- (void)sendToJSWithFeature:(NSString *)feature on:(BOOL)on {
    JSContextRef ctx = dxct_get_js_context();
    if (!ctx) {
        NSLog(@"[DXCT] No JS context yet");
        return;
    }
    
    const char *js;
    if ([feature isEqualToString:@"oneHitKill"]) {
        js = on 
            ? "if(window.GM){try{GM.toggleOneHitKill();}catch(e){console.log('[GM] error:'+e);}}"
            : "if(window.GM){try{GM.unhookDamage();GM.oneHitKill=false;}catch(e){}}";
    } else {
        js = on
            ? "if(window.GM){try{GM.toggleGodMode();}catch(e){console.log('[GM] error:'+e);}}"
            : "if(window.GM){try{GM.unhookTakeDamage();GM.godMode=false;}catch(e){}}";
    }
    
    dxct_eval_js(js);
}

- (void)onPan:(UIPanGestureRecognizer *)pan {
    UIView *view = pan.view;
    if (pan.state == UIGestureRecognizerStateBegan || pan.state == UIGestureRecognizerStateChanged) {
        CGPoint translation = [pan translationInView:self.view];
        CGRect frame = view.frame;
        frame.origin.x += translation.x;
        frame.origin.y += translation.y;
        // Clamp to screen bounds
        CGRect screen = [UIApplication sharedApplication].keyWindow.bounds;
        frame.origin.x = fmax(0, fmin(frame.origin.x, screen.size.width - frame.size.width));
        frame.origin.y = fmax(0, fmin(frame.origin.y, screen.size.height - frame.size.height));
        view.frame = frame;
        [pan setTranslation:CGPointZero inView:self.view];
    }
}

@end

// ========== Global State ==========
static UIWindow *gGMWindow = nil;
static DXCTOverlayViewController *gViewController = nil;
static dispatch_once_t gInitOnce;

// ========== Constructor ==========
__attribute__((constructor))
static void dxct_overlay_init(void) {
    dispatch_once(&gInitOnce, ^{
        NSLog(@"[DXCT] Native overlay initializing...");
        
        gViewController = [[DXCTOverlayViewController alloc] init];
        
        gGMWindow = [[UIWindow alloc] initWithFrame:CGRectZero];
        gGMWindow.windowLevel = UIWindowLevelAlert + 100;
        gGMWindow.rootViewController = gViewController;
        gGMWindow.backgroundColor = [UIColor clearColor];
        gGMWindow.userInteractionEnabled = YES;
        
        // Delay creation to let game load
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(5.0 * NSEC_PER_SEC)),
                      dispatch_get_main_queue(), ^{
            [gGMWindow setFrame:[UIApplication sharedApplication].keyWindow.bounds]];
            [gGMWindow makeKeyAndVisible];
            NSLog(@"[DXCT] GM overlay visible");
        });
    });
}
