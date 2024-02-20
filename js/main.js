class Main {
    constructor () {
        this._procServerCaller;
        this._psServerPort;
        this._psServerCaller;
        this._viewer;
        this._ceeViewer;
        this._sessionId;
        this._bodyMemberArr = [];
        this._entityOneClickOp;
        this._entityOneClickOpHandle;
        this._nodeSelectOp;
        this._nodeSelectOpHandle;
        this._handleOpOp;
        this._handleOpOpHandle;
        this._historys = new Array();
        this._currentHistory;
        this._faceColorMap;
        this._clipFunc;
        this.m_cuttingPlaneDialog = null;
        this.m_particleTraceDialog = null;
        this._cuttingSection;
        this._constFaceArr = new Array(0);
        this._loadFaceArr = new Array(0);
        this._materials;
        this._warningTimer = 0;
        this._errorTimer = 0;
        this._modelTree;
        this._tempNode = null;
        this._tempMeshIdsArr = new Array();
        this._closestMarkup = null;
        this._bodyMeshCreator;
    }

    start (port, viewerMode, modelName, reverseProxy) {
        this._requestServerProcess(port, reverseProxy).then(() => {
            this._createViewer(viewerMode, modelName, reverseProxy);
            this._initEvents();
            this._initHistorys();
            this._updateToolbar();
            this._updateTimer();
        }).catch((e) => {
            alert(e);
        });
    }

    _requestServerProcess (port, reverseProxy) {
        return new Promise((resolve, reject) => {
            // Create GUID for thie session
            this._sessionId = create_UUID();
            
            // Create PsServer caller
            let psServerURL = window.location.protocol + "//" + window.location.hostname + ":" + port;
            if (undefined != reverseProxy) {
                psServerURL = window.location.protocol + "//" + window.location.hostname + "/httpproxy/" + port;
            }

            this._psServerCaller = new ServerCaller(psServerURL, this._sessionId);

            // Request a new Ps Session
            this._psServerCaller.CallServerPost("Create").then(() => {
                resolve();
            }).catch((error) => {
                reject("PsServer not responding");
            });
        });

    }

    _updateTimer() {
        // Clear current timer
        if (this._warningTimer) {
            clearTimeout(this._warningTimer);
        }
        if (this._errorTimer) {
            clearTimeout(this._errorTimer);
        }

        // Set new timer
        this._warningTimer = setTimeout(() => {
            $("#timeoutWarnDlg").dialog({
                modal:true,
                title:"Timeout",
                buttons: {"Yes": () => {
                    this._updateTimer();
                    $("#timeoutWarnDlg").dialog("close");
                }}
              });
        }, 55 * 60 * 1000);

        this._errorTimer = setTimeout(() => {
            alert("Your server side process has timed out.");
            $("#timeoutWarnDlg").dialog("close");
        }, 60 * 60 * 1000);
    }

    _createViewer (viewerMode, modelName, reverseProxy) {
        createViewer(viewerMode, modelName, "container", reverseProxy).then((hwv) => {
            this._viewer = hwv;
                    
            this._viewer.setCallbacks({
                sceneReady: () => {
                    // Set background color
                    this._viewer.view.setBackgroundColor(new Communicator.Color(255, 255, 255), new Communicator.Color(230, 204, 179));
                    
                    this._viewer.view.getAxisTriad().enable();
                    this._viewer.view.setBackfacesVisible(true);

                    this._viewer.selectionManager.setNodeSelectionColor(new Communicator.Color(128, 255, 255));
                    this._viewer.selectionManager.setNodeElementSelectionColor(new Communicator.Color(255, 255, 0));
                    this._viewer.selectionManager.setNodeElementSelectionOutlineColor(new Communicator.Color(0, 255, 0));
                },
                modelStructureReady: () => {
                    this._layoutPage();

                    // Initialize classes
                    this._bodyMeshCreator = new BodyMeshCreator(this._viewer);

                    // Create model tree
                    this._modelTree = new ModelTree(this._viewer, "#tree");
                    const root = this._viewer.model.getAbsoluteRootNode();
                    this._modelTree.createRoot("Model", String(root));

                    this._clipFunc = new ClipFunc(this._viewer);
                },
                selectionArray: (selectionEvents) => {
                    if (0 == selectionEvents.length) {
                        $("#dataInfo").html('');
                        this._viewer.model.resetModelHighlight();
                        return;
                    }
                    
                    let selItem = selectionEvents[selectionEvents.length - 1].getSelection();
                    let nodeId = selItem.getNodeId();
    
                    const faceEntity = selItem.getFaceEntity();
                    const lineEntity = selItem.getLineEntity();
                    let str = "";
                    if (null != faceEntity) {
                        const faceId = faceEntity.getCadFaceIndex();

                        const faceInfo = this.getFaceInfo(nodeId, faceId);
                        if (undefined != faceInfo) {
                            str = 'Face tag: ' + faceInfo.faceTag + '<br>Body tag: ' + faceInfo.bodyTag;
                            if (null != faceInfo.instanceTag) {
                                str += '<br>Instance tag: ' + faceInfo.instanceTag;
                            }
                        } 
                    }
                    else if (null != lineEntity) {
                        const lineId = lineEntity.getLineId();

                        const edgeInfo = this.getEdgeInfo(nodeId, lineId);
                        if (undefined != edgeInfo) {
                            str = 'Edge tag: ' + edgeInfo.edgeTag + '<br>Bodyt tag: ' + edgeInfo.bodyTag;
                            if (null != edgeInfo.instanceTag) {
                                str += '<br>Instance tag: ' + edgeInfo.instanceTag;
                            }
                        }
                    }

                    $("#dataInfo").html(str);
                    
                    const id = this._viewer.operatorManager.indexOf(this._handleOpOpHandle);
                    if (-1 != id) {
                        // Show handle
                        const selectionEvent = selectionEvents.pop();
                        const selectionType = selectionEvent.getType();
                        if (selectionType != Communicator.SelectionType.None) {
                            const selection = selectionEvent.getSelection();
                            const selectedNode = selection.getNodeId();
                            const parentId = this._viewer.model.getNodeParent(selectedNode);
                
                            // Show handle
                            this._handleOpOp.addHandle(parentId);
                        }
                    }
                },
                handleEventEnd: (event, nodeIds, initialMatrices, newMatrices) => {
                    const history = {
                        type: "transform",
                        nodes: nodeIds,
                        initialMatrices, initialMatrices,
                        newMatrices: newMatrices
                    }

                    this._historys.push(history);
                    this._currentHistory++;

                    this._modelTree.incrementHistory();

                    let promiseArr = new Array(0);
                    for (let i = 0; i < nodeIds.length; i++) {
                        const params = {entity: nodeIds[i], matrix: newMatrices[i].toJson()};
                        promiseArr.push(this._psServerCaller.CallServerPost("Transform", params));
                    }
                    Promise.all(promiseArr);
                },
            });
            
            this._entityOneClickOp = new entityOneClickOperator(this._viewer);
            this._entityOneClickOpHandle = this._viewer.registerCustomOperator(this._entityOneClickOp);
            
            this._nodeSelectOp = new NodeSelectOperator(this._viewer);
            this._nodeSelectOpHandle = this._viewer.registerCustomOperator(this._nodeSelectOp);
            
            // Register HandleOperator operator
            const handleOp = this._viewer.operatorManager.getOperator(Communicator.OperatorId.Handle);
            const handleOpHandle = Communicator.OperatorId.Handle;
            this._handleOpOp = new HandleOperatorOperator(this._viewer, handleOp, handleOpHandle);
            this._handleOpOpHandle = this._viewer.operatorManager.registerCustomOperator(this._handleOpOp);

            this._viewer.start();

        });
    }

    layoutTree() {
        const conOffset = $("#container").offset();

        const canvasSize = this._viewer.view.getCanvasSize();
        const conWidth = canvasSize.x;
        const conHeight = canvasSize.y;

        const treeWidth = $("#modelTree").innerWidth();
        
        const topOff = 80;
        const rightMargin = 40;
        let left = conWidth - treeWidth - rightMargin;
        if (0 > left) {
            left = 0;
        }

        $("#modelTree").offset({
            top: conOffset.top + topOff,
            left: conOffset.left + left
        });
        
        $("#modelTree").css("width", "auto");
        if (conWidth < treeWidth + rightMargin) {
            $("#modelTree").width(conWidth - rightMargin);
        }
        
        $("#modelTree").css("height", "auto");
        const treeHeight =  $("#modelTree").height();
        if (conHeight < treeHeight + topOff) {
            $("#modelTree").height(conHeight - topOff);
        }
    }

    _layoutPage() {
        this._viewer.resizeCanvas();
        this.layoutTree();
    }

    _initEvents () {
        let resizeTimer;
        let interval = Math.floor(1000 / 60 * 10);
        $(window).resize(() => {
            if (resizeTimer !== false) {
                clearTimeout(resizeTimer);
            }
            resizeTimer = setTimeout(() => {
                this._layoutPage();
            }, interval);
        });

        // File dropped
        $("#container, #myGlCanvas").on("dragover", (e) => {
            e.originalEvent.preventDefault(); 
        }).on("drop", (e) => {
            console.log("File dropped");
            e.originalEvent.preventDefault();
            
            let dt = e.originalEvent.dataTransfer;
            if (dt.items) {
                for (let i = 0; i < dt.items.length; i++) {
                    if (dt.items[i].kind == "file") {
                        let f = dt.items[i].getAsFile();
                        console.log(f.name);
                        this.invokeFileImport(f);
                    }
                }
            }
        });

        // Simple command
        $(".simpleCmd").on("click", (e) => {
            this.resetCommand();
            this.resetOperator();

            let command = $(e.currentTarget).data("command");
            let isOn = $(e.currentTarget).data("on");

            switch(command) {
                case "New": {
                    $("#loadingImage").show();
                    this._psServerCaller.CallServerPost("Reset").then(() => {
                        $("#loadingImage").hide();
                    }).catch((error) => {
                        $("#loadingImage").hide();
                        alert("New command failed");
                    });
                    this._initHistorys();
                    this._updateToolbar();
                    this._constFaceArr.length = 0;
                    this._loadFaceArr.length = 0;
                    const root = this._viewer.model.getAbsoluteRootNode();
                    this._modelTree.createRoot("Model", String(root));
                } break;
                case "Undo": this.invokeModelerUndo(); break;
                case "Redo": this.invokeModelerRedo(); break;
                case "SwitchProjection": {
                    const projection = this._viewer.view.getProjectionMode()
                    if (Communicator.Projection.Perspective == projection) {
                        this._viewer.view.setProjectionMode(Communicator.Projection.Orthographic);
                        $(e.currentTarget).children('img').attr('src', 'css/images/orthographic.png');
                    }
                    else if (Communicator.Projection.Orthographic == projection) {
                        this._viewer.view.setProjectionMode(Communicator.Projection.Perspective);
                        $(e.currentTarget).children('img').attr('src', 'css/images/perspective.png');
                    }
                } break;
                case "SectionCAD": {
                    if (undefined == this._cuttingSection) {
                        this._viewer.model.getModelBounding(true, false).then((box) => {
                            // specify capping face color
                            this._viewer.cuttingManager.setCappingFaceColor(new Communicator.Color(0, 255, 255));
                            
                            // create plane for cutting plane
                            let plane = new Communicator.Plane();
                            plane.normal.set(0, -1, 0);
                            plane.d = 0;
                            
                            // create array for cutting reference geometry
                            let referenceGeometry = [];
                            referenceGeometry.push(new Communicator.Point3(box.max.x, 0, box.max.z));
                            referenceGeometry.push(new Communicator.Point3(box.max.x, 0, box.min.z));
                            referenceGeometry.push(new Communicator.Point3(box.min.x, 0, box.min.z));
                            referenceGeometry.push(new Communicator.Point3(box.min.x, 0, box.max.z));
                            
                            // get cuting section and activate
                            this._cuttingSection = this._viewer.cuttingManager.getCuttingSection(0);
                            this._cuttingSection.addPlane(plane, referenceGeometry);
                            this._cuttingSection.activate();
                        });

                    }
                    else {
                        this._cuttingSection.deactivate();
                        this._cuttingSection.removePlane(0);
                        this._cuttingSection = undefined;
                    }
                } break;
                case "Transform": {
                    const id = this._viewer.operatorManager.indexOf(this._handleOpOpHandle);
                    if (-1 == id) {
                        this._viewer.operatorManager.push(this._handleOpOpHandle);
                    }
                    else {
                        this._viewer.operatorManager.remove(this._handleOpOpHandle);
                    }

                } break;
                case "CheckCollision": {
                    if (!isOn) {
                        $("#loadingImage").show();

                        const params = {};
                        const selItem = this._viewer.selectionManager.getLast();
                        if (null != selItem) {
                            let nodeId = selItem.getNodeId();
                            if (0 > nodeId) {
                                nodeId = this._viewer.model.getNodeParent(nodeId);
                            }
                            params.target = nodeId;
                        }
                        else {
                            params.target = -1;
                        }

                        const startTime = performance.now();
                        this._psServerCaller.CallServerPost(command, params, "float").then((results) => {
                            const endTime = performance.now();
                            const time = endTime - startTime;

                            if (0 == results[0]) {
                                $("#dataInfo").html('No collision<br>shortest distance = ' + results[1].toFixed(2) + '<br>Process time = ' + time.toFixed(0) + 'msec');
            
                                if (2 < results.length) {
                                    let pnt1 = new Communicator.Point3(results[2], results[3], results[4]);
                                    let pnt2 = new Communicator.Point3(results[5], results[6], results[7]);
                                    let closestMarkup = new ClosestLineMarkup(this._viewer, pnt1, pnt2, String(results[1].toFixed(2)));
                                    var markupHandle = this._viewer.markupManager.registerMarkup(closestMarkup);
                                    this._closestMarkup = markupHandle;
                                }
                            }
                            else {
                                let arr = Array.from(results);
                                let count = parseInt(arr.shift());
            
                                let promiseArr = new Array(0);

                                this._tempMeshIdsArr.length = 0;
                                const root = this._viewer.model.getAbsoluteRootNode();
                                this._tempNode = this._viewer.model.createNode(root);

                                let currentId = 0;
                                for (let i = 0; i < count; i++) {
                                    let size = parseInt(arr[currentId++]);
                                    let bodyDataArr = arr.slice(currentId,  currentId + size);
                                    promiseArr.push(this._bodyMeshCreator.addMesh(bodyDataArr, null, null, this._tempNode));
                                    currentId += size;
                                }
            
                                Promise.all(promiseArr).then((bodies) => {
                                    // Get mesh ID of collision bodies
                                    for (let body of bodies) {
                                        if (null != body.meshInstanceData) {
                                            this._tempMeshIdsArr.push(body.meshInstanceData.getMeshId());
                                        }
                                    }

                                    // Set parts transparent 
                                    let nodeArr = new Array(0);
                                    for (let body of this._bodyMemberArr) {
                                        let nodeId = body.bodyTag;
                                        if (null != body.instanceTag) {
                                            nodeId = body.instanceTag;
                                        }
                                        nodeArr.push(nodeId);
                                    }
                                    this._viewer.model.setNodesOpacity(nodeArr, 0.5);

                                    // Set collision parts color
                                    this._viewer.model.setNodesFaceColor([this._tempNode], new Communicator.Color(255, 255, 128));

                                    $("#dataInfo").html(count + ' collision(s)<br>Process time = ' + time.toFixed(0) + 'msec');
                                })
                            }
                            $("#loadingImage").hide();
                
                            $("#loadingImage").hide();
                        }).catch((error) => {
                            $("#loadingImage").hide();
                            alert("Check Collision failed");
                        });
                    }
                } break;
                case "Silhouette": {
                    if (!isOn) {
                        $("#loadingImage").show();

                        // Get camera ray
                        const camera = this._viewer.view.getCamera();
                        const target = camera.getTarget();
                        const position = camera.getPosition();
                        let ray = target.copy().subtract(position);
                        ray.normalize();

                        // Get model bounding
                        this._viewer.model.getModelBounding(true, false).then((box) => {
                            // Compute silhouette position
                            const min = box.min;
                            const max = box.max;
                            let center = max.copy().subtract(min).scale(0.5);
                            const diagonal = center.length();
                            center = min.copy().add(center);
                            const pos = center.copy().add(ray.copy().scale(diagonal));

                            const params = {
                                xRay: ray.x, yRay: ray.y, zRay: ray.z,
                                xPos: position.x, yPos: position.y, zPos: position.z 
                            };

                            const startTime = performance.now();
                            this._psServerCaller.CallServerPost(command, params, "float").then((results) => {
                                const endTime = performance.now();
                                const time = endTime - startTime;

                                if (results.length) {
                                    let arr = Array.from(results);
                                    let area = Number(arr.pop())  * 1000 * 1000;
                                    $("#dataInfo").html('Area = ' + String(area.toFixed(2)).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,') + 'mm2');
                                    
                                    this._tempMeshIdsArr.length = 0;
                                    const root = this._viewer.model.getAbsoluteRootNode();
                                    this._tempNode = this._viewer.model.createNode(root);
                                    this._bodyMeshCreator.addMesh(arr, null, null, this._tempNode).then((body) => {
                                        // Get mesh ID of collision bodies
                                        if (null != body.meshInstanceData) {
                                            this._tempMeshIdsArr.push(body.meshInstanceData.getMeshId());
                                        }

                                        // Set silhouette face color
                                        this._viewer.model.setNodesFaceColor([this._tempNode], new Communicator.Color(255, 255, 128));
                                    });
                                }
                                else {
                                    alert('Compute silhouette failed.');
                                }
                                $("#loadingImage").hide();
                            }).catch((error) => {
                                $("#loadingImage").hide();
                                alert("Compute Silhouette failed");
                            });

                        });

                    }
                } break;
                case "DownloadCAD": {
                    $('#DownloadDlg').dialog('open');
                } break;
                case "BackToCAD":           this._backToCAD();                      break;
                case "ModelMesh":           this._ceeViewer.toggleModelMesh();      break;
                case "Opacity":             this._ceeViewer.toggleOpacity();        break;
                case "ShowNodeAveraged":    this._ceeViewer.toggleNodeAveraged();   break;
                case "ShowMaxValue":        this._ceeViewer.toggleMaxValue();       break;
                case "CreateSeparator": {
                    let params = { 
                        shape: "B",
                        xSize:9, ySize: 100, zSize: 1, 
                        xOff: 4.5, yOff: 0, zOff: 9,
                        xDir: 1, yDir: 0, zDir: 0,
                        xAxis: 0, yAxis: 0, zAxis: 1
                    };
                    this.invokeModelerCreateEdit("CreateSolid", "create", undefined, params).then(() => {
                        params.xSize = 80;
                        params.ySize = 9;
                        params.zSize = 1;
                        params.xOff = 49;
                        params.yOff = 0;
                        params.zOff = 9 
                        
                        this.invokeModelerCreateEdit("CreateSolid", "create", undefined, params).then(() => {
                            params.xSize = 1;
                            params.ySize = 9;
                            params.zSize = 96;
                            params.xOff = 9.5;
                            params.yOff = 0;
                            params.zOff = 9 
                            this.invokeModelerCreateEdit("CreateSolid", "create", undefined, params).then(() => {
                            });
                        });
                    });
                } break;
                default: {
                } break;
            }
        });

        $(".dlgOkCmd").on("click", (e) => {
            let isOn = $(e.currentTarget).data("on");

            this.resetCommand();
            this.resetOperator();

            if(!isOn) {
                $(e.currentTarget).data("on", true).css("background-color", "khaki");

                let command = $(e.currentTarget).data("command");
                let type = $(e.currentTarget).data("type");
                $("#" + command + "Dlg").show();
                $("#okCancel").show();

                const off = $("#" + command + "Dlg").offset();
                const h = $("#" + command + "Dlg").height();
                $('#okCancel').offset({top: (off.top + h + 10)});
                
                $("#OkBtn").removeData();
                $("#OkBtn").attr({ 
                    'data-command': command,
                    'data-type': type
                });

                switch (command) {
                    case "CreateSolid": {
                        $("#info1").show().html("Create Solid:");

                        const shape = $('input:radio[name="solidType"]:checked').val();
                        switch (shape) {
                            case "B": $("#blockSize").show(); break;
                            case "Y": $("#cylinderSize").show(); break;
                            case "P": $("#prismSize").show(); break;
                            case "C": $("#coneSize").show(); break;
                            case "T": $("#torusSize").show(); break;
                            case "S": $("#sphereSize").show(); break;
                            default: break;
                        }
                        
                        $("#offsetInput").show();

                        switch (shape) {
                            case "B": 
                            case "Y": 
                            case "P": 
                            case "C": 
                            case "T": $("#dirInput").show(); break;
                            case "S": $("#dirInput").hide(); break;
                            default: break;
                        }

                        const off2 = $("#offsetInput").offset();
                        const h2 = $("#offsetInput").height();
                        $('#okCancel').offset({top: (off2.top + h2 + 10)});
                    } break;
                    case "Boolean": {
                        $("#info1").show().html("Boolean: Select target and tool bodies.");
                        $("#targetBody").focus();
                        
                        this._nodeSelectOp.setSelectId("targetBody", "toolBody");
                        this._nodeSelectOp.setPickConfig(new Communicator.PickConfig(Communicator.SelectionMask.Face), true);
                        const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Select);
                        this._viewer.operatorManager.set(this._nodeSelectOpHandle, id);
                    } break;
                    case "Blend": {
                        $("#info1").show().html("Blend R: Select edge(s) to blend.");
                        $("#targetEdgesBlend").focus();
                        this._nodeSelectOp.setSelectId("targetEdgesBlend");
                        this._nodeSelectOp.setPickConfig(new Communicator.PickConfig(Communicator.SelectionMask.Line));
                        const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Select);
                        this._viewer.operatorManager.set(this._nodeSelectOpHandle, id);
                    } break;
                    case "Hollow": {
                        $("#info1").show().html("Hollow: Select face(s) to pierce.");
                        $("#pierceFacesHollow").focus();
                        this._nodeSelectOp.setSelectId("pierceFacesHollow");
                        this._nodeSelectOp.setPickConfig(new Communicator.PickConfig(Communicator.SelectionMask.Face));
                        const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Select);
                        this._viewer.operatorManager.set(this._nodeSelectOpHandle, id);
                    } break;
                    case "Offset": {
                        $("#info1").show().html("Offset: Select face(s) to offset.");
                        $("#offsetFaces").focus();
                        this._nodeSelectOp.setSelectId("offsetFaces");
                        this._nodeSelectOp.setPickConfig(new Communicator.PickConfig(Communicator.SelectionMask.Face));
                        const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Select);
                        this._viewer.operatorManager.set(this._nodeSelectOpHandle, id);
                    } break;
                    case "ImprintRo": {
                        $("#info1").show().html("Imprint-Round: Select circle hole edge(s) to offset.");
                        $("#targetEdgesImpRo").focus();
                        this._nodeSelectOp.setSelectId("targetEdgesImpRo");
                        this._nodeSelectOp.setPickConfig(new Communicator.PickConfig(Communicator.SelectionMask.Line));
                        const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Select);
                        this._viewer.operatorManager.set(this._nodeSelectOpHandle, id);
                    } break;
                    case "ImprintFace": {
                        $("#info1").show().html("Imprint-Face: Select face(s) and target body.");
                        $("#toolFacesImpFace").focus();
                        this._nodeSelectOp.setSelectId("toolFacesImpFace", "targetBodyImpFace");
                        this._nodeSelectOp.setPickConfig(new Communicator.PickConfig(Communicator.SelectionMask.Face));
                        const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Select);
                        this._viewer.operatorManager.set(this._nodeSelectOpHandle, id);
                    } break;
                    case "CopyFace": {
                        $("#info1").show().html("Copy Face: Select face(s) to delete.");
                        $("#targetFacesCopy").focus();
                        this._nodeSelectOp.setSelectId("targetFacesCopy");
                        this._nodeSelectOp.setPickConfig(new Communicator.PickConfig(Communicator.SelectionMask.Face));
                        const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Select);
                        this._viewer.operatorManager.set(this._nodeSelectOpHandle, id);
                    } break;
                    case "DeleteFace": {
                        $("#info1").show().html("Delete Face: Select face(s) to delete.");
                        $("#targetFacesDel").focus();
                        this._nodeSelectOp.setSelectId("targetFacesDel");
                        this._nodeSelectOp.setPickConfig(new Communicator.PickConfig(Communicator.SelectionMask.Face));
                        const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Select);
                        this._viewer.operatorManager.set(this._nodeSelectOpHandle, id);
                    } break;
                    case "FR_Holes": {
                        $("select#smallHoles option").remove();
                        $("#info1").show().html("FR Small holes: Select a body to recognize.");
                        $("#smallHoles").focus();
                        $("#OkBtn").val("Delete");

                        this._entityOneClickOp.setCommand(command, new Communicator.PickConfig(Communicator.SelectionMask.Face));
                        const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Select);
                        this._viewer.operatorManager.set(this._entityOneClickOpHandle, id);
                    } break;
                    case "FR_Concaves": {
                        $("select#concaves option").remove();
                        $("#info1").show().html("FR Concave edges: Select a body to recognize.");
                        $("#concaves").focus();
                        $("#OkBtn").val("Chamfer");

                        this._entityOneClickOp.setCommand(command, new Communicator.PickConfig(Communicator.SelectionMask.Face));
                        const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Select);
                        this._viewer.operatorManager.set(this._entityOneClickOpHandle, id);
                    } break;
                    default:
                        break;
                }
            }
        });

        $('input[name="solidType"]:radio').change((e) => {
            const shape = $(e.currentTarget).val();
            $(".sizeInput").hide();
            switch (shape) {
                case "B": $("#blockSize").show(); break;
                case "Y": $("#cylinderSize").show(); break;
                case "P": $("#prismSize").show(); break;
                case "C": $("#coneSize").show(); break;
                case "T": $("#torusSize").show(); break;
                case "S": $("#sphereSize").show(); break;
                default: break;
            }

            switch (shape) {
                case "B": 
                case "Y": 
                case "P": 
                case "C": 
                case "T": $("#dirInput").show(); break;
                case "S": $("#dirInput").hide(); break;
                default: break;
            }
        });

        $('input[name="boolType"]:radio').change((e) => {
            $("#targetBody").focus();
        });

        $("#OkBtn").on("click", (e) => {
            let command = $(e.currentTarget).data("command");
            let type = $(e.currentTarget).data("type");
            
            let str = command;
            let deleteNodes;
            let params;
            switch (command) {
                case "CreateSolid": {
                    let dir = new Communicator.Point3(Number($("#DX").val()), Number($("#DY").val()), Number($("#DZ").val()));
                    dir.normalize();
                    if (isNaN(dir.x) || isNaN(dir.y) || isNaN(dir.z)) {
                        $("#DX").select();
                        return;
                    }

                    const vecAng = vectorsAngleDeg(new Communicator.Point3(0, 0, 1), dir);
                    let axis = new Communicator.Point3(0, 0, 1);
                    let xDir = new Communicator.Point3(1, 0, 0);

                    const shape = $('input:radio[name="solidType"]:checked').val();
                    switch (shape) {
                        case "B": {
                            let val;
                            val = $("#SX").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#SX").select();
                                return;
                            }
                            val = $("#SY").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#SY").select();
                                return;
                            }
                            val = $("#SZ").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#SZ").select();
                                return;
                            }
                            
                            if (0 == vecAng.angleDeg) {}
                            else if (180 == vecAng.angleDeg) {
                                axis = new Communicator.Point3(0, 0, -1);
                            }
                            else {
                                const matrix = Communicator.Matrix.createFromOffAxisRotation(vecAng.axis, vecAng.angleDeg);
                                axis = matrix.transform(axis);
                                xDir = matrix.transform(xDir);
                            }

                            params = { 
                                shape: shape,
                                xSize: $("#SX").val(), ySize: $("#SY").val(), zSize: $("#SZ").val(), 
                                xOff: $("#OX").val(), yOff: $("#OY").val(), zOff: $("#OZ").val(),
                                xDir: xDir.x, yDir: xDir.y, zDir: xDir.z,
                                xAxis: axis.x, yAxis: axis.y, zAxis: axis.z
                            };
                        } break;
                        case "Y": {
                            let val;
                            val = $("#CylR").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#CylR").select();
                                return;
                            }
                            val = $("#CylH").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#CylH").select();
                                return;
                            }

                            if (0 == vecAng.angleDeg) {}
                            else if (180 == vecAng.angleDeg) {
                                axis = new Communicator.Point3(0, 0, -1);
                            }
                            else {
                                const matrix = Communicator.Matrix.createFromOffAxisRotation(vecAng.axis, vecAng.angleDeg);
                                axis = matrix.transform(axis);
                                xDir = matrix.transform(xDir);
                            }

                            params = { 
                                shape: shape,
                                r: $("#CylR").val(), h: $("#CylH").val(), 
                                xOff: $("#OX").val(), yOff: $("#OY").val(), zOff: $("#OZ").val(),
                                xDir: xDir.x, yDir: xDir.y, zDir: xDir.z,
                                xAxis: axis.x, yAxis: axis.y, zAxis: axis.z
                            };
                        } break;
                        case "P": {
                            let val;
                            val = $("#PrismR").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#PrismR").select();
                                return;
                            }
                            val = $("#PrismH").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#PrismH").select();
                                return;
                            }
                            val = $("#PrismN").val();
                            if (isNaN(val) || Number(val) < 3) {
                                $("#PrismN").select();
                                return;
                            }
                            val = Math.floor(Number(val));
                            $("#PrismN").val(val);

                            if (0 == vecAng.angleDeg) {}
                            else if (180 == vecAng.angleDeg) {
                                axis = new Communicator.Point3(0, 0, -1);
                            }
                            else {
                                const matrix = Communicator.Matrix.createFromOffAxisRotation(vecAng.axis, vecAng.angleDeg);
                                axis = matrix.transform(axis);
                                xDir = matrix.transform(xDir);
                            }

                            params = { 
                                shape: shape,
                                r: $("#PrismR").val(), h: $("#PrismH").val(), n: $("#PrismN").val(),
                                xOff: $("#OX").val(), yOff: $("#OY").val(), zOff: $("#OZ").val(),
                                xDir: xDir.x, yDir: xDir.y, zDir: xDir.z,
                                xAxis: axis.x, yAxis: axis.y, zAxis: axis.z
                            };
                        } break;
                        case "C": {
                            let val;
                            val = $("#ConeTR").val();
                            if (isNaN(val) || Number(val) < 0) {
                                $("#ConeTR").select();
                                return;
                            }
                            val = $("#ConeBR").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#ConeBR").select();
                                return;
                            }
                            val = $("#ConeH").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#ConeH").select();
                                return;
                            }
                            if (Number($("#ConeTR").val()) >= Number($("#ConeBR").val())) {
                                $("#ConeBR").select();
                                return;
                            }

                            const height = Number($("#ConeH").val());
                            let offset = new Communicator.Point3(0, 0, height);;
                            if (0 == vecAng.angleDeg) {
                                axis = new Communicator.Point3(0, 0, -1);
                            }
                            else if (180 == vecAng.angleDeg) {
                                axis = new Communicator.Point3(0, 0, 1);
                                offset = new Communicator.Point3(0, 0, 0);
                            }
                            else {
                                const matrix = Communicator.Matrix.createFromOffAxisRotation(vecAng.axis, vecAng.angleDeg);

                                axis = matrix.transform(new Communicator.Point3(0, 0, -1));
                                xDir = matrix.transform(xDir);
                                offset = matrix.transform(offset);
                            }

                            offset.x += Number($("#OX").val());
                            offset.y += Number($("#OY").val());
                            offset.z += Number($("#OZ").val());
                            
                            params = { 
                                shape: shape,
                                topR: $("#ConeTR").val(), bottomR: $("#ConeBR").val(), h: $("#ConeH").val(), 
                                xOff: offset.x, yOff: offset.y, zOff: offset.z,
                                xDir: xDir.x, yDir: xDir.y, zDir: xDir.z,
                                xAxis: axis.x, yAxis: axis.y, zAxis: axis.z
                            };
                        } break;
                        case "T": {
                            var val = $("#MajorR").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#MajorR").select();
                                return;
                            }
                            var val = $("#MinerR").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#MinerR").select();
                                return;
                            }
                            if (Number($("#MinerR").val()) >= Number($("#MajorR").val())) {
                                $("#MajorR").select();
                                return;
                            }

                            if (0 == vecAng.angleDeg) {}
                            else if (180 == vecAng.angleDeg) {
                                axis = new Communicator.Point3(0, 0, -1);
                            }
                            else {
                                const matrix = Communicator.Matrix.createFromOffAxisRotation(vecAng.axis, vecAng.angleDeg);
                                axis = matrix.transform(axis);
                                xDir = matrix.transform(xDir);
                            }

                            params = { 
                                shape: shape,
                                majorR: $("#MajorR").val(), minerR: $("#MinerR").val(), 
                                xOff: $("#OX").val(), yOff: $("#OY").val(), zOff: $("#OZ").val(),
                                xDir: xDir.x, yDir: xDir.y, zDir: xDir.z,
                                xAxis: axis.x, yAxis: axis.y, zAxis: axis.z
                            };
                        } break;
                        case "S": {
                            let val;
                            val = $("#SphereR").val();
                            if (isNaN(val) || Number(val) <= 0) {
                                $("#SphereR").select();
                                return;
                            }

                            if (0 == vecAng.angleDeg) {}
                            else if (180 == vecAng.angleDeg) {
                                axis = new Communicator.Point3(0, 0, -1);
                            }
                            else {
                                const matrix = Communicator.Matrix.createFromOffAxisRotation(vecAng.axis, vecAng.angleDeg);
                                axis = matrix.transform(axis);
                                xDir = matrix.transform(xDir);
                            }

                            params = { 
                                shape: shape,
                                r: $("#SphereR").val(), 
                                xOff: $("#OX").val(), yOff: $("#OY").val(), zOff: $("#OZ").val(),
                                xDir: xDir.x, yDir: xDir.y, zDir: xDir.z,
                                xAxis: axis.x, yAxis: axis.y, zAxis: axis.z
                            };
                        } break;
                        default:
                            break;
                    }
                } break;
                case "Boolean": {
                    const boolType = $('input:radio[name="boolType"]:checked').val();

                    const targetBodyArr = this._nodeSelectOp.getPsEntities(0);
                    const toolBodyArr = this._nodeSelectOp.getPsEntities(1);

                    if (targetBodyArr.length && toolBodyArr.length){
                        $("#loadingImage").show();
                        deleteNodes = targetBodyArr.concat(toolBodyArr);
                        params = {
                            type: boolType,
                            targetEntity: targetBodyArr[0],
                            toolEntities: toolBodyArr
                        };
                    }
                } break;
                case "Blend": {
                    const blendType = $('input:radio[name="blendType"]:checked').val();

                    const size = $("#blendSize").val();
                    if (isNaN(size) || 0 >= Number(size)) {
                        $("#sizeChamf").select();
                        return;
                    }

                    const psEntities = this._nodeSelectOp.getPsEntities(0);
                    if (0 == psEntities.length) {
                        return;
                    }

                    deleteNodes = [];
                    const nodes = this._nodeSelectOp.getNodes(0);
                    for (let nodeId of nodes) {
                        const parent = this._viewer.model.getNodeParent(nodeId);
                        if (-1 == deleteNodes.indexOf(parent)) {
                            deleteNodes = [parent];
                        }
                    }

                    params = { type: blendType, size: size, entities: psEntities };
                } break;
                case "Hollow": {
                    let thickness = $("#hollowThickness").val();
                    if (isNaN(thickness) || 0 >= Number(thickness)) {
                        $("#hollowThickness").select();
                        return;
                    }
                    thickness = 0 - Number(thickness);
            
                    const outside = $("#hollowOutside").prop("checked");
                    if (outside) {
                        thickness *= -1;
                    }

                    const faces = this._nodeSelectOp.getPsEntities(0);

                    deleteNodes = [];
                    const nodes = this._nodeSelectOp.getNodes(0);
                    for (let nodeId of nodes) {
                        const parent = this._viewer.model.getNodeParent(nodeId);
                        if (-1 == deleteNodes.indexOf(parent)) {
                            deleteNodes = [parent];
                        }
                    }
        
                    if (2 < deleteNodes.length) {
                        alert("Please select pierce faces in one body.");
                        return;
                    }

                    params = { thickness: thickness, targetEntity: deleteNodes[0], pierceFaces: faces };
                } break;
                case "Offset": {
                    let value = $("#offsetValue").val();
                    if (isNaN(value) || 0 >= Number(value)) {
                        $("#offsetValue").select();
                        return;
                    }
                    value = Number(value);
            
                    const inside = $("#offsetInside").prop("checked");
                    if (inside) {
                        value *= -1;
                    }

                    const faces = this._nodeSelectOp.getPsEntities(0);

                    deleteNodes = [];
                    const nodes = this._nodeSelectOp.getNodes(0);
                    for (let nodeId of nodes) {
                        const parent = this._viewer.model.getNodeParent(nodeId);
                        if (-1 == deleteNodes.indexOf(parent)) {
                            deleteNodes = [parent];
                        }
                    }
        
                    if (2 < deleteNodes.length) {
                        alert("Please select offset faces in one body.");
                        return;
                    }

                    params = { value: value, targetEntity: deleteNodes[0], offsetFaces: faces };
                } break;
                case "ImprintRo": {
                    const offset = $("#offsetImp").val();
                    if (isNaN(offset) || 0 >= Number(offset)) {
                        $("#offsetImp").select();
                        return;
                    }

                    const psEntities = this._nodeSelectOp.getPsEntities(0);
                    if (0 == psEntities.length) {
                        return;
                    }

                    const nodes = this._nodeSelectOp.getNodes(0);
                    const target = this.getEdgeInfo(nodes[0]);
                    let entityTag = target.bodyTag;
                    if (null != target.instanceTag) {
                        entityTag = target.instanceTag;
                    }
                    deleteNodes = [entityTag];
    
                    params = { offset: offset, entities: psEntities };
                } break;
                case "ImprintFace": {
                    const psEntities0 = this._nodeSelectOp.getPsEntities(0);
                    if (0 == psEntities0.length) {
                        return;
                    }

                    const psEntities1 = this._nodeSelectOp.getPsEntities(1);
                    if (0 == psEntities1.length) {
                        return;
                    }

                    const nodes = this._nodeSelectOp.getNodes(1);
                    const target = this.getFaceInfo(nodes[0]);
                    let entityTag = target.bodyTag;
                    if (null != target.instanceTag) {
                        entityTag = target.instanceTag;
                    }
                    deleteNodes = [entityTag];
    
                    params = { target: psEntities1[0], entities: psEntities0 };
                } break;
                case "CopyFace": {
                    const psEntities = this._nodeSelectOp.getPsEntities(0);
                    if (0 == psEntities.length) {
                        return;
                    }

                    const nodes = this._nodeSelectOp.getNodes(0);
                    const target = this.getFaceInfo(nodes[0]);
                    let entityTag = target.bodyTag;
                    if (null != target.instanceTag) {
                        entityTag = target.instanceTag;
                    }

                    params = { entities: psEntities };
                } break;
                case "DeleteFace": {
                    const psEntities = this._nodeSelectOp.getPsEntities(0);
                    if (0 == psEntities.length) {
                        return;
                    }

                    const nodes = this._nodeSelectOp.getNodes(0);
                    const target = this.getFaceInfo(nodes[0]);
                    let entityTag = target.bodyTag;
                    if (null != target.instanceTag) {
                        entityTag = target.instanceTag;
                    }
                    deleteNodes = [entityTag];

                    params = { entities: psEntities };
                } break;
                case "FR_Holes": {
                    let psEntityArr = new Array(0);

                    $("#smallHoles option").each((key, value) => {
                        psEntityArr.push($(value).attr('value'));
                    });

                    if (psEntityArr.length) {
                        let str = "DeleteFace";
                        for (let i = 0; i < psEntityArr.length; i++) {
                            psEntityArr[i] = Number(psEntityArr[i]);
                        }
                        params = { entities: psEntityArr };
        
                        const nodeId = this.getBodyNodeId(psEntityArr[0]);
                        if (nodeId) {
                            this.invokeModelerCreateEdit(str, "edit", [nodeId], params);
                        }
                    }
                } break;
                case "FR_Concaves": {
                    let psEntityArr = new Array(0);

                    $("#concaves option").each((key, value) => {
                        psEntityArr.push($(value).attr('value'));
                    });

                    if (psEntityArr.length) {
                        const size = $("#concaveChamfSize").val();
                        let str = "Blend";
                        for (let i = 0; i < psEntityArr.length; i++) {
                            psEntityArr[i] = Number(psEntityArr[i]);
                        }
                        params = { type: "C", size: size, entities: psEntityArr };

                        const nodeId = this.getBodyNodeId(psEntityArr[0]);
                        if (nodeId) {
                            this.invokeModelerCreateEdit(str, "edit", [nodeId], params);
                        }
                    }
                } break;
                case "SetupAnimation": {
                    const frame = Number($("#numberOfFrames").val());
                    const type = Number($('input:radio[name="animationType"]:checked').val());
                    const scale = Number($("#scaleFactor").val()); 
                    if (this._ceeViewer.setupAnimation(frame, type, scale)) {
                        $(".animationCmd").prop("disabled", false).css("background-color", "gainsboro");
                    }
                } break;
                default:
                    break;
            }

            if ('create' == type || 'edit' == type) {
                this.invokeModelerCreateEdit(str, type, deleteNodes, params);
            }
            
            this.resetCommand();
            this.resetOperator();
            this._viewer.model.resetModelHighlight();
        });
        
        $(".cancelBtn").on("click", () => {
            this.resetCommand();
            this.resetOperator();
            this._viewer.model.resetModelHighlight();
        });
        
        $(".entityOneClickCmd").on("click", (e) => {
            let isOn = $(e.currentTarget).data("on");
            this.resetCommand();
            this.resetOperator();

            if(!isOn) {
                $(e.currentTarget).data("on", true).css("background-color", "khaki");

                let command = $(e.currentTarget).data("command");
                $("#" + command + "Dlg").show();

                switch (command) {
                    case "DeleteBody": {
                        $("#info1").show().html("Select a body to delete.");
                        this._entityOneClickOp.setCommand(command, new Communicator.PickConfig(Communicator.SelectionMask.Face));
                    } break;
                    case "MassProps": {
                        $("#info1").show().html("Mass Properties: Select a body to inquiry.");
                        this._entityOneClickOp.setCommand(command, new Communicator.PickConfig(Communicator.SelectionMask.Face));
                    } break;
                    default:
                        break;
                }
                const id = this._viewer.operatorManager.indexOf(Communicator.OperatorId.Select);
                this._viewer.operatorManager.set(this._entityOneClickOpHandle, id);
            }
        });

        // Animation command
        $(".animationCmd").on("click", (e) => {
            let command = $(e.currentTarget).data("command");
            switch(command) {
                case "AnimationStart": {
                    this._ceeViewer.AnimationStart();
                }
                break;
                case "AnimationBack": {
                    this._ceeViewer.AnimationBack();
                }
                break;
                case "AnimationPlay": {
                    if ("css/images/animation_play_fwd.svg" == $("#AnimationPlay").attr("src")) {
                        $("#AnimationPlay").attr("src", "css/images/animation_pause.svg");
                    }
                    else {
                        $("#AnimationPlay").attr("src", "css/images/animation_play_fwd.svg");
                    }
                    
                    this._ceeViewer.AnimationPlay();
                }
                break;
                case "AnimationForward": {
                    this._ceeViewer.AnimationForward();
                }
                break;
                case "AnimationEnd": {
                    this._ceeViewer.AnimationEnd();
                }
                break;
                default:
                    break;
            }
        });

        // Normal button common
        $(".normalBtn")
        .on("mousedown", (e) => {
            $(e.currentTarget).css("background-color", "khaki");
        })
        .on("mouseup", (e) => {
            $(e.currentTarget).css("background-color", "gainsboro");
        });

        // Toggle button common
        $(".toggleBtn").on("click", (e) => {
            let isOn = $(e.currentTarget).data("on");
            if(isOn) {
                $(e.currentTarget).data("on", false).css("background-color", "gainsboro");
            } else {
                $(e.currentTarget).data("on", true).css("background-color", "khaki");
            }
        });

        $(".selList").on({
            "focus": (e) => {
                const listId = $(e.currentTarget).data("listid");
                if (0 <= listId) {
                    this._nodeSelectOp.setCurrentList(listId);
                }
                $(e.currentTarget).css("background-color", "#FF8080");
            },
            "focusout": (e) => {
                const unfocus = $(e.currentTarget).data("unfocus");
                if (unfocus) {
                    $(e.currentTarget).css("background-color", "#FFFFFF");
                }
            }
        });

        $("#scalerResults").change((e) => {
            const id = Number($(e.currentTarget).val());
            this._ceeViewer.showScalarResult(id);
        });

        $("#materialSelect").change((e) => {
            const id = Number($(e.currentTarget).val());
            const material = this._materials[id];
            $("#density").val(material.density);
            $("#young").val(material.young);
            $("#poisson").val(material.poisson);
        });

        // Hollow
        $("#noPierceFace").change((e) => {
            if($(e.currentTarget).prop('checked')) {
                $("#info1").show().html("Hollow: Select a body to hollow.");
                $("#selListHollow").hide();
                $("#okCancel").hide();
                this._entityOneClickOp.setCommand("Hollow", new Communicator.PickConfig(Communicator.SelectionMask.Face));
                const id = this._viewer.operatorManager.indexOf(this._nodeSelectOpHandle);
                this._viewer.operatorManager.set(this._entityOneClickOpHandle, id);
                this._viewer.model.resetModelHighlight();
            }
            else {
                $("#info1").show().html("Hollow: Select face(s) to pierce.");
                $("#selListHollow").show();
                $("#okCancel").show();
                const id = this._viewer.operatorManager.indexOf(this._entityOneClickOpHandle);
                this._viewer.operatorManager.set(this._nodeSelectOpHandle, id);
            }
        });

        // Offset
        $("#wholeOffset").change((e) => {
            if($(e.currentTarget).prop('checked')) {
                $("#info1").show().html("Offset: Select a body to offset.");
                $("#selListOffset").hide();
                $("#okCancel").hide();
                this._entityOneClickOp.setCommand("Offset", new Communicator.PickConfig(Communicator.SelectionMask.Face));
                const id = this._viewer.operatorManager.indexOf(this._nodeSelectOpHandle);
                this._viewer.operatorManager.set(this._entityOneClickOpHandle, id);
                this._viewer.model.resetModelHighlight();
            }
            else {
                $("#info1").show().html("Offset: Select face(s) to offset.");
                $("#selListOffset").show();
                $("#okCancel").show();
                const id = this._viewer.operatorManager.indexOf(this._entityOneClickOpHandle);
                this._viewer.operatorManager.set(this._nodeSelectOpHandle, id);
            }
        });

        $("#DownloadDlg").dialog({
            autoOpen: false,
            height: 300,
            width: 260,
            modal: true,
            title: "Download file format",
            closeOnEscape: true,
            position: {my: "center", at: "center", of: window},
            buttons: {
                'OK': () => {
                    const fileFormat = $('input:radio[name="fileFormat"]:checked').val();

                    $("#loadingImage").show();
                    const params = { format: fileFormat };
                    this._psServerCaller.CallServerPost("DownloadCAD", params).then(() => {
                        let ext = "";
                        switch (fileFormat) {
                            case "X": ext = ".x_t"; break;
                            case "S": ext = ".stp"; break;
                            case "P": ext = ".prc"; break;
                            default: break;
                        }

                        const serverName = this._sessionId + ext;
                        const downloadName = "model" + ext;

                        this._fileDownload(serverName, downloadName);
                        $("#loadingImage").hide();
                    }).catch((error) => {
                        $("#loadingImage").hide();
                        alert("Download failed");
                    });
                   
                    
                    $("#DownloadDlg").dialog('close');
                },
                'Cancel': () => {
                    $("#DownloadDlg").dialog('close');
                }
            }
        });
    }

    _initHistorys() {
        this._deleteAllBodies();

        this._historys.length = 0;

        let history = {
            type: 'new'
        }
        this._historys.push(history);
        this._currentHistory = 0;
    }

    _updateToolbar () {
        if (this._bodyMemberArr.length < 1) {
            $('[data-type="edit"],[data-type="inquiry"]').prop("disabled", true).css("background-color", "darkgrey");
            $(".entityOneClickCmd").prop("disabled", true).css("background-color", "darkgrey");
            $(".download").prop("disabled", true).css("background-color", "darkgrey");
        }
        else {
            $('[data-type="edit"],[data-type="inquiry"]').prop("disabled", false).css("background-color", "gainsboro");
            $(".entityOneClickCmd").prop("disabled", false).css("background-color", "gainsboro");
            $(".download").prop("disabled", false).css("background-color", "gainsboro");
        }

        if (this._bodyMemberArr.length < 2) {
            $(".boolean").prop("disabled", true).css("background-color", "darkgrey");
        }
        else {
            $(".boolean").prop("disabled", false).css("background-color", "gainsboro");
        }

        $(".animationCmd").prop("disabled", true).css("background-color", "darkgrey");
        
        // Undo
        if (0 == this._currentHistory) {
            $('[data-command="Undo"]').prop("disabled", true).css("background-color", "darkgrey");
        }
        else {
            $('[data-command="Undo"]').prop("disabled", false).css("background-color", "gainsboro");
        }

        // Redo
        if (this._historys.length == this._currentHistory + 1) {
            $('[data-command="Redo"]').prop("disabled", true).css("background-color", "darkgrey");
        }
        else {
            $('[data-command="Redo"]').prop("disabled", false).css("background-color", "gainsboro");
        }
    };

    _createTreeData(data, treeData, parentId, level, bodyInfoArr) {
        level++;

        const children = data.children;

        let icon = 'jstree-folder';
        if (undefined == children) {
            icon = 'jstree-file';
        }
        
        let opened = true;
        if (2 <= level) {
            opened = false;
        }

        const nodeData = {
            id: String(data.tag),
            text: data.text,
            icon: icon,
            state: {
                opened: opened,
                checked: true
            },
            children: []
        }
        treeData.push(nodeData);

        const entityTag = Number(data.tag);
        let matrix = new Communicator.Matrix();
        if (undefined != data.transf) {
            if (data.transf.length) {
                matrix = Communicator.Matrix.createFromArray(data.transf);
            }
        }

        if (undefined == children) {
            bodyInfoArr.push( {
                body: entityTag,
                parent: parentId,
                matrix: matrix
            })
            return;
        }

        const nodeId = this._viewer.model.createNode(parentId, String(entityTag), entityTag);
        this._viewer.model.setNodeMatrix(nodeId, matrix, false);

        let childData = treeData[treeData.length - 1]["children"];
        for (const child of children) {
            const childNode = this._createTreeData(child, childData, entityTag, level, bodyInfoArr);
        }
        
        level--;
    }

    invokeFileImport(f) {
        $("#loadingImage").show();
        this._psServerCaller.CallServerPostFile(f).then((res) => {
            if (Object.keys(res).length) {
                // CAD model
                this._initHistorys();

                let history = {
                    type: 'create',
                }
    
                this._historys.push(history);
                this._currentHistory++;
                
                let treeData = [];
                let bodyInfoArr = [];
                const root = this._viewer.model.getAbsoluteRootNode();
                this._createTreeData(res.data, treeData, root, 0, bodyInfoArr);
                this._modelTree.createFromJson(treeData);

                let promiseArr = new Array(0);
                for (let bodyInfo of bodyInfoArr) {
                    const params = { body: bodyInfo.body };
                    promiseArr.push(this._psServerCaller.CallServerPost('RequestBody', params, "float"));
                }

                Promise.all(promiseArr).then((arrArr) => {
                    promiseArr.length = 0;
                    for (let i = 0; i < arrArr.length; i++) {
                        const arr = arrArr[i];
                        let instanceTag = null;
                        if (arr[0] != bodyInfoArr[i].body) {
                            instanceTag = bodyInfoArr[i].body;
                        }
                        promiseArr.push(this._bodyMeshCreator.addMesh(arr, bodyInfoArr[i].matrix, instanceTag, bodyInfoArr[i].parent));
                    }
                    Promise.all(promiseArr).then((bodies) => {
                        let bodyTagArr = new Array(0);
                        let instanceTagArr = new Array(0);
                        let meshInstanceDataArr = new Array(0);
                        let facesArr = new Array(0);
                        let edgesArr = new Array(0);
                        let parentNodeArr = new Array(0);
                        let faceCnt = 0;
                        let edgeCnt = 0;
                        let triCnt = 0;
                        for (let i = 0; i < bodies.length; i++) {
                            const body = bodies[i];
                            this._updateBodyMemberArr(body);

                            bodyTagArr.push(body.bodyTag);
                            instanceTagArr.push(body.instanceTag);
                            meshInstanceDataArr.push(body.meshInstanceData);
                            facesArr.push(body.faces);
                            edgesArr.push(body.edges);
                            parentNodeArr.push(bodyInfoArr[i].parent);
                            faceCnt += body.faces.length;
                            edgeCnt += body.edges.length;
                            triCnt += body.triangleCnt;
                        }
                        history["bodyTagArr"] = bodyTagArr;
                        history["instanceTagArr"] = instanceTagArr;
                        history["meshInstanceDataArr"] = meshInstanceDataArr;
                        history["facesArr"] = facesArr;
                        history["edgesArr"] = edgesArr;
                        history["parentNodeArr"] = parentNodeArr;

                        this._viewer.view.setViewOrientation(Communicator.ViewOrientation.Iso);
                        this._viewer.fitWorld();
                        this._updateToolbar();
                        $("#dataInfo").html(
                            "No. of faces: " + String(faceCnt).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,') + 
                            "<br>No. of edges: " + String(edgeCnt).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,') + 
                            "<br>No. of triangles: " + String(triCnt).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,'));
                        $("#loadingImage").hide();
                    });
                });
            }
        }).catch((error) => {
            $("#loadingImage").hide();
            alert("File uploading failed");
        });
    }

    _updateBodyMemberArr(newBody) {
        let flg = false;
        for (let i = 0; i < this._bodyMemberArr.length; i++) {
            const body = this._bodyMemberArr[i];
            if (newBody.instanceTag == body.instacdId && newBody.bodyTag == body.bodyTag) {
                this._bodyMemberArr[i] = newBody;
                flg = true;
                break;
            }
        }
        if (!flg) {
            this._bodyMemberArr.push(newBody);
        }
    }

    invokeModelerUndo () {
        $("#loadingImage").show();
        this._faceColorMap = {};

        this._psServerCaller.CallServerPost("Undo").then(() => {
            if (1 < this._currentHistory)
            {
                let promiseArr = new Array(0);
                let currentHistory = this._historys[this._currentHistory];
                let deleteBodies = new Array(0);

                if ("create" == currentHistory.type || "edit" == currentHistory.type) {
                    // Delete bodies which are created in the current history
                    for (let i = 0; i < currentHistory.bodyTagArr.length; i++) {
                        let psEntityId = currentHistory.bodyTagArr[i];
                        if (null != currentHistory.instanceTagArr[i]) 
                            psEntityId = currentHistory.instanceTagArr[i];
                        promiseArr.push(this._deleteBody(psEntityId));
                        deleteBodies.push(psEntityId);
                    }
                }
                else if ('transform' == currentHistory.type) {
                    for (let i = 0; i < currentHistory.nodes.length; i++) {
                        promiseArr.push(this._viewer.model.setNodeMatrix(currentHistory.nodes[i], currentHistory.initialMatrices[i]));
                    }
                }

                Promise.all(promiseArr).then(() => {
                    if ("edit" == currentHistory.type || "delete" == currentHistory.type) {
                        promiseArr.length = 0;

                        if (undefined != currentHistory.deleteBodies) {
                            deleteBodies = deleteBodies.concat(currentHistory.deleteBodies);
                        }

                        // Search and restore previous body
                        for (const deleteBody of deleteBodies) {
                            for (let i = this._currentHistory; i > 0; i--) {
                                let flg = false;
                                const history = this._historys[i];
                                if (undefined != history.bodyTagArr) {
                                    for (let j = 0; j < history.bodyTagArr.length; j++) {
                                        const bodyTag = history.bodyTagArr[j];
                                        const instanceTag = history.instanceTagArr[j];
                                        if (deleteBody == bodyTag || deleteBody == instanceTag) {
                                            const meshInstanceData = history.meshInstanceDataArr[j];
                                            const parentNode = history.parentNodeArr[j];
                                            let entityTag = bodyTag;
                                            if (null != instanceTag) {
                                                entityTag = instanceTag;
                                            }

                                            if (null != meshInstanceData) {
                                                promiseArr.push(this._bodyMeshCreator.createMeshInstance(meshInstanceData, entityTag, parentNode));
                                            }
                                            else {
                                                this._viewer.model.createNode(parentNode, String(entityTag), entityTag);
                                            }
                                            
                                            const unBody = {
                                                instanceTag: instanceTag, 
                                                bodyTag: bodyTag,
                                                faces: history.facesArr[j],
                                                edges: history.edgesArr[j]
                                            }
                                            this._updateBodyMemberArr(unBody);
                                            flg = true;
                                            break;
                                        }
                                    }
                                    if (flg) break;
                                }
                            }
                        }
                        Promise.all(promiseArr).then((arrAr) => {
                            this._modelTree.undoTree();
                            this._updateToolbar();
                            $("#loadingImage").hide();
                        });
                    }
                    else {
                        this._modelTree.undoTree();
                        this._updateToolbar();
                        $("#loadingImage").hide();
                    }
                });
            }
            else {
                this._deleteAllBodies().then(() => {
                    this._modelTree.undoTree();
                    this._updateToolbar();
                    $("#loadingImage").hide();
                });
            }

            this._currentHistory--;

        }).catch((error) => {
            alert("Undo failed");
        });
    };

    invokeModelerRedo () {
        $("#loadingImage").show();

        this._faceColorMap = {};
        this._psServerCaller.CallServerPost("Redo").then(() => {
            this._currentHistory++;
            let history = this._historys[this._currentHistory];

            if ('create' == history.type || 'edit' == history.type) {
                let promiseArr = new Array(0);

                // If edit, delete previous bodies of this history
                if ('edit' == history.type) {
                    for (let i = 0; i < history.bodyTagArr.length; i++) {
                        let entityTag = history.bodyTagArr[i];
                        if (history.instanceTagArr[i]) {
                            entityTag = history.instanceTagArr[i];
                        }
                        promiseArr.push(this._deleteBody(entityTag));
                    }
                }

                Promise.all(promiseArr).then(() => {
                    promiseArr.length = 0;
                    // Redo bodies of this history
                    if (undefined != history.meshInstanceDataArr) {
                        for (let i = 0; i < history.meshInstanceDataArr.length; i++) {
                            let entityTag = history.bodyTagArr[i];
                            if (history.instanceTagArr[i]) {
                                entityTag = history.instanceTagArr[i];
                            }
                            const meshInstanceData = history.meshInstanceDataArr[i];
                            const parentNode = history.parentNodeArr[i];

                            if (null != meshInstanceData) {
                                promiseArr.push(this._bodyMeshCreator.createMeshInstance(meshInstanceData, entityTag, parentNode));
                            }
                            else {
                                this._viewer.model.createNode(parentNode, String(entityTag), entityTag);
                            }

                            // Update body DB
                            const reBody = {
                                instanceTag: history.instanceTagArr[i], 
                                bodyTag: history.bodyTagArr[i],
                                faces: history.facesArr[i],
                                edges: history.edgesArr[i]
                            }
                            this._updateBodyMemberArr(reBody);
                        }
                        Promise.all(promiseArr).then((nodeIdArr) => {
                            promiseArr.length = 0;
                            if (undefined != history.deleteBodies) {
                                for (let psBodyId of history.deleteBodies) {
                                    promiseArr.push(this._deleteBody(psBodyId));
                                }
                            }
                            Promise.all(promiseArr).then(() => {
                                this._modelTree.redoTree();
                                this._updateToolbar();
                                $("#loadingImage").hide();
                            });
                        });
                    }
                });
            }
            else if ('delete' == history.type) {
                let promiseArr = new Array(0);
                for (let psBodyId of history.deleteBodies) {
                    promiseArr.push(this._deleteBody(psBodyId));
                }
                Promise.all(promiseArr).then(() => {
                    this._updateToolbar();
                    $("#loadingImage").hide();
                });
            }
            else if ('transform' == history.type) {
                let promiseArr = new Array(0);
                for (let i = 0; i < history.nodes.length; i++) {
                    promiseArr.push(this._viewer.model.setNodeMatrix(history.nodes[i], history.newMatrices[i]));
                }
                Promise.all(promiseArr).then(() => {
                    $("#loadingImage").hide();
                });;
            }
        }).catch((error) => {
            alert("Undo/Redo failed");
        });
    };

    invokeModelerCreateEdit (command, type, deletePsBodyIDs, params) {
        return new Promise((resolve, reject) => {
            $("#loadingImage").show();
            this._faceColorMap = {};
            this._psServerCaller.CallServerPost(command, params, "float").then((floatarr) => {
                if (0 < floatarr.length) {
                    let arr = Array.from(floatarr);
                    let entityTag = arr[0];

                    let instanceTag = null;
                    if ('create' == type) {
                        instanceTag = arr.shift();
                        entityTag = instanceTag;
                    }
                    
                    let history = {
                        type: type,
                        bodyTagArr: [arr[0]],
                        instanceTagArr: [instanceTag]
                    }

                    let parentNode = this._viewer.model.getAbsoluteRootNode();

                    if ('edit' == type && undefined != deletePsBodyIDs) {
                        if (entityTag != deletePsBodyIDs[0]){
                            entityTag = deletePsBodyIDs[0];
                            instanceTag = deletePsBodyIDs[0];
                            history['instanceTagArr'] = [deletePsBodyIDs[0]];
                        }

                        let deleteBodies = new Array(0);
                        for (let i = 1; i < deletePsBodyIDs.length; i++) {
                            deleteBodies.push(deletePsBodyIDs[i]);
                        }
                        if (deleteBodies.length) {
                            history["deleteBodies"] = deleteBodies;
                        }

                        // Get parent node
                        parentNode = this._viewer.model.getNodeParent(entityTag);
                    }
                    history["parentNodeArr"] = [parentNode];

                    // Delete branch histories and meshes
                    let meshIdsArr = new Array(0);
                    while (this._currentHistory + 1 < this._historys.length) {
                        const history = this._historys.pop();
                        if (undefined != history.meshInstanceDataArr) {
                            for (let meshInstanceData of history.meshInstanceDataArr) {
                                meshIdsArr.push(meshInstanceData.getMeshId());
                            }
                        }
                    }
                    if (meshIdsArr.length) {
                        this._viewer.model.deleteMeshes(meshIdsArr);
                    }

                    this._historys.push(history);
                    this._currentHistory++;

                    let promiseArr = new Array(0);
                    // Delete previous body
                    if (undefined != deletePsBodyIDs) {
                        for (let id of deletePsBodyIDs) {
                            promiseArr.push(this._deleteBody(id));
                        }
                    }
                    Promise.all(promiseArr).then(() => {
                        this._bodyMeshCreator.addMesh(arr, null, instanceTag, parentNode).then((body) => {
                            // Update body DB
                            this._updateBodyMemberArr(body);

                            // Update history
                            history["meshInstanceDataArr"] = [body.meshInstanceData];
                            history["facesArr"] = [body.faces];
                            history["edgesArr"] = [body.edges];

                            // Update tree
                            this._modelTree.addNode(body.meshInstanceData.getInstanceName(), entityTag, parentNode, true);

                            if (this._bodyMemberArr.length == 1 && 'edit' != type) {
                                this._viewer.fitWorld();
                            }
                            
                            $("#dataInfo").html(
                                "No. of faces: " + String(body.faces.length).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,') + 
                                "<br>No. of edges: " + String(body.edges.length).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,') + 
                                "<br>No. of triangles: " + String(body.triangleCnt).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,'));
                            $("#loadingImage").hide();
                            this._updateToolbar();
                        });
                        resolve();
                    });
                }
                else {
                    $("#loadingImage").hide();
                    reject();
                }
            }).catch((error) => {
                $("#loadingImage").hide();
                alert("Command failed");
            });
        })
    }

    invokeModelerDeleteBody (command, type, deleteentityTag, params) {
        this._faceColorMap = {};
        this._psServerCaller.CallServerPost(command, params).then(() => {
            let history = {
                type: type,
                deleteBodies: [deleteentityTag]
            }

            if (this._currentHistory + 1 < this._historys.length) {
                this._historys.splice(this._currentHistory + 1)
            }

            this._historys.push(history);
            this._currentHistory++;

            this._deleteBody(deleteentityTag, true).then(() => {
                this._updateToolbar();
            });
        }).catch((error) => {
            $("#loadingImage").hide();
            alert("Delete body failed");
        });
    }

    invokeModelerInquiry (command, params) {
        $("#loadingImage").show();
        this._psServerCaller.CallServerPost(command, params, "float").then((arr) => {
            if (arr.length) {
                if (0 == command.indexOf("MassProps")) {
                    $("#dataInfo").html(
                        'Volume: ' + String(arr[0].toFixed(1)).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,') + ' mm^3<br>' + 
                        'Surface area: ' + String(arr[1].toFixed(1)).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,') + ' mm^2<br>' + 
                        'Center of gravity:<br>' + 
                        '&nbsp;X: ' + String(arr[2].toFixed(1)).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,') + ' mm<br>' + 
                        '&nbsp;Y: ' + String(arr[3].toFixed(1)).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,') + ' mm<br>' + 
                        '&nbsp;Z: ' + String(arr[4].toFixed(1)).replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,') + ' mm');
                }
                else if (0 == command.indexOf("FR_Holes")) {
                    this._viewer.model.resetModelHighlight().then(() => {
                        for (let faceTag of arr) {
                            const ids = this.getFaceNodeId(faceTag);
                            if (null != ids) {
                                this._viewer.model.setNodeFaceHighlighted(ids.nodeId, ids.faceId, true);
                            }

                            $("#smallHoles").append($("<option>").val(faceTag).text("face<" + faceTag + ">"));
                        }
                    });
                }
                else if (0 == command.indexOf("FR_Concaves")) {
                    this._viewer.model.resetModelHighlight().then(() => {
                        for (let edgeTag of arr) {
                            const ids = this.getEdgeNodeId(edgeTag);
                            if (null != ids) {
                                this._viewer.model.setNodeLineHighlighted(ids.nodeId, ids.edgeId, true);
                            }

                            $("#concaves").append($("<option>").val(edgeTag).text("Edge<" + edgeTag + ">"));
                        }
                    });
                }
            }
            $("#loadingImage").hide();
        }).catch((error) => {
            $("#loadingImage").hide();
            alert("Command failed");
        });
    };

    invokeHollow(targetEntity) {
        let thickness = $("#hollowThickness").val();
        if (isNaN(thickness) || 0 >= Number(thickness)) {
            $("#hollowThickness").select();
            return;
        }
        thickness = 0 - Number(thickness);

        const outside = $("#hollowOutside").prop("checked");
        if (outside) {
            thickness *= -1;
        }

        let deleteNodes = [targetEntity];

        const params = { thickness: thickness, targetEntity: [targetEntity], pierceFaces: [] };

        this.invokeModelerCreateEdit("Hollow", "edit", deleteNodes, params);

        this.resetCommand();
        this.resetOperator();
        this._viewer.model.resetModelHighlight();
    }

    invokeOffset(targetEntity) {
        let value = $("#offsetValue").val();
        if (isNaN(value) || 0 >= Number(value)) {
            $("#offsetValue").select();
            return;
        }
        value = Number(value);

        const inside = $("#offsetInside").prop("checked");
        if (inside) {
            value *= -1;
        }

        let deleteNodes = [targetEntity];

        const params = { value: value, targetEntity: [targetEntity], offsetFaces: [] };

        this.invokeModelerCreateEdit("Offset", "edit", deleteNodes, params);

        this.resetCommand();
        this.resetOperator();
        this._viewer.model.resetModelHighlight();
    }

    getBodyId (nodeId) {
        const parent = this._viewer.model.getNodeParent(nodeId);
        for (let body of this._bodyMemberArr) {
            if (parent == body.bodyTag || parent == body.instanceTag) {
                return body;
            }
        }
        return null;
    };

    getEdgeInfo (nodeId, edgeId) {
        const parentNode = this._viewer.model.getNodeParent(nodeId);
        for (let body of this._bodyMemberArr) {
            if (parentNode == body.bodyTag || parentNode == body.instanceTag) {
                if (undefined != edgeId) {
                    return {
                        instanceTag: body.instanceTag,
                        bodyTag: body.bodyTag,
                        edgeTag: body.edges[edgeId]
                    };
                }
                else {
                    return {
                        instanceTag: body.instanceTag,
                        bodyTag: body.bodyTag,
                    };
                }
            }
        }
        return;
    };

    getFaceInfo (nodeId, faceId) {
        const parentNode = this._viewer.model.getNodeParent(nodeId);
        for (let body of this._bodyMemberArr) {
            if (parentNode == body.bodyTag || parentNode == body.instanceTag) {
                if (undefined != faceId) {
                    return {
                        instanceTag: body.instanceTag,
                        bodyTag: body.bodyTag,
                        faceTag: body.faces[faceId]
                    };
                }
                else {
                    return {
                        instanceTag: body.instanceTag,
                        bodyTag: body.bodyTag,
                    };
                }
            }
        }
        return;
    };

    getFaceNodeId (faceTag) {
        for (let body of this._bodyMemberArr) {
            const id = body.faces.indexOf(faceTag);
            if (-1 != id) {
                let parent = body.bodyTag;
                if (undefined != body.instanceTag) {
                    parent = body.instanceTag;
                }
                const childlen = this._viewer.model.getNodeChildren(parent);
                if (1 == childlen.length) {
                    return {
                        faceId: id,
                        nodeId: childlen[0]
                    };
                }
            }
        }
        return null;
    }

    getEdgeNodeId (edgeTag) {
        for (let body of this._bodyMemberArr) {
            const id = body.edges.indexOf(edgeTag);
            if (-1 != id) {
                let parent = body.bodyTag;
                if (undefined != body.instanceTag) {
                    parent = body.instanceTag;
                }
                const childlen = this._viewer.model.getNodeChildren(parent);
                if (1 == childlen.length) {
                    return {
                        edgeId: id,
                        nodeId: childlen[0]
                    };
                }
            }
        }
        return null;
    }

    getBodyNodeId (entityTag) {
        for (let body of this._bodyMemberArr) {
            {
                const id = body.faces.indexOf(entityTag);
                if (-1 != id) {
                    return (null != body.instanceTag? body.instanceTag: body.bodyTag);
                }
            }
            {
                const id = body.edges.indexOf(entityTag);
                if (-1 != id) {
                    return (null != body.instanceTag? body.instanceTag: body.bodyTag);
                }
            }
        }
    }

    _deleteTempBodies() {
        if (null != this._tempNode) {
            const childNodes = this._viewer.model.getNodeChildren(this._tempNode);
            if (null != childNodes) {
                // Delete temp mesh instances
                this._viewer.model.deleteMeshInstances(childNodes).then(() => {
                    // Delete temp node
                    this._viewer.model.deleteNode(this._tempNode).then(() => {
                        this._tempNode = null;

                        // Delete mesh prototypes
                        if (this._tempMeshIdsArr.length) {
                            this._viewer.model.deleteMeshes(this._tempMeshIdsArr).then(() => {
                                this._tempMeshIdsArr.length = 0;
                            });
                        }

                        // Reset part transparency
                        let nodeArr = new Array(0);
                        for (let body of this._bodyMemberArr) {
                            let nodeId = body.bodyTag;
                            if (null != body.instanceTag) {
                                nodeId = body.instanceTag;
                            }
                            nodeArr.push(nodeId);
                        }
                        this._viewer.model.setNodesOpacity(nodeArr, 1.0);
                    });
                });
            }
        }

        if (null != this._closestMarkup) {
            this._viewer.markupManager.unregisterMarkup(this._closestMarkup);
            this._closestMarkup = null;
        }
    }

    resetCommand () {
        $("#info1").html("");
        $(".cmdDlg").hide();
        $(".inpuSet").hide();
        $("#dataInfo").html("");
        $("#OkBtn").val("OK");
        $(".entityOneClickCmd,.dlgOkCmd").data("on", false).css("background-color", "gainsboro");

        // Hollow
        $("#selListHollow").show();
        $("#noPierceFace").prop('checked', false);

        // Offset
        $("#selListOffset").show();
        $("#wholeOffset").prop('checked', false);

        this._deleteTempBodies();
    }

    resetOperator () {
        let OM = this._viewer.operatorManager;

        if (-1 != OM.indexOf(this._handleOpOpHandle)) return;

        OM.clear();
        OM.push(Communicator.OperatorId.Navigate);
        OM.push(Communicator.OperatorId.Select);
        OM.push(Communicator.OperatorId.Cutting);
        OM.push(Communicator.OperatorId.Handle);
        OM.push(Communicator.OperatorId.NavCube);
    };

    _deleteBody (entityTag, mark = false) {
        return new Promise((resolve, reject) => {
            this._viewer.model.resetModelHighlight().then(() => {
                for (let i = 0; i < this._bodyMemberArr.length; i++) {
                    const body = this._bodyMemberArr[i];
                    if (entityTag == body.bodyTag || entityTag == body.instanceTag) {
                        this._bodyMemberArr.splice(i, 1);
                        // Update model tree
                        this._modelTree.deleteNode(entityTag, mark);
                    }
                }

                const childNodes = this._viewer.model.getNodeChildren(entityTag);
                if (null != childNodes) {
                    this._viewer.model.deleteMeshInstances(childNodes).then(() => {
                        this._viewer.model.deleteNode(entityTag).then(() => {
                            resolve();
                        });
                    });
                }
                else {
                    reject();
                }
            });
        });
    }

    _deleteAllBodies () {
        return new Promise((resolve, reject) => {
            if (0 == this._bodyMemberArr.length) {
                return;
            }

            let promiseArr = new Array(0);
            for (const body of this._bodyMemberArr) {
                let entityTag = body.bodyTag;
                if (null != body.instanceTag) {
                    entityTag = body.instanceTag;
                }
                promiseArr.push(this._deleteBody(entityTag));
            }

            Promise.all(promiseArr).then(() => {
                resolve();
            }).catch((e) => {
                reject(e);
            });
        });
    }

    showCuttingPlaneDialog() {
        var model = this._ceeViewer.getModel();
        if (!this.m_cuttingPlaneDialog) {
            this.m_cuttingPlaneDialog = new CuttingPlaneDialog(model);
        }

        this.m_particleTraceDialog = new ParticleTraceDialog(this, model);
        if (this.m_particleTraceDialog)
            this.m_particleTraceDialog.clearPickingSettings();
        this.m_cuttingPlaneDialog.showDialog();
    }

    _backToCAD() {
        // Get camera of Ceetron view
        const ceeCam = this._ceeViewer.getCamera();
        const ceePos = ceeCam.getPosition();
        const ceeDir = ceeCam.getDirection();
        const ceeUp = ceeCam.getUp();

        const newPos = new Communicator.Point3(ceePos.x, ceePos.y, ceePos.z).scale(1000);
        // Compute new target
        let camera = this._viewer.view.getCamera();
        const pos = camera.getPosition();
        let tar = camera.getTarget();
        const len = Communicator.Point3.subtract(tar, pos).length();
        const dir = new Communicator.Point3(ceeDir.x, ceeDir.y, ceeDir.z).normalize();
        tar = Communicator.Point3.add(newPos, dir.scale(len));

        camera.setPosition(newPos);
        camera.setTarget(tar);
        camera.setUp(new Communicator.Point3(ceeUp.x, ceeUp.y, ceeUp.z));

        const projection = camera.getProjection();
        if (Communicator.Projection.Orthographic == projection) {
            const ceeHeight = ceeCam.frontPlaneFrustumHeight;
            camera.setHeight(ceeHeight * 1000);
        }

        this._viewer.view.setCamera(camera);

        $(".CAD_contents").show();
    }

    _fileDownload(from, to) {
        let oReq = new XMLHttpRequest(),
            a = document.createElement('a'), file;
        let versioningNum = new Date().getTime()
        oReq.open('GET', from + "?" + versioningNum, true);
        oReq.responseType = 'blob';
        oReq.onload = (oEvent) => {
            var blob = oReq.response;
            if (window.navigator.msSaveBlob) {
                // IE or Edge
                window.navigator.msSaveBlob(blob, filename);
            }
            else {
                // Other
                var objectURL = window.URL.createObjectURL(blob);
                var link = document.createElement("a");
                document.body.appendChild(link);
                link.href = objectURL;
                link.download = to;
                link.click();
                document.body.removeChild(link);
            }
            // Delete download source file in server
            this._psServerCaller.CallServerPost("Downloaded");
            $("#loadingImage").hide();
        };
        oReq.send();
    }
}
