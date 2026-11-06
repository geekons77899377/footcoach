import QtQuick
import QtQuick.Controls
import "PrivacyText.js" as Privacy

// Politique de confidentialité, lue dans l'application : aucun navigateur, aucune connexion réseau.
Item {
    id: root

    property bool showBack: true
    signal closeRequested()

    PageHeader {
        id: header
        width: parent.width
        leftInset: root.showBack ? 44 : 0
        title: "Confidentialité"
        subtitle: "Vos données restent sur cet appareil"
    }
    IconButton {
        visible: root.showBack
        anchors.left: parent.left
        anchors.leftMargin: 2
        anchors.verticalCenter: header.verticalCenter
        z: 2
        iconName: "back"
        accessibleName: "Retour"
        onClicked: root.closeRequested()
    }

    Flickable {
        id: scroll
        objectName: "privacyScroll"
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        contentWidth: width
        contentHeight: body.height + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Text {
            id: body
            objectName: "privacyText"
            x: Math.max(Theme.gutter, (scroll.width - width) / 2)
            y: 8
            width: Math.min(scroll.width - 2 * Theme.gutter, 640)
            text: Privacy.TEXT
            textFormat: Text.MarkdownText
            wrapMode: Text.Wrap
            font.pixelSize: 15
            lineHeight: 1.25
            color: Theme.text
            linkColor: Theme.accent
            // Aucun lien n'est suivi : l'application n'ouvre pas de navigateur.
        }
    }

    onVisibleChanged: if (visible) scroll.contentY = 0
}
