import QtQuick
import QtQuick.Controls

// Bouton aux couleurs de la maquette. kind : "primary" (vert plein), "outline", "ghost", "danger".
Button {
    id: root

    property string kind: "outline"
    property string iconName: ""
    property color fg: kind === "primary" ? Theme.textOnAccent : (kind === "danger" ? Theme.danger : Theme.text)

    implicitHeight: Theme.touch
    implicitWidth: Math.max(Theme.touch, contentRow.implicitWidth + 2 * 18)
    padding: 0
    hoverEnabled: true

    background: Rectangle {
        radius: height / 2
        color: root.kind === "primary" ? (root.down ? "#27ae60" : Theme.accent)
               : root.kind === "ghost" ? (root.down ? Theme.surfaceHi : "transparent")
               : (root.down ? Theme.surfaceHi : Theme.surface)
        border.width: root.kind === "primary" || root.kind === "ghost" ? 0 : 1
        border.color: root.kind === "danger" ? Theme.danger : Theme.border
        opacity: root.enabled ? 1 : 0.4
    }

    contentItem: Item {
        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: 8
            opacity: root.enabled ? 1 : 0.4
            Icon {
                visible: root.iconName !== ""
                name: root.iconName
                color: root.fg
                size: 22
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                visible: root.text !== ""
                text: root.text
                color: root.fg
                font.pixelSize: 15
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
