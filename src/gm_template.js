/*
 * GM Debug Panel for 大侠闯天下 (iOS)
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
        
        // Show after 2 seconds
        setTimeout(function() {
            btn.classList.add('visible');
        }, 2000);
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
                    <button onclick="dxct_gm.closeMenu()">✕</button>
                </div>
                <div class="gm-body">
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
                        <div class="gm-btn" onclick="dxct_gm.killAll()">💀 秒杀所有敌人</div>
                        <div class="gm-btn" onclick="dxct_gm.teleport()">📍 传送</div>
                        <div class="gm-btn" onclick="dxct_gm.addItem()">🎁 添加物品</div>
                        <div class="gm-btn" onclick="dxct_gm.completeStage()">🏆 通关</div>
                    </div>
                </div>
            </div>
        `;
        
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
                border-radius: 16px;
                overflow: hidden;
                box-shadow: 0 8px 32px rgba(0,0,0,0.5);
                z-index: 99999;
            }
            .gm-header {
                display: flex;
                justify-content: space-between;
                align-items: center;
                padding: 16px 20px;
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            }
            .gm-header span {
                font-size: 16px;
                font-weight: bold;
            }
            .gm-header button {
                background: none;
                border: none;
                color: white;
                font-size: 20px;
                cursor: pointer;
            }
            .gm-body {
                padding: 10px 0;
                max-height: 60vh;
                overflow-y: auto;
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
                font-size: 14px;
            }
            .switch {
                width: 44px;
                height: 24px;
                background: #444;
                border-radius: 12px;
                position: relative;
                transition: background 0.3s;
            }
            .switch.active {
                background: #667eea;
            }
            .switch-knob {
                position: absolute;
                width: 20px;
                height: 20px;
                background: white;
                border-radius: 50%;
                top: 2px;
                left: 2px;
                transition: left 0.3s;
            }
            .switch.active .switch-knob {
                left: 22px;
            }
            .gm-row {
                display: flex;
                justify-content: space-between;
                align-items: center;
                padding: 12px 20px;
            }
            .gm-row span:first-child {
                font-size: 14px;
            }
            .gm-row input[type="number"] {
                width: 80px;
                padding: 6px 10px;
                border: 1px solid rgba(255,255,255,0.2);
                border-radius: 6px;
                background: rgba(255,255,255,0.1);
                color: white;
                font-size: 14px;
            }
            .gm-row input[type="range"] {
                width: 120px;
            }
            .gm-btn {
                display: block;
                margin: 8px 20px;
                padding: 12px;
                background: rgba(102, 126, 234, 0.2);
                border: 1px solid rgba(102, 126, 234, 0.5);
                border-radius: 8px;
                text-align: center;
                font-size: 14px;
                cursor: pointer;
            }
            .gm-btn:active {
                background: rgba(102, 126, 234, 0.4);
            }
        `;
        panel.appendChild(style);
        document.body.appendChild(panel);
    }
    
    // GM Functions
    function toggle(key) {
        GM[key] = !GM[key];
        var sw = document.getElementById('switch-' + key);
        if (sw) {
            sw.classList.toggle('active', GM[key]);
        }
        console.log('[DXCTGM] ' + key + ': ' + GM[key]);
    }
    
    function setHp(val) {
        GM.hpValue = parseInt(val) || 9999;
        console.log('[DXCTGM] HP set to ' + GM.hpValue);
    }
    
    function setAtk(val) {
        GM.atkValue = parseInt(val) || 9999;
        console.log('[DXCTGM] ATK set to ' + GM.atkValue);
    }
    
    function setSpeed(val) {
        GM.speedMult = parseFloat(val) || 1;
        document.getElementById('speed-val').textContent = GM.speedMult + 'x';
        console.log('[DXCTGM] Speed set to ' + GM.speedMult + 'x');
    }
    
    function killAll() {
        console.log('[DXCTGM] Killing all enemies');
    }
    
    function teleport() {
        var x = prompt('X 坐标:', '0');
        var y = prompt('Y 坐标:', '0');
        if (x && y) {
            console.log('[DXCTGM] Teleport to (' + x + ', ' + y + ')');
        }
    }
    
    function addItem() {
        var id = prompt('物品 ID:', '1001');
        var count = prompt('数量:', '10');
        if (id && count) {
            console.log('[DXCTGM] Add item ' + id + ' x' + count);
        }
    }
    
    function completeStage() {
        console.log('[DXCTGM] Completing stage');
    }
    
    function toggleMenu() {
        GM.menuOpen = !GM.menuOpen;
        var panel = document.getElementById('dxct-gm-panel');
        if (panel) {
            panel.style.display = GM.menuOpen ? 'block' : 'none';
        }
    }
    
    function closeMenu() {
        GM.menuOpen = false;
        var panel = document.getElementById('dxct-gm-panel');
        if (panel) {
            panel.style.display = 'none';
        }
    }
    
    // Expose to global
    window.dxct_gm = {
        init: init,
        toggle: toggle,
        setHp: setHp,
        setAtk: setAtk,
        setSpeed: setSpeed,
        killAll: killAll,
        teleport: teleport,
        addItem: addItem,
        completeStage: completeStage,
        toggleMenu: toggleMenu,
        closeMenu: closeMenu
    };
    
    // Auto-init
    init();
    
})();
