import QtQuick
import "BoardGeometry.js" as Geo

// Terrain interactif : dessin du terrain et des flèches, éléments déplaçables, zoom et déplacement.
//
// Gestes (tactile) : un doigt sur un élément le déplace ; un doigt dans le vide déplace la vue (outils de pose et
// de sélection) ou trace une flèche (outils de flèche) ; deux doigts zooment et déplacent la vue ; toucher un élément le
// sélectionne, un appui long ajoute à la sélection, un toucher dans le vide pose l'élément de l'outil actif.
// Souris : molette pour zoomer, clic droit pour revenir à la sélection.
Item {
    id: view

    required property var board              // BoardModel
    property string tool: "select"
    property real zoom: 1
    property real panX: 0                    // position du coin haut gauche du terrain dans la vue
    property real panY: 0
    property var preview: null               // flèche en cours de tracé : {style, x1, y1, x2, y2} (normalisé)
    property var pendingArrow: null
    property var lasso: null                 // rectangle de sélection en cours (écran) : {x1, y1, x2, y2}
    readonly property bool multi: tool === "multi"

    readonly property real margin: 14
    readonly property real maxZoom: 8
    readonly property var frame: Geo.frameFor(board.pitch, width, height, margin)
    readonly property bool vertical: frame.vertical
    readonly property real pw: frame.pw * zoom
    readonly property real ph: frame.ph * zoom
    // Les éléments restent à taille d'écran constante (un peu plus petits tant qu'on n'a pas zoomé), ce qui
    // permet de les écarter en zoomant pour les placer précisément.
    readonly property real tokenUnit: Math.min(width, height) >= 560 ? 1.2 : 1.0
    readonly property real tokenScale: tokenUnit * Geo.clamp(0.72 + 0.28 * (zoom - 1) / 1.5, 0.72, 1.0)
    readonly property bool arrowTool: tool === "run" || tool === "pass" || tool === "dribble" || tool === "cross"

    signal placeRequested(real nx, real ny)
    signal labelEditRequested(int index)
    signal selectToolRequested()

    clip: true
    focus: true

    // --- Repère --------------------------------------------------------------------------------

    function toScreen(nx, ny) {
        var uv = Geo.toUV(nx, ny, vertical);
        return Qt.point(panX + uv.u * pw, panY + uv.v * ph);
    }

    function toNorm(sx, sy) {
        var u = Geo.clamp((sx - panX) / pw, 0, 1);
        var v = Geo.clamp((sy - panY) / ph, 0, 1);
        var n = Geo.fromUV(u, v, vertical);
        return Qt.point(n.x, n.y);
    }

    // Applique un zoom et une position (corrigés pour que le terrain ne sorte pas de la vue).
    function place(z, x, y) {
        z = Geo.clamp(z, 1, maxZoom);
        var w = frame.pw * z, h = frame.ph * z;
        var nx = w <= width - 2 * margin ? (width - w) / 2 : Geo.clamp(x, width - w - margin, margin);
        var ny = h <= height - 2 * margin ? (height - h) / 2 : Geo.clamp(y, height - h - margin, margin);
        zoom = z;
        panX = nx;
        panY = ny;
    }

    function fit() { place(1, 0, 0); }

    function zoomAt(factor, cx, cy) {
        var z = Geo.clamp(zoom * factor, 1, maxZoom);
        var k = z / zoom;
        place(z, cx - (cx - panX) * k, cy - (cy - panY) * k);
    }

    function zoomIn() { zoomAt(1.5, width / 2, height / 2); }
    function zoomOut() { zoomAt(1 / 1.5, width / 2, height / 2); }

    onFrameChanged: fit()
    Component.onCompleted: fit()
    onPanXChanged: canvas.requestPaint()
    onPanYChanged: canvas.requestPaint()
    onZoomChanged: canvas.requestPaint()
    onVerticalChanged: canvas.requestPaint()
    onPreviewChanged: canvas.requestPaint()
    Connections {
        target: view.board
        function onRevisionChanged() { canvas.requestPaint(); }
    }

    // --- Dessin du terrain et des flèches ------------------------------------------------------------

    Canvas {
        id: canvas
        anchors.fill: parent

        function arrowOpts() {
            var s = view.tokenScale;
            return { head: 18 * s, amp: 6 * s, period: 28 * s };
        }

        function strokeArrow(ctx, a, selected) {
            var p1 = view.toScreen(a.x1, a.y1), p2 = view.toScreen(a.x2, a.y2);
            var g = Geo.arrowGeometry(a.style, p1.x, p1.y, p2.x, p2.y, a.curve || 0, arrowOpts());
            if (g.line.length < 2 && g.head.length === 0)
                return;
            var w = 3.5 * view.tokenScale;
            var color = selected ? "#facc15" : "#ffffff";
            ctx.strokeStyle = color;
            ctx.fillStyle = color;
            ctx.lineWidth = w;
            ctx.lineCap = "round";
            ctx.lineJoin = "round";
            ctx.setLineDash(a.style === "run" ? [2.2 * w, 2.0 * w] : []);
            ctx.beginPath();
            ctx.moveTo(g.line[0][0], g.line[0][1]);
            for (var i = 1; i < g.line.length; ++i)
                ctx.lineTo(g.line[i][0], g.line[i][1]);
            ctx.stroke();
            ctx.setLineDash([]);
            if (g.head.length === 3) {
                ctx.lineWidth = 1;
                ctx.beginPath();
                ctx.moveTo(g.head[0][0], g.head[0][1]);
                ctx.lineTo(g.head[1][0], g.head[1][1]);
                ctx.lineTo(g.head[2][0], g.head[2][1]);
                ctx.closePath();
                ctx.fill();
                ctx.stroke();
            }
        }

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            var half = view.board.pitch === "half";
            var x0 = half ? Geo.HALF_L : 0;
            var depth = view.frame.depth;

            // Champ terrain -> écran.
            function fp(xf, yf) {
                return view.toScreen((xf - x0) / depth, yf / Geo.FIELD_W);
            }
            function path(pts) {
                var p = fp(pts[0][0], pts[0][1]);
                ctx.moveTo(p.x, p.y);
                for (var i = 1; i < pts.length; ++i) {
                    p = fp(pts[i][0], pts[i][1]);
                    ctx.lineTo(p.x, p.y);
                }
            }

            // Fond autour du terrain.
            ctx.fillStyle = "#245a2b";
            ctx.fillRect(0, 0, view.width, view.height);

            // Bandes de pelouse.
            var stripe = Geo.FIELD_L / 10;
            for (var i = 0; i < 10; ++i) {
                var xa = Math.max(i * stripe, x0), xb = Math.min((i + 1) * stripe, x0 + depth);
                if (xb <= xa)
                    continue;
                ctx.fillStyle = i % 2 === 0 ? "#2e7d32" : "#348c39";
                ctx.beginPath();
                path([[xa, 0], [xb, 0], [xb, Geo.FIELD_W], [xa, Geo.FIELD_W]]);
                ctx.closePath();
                ctx.fill();
            }

            // Lignes (rien au-delà de la ligne médiane en demi-terrain).
            ctx.save();
            if (half) {
                ctx.beginPath();
                path([[x0, -4], [Geo.FIELD_L + 4, -4], [Geo.FIELD_L + 4, Geo.FIELD_W + 4], [x0, Geo.FIELD_W + 4]]);
                ctx.closePath();
                ctx.clip();
            }
            ctx.strokeStyle = "rgba(255,255,255,0.92)";
            ctx.fillStyle = "rgba(255,255,255,0.24)";
            ctx.lineWidth = 2;
            ctx.lineJoin = "round";
            var lines = Geo.pitchLines();
            for (var k = 0; k < lines.length; ++k) {
                var ln = lines[k];
                if (ln.dot) {
                    var d = fp(ln.dot[0], ln.dot[1]);
                    ctx.fillStyle = "rgba(255,255,255,0.92)";
                    ctx.beginPath();
                    ctx.arc(d.x, d.y, 3, 0, 2 * Math.PI);
                    ctx.fill();
                    continue;
                }
                ctx.beginPath();
                path(ln.pts);
                if (ln.fill) {
                    ctx.fillStyle = "rgba(255,255,255,0.24)";
                    ctx.fill();
                }
                ctx.stroke();
            }
            ctx.restore();

            // Flèches.
            var arrows = view.board.arrows;
            for (var j = 0; j < arrows.length; ++j)
                strokeArrow(ctx, arrows[j], arrows[j].sel);
            if (view.preview)
                strokeArrow(ctx, view.preview, false);
        }
    }

    // --- Éléments -------------------------------------------------------------------------------------

    Repeater {
        id: rep
        model: view.board.items
        delegate: BoardToken {
            boardView: view
        }
    }

    // --- Recherche de ce qui est touché ---------------------------------------------------------

    // Indice de l'élément sous le point (coordonnées de la vue), -1 s'il n'y en a pas. Le plus proche du centre gagne.
    function tokenAt(px, py) {
        var best = -1, bestD = 1e9;
        for (var k = 0; k < rep.count; ++k) {
            var t = rep.itemAt(k);
            if (!t)
                continue;
            var p = t.mapFromItem(view, px, py);
            if (p.x < 0 || p.y < 0 || p.x > t.width || p.y > t.height)
                continue;
            var c = t.mapToItem(view, t.width / 2, t.height / 2);
            var d = Math.hypot(c.x - px, c.y - py);
            if (d < bestD) {
                best = k;
                bestD = d;
            }
        }
        return best;
    }

    function arrowAt(px, py) {
        var best = -1, bestD = 18 * tokenUnit;
        var arrows = board.arrows;
        for (var i = 0; i < arrows.length; ++i) {
            var a = arrows[i];
            var p1 = toScreen(a.x1, a.y1), p2 = toScreen(a.x2, a.y2);
            var g = Geo.arrowGeometry(a.style, p1.x, p1.y, p2.x, p2.y, a.curve || 0, canvas.arrowOpts());
            var d = Geo.distToArrow(g, px, py);
            if (d < bestD) {
                best = i;
                bestD = d;
            }
        }
        return best;
    }

    // Sélectionne (en plus de l'existant) tout ce qui est dans le rectangle tracé.
    function selectInLasso() {
        var r = lasso;
        lasso = null;
        if (!r)
            return;
        var x0 = Math.min(r.x1, r.x2), x1 = Math.max(r.x1, r.x2);
        var y0 = Math.min(r.y1, r.y2), y1 = Math.max(r.y1, r.y2);
        function inside(p) { return p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1; }
        var its = [], ars = [];
        for (var i = 0; i < board.items.count; ++i) {
            var it = board.items.get(i);
            if (inside(toScreen(it.nx, it.ny)))
                its.push(i);
        }
        for (var j = 0; j < board.arrows.length; ++j) {
            var a = board.arrows[j];
            var p1 = toScreen(a.x1, a.y1), p2 = toScreen(a.x2, a.y2);
            if (inside(p1) || inside(p2) || inside(Qt.point((p1.x + p2.x) / 2, (p1.y + p2.y) / 2)))
                ars.push(j);
        }
        board.selectMany(its, ars);
    }

    function flushArrow() {
        if (pendingArrow) {
            var a = pendingArrow;
            pendingArrow = null;
            board.addArrow(a.style, a.x1, a.y1, a.x2, a.y2);
        }
        preview = null;
    }

    function cancelPendingArrow() {
        arrowTimer.stop();
        pendingArrow = null;
        preview = null;
    }

    // Laisse un instant au deuxième doigt pour arriver : un tracé interrompu par un zoom n'est pas gardé.
    Timer {
        id: arrowTimer
        interval: 160
        onTriggered: view.flushArrow()
    }

    Rectangle {
        visible: view.lasso !== null
        x: view.lasso ? Math.min(view.lasso.x1, view.lasso.x2) : 0
        y: view.lasso ? Math.min(view.lasso.y1, view.lasso.y2) : 0
        width: view.lasso ? Math.abs(view.lasso.x2 - view.lasso.x1) : 0
        height: view.lasso ? Math.abs(view.lasso.y2 - view.lasso.y1) : 0
        color: "#332ecc71"
        border.width: 2
        border.color: Theme.accent
        radius: 4
    }

    // --- Gestes ---------------------------------------------------------------------------------------

    DragHandler {
        id: drag
        target: null
        maximumPointCount: 1
        acceptedButtons: Qt.LeftButton

        property string mode: ""
        property int index: -1
        property real startLeft: 0
        property real startTop: 0
        property point startNorm: Qt.point(0, 0)
        property point pressNorm: Qt.point(0, 0)

        onActiveChanged: {
            if (active) {
                var press = centroid.pressPosition;
                if (view.arrowTool) {
                    view.flushArrow();
                    mode = "arrow";
                    var n = view.toNorm(press.x, press.y);
                    view.preview = { style: view.tool, x1: n.x, y1: n.y, x2: n.x, y2: n.y,
                                     curve: view.tool === "cross" ? view.board.defaultCurve : 0 };
                    return;
                }
                var hit = view.tokenAt(press.x, press.y);
                if (hit < 0 && view.multi) {
                    mode = "lasso";
                    view.lasso = { x1: press.x, y1: press.y, x2: press.x, y2: press.y };
                    return;
                }
                if (hit >= 0) {
                    mode = "token";
                    index = hit;
                    if (!view.board.items.get(hit).sel)
                        view.board.selectItem(hit, false);
                    view.board.beginMove();
                    startNorm = Qt.point(view.board.items.get(hit).nx, view.board.items.get(hit).ny);
                    var s0 = view.toScreen(startNorm.x, startNorm.y);
                    pressNorm = Qt.point(s0.x, s0.y);          // position d'écran de l'élément au départ
                    return;
                }
                mode = "pan";
                startLeft = view.panX;
                startTop = view.panY;
            } else {
                if (mode === "lasso") {
                    view.selectInLasso();
                } else if (mode === "token") {
                    view.board.commit();
                } else if (mode === "arrow" && view.preview) {
                    var p = view.preview;
                    var a = view.toScreen(p.x1, p.y1), b = view.toScreen(p.x2, p.y2);
                    if (Math.hypot(b.x - a.x, b.y - a.y) >= 20) {
                        view.pendingArrow = p;
                        arrowTimer.restart();
                    } else {
                        view.preview = null;
                    }
                }
                mode = "";
            }
        }

        onCentroidChanged: {
            if (!active)
                return;
            var press = centroid.pressPosition, pos = centroid.position;
            var dx = pos.x - press.x, dy = pos.y - press.y;
            if (mode === "lasso") {
                view.lasso = { x1: press.x, y1: press.y, x2: pos.x, y2: pos.y };
            } else if (mode === "pan") {
                view.place(view.zoom, startLeft + dx, startTop + dy);
            } else if (mode === "token") {
                var target = view.toNorm(pressNorm.x + dx, pressNorm.y + dy);
                view.board.moveBy(target.x - startNorm.x, target.y - startNorm.y);
            } else if (mode === "arrow" && view.preview) {
                var n = view.toNorm(pos.x, pos.y);
                var p = view.preview;
                view.preview = { style: p.style, x1: p.x1, y1: p.y1, x2: n.x, y2: n.y, curve: p.curve };
            }
        }
    }

    PinchHandler {
        id: pinch
        target: null
        minimumPointCount: 2
        maximumPointCount: 2
        rotationAxis.enabled: false

        property real z0: 1
        property real l0: 0
        property real t0: 0
        property point c0: Qt.point(0, 0)

        function apply() {
            if (!active)
                return;
            var z = Geo.clamp(z0 * activeScale, 1, view.maxZoom);
            var k = z / z0;
            view.place(z, centroid.position.x - (c0.x - l0) * k, centroid.position.y - (c0.y - t0) * k);
        }
        onActiveChanged: {
            if (active) {
                z0 = view.zoom;
                l0 = view.panX;
                t0 = view.panY;
                c0 = centroid.position;
                view.cancelPendingArrow();
            }
        }
        onActiveScaleChanged: apply()
        onCentroidChanged: apply()
    }

    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: (event) => view.zoomAt(Math.pow(1.0015, event.angleDelta.y), point.position.x, point.position.y)
    }

    TapHandler {
        id: tap
        acceptedButtons: Qt.LeftButton
        property bool wasLongPress: false
        onPressedChanged: if (pressed) wasLongPress = false
        onLongPressed: {
            var hit = view.tokenAt(point.position.x, point.position.y);
            if (hit >= 0) {
                wasLongPress = true;
                view.board.selectItem(hit, true);
            }
        }
        onTapped: (eventPoint) => {
            if (wasLongPress)
                return;
            var px = eventPoint.position.x, py = eventPoint.position.y;
            var hit = view.tokenAt(px, py);
            if (hit >= 0) {
                var kind = view.board.items.get(hit).kind;
                if (tapCount === 2 && view.board.hasLabel(kind))
                    view.labelEditRequested(hit);
                else
                    view.board.selectItem(hit, view.multi);
                return;
            }
            if (view.tool === "select" || view.multi || view.arrowTool) {
                var ai = view.arrowAt(px, py);
                if (ai >= 0)
                    view.board.selectArrow(ai, view.multi);
                else if (!view.multi)
                    view.board.clearSelection();
                return;
            }
            var n = view.toNorm(px, py);
            view.placeRequested(n.x, n.y);
        }
    }

    // Clic droit (souris) : retour à la sélection. Sur écran tactile, acceptedButtons est ignoré : sans cette
    // condition, chaque toucher remettait l'outil sur « sélection » et rien ne se posait.
    TapHandler {
        acceptedButtons: Qt.RightButton
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onTapped: view.selectToolRequested()
    }

    Keys.onPressed: (event) => {
        var ctrl = event.modifiers & Qt.ControlModifier;
        if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace) {
            board.deleteSelected();
        } else if (ctrl && event.key === Qt.Key_Z) {
            board.undo();
        } else if (ctrl && event.key === Qt.Key_Y) {
            board.redo();
        } else if (event.key === Qt.Key_Escape) {
            selectToolRequested();
        } else if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
            board.scaleSelected(1.25);
        } else if (event.key === Qt.Key_Minus) {
            board.scaleSelected(1 / 1.25);
        } else if (event.key === Qt.Key_R) {
            board.rotateSelected(event.modifiers & Qt.ShiftModifier ? -45 : 45);
        } else if (event.key === Qt.Key_C) {
            board.flipSelectedCurves();
        } else {
            return;
        }
        event.accepted = true;
    }
}
