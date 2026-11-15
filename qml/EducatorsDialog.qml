import QtQuick
import QtQuick.Controls

// Éducateurs d'un club (schéma 12) : liste, ajout, modification, suppression. Un éducateur peut être le responsable de
// plusieurs équipes du club ; le responsable d'une équipe se choisit dans « Modifier l'équipe » et s'imprime sur la
// feuille de challenge (« Nom Responsable », « N° de licence »).
Popup {
    id: root
    objectName: "educatorsDialog"

    property int clubId: -1
    property string clubName: ""
    property var educators: []
    property int editingId: -1              // -1 : nouvel éducateur
    property string errorText: ""
    property real navInset: AppState.bottomInset
    signal changed()

    function openFor(club, name) {
        clubId = club;
        clubName = name || "";
        reload();
        resetForm();
        open();
    }
    // Même règle que la feuille de challenge : 10 chiffres, espaces ignorés.
    function licenceOk(text) {
        return /^[0-9]{10}$/.test(String(text).replace(/\s+/g, ""));
    }
    function reload() {
        educators = clubId > 0 ? backend.educators(clubId) : [];
    }
    function resetForm() {
        editingId = -1;
        firstField.text = "";
        lastField.text = "";
        licenceField.text = "";
        errorText = "";
    }
    function edit(e) {
        editingId = e.id;
        firstField.text = e.firstName;
        lastField.text = e.lastName;
        licenceField.text = e.licence;
        errorText = "";
    }
    function save() {
        var r = backend.saveEducator({ id: editingId, clubId: clubId, firstName: firstField.text,
                                       lastName: lastField.text, licence: licenceField.text });
        if (!r.ok) {
            errorText = r.error;
            return;
        }
        reload();
        resetForm();
        changed();
    }

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 16
    width: Math.min(parent ? parent.width - 24 : 360, 520)
    height: parent ? Math.min(parent.height - navInset - 48, 720) : 560
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? Math.max(12, parent.height - height - 12 - navInset) : 0
    closePolicy: Popup.CloseOnEscape
    Overlay.modal: Rectangle { color: "#99000000" }
    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
        KeyboardDismissAll { popup: root }
    }

    contentItem: Flickable {
        id: flick
        clip: true
        contentWidth: width
        contentHeight: body.implicitHeight
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: body
            width: flick.width
            spacing: 12

            Item {
                width: parent.width
                height: 40
                Column {
                    anchors.left: parent.left
                    anchors.right: closeButton.left
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text: "ÉDUCATEURS DU CLUB"
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        font.letterSpacing: 1.2
                        color: Theme.textMuted
                    }
                    Text {
                        width: parent.width
                        text: root.clubName
                        elide: Text.ElideRight
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                }
                IconButton {
                    id: closeButton
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    iconName: "close"
                    accessibleName: "Fermer"
                    onClicked: root.close()
                }
            }

            Text {
                visible: root.educators.length === 0
                width: parent.width
                text: "Aucun éducateur pour l'instant. Ajoutez-en un ci-dessous, puis choisissez-le comme responsable "
                      + "dans « Modifier l'équipe »."
                wrapMode: Text.Wrap
                font.pixelSize: 14
                color: Theme.textMuted
            }

            Repeater {
                model: root.educators
                delegate: Rectangle {
                    id: row
                    required property var modelData
                    width: body.width
                    height: 64
                    radius: Theme.radiusSm
                    color: root.editingId === modelData.id ? Theme.accentSoft : Theme.bg
                    border.width: 1
                    border.color: root.editingId === modelData.id ? Theme.accent : Theme.border
                    Accessible.role: Accessible.Button
                    Accessible.name: "Modifier " + modelData.fullName
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.edit(row.modelData)
                    }
                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        anchors.right: trash.left
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2
                        Text {
                            width: parent.width
                            text: row.modelData.fullName
                            elide: Text.ElideRight
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: Theme.text
                        }
                        Text {
                            width: parent.width
                            text: (row.modelData.licence
                                   ? "Licence " + row.modelData.licence
                                     + (root.licenceOk(row.modelData.licence) ? "" : " (10 chiffres attendus)")
                                   : "Licence non renseignée")
                                  + (row.modelData.teamCount > 0
                                     ? " · responsable de " + row.modelData.teamCount
                                       + (row.modelData.teamCount > 1 ? " équipes" : " équipe") : "")
                            elide: Text.ElideRight
                            font.pixelSize: 13
                            color: Theme.textMuted
                        }
                    }
                    IconButton {
                        id: trash
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        iconName: "trash"
                        iconColor: Theme.danger
                        accessibleName: "Supprimer " + row.modelData.fullName
                        onClicked: {
                            confirm.educatorId = row.modelData.id;
                            confirm.confirm("Supprimer l'éducateur ?",
                                            row.modelData.fullName + " sera retiré de la liste du club."
                                            + (row.modelData.teamCount > 0
                                               ? " Ses équipes n'auront plus de responsable sur la feuille de challenge." : ""),
                                            "Supprimer", true);
                        }
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: Theme.border }

            Text {
                text: root.editingId > 0 ? "Modifier l'éducateur" : "Nouvel éducateur"
                font.pixelSize: 16
                font.weight: Font.Bold
                color: Theme.text
            }
            FormField { id: firstField; objectName: "educatorFirstName"; width: parent.width; label: "Prénom"; required: true; maxLength: 60 }
            FormField { id: lastField; objectName: "educatorLastName"; width: parent.width; label: "Nom"; required: true; maxLength: 60 }
            FormField {
                id: licenceField
                objectName: "educatorLicence"
                width: parent.width
                label: "N° de licence"
                placeholder: "10 chiffres (facultatif)"
                maxLength: 20
                inputHints: Qt.ImhPreferNumbers | Qt.ImhNoPredictiveText
            }
            Text {
                objectName: "educatorLicenceWarning"
                visible: licenceField.text.trim() !== "" && !root.licenceOk(licenceField.text)
                width: parent.width
                text: "Le n° de licence a d'habitude 10 chiffres. Il sera imprimé tel quel sur la feuille de challenge."
                wrapMode: Text.Wrap
                font.pixelSize: 13
                color: Theme.warn
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
                    objectName: "educatorSave"
                    kind: "primary"
                    text: root.editingId > 0 ? "Enregistrer" : "Ajouter"
                    onClicked: root.save()
                }
                AppButton {
                    visible: root.editingId > 0
                    kind: "ghost"
                    text: "Annuler"
                    onClicked: root.resetForm()
                }
            }
            Item { width: 1; height: 8 }
        }
    }

    PromptDialog {
        id: confirm
        objectName: "educatorConfirm"
        property int educatorId: -1
        fieldVisible: false
        onAccepted: {
            var r = backend.deleteEducator(educatorId);
            if (!r.ok) {
                root.errorText = r.error;
                return;
            }
            if (root.editingId === educatorId)
                root.resetForm();
            root.reload();
            root.changed();
        }
    }
}
