class ServerCaller {
    constructor(serverURL, sessionId) {
        this._serverURL = serverURL;
        this._sessionId = sessionId;
    }

    _encodeHTMLForm (data)
    {
        let params = new Array(0);

        for (let name in data ) {
            const value = data[name];
            const param = encodeURIComponent(name) + '=' + encodeURIComponent(value);

            params.push( param );
        }

        return params.join("&").replace( /%20/g, "+");
    }

    CallServerGet(command, retType) {
        return new Promise((resolve, reject) => {
            let oReq = new XMLHttpRequest();
            oReq.open("GET", this._serverURL + "/" + command + "?session_id=" + this._sessionId, true);
            oReq.responseType = "arraybuffer";

            oReq.onreadystatechange = () => {
                if(oReq.readyState === XMLHttpRequest.DONE && oReq.status === 200) {
                    switch (retType) {
                        case "int": {
                            let arr;
                            if (16 <= oReq.response.byteLength) arr = new Int16Array(oReq.response);
                            else arr = new Int8Array(oReq.response);
                            if (arr.length) resolve(arr);
                            else reject();
                        } break;
                        case "float": {
                            const arr = new Float32Array(oReq.response); 
                            if (arr.length) resolve(arr);
                            else reject();
                        } break;
                        default: {
                            if ("OK" == oReq.statusText) resolve();
                            else reject();
                        } break;
                    }
                }
            }
            oReq.onerror = (e) => {
                reject(e);
              };
            oReq.send(null);
        });
    }

    CallServerPost(command, params = {}, retType = null) {
        return new Promise((resolve, reject) => {
            // Add session ID in params
            params["sessionId"] = this._sessionId;
            const encParams = this._encodeHTMLForm(params);

            let oReq = new XMLHttpRequest();
            oReq.open("POST", this._serverURL + "/" + command, true);
            oReq.setRequestHeader( 'Content-Type', 'application/x-www-form-urlencoded' );
            if ("int" == retType || "float" == retType) {
                oReq.responseType = "arraybuffer";
            }
            oReq.onreadystatechange = () => {
                if(oReq.readyState === XMLHttpRequest.DONE && oReq.status === 200) {
                    switch (retType) {
                        case "int": {
                            let arr = new Int32Array(oReq.response);
                            if (arr.length) resolve(arr);
                            else reject();
                        } break;
                        case "float": {
                            const arr = new Float32Array(oReq.response); 
                            if (arr.length) resolve(arr);
                            else reject();
                        } break;
                        default: {
                            if ("OK" == oReq.statusText) resolve();
                            else reject();
                        } break;
                    }
                }
            }
            oReq.onerror = () => {
                reject(oReq.statusText);
              };
            oReq.send(encParams);
        });
    }

    CallServerPostFile(f) {
        return new Promise((resolve, reject) => {
            let oReq = new XMLHttpRequest();
            oReq.open("POST", this._serverURL + "/" + f.name + "?session_id=" + this._sessionId, true);

            oReq.onreadystatechange = () => {
                if(oReq.readyState === XMLHttpRequest.DONE && oReq.status === 200) {
                    const str = oReq.response;
                    const data = JSON.parse(str); 
                    resolve(data);
                }
            }
            oReq.onerror = () => {
                reject(oReq.statusText);
              };
            oReq.send(f);
        });
    }


}