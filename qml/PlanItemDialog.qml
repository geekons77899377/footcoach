import QtQuick
import QtQuick.Controls

// Durée et notes d'un exercice dans le plan d'une séance.
Popup {
    id: root

    property int itemId: -1
    property string heading: ""
    property string errorText: ""
    signal accepted(int itemId, int minutes, string notes)

    function openFor(entry) {
        itemId = entry.itemId;
        heading = entry.title;
        durationField.text = entry.durationMin > 0 ? String(entry.durationMin) : "";
        notesField.text = entry.notes;
        errorText = "";
        open();
    }

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 20
    width: Math.min(parent ? parent.width - 32 : 360, 420)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? Math.min(parent.height * 0.1, Math.max(12, parent.height - height - 12)) : 0
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle { color: "#99000000" }
    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }
    onOpened: durationField.focusField()

    contentItem: Column {
        spacing: 14
        Text {
            width: parent.width
            text: root.heading
            elide: Text.ElideRight
            font.pixelSize: 20
            font.weight: Font.Bold
            color: Theme.text
        }
        FormField {
            id: durationField
            width: parent.width
            label: "Durée dans cette séance (min)"
            required: true
            maxLength: 3
            inputHints: Qt.ImhDigitsOnly
        }
        FormField {
            id: notesField
            width: parent.width
            label: "Notes"
            placeholder: "Consignes du jour, variante, groupes…"
            multiline: true
            maxLength: 500
        }
        Text {
            visible: root.errorText !== ""
            width: parent.width
            text: root.errorText
            wrapMode: Text.Wrap
            font.pixelSize: 14
            color: Theme.danger
        }
        Row {
            width: parent.width
            spacing: 10
            layoutDirection: Qt.RightToLeft
            AppButton {
                kind: "primary"
                text: "Enregistrer"
                onClicked: {
                    var m = parseInt(durationField.text.trim(), 10);
                    if (isNaN(m) || m < 1 || m > 300) {
                        root.errorText = "La durée doit être comprise entre 1 et 300 minutes.";
                        return;
                    }
                    root.close();
                    root.accepted(root.itemId, m, notesField.text);
                }
            }
            AppButton { kind: "ghost"; text: "Annuler"; onClicked: root.close() }
        }
    }
}
