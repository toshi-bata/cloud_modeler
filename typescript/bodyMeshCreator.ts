/// <reference path="../../typescript/hoops_web_viewer.d.ts" />
/// import * as Communicator from "../hoops-web-viewer.mjs";

export class BodyMeshCreator {
    private _viewer: Communicator.WebViewer;

    constructor(viewer: Communicator.WebViewer) {
        this._viewer = viewer;
    }

    _lineArrToLoop (lineArr: [number[]]) {
        let lineArrCopy = lineArr.concat();
        let loopPnts = new Array(0);
        let floatArr = new Array(0);
        while (lineArrCopy.length) {
            const prevLen = lineArrCopy.length;
            for (let i = 0; i < lineArrCopy.length; i++) {
                const line: number[] = lineArrCopy[i];
                let stPnt = new Communicator.Point3(line[0], line[1], line[2]);
                let enPnt = new Communicator.Point3(line[3], line[4], line[5]);
                if (0 == loopPnts.length) {
                    loopPnts.push(stPnt);
                    loopPnts.push(enPnt);
                    lineArrCopy.splice(i, 1);
                    break;
                }
                else {
                    const loopSt = loopPnts[0];
                    const loopEn = loopPnts[loopPnts.length - 1]
                    if (loopEn.equals(stPnt)) {
                        loopPnts.push(enPnt);
                        lineArrCopy.splice(i, 1);
                        break;
                    }
                    else if (loopEn.equals(enPnt)) {
                        loopPnts.push(stPnt);
                        lineArrCopy.splice(i, 1);
                        break;
                    }
                    else if (loopSt.equals(enPnt)) {
                        loopPnts.unshift(stPnt);
                        lineArrCopy.splice(i, 1);
                        break;
                    }
                    else if (loopSt.equals(stPnt)) {
                        loopPnts.unshift(enPnt);
                        lineArrCopy.splice(i, 1);
                        break;
                    }
                }
            }
            if (prevLen == lineArrCopy.length) return floatArr;
        }

        for (let pnt of loopPnts) {
            floatArr.push(pnt.x);
            floatArr.push(pnt.y);
            floatArr.push(pnt.z);
        }
        return floatArr;
    }

    createMeshInstance(meshInstanceData: Communicator.MeshInstanceData, entityTag: number, parentNode: number) {
        return new Promise((resolve, reject) => {
            const nodeId = this._viewer.model.createNode(parentNode, String(entityTag), entityTag);
            this._viewer.model.createMeshInstance(meshInstanceData, nodeId).then((nodeId) => {
                resolve(nodeId);
            });
        });
    }

    addMesh (arr: number[], matrix: Communicator.Matrix | null = null, instanceTag: number | null = null, parentNode: number) {
        return new Promise((resolve, reject) => {
            let bodyTag = arr[0]

            let faceMeshIDs:number[] = [];
            let edgeMeshIDs: number[] = [];
            let edgeCnt = 0;
            let faceId: number = 0;
            let faceRGB = new Array(3);
            let faceColorArr = new Array(0);
            let edgeGrArr: any[] = [];
            let meshData = new Communicator.MeshData();
            meshData.setFaceWinding(Communicator.FaceWinding.CounterClockwise);
            meshData.setManifold(true);
            let triCnt = 0;

            let entityTag = bodyTag;
            if (null != instanceTag) {
                entityTag = instanceTag;
            }
            
            if (2 > arr.length) {
                this._viewer.model.createNode(parentNode, String(entityTag), entityTag);

                let body = {
                    meshInstanceData: null,
                    instanceTag: instanceTag, 
                    bodyTag: bodyTag,
                    faces: faceMeshIDs,
                    edges: edgeMeshIDs,
                }
                return resolve(body);
            }

            for (let i = 1; i < arr.length; i++) {
                let edgeId;
                if (edgeCnt == 0) {
                    faceId = arr[i++];
                    faceRGB[0] = Math.round(arr[i++] * 255);
                    faceRGB[1] = Math.round(arr[i++] * 255);
                    faceRGB[2] = Math.round(arr[i++] * 255);
                    faceColorArr.push(faceRGB.concat());
                }

                if (faceId == 0) {
                    edgeId = arr[i++];
                }

                let len = arr[i++];
                let meshArr = arr.slice(i, i + len);

                if (faceId != 0) {
                    // add face mesh data
                    i += len;
                    let normalArr = arr.slice(i, i + len); 
                    let colorArr: any[] = [];
                    for (let i = 0; i < len / 3; i++) {
                        colorArr.push(faceRGB[0]);
                        colorArr.push(faceRGB[1]);
                        colorArr.push(faceRGB[2]);
                        colorArr.push(255);
                    }
                    meshData.addFaces(meshArr, normalArr, colorArr);
                    triCnt += len / 9;
                    faceMeshIDs.push(faceId);
                    faceId = 0;
                    i += len;
                    edgeCnt = arr[i];
                } else {
                    // Group edge
                    let flg = false;
                    for (let edgeGr of edgeGrArr) {
                        if (edgeGr.edgeId == edgeId) {
                            edgeGr.lineArr.push(meshArr);
                            flg = true;
                            break;
                        }
                    }
                    if (!flg) {
                        const edgeGr = {
                            edgeId: edgeId,
                            lineArr: [meshArr]
                        }
                        edgeGrArr.push(edgeGr);
                    }

                    edgeCnt--;
                    i += len - 1;
                }
            }
            
            for (let edgeGr of  edgeGrArr) {
                const floatArr = this._lineArrToLoop(edgeGr.lineArr);
                if (floatArr.length) {
                    // add edge mash data
                    meshData.addPolyline(floatArr);
                    edgeMeshIDs.push(edgeGr.edgeId);
                }
            }

            // Group faces by same color
            let faceColorGrArr = new Array(0);
            for (let i = 0; i < faceColorArr.length; i++) {
                const faceColor = faceColorArr[i];
                let flg = false;
                for (let faceColorGr of faceColorGrArr) {
                    const grColor = faceColorGr.color;
                    if (grColor[0] == faceColor[0] && grColor[1] == faceColor[1] && grColor[2] == faceColor[2]) {
                        faceColorGr.faceIdArr.push(i);
                        flg = true;
                        break;
                    }
                }

                if (!flg) {
                    const faceColorGr = {
                        color: faceColor.concat(),
                        faceIdArr: [i]
                    }
                    faceColorGrArr.push(faceColorGr);
                }
            }

            // Get major color of the body
            let max = 0;
            let majorColor = [-1, -1, -1];
            let majorGrId = -1;
            for (let i = 0; i <  faceColorGrArr.length; i++) {
                const faceColorGr = faceColorGrArr[i];
                if (max < faceColorGr.faceIdArr.length) {
                    max = faceColorGr.faceIdArr.length;
                    majorColor = faceColorGr.color;
                    majorGrId = i;
                }
            }

            let faceColor = new Communicator.Color(100, 100, 100);
            if (-1 != majorColor[0] && -1 != majorColor[1] && -1 != majorColor[2]) {
                faceColor.r = majorColor[0];
                faceColor.g = majorColor[1];
                faceColor.b = majorColor[2];
            }
            
            // Create body mesh
            this._viewer.model.createMesh(meshData).then((meshIdArr) => {
                let edgeColor = new Communicator.Color(0, 0, 0);
                let meshInstanceData = new Communicator.MeshInstanceData(meshIdArr, matrix, "body<" + entityTag + ">", faceColor, edgeColor);
                this.createMeshInstance(meshInstanceData, entityTag, parentNode).then((nodeId: any) => {
                    let body = {
                        meshInstanceData: meshInstanceData,
                        instanceTag: instanceTag, 
                        bodyTag: bodyTag,
                        faces: faceMeshIDs,
                        edges: edgeMeshIDs,
                        triangleCnt: triCnt
                    }
                    resolve(body);
                });
            });
        });
    }
}