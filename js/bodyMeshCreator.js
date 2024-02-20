"use strict";
/// <reference path="../../typescript/hoops_web_viewer.d.ts" />
class BodyMeshCreator {
    constructor(viewer) {
        this._viewer = viewer;
    }
    _lineArrToLoop(lineArr) {
        let lineArrCopy = lineArr.concat();
        let loopPnts = new Array(0);
        let floatArr = new Array(0);
        while (lineArrCopy.length) {
            const prevLen = lineArrCopy.length;
            for (let i = 0; i < lineArrCopy.length; i++) {
                const line = lineArrCopy[i];
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
                    const loopEn = loopPnts[loopPnts.length - 1];
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
            if (prevLen == lineArrCopy.length)
                return floatArr;
        }
        for (let pnt of loopPnts) {
            floatArr.push(pnt.x);
            floatArr.push(pnt.y);
            floatArr.push(pnt.z);
        }
        return floatArr;
    }
    createMeshInstance(meshInstanceData, entityTag, parentNode) {
        return new Promise((resolve, reject) => {
            const nodeId = this._viewer.model.createNode(parentNode, String(entityTag), entityTag);
            this._viewer.model.createMeshInstance(meshInstanceData, nodeId).then((nodeId) => {
                resolve(nodeId);
            });
        });
    }
    addMesh(arr, matrix = null, instanceTag = null, parentNode) {
        return new Promise((resolve, reject) => {
            let bodyTag = arr[0];
            let faceMeshIDs = [];
            let edgeMeshIDs = [];
            let edgeCnt = 0;
            let faceId = 0;
            let faceRGB = new Array(3);
            let faceColorArr = new Array(0);
            let edgeGrArr = [];
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
                };
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
                    let colorArr = [];
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
                }
                else {
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
                        };
                        edgeGrArr.push(edgeGr);
                    }
                    edgeCnt--;
                    i += len - 1;
                }
            }
            for (let edgeGr of edgeGrArr) {
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
                    };
                    faceColorGrArr.push(faceColorGr);
                }
            }
            // Get major color of the body
            let max = 0;
            let majorColor = [-1, -1, -1];
            let majorGrId = -1;
            for (let i = 0; i < faceColorGrArr.length; i++) {
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
                this.createMeshInstance(meshInstanceData, entityTag, parentNode).then((nodeId) => {
                    let body = {
                        meshInstanceData: meshInstanceData,
                        instanceTag: instanceTag,
                        bodyTag: bodyTag,
                        faces: faceMeshIDs,
                        edges: edgeMeshIDs,
                        triangleCnt: triCnt
                    };
                    resolve(body);
                });
            });
        });
    }
}
//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoiYm9keU1lc2hDcmVhdG9yLmpzIiwic291cmNlUm9vdCI6IiIsInNvdXJjZXMiOlsiLi4vdHlwZXNjcmlwdC9ib2R5TWVzaENyZWF0b3IudHMiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IjtBQUFBLCtEQUErRDtBQUUvRCxNQUFNLGVBQWU7SUFHakIsWUFBWSxNQUE4QjtRQUN0QyxJQUFJLENBQUMsT0FBTyxHQUFHLE1BQU0sQ0FBQztJQUMxQixDQUFDO0lBRUQsY0FBYyxDQUFFLE9BQW1CO1FBQy9CLElBQUksV0FBVyxHQUFHLE9BQU8sQ0FBQyxNQUFNLEVBQUUsQ0FBQztRQUNuQyxJQUFJLFFBQVEsR0FBRyxJQUFJLEtBQUssQ0FBQyxDQUFDLENBQUMsQ0FBQztRQUM1QixJQUFJLFFBQVEsR0FBRyxJQUFJLEtBQUssQ0FBQyxDQUFDLENBQUMsQ0FBQztRQUM1QixPQUFPLFdBQVcsQ0FBQyxNQUFNLEVBQUU7WUFDdkIsTUFBTSxPQUFPLEdBQUcsV0FBVyxDQUFDLE1BQU0sQ0FBQztZQUNuQyxLQUFLLElBQUksQ0FBQyxHQUFHLENBQUMsRUFBRSxDQUFDLEdBQUcsV0FBVyxDQUFDLE1BQU0sRUFBRSxDQUFDLEVBQUUsRUFBRTtnQkFDekMsTUFBTSxJQUFJLEdBQWEsV0FBVyxDQUFDLENBQUMsQ0FBQyxDQUFDO2dCQUN0QyxJQUFJLEtBQUssR0FBRyxJQUFJLFlBQVksQ0FBQyxNQUFNLENBQUMsSUFBSSxDQUFDLENBQUMsQ0FBQyxFQUFFLElBQUksQ0FBQyxDQUFDLENBQUMsRUFBRSxJQUFJLENBQUMsQ0FBQyxDQUFDLENBQUMsQ0FBQztnQkFDL0QsSUFBSSxLQUFLLEdBQUcsSUFBSSxZQUFZLENBQUMsTUFBTSxDQUFDLElBQUksQ0FBQyxDQUFDLENBQUMsRUFBRSxJQUFJLENBQUMsQ0FBQyxDQUFDLEVBQUUsSUFBSSxDQUFDLENBQUMsQ0FBQyxDQUFDLENBQUM7Z0JBQy9ELElBQUksQ0FBQyxJQUFJLFFBQVEsQ0FBQyxNQUFNLEVBQUU7b0JBQ3RCLFFBQVEsQ0FB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