/// <reference path="../../typescript/hoops_web_viewer.d.ts" />

class ClosestLineMarkup extends Communicator.Markup.MarkupItem {
    private _viewer: Communicator.WebViewer;
    private _point1: Communicator.Point3;
    private _point2: Communicator.Point3;
    private _line: Communicator.Markup.Shape.Line;
    private _dimText = new Communicator.Markup.Shape.Text("", Communicator.Point2.zero());
    
    constructor(viewer: Communicator.WebViewer, point1: Communicator.Point3, point2: Communicator.Point3, text: string) {
        super();
        this._viewer = viewer;
        this._point1 = point1.copy();
        this._point2 = point2.copy();
        this._line = new Communicator.Markup.Shape.Line();
        this._line.setStrokeColor(new Communicator.Color(255, 0, 255));
        this._line.setStartEndcapColor(new Communicator.Color(255, 0, 255));
        this._line.setEndEndcapColor(new Communicator.Color(255, 0, 255));
        this._line.setStartEndcapSize(3);
        this._line.setEndEndcapSize(3);
        this._line.setStartEndcapType(Communicator.Markup.Shape.EndcapType.Circle);
        this._line.setEndEndcapType(Communicator.Markup.Shape.EndcapType.Circle);

        this._dimText.setText(text);
        this._dimText.setFontSize(20);
    }

    draw() {
        var p1 = Communicator.Point2.fromPoint3(this._viewer.view.projectPoint(this._point1));
        var p2 = Communicator.Point2.fromPoint3(this._viewer.view.projectPoint(this._point2));
        this._line.set(p1, p2);
        this._viewer.markupManager.getRenderer().drawLine(this._line);

        let miPnt = Communicator.Point2.subtract(p2, p1);
        miPnt = Communicator.Point2.add(p1, miPnt.scale(0.5));
        this._dimText.setPosition(miPnt);
        this._viewer.markupManager.getRenderer().drawText(this._dimText);
    }

    hit() {
        return false;
    }

    public remove() {
        return;
    }
}

