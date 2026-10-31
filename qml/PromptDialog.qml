import QtQuick
import QtQuick.Controls

// Saisie d'un texte (court ou multiligne) ou confirmation (sans champ si fieldVisible est faux).
Popup {
    id: root

    property string title: ""
    property string message: ""
    property string value: ""
    property bool fieldVisible: true
    property bool multiline: false
    property int maxLength: 100
    property string okText: "Valider"
    property bool danger: false
    signal accepted(string text)

    function ask(titleText, initial, isMultiline, limit) {
        title = titleText;
        message = "";
        value = initial;
        fieldVisible = true;
        multiline = isMultiline;
        maxLength = limit;
        okText = "Valider";
        danger = false;
        open();
    }

    function confirm(titleText, text, ok, isDanger) {
        title = titleText;
        message = text;
        fieldVisible = false;
        okText = ok;
        danger = isDanger;
        open();
    }

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 20
    width: Math.min(parent ? parent.width - 32 : 360, 420)
    x: parent ? (parent.width - width) / 2 : 0
    // Dans le haut de l'écran : le clavier ne la recouvre pas.
    y: parent ? Math.min(parent.height * 0.18, Math.max(12, parent.height - height - 12)) : 0
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle { color: "#99000000" }

    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
        KeyboardDismissAll { popup: root }   // y compris la zone assombrie autour
    }

    onOpened: if (fieldVisible) field.forceActiveFocus()

    contentItem: Column {
        spacing: 14

        Text {
            width: parent.width
            text: root.title
            font.pixelSize: 20
            font.weight: Font.Bold
            color: Theme.text
            wrapMode: Text.Wrap
        }
        Text {
            visible: root.message !== ""
            width: parent.width
            text: root.message
            font.pixelSize: 15
            color: Theme.textMuted
            wrapMode: Text.Wrap
        }
        Rectangle {
            visible: root.fieldVisible
            width: parent.width
            height: root.multiline ? 120 : 52
            radius: 14
            color: Theme.bg
            border.width: 1
            border.color: field.activeFocus ? Theme.accent : Theme.border
            Flickable {
                anchors.fill: parent
                anchors.margins: 4
                clip: true
                contentWidth: width
                contentHeight: field.implicitHeight
                TextArea.flickable: TextArea {
                    id: field
                    text: root.value
                    color: Theme.text
                    font.pixelSize: 16
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    background: null
                    leftPadding: 10
                    rightPadding: 10
                    verticalAlignment: root.multiline ? TextEdit.AlignTop : TextEdit.AlignVCenter
                    onTextChanged: {
                        if (text.length > root.maxLength)
                            text = text.substring(0, root.maxLength);
                    }
                    Keys.onReturnPressed: (event) => {
                        if (root.multiline) {
                            event.accepted = false;
                        } else {
                            root.close();
                            root.accepted(field.text);
                        }
                    }
                }
            }
        }
        Row {
            width: parent.width
            spacing: 10
            layoutDirection: Qt.RightToLeft
            AppButton {
                kind: root.danger ? "danger" : "primary"
                text: root.okText
                onClicked: {
                    root.close();
                    root.accepted(root.fieldVisible ? field.text : "");
                }
            }
            AppButton {
                kind: "ghost"
                text: "Annuler"
                onClicked: root.close()
            }
        }
    }
}
