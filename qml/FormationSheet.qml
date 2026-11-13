import QtQuick
import QtQuick.Controls

// Choix d'une formation de départ, avec ou sans adversaires placés en miroir.
Popup {
    id: root

    // Formats de jeu : [{ n: joueurs par équipe, label, names: formations }]. Les adversaires ont toujours autant de
    // joueurs que l'équipe (formation du même nom placée en miroir).
    property var formats: []
    property int perSide: 11
    readonly property var names: {
        for (var i = 0; i < formats.length; ++i)
            if (formats[i].n === perSide)
                return formats[i].names;
        return formats.length > 0 ? formats[0].names : [];
    }
    property bool withOpponents: false
    signal picked(string name, bool withOpponents)

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 18
    width: Math.min(parent ? parent.width - 24 : 340, 400)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? parent.height - height - 12 : 0

    Overlay.modal: Rectangle { color: "#99000000" }

    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    contentItem: Column {
        spacing: 16
        Text {
            text: "FORMATION"
            font.pixelSize: 12
            font.weight: Font.Bold
            font.letterSpacing: 1.2
            color: Theme.textMuted
        }
        Text {
            width: parent.width
            text: "Remplace les joueurs de l'équipe par la formation choisie (terrain entier)."
            wrapMode: Text.Wrap
            font.pixelSize: 14
            color: Theme.textMuted
        }
        Row {
            objectName: "formatRow"
            spacing: 10
            Repeater {
                model: root.formats
                delegate: Rectangle {
                    id: fchip
                    required property var modelData
                    objectName: "format" + modelData.n
                    width: 84
                    height: 40
                    radius: 20
                    color: root.perSide === modelData.n ? Theme.accent : Theme.bg
                    border.width: 1
                    border.color: Theme.border
                    Text {
                        anchors.centerIn: parent
                        text: fchip.modelData.label
                        font.pixelSize: 15
                        font.weight: Font.Bold
                        color: root.perSide === fchip.modelData.n ? Theme.textOnAccent : Theme.text
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.perSide = fchip.modelData.n
                    }
                }
            }
        }
        Row {
            width: parent.width
            spacing: 12
            Switch {
                id: opp
                checked: root.withOpponents
                onToggled: root.withOpponents = checked
            }
            Text {
                text: "Avec adversaires (autant de joueurs)"
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: 16
                color: Theme.text
            }
        }
        Flow {
            width: parent.width
            spacing: 10
            Repeater {
                model: root.names
                delegate: Rectangle {
                    id: chip
                    required property string modelData
                    width: 84
                    height: 48
                    radius: 24
                    color: area.pressed ? Theme.accent : Theme.bg
                    border.width: 1
                    border.color: Theme.border
                    Text {
                        anchors.centerIn: parent
                        text: chip.modelData
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        color: area.pressed ? Theme.textOnAccent : Theme.text
                    }
                    MouseArea {
                        id: area
                        anchors.fill: parent
                        onClicked: {
                            root.close();
                            root.picked(chip.modelData, root.withOpponents);
                        }
                    }
                }
            }
        }
    }
}
