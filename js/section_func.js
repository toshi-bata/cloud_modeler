var ClipFunc = function(viewer) {
    this._viewer = viewer;
    this._cuttingSections = new Array(0);
    this._box;

    var _this = this;
    _this._viewer.getModel().getModelBounding(true, false).then(function(box) {
        _this._box = box;
    });
};

ClipFunc.prototype = {
    on: function() {
        var _this = this;
        return new Promise(function (resolve, reject) {
            if (0 == _this._cuttingSections.length) {
                _this._viewer.getModel().getModelBounding(true, false).then(function(box) {
                    _this._box = box;
                    var CM = _this._viewer.cuttingManager;
                    CM.setCappingFaceColor(new Communicator.Color(0, 255, 255));

                    var refGeom = [];

                    // x
                    refGeom.length = 0;
                    refGeom.push(new Communicator.Point3(0, box.min.y, box.min.z));
                    refGeom.push(new Communicator.Point3(0, box.min.y, box.max.z));
                    refGeom.push(new Communicator.Point3(0, box.max.y, box.max.z));
                    refGeom.push(new Communicator.Point3(0, box.max.y, box.min.z));
                    // -x
                    {
                        var section = CM.getCuttingSection(0);
                        section.setColor(new Communicator.Color(255, 0, 0));
                        var plane = section.getPlane(0);
                        if(plane == null) {
                            plane = new Communicator.Plane();
                            plane.normal.set(-1, 0, 0);
                            plane.d = box.min.x;
                            section.addPlane(plane, refGeom);
                        } else {
                            section.clear();
                            section.addPlane(plane, refGeom);
                        }
                        section.activate();
                        _this._cuttingSections.push(section);
                    }
                    // +x
                    {
                        var section = CM.getCuttingSection(1);
                        var plane = section.getPlane(0);
                        if(plane == null) {
                            plane = new Communicator.Plane();
                            plane.normal.set(1, 0, 0);
                            plane.d = - box.max.x;
                            section.addPlane(plane, refGeom);
                        } else {
                            section.clear();
                            section.addPlane(plane, refGeom);
                        }
                        section.activate();
                        _this._cuttingSections.push(section);
                    }
                    
                    resolve();
                });
            }
            else {
                for(var i = 0; i < _this._cuttingSections.length; i++) {
                    var section = _this._cuttingSections[i];
                    section.activate();
                }
            }
        });
    },
    
    off: function() {
        var _this = this;
        
        for(var i = 0; i < _this._cuttingSections.length; i++) {
            var section = _this._cuttingSections[i];
            section.deactivate();
        }
    },
    
    hideCuttingPlane: function(val) {
        var _this = this;
        
        var CM = _this._viewer.cuttingManager;
        for(var i = 0; i < 2; i++) {
            var section = CM.getCuttingSection(i);
            var plane = section.getPlane(0);
            section.clear();
            section.addPlane(plane, null);
            section.activate();
        }
    },
    
    reset: function() {
        var _this = this;
        
        for(var i = 0; i < _this._cuttingSections.length; i++) {
            var section = _this._cuttingSections[i];
            section.clear();
        }        
    },

    getScope: function() {
        var _this = this;
        let scope = {};
        {
            let section = _this._viewer.cuttingManager.getCuttingSection(0);
            let plane = section.getPlane(0);
            if (null != plane) {
                scope.min = plane.d;
            }
            else {
                scope.min = _this._box.min.x;
            }
        }
        {
            let section = _this._viewer.cuttingManager.getCuttingSection(1);
            let plane = section.getPlane(0);
            if (null != plane) {
                scope.max = plane.d * -1;
            }
            else {
                scope.max = _this._box.max.x;
            }
        }

        return scope;
    }

    
};