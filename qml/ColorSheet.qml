import QtQuick
import QtQuick.Controls

// Choix d'une couleur parmi la palette de l'application ("" = couleur par défaut).
Popup {
    id: root

    property var palette: []          // [{name, color}]
    signal picked(string color)

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 18
    width: Math.min(parent ? parent.width - 24 : 340, 380)
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
        spacing: 14
        Text {
            text: "COULEUR"
            font.pixelSize: 12
            font.weight: Font.Bold
            font.letterSpacing: 1.2
            color: Theme.textMuted
        }
        Flow {
            width: parent.width
            spacing: 14
            Repeater {
                model: root.palette
                delegate: Item {
                    id: sw
                    required property var modelData
                    width: 48
                    height: 48
                    Rectangle {
                        anchors.fill: parent
                        radius: 24
                        color: sw.modelData.color
                        border.width: 2
                        border.color: "#ffffff"
                    }
                    Accessible.role: Accessible.Button
                    Accessible.name: sw.modelData.name
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.close();
                            root.picked(sw.modelData.color);
                        }
                    }
                }
            }
            // Couleur par défaut de l'élément.
            Item {
                width: 48
                height: 48
                Rectangle {
                    anchors.fill: parent
                    radius: 24
                    color: "transparent"
                    border.width: 2
                    border.color: Theme.textMuted
                }
                Icon {
                    anchors.centerIn: parent
                    name: "close"
                    color: Theme.textMuted
                    size: 22
                }
                Accessible.role: Accessible.Button
                Accessible.name: "Couleur par défaut"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.close();
                        root.picked("");
                    }
                }
            }
        }
    }
}
