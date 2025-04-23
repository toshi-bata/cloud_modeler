import * as Communicator from "../hoops-web-viewer.mjs";
export function createViewer(viewerMode, modelName, containerId, reverseProxy, modelDirArr) {
    return new Promise(function (resolve, reject) {
        if (viewerMode == "SCS" || viewerMode == "scs") {
            var scsFileName = modelName;
            var UP = modelName.toUpperCase();
            if(UP.indexOf('.SCS') == -1)
                scsFileName = "model_data/" + modelName + ".scs";
            createScsViewer(scsFileName, containerId).then(function (viewer) {
                resolve(viewer);
            })
        } else {    
            let wsProtcol, rendererType, endpoint;

            if ("http:" == location.protocol) {
                wsProtcol = "ws:"
            }
            else {
                wsProtcol = "wss:"
            }

            if (viewerMode == "SSR" || viewerMode == "ssr") {
                rendererType = Communicator.RendererType.Server;
            } 
            else {
                rendererType = Communicator.RendererType.Client;
            }

            endpoint = wsProtcol + "//" + window.location.hostname + ":11182";
            if (reverseProxy != undefined) {
               endpoint = wsProtcol + "//" + window.location.hostname + "/wsproxy/11182";
            }

            // if ("_empty" != modelName) {
                var viewer = new Communicator.WebViewer({
                    containerId: containerId,
                    endpointUri: endpoint,
                    model: modelName,
                    rendererType: rendererType,
                    boundingPreviewMode: "none",
                });
            // }
            // else {
            //     var viewer = new Communicator.WebViewer({
            //         containerId: containerId,
            //         endpointUri: endpoint,
            //         empty: true,
            //         rendererType: rendererType,
            //         boundingPreviewMode: "none",
            //     });
            // }

            resolve(viewer);
        }
    });
}

function createScsViewer(scsFileName, containerId) {
    return new Promise(function (resolve, reject) {
        if ("_empty.scs" != scsFileName) {
            var viewer = new Communicator.WebViewer({
                containerId: containerId,
                endpointUri: scsFileName
            });
        }
        else {
            var viewer = new Communicator.WebViewer({
                containerId: containerId,
                empty: true
            });
        }
        resolve(viewer);
    });
};

function requestEndpoint(rendererType, reverseProxy, modelDirArr) {
    var request = new XMLHttpRequest();
    var promise = new Promise(function (resolve, reject) {
        request.onreadystatechange = function () {
            if (request.readyState == 4) {
                if (request.status == 200) {
                    var response = request.responseText;
                    var obj = JSON.parse(response);
                    resolve(obj.endpoints.ws);
                } else {
                    reject("ws://localhost:55555");
                }
            }
        };
        var serviceBrokerURL = window.location.protocol + "//" + window.location.hostname + ":11182";
        if (reverseProxy != undefined)
            serviceBrokerURL = window.location.protocol + "//" + window.location.hostname + "/httpproxy/11182";
        
        request.open("POST", serviceBrokerURL + "/service", true);
        var data;
        var strDir = "";
        
        if(modelDirArr != undefined) {
            strDir = ',"params":{"modelSearchDirectories":' + JSON.stringify(modelDirArr) + '}';
        }
        
        if (rendererType == Communicator.RendererType.Server) {
            data = '{"class": "ssr_session"' + strDir + '}';
        } else {
            data = '{"class": "csr_session"' + strDir + '}';
        }
        request.send(data);
    });
    return promise;
}

