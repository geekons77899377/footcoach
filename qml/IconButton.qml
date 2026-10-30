import QtQuick

// Bouton rond avec une icône.
Item {
    id: root

    property string iconName: ""
    property color iconColor: Theme.text
    property color fillColor: "transparent"
    property color borderColor: "transparent"
    property color activeColor: Theme.surfaceHi
    property int diameter: 44
    property real iconSize: 22
    property bool glow: false
    property string accessibleName: ""
    signal clicked()

    implicitWidth: diameter
    implicitHeight: diameter
    opacity: enabled ? 1 : 0.35

    Accessible.role: Accessible.Button
    Accessible.name: accessibleName

    Rectangle {
        visible: root.glow
        anchors.centerIn: parent
        width: root.diameter + 10
        height: width
        radius: width / 2
        color: root.fillColor
        opacity: 0.18
    }
    Rectangle {
        anchors.centerIn: parent
        width: root.diameter
        height: width
        radius: width / 2
        color: area.pressed ? root.activeColor : root.fillColor
        border.width: root.borderColor.a > 0 ? 1 : 0
        border.color: root.borderColor
    }
    Icon {
        anchors.centerIn: parent
        name: root.iconName
        color: root.iconColor
        size: root.iconSize
    }
    MouseArea {
        id: area
        anchors.fill: parent
        anchors.margins: -4
        onClicked: root.clicked()
    }
}
