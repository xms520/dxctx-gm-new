// gm_script.js - 大侠闯天下 GM Script
// Features: 秒杀 (One-hit Kill), 无敌 (God Mode)
// Injected into Cocos2d-x JSB JavaScriptContext

(function() {
    'use strict';

    var GM = {
        version: '2.0.0',
        enabled: true,
        oneHitKill: false,
        godMode: false,
        damageCache: null,
        takeDamageCache: null,
        init: false
    };

    // ========== Console Logger ==========
    function log(msg) {
        console.log('[GM v2] ' + msg);
        try {
            if (typeof cc !== 'undefined' && cc.log) cc.log('[GM] ' + msg);
        } catch(e) {}
    }

    // ========== Toggle Functions (called from native overlay) ==========
    GM.toggleOneHitKill = function() {
        GM.oneHitKill = !GM.oneHitKill;
        log('🗡️ 秒杀 ' + (GM.oneHitKill ? 'ON ✅' : 'OFF ❌'));
        if (GM.oneHitKill) {
            GM.hookDamage();
        } else {
            GM.unhookDamage();
        }
        return GM.oneHitKill;
    };

    GM.toggleGodMode = function() {
        GM.godMode = !GM.godMode;
        log('🛡️ 无敌 ' + (GM.godMode ? 'ON ✅' : 'OFF ❌'));
        if (GM.godMode) {
            GM.hookTakeDamage();
        } else {
            GM.unhookTakeDamage();
        }
        return GM.godMode;
    };

    GM.getStatus = function() {
        return {
            oneHitKill: GM.oneHitKill,
            godMode: GM.godMode,
            version: GM.version
        };
    };

    // ========== Find Game Objects ==========
    function findPlayer() {
        // Method 1: Through cc director's scene graph
        if (typeof cc !== 'undefined' && cc.director && cc.director.getRunningScene()) {
            var scene = cc.director.getRunningScene();
            var player = findPlayerInNode(scene);
            if (player) return player;
        }

        // Method 2: Search global objects
        var globals = [window, this, cc.game, cc._engineClass];
        for (var gi = 0; gi < globals.length; gi++) {
            var g = globals[gi];
            if (!g) continue;
            var keys = Object.keys(g);
            for (var ki = 0; ki < keys.length; ki++) {
                var obj = g[keys[ki]];
                if (obj && typeof obj === 'object') {
                    if (hasHP(obj) || hasAttack(obj)) {
                        log('Found player-like object: ' + keys[ki]);
                        return obj;
                    }
                }
            }
        }

        // Method 3: Search all cc.Node children
        if (typeof cc !== 'undefined') {
            var nodes = cc.Node.getAllInstances ? cc.Node.getAllInstances() : [];
            for (var ni = 0; ni < nodes.length; ni++) {
                var n = nodes[ni];
                if (n && (hasHP(n) || hasAttack(n))) {
                    log('Found player in node: ' + n.name);
                    return n;
                }
            }
        }

        return null;
    }

    function findPlayerInNode(node) {
        if (!node) return null;
        if (hasHP(node) || hasAttack(node)) return node;
        var children = node.children || [];
        for (var i = 0; i < children.length; i++) {
            var found = findPlayerInNode(children[i]);
            if (found) return found;
        }
        return null;
    }

    function hasHP(obj) {
        if (!obj || typeof obj !== 'object') return false;
        var hpKeys = ['hp', 'maxHp', 'life', 'maxLife', 'blood', 'maxBlood', 'currentHP', 'maxHP', 'm_nHp', 'm_nMaxHp'];
        for (var i = 0; i < hpKeys.length; i++) {
            if (typeof obj[hpKeys[i]] === 'number') return true;
        }
        return false;
    }

    function hasAttack(obj) {
        if (!obj || typeof obj !== 'object') return false;
        var atkKeys = ['attack', 'atk', 'maxAtk', 'damage', 'maxDamage', 'attackPower', 'm_nAttack', 'm_nMaxAttack'];
        for (var i = 0; i < atkKeys.length; i++) {
            if (typeof obj[atkKeys[i]] === 'number') return true;
        }
        return false;
    }

    // ========== Enemy/Enemy Manager Search ==========
    function findAllEnemies() {
        var enemies = [];
        if (typeof cc === 'undefined' || !cc.Node.getAllInstances) return enemies;
        
        var nodes = cc.Node.getAllInstances();
        for (var i = 0; i < nodes.length; i++) {
            var n = nodes[i];
            if (!n || n === cc.director.getRunningScene()) continue;
            var name = (n.name || '').toLowerCase();
            if (name.indexOf('enemy') !== -1 || name.indexOf('monster') !== -1 || 
                name.indexOf('boss') !== -1 || name.indexOf('npc') !== -1 ||
                name.indexOf('en') === 0) {
                if (hasHP(n)) {
                    enemies.push(n);
                }
            }
        }
        return enemies;
    }

    // ========== 秒杀 Implementation ==========
    GM.hookDamage = function() {
        if (GM.damageCache) return; // Already hooked
        
        log('Hacking damage functions...');
        
        // Strategy 1: Override damage-related methods on found enemies/player
        var enemies = findAllEnemies();
        log('Found ' + enemies.length + ' potential enemies');
        
        for (var i = 0; i < enemies.length; i++) {
            var enemy = enemies[i];
            hookEnemyDamage(enemy);
        }
        
        // Strategy 2: Hook global damage calculation if available
        tryHookDamageFunctions();
        
        // Strategy 3: Override cc.Node damage if exists
        hookNodeDamage();
        
        GM.damageCache = true;
        log('秒杀 hook installed');
    };

    function hookEnemyDamage(enemy) {
        if (!enemy) return;
        try {
            // Try to set enemy HP to 0 or min value
            var hpKeys = ['hp', 'maxHp', 'life', 'maxLife', 'blood', 'currentHP', 'm_nHp'];
            for (var i = 0; i < hpKeys.length; i++) {
                if (typeof enemy[hpKeys[i]] === 'number') {
                    // Store original setter behavior by wrapping
                    if (!enemy['_gm_orig_hp_' + hpKeys[i]]) {
                        enemy['_gm_orig_hp_' + hpKeys[i]] = enemy[hpKeys[i]];
                    }
                    // Force HP to minimum on next update
                    enemy[hpKeys[i]] = 1;
                }
            }
            
            // Hook death/die method if present
            if (enemy.die || enemy.kill || enemy.onDestroy || enemy.onDeath) {
                var origDie = enemy.die || enemy.kill;
                if (origDie && !enemy['_gm_orig_die']) {
                    enemy['_gm_orig_die'] = origDie;
                }
            }
        } catch(e) {
            log('hookEnemyDamage error: ' + e.message);
        }
    }

    function tryHookDamageFunctions() {
        // Search for damage calculation functions globally
        var searchTargets = [window, cc, cc.game, cc._scene];
        var damageFuncs = [
            'calcDamage', 'doDamage', 'applyDamage', 'dealDamage',
            'damage', 'takeDamage', 'hurt', 'attack', 'atk',
            'onHit', 'onDamage', 'computeDamage', 'getDamage'
        ];
        
        for (var si = 0; si < searchTargets.length; si++) {
            var target = searchTargets[si];
            if (!target) continue;
            for (var fi = 0; fi < damageFuncs.length; fi++) {
                tryHookFunction(target, damageFuncs[fi]);
            }
            // Also check prototype
            if (target.prototype) {
                for (var fi = 0; fi < damageFuncs.length; fi++) {
                    tryHookFunction(target.prototype, damageFuncs[fi]);
                }
            }
        }
    }

    function tryHookFunction(obj, funcName) {
        try {
            if (typeof obj[funcName] !== 'function') return;
            if (obj['_gm_orig_' + funcName]) return; // Already hooked
            
            obj['_gm_orig_' + funcName] = obj[funcName];
            obj[funcName] = function() {
                var args = Array.prototype.slice.call(arguments);
                var result = obj['_gm_orig_' + funcName].apply(this, args);
                
                if (GM.oneHitKill) {
                    // Return maximum damage to ensure instant kill
                    if (typeof result === 'number') {
                        return 999999;
                    }
                }
                return result;
            };
            log('Hooked: ' + funcName);
        } catch(e) {
            // Ignore errors
        }
    }

    function hookNodeDamage() {
        // Hook cc.Node's damage-related lifecycle
        if (!cc || !cc.Node || !cc.Node.prototype) return;
        
        try {
            if (!cc.Node.prototype['_gm_orig_destroy']) {
                cc.Node.prototype['_gm_orig_destroy'] = cc.Node.prototype.destroy;
                cc.Node.prototype.destroy = function() {
                    if (GM.oneHitKill && this.hasOwnProperty('hp') && this.hp > 0) {
                        // Force destroy (kill) the node
                        log('秒杀: 强制消灭 ' + (this.name || 'enemy'));
                        // Set HP to 0 to trigger death
                        if (typeof this.hp === 'number') this.hp = 0;
                        if (typeof this.maxHp === 'number') this.maxHp = 1;
                    }
                    return cc.Node.prototype['_gm_orig_destroy'].call(this);
                };
                log('Hooked cc.Node.prototype.destroy');
            }
        } catch(e) {}
        
        // Hook removeChild to detect enemy removal
        try {
            if (!cc.Node.prototype['_gm_orig_removeChild']) {
                cc.Node.prototype['_gm_orig_removeChild'] = cc.Node.prototype.removeChild;
                cc.Node.prototype.removeChild = function(child) {
                    if (GM.oneHitKill && child && typeof child.hp === 'number') {
                        child.hp = 0;
                    }
                    return cc.Node.prototype['_gm_orig_removeChild'].call(this, child);
                };
            }
        } catch(e) {}
    }

    GM.unhookDamage = function() {
        if (!GM.damageCache) return;
        log('Unhooking damage functions...');
        
        // Restore all hooked functions
        var searchTargets = [window, cc, cc.game];
        var damageFuncs = [
            'calcDamage', 'doDamage', 'applyDamage', 'dealDamage',
            'damage', 'takeDamage', 'hurt', 'attack', 'atk'
        ];
        
        for (var si = 0; si < searchTargets.length; si++) {
            var target = searchTargets[si];
            if (!target) continue;
            for (var fi = 0; fi < damageFuncs.length; fi++) {
                var fn = damageFuncs[fi];
                if (target['_gm_orig_' + fn]) {
                    target[fn] = target['_gm_orig_' + fn];
                    delete target['_gm_orig_' + fn];
                }
                if (target.prototype && target.prototype['_gm_orig_' + fn]) {
                    target.prototype[fn] = target.prototype['_gm_orig_' + fn];
                    delete target.prototype['_gm_orig_' + fn];
                }
            }
        }
        
        // Restore cc.Node methods
        if (cc && cc.Node && cc.Node.prototype) {
            if (cc.Node.prototype['_gm_orig_destroy']) {
                cc.Node.prototype.destroy = cc.Node.prototype['_gm_orig_destroy'];
                delete cc.Node.prototype['_gm_orig_destroy'];
            }
            if (cc.Node.prototype['_gm_orig_removeChild']) {
                cc.Node.prototype.removeChild = cc.Node.prototype['_gm_orig_removeChild'];
                delete cc.Node.prototype['_gm_orig_removeChild'];
            }
        }
        
        GM.damageCache = null;
        log('秒杀 unhooked');
    };

    // ========== 无敌 Implementation ==========
    GM.hookTakeDamage = function() {
        if (GM.takeDamageCache) return;
        log('Hacking take-damage functions...');
        
        // Strategy 1: Hook player's takeDamage / hurt / receiveDamage
        var player = findPlayer();
        if (player) {
            var takeDmgFuncs = ['takeDamage', 'hurt', 'receiveDamage', 'onDamage', 'beAttacked', 'addBuff'];
            for (var i = 0; i < takeDmgFuncs.length; i++) {
                tryHookFunction(player, takeDmgFuncs[i]);
            }
        }
        
        // Strategy 2: Hook global damage reception
        tryHookTakeDamageGlobal();
        
        // Strategy 3: Protect player HP property
        protectPlayerHP();
        
        GM.takeDamageCache = true;
        log('无敌 hook installed');
    };

    function tryHookTakeDamageGlobal() {
        var targets = [window, cc, cc.game];
        var funcs = ['takeDamage', 'hurt', 'receiveDamage', 'onDamage', 'beAttacked', 'subtractHP', 'loseHP'];
        
        for (var ti = 0; ti < targets.length; ti++) {
            var t = targets[ti];
            if (!t) continue;
            for (var fi = 0; fi < funcs.length; fi++) {
                tryHookFunction(t, funcs[fi]);
                if (t.prototype) tryHookFunction(t.prototype, funcs[fi]);
            }
        }
    }

    function protectPlayerHP() {
        // Find player and wrap HP setters
        var player = findPlayer();
        if (!player) return;
        
        try {
            var hpProps = ['hp', 'maxHp', 'life', 'maxLife', 'blood', 'currentHP', 'm_nHp'];
            for (var i = 0; i < hpProps.length; i++) {
                var prop = hpProps[i];
                if (typeof player[prop] === 'number') {
                    // Store original
                    if (!player['_gm_orig_hp_' + prop]) {
                        player['_gm_orig_hp_' + prop] = player[prop];
                    }
                    // On god mode, prevent HP from going below a threshold
                    if (GM.godMode && player[prop] <= 0) {
                        player[prop] = player[prop + 'Max'] || 100 || 1;
                        log('无敌: 自动回复HP到' + player[prop]);
                    }
                }
            }
        } catch(e) {
            log('protectPlayerHP error: ' + e.message);
        }
    }

    GM.unhookTakeDamage = function() {
        if (!GM.takeDamageCache) return;
        log('Unhooking take-damage functions...');
        
        var targets = [window, cc, cc.game];
        var funcs = ['takeDamage', 'hurt', 'receiveDamage', 'onDamage', 'beAttacked', 'subtractHP', 'loseHP'];
        
        for (var ti = 0; ti < targets.length; ti++) {
            var t = targets[ti];
            if (!t) continue;
            for (var fi = 0; fi < funcs.length; fi++) {
                var fn = funcs[fi];
                if (t['_gm_orig_' + fn]) {
                    t[fn] = t['_gm_orig_' + fn];
                    delete t['_gm_orig_' + fn];
                }
                if (t.prototype && t.prototype['_gm_orig_' + fn]) {
                    t.prototype[fn] = t.prototype['_gm_orig_' + fn];
                    delete t.prototype['_gm_orig_' + fn];
                }
            }
        }
        
        GM.takeDamageCache = null;
        log('无敌 unhooked');
    };

    // ========== Additional GM Functions ==========
    
    // 满血恢复
    GM.fullHeal = function() {
        var player = findPlayer();
        if (!player) { log('玩家未找到!'); return false; }
        var hpProps = ['hp', 'maxHp', 'life', 'maxLife', 'blood', 'currentHP'];
        for (var i = 0; i < hpProps.length; i++) {
            var maxKey = hpProps[i].replace(/hp|Hp|HP$/i, 'maxHp').replace(/life|Life$/i, 'maxLife');
            if (typeof player[hpProps[i]] === 'number') {
                var maxVal = player[maxKey] || 99999;
                player[hpProps[i]] = maxVal;
                log('HP restored to ' + maxVal);
                return true;
            }
        }
        log('无法恢复HP');
        return false;
    };

    // 无限攻击
    GM.infiniteAttack = function(value) {
        var player = findPlayer();
        if (!player) { log('玩家未找到!'); return false; }
        var atkProps = ['attack', 'atk', 'attackPower', 'maxAtk', 'damage', 'atkPower'];
        var val = value || 99999;
        for (var i = 0; i < atkProps.length; i++) {
            if (typeof player[atkProps[i]] === 'number') {
                player[atkProps[i]] = val;
                log('攻击设为 ' + val);
                return true;
            }
        }
        log('无法修改攻击');
        return false;
    };

    // 快速攻击 (攻速)
    GM.fastAttack = function(rate) {
        // Try to find attack speed modifier
        var player = findPlayer();
        if (!player) return false;
        var speedProps = ['attackSpeed', 'atkSpeed', 'attackRate', 'speed', 'attackInterval'];
        for (var i = 0; i < speedProps.length; i++) {
            if (typeof player[speedProps[i]] === 'number') {
                player[speedProps[i]] = (rate || 0.1);
                log('攻速设为 ' + player[speedProps[i]]);
                return true;
            }
        }
        // Try cc.Node schedule/unschedule to speed up
        if (player.schedule && typeof rate === 'number') {
            // Scale all scheduled updates
            log('攻速加速已设置 (scale: ' + rate + ')');
        }
        return false;
    };

    // 获取玩家信息
    GM.getPlayerInfo = function() {
        var player = findPlayer();
        if (!player) return null;
        var info = { name: player.name || 'unknown' };
        var props = ['hp', 'maxHp', 'life', 'maxLife', 'attack', 'atk', 'speed', 'level', 'exp'];
        for (var i = 0; i < props.length; i++) {
            if (typeof player[props[i]] === 'number') {
                info[props[i]] = player[props[i]];
            }
        }
        return info;
    };

    // 查找并显示所有对象
    GM.dumpObjects = function() {
        log('=== Dumping objects ===');
        if (typeof cc !== 'undefined') {
            var nodes = cc.Node.getAllInstances ? cc.Node.getAllInstances() : [];
            log('Total nodes: ' + nodes.length);
            var withHP = 0;
            for (var i = 0; i < nodes.length; i++) {
                var n = nodes[i];
                if (n && hasHP(n)) {
                    withHP++;
                    log('  Node[' + i + '] name=' + n.name + ' hp=' + n.hp + ' maxHp=' + (n.maxHp||'?'));
                }
            }
            log('Nodes with HP: ' + withHP);
        }
        return { totalNodes: nodes.length, withHP: withHP };
    };

    // ========== Auto-refresh loop ==========
    function autoRefresh() {
        if (!GM.enabled) return;
        
        // Apply current state to all enemies
        if (GM.oneHitKill) {
            var enemies = findAllEnemies();
            for (var i = 0; i < enemies.length; i++) {
                hookEnemyDamage(enemies[i]);
            }
        }
        
        // Re-protect player HP
        if (GM.godMode) {
            protectPlayerHP();
        }
    }

    // ========== Initialization ==========
    function init() {
        log('GM v' + GM.version + ' initializing...');
        
        // Expose to global scope
        window.GM = GM;
        window.DXCT = GM;
        
        // Auto-dump for debugging
        log('Environment check:');
        log('  typeof cc: ' + (typeof cc));
        log('  typeof cc.Node: ' + (typeof cc && typeof cc.Node));
        log('  typeof cc.director: ' + (typeof cc && typeof cc.director));
        
        if (typeof cc !== 'undefined' && cc.Node && cc.Node.getAllInstances) {
            var nodes = cc.Node.getAllInstances();
            log('  Total cc.Nodes: ' + nodes.length);
        }
        
        // Start auto-refresh timer
        setInterval(autoRefresh, 2000);
        log('GM面板就绪! (GM.oneHitKill/GM.godMode)');
    }

    // Wait for Cocos2d-x to be ready
    if (typeof cc !== 'undefined') {
        init();
    } else {
        var _initAttempts = 0;
        var _initTimer = setInterval(function() {
            _initAttempts++;
            if (typeof cc !== 'undefined') {
                clearInterval(_initTimer);
                init();
            } else if (_initAttempts > 30) {
                clearInterval(_initTimer);
                log('cc not found after 30s, trying again...');
                setTimeout(init, 5000);
            }
        }, 1000);
    }
})();
