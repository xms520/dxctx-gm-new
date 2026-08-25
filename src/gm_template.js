// 大侠闯天下 GM调试模板 v1.0
// 通过运行时注入到游戏JavaScriptContext

(function() {
    'use strict';
    
    console.log('[DXCT GM] Initializing GM Debug Panel...');
    
    // ========== 全局状态 ==========
    var GM = {
        version: '1.0.0',
        enabled: true,
        playerData: null,
        flags: {
            oneHitKill: false,
            infiniteHP: false,
            speedHack: false,
            godMode: false
        }
    };
    
    // ========== 工具函数 ==========
    function log(msg) {
        console.log('[DXCT GM] ' + msg);
    }
    
    function showNotification(msg, type) {
        type = type || 'info';
        log(type.toUpperCase() + ': ' + msg);
        if (typeof cc !== 'undefined') {
            cc.log('[GM] ' + msg);
        }
    }
    
    // ========== 获取游戏对象 ==========
    function getPlayer() {
        var paths = [
            'Game.Player', 'game.Player',
            'CCGame.Player', 'cc.game.Player',
            'PlayerManager.Instance',
            'GameManager.Instance.Player',
            'window.player',
            'window.Game'
        ];
        for (var i = 0; i < paths.length; i++) {
            try {
                var obj = eval(paths[i]);
                if (obj && obj !== null && obj !== undefined) {
                    log('Found player at: ' + paths[i]);
                    return obj;
                }
            } catch(e) {
                // Try next path
            }
        }
        return null;
    }
    
    function getGameManager() {
        var paths = [
            'Game.GameManager',
            'game.GameManager',
            'GameManager.Instance',
            'CCGame.Instance'
        ];
        for (var i = 0; i < paths.length; i++) {
            try {
                var mgr = eval(paths[i]);
                if (mgr && mgr !== null) return mgr;
            } catch(e) {}
        }
        return null;
    }
    
    // ========== GM功能 ==========
    
    // 1. 一刀秒杀
    GM.oneHitKill = function() {
        GM.flags.oneHitKill = !GM.flags.oneHitKill;
        showNotification('一刀秒杀: ' + (GM.flags.oneHitKill ? '开启' : '关闭'));
        return GM.flags.oneHitKill;
    };
    
    // 2. 无敌模式
    GM.godMode = function() {
        GM.flags.godMode = !GM.flags.godMode;
        showNotification('无敌模式: ' + (GM.flags.godMode ? '开启' : '关闭'));
        return GM.flags.godMode;
    };
    
    // 3. 无限血量
    GM.infiniteHP = function() {
        GM.flags.infiniteHP = !GM.flags.infiniteHP;
        showNotification('无限血量: ' + (GM.flags.infiniteHP ? '开启' : '关闭'));
        return GM.flags.infiniteHP;
    };
    
    // 4. 修改血量
    GM.setHP = function(value) {
        var player = getPlayer();
        if (!player) { showNotification('玩家未找到!', 'error'); return false; }
        var hpProps = ['hp', 'maxHp', 'life', 'maxLife', 'blood', 'maxBlood', 'currentHP', 'maxHP'];
        for (var i = 0; i < hpProps.length; i++) {
            if (player[hpProps[i]] !== undefined) {
                player[hpProps[i]] = value;
                showNotification('血量设为: ' + value);
                return true;
            }
        }
        showNotification('未找到血量属性', 'error');
        return false;
    };
    
    // 5. 修改攻击
    GM.setAttack = function(value) {
        var player = getPlayer();
        if (!player) { showNotification('玩家未找到!', 'error'); return false; }
        var atkProps = ['attack', 'atk', 'maxAtk', 'damage', 'maxDamage', 'attackPower', 'atkPower'];
        for (var i = 0; i < atkProps.length; i++) {
            if (player[atkProps[i]] !== undefined) {
                player[atkProps[i]] = value;
                showNotification('攻击设为: ' + value);
                return true;
            }
        }
        showNotification('未找到攻击属性', 'error');
        return false;
    };
    
    // 6. 速度加速
    GM.speedHack = function(speed) {
        GM.flags.speedHack = true;
        var player = getPlayer();
        if (!player) { showNotification('玩家未找到!', 'error'); return false; }
        var speedProps = ['speed', 'moveSpeed', 'walkSpeed', 'runSpeed', 'moveSpeed'];
        for (var i = 0; i < speedProps.length; i++) {
            if (player[speedProps[i]] !== undefined) {
                player[speedProps[i]] = speed || 5;
                showNotification('速度设为: ' + (speed || 5));
                return true;
            }
        }
        showNotification('未找到速度属性', 'error');
        return false;
    };
    
    // 7. 传送
    GM.teleport = function(x, y) {
        var player = getPlayer();
        if (!player) { showNotification('玩家未找到!', 'error'); return false; }
        if (player.node && player.node.setPosition) {
            player.node.setPosition(x, y);
            showNotification('传送到: ' + x + ', ' + y);
            return true;
        }
        if (player.setPosition) {
            player.setPosition(x, y);
            showNotification('传送到: ' + x + ', ' + y);
            return true;
        }
        showNotification('无法传送', 'error');
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
                    if (mgr.addItem) { mgr.addItem(itemId, count); showNotification('添加物品: ' + itemId + ' x' + count); return true; }
                    if (mgr.add) { mgr.add(itemId, count); showNotification('添加物品: ' + itemId + ' x' + count); return true; }
                    if (mgr.AddItem) { mgr.AddItem(itemId, count); showNotification('添加物品: ' + itemId + ' x' + count); return true; }
                }
            } catch(e) {}
        }
        showNotification('ItemManager未找到', 'error');
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
                            showNotification('金币 +: ' + amount);
                            return true;
                        }
                    }
                }
            } catch(e) {}
        }
        showNotification('无法添加金币', 'error');
        return false;
    };
    
    // 10. 一键通关
    GM.autoWin = function() {
        showNotification('自动通关: 尝试中...');
        var paths = ['Game.LevelManager', 'game.LevelManager', 'LevelManager.Instance', 'GameManager.Instance'];
        for (var i = 0; i < paths.length; i++) {
            try {
                var mgr = eval(paths[i]);
                if (mgr) {
                    if (mgr.nextLevel) mgr.nextLevel();
                    if (mgr.completeLevel) mgr.completeLevel();
                    if (mgr.win) mgr.win();
                    if (mgr.onWin) mgr.onWin();
                    showNotification('通关成功');
                    return true;
                }
            } catch(e) {}
        }
        showNotification('自动通关失败', 'error');
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
                        // Hook damage calculation
                        if (mgr.calcDamage) {
                            mgr._origCalcDamage = mgr.calcDamage;
                            mgr.calcDamage = function() {
                                var args = arguments;
                                if (GM.flags.oneHitKill) {
                                    return 999999;
                                }
                                return mgr._origCalcDamage.apply(mgr, args);
                            };
                            showNotification(combatProps[i] + ' hooked!');
                            return true;
                        }
                    }
                } catch(e) {}
            }
        }
        showNotification('无法hook战斗', 'error');
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
                    showNotification('跳过剧情');
                    return true;
                }
            } catch(e) {}
        }
        showNotification('无法跳过剧情', 'error');
        return false;
    };
    
    // 13. 修改经验
    GM.setExp = function(value) {
        var player = getPlayer();
        if (!player) { showNotification('玩家未找到!', 'error'); return false; }
        var expProps = ['exp', 'EXP', 'experience', 'maxExp', 'level'];
        for (var i = 0; i < expProps.length; i++) {
            if (player[expProps[i]] !== undefined) {
                player[expProps[i]] = value;
                showNotification('经验设为: ' + value);
                return true;
            }
        }
        showNotification('未找到经验属性', 'error');
        return false;
    };
    
    // 14. 获取当前玩家信息
    GM.dumpPlayer = function() {
        var player = getPlayer();
        if (!player) { showNotification('玩家未找到!', 'error'); return; }
        log('Player object: ' + JSON.stringify(player));
        showNotification('玩家信息已输出到日志');
    };
    
    // 15. 获取全局对象
    GM.dumpGlobal = function() {
        log('Global keys: ' + Object.keys(window).join(', '));
        showNotification('全局对象已输出到日志');
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
        log('GM Panel initialized');
        log('Version: ' + GM.version);
        
        // 暴露到全局
        window.GM = GM;
        window.DXCT = GM;
        
        // 键盘监听
        if (typeof cc !== 'undefined' && cc.eventManager) {
            cc.eventManager.addListener({
                event: cc.EventListener.KEYBOARD,
                onKeyPressed: function(key, event) {
                    var keyName = 'F' + (key - 111);
                    if (GM.keys[keyName]) {
                        GM.keys[keyName]();
                    }
                }
            }, cc.game.canvas);
        }
        
        // 定时器检查玩家数据
        setInterval(function() {
            if (GM.enabled && !GM.playerData) {
                var player = getPlayer();
                if (player) {
                    GM.playerData = player;
                    log('Player data cached');
                }
            }
        }, 1000);
        
        showNotification('GM面板就绪! (F1-F9快捷键)');
    }
    
    // 延迟初始化确保游戏框架加载完成
    if (typeof cc !== 'undefined') {
        init();
    } else {
        setTimeout(init, 3000);
    }
})();
