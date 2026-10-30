import QtQuick

// Carte arrondie à bordure fine, comme dans la maquette.
Rectangle {
    id: root

    property bool highlighted: false
    default property alias content: inner.data
    property int padding: 16

    implicitHeight: inner.childrenRect.height + 2 * padding
    radius: Theme.radius
    color: Theme.surface
    border.width: 1
    border.color: highlighted ? Theme.accent : Theme.border

    Item {
        id: inner
        anchors.fill: parent
        anchors.margins: root.padding
    }
}
