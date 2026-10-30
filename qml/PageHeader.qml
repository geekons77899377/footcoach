import QtQuick

// En-tête de page : titre à gauche, actions rondes à droite (comme « Squad » dans la maquette).
Item {
    id: root

    property string title: ""
    property string subtitle: ""
    default property alias actions: actionRow.data

    implicitHeight: 76

    Column {
        anchors.left: parent.left
        anchors.leftMargin: Theme.gutter + 4
        anchors.right: actionRow.left
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        Text {
            width: parent.width
            text: root.title
            elide: Text.ElideRight
            font.pixelSize: 26
            font.weight: Font.ExtraBold
            color: Theme.text
        }
        Text {
            visible: root.subtitle !== ""
            width: parent.width
            text: root.subtitle
            elide: Text.ElideRight
            font.pixelSize: 13
            color: Theme.textMuted
        }
    }

    Row {
        id: actionRow
        anchors.right: parent.right
        anchors.rightMargin: Theme.gutter
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
        opacity: 0.6
    }
}
