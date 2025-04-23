/// <reference path="../../typescript/hoops_web_viewer.d.ts" />
import * as Communicator from "../hoops-web-viewer.mjs";
export class BodyMeshCreator {
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
//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoiYm9keU1lc2hDcmVhdG9yLmpzIiwic291cmNlUm9vdCI6IiIsInNvdXJjZXMiOlsiLi4vdHlwZXNjcmlwdC9ib2R5TWVzaENyZWF0b3IudHMiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IkFBQUEsK0RBQStEO0FBQy9ELDREQUE0RDtBQUU1RCxNQUFNLE9BQU8sZUFBZTtJQUd4QixZQUFZLE1BQThCO1FBQ3RDLElBQUksQ0FBQyxPQUFPLEdBQUcsTUFBTSxDQUFDO0lBQzFCLENBQUM7SUFFRCxjQUFjLENBQUUsT0FBbUI7UUFDL0IsSUFBSSxXQUFXLEdBQUcsT0FBTyxDQUFDLE1BQU0sRUFBRSxDQUFDO1FBQ25DLElBQUksUUFBUSxHQUFHLElBQUksS0FBSyxDQUFDLENBQUMsQ0FBQyxDQUFDO1FBQzVCLElBQUksUUFBUSxHQUFHLElBQUksS0FBSyxDQUFDLENBQUMsQ0FBQyxDQUFDO1FBQzVCLE9BQU8sV0FBVyxDQUFDLE1BQU0sRUFBRSxDQUFDO1lBQ3hCLE1BQU0sT0FBTyxHQUFHLFdBQVcsQ0FBQyxNQUFNLENBQUM7WUFDbkMsS0FBSyxJQUFJLENBQUMsR0FBRyxDQUFDLEVBQUUsQ0FBQyxHQUFHLFdBQVcsQ0FBQyxNQUFNLEVBQUUsQ0FBQyxFQUFFLEVBQUUsQ0FBQztnQkFDMUMsTUFBTSxJQUFJLEdBQWEsV0FBVyxDQUFDLENBQUMsQ0FBQyxDQUFDO2dCQUN0QyxJQUFJLEtBQUssR0FBRyxJQUFJLFlBQVksQ0FBQyxNQUFNLENBQUMsSUFBSSxDQUFDLENBQUMsQ0FBQyxFQUFFLElBQUksQ0FBQyxDQUFDLENBQUMsRUFBRSxJQUFJLENBQUMsQ0FBQyxDQUFDLENBQUMsQ0FBQztnQkFDL0QsSUFBSSxLQUFLLEdBQUcsSUFBSSxZQUFZLENBQUMsTUFBTSxDQUFDLElBQUksQ0FBQyxDQUFDLENBQUMsRUFBRSxJQUFJLENBQUMsQ0FBQyxDQUFDLEVBQUUsSUFBSSxDQUFDLENBQUMsQ0FBQyxDQUFDLENBQUM7Z0JBQy9ELElBQUksQ0FBQyxJQUFJLFFBQVEsQ0FBQyxNQUFNLEVBQUUsQ0FBQztvQkFDdkIsUUFBUSxDQUFDLElBQUksQ0FBQyxLQUFLLENBQUMsQ0FBQztvQkFDckIsUUFBUSxDQUFDLElBQUksQ0FBQyxLQUFLLENBQUMsQ0FBQztvQkFDckIsV0FBVyxDQUFDLE1BQU0sQ0FBQyxDQUFDLEVBQUUsQ0FBQyxDQUFDLENBQUM7b0JBQ3pCLE1BQU07Z0JBQ1YsQ0FBQztxQkFDSSxDQUFDO29CQUNGLE1BQU0sTUFBTSxHQUFHLFFBQVEsQ0FBQyxDQUFDLENBQUMsQ0FBQztvQkFDM0IsTUFBTSxNQUFNLEdBQUcsUUFBUSxDQUFDLFFBQVEsQ0FBQyxNQUFNLEdBQUcsQ0FBQyxDQUFDLENBQUE7b0JBQzVDLElBQUksTUFBTSxDQUFDLE1BQU0sQ0FBQyxLQUFLLENBQUMsRUFBRSxDQUFDO3dCQUN2QixRQUFRLENBQUMsSUFBSSxDQUFDLEtBQUssQ0FBQyxDQUFDO3dCQUNyQixXQUFXLENBQUMsTUFBTSxDQUFDLENBQUMsRUFBRSxDQUFDLENBQUMsQ0FBQzt3QkFDekIsTUFBTTtvQkFDVixDQUFDO3lCQUNJLElBQUksTUFBTSxDQUFDLE1BQU0sQ0FBQyxLQUFLLENBQUMsRUFBRSxDQUFDO3dCQUM1QixRQUFRLENBQUMsSUFBSSxDQUFDLEtBQUssQ0FBQyxDQUFDO3dCQUNyQixXQUFXLENBQUMsTUFBTSxDQUFDLENBQUMsRUFBRSxDQUFDLENBQUMsQ0FBQzt3QkFDekIsTUFBTTtvQkFDVixDQUFDO3lCQUNJLElBQUksTUFBTSxDQUFDLE1BQU0sQ0FBQyxLQUFLLENBQUMsRUFBRSxDQUFDO3dCQUM1QixRQUFRLENBQUMsT0FBTyxDQUFDLEtBQUssQ0FBQyxDQUFDO3dCQUN4QixXQUFXLENBQUMsTUFBTSxDQUFDLENBQUMsRUFBRSxDQUFDLENBQUMsQ0FBQzt3QkFDekIsTUFBTTtvQkFDVixDQUFDO3lCQUNJLElBQUksTUFBTSxDQUFDLE1BQU0sQ0FBQyxLQUFLLENBQUMsRUFBRSxDQUFDO3dCQUM1QixRQUFRLENBQUMsT0FBTyxDQUFDLEtBQUssQ0FBQyxDQUFDO3dCQUN4QixXQUFXLENBQUMsTUFBTSxDQUFDLENBQUMsRUFBRSxDQUFDLENBQUMsQ0FBQzt3QkFDekIsTUFBTTtvQkFDVixDQUFDO2dCQUNMLENBQUM7WUFDTCxDQUFDO1lBQ0QsSUFBSSxPQUFPLElBQUksV0FBVyxDQUFDLE1BQU07Z0JBQUUsT0FBTyxRQUFRLENBQUM7UUFDdkQsQ0FBQztRQUVELEtBQUssSUFBSSxHQUFHLElBQUksUUFBUSxFQUFFLENBQUM7WUFDdkIsUUFBUSxDQUFDLElBQUksQ0FBQyxHQUFHLENBQUMsQ0FBQyxDQUFDLENBQUM7WUFDckIsUUFBUSxDQUFDLElBQUksQ0FBQyxHQUFHLENBQUMsQ0FBQyxDQUFDLENBQUM7WUFDckIsUUFBUSxDQUFDLElBQUksQ0FBQyxHQUFHLENBQUMsQ0FBQyxDQUFDLENBQUM7UUFDekIsQ0FBQztRQUNELE9BQU8sUUFBUSxDQUFDO0lBQ3BCLENBQUM7SUFFRCxrQkFBa0IsQ0FBQyxnQkFBK0MsRUFBRSxTQUFpQixFQUFFLFVBQWtCO1FBQ3JHLE9BQU8sSUFBSSxPQUFPLENBQUMsQ0FBQyxPQUFPLEVBQUUsTUFBTSxFQUFFLEVBQUU7WUFDbkMsTUFBTSxNQUFNLEdBQUcsSUFBSSxDQUFDLE9BQU8sQ0FBQyxLQUFLLENBQUMsVUFBVSxDQUFDLFVBQVUsRUFBRSxNQUFNLENBQUMsU0FBUyxDQUFDLEVBQUUsU0FBUyxDQUFDLENBQUM7WUFDdkYsSUFBSSxDQUFDLE9BQU8sQ0FBQyxLQUFLLENBQUMsa0JBQWtCLENBQUMsZ0JBQWdCLEVBQUUsTUFBTSxDQUFDLENBQUMsSUFBSSxDQUFDLENBQUMsTUFBTSxFQUFFLEVBQUU7Z0JBQzVFLE9BQU8sQ0FBQyxNQUFNLENBQUMsQ0FBQztZQUNwQixDQUFDLENBQUMsQ0FBQztRQUNQLENBQUMsQ0FBQyxDQUFDO0lBQ1AsQ0FBQztJQUVELE9BQU8sQ0FBRSxHQUFhLEVBQUUsU0FBcUMsSUFBSSxFQUFFLGNBQTZCLElBQUksRUFBRSxVQUFrQjtRQUNwSCxPQUFPLElBQUksT0FBTyxDQUFDLENBQUMsT0FBTyxFQUFFLE1BQU0sRUFBRSxFQUFFO1lBQ25DLElBQUksT0FBTyxHQUFHLEdBQUcsQ0FBQyxDQUFDLENBQUMsQ0FBQTtZQUVwQixJQUFJLFdBQVcsR0FBWSxFQUFFLENBQUM7WUFDOUIsSUFBSSxXQUFXLEdBQWEsRUFBRSxDQUFDO1lBQy9CLElBQUksT0FBTyxHQUFHLENBQUMsQ0FBQztZQUNoQixJQUFJLE1BQU0sR0FBVyxDQUFDLENBQUM7WUFDdkIsSUFBSSxPQUFPLEdBQUcsSUFBSSxLQUFLLENBQUMsQ0FBQyxDQUFDLENBQUM7WUFDM0IsSUFBSSxZQUFZLEdBQUcsSUFBSSxLQUFLLENBQUMsQ0FBQyxDQUFDLENBQUM7WUFDaEMsSUFBSSxTQUFTLEdBQVUsRUFBRSxDQUFDO1lBQzFCLElBQUksUUFBUSxHQUFHLElBQUksWUFBWSxDQUFDLFFBQVEsRUFBRSxDQUFDO1lBQzNDLFFBQVEsQ0FBQyxjQUFjLENBQUMsWUFBWSxDQUFDLFdBQVcsQ0FBQyxnQkFBZ0IsQ0FBQyxDQUFDO1lBQ25FLFFBQVEsQ0FBQyxXQUFXLENBQUMsSUFBSSxDQUFDLENBQUM7WUFDM0IsSUFBSSxNQUFNLEdBQUcsQ0FBQyxDQUFDO1lBRWYsSUFBSSxTQUFTLEdBQUcsT0FBTyxDQUFDO1lBQ3hCLElBQUksSUFBSSxJQUFJLFdBQVcsRUFBRSxDQUFDO2dCQUN0QixTQUFTLEdBQUcsV0FBVyxDQUFDO1lBQzVCLENBQUM7WUFFRCxJQUFJLENBQUMsR0FBRyxHQUFHLENBQUMsTUFBTSxFQUFFLENBQUM7Z0JBQ2pCLElBQUksQ0FBQyxPQUFPLENBQUMsS0FBSyxDQUFDLFVBQVUsQ0FBQyxVQUFVLEVBQUUsTUFBTSxDQUFDLFNBQVMsQ0FBQyxFQUFFLFNBQVMsQ0FBQyxDQUFDO2dCQUV4RSxJQUFJLElBQUksR0FBRztvQkFDUCxnQkFBZ0IsRUFBRSxJQUFJO29CQUN0QixXQUFXLEVBQUUsV0FBVztvQkFDeEIsT0FBTyxFQUFFLE9BQU87b0JBQ2hCLEtBQUssRUFBRSxXQUFXO29CQUNsQixLQUFLLEVBQUUsV0FBVztpQkFDckIsQ0FBQTtnQkFDRCxPQUFPLE9BQU8sQ0FBQyxJQUFJLENBQUMsQ0FBQztZQUN6QixDQUFDO1lBRUQsS0FBSyxJQUFJLENBQUMsR0FBRyxDQUFDLEVBQUUsQ0FBQyxHQUFHLEdBQUcsQ0FBQyxNQUFNLEVBQUUsQ0FBQyxFQUFFLEVBQUUsQ0FBQztnQkFDbEMsSUFBSSxNQUFNLENBQUM7Z0JBQ1gsSUFBSSxPQUFPLElBQUksQ0FBQyxFQUFFLENBQUM7b0JBQ2YsTUFBTSxHQUFHLEdBQUcsQ0FBQyxDQUFDLEVBQUUsQ0FBQyxDQUFDO29CQUNsQixPQUFPLENBQUMsQ0FBQyxDQUFDLEdBQUcsSUFBSSxDQUFDLEtBQUssQ0FBQyxHQUFHLENBQUMsQ0FBQyxFQUFFLENBQUMsR0FBRyxHQUFHLENBQUMsQ0FBQztvQkFDeEMsT0FBTyxDQUFDLENBQUMsQ0FBQyxHQUFHLElBQUksQ0FBQyxLQUFLLENBQUMsR0FBRyxDQUFDLENBQUMsRUFBRSxDQUFDLEdBQUcsR0FBRyxDQUFDLENBQUM7b0JBQ3hDLE9BQU8sQ0FBQyxDQUFDLENBQUMsR0FBRyxJQUFJLENBQUMsS0FBSyxDQUFDLEdBQUcsQ0FBQyxDQUFDLEVBQUUsQ0FBQyxHQUFHLEdBQUcsQ0FBQyxDQUFDO29CQUN4QyxZQUFZLENBQUMsSUFBSSxDQUFDLE9BQU8sQ0FBQyxNQUFNLEVBQUUsQ0FBQyxDQUFDO2dCQUN4QyxDQUFDO2dCQUVELElBQUksTUFBTSxJQUFJLENBQUMsRUFBRSxDQUFDO29CQUNkLE1BQU0sR0FBRyxHQUFHLENBQUMsQ0FBQyxFQUFFLENBQUMsQ0FBQztnQkFDdEIsQ0FBQztnQkFFRCxJQUFJLEdBQUcsR0FBRyxHQUFHLENBQUMsQ0FBQyxFQUFFLENBQUMsQ0FBQztnQkFDbkIsSUFBSSxPQUFPLEdBQUcsR0FBRyxDQUFDLEtBQUssQ0FBQyxDQUFDLEVBQUUsQ0FBQyxHQUFHLEdBQUcsQ0FBQyxDQUFDO2dCQUVwQyxJQUFJLE1BQU0sSUFBSSxDQUFDLEVBQUUsQ0FBQztvQkFDZCxxQkFBcUI7b0JBQ3JCLENBQUMsSUFBSSxHQUFHLENBQUM7b0JBQ1QsSUFBSSxTQUFTLEdBQUcsR0FBRyxDQUFDLEtBQUssQ0FBQyxDQUFDLEVBQUUsQ0FBQyxHQUFHLEdBQUcsQ0FBQyxDQUFDO29CQUN0QyxJQUFJLFFBQVEsR0FBVSxFQUFFLENBQUM7b0JBQ3pCLEtBQUssSUFBSSxDQUFDLEdBQUcsQ0FBQyxFQUFFLENBQUMsR0FBRyxHQUFHLEdBQUcsQ0FBQyxFQUFFLENBQUMsRUFBRSxFQUFFLENBQUM7d0JBQy9CLFFBQVEsQ0FBQyxJQUFJLENBQUMsT0FBTyxDQUFDLENBQUMsQ0FBQyxDQUFDLENBQUM7d0JBQzFCLFFBQVEsQ0FBQyxJQUFJLENBQUMsT0FBTyxDQUFDLENBQUMsQ0FBQyxDQUFDLENBQUM7d0JBQzFCLFFBQVEsQ0FBQyxJQUFJLENBQUMsT0FBTyxDQUFDLENBQUMsQ0FBQyxDQUFDLENBQUM7d0JBQzFCLFFBQVEsQ0FBQyxJQUFJLENBQUMsR0FBRyxDQUFDLENBQUM7b0JBQ3ZCLENBQUM7b0JBQ0QsUUFBUSxDQUFDLFFBQVEsQ0FBQyxPQUFPLEVBQUUsU0FBUyxFQUFFLFFBQVEsQ0FBQyxDQUFDO29CQUNoRCxNQUFNLElBQUksR0FBRyxHQUFHLENBQUMsQ0FBQztvQkFDbEIsV0FBVyxDQUFDLElBQUksQ0FBQyxNQUFNLENBQUMsQ0FBQztvQkFDekIsTUFBTSxHQUFHLENBQUMsQ0FBQztvQkFDWCxDQUFDLElBQUksR0FBRyxDQUFDO29CQUNULE9BQU8sR0FBRyxHQUFHLENBQUMsQ0FBQyxDQUFDLENBQUM7Z0JBQ3JCLENBQUM7cUJBQU0sQ0FBQztvQkFDSixhQUFhO29CQUNiLElBQUksR0FBRyxHQUFHLEtBQUssQ0FBQztvQkFDaEIsS0FBSyxJQUFJLE1BQU0sSUFBSSxTQUFTLEVBQUUsQ0FBQzt3QkFDM0IsSUFBSSxNQUFNLENBQUMsTUFBTSxJQUFJLE1BQU0sRUFBRSxDQUFDOzRCQUMxQixNQUFNLENBQUMsT0FBTyxDQUFDLElBQUksQ0FBQyxPQUFPLENBQUMsQ0FBQzs0QkFDN0IsR0FBRyxHQUFHLElBQUksQ0FBQzs0QkFDWCxNQUFNO3dCQUNWLENBQUM7b0JBQ0wsQ0FBQztvQkFDRCxJQUFJLENBQUMsR0FBRyxFQUFFLENBQUM7d0JBQ1AsTUFBTSxNQUFNLEdBQUc7NEJBQ1gsTUFBTSxFQUFFLE1BQU07NEJBQ2QsT0FBTyxFQUFFLENBQUMsT0FBTyxDQUFDO3lCQUNyQixDQUFBO3dCQUNELFNBQVMsQ0FBQyxJQUFJLENBQUMsTUFBTSxDQUFDLENBQUM7b0JBQzNCLENBQUM7b0JBRUQsT0FBTyxFQUFFLENBQUM7b0JBQ1YsQ0FBQyxJQUFJLEdBQUcsR0FBRyxDQUFDLENBQUM7Z0JBQ2pCLENBQUM7WUFDTCxDQUFDO1lBRUQsS0FBSyxJQUFJLE1BQU0sSUFBSyxTQUFTLEVBQUUsQ0FBQztnQkFDNUIsTUFBTSxRQUFRLEdBQUcsSUFBSSxDQUFDLGNBQWMsQ0FBQyxNQUFNLENBQUMsT0FBTyxDQUFDLENBQUM7Z0JBQ3JELElBQUksUUFBUSxDQUFDLE1BQU0sRUFBRSxDQUFDO29CQUNsQixxQkFBcUI7b0JBQ3JCLFFBQVEsQ0FBQyxXQUFXLENBQUMsUUFBUSxDQUFDLENBQUM7b0JBQy9CLFdBQVcsQ0FBQyxJQUFJLENBQUMsTUFBTSxDQUFDLE1BQU0sQ0FBQyxDQUFDO2dCQUNwQyxDQUFDO1lBQ0wsQ0FBQztZQUVELDRCQUE0QjtZQUM1QixJQUFJLGNBQWMsR0FBRyxJQUFJLEtBQUssQ0FBQyxDQUFDLENBQUMsQ0FBQztZQUNsQyxLQUFLLElBQUksQ0FBQyxHQUFHLENBQUMsRUFBRSxDQUFDLEdBQUcsWUFBWSxDQUFDLE1BQU0sRUFBRSxDQUFDLEVBQUUsRUFBRSxDQUFDO2dCQUMzQyxNQUFNLFNBQVMsR0FBRyxZQUFZLENBQUMsQ0FBQyxDQUFDLENBQUM7Z0JBQ2xDLElBQUksR0FBRyxHQUFHLEtBQUssQ0FBQztnQkFDaEIsS0FBSyxJQUFJLFdBQVcsSUFBSSxjQUFjLEVBQUUsQ0FBQztvQkFDckMsTUFBTSxPQUFPLEdBQUcsV0FBVyxDQUFDLEtBQUssQ0FBQztvQkFDbEMsSUFBSSxPQUFPLENBQUMsQ0FBQyxDQUFDLElBQUksU0FBUyxDQUFDLENBQUMsQ0FBQyxJQUFJLE9BQU8sQ0FBQyxDQUFDLENBQUMsSUFBSSxTQUFTLENBQUMsQ0FBQyxDQUFDLElBQUksT0FBTyxDQUFDLENBQUMsQ0FBQyxJQUFJLFNBQVMsQ0FBQyxDQUFDLENBQUMsRUFBRSxDQUFDO3dCQUN6RixXQUFXLENBQUMsU0FBUyxDQUFDLElBQUksQ0FBQyxDQUFDLENBQUMsQ0FBQzt3QkFDOUIsR0FBRyxHQUFHLElBQUksQ0FBQzt3QkFDWCxNQUFNO29CQUNWLENBQUM7Z0JBQ0wsQ0FBQztnQkFFRCxJQUFJLENBQUMsR0FBRyxFQUFFLENBQUM7b0JBQ1AsTUFBTSxXQUFXLEdBQUc7d0JBQ2hCLEtBQUssRUFBRSxTQUFTLENBQUMsTUFBTSxFQUFFO3dCQUN6QixTQUFTLEVBQUUsQ0FBQyxDQUFDLENBQUM7cUJBQ2pCLENBQUE7b0JBQ0QsY0FBYyxDQUFDLElBQUksQ0FBQyxXQUFXLENBQUMsQ0FBQztnQkFDckMsQ0FBQztZQUNMLENBQUM7WUFFRCw4QkFBOEI7WUFDOUIsSUFBSSxHQUFHLEdBQUcsQ0FBQyxDQUFDO1lBQ1osSUFBSSxVQUFVLEdBQUcsQ0FBQyxDQUFDLENBQUMsRUFBRSxDQUFDLENBQUMsRUFBRSxDQUFDLENBQUMsQ0FBQyxDQUFDO1lBQzlCLElBQUksU0FBUyxHQUFHLENBQUMsQ0FBQyxDQUFDO1lBQ25CLEtBQUssSUFBSSxDQUFDLEdBQUcsQ0FBQyxFQUFFLENBQUMsR0FBSSxjQUFjLENBQUMsTUFBTSxFQUFFLENBQUMsRUFBRSxFQUFFLENBQUM7Z0JBQzlDLE1BQU0sV0FBVyxHQUFHLGNBQWMsQ0FBQyxDQUFDLENBQUMsQ0FBQztnQkFDdEMsSUFBSSxHQUFHLEdBQUcsV0FBVyxDQUFDLFNBQVMsQ0FBQyxNQUFNLEVBQUUsQ0FBQztvQkFDckMsR0FBRyxHQUFHLFdBQVcsQ0FBQyxTQUFTLENBQUMsTUFBTSxDQUFDO29CQUNuQyxVQUFVLEdBQUcsV0FBVyxDQUFDLEtBQUssQ0FBQztvQkFDL0IsU0FBUyxHQUFHLENBQUMsQ0FBQztnQkFDbEIsQ0FBQztZQUNMLENBQUM7WUFFRCxJQUFJLFNBQVMsR0FBRyxJQUFJLFlBQVksQ0FBQyxLQUFLLENBQUMsR0FBRyxFQUFFLEdBQUcsRUFBRSxHQUFHLENBQUMsQ0FBQztZQUN0RCxJQUFJLENBQUMsQ0FBQyxJQUFJLFVBQVUsQ0FBQyxDQUFDLENBQUMsSUFBSSxDQUFDLENBQUMsSUFBSSxVQUFVLENBQUMsQ0FBQyxDQUFDLElBQUksQ0FBQyxDQUFDLElBQUksVUFBVSxDQUFDLENBQUMsQ0FBQyxFQUFFLENBQUM7Z0JBQ3BFLFNBQVMsQ0FBQyxDQUFDLEdBQUcsVUFBVSxDQUFDLENBQUMsQ0FBQyxDQUFDO2dCQUM1QixTQUFTLENBQUMsQ0FBQyxHQUFHLFVBQVUsQ0FBQyxDQUFDLENBQUMsQ0FBQztnQkFDNUIsU0FBUyxDQUFDLENBQUMsR0FBRyxVQUFVLENBQUMsQ0FBQyxDQUFDLENBQUM7WUFDaEMsQ0FBQztZQUVELG1CQUFtQjtZQUNuQixJQUFJLENBQUMsT0FBTyxDQUFDLEtBQUssQ0FBQyxVQUFVLENBQUMsUUFBUSxDQUFDLENBQUMsSUFBSSxDQUFDLENBQUMsU0FBUyxFQUFFLEVBQUU7Z0JBQ3ZELElBQUksU0FBUyxHQUFHLElBQUksWUFBWSxDQUFDLEtBQUssQ0FBQyxDQUFDLEVBQUUsQ0FBQyxFQUFFLENBQUMsQ0FBQyxDQUFDO2dCQUNoRCxJQUFJLGdCQUFnQixHQUFHLElBQUksWUFBWSxDQUFDLGdCQUFnQixDQUFDLFNBQVMsRUFBRSxNQUFNLEVBQUUsT0FBTyxHQUFHLFNBQVMsR0FBRyxHQUFHLEVBQUUsU0FBUyxFQUFFLFNBQVMsQ0FBQyxDQUFDO2dCQUM3SCxJQUFJLENBQUMsa0JBQWtCLENBQUMsZ0JBQWdCLEVBQUUsU0FBUyxFQUFFLFVBQVUsQ0FBQyxDQUFDLElBQUksQ0FBQyxDQUFDLE1BQVcsRUFBRSxFQUFFO29CQUNsRixJQUFJLElBQUksR0FBRzt3QkFDUCxnQkFBZ0IsRUFBRSxnQkFBZ0I7d0JBQ2xDLFdBQVcsRUFBRSxXQUFXO3dCQUN4QixPQUFPLEVBQUUsT0FBTzt3QkFDaEIsS0FBSyxFQUFFLFdBQVc7d0JBQ2xCLEtBQUssRUFBRSxXQUFXO3dCQUNsQixXQUFXLEVBQUUsTUFBTTtxQkFDdEIsQ0FBQTtvQkFDRCxPQUFPLENBQUMsSUFBSSxDQUFDLENBQUM7Z0JBQ2xCLENBQUMsQ0FBQyxDQUFDO1lBQ1AsQ0FBQyxDQUFDLENBQUM7UUFDUCxDQUFDLENBQUMsQ0FBQztJQUNQLENBQUM7Q0FDSiJ9