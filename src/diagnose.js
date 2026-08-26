// 大侠闯天下 GM诊断脚本
// 用于检测游戏JS环境并输出关键信息

(function() {
    'use strict';
    
    console.log('[DXCT DIAG] Starting diagnostic...');
    
    var results = {
        timestamp: new Date().toISOString(),
        windowKeys: [],
        globalKeys: [],
        gameObjects: [],
        ccExists: false,
        gameExists: false,
        playerFound: false,
        hpProps: [],
        attackProps: []
    };
    
    // 检查window对象
    if (typeof window !== 'undefined') {
        results.windowKeys = Object.keys(window).slice(0, 50);
    }
    
    // 检查global对象
    if (typeof global !== 'undefined') {
        results.globalKeys = Object.keys(global).slice(0, 50);
    }
    
    // 检查cc对象
    if (typeof cc !== 'undefined') {
        results.ccExists = true;
        console.log('[DXCT DIAG] cc object found!');
        results.ccKeys = Object.keys(cc).slice(0, 30);
    }
    
    // 检查Game对象
    if (typeof Game !== 'undefined') {
        results.gameExists = true;
        console.log('[DXCT DIAG] Game object found!');
        results.gameKeys = Object.keys(Game).slice(0, 30);
    }
    
    // 查找玩家对象
    var playerPaths = [
        'Game.Player', 'game.Player',
        'CCGame.Player', 'cc.game.Player',
        'PlayerManager.Instance',
        'GameManager.Instance.Player',
        'window.player',
        'window.Game'
    ];
    
    for (var i = 0; i < playerPaths.length; i++) {
        try {
            var player = eval(playerPaths[i]);
            if (player && typeof player === 'object') {
                results.playerFound = true;
                results.playerPath = playerPaths[i];
                results.playerKeys = Object.keys(player).slice(0, 50);
                
                // 查找血量属性
                var hpProps = ['hp', 'maxHp', 'life', 'maxLife', 'blood', 'maxBlood', 'currentHP', 'maxHP'];
                for (var j = 0; j < hpProps.length; j++) {
                    if (player[hpProps[j]] !== undefined) {
                        results.hpProps.push(hpProps[j] + '=' + player[hpProps[j]]);
                    }
                }
                
                // 查找攻击属性
                var atkProps = ['attack', 'atk', 'maxAtk', 'damage', 'maxDamage', 'attackPower', 'atkPower'];
                for (var k = 0; k < atkProps.length; k++) {
                    if (player[atkProps[k]] !== undefined) {
                        results.attackProps.push(atkProps[k] + '=' + player[atkProps[k]]);
                    }
                }
                
                console.log('[DXCT DIAG] Player found at: ' + playerPaths[i]);
                console.log('[DXCT DIAG] Player keys: ' + results.playerKeys.join(', '));
                break;
            }
        } catch(e) {
            // Try next path
        }
    }
    
    // 查找游戏管理器
    var mgrPaths = [
        'Game.GameManager',
        'game.GameManager',
        'GameManager.Instance',
        'CCGame.Instance'
    ];
    
    for (var m = 0; m < mgrPaths.length; m++) {
        try {
            var mgr = eval(mgrPaths[m]);
            if (mgr) {
                results.gameObjects.push(mgrPaths[m] + ': ' + typeof mgr);
            }
        } catch(e) {}
    }
    
    // 输出结果
    console.log('[DXCT DIAG] === Results ===');
    console.log(JSON.stringify(results, null, 2));
    
    // 同时写入文件
    try {
        var fs = require('fs');
        var logPath = '/var/mobile/Library/Logs/dxct_diag.json';
        fs.writeFileSync(logPath, JSON.stringify(results, null, 2));
        console.log('[DXCT DIAG] Results written to: ' + logPath);
    } catch(e) {
        console.log('[DXCT DIAG] Failed to write log: ' + e.message);
    }
    
    console.log('[DXCT DIAG] Diagnostic complete!');
})();
