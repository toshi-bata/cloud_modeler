var PORT = process.env.PORT || 8000;
const USE_HTTPS = process.env.CEW_USE_HTTPS || false;

var connect = require('connect');
var serveStatic = require('serve-static');
var http = require('http');
var https = require('https');
var fs = require('fs');

var dirsToServe = [];
dirsToServe.push(__dirname + "/.");     // For serving main index page in this catalog  
dirsToServe.push(__dirname + "/..")     // For serving the examples
dirsToServe.push(__dirname + "/../..")  // For documentation


function setHeaders(res, path) {
    res.setHeader("Access-Control-Allow-Origin", "*");
    res.setHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept, Authorization");
}


var connectApp = connect(); 

console.log("HttpServer serving files from:");
for (var i = 0; i < dirsToServe.length; i++) {
    console.log("  " + dirsToServe[i]);
    connectApp.use(serveStatic(dirsToServe[i], {"setHeaders": setHeaders}));
}

if (USE_HTTPS) {
    // 
    // HTTPS server
    console.log("Creating HTTPS server on port " + PORT);

    let privateKey = fs.readFileSync(__dirname + '/MyDomain_private.key', 'utf8');
    let certificate = fs.readFileSync(__dirname + '/MyDomain.crt', 'utf8');
    let credentials = { key: privateKey, cert: certificate };
    webServer = https.createServer(credentials, connectApp);
}
else {
    //
    // HTTP server
    console.log("Creating HTTP server on port " + PORT);

    webServer = http.createServer(connectApp);
}

webServer.listen(PORT, function() {
    console.log('HttpServer listening on port ' + PORT + '...');
});
