// 最简诊断GM脚本 - 测试注入是否工作
(function() {
    'use strict';
    
    // 尝试多种方式写入日志
    function writeLog(msg) {
        console.log('[DXCT] ' + msg);
        
        // 方法1: 尝试fs模块
        try {
            var fs = require('fs');
            var path = '/var/mobile/Library/Logs/dxct_test.log';
            var fd = fs.open(path, fs.O_WRONLY | fs.O_CREAT | fs.O_APPEND, 0644);
            if (fd >= 0) {
                fs.write(fd, '[DXCT] ' + msg + '\n', null, 'utf8');
                fs.close(fd);
            }
        } catch(e) {}
        
        // 方法2: 尝试window对象
        try {
            if (window.DXCT_LOG) {
                window.DXCT_LOG.push('[DXCT] ' + msg);
            } else {
                window.DXCT_LOG = [['DXCT] ' + msg];
            }
        } catch(e) {}
    }
    
    writeLog('=== GM Test Script Loaded ===');
    writeLog('Timestamp: ' + new Date().toISOString());
    
    // 检查环境
    writeLog('typeof window: ' + typeof window);
    writeLog('typeof global: ' + typeof global);
    writeLog('typeof cc: ' + typeof cc);
    writeLog('typeof Game: ' + typeof Game);
    
    // 列出window keys
    if (typeof window !== 'undefined') {
        var keys = Object.keys(window);
        writeLog('Window keys count: ' + keys.length);
        writeLog('Sample keys: ' + keys.slice(0, 10).join(', '));
    }
    
    // 检查游戏对象
    if (typeof cc !== 'undefined') {
        writeLog('cc object found!');
        writeLog('cc keys: ' + Object.keys(cc).slice(0, 20).join(', '));
    }
    
    if (typeof Game !== 'undefined') {
        writeLog('Game object found!');
        writeLog('Game keys: ' + Object.keys(Game).slice(0, 20).join(', '));
    }
    
    // 尝试查找玩家
    var playerPaths = ['Game.Player', 'game.Player', 'window.player', 'PlayerManager'];
    for (var i = 0; i < playerPaths.length; i++) {
        try {
            var obj = eval(playerPaths[i]);
            if (obj) {
                writeLog('Found: ' + playerPaths[i]);
                writeLog('Object keys: ' + Object.keys(obj).slice(0, 20).join(', '));
                break;
            }
        } catch(e) {}
    }
    
    writeLog('=== Diagnostic Complete ===');
    
    // 暴露到全局
    window.DXCT_TEST = {
        log: function(msg) { writeLog(msg); },
        dump: function() {
            writeLog('=== Full Dump ===');
            writeLog('window keys: ' + Object.keys(window).join(', '));
            if (typeof cc !== 'undefined') {
                writeLog('cc keys: ' + Object.keys(cc).join(', '));
            }
            if (typeof Game !== 'undefined') {
                writeLog('Game keys: ' + Object.keys(Game).join(', '));
            }
        }
    };
    
    writeLog('DXCT_TEST exposed to window');
})();
