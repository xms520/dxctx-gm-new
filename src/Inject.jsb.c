#include <UIKit/UIKit.h>
#include <stdio.h>

@interface GMOverlay : UIView
@end

@implementation GMOverlay
- (instancetype)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        self.backgroundColor = [UIColor redColor];
        self.alpha = 0.8;
    }
    return self;
}
@end

@interface GMController : NSObject
@property UIWindow *window;
@property GMOverlay *overlay;
@end

@implementation GMController
- (void)showGM {
    NSLog(@"[GM] showGM called");
    
    UIApplication *app = [UIApplication sharedApplication];
    UIWindow *mainWindow = app.keyWindow ? app.keyWindow : app.windows.firstObject;
    
    if (!mainWindow) {
        NSLog(@"[GM] No main window found!");
        return;
    }
    
    NSLog(@"[GM] Found window: %@", mainWindow);
    
    self.window = [[UIWindow alloc] initWithFrame:mainWindow.bounds];
    self.window.windowLevel = UIWindowLevelAlert + 1;
    self.window.backgroundColor = [UIColor clearColor];
    self.window.rootViewController = [[UIViewController alloc] init];
    
    self.overlay = [[GMOverlay alloc] initWithFrame:CGRectMake(100, 100, 100, 100)];
    [self.window addSubview:self.overlay];
    
    [self.window makeKeyAndVisible];
    
    NSLog(@"[GM] GM overlay created and visible!");
}
@end

static GMController *gmController = nil;

__attribute__((constructor))
static void dxct_init() {
    NSLog(@"[GM] DXCT GM initialized!");
    
    gmController = [[GMController alloc] init];
    
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.0 * NSEC_PER_SEC)), 
                   dispatch_get_main_queue(), ^{
        [gmController showGM];
    });
}
