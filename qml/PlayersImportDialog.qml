import QtQuick
import QtQuick.Controls

// Import d'une liste de joueurs collée : un joueur par ligne, seul le prénom est obligatoire (voir Backend::importPlayers).
// Tout ou rien : à la moindre ligne refusée, rien n'est créé et les lignes à corriger sont listées.
Popup {
    id: root

    property int teamId: -1
    property string errorText: ""
    property var lineErrors: []
    signal imported(int count)

    function openFor(id) {
        teamId = id;
        listField.text = "";
        errorText = "";
        lineErrors = [];
        open();
    }

    function submit() {
        var r = backend.importPlayers(teamId, listField.text);
        if (r.ok) {
            close();
            imported(r.created);
            return;
        }
        errorText = r.error;
        lineErrors = r.errors;
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
        KeyboardDismissAll { popup: root }
    }
    onOpened: listField.focusField()

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
                text: "Importer des joueurs"
                font.pixelSize: 20
                font.weight: Font.Bold
                color: Theme.text
            }
            Text {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: 14
                color: Theme.textMuted
                text: "Un joueur par ligne. Seul le prénom est obligatoire ; ajoutez le reste si vous le souhaitez en séparant "
                      + "par « ; » : Prénom ; Nom ; Numéro ; Poste ; Naissance. Un même prénom et nom ne peut figurer qu'une fois."
            }
            FormField {
                id: listField
                objectName: "importList"
                width: parent.width
                label: "Liste"
                multiline: true
                multilineHeight: 200
                maxLength: 20000
                placeholder: "Lucas\nThéo;Martin\nHugo;Durand;10;Milieu;04/05/2012"
            }
            Text {
                objectName: "importError"
                visible: root.errorText !== ""
                width: parent.width
                text: root.errorText
                wrapMode: Text.Wrap
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: Theme.danger
            }
            Column {
                visible: root.lineErrors.length > 0
                width: parent.width
                spacing: 4
                Repeater {
                    model: Math.min(root.lineErrors.length, 8)
                    Text {
                        required property int index
                        objectName: "importLineError"
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 13
                        color: Theme.danger
                        text: "Ligne " + root.lineErrors[index].line + " : " + root.lineErrors[index].message
                    }
                }
                Text {
                    visible: root.lineErrors.length > 8
                    width: parent.width
                    font.pixelSize: 13
                    color: Theme.textMuted
                    text: "… et " + (root.lineErrors.length - 8) + " autre(s) ligne(s)."
                }
            }
            Row {
                width: parent.width
                spacing: 10
                layoutDirection: Qt.RightToLeft
                AppButton {
                    objectName: "importSubmit"
                    kind: "primary"
                    text: "Importer"
                    onClicked: root.submit()
                }
                AppButton { kind: "ghost"; text: "Annuler"; onClicked: root.close() }
            }
        }
    }
}
