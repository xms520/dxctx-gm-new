// GM Template - Simple Test
(function(global) {
    if (global.DXCT_GM_LOADED) return;
    global.DXCT_GM_LOADED = true;
    
    console.log('[DXCT] GM template loaded');
    
    // Test function - simple alert
    try {
        var testBtn = cc.MenuItemLabel.create(
            cc.Label.create("GM", "Arial", 24),
            function() {
                cc.log('[DXCT] GM button clicked!');
                cc.alert('GM Panel', 'Debug Panel Works!', 'OK');
            }
        );
        var menu = cc.Menu.create(testBtn);
        menu.setPosition(cc.winSize.width / 2, cc.winSize.height / 2);
        cc.game.addPersistRootNode(menu);
        console.log('[DXCT] Menu created at ' + menu.getPosition());
    } catch(e) {
        console.log('[DXCT] CC menu error: ' + e);
        // Fallback: simple console log
        console.log('[DXCT] Using fallback debug mode');
    }
    
    // Touch handler - show alert on tap
    cc.eventManager.addListener({
        event: cc.EventListener.TOUCH_ONE_BY_ONE,
        swallowTouches: true,
        onTouchBegan: function(touch, event) {
            var location = touch.getLocation();
            if (location.x > cc.winSize.width - 80 && location.y < 80) {
                console.log('[DXCT] Bottom-right tap detected');
                cc.alert('GM Panel', 'Test: ' + Math.round(location.x) + ',' + Math.round(location.y), 'OK');
                return true;
            }
            return true;
        }
    }, cc.director.getCollisionManager());
    
    console.log('[DXCT] GM debug initialized');
})(this);