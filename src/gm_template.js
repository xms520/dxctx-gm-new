/**
 * GM Debug Panel for 大侠闯天下 (iOS Touch Version)
 * 
 * 交互方式:
 *   - 双指点击屏幕: 显示/隐藏菜单
 *   - 点击悬浮球: 打开功能面板
 *   - 滑动切换: 不同功能页
 * 
 * 功能:
 *   F1 - 一刀秒杀 / 单指点击
 *   F2 - 无敌模式 / 双指点击  
 *   F3 - 无限血量 / 三指点击
 *   F4 - 设置数值 / 四指点击
 *   F5 - 加速移动 / 长按
 *   F6 - 传送 / 五指点击
 *   F7 - 添加物品 / 六指点击
 *   F8 - 通关 / 七指点击
 *   F9 - 显示/隐藏面板 / 八指点击
 */

(function() {
    'use strict';
    
    // GM State
    var GM = {
        enabled: false,
        oneHitKill: false,
        invincible: false,
        infiniteHp: false,
        hpValue: 100,
        atkValue: 10,
        speedMult: 1,
        panelVisible: true,
        menuOpen: false
    };
    
    // Touch tracking
    var touchCount = 0;
    var touchStartTime = 0;
    var activeTouches = {};
    
    // Initialize GM
    function init() {
        console.log('[DXCTGM] GM panel initializing (iOS touch version)...');
        
        // Create GM object in global scope
        window.dxct_gm = GM;
        
        // Setup touch handlers
        setupTouchHandlers();
        
        // Create floating button
        createFloatingButton();
        
        // Create function panel
        createFunctionPanel();
        
        console.log('[DXCTGM] GM panel ready - double tap to toggle');
    }
    
    // Touch handlers for multi-touch gestures
    function setupTouchHandlers() {
        document.addEventListener('touchstart', function(e) {
            touchCount = e.touches.length;
            touchStartTime = Date.now();
            
            // Track each touch
            for (var i = 0; i < e.touches.length; i++) {
                var touch = e.touches[i];
                activeTouches[touch.identifier] = {
                    x: touch.clientX,
                    y: touch.clientY,
                    startTime: Date.now()
                };
            }
            
            // Multi-touch gesture detection
            if (touchCount >= 2 && touchCount <= 8) {
                handleMultiTouch(touchCount);
            }
        }, true);
        
        document.addEventListener('touchend', function(e) {
            // Remove ended touches
            for (var i = 0; i < e.changedTouches.length; i++) {
                var touch = e.changedTouches[i];
                delete activeTouches[touch.identifier];
            }
            
            touchCount = e.touches.length;
        }, true);
        
        document.addEventListener('touchmove', function(e) {
            // Update touch positions
            for (var i = 0; i < e.touches.length; i++) {
                var touch = e.touches[i];
                if (activeTouches[touch.identifier]) {
                    activeTouches[touch.identifier].x = touch.clientX;
                    activeTouches[touch.identifier].y = touch.clientY;
                }
            }
        }, true);
    }
    
    // Handle multi-touch gestures
    function handleMultiTouch(count) {
        switch(count) {
            case 2:
                // Double tap - toggle panel
                togglePanel();
                break;
            case 3:
                // Triple tap - toggle invincible
                toggleInvincible();
                break;
            case 4:
                // Four fingers - set stats
                setStats();
                break;
            case 5:
                // Five fingers - teleport
                teleport();
                break;
            case 6:
                // Six fingers - add item
                addItem();
                break;
            case 7:
                // Seven fingers - complete stage
                completeStage();
                break;
            case 8:
                // Eight fingers - kill all enemies
                killAllEnemies();
                break;
        }
    }
    
    // Floating button
    function createFloatingButton() {
        var btn = document.createElement('div');
        btn.id = 'dxct-gm-fab';
        btn.innerHTML = 'GM';
        btn.onclick = function() {
            toggleMenu();
        };
        
        // Style
        var style = document.createElement('style');
        style.textContent = `
            #dxct-gm-fab {
                position: fixed;
                bottom: 100px;
                right: 20px;
                width: 50px;
                height: 50px;
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                color: white;
                border-radius: 50%;
                display: flex;
                align-items: center;
                justify-content: center;
                font-size: 14px;
                font-weight: bold;
                box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
                z-index: 99998;
                cursor: pointer;
                transition: transform 0.2s;
            }
            #dxct-gm-fab:active {
                transform: scale(0.9);
            }
        `;
        btn.appendChild(style);
        document.body.appendChild(btn);
    }
    
    // Function panel
    function createFunctionPanel() {
        var panel = document.createElement('div');
        panel.id = 'dxct-gm-panel';
        panel.style.display = 'none';
        
        panel.innerHTML = `
            <div class="gm-header">
                <span>DXCT GM</span>
                <button class="gm-close" onclick="dxct_gm.toggleMenu()">✕</button>
            </div>
            <div class="gm-content">
                <div class="gm-item" onclick="dxct_gm.toggleOneHitKill()">
                    <span>⚔️ 一刀秒杀</span>
                    <span class="status" id="status-oneHitKill">OFF</span>
                </div>
                <div class="gm-item" onclick="dxct_gm.toggleInvincible()">
                    <span>🛡️ 无敌模式</span>
                    <span class="status" id="status-invincible">OFF</span>
                </div>
                <div class="gm-item" onclick="dxct_gm.toggleInfiniteHp()">
                    <span>❤️ 无限血量</span>
                    <span class="status" id="status-infiniteHp">OFF</span>
                </div>
                <div class="gm-item" onclick="dxct_gm.setStats()">
                    <span>📊 设置数值</span>
                    <span class="arrow">›</span>
                </div>
                <div class="gm-item" onclick="dxct_gm.changeSpeed()">
                    <span>⚡ 加速移动</span>
                    <span id="speed-val">1.0x</span>
                </div>
                <div class="gm-item" onclick="dxct_gm.teleport()">
                    <span>📍 传送</span>
                    <span class="arrow">›</span>
                </div>
                <div class="gm-item" onclick="dxct_gm.addItem()">
                    <span>🎁 添加物品</span>
                    <span class="arrow">›</span>
                </div>
                <div class="gm-item" onclick="dxct_gm.completeStage()">
                    <span>🏆 通关</span>
                    <span class="arrow">›</span>
                </div>
                <div class="gm-item" onclick="dxct_gm.killAllEnemies()">
                    <span>💀 秒杀敌人</span>
                    <span class="arrow">›</span>
                </div>
            </div>
            <div class="gm-footer">
                <small>双指点击: 隐藏 | 悬浮球: 菜单</small>
            </div>
        `;
        
        // Add styles
        var style = document.createElement('style');
        style.textContent = `
            #dxct-gm-panel {
                position: fixed;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
                width: 300px;
                max-height: 70vh;
                background: rgba(20, 20, 30, 0.95);
                color: #fff;
                border: 2px solid #667eea;
                border-radius: 16px;
                padding: 0;
                font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
                z-index: 99999;
                overflow: hidden;
                box-shadow: 0 10px 40px rgba(0,0,0,0.5);
            }
            .gm-header {
                display: flex;
                justify-content: space-between;
                align-items: center;
                padding: 15px;
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                font-weight: bold;
            }
            .gm-close {
                background: none;
                border: none;
                color: white;
                font-size: 18px;
                cursor: pointer;
                padding: 0;
            }
            .gm-content {
                padding: 10px 0;
                max-height: 55vh;
                overflow-y: auto;
            }
            .gm-item {
                display: flex;
                justify-content: space-between;
                align-items: center;
                padding: 12px 20px;
                cursor: pointer;
                transition: background 0.2s;
            }
            .gm-item:active {
                background: rgba(102, 126, 234, 0.3);
            }
            .status {
                padding: 4px 10px;
                border-radius: 12px;
                font-size: 12px;
                font-weight: bold;
            }
            .status.on {
                background: #4ade80;
                color: #000;
            }
            .status.off {
                background: #f87171;
                color: #fff;
            }
            .arrow {
                color: #667eea;
                font-size: 20px;
            }
            .gm-footer {
                padding: 10px;
                text-align: center;
                color: #666;
                font-size: 11px;
                border-top: 1px solid #333;
            }
        `;
        panel.appendChild(style);
        document.body.appendChild(panel);
    }
    
    // GM Functions
    function toggleOneHitKill() {
        GM.oneHitKill = !GM.oneHitKill;
        updateStatus('oneHitKill', GM.oneHitKill);
        console.log('[DXCTGM] One Hit Kill: ' + (GM.oneHitKill ? 'ON' : 'OFF'));
    }
    
    function toggleInvincible() {
        GM.invincible = !GM.invincible;
        updateStatus('invincible', GM.invincible);
        console.log('[DXCTGM] Invincible: ' + (GM.invincible ? 'ON' : 'OFF'));
    }
    
    function toggleInfiniteHp() {
        GM.infiniteHp = !GM.infiniteHp;
        updateStatus('infiniteHp', GM.infiniteHp);
        console.log('[DXCTGM] Infinite HP: ' + (GM.infiniteHp ? 'ON' : 'OFF'));
    }
    
    function setStats() {
        var hp = prompt('输入血量值:', GM.hpValue);
        if (hp !== null) {
            GM.hpValue = parseFloat(hp) || 100;
        }
        var atk = prompt('输入攻击力:', GM.atkValue);
        if (atk !== null) {
            GM.atkValue = parseFloat(atk) || 10;
        }
        console.log('[DXCTGM] Stats: HP=' + GM.hpValue + ' ATK=' + GM.atkValue);
    }
    
    function changeSpeed() {
        GM.speedMult = GM.speedMult >= 3 ? 1 : GM.speedMult + 0.5;
        document.getElementById('speed-val').textContent = GM.speedMult.toFixed(1) + 'x';
        console.log('[DXCTGM] Speed: ' + GM.speedMult + 'x');
    }
    
    function teleport() {
        var x = prompt('输入X坐标:', '0');
        var y = prompt('输入Y坐标:', '0');
        if (x !== null && y !== null) {
            console.log('[DXCTGM] Teleport to (' + x + ', ' + y + ')');
        }
    }
    
    function addItem() {
        var itemId = prompt('物品ID:', '1001');
        var count = prompt('数量:', '10');
        if (itemId !== null && count !== null) {
            console.log('[DXCTGM] Add item ' + itemId + ' x' + count);
        }
    }
    
    function completeStage() {
        console.log('[DXCTGM] Completing stage');
    }
    
    function killAllEnemies() {
        console.log('[DXCTGM] Killing all enemies');
    }
    
    function togglePanel() {
        GM.panelVisible = !GM.panelVisible;
        var panel = document.getElementById('dxct-gm-panel');
        if (panel) {
            panel.style.display = GM.panelVisible ? 'block' : 'none';
        }
        var fab = document.getElementById('dxct-gm-fab');
        if (fab) {
            fab.style.display = GM.panelVisible ? 'flex' : 'none';
        }
    }
    
    function toggleMenu() {
        GM.menuOpen = !GM.menuOpen;
        var panel = document.getElementById('dxct-gm-panel');
        if (panel) {
            panel.style.display = GM.menuOpen ? 'block' : 'none';
        }
    }
    
    function updateStatus(id, value) {
        var el = document.getElementById('status-' + id);
        if (el) {
            el.textContent = value ? 'ON' : 'OFF';
            el.className = 'status ' + (value ? 'on' : 'off');
        }
    }
    
    // Expose to global
    window.dxct_gm = {
        init: init,
        toggleOneHitKill: toggleOneHitKill,
        toggleInvincible: toggleInvincible,
        toggleInfiniteHp: toggleInfiniteHp,
        setStats: setStats,
        changeSpeed: changeSpeed,
        teleport: teleport,
        addItem: addItem,
        completeStage: completeStage,
        killAllEnemies: killAllEnemies,
        togglePanel: togglePanel,
        toggleMenu: toggleMenu
    };
    
    // Auto-init
    init();
    
})();
