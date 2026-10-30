import QtQuick
import "BoardGeometry.js" as Geo

// Un élément du terrain (joueur, adversaire, ballon, plot, but ou note) à sa position à l'écran.
// Les dimensions sont en « unités de conception » (environ des pixels) ; l'élément entier est mis à l'échelle
// avec le zoom de la vue et la taille choisie, ce qui agrandit aussi sa zone de toucher.
Item {
    id: tok

    required property int index
    required property string kind
    required property real nx
    required property real ny
    required property string label
    required property real rot
    required property real scl
    required property string color
    required property bool sel
    property var boardView

    readonly property bool isCircle: kind === "player" || kind === "opponent"
    readonly property bool isNote: kind === "text"

    // Taille visible (unités de conception).
    readonly property real vw: isCircle ? 36 : kind === "ball" ? 22 : kind === "cone" ? 28 : kind === "goal" ? 14 : note.width
    readonly property real vh: isCircle ? 36 : kind === "ball" ? 22 : kind === "cone" ? 28 : kind === "goal" ? 76 : note.height

    // Zone de toucher : jamais plus petite qu'un doigt (environ 44 px à l'écran à l'échelle normale).
    width: Math.max(vw + 8, 54)
    height: Math.max(vh + 8, 54)

    readonly property point screenPos: boardView.toScreen(nx, ny)
    x: screenPos.x - width / 2
    y: screenPos.y - height / 2
    transformOrigin: Item.Center
    scale: boardView.tokenScale * ((kind === "goal" || kind === "cone" || kind === "text") ? scl : 1)
    // Les buts tournent avec l'orientation du terrain ; les plots et les notes gardent leur angle d'écran.
    rotation: kind === "goal" ? rot + Geo.orientationOffset(boardView.board.pitch, boardView.vertical) : rot
    z: (kind === "goal" ? 2 : kind === "cone" ? 3 : isCircle ? 4 : kind === "ball" ? 5 : 6) + index * 0.001

    readonly property color fill: {
        if (color !== "")
            return color;
        return kind === "player" ? "#1565c0" : kind === "opponent" ? "#c62828" : kind === "cone" ? "#ff9800" : "#ffffff";
    }
    readonly property bool lightFill: Qt.color(fill).hslLightness > 0.67

    // Contour de sélection.
    Rectangle {
        visible: tok.sel
        anchors.centerIn: parent
        width: tok.vw + 10
        height: tok.vh + 10
        radius: tok.isCircle || tok.kind === "ball" ? width / 2 : 8
        color: "transparent"
        border.width: 2.5
        border.color: Theme.selection
    }

    // Joueur ou adversaire.
    Rectangle {
        visible: tok.isCircle
        anchors.centerIn: parent
        width: 36
        height: 36
        radius: 18
        color: tok.fill
        border.width: 2.5
        border.color: "#ffffff"
        Text {
            anchors.centerIn: parent
            text: tok.label
            font.bold: true
            font.pixelSize: tok.label.length <= 2 ? 16 : 11
            color: tok.lightFill ? "#212121" : "#ffffff"
        }
    }

    // Ballon.
    Rectangle {
        visible: tok.kind === "ball"
        anchors.centerIn: parent
        width: 22
        height: 22
        radius: 11
        color: "#ffffff"
        border.width: 2
        border.color: "#000000"
    }

    // Plot.
    Canvas {
        id: cone
        visible: tok.kind === "cone"
        anchors.centerIn: parent
        width: 28
        height: 28
        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            ctx.fillStyle = tok.fill;
            ctx.strokeStyle = Qt.darker(tok.fill, 2.0);
            ctx.lineWidth = 2;
            ctx.lineJoin = "round";
            ctx.beginPath();
            ctx.moveTo(14, 2);
            ctx.lineTo(25, 24);
            ctx.lineTo(3, 24);
            ctx.closePath();
            ctx.fill();
            ctx.stroke();
        }
        Connections {
            target: tok
            function onFillChanged() { cone.requestPaint(); }
        }
    }

    // But mobile : la ligne de but est verticale à rotation 0.
    Rectangle {
        visible: tok.kind === "goal"
        anchors.centerIn: parent
        width: 12
        height: 70
        color: "#46ffffff"
        border.width: 2.5
        border.color: "#ffffff"
    }

    // Note texte.
    Rectangle {
        id: note
        visible: tok.isNote
        anchors.centerIn: parent
        width: noteText.width + 16
        height: noteText.height + 10
        radius: 6
        color: "#96000000"
        Text {
            id: noteText
            anchors.centerIn: parent
            width: Math.min(implicitWidth, 200)
            text: tok.label
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 16
            color: tok.color === "" ? "#ffffff" : tok.color
        }
    }
}
