import QtQuick
import QtQuick.Controls

// Pourquoi la feuille de challenge ne peut pas être créée : message, puis les joueurs à corriger (licence manquante ou invalide).
Popup {
    id: root

    property string message: ""
    property var problems: []

    function show(check) {
        message = check.error || "";
        problems = check.problems || [];
        open();
    }

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 20
    width: Math.min(parent ? parent.width - 32 : 360, 460)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? Math.min(parent.height * 0.06, Math.max(12, parent.height - height - 12)) : 0
    height: Math.min(implicitHeight, parent ? parent.height - 24 : implicitHeight)
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle { color: "#99000000" }
    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    contentItem: Flickable {
        id: scroller
        implicitHeight: col.implicitHeight
        contentWidth: width
        contentHeight: col.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        Column {
            id: col
            width: scroller.width
            spacing: 14
            Text {
                width: parent.width
                text: "Feuille de challenge impossible"
                font.pixelSize: 20
                font.weight: Font.Bold
                wrapMode: Text.Wrap
                color: Theme.text
            }
            Text {
                objectName: "challengeMessage"
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: 15
                color: Theme.text
                text: root.message
            }
            Column {
                visible: root.problems.length > 0
                width: parent.width
                spacing: 4
                Text {
                    width: parent.width
                    wrapMode: Text.Wrap
                    font.pixelSize: 13
                    color: Theme.textMuted
                    text: "Corrigez la fiche du joueur (N° de licence : 10 chiffres), puis recommencez."
                }
                Repeater {
                    model: root.problems.length
                    Text {
                        required property int index
                        objectName: "challengeProblem"
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 14
                        color: Theme.danger
                        text: root.problems[index].name + " : " + root.problems[index].message
                    }
                }
            }
            Row {
                width: parent.width
                layoutDirection: Qt.RightToLeft
                AppButton { objectName: "challengeClose"; kind: "primary"; text: "Fermer"; onClicked: root.close() }
            }
        }
    }
}
