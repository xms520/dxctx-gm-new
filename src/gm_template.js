// 大侠闯天下 GM调试模板 v1.1 - 诊断版
// 通过运行时注入到游戏JavaScriptContext

(function() {
    'use strict';
    
    // 诊断日志
    function diag(msg) {
        console.log('[DXCT GM] ' + msg);
        // 尝试写入沙盒日志
        try {
            var fs = require('fs');
            var logPath = '/var/mobile/Library/Logs/dxct_gm_js.log';
            fs.appendFileSync(logPath, '[DXCT] ' + msg + '\n');
        } catch(e) {}
    }
    
    diag('Initializing GM Debug Panel v1.1...');
    
    // ========== 全局状态 ==========
    var GM = {
        version: '1.1.0',
        enabled: true,
        playerData: null,
        flags: {
            oneHitKill: false,
            infiniteHP: false,
            speedHack: false,
            godMode: false
        }
    };
    
    // ========== 诊断函数 ==========
    GM.dumpWorld = function() {
        diag('=== Dumping global objects ===');
        var keys = Object.keys(window || globalThis || this);
        diag('Window keys count: ' + keys.length);
        
        // 查找可能的游戏对象
        var gameObjects = [];
        for (var i = 0; i < keys.length; i++) {
            var key = keys[i];
            var val = window[key];
            if (val && typeof val === 'object') {
                // 检查是否是游戏相关对象
                var str = JSON.stringify(val).substring(0, 100);
                if (str.indexOf('hp') !== -1 || str.indexOf('attack') !== -1 || 
                    str.indexOf('player') !== -1 || str.indexOf('game') !== -1) {
                    gameObjects.push(key + ': ' + str);
                }
            }
        }
        
        diag('Potential game objects: ' + gameObjects.length);
        for (var j = 0; j < Math.min(10, gameObjects.length); j++) {
            diag('  ' + gameObjects[j]);
        }
        
        // 检查 cc 对象
        if (typeof cc !== 'undefined') {
            diag('cc object found!');
            diag('cc keys: ' + Object.keys(cc).slice(0, 20).join(', '));
        } else {
            diag('cc object NOT found');
        }
        
        // 检查 Game 对象
        if (typeof Game !== 'undefined') {
            diag('Game object found!');
            diag('Game keys: ' + Object.keys(Game).slice(0, 20).join(', '));
        } else {
            diag('Game object NOT found');
        }
        
        return { keys: keys.length, gameObjects: gameObjects.length };
    };
    
    GM.checkJSContext = function() {
        diag('=== Checking JSContext ===');
        diag('typeof window: ' + typeof window);
        diag('typeof global: ' + typeof global);
        diag('typeof globalThis: ' + typeof globalThis);
        
        // 尝试获取执行上下文
        try {
            var ctx = (window || globalThis || this);
            diag('Context type: ' + typeof ctx);
            diag('Context keys sample: ' + Object.keys(ctx).slice(0, 10).join(', '));
        } catch(e) {
            diag('Error checking context: ' + e.message);
        }
    };
    
    // ========== 游戏对象查找 ==========
    function findObject(paths) {
        for (var i = 0; i < paths.length; i++) {
            try {
                // 使用 eval 或间接访问
                var obj = eval(paths[i]);
                if (obj && obj !== null && obj !== undefined) {
                    diag('Found object at: ' + paths[i]);
                    return obj;
                }
            } catch(e) {
                // Try next path
            }
        }
        return null;
    }
    
    function getPlayer() {
        var paths = [
            'Game.Player', 'game.Player',
            'CCGame.Player', 'cc.game.Player',
            'PlayerManager.Instance',
            'GameManager.Instance.Player',
            'window.player',
            'window.Game'
        ];
        return findObject(paths);
    }
    
    function getGameManager() {
        var paths = [
            'Game.GameManager',
            'game.GameManager',
            'GameManager.Instance',
            'CCGame.Instance'
        ];
        return findObject(paths);
    }
    
    // ========== GM功能 ==========
    
    // 1. 一刀秒杀
    GM.oneHitKill = function() {
        GM.flags.oneHitKill = !GM.flags.oneHitKill;
        diag('一刀秒杀: ' + (GM.flags.oneHitKill ? '开启' : '关闭'));
        return GM.flags.oneHitKill;
    };
    
    // 2. 无敌模式
    GM.godMode = function() {
        GM.flags.godMode = !GM.flags.godMode;
        diag('无敌模式: ' + (GM.flags.godMode ? '开启' : '关闭'));
        return GM.flags.godMode;
    };
    
    // 3. 无限血量
    GM.infiniteHP = function() {
        GM.flags.infiniteHP = !GM.flags.infiniteHP;
        diag('无限血量: ' + (GM.flags.infiniteHP ? '开启' : '关闭'));
        return GM.flags.infiniteHP;
    };
    
    // 4. 修改血量
    GM.setHP = function(value) {
        var player = getPlayer();
        if (!player) { diag('玩家未找到!'); return false; }
        var hpProps = ['hp', 'maxHp', 'life', 'maxLife', 'blood', 'maxBlood', 'currentHP', 'maxHP'];
        for (var i = 0; i < hpProps.length; i++) {
            if (player[hpProps[i]] !== undefined) {
                player[hpProps[i]] = value;
                diag('血量设为: ' + value);
                return true;
            }
        }
        diag('未找到血量属性');
        return false;
    };
    
    // 5. 修改攻击
    GM.setAttack = function(value) {
        var player = getPlayer();
        if (!player) { diag('玩家未找到!'); return false; }
        var atkProps = ['attack', 'atk', 'maxAtk', 'damage', 'maxDamage', 'attackPower', 'atkPower'];
        for (var i = 0; i < atkProps.length; i++) {
            if (player[atkProps[i]] !== undefined) {
                player[atkProps[i]] = value;
                diag('攻击设为: ' + value);
                return true;
            }
        }
        diag('未找到攻击属性');
        return false;
    };
    
    // 6. 速度加速
    GM.speedHack = function(speed) {
        GM.flags.speedHack = true;
        var player = getPlayer();
        if (!player) { diag('玩家未找到!'); return false; }
        var speedProps = ['speed', 'moveSpeed', 'walkSpeed', 'runSpeed'];
        for (var i = 0; i < speedProps.length; i++) {
            if (player[speedProps[i]] !== undefined) {
                player[speedProps[i]] = speed || 5;
                diag('速度设为: ' + (speed || 5));
                return true;
            }
        }
        diag('未找到速度属性');
        return false;
    };
    
    // 7. 传送
    GM.teleport = function(x, y) {
        var player = getPlayer();
        if (!player) { diag('玩家未找到!'); return false; }
        if (player.node && player.node.setPosition) {
            player.node.setPosition(x, y);
            diag('传送到: ' + x + ', ' + y);
            return true;
        }
        if (player.setPosition) {
            player.setPosition(x, y);
            diag('传送到: ' + x + ', ' + y);
            return true;
        }
        diag('无法传送');
        return false;
    };
    
    // 8. 添加物品
    GM.addItem = function(itemId, count) {
        var paths = [
            'Game.ItemManager', 'game.ItemManager',
            'CCGame.ItemManager', 'ItemManager.Instance',
            'GameManager.Instance.ItemManager',
            'window.ItemManager'
        ];
        for (var i = 0; i < paths.length; i++) {
            try {
                var mgr = eval(paths[i]);
                if (mgr) {
                    if (mgr.addItem) { mgr.addItem(itemId, count); diag('添加物品: ' + itemId); return true; }
                    if (mgr.add) { mgr.add(itemId, count); diag('添加物品: ' + itemId); return true; }
                    if (mgr.AddItem) { mgr.AddItem(itemId, count); diag('添加物品: ' + itemId); return true; }
                }
            } catch(e) {}
        }
        diag('ItemManager未找到');
        return false;
    };
    
    // 9. 添加金币
    GM.addGold = function(amount) {
        var paths = ['Game.Player', 'game.Player', 'PlayerManager.Instance', 'GameManager.Instance'];
        for (var i = 0; i < paths.length; i++) {
            try {
                var obj = eval(paths[i]);
                if (obj) {
                    var goldProps = ['gold', 'coin', 'coins', 'money', 'currency', 'goldNum', 'coinNum'];
                    for (var j = 0; j < goldProps.length; j++) {
                        if (obj[goldProps[j]] !== undefined) {
                            obj[goldProps[j]] += amount;
                            diag('金币 +: ' + amount);
                            return true;
                        }
                    }
                }
            } catch(e) {}
        }
        diag('无法添加金币');
        return false;
    };
    
    // 10. 一键通关
    GM.autoWin = function() {
        diag('自动通关: 尝试中...');
        var paths = ['Game.LevelManager', 'game.LevelManager', 'LevelManager.Instance', 'GameManager.Instance'];
        for (var i = 0; i < paths.length; i++) {
            try {
                var mgr = eval(paths[i]);
                if (mgr) {
                    if (mgr.nextLevel) mgr.nextLevel();
                    if (mgr.completeLevel) mgr.completeLevel();
                    if (mgr.win) mgr.win();
                    if (mgr.onWin) mgr.onWin();
                    diag('通关成功');
                    return true;
                }
            } catch(e) {}
        }
        diag('自动通关失败');
        return false;
    };
    
    // 11. 战斗秒杀
    GM.battleOneHit = function() {
        if (typeof cc !== 'undefined') {
            var combatProps = ['CombatManager', 'BattleManager', 'FightManager', 'DamageManager'];
            for (var i = 0; i < combatProps.length; i++) {
                try {
                    var mgr = eval('Game.' + combatProps[i]);
                    if (mgr) {
                        if (mgr.calcDamage) {
                            mgr._origCalcDamage = mgr.calcDamage;
                            mgr.calcDamage = function() {
                                var args = arguments;
                                if (GM.flags.oneHitKill) {
                                    return 999999;
                                }
                                return mgr._origCalcDamage.apply(mgr, args);
                            };
                            diag(combatProps[i] + ' hooked!');
                            return true;
                        }
                    }
                } catch(e) {}
            }
        }
        diag('无法hook战斗');
        return false;
    };
    
    // 12. 跳过剧情
    GM.skipStory = function() {
        var paths = ['Game.StoryManager', 'game.StoryManager', 'StoryManager.Instance', 'Game.DialogManager'];
        for (var i = 0; i < paths.length; i++) {
            try {
                var mgr = eval(paths[i]);
                if (mgr) {
                    if (mgr.skip) mgr.skip();
                    if (mgr.next) mgr.next();
                    if (mgr.close) mgr.close();
                    diag('跳过剧情');
                    return true;
                }
            } catch(e) {}
        }
        diag('无法跳过剧情');
        return false;
    };
    
    // 13. 修改经验
    GM.setExp = function(value) {
        var player = getPlayer();
        if (!player) { diag('玩家未找到!'); return false; }
        var expProps = ['exp', 'EXP', 'experience', 'maxExp', 'level'];
        for (var i = 0; i < expProps.length; i++) {
            if (player[expProps[i]] !== undefined) {
                player[expProps[i]] = value;
                diag('经验设为: ' + value);
                return true;
            }
        }
        diag('未找到经验属性');
        return false;
    };
    
    // 14. 获取当前玩家信息
    GM.dumpPlayer = function() {
        var player = getPlayer();
        if (!player) { diag('玩家未找到!'); return; }
        diag('Player: ' + JSON.stringify(player));
    };
    
    // 15. 获取全局对象
    GM.dumpGlobal = function() {
        GM.dumpWorld();
    };
    
    // ========== 快捷按键 ==========
    GM.keys = {};
    GM.keys['F1'] = GM.oneHitKill;
    GM.keys['F2'] = GM.godMode;
    GM.keys['F3'] = GM.infiniteHP;
    GM.keys['F4'] = GM.speedHack;
    GM.keys['F5'] = GM.autoWin;
    GM.keys['F6'] = GM.battleOneHit;
    GM.keys['F7'] = GM.skipStory;
    GM.keys['F8'] = GM.dumpPlayer;
    GM.keys['F9'] = GM.dumpGlobal;
    
    // ========== 初始化 ==========
    function init() {
        diag('GM Panel initialized');
        diag('Version: ' + GM.version);
        
        // 暴露到全局
        window.GM = GM;
        window.DXCT = GM;
        
        // 自动运行诊断
        diag('Running diagnostics...');
        GM.checkJSContext();
        
        // 键盘监听
        if (typeof cc !== 'undefined' && cc.eventManager) {
            try {
                cc.eventManager.addListener({
                    event: cc.EventListener.KEYBOARD,
                    onKeyPressed: function(key, event) {
                        var keyName = 'F' + (key - 111);
                        if (GM.keys[keyName]) {
                            GM.keys[keyName]();
                        }
                    }
                }, cc.game.canvas);
                diag('Keyboard listener installed');
            } catch(e) {
                diag('Failed to install keyboard listener: ' + e.message);
            }
        } else {
            diag('cc.eventManager not available');
        }
        
        // 定时器检查玩家数据
        setInterval(function() {
            if (GM.enabled && !GM.playerData) {
                var player = getPlayer();
                if (player) {
                    GM.playerData = player;
                    diag('Player data cached');
                }
            }
        }, 1000);
        
        diag('GM面板就绪! (F1-F9快捷键)');
    }
    
    // 延迟初始化确保游戏框架加载完成
    if (typeof cc !== 'undefined') {
        init();
    } else {
        setTimeout(init, 3000);
    }
})();
