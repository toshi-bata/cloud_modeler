import * as Communicator from "../hoops-web-viewer.mjs";
import { ArrowMarkup } from "./common_utilities.js";
export class NodeSelectOperator {
    constructor(viewer) {
        this._viewer = viewer;
        this._ptFirst;
        this._selectIdArr = new Array(0);
        this._pickConfig = new Communicator.PickConfig(Communicator.SelectionMask.Line);
        this._selectionMsg = "Select edge(s)";
        this._entityLabel = "Edge";
        this._currentList = 0;
        this._entityIdArr = [new Array(0), new Array(0)];
        this._psEntitiesArr = [new Array(0), new Array(0)];
        this._nodeIdArr = [new Array(0), new Array(0)];
        this._isTargetBody;

        this._showNormalArrow = [false, false];
        this._markupHandle = new Array();
    };

    setSelectId(id, id2) {
        this._selectIdArr[0] = id;
        this._selectIdArr[1] = id2;
    }

    setPickConfig(config, isTargetBody) {
        this._pickConfig = config;
        if (Communicator.SelectionMask.Line == this._pickConfig.selectionMask) {
            this._selectionMsg = "Select edge(s)";
            this._entityLabel = "Edge";
        }
        else {
            this._selectionMsg = "Select face(s)";
            this._entityLabel = "Face";
        }

        if (isTargetBody) {
            this._isTargetBody = true;
            this._entityLabel = "Body";
        }
        else {
            this._isTargetBody = false;
        }
    }

    setCurrentList(id) {
        this._currentList = id;
        if (this._isTargetBody) {
            this._viewer.model.setNodesHighlighted(this._nodeIdArr[id], true);           
        }
        else {
            for (let i = 0; i < this._entityIdArr[id].length; i++) {
                const entityId = this._entityIdArr[id][i];
                const nodeId = this._nodeIdArr[id][i];
                if (Communicator.SelectionMask.Line == this._pickConfig.selectionMask) {
                    this._viewer.model.setNodeLineHighlighted(nodeId, entityId, true);
                }
                else {
                    this._viewer.model.setNodeFaceHighlighted(nodeId, entityId, true);
                }
            }
        }
    }

    setShowNormalArrow(flgArr) {
        this._showNormalArrow = flgArr;
    }

    _updateSelectList(id) {
        $("select#" + this._selectIdArr[id] + " option").remove();
        if (this._psEntitiesArr[id].length) {
            // Highlight entities
            this._viewer.model.resetModelHighlight().then(() => {
                if (this._isTargetBody) {
                    this._viewer.model.setNodesHighlighted(this._nodeIdArr[id], true);           
                }
                else {
                    for (let i = 0; i < this._psEntitiesArr[id].length; i++) {
                        const nodeId = this._nodeIdArr[id][i];
                        const entityId = this._entityIdArr[id][i];
                        if (Communicator.SelectionMask.Line == this._pickConfig.selectionMask) {
                            this._viewer.model.setNodeLineHighlighted(nodeId, entityId, true);
                        }
                        else {
                            this._viewer.model.setNodeFaceHighlighted(nodeId, entityId, true);
                        }
                    }
                }
            });

            // Updte select options
            for (let entity of this._psEntitiesArr[id]) {
                $("#" + this._selectIdArr[id]).append($("<option>").val(entity).text(this._entityLabel + "<" + entity + ">"));
            }

            if (this._showNormalArrow[id] && this._entityIdArr[id].length) {
                let flg = false;
                let nodeId = this._nodeIdArr[id][0];
                this._viewer.model.getNodeMeshData(nodeId).then((meshDataCopy) => {
                    for (let i = 0; i < this._entityIdArr[id].length; i++) {
                        if (nodeId == this._nodeIdArr[id][i]) {
                            let faces = meshDataCopy.faces;
                            if (faces.hasNormals) {
                                const faceId = this._entityIdArr[id][i];
                                let face = faces.element(faceId);
                                let it = face.iterate();
                                while(!it.done()) {
                                    const vertex = it.next();
                                    let normal = new Communicator.Point3(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
                                    const stPnt = new Communicator.Point3(vertex.position[0], vertex.position[1], vertex.position[2]);
                                    let enPnt = normal.copy();
                                    enPnt.scale(15);
                                    enPnt = enPnt.add(stPnt);

                                    let markupItem = new ArrowMarkup(this._viewer, new Communicator.Color(255, 255, 0));
                                    markupItem.setStartEndCap(Communicator.Markup.Shapes.EndcapType.Arrowhead, Communicator.Markup.Shapes.EndcapType.None);
                                    markupItem.setPosiiton(stPnt, enPnt);
                                    this._markupHandle.push(this._viewer.markupManager.registerMarkup(markupItem, this._viewer.view));

                                    if (!flg) {
                                        normal.scale(-1);
                                        $("#loadVecX").val(Number(normal.x));
                                        $("#loadVecY").val(Number(normal.y));
                                        $("#loadVecZ").val(Number(normal.z));
                                        flg = true;
                                    }
                                }
                            }
                        }
                    }
                });
            }
        }
        else {
            this._viewer.model.resetModelHighlight();
            $("#" + this._selectIdArr[id]).append($("<option>").val("").text(this._selectionMsg).prop("disabled", true));
        }
    }

    setPreSelection(nodes1, nodes2, faces1, faces2, psEntities1, psEntities2) {
        this._nodeIdArr = [nodes1, nodes2];
        this._entityIdArr = [faces1, faces2];
        this._psEntitiesArr = [psEntities1, psEntities2];

        this._updateSelectList(0);
        this._updateSelectList(1);
    }

    onActivate() {
        $("select#" + this._selectIdArr[0] + " option").remove();
        $("#" + this._selectIdArr[0]).append($("<option>").val("").text(this._selectionMsg).prop("disabled", true));

        if (undefined != this._selectIdArr[1]) {
            $("select#" + this._selectIdArr[1] + " option").remove();
            $("#" + this._selectIdArr[1]).append($("<option>").val("").text(this._selectionMsg).prop("disabled", true));
        }
    }
    
    onDeactivate() {
        this._viewer.model.setNodesHighlighted([this._viewer.model.getAbsoluteRootNode()], false);

        for (let handle of this._markupHandle) {
            this._viewer.markupManager.unregisterMarkup(handle, this._viewer.view);
        }
        this._markupHandle.length = 0;

        this._showNormalArrow = [false, false];

        this._currentList = 0;
        this._entityIdArr[0].length = 0;
        this._entityIdArr[1].length = 0;

        this._psEntitiesArr[0].length = 0;
        this._psEntitiesArr[1].length = 0;

        this._nodeIdArr[0].length = 0;
        this._nodeIdArr[1].length = 0;
    }

    onMouseDown(event) {
        this._ptFirst = event.getPosition();
    }

    onMouseUp(event) {
        const ptCurrent = event.getPosition();
        const pointDistance = Communicator.Point2.subtract(this._ptFirst, ptCurrent).length();
        
        if (5 > pointDistance && event.getButton() == Communicator.Button.Left) {
            this._viewer.view.pickFromPoint(ptCurrent, this._pickConfig).then((selectionItem) => {
                const nodeId = selectionItem.getNodeId();
                if (null != nodeId) {
                    this.selected(selectionItem);
                    event.setHandled(true);
                }
            });
        }
    }
    
    getNodes(id) {
        return this._nodeIdArr[id].slice();
    }

    getPsEntities(id) {
        return this._psEntitiesArr[id].slice();
    }

    getEntities(id) {
        return this._entityIdArr[id].slice();
    }
    
    selected(selectionItem) {
        this._viewer.model.setNodesHighlighted([this._viewer.model.getAbsoluteRootNode()], false).then(() => {
            const nodeId = selectionItem.getNodeId();
            let entityTag;
            let entityId;
            if (Communicator.SelectionMask.Line == this._pickConfig.selectionMask) {
                const lineEntity = selectionItem.getLineEntity();
                entityId = lineEntity.getLineId();
                const edgeInfo = Window.cloudModeler.getEdgeInfo(nodeId, entityId);
                entityTag = edgeInfo.edgeTag;
            }
            else {
                const faceEntity = selectionItem.getFaceEntity();
                entityId = faceEntity.getCadFaceIndex();
                const faceInfo = Window.cloudModeler.getFaceInfo(nodeId, entityId);
                entityTag = faceInfo.faceTag;

                if (this._isTargetBody) {
                    entityTag = faceInfo.bodyTag;
                    if (undefined != faceInfo.instanceTag) {
                        entityTag = faceInfo.instanceTag;
                    }
                }
            }

            const i = this._psEntitiesArr[this._currentList].indexOf(entityTag);
            if(-1 == i) {
                this._psEntitiesArr[this._currentList].push(entityTag);
                this._entityIdArr[this._currentList].push(entityId);
                this._nodeIdArr[this._currentList].push(nodeId);
            } else {
                this._psEntitiesArr[this._currentList].splice(i, 1);
                this._entityIdArr[this._currentList].splice(i, 1);
                this._nodeIdArr[this._currentList].splice(i, 1);
            }
            
            
            for (let handle of this._markupHandle) {
                this._viewer.markupManager.unregisterMarkup(handle, this._viewer.view);
            }
            this._markupHandle.length = 0;

            this._updateSelectList(this._currentList);

            if (this._isTargetBody && 0 == this._currentList && 1 == this._entityIdArr[this._currentList].length) {
                $("#toolBody").focus();
            }
        });
    }
}

export class entityOneClickOperator {
    constructor(viewer) {
        this._viewer = viewer;
        this._ptFirst;
        this._command;
        this._pickConfig;
    }

    setCommand(command, pickConfig) {
        this._command = command;
        this._pickConfig = pickConfig;
    }

    onTouchEnd (event) {
        this._onSelect(event);
    }

    onMouseDown(event) {
        this._ptFirst = event.getPosition();
    }

    onMouseUp (event) {
        const ptCurrent = event.getPosition();
        const pointDistance = Communicator.Point2.subtract(this._ptFirst, ptCurrent).length();
        
        if (5 > pointDistance && event.getButton() == Communicator.Button.Left) {
            this._onSelect(event);
            event.setHandled(true);
        }
    }

    _onSelect (event) {
        this._viewer.view.pickFromPoint(event.getPosition(), this._pickConfig).then((selectionItem) => {
            const nodeId = selectionItem.getNodeId();
            if (nodeId) {
                const body = Window.cloudModeler.getBodyId(nodeId);
                if (null == body) return;

                const psBodyId = body.bodyTag;
                const psInstanceId = body.instanceTag;

                let entityTag = psBodyId;
                if (undefined != psInstanceId) {
                    entityTag = psInstanceId;
                }

                if (psBodyId != undefined) {
                    this._viewer.model.setNodesHighlighted([nodeId], true);           

                    switch (this._command) {
                        case "Hollow": {
                            Window.cloudModeler.invokeHollow(entityTag);
                        } break;
                        case "Offset": {
                            Window.cloudModeler.invokeOffset(entityTag);
                        } break;
                        case "DeleteBody": {
                            const params = { body: entityTag };

                            Window.cloudModeler.invokeModelerDeleteBody(this._command, "delete", entityTag, params);

                            $("#info1").html("");
                            $("#" + this._command + "Dlg").hide();
        
                            Window.cloudModeler.resetCommand();
                            Window.cloudModeler.resetOperator();
                        } break;
                        case "MassProps": {
                            const params = { body: psBodyId };
                            Window.cloudModeler.invokeModelerInquiry(this._command, params);

                            $("#info1").html("");
                            $("#" + this._command + "Dlg").hide();
        
                            Window.cloudModeler.resetCommand();
                            Window.cloudModeler.resetOperator();
                        } break;
                        case "FR_Holes": {
                            const size = $("#maxDiameter").val();
                            const params = { size: size, body: psBodyId };
                            Window.cloudModeler.invokeModelerInquiry(this._command, params);
                        } break;
                        case "FR_Concaves": {
                            const min = $("#concaveAngleMin").val();
                            const max = $("#concaveAngleMax").val();
                            const params = { min: min, max: max, body: psBodyId };
                            Window.cloudModeler.invokeModelerInquiry(this._command, params);
                        } break;
                        case "VolumeMesh": {
                            const size = $("#meshSize").val();
                            const str = this._command;
                            let params = { body: psBodyId, meshSize: size};
                            Window.cloudModeler.invokeCAE(str, params);

                            $("#info1").html("");
                            $("#" + this._command + "Dlg").hide();
        
                            Window.cloudModeler.resetCommand();
                            Window.cloudModeler.resetOperator();
                        } break;
                        default: {
                        } break;
                    }
                }
            }
        });
        
    }
}
