// Overlay.m - Native GM overlay for 大侠闯天下 v2.0
// Floating panel with 秒杀 and 无敌 toggles

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

// From Inject.jsb.c
extern JSContextRef dxct_get_js_context(void);
extern void dxct_eval_js(const char *js_code);
extern void dxct_set_one_hit_kill(int v);
extern void dxct_set_god_mode(int v);

// Forward declarations
@interface DXCTOverlayVC : UIViewController
@property (nonatomic, strong) UIView *cardView;
@property (nonatomic, strong) UISwitch *killSwitch;
@property (nonatomic, strong) UISwitch *godSwitch;
@property (nonatomic, strong) UILabel *statusLabel;
@property (nonatomic, assign) BOOL oneHitKill;
@property (nonatomic, assign) BOOL godMode;
@end

@implementation DXCTOverlayVC

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor clearColor];
    [self setupUI];
}

- (void)setupUI {
    // Card container
    UIView *card = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 210, 190)];
    card.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.88];
    card.layer.cornerRadius = 14;
    card.layer.borderWidth = 1.0;
    card.layer.borderColor = [UIColor whiteColor].CGColor;
    card.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:card];
    self.cardView = card;
    
    // Drag gesture
    UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onDrag:)];
    [card addGestureRecognizer:pan];
    
    // Title
    UILabel *title = [[UILabel alloc] init];
    title.text = @"🎮 大侠闯天下 GM";
    title.font = [UIFont boldSystemFontOfSize:14];
    title.textColor = [UIColor whiteColor];
    title.textAlignment = NSTextAlignmentCenter;
    title.translatesAutoresizingMaskIntoConstraints = NO;
    [card addSubview:title];
    [NSLayoutConstraint activateConstraints:@[
        [title.topAnchor constraintEqualToAnchor:card.topAnchor constant:14],
        [title.centerXAnchor constraintEqualToAnchor:card.centerXAnchor],
    ]];
    
    // Divider
    UIView *div = [[UIView alloc] init];
    div.backgroundColor = [UIColor whiteColor];
    div.alpha = 0.2;
    div.translatesAutoresizingMaskIntoConstraints = NO;
    [card addSubview:div];
    [NSLayoutConstraint activateConstraints:@[
        [div.topAnchor constraintEqualToAnchor:title.bottomAnchor constant:10],
        [div.leadingAnchor constraintEqualToAnchor:card.leadingAnchor constant:20],
        [div.trailingAnchor constraintEqualToAnchor:card.trailingAnchor constant:-20],
        [div.heightAnchor constraintEqualToConstant:1],
    ]];
    
    // === 秒杀 row ===
    UILabel *killTxt = [[UILabel alloc] init];
    killTxt.text = @"🗡️ 秒杀";
    killTxt.font = [UIFont systemFontOfSize:15];
    killTxt.textColor = [UIColor systemYellowColor];
    killTxt.translatesAutoresizingMaskIntoConstraints = NO;
    [card addSubview:killTxt];
    [NSLayoutConstraint activateConstraints:@[
        [killTxt.topAnchor constraintEqualToAnchor:div.bottomAnchor constant:14],
        [killTxt.leadingAnchor constraintEqualToAnchor:card.leadingAnchor constant:20],
    ]];
    
    UISwitch *killSw = [[UISwitch alloc] init];
    killSw.on = NO;
    [killSw addTarget:self action:@selector(onKillToggled:) forControlEvents:UIControlEventValueChanged];
    killSw.translatesAutoresizingMaskIntoConstraints = NO;
    [card addSubview:killSw];
    [NSLayoutConstraint activateConstraints:@[
        [killSw.centerYAnchor constraintEqualToAnchor:killTxt.centerYAnchor],
        [killSw.trailingAnchor constraintEqualToAnchor:card.trailingAnchor constant:-20],
    ]];
    self.killSwitch = killSw;
    
    // === 无敌 row ===
    UILabel *godTxt = [[UILabel alloc] init];
    godTxt.text = @"🛡️ 无敌";
    godTxt.font = [UIFont systemFontOfSize:15];
    godTxt.textColor = [UIColor systemGreenColor];
    godTxt.translatesAutoresizingMaskIntoConstraints = NO;
    [card addSubview:godTxt];
    [NSLayoutConstraint activateConstraints:@[
        [godTxt.topAnchor constraintEqualToAnchor:killTxt.bottomAnchor constant:16],
        [godTxt.leadingAnchor constraintEqualToAnchor:card.leadingAnchor constant:20],
    ]];
    
    UISwitch *godSw = [[UISwitch alloc] init];
    godSw.on = NO;
    [godSw addTarget:self action:@selector(onGodToggled:) forControlEvents:UIControlEventValueChanged];
    godSw.translatesAutoresizingMaskIntoConstraints = NO;
    [card addSubview:godSw];
    [NSLayoutConstraint activateConstraints:@[
        [godSw.centerYAnchor constraintEqualToAnchor:godTxt.centerYAnchor],
        [godSw.trailingAnchor constraintEqualToAnchor:card.trailingAnchor constant:-20],
    ]];
    self.godSwitch = godSw;
    
    // Status
    UILabel *st = [[UILabel alloc] init];
    st.text = @"状态: 关闭";
    st.font = [UIFont systemFontOfSize:11];
    st.textColor = [UIColor lightGrayColor];
    st.textAlignment = NSTextAlignmentCenter;
    st.translatesAutoresizingMaskIntoConstraints = NO;
    [card addSubview:st];
    [NSLayoutConstraint activateConstraints:@[
        [st.topAnchor constraintEqualToAnchor:godTxt.bottomAnchor constant:16],
        [st.leadingAnchor constraintEqualToAnchor:card.leadingAnchor constant:0],
        [st.trailingAnchor constraintEqualToAnchor:card.trailingAnchor constant:0],
    ]];
    self.statusLabel = st;
    
    // Hint
    UILabel *hint = [[UILabel alloc] init];
    hint.text = @"拖拽移动 · 点击切换";
    hint.font = [UIFont systemFontOfSize:9];
    hint.textColor = [UIColor colorWithWhite:0.5 alpha:1.0];
    hint.textAlignment = NSTextAlignmentCenter;
    hint.translatesAutoresizingMaskIntoConstraints = NO;
    [card addSubview:hint];
    [NSLayoutConstraint activateConstraints:@[
        [hint.topAnchor constraintEqualToAnchor:st.bottomAnchor constant:8],
        [hint.leadingAnchor constraintEqualToAnchor:card.leadingAnchor constant:0],
        [hint.trailingAnchor constraintEqualToAnchor:card.trailingAnchor constant:0],
        [hint.bottomAnchor constraintEqualToAnchor:card.bottomAnchor constant:-12],
    ]];
}

- (void)onKillToggled:(UISwitch *)sw {
    self.oneHitKill = sw.on;
    dxct_set_one_hit_kill(sw.on ? 1 : 0);
    [self refreshStatus];
    NSLog(@"[DXCT] 秒杀 %@", sw.on ? @"ON" : @"OFF");
}

- (void)onGodToggled:(UISwitch *)sw {
    self.godMode = sw.on;
    dxct_set_god_mode(sw.on ? 1 : 0);
    [self refreshStatus];
    NSLog(@"[DXCT] 无敌 %@", sw.on ? @"ON" : @"OFF");
}

- (void)refreshStatus {
    NSString *txt;
    UIColor *clr;
    if (self.oneHitKill && self.godMode) {
        txt = @"🔥 秒杀+无敌 ON";
        clr = [UIColor systemRedColor];
    } else if (self.oneHitKill) {
        txt = @"🗡️ 秒杀 ON";
        clr = [UIColor systemYellowColor];
    } else if (self.godMode) {
        txt = @"🛡️ 无敌 ON";
        clr = [UIColor systemGreenColor];
    } else {
        txt = @"状态: 全部关闭";
        clr = [UIColor lightGrayColor];
    }
    self.statusLabel.text = txt;
    self.statusLabel.textColor = clr;
}

- (void)onDrag:(UIPanGestureRecognizer *)pan {
    UIView *v = pan.view;
    if (pan.state == UIGestureRecognizerStateBegan || pan.state == UIGestureRecognizerStateChanged) {
        CGPoint t = [pan translationInView:self.view];
        CGRect f = v.frame;
        f.origin.x += t.x;
        f.origin.y += t.y;
        CGSize s = self.view.bounds.size;
        f.origin.x = fmax(0, fmin(f.origin.x, s.width  - f.size.width));
        f.origin.y = fmax(0, fmin(f.origin.y, s.height - f.size.height));
        v.frame = f;
        [pan setTranslation:CGPointZero inView:self.view];
    }
}

@end

// ========== Singleton wrapper ==========
static UIWindow *gWindow    = nil;
static DXCTOverlayVC  *gVC    = nil;

__attribute__((constructor))
static void dxct_overlay_ctor(void) {
    NSLog(@"[DXCT] Overlay initializing...");
    
    gVC = [[DXCTOverlayVC alloc] init];
    
    gWindow = [[UIWindow alloc] initWithFrame:CGRectZero];
    gWindow.windowLevel = UIWindowLevelAlert + 100;
    gWindow.rootViewController = gVC;
    gWindow.backgroundColor = [UIColor clearColor];
    gWindow.userInteractionEnabled = YES;
    
    // Show after game is ready
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(5.0 * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
        CGRect screen = [[UIScreen mainScreen] bounds];
        [gWindow setFrame:screen];
        [gWindow makeKeyAndVisible];
        
        // Center the card
        CGRect cardFrame = gVC.cardView.frame;
        cardFrame.origin.x = (screen.size.width  - cardFrame.size.width)  / 2.0;
        cardFrame.origin.y = (screen.size.height - cardFrame.size.height) / 2.0 - 40;
        gVC.cardView.frame = cardFrame;
        
        NSLog(@"[DXCT] GM overlay visible");
    });
}
