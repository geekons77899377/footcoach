import QtQuick

// Message bref en bas de la page (disparaît seul).
Rectangle {
    id: root

    property string message: ""
    function show(text) {
        message = text;
        opacity = 1;
        timer.restart();
    }

    opacity: 0
    visible: opacity > 0
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 24
    width: Math.min(parent.width - 32, label.implicitWidth + 36)
    height: label.implicitHeight + 24
    radius: 16
    color: Theme.surfaceHi
    border.width: 1
    border.color: Theme.border
    z: 10
    Text {
        id: label
        anchors.centerIn: parent
        width: parent.width - 28
        wrapMode: Text.Wrap
        text: root.message
        font.pixelSize: 14
        color: Theme.text
    }
    Timer { id: timer; interval: 3000; onTriggered: root.opacity = 0 }
}
