import QtQuick

// Miniature d'un schéma (terrain, éléments et flèches) à partir de son JSON.
Rectangle {
    id: root

    property string json: ""

    width: 72
    height: 52
    radius: 12
    color: "#2e7d32"
    border.width: 1
    border.color: "#4cae50"
    clip: true

    onJsonChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            var data = {};
            try {
                data = root.json === "" ? {} : JSON.parse(root.json);
            } catch (e) {
                data = {};
            }
            var half = data.pitch === "half";
            var w = width, h = height, m = 5;
            // Repère : terrain entier horizontal, demi-terrain vertical (but en haut).
            function pt(nx, ny) {
                return half ? [m + ny * (w - 2 * m), m + (1 - nx) * (h - 2 * m)]
                            : [m + nx * (w - 2 * m), m + ny * (h - 2 * m)];
            }
            ctx.strokeStyle = "rgba(255,255,255,0.7)";
            ctx.lineWidth = 1;
            ctx.strokeRect(m, m, w - 2 * m, h - 2 * m);
            ctx.beginPath();
            if (half) {
                ctx.moveTo(m, h - m);
                ctx.lineTo(w - m, h - m);
            } else {
                ctx.moveTo(w / 2, m);
                ctx.lineTo(w / 2, h - m);
            }
            ctx.stroke();
            var arrows = data.arrows || [];
            ctx.strokeStyle = "rgba(255,255,255,0.9)";
            for (var j = 0; j < arrows.length; ++j) {
                var a = arrows[j];
                var p1 = pt(a.x1, a.y1), p2 = pt(a.x2, a.y2);
                ctx.beginPath();
                ctx.moveTo(p1[0], p1[1]);
                ctx.lineTo(p2[0], p2[1]);
                ctx.stroke();
            }
            var items = data.items || [];
            for (var i = 0; i < items.length; ++i) {
                var it = items[i];
                var p = pt(it.x, it.y);
                ctx.fillStyle = it.color ? it.color
                    : it.kind === "player" ? "#1565c0" : it.kind === "opponent" ? "#c62828"
                    : it.kind === "cone" ? "#ff9800" : "#ffffff";
                ctx.beginPath();
                ctx.arc(p[0], p[1], it.kind === "ball" ? 1.8 : 2.6, 0, 2 * Math.PI);
                ctx.fill();
            }
        }
    }
}
