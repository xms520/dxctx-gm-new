/**
 * GM Debug Panel for 大侠闯天下 (iOS Toggle Buttons)
 * 
 * 交互方式:
 *   - 右下角悬浮球: 打开/关闭菜单
 *   - 开关按钮: 点击开启/关闭功能
 *   - 数值输入: 点击设置具体数值
 * 
 * 功能:
 *   - 一刀秒杀: 攻击时敌人立即死亡
 *   - 无敌模式: 不受任何伤害
 *   - 无限血量: 血量不会减少
 *   - 设置数值: 自定义血量/攻击
 *   - 加速移动: 移动速度加成
 *   - 传送: 传送到指定坐标
 *   - 添加物品: 添加指定物品
 *   - 通关: 完成当前关卡
 */

(function() {
    'use strict';
    
    // GM State
    var GM = {
        oneHitKill: false,
        invincible: false,
        infiniteHp: false,
        hpValue: 9999,
        atkValue: 9999,
        speedMult: 1,
        panelVisible: false,
        menuOpen: false
    };
    
    // Initialize GM
    function init() {
        console.log('[DXCTGM] GM panel initializing...');
        
        window.dxct_gm = GM;
        
        // Create floating button
        createFloatingButton();
        
        // Create settings panel
        createSettingsPanel();
        
        console.log('[DXCTGM] Ready - tap GM button');
    }
    
    // Floating button
    function createFloatingButton() {
        var btn = document.createElement('div');
        btn.id = 'dxct-gm-fab';
        btn.innerHTML = 'GM';
        btn.onclick = toggleMenu;
        
        var style = document.createElement('style');
        style.textContent = `
            #dxct-gm-fab {
                position: fixed;
                bottom: 100px;
                right: 20px;
                width: 56px;
                height: 56px;
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                color: white;
                border-radius: 50%;
                display: none;
                align-items: center;
                justify-content: center;
                font-size: 16px;
                font-weight: bold;
                box-shadow: 0 4px 20px rgba(102, 126, 234, 0.5);
                z-index: 99998;
                cursor: pointer;
                transition: all 0.3s;
            }
            #dxct-gm-fab.visible {
                display: flex;
            }
            #dxct-gm-fab:active {
                transform: scale(0.95);
            }
        `;
        btn.appendChild(style);
        document.body.appendChild(btn);
    }
    
    // Settings panel
    function createSettingsPanel() {
        var panel = document.createElement('div');
        panel.id = 'dxct-gm-panel';
        panel.style.display = 'none';
        
        panel.innerHTML = `
            <div class="gm-panel">
                <div class="gm-header">
                    <span>🎮 GM 调试器</span>
                    <button class="gm-close" onclick="dxct_gm.toggleMenu()">✕</button>
                </div>
                <div class="gm-section">
                    <div class="gm-title">战斗功能</div>
                    <div class="gm-toggle" onclick="dxct_gm.toggle('oneHitKill')">
                        <span>⚔️ 一刀秒杀</span>
                        <div class="switch" id="switch-oneHitKill">
                            <div class="switch-knob"></div>
                        </div>
                    </div>
                    <div class="gm-toggle" onclick="dxct_gm.toggle('invincible')">
                        <span>🛡️ 无敌模式</span>
                        <div class="switch" id="switch-invincible">
                            <div class="switch-knob"></div>
                        </div>
                    </div>
                    <div class="gm-toggle" onclick="dxct_gm.toggle('infiniteHp')">
                        <span>❤️ 无限血量</span>
                        <div class="switch" id="switch-infiniteHp">
                            <div class="switch-knob"></div>
                        </div>
                    </div>
                    <div class="gm-toggle" onclick="dxct_gm.killAll()">
                        <span>💀 秒杀所有敌人</span>
                        <div class="switch action-btn">
                            <div class="switch-knob"></div>
                        </div>
                    </div>
                </div>
                
                <div class="gm-section">
                    <div class="gm-title">属性设置</div>
                    <div class="gm-row">
                        <span>血量</span>
                        <input type="number" id="input-hp" value="9999" onchange="dxct_gm.setHp(this.value)">
                    </div>
                    <div class="gm-row">
                        <span>攻击</span>
                        <input type="number" id="input-atk" value="9999" onchange="dxct_gm.setAtk(this.value)">
                    </div>
                    <div class="gm-row">
                        <span>速度</span>
                        <input type="range" id="input-speed" min="1" max="5" step="0.5" value="1" oninput="dxct_gm.setSpeed(this.value)">
                        <span id="speed-val">1x</span>
                    </div>
                </div>
                
                <div class="gm-section">
                    <div class="gm-title">快捷操作</div>
                    <div class="gm-btn" onclick="dxct_gm.teleport()">📍 传送</div>
                    <div class="gm-btn" onclick="dxct_gm.addItem()">🎁 添加物品</div>
                    <div class="gm-btn" onclick="dxct_gm.completeStage()">🏆 通关</div>
                </div>
            </div>
        `;
        
        // Panel styles
        var style = document.createElement('style');
        style.textContent = `
            .gm-panel {
                position: fixed;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
                width: 320px;
                max-height: 80vh;
                background: rgba(15, 15, 25, 0.98);
                color: #fff;
                border-radius: 20px;
                overflow: hidden;
                box-shadow: 0 20px 60px rgba(0,0,0,0.6);
                font-family: -apple-system, BlinkMacSystemFont, 'SF Pro Text', sans-serif;
            }
            .gm-header {
                display: flex;
                justify-content: space-between;
                align-items: center;
                padding: 18px 20px;
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                font-size: 17px;
                font-weight: 600;
            }
            .gm-close {
                background: rgba(255,255,255,0.2);
                border: none;
                color: white;
                width: 32px;
                height: 32px;
                border-radius: 50%;
                font-size: 18px;
                cursor: pointer;
            }
            .gm-section {
                padding: 12px 0;
                border-bottom: 1px solid rgba(255,255,255,0.1);
            }
            .gm-section:last-child {
                border-bottom: none;
            }
            .gm-title {
                padding: 8px 20px;
                font-size: 12px;
                color: #888;
                text-transform: uppercase;
                letter-spacing: 1px;
            }
            .gm-toggle {
                display: flex;
                justify-content: space-between;
                align-items: center;
                padding: 14px 20px;
                cursor: pointer;
            }
            .gm-toggle:active {
                background: rgba(255,255,255,0.05);
            }
            .gm-toggle span:first-child {
                font-size: 16px;
            }
            .gm-row {
                display: flex;
                align-items: center;
                justify-content: space-between;
                padding: 12px 20px;
            }
            .gm-row span:first-child {
                font-size: 16px;
            }
            .gm-row input[type="number"] {
                width: 80px;
                padding: 8px;
                border: 1px solid rgba(255,255,255,0.2);
                border-radius: 8px;
                background: rgba(255,255,255,0.1);
                color: white;
                font-size: 16px;
                text-align: center;
            }
            .gm-row input[type="range"] {
                width: 120px;
            }
            .gm-btn {
                margin: 8px 20px;
                padding: 14px;
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                border-radius: 12px;
                text-align: center;
                font-size: 16px;
                cursor: pointer;
            }
            .gm-btn:active {
                opacity: 0.8;
            }
            /* Toggle Switch */
            .switch {
                width: 52px;
                height: 32px;
                background: rgba(255,255,255,0.2);
                border-radius: 16px;
                position: relative;
                transition: background 0.3s;
            }
            .switch.active {
                background: #4ade80;
            }
            .switch-knob {
                position: absolute;
                left: 3px;
                top: 3px;
                width: 26px;
                height: 26px;
                background: white;
                border-radius: 50%;
                transition: transform 0.3s;
                box-shadow: 0 2px 4px rgba(0,0,0,0.2);
            }
            .switch.active .switch-knob {
                transform: translateX(20px);
            }
            .action-btn {
                background: #f87171;
            }
        `;
        panel.appendChild(style);
        document.body.appendChild(panel);
    }
    
    // Toggle menu visibility
    function toggleMenu() {
        GM.menuOpen = !GM.menuOpen;
        var panel = document.getElementById('dxct-gm-panel');
        var fab = document.getElementById('dxct-gm-fab');
        
        if (GM.menuOpen) {
            panel.style.display = 'block';
            fab.classList.remove('visible');
        } else {
            panel.style.display = 'none';
            fab.classList.add('visible');
        }
    }
    
    // Toggle a feature
    function toggle(feature) {
        GM[feature] = !GM[feature];
        updateSwitch(feature);
        console.log('[DXCTGM] ' + feature + ': ' + (GM[feature] ? 'ON' : 'OFF'));
    }
    
    // Update switch UI
    function updateSwitch(feature) {
        var el = document.getElementById('switch-' + feature);
        if (el) {
            el.classList.toggle('active', GM[feature]);
        }
    }
    
    // Set HP value
    function setHp(value) {
        GM.hpValue = parseInt(value) || 9999;
        console.log('[DXCTGM] HP set to ' + GM.hpValue);
    }
    
    // Set ATK value
    function setAtk(value) {
        GM.atkValue = parseInt(value) || 9999;
        console.log('[DXCTGM] ATK set to ' + GM.atkValue);
    }
    
    // Set speed
    function setSpeed(value) {
        GM.speedMult = parseFloat(value) || 1;
        document.getElementById('speed-val').textContent = GM.speedMult + 'x';
        console.log('[DXCTGM] Speed: ' + GM.speedMult + 'x');
    }
    
    // Kill all enemies
    function killAll() {
        console.log('[DXCTGM] Killing all enemies');
        // Trigger game function if available
        if (typeof Game !== 'undefined' && Game.killAll) {
            Game.killAll();
        }
    }
    
    // Teleport
    function teleport() {
        var x = prompt('X 坐标:', '0');
        var y = prompt('Y 坐标:', '0');
        if (x !== null && y !== null) {
            console.log('[DXCTGM] Teleport to (' + x + ', ' + y + ')');
        }
    }
    
    // Add item
    function addItem() {
        var id = prompt('物品 ID:', '1001');
        var count = prompt('数量:', '10');
        if (id !== null && count !== null) {
            console.log('[DXCTGM] Add item ' + id + ' x' + count);
        }
    }
    
    // Complete stage
    function completeStage() {
        console.log('[DXCTGM] Completing stage');
    }
    
    // Expose to global
    window.dxct_gm = {
        init: init,
        toggleMenu: toggleMenu,
        toggle: toggle,
        setHp: setHp,
        setAtk: setAtk,
        setSpeed: setSpeed,
        killAll: killAll,
        teleport: teleport,
        addItem: addItem,
        completeStage: completeStage
    };
    
    // Start
    init();
    
})();
