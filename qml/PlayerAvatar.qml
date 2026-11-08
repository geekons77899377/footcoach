import QtQuick

// Pastille d'un joueur dans les listes : son numéro, ou sa photo (rognée en rond) avec le numéro en petite pastille.
Item {
    id: root

    property int playerId: -1
    property bool hasPhoto: false
    property int number: 0
    property int size: 46
    property int numberPixelSize: 18
    property string numberFont: ""
    property color maskColor: Theme.surface         // couleur de la carte derrière : les coins de la photo sont recouverts

    readonly property string photoSource: hasPhoto && playerId > 0 ? backend.playerPhoto(playerId) : ""
    readonly property bool showPhoto: photoSource !== ""

    width: size
    height: size

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.accentSoft
        border.width: 1
        border.color: Theme.accent
        visible: !root.showPhoto
        Text {
            anchors.centerIn: parent
            text: root.number > 0 ? root.number : "–"
            font.family: root.numberFont !== "" ? root.numberFont : Qt.application.font.family
            font.pixelSize: root.numberPixelSize
            font.weight: Font.ExtraBold
            color: Theme.accent
        }
    }

    Item {
        anchors.fill: parent
        visible: root.showPhoto
        Image {
            objectName: "avatarPhoto"
            anchors.fill: parent
            source: root.photoSource
            sourceSize: Qt.size(root.size * 2, root.size * 2)
            fillMode: Image.PreserveAspectCrop
            asynchronous: false
            smooth: true
        }
        Canvas {
            anchors.fill: parent
            onPaint: {
                var c = getContext("2d");
                c.reset();
                c.fillStyle = root.maskColor;
                c.beginPath();
                c.rect(0, 0, width, height);
                c.arc(width / 2, height / 2, width / 2 - 0.5, 0, Math.PI * 2, true);
                c.fill();
                c.strokeStyle = Theme.accent;
                c.lineWidth = 1.5;
                c.beginPath();
                c.arc(width / 2, height / 2, width / 2 - 0.75, 0, Math.PI * 2);
                c.stroke();
            }
            Component.onCompleted: requestPaint()
            Connections {
                target: root
                function onMaskColorChanged() { requestPaint(); }
            }
        }
        // Le numéro reste lisible : petite pastille en bas à droite.
        Rectangle {
            visible: root.number > 0
            objectName: "avatarNumber"
            width: Math.max(18, numText.implicitWidth + 8)
            height: 18
            radius: 9
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: -3
            anchors.bottomMargin: -3
            color: Theme.accent
            border.width: 1.5
            border.color: root.maskColor
            Text {
                id: numText
                anchors.centerIn: parent
                text: root.number
                font.pixelSize: 11
                font.weight: Font.ExtraBold
                color: Theme.textOnAccent
            }
        }
    }
}
