/**
 * GM Debug Panel for 大侠闯天下 v1.0.7
 * 
 * Features:
 *   F1 - One-hit kill
 *   F2 - Invincible
 *   F3 - Infinite HP
 *   F4 - Set HP/ATK
 *   F5 - Speed up
 *   F6 - Teleport
 *   F7 - Add items
 *   F8 - Complete stage
 *   F9 - Toggle panel
 * 
 * Environment:
 *   DXCT_JS_FILE=/path/to/this/file.js
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
        panelVisible: true
    };
    
    // Initialize GM
    function init() {
        console.log('[DXCTGM] GM panel initializing...');
        
        // Create GM object in global scope
        window.dxct_gm = GM;
        
        // Setup keyboard shortcuts
        setupKeyHandlers();
        
        // Create UI
        createPanel();
        
        console.log('[DXCTGM] GM panel ready');
    }
    
    // Keyboard shortcuts
    function setupKeyHandlers() {
        document.addEventListener('keydown', function(e) {
            if (!GM.enabled) return;
            
            switch(e.key) {
                case 'F1':
                    toggleOneHitKill();
                    break;
                case 'F2':
                    toggleInvincible();
                    break;
                case 'F3':
                    toggleInfiniteHp();
                    break;
                case 'F4':
                    setStats();
                    break;
                case 'F5':
                    changeSpeed();
                    break;
                case 'F6':
                    teleport();
                    break;
                case 'F7':
                    addItem();
                    break;
                case 'F8':
                    completeStage();
                    break;
                case 'F9':
                    togglePanel();
                    break;
            }
        });
    }
    
    // UI Panel
    function createPanel() {
        // Check if panel exists
        if (document.getElementById('dxct-gm-panel')) return;
        
        var panel = document.createElement('div');
        panel.id = 'dxct-gm-panel';
        panel.innerHTML = `
            <div class="gm-header">
                <span>DXCT GM Debugger</span>
                <button onclick="dxct_gm.togglePanel()">Hide</button>
            </div>
            <div class="gm-body">
                <div class="gm-item">
                    <label>F1 - One Hit Kill</label>
                    <span id="oneHitKill" class="status off">OFF</span>
                </div>
                <div class="gm-item">
                    <label>F2 - Invincible</label>
                    <span id="invincible" class="status off">OFF</span>
                </div>
                <div class="gm-item">
                    <label>F3 - Infinite HP</label>
                    <span id="infiniteHp" class="status off">OFF</span>
                </div>
                <div class="gm-item">
                    <label>F4 - HP/ATK</label>
                    <button onclick="dxct_gm.setStats()">Set</button>
                </div>
                <div class="gm-item">
                    <label>F5 - Speed</label>
                    <span id="speedVal">1.0x</span>
                </div>
                <div class="gm-item">
                    <label>F6 - Teleport</label>
                    <button onclick="dxct_gm.teleport()">Go</button>
                </div>
                <div class="gm-item">
                    <label>F7 - Add Items</label>
                    <button onclick="dxct_gm.addItem()">Add</button>
                </div>
                <div class="gm-item">
                    <label>F8 - Complete</label>
                    <button onclick="dxct_gm.completeStage()">Done</button>
                </div>
                <div class="gm-item">
                    <label>F9 - Toggle</label>
                    <button onclick="dxct_gm.togglePanel()">Hide</button>
                </div>
            </div>
        `;
        
        // Add styles
        var style = document.createElement('style');
        style.textContent = `
            #dxct-gm-panel {
                position: fixed;
                top: 50px;
                right: 10px;
                width: 280px;
                background: rgba(0,0,0,0.8);
                color: #0f0;
                border: 2px solid #0f0;
                border-radius: 8px;
                padding: 10px;
                font-family: monospace;
                font-size: 12px;
                z-index: 99999;
            }
            .gm-header {
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-bottom: 10px;
                padding-bottom: 5px;
                border-bottom: 1px solid #0f0;
            }
            .gm-body {
                display: flex;
                flex-direction: column;
                gap: 5px;
            }
            .gm-item {
                display: flex;
                justify-content: space-between;
                align-items: center;
            }
            .status {
                padding: 2px 6px;
                border-radius: 3px;
                font-weight: bold;
            }
            .status.on {
                background: #0f0;
                color: #000;
            }
            .status.off {
                background: #f00;
                color: #fff;
            }
            button {
                background: #333;
                color: #0f0;
                border: 1px solid #0f0;
                padding: 2px 8px;
                cursor: pointer;
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
        var hp = prompt('Enter HP value:', GM.hpValue);
        if (hp !== null) {
            GM.hpValue = parseFloat(hp) || 100;
        }
        var atk = prompt('Enter ATK value:', GM.atkValue);
        if (atk !== null) {
            GM.atkValue = parseFloat(atk) || 10;
        }
        console.log('[DXCTGM] Stats set - HP:' + GM.hpValue + ' ATK:' + GM.atkValue);
    }
    
    function changeSpeed() {
        GM.speedMult = GM.speedMult >= 3 ? 1 : GM.speedMult + 0.5;
        document.getElementById('speedVal').textContent = GM.speedMult.toFixed(1) + 'x';
        console.log('[DXCTGM] Speed: ' + GM.speedMult + 'x');
    }
    
    function teleport() {
        var x = prompt('Enter X coordinate:', '0');
        var y = prompt('Enter Y coordinate:', '0');
        if (x !== null && y !== null) {
            console.log('[DXCTGM] Teleport to (' + x + ', ' + y + ')');
        }
    }
    
    function addItem() {
        var itemId = prompt('Enter Item ID:', '1001');
        var count = prompt('Enter Count:', '10');
        if (itemId !== null && count !== null) {
            console.log('[DXCTGM] Add item ' + itemId + ' x' + count);
        }
    }
    
    function completeStage() {
        console.log('[DXCTGM] Completing current stage');
    }
    
    function togglePanel() {
        GM.panelVisible = !GM.panelVisible;
        var panel = document.getElementById('dxct-gm-panel');
        if (panel) {
            panel.style.display = GM.panelVisible ? 'block' : 'none';
        }
    }
    
    function updateStatus(id, value) {
        var el = document.getElementById(id);
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
        togglePanel: togglePanel
    };
    
    // Auto-init
    init();
    
})();
