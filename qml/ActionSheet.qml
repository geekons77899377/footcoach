import QtQuick
import QtQuick.Controls

// Feuille d'actions en bas de l'écran (liste de choix), au style de la maquette.
// model : [{id, text, icon, danger, checked}]
Popup {
    id: root

    property string title: ""
    property var model: []
    property real navInset: 0          // espace à laisser en bas (barre de navigation)
    signal chosen(string id)

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 8
    width: Math.min(parent ? parent.width - 24 : 360, 440)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? parent.height - height - 12 - navInset : 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle { color: "#99000000" }

    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    contentItem: Column {
        spacing: 2

        Text {
            visible: root.title !== ""
            width: parent.width
            leftPadding: 14
            topPadding: 10
            bottomPadding: 6
            text: root.title.toUpperCase()
            font.pixelSize: 12
            font.weight: Font.Bold
            font.letterSpacing: 1.2
            color: Theme.textMuted
        }

        Repeater {
            model: root.model
            delegate: Rectangle {
                id: row
                required property var modelData
                width: root.availableWidth
                height: 56
                radius: 16
                color: area.pressed ? Theme.surfaceHi : "transparent"

                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    spacing: 14
                    Icon {
                        visible: row.modelData.icon !== undefined && row.modelData.icon !== ""
                        name: row.modelData.icon || ""
                        color: row.modelData.danger ? Theme.danger : Theme.textMuted
                        size: 24
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: row.modelData.text
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: row.modelData.danger ? Theme.danger : Theme.text
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Icon {
                    visible: row.modelData.checked === true
                    name: "check"
                    color: Theme.accent
                    anchors.right: parent.right
                    anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    onClicked: {
                        root.close();
                        root.chosen(row.modelData.id);
                    }
                }
            }
        }
    }
}
