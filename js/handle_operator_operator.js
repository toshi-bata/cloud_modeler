import * as Communicator from "../hoops-web-viewer.mjs";
import { ArrowMarkup, MarkerMarkup } from "./common_utilities.js";
export class HandleOperatorOperator {
    constructor(viewer, handleOp, handleOpHandle) {
        this._viewer = viewer;
        this._handleOp = handleOp;
        this._isBusy = false;
        this._preNodeId;
        this._preEntiryId;
        this._centerPnt;
        this._centerAxis;
        this._preMarkupHandles = new Array(0);
        this._meshIDs = new Array(0);

        // Set myHandleOp
        if (Communicator.OperatorId.Handle != handleOpHandle) {
            const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Handle);
            if (-1 != id) {
                this._viewer.operatorManager.set(handleOpHandle, id);
            }
            else {
                this._viewer.operatorManager.push(handleOpHandle)
            }
        }

        // Create marker line for pre-select
        this._preMarkupItem = new ArrowMarkup(this._viewer, new Communicator.Color(255, 0, 0));
        this._preMarkupItem.setStartEndCap(Communicator.Markup.Shapes.EndcapType.None, Communicator.Markup.Shapes.EndcapType.None);

        // Create marker circle for pre-select
        this._preMarkupCircleItem = new MarkerMarkup(this._viewer, new Communicator.Color(255, 0, 0));
    }

    onMouseMove(event) {
        // Stop pre-selection while animation
        if (this._isBusy) {
            return;
        }

        // Avoid Erroe: Cannot pick from outside the canvas area.
        const canvasSize = this._viewer.view.getCanvasSize();
        const position = event.getPosition();
        if (0 >= position.x || 0 >= position.y || canvasSize.x <= position.x || canvasSize.y <= position.y) {
            // console.log(position);
            return;
        }

        const pickConfig = new Communicator.PickConfig(Communicator.SelectionMask.Face | Communicator.SelectionMask.Line);
        this._viewer.getView().pickFromPoint(event.getPosition(), pickConfig).then((selectionItem)=> {
            if (selectionItem.isFaceSelection()) {
                // Get selectd node and face IDs
                const nodeId = selectionItem.getNodeId();
                const faceEntity = selectionItem.getFaceEntity();
                const faceId = faceEntity.getCadFaceIndex();

                if (0 > faceId) {
                    return;
                }
                
                // Retur if same node & line IDs are pre-selected
                // if (this._preNodeId == nodeId && this._preEntiryId == faceId) {
                //     return;
                // } 

                this.reset();

                // Get pre-select face properties
                this._viewer.model.getFaceProperty(nodeId, faceId).then((prop)=>{
                    if (null == prop) {
                        this._viewer.model.getNodesBounding([nodeId]).then((box) => {
                            this._centerAxis = new Communicator.Point3(0, 0, 1);
                            this._centerPnt = box.min.add(box.max.copy().subtract(box.min).scale(0.5));
                        });
                        return;
                    }

                    const netMatrix = this._viewer.model.getNodeNetMatrix(nodeId);

                    if (undefined != prop.radius) {
                        // Cylinder face
                        const r = prop.radius;

                        const center = netMatrix.transform(prop.origin);
                        this._centerAxis = rotatePoint(netMatrix, prop.normal);
                        this._centerAxis.normalize();

                        // Compute start and end points of center axis using face bounding
                        let stPnt = center.copy();
                        let enPnt = center.copy().add(this._centerAxis);
                        
                        const box = faceEntity.getBounding();
                        let planeMin = Communicator.Plane.createFromPointAndNormal(box.min, this._centerAxis);
                        let planeMax = Communicator.Plane.createFromPointAndNormal(box.max, this._centerAxis);

                        Communicator.Util.intersectionPlaneLine2(stPnt, enPnt, planeMin, stPnt);
                        Communicator.Util.intersectionPlaneLine2(stPnt, enPnt, planeMax, enPnt);

                        this._centerAxis = enPnt.copy().subtract(center);
                        this._centerAxis.normalize();

                        // Compute center point
                        this._centerPnt = enPnt.copy().subtract(stPnt)
                        this._centerPnt = this._centerPnt.scale(0.5);
                        this._centerPnt = stPnt.copy().add(this._centerPnt);

                        // Extend start and end points
                        const distance = Communicator.Point3.distance(center, enPnt);
                        let inc = Communicator.Point3.scale(this._centerAxis.copy(), distance * 0.2);
                        stPnt = stPnt.subtract(inc);
                        enPnt = enPnt.add(inc);

                        // Draw markup line at center of pre-select arc / circle face
                        this._preMarkupItem.setPosiiton(stPnt, enPnt);
                        const guid  = this._viewer.markupManager.registerMarkup(this._preMarkupItem);
                        this._preMarkupHandles.push(guid);

                        this._preNodeId = nodeId;
                        this._preEntiryId = faceId;
                    }
                    else if (undefined != prop.normal) {
                        let normal = rotatePoint(netMatrix, prop.normal);
                        let stPnt = selectionItem.getPosition();
                        let enPnt = stPnt.copy().add(normal.copy().scale(10));

                        this._centerAxis = enPnt.copy().subtract(stPnt);
                        this._centerAxis.normalize();

                        this._centerPnt = stPnt;

                        // Draw markup line at center of pre-select arc / circle face
                        this._preMarkupCircleItem.setPosiiton(stPnt);
                        const guid  = this._viewer.markupManager.registerMarkup(this._preMarkupCircleItem);
                        this._preMarkupHandles.push(guid);

                        this._preNodeId = nodeId;
                        this._preEntiryId = faceId;
                    }

                });
            }
            else if (selectionItem.isLineSelection()) {
                // Get selectd node and face IDs
                const nodeId = selectionItem.getNodeId();
                const lineEntity = selectionItem.getLineEntity();
                const lineId = lineEntity.getLineId();

                // Retur if same node & line IDs are pre-selected
                if (this._preNodeId == nodeId && this._preEntiryId == lineId) {
                    return;
                } 

                this.reset();

                // Check whether straight line
                const points = lineEntity.getPoints()
                if (2 != points.length) {
                    return;
                }

                let stPnt = points[0];
                let enPnt = points[1];

                this._centerAxis = enPnt.copy().subtract(stPnt);
                this._centerAxis.normalize();

                // Compute center point
                this._centerPnt = enPnt.copy().subtract(stPnt)
                this._centerPnt = this._centerPnt.scale(0.5);
                this._centerPnt = stPnt.copy().add(this._centerPnt);

                // Draw markup line
                this._preMarkupItem.setPosiiton(stPnt, enPnt);
                const guid  = this._viewer.markupManager.registerMarkup(this._preMarkupItem);
                this._preMarkupHandles.push(guid);

                this._preNodeId = nodeId;
                this._preEntiryId = lineId;

            }
            else {
                this.reset();
            }
        });
    }

    onMouseDown() {
        this._isBusy = true;
    }

    onMouseUp() {
        this._isBusy = false;
    }
    
    reset() {
        // Remove previous markup line and reset highlight
        if (this._preMarkupHandles.length) {
            for (let guid of this._preMarkupHandles) {
                this._viewer.markupManager.unregisterMarkup(guid);
            }
            this._preMarkupHandles.length = 0;
        }

        this._preNodeId = undefined;
        this._preEntiryId = undefined;
    }

    addHandle(nodeId) {
        // Show handle
        this._handleOp.addHandles([nodeId], this._centerPnt, 0).then(() => {
            // Update handle directon
            const ret = vectorsAngleDeg(new Communicator.Point3(0, 0, 1), this._centerAxis);
            if (undefined != ret.axis) {
                const rotation = Communicator.Matrix.createFromOffAxisRotation(ret.axis, ret.angleDeg);
                this._handleOp.updatePosition(new Communicator.Point3(0, 0, 0), rotation, true, 0);
            }
            this.reset();
        });
    }

    createStepLine(stPnt, enPnt) {
        // Draw step line
        const vertices = [stPnt.x, stPnt.y, stPnt.z, enPnt.x, enPnt.y, enPnt.z]
        let meshData = new Communicator.MeshData();
        meshData.addPolyline(vertices);
        this._viewer.model.createMesh(meshData).then((meshId)=> {
            const lineColor = new Communicator.Color(128, 128, 128);
            const meshInstanceData = new Communicator.MeshInstanceData(meshId, undefined, "guide_line", undefined, lineColor);
            this._viewer.model.createMeshInstance(meshInstanceData).then((instacdId)=> {
                this._meshIDs.push(instacdId);
            });
        });
    }

    setHandleNodeId(nodeId) {
        this._handleOp.setNodeIds([nodeId], 0);
    }

    resetNodesTransform() {
        this.rewind();
    }

    rewind() {
        if (this._meshIDs.length) {
            // Delete step lines
            this._viewer.model.deleteMeshInstances(this._meshIDs);
            this._meshIDs.length = 0;
        }

        this._handleOp.removeHandles();

        this._viewer.model.resetModelHighlight();
        this._viewer.model.resetNodesTransform();
    }
    
    removeHandles() {
        this._handleOp.removeHandles();
    }
}