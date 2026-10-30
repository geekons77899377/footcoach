import QtQuick

// Page pas encore migrée vers la nouvelle interface.
Item {
    id: root

    property string title: ""
    property string iconName: "home"
    property string description: ""

    PageHeader {
        id: header
        width: parent.width
        title: root.title
    }

    Card {
        anchors.top: header.bottom
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.gutter
        anchors.rightMargin: Theme.gutter
        Column {
            width: parent.width
            spacing: 12
            Icon {
                name: root.iconName
                color: Theme.accent
                size: 32
            }
            Text {
                width: parent.width
                text: "Bientôt disponible"
                font.pixelSize: 18
                font.weight: Font.Bold
                color: Theme.text
            }
            Text {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: 14
                color: Theme.textMuted
                text: root.description !== "" ? root.description
                      : "Cette page est en cours de migration vers la nouvelle interface. "
                        + "Elle arrive dans une prochaine étape."
            }
        }
    }
}
