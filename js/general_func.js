function browserLanguage() {
    try {
        return (navigator.browserLanguage || navigator.language || navigator.userLanguage).substr(0,2)
    }
        catch(e) {
        return undefined;
    }
}

function unitVector (P1, P2, l) {
    var dx = P2.x - P1.x;
    var dy = P2.y - P1.y;
    var dz = P2.z - P1.z;
    l = Math.sqrt(dx * dx + dy * dy + dz * dz);
    return {x: dx / l, y: dy / l, z: dz / l};
}

function exportAsJson(str, filePath) {
    return new Promise(function (resolve, reject) {
        $.ajax({
            url: "../common/php/exportJson.php",
            type: "post",
            dataType: 'json',
            data: {
                data: str,
                filePath: filePath
            }
        }).done(function (response) {
            resolve();
        }).fail(function (response) {
            resolve();
        });
    });
}

function getURLArgument(name) {
    if (1 < document.location.search.length) {
        var query = document.location.search.substring(1);
        var parameters = query.split('&');
        var result = new Object();
        for (var i = 0; i < parameters.length; i++) {
            var element = parameters[i].split('=');
            var paramName = decodeURIComponent(element[0]);
            var paramValue = decodeURIComponent(element[1]);
            result[paramName] = decodeURIComponent(paramValue);
        }
        return result[name];
    }
}

function htmlspecialchars(ch) {
    ch = ch.replace(/&/g,"&amp;");
    ch = ch.replace(/"/g,"&quot;");
    ch = ch.replace(/'/g,"&#039;");
    ch = ch.replace(/</g,"&lt;");
    ch = ch.replace(/>/g,"&gt;");
    return ch ;
} 

function create_UUID(){
    var dt = new Date().getTime();
    var uuid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
        var r = (dt + Math.random()*16)%16 | 0;
        dt = Math.floor(dt/16);
        return (c=='x' ? r :(r&0x3|0x8)).toString(16);
    });
    return uuid;
}