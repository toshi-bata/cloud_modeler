import * as Communicator from "../hoops-web-viewer.mjs";
// compute angle and rotation axis between two vectors
export function vectorsAngleDeg(point3d1, point3d2) {
    if (point3d1.equalsWithTolerance(point3d2, 1.0E-8)) {
        return {
            angleDeg: 0, 
            axis: undefined
        }
    }

    if (point3d1.equalsWithTolerance(point3d2.copy().negate(), 1.0E-8)) {
        return {
            angleDeg: 180, 
            axis: undefined
        }
    }

    // compute angle
    var dot = Communicator.Point3.dot(point3d1, point3d2);

    var angleDeg = Math.acos(dot) / Math.PI * 180;
    
    // consider rotation direction
    var rotateAxis = Communicator.Point3.cross(point3d1, point3d2);
    rotateAxis.normalize();

    return {
        angleDeg: angleDeg, 
        axis: rotateAxis
    }
}

function rotatePoint(matrix, point) {
    const zero =  Communicator.Point3.zero();
    let zeroTranse = Communicator.Point3.zero();
    let pointTranse = Communicator.Point3.zero();
    matrix.transform(zero, zeroTranse);
    matrix.transform(point, pointTranse);
    pointTranse = pointTranse.subtract(zeroTranse);
    
    return pointTranse;
}

function CalcNormal(points) {
	const uVect = Communicator.Point3.subtract(points[1].copy(), points[0].copy()).normalize();
	const vVect = Communicator.Point3.subtract(points[2].copy(), points[0].copy()).normalize();
    let normal = Communicator.Point3.cross(uVect, vVect).normalize();
    
	return normal;
}

function DetectPolyArc(vertices) {
    let vertex1;
    let centerAxis;
    let pitchAngle;
    let totalAngle = 0;
    let rSum = 0;
    let centSum = new Communicator.Point3(0, 0, 0);
    let count = 0;

    if (vertices[0].equalsWithTolerance(vertices[vertices.length - 1], 1.0E-8)) {
        vertices.push(vertices[1]);
    }

    if (3 > vertices.length) {
        return;
    }

    for (let i = 0; i < vertices.length - 1; i++) {
        let vertex2 = Communicator.Point3.subtract(vertices[i + 1], vertices[i]);
        const dist = vertex2.length();
        
        vertex2 = vertex2.normalize();
        if (0 < i) {
            const nRotation = vectorsAngleDeg(vertex2, vertex1);
            if (undefined == nRotation.axis) {
                return;
            }

            totalAngle += nRotation.angleDeg;
            console.log(nRotation.angleDeg);

            if (undefined == centerAxis) {
                centerAxis = nRotation.axis;
                pitchAngle = nRotation.angleDeg;
            }
            else {
                if (!centerAxis.equalsWithTolerance(nRotation.axis, 1.0E-3)) {
                    return;
                }
            }

            const cRotation = vectorsAngleDeg(vertex2, nRotation.axis);

            const rad = nRotation.angleDeg * (Math.PI / 180);
            const r = dist / 2 / Math.tan(rad / 2);

            const inc = Communicator.Point3.scale(cRotation.axis, r);
            const cent = vertices[i].copy().add(inc);

            rSum += r;
            centSum = centSum.add(cent);

            count++;
        }
        vertex1 = vertex2.copy();
    }

    let r = rSum / count;
    let center = Communicator.Point3.scale(centSum, 1 / count);

    // Special case for 180 deg arc
    totalAngle +=  totalAngle / count;
    if (179.99 < Math.abs(totalAngle) && Math.abs(totalAngle) < 180.01) {
        const stPnt = vertices[0];
        const enPnt = vertices[vertices.length - 1];
        let sub = Communicator.Point3.subtract(enPnt, stPnt);
        center = new Communicator.Point3(stPnt.x + sub.x / 2, stPnt.y + sub.y / 2, stPnt.z + sub.z / 2);
        r = sub.length() / 2;
    }

    return {
        axis: centerAxis,
        r: r,
        center: center
    }
}

export class ArrowMarkup extends Communicator.Markup.MarkupItem {
    constructor(viewer, color, constntLength) {
        super();
        this._viewer = viewer;
        this._stPnt = Communicator.Point3.zero();
        this._enPnt = Communicator.Point3.zero();
        this._line = new Communicator.Markup.Shapes.Line();
        this._line.setStartEndcapType(Communicator.Markup.Shapes.EndcapType.Circle);
        this._line.setEndEndcapType(Communicator.Markup.Shapes.EndcapType.Arrowhead);
        this._line.setStrokeWidth(2);
        this._line.setStartEndcapColor(color);
        this._line.setEndEndcapColor(color);
        this._line.setStrokeColor(color);
        this._constantLength = false;
        if (undefined != constntLength) this._constantLength = true;
    }

    draw() {
        const stPnt = Communicator.Point2.fromPoint3(this._viewer.view.projectPoint(this._stPnt));
        let enPnt = Communicator.Point2.fromPoint3(this._viewer.view.projectPoint(this._enPnt));

        if (this._constantLength) {
            const p0 = new Communicator.Point2(0, 0);
            const canvasSize = this._viewer.view.getCanvasSize();   
            const pMin = this._viewer.view.unprojectPoint(p0, 0)
            const pMax = this._viewer.view.unprojectPoint(canvasSize, 0)
            let diagonalLength = Communicator.Point3.distance(pMin, pMax);
            diagonalLength /= 10;
            if (isNaN(diagonalLength) || diagonalLength < 1) diagonalLength = 1;

            const endPnt = this._stPnt.copy().add(this._enPnt.copy().scale(diagonalLength));
            enPnt = Communicator.Point2.fromPoint3(this._viewer.view.projectPoint(endPnt));
        }

        this._line.set(stPnt, enPnt);
        this._viewer.markupManager.getRenderer().drawLine(this._line);
    }
 
    hit() {
        return false;
    }

    remove () {
        return;
    }

    setPosiiton(stPnt, enPnt) {
        this._stPnt = stPnt.copy();
        this._enPnt = enPnt.copy();
    }

    getPosition() {
        return [this._stPnt, this._enPnt];
    }

    setStartEndCap(startCap, endCap) {
        this._line.setStartEndcapType(startCap);
        this._line.setEndEndcapType(endCap);
    }

}

export class MarkerMarkup extends Communicator.Markup.MarkupItem {
    constructor(viewer, color) {
        super();
        this._viewer = viewer;
        this._point = Communicator.Point3.zero();
        this._circle = new Communicator.Markup.Shapes.Circle();
        this._circle.setStrokeWidth(2);
        this._circle.setStrokeColor(color);
        this._circle.setFillColor(color);
        this._circle.setRadius(5);
    }

    draw() {
        const point = Communicator.Point2.fromPoint3(this._viewer.view.projectPoint(this._point));
        this._circle.setCenter(point);
        this._viewer.markupManager.getRenderer().drawCircle(this._circle);
    }
 
    hit() {
        return false;
    }

    remove () {
        return;
    }

    setPosiiton(point) {
        this._point = point.copy();
    }

    getPosition() {
        return [this._point];
    }

}