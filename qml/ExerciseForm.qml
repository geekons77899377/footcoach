import QtQuick
import QtQuick.Controls

// Fiche d'un exercice de la bibliothèque : création ou modification. Les erreurs de validation viennent du backend.
Item {
    id: root

    property int exerciseId: -1            // -1 : nouvel exercice
    property bool archived: false
    property int usage: 0                  // nombre de séances qui l'utilisent
    property bool showBack: true
    property string errorText: ""
    property var categories: []

    signal closeRequested()
    signal saved()
    signal archiveRequested()
    signal deleteRequested()

    function openNew() {
        exerciseId = -1;
        archived = false;
        usage = 0;
        errorText = "";
        titleField.text = "";
        category.value = "technical";
        objective.text = "";
        duration.text = "";
        playersMin.text = "";
        playersMax.text = "";
        equipment.text = "";
        description.text = "";
        scroll.contentY = 0;
    }

    function openExercise(e) {
        exerciseId = e.id;
        archived = e.archived;
        usage = e.usage;
        errorText = "";
        titleField.text = e.title;
        category.value = e.category;
        objective.text = e.objective;
        duration.text = e.durationMin > 0 ? String(e.durationMin) : "";
        playersMin.text = e.playersMin > 0 ? String(e.playersMin) : "";
        playersMax.text = e.playersMax > 0 ? String(e.playersMax) : "";
        equipment.text = e.equipment;
        description.text = e.description;
        scroll.contentY = 0;
    }

    function number(text) {
        var t = text.trim();
        return t === "" ? 0 : parseInt(t, 10);
    }

    function save() {
        var d = number(duration.text), a = number(playersMin.text), b = number(playersMax.text);
        if (isNaN(d) || isNaN(a) || isNaN(b)) {
            errorText = "La durée et le nombre de joueurs doivent être des nombres.";
            scroll.contentY = 0;
            return;
        }
        var r = backend.saveExercise({
            id: exerciseId, title: titleField.text, category: category.value || "technical",
            objective: objective.text, durationMin: d, playersMin: a, playersMax: b,
            equipment: equipment.text, description: description.text, archived: archived
        });
        if (r.ok) {
            errorText = "";
            exerciseId = r.id;
            saved();
        } else {
            errorText = r.error;
            scroll.contentY = 0;
        }
    }

    PageHeader {
        id: header
        width: parent.width
        title: root.exerciseId > 0 ? "Exercice" : "Nouvel exercice"
        leftInset: root.showBack ? 40 : 0
        IconButton {
            visible: root.exerciseId > 0
            iconName: "more"
            accessibleName: "Actions de l'exercice"
            onClicked: moreSheet.open()
        }
        IconButton {
            iconName: "check"
            iconColor: Theme.textOnAccent
            fillColor: Theme.accent
            diameter: 48
            glow: true
            accessibleName: "Enregistrer"
            onClicked: root.save()
        }
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

    ActionSheet {
        id: moreSheet
        title: "Exercice"
        model: [
            { id: "archive", text: root.archived ? "Remettre dans la bibliothèque" : "Archiver", icon: "archive" },
            { id: "delete", text: "Supprimer définitivement", icon: "trash", danger: true }
        ]
        onChosen: (id) => id === "archive" ? root.archiveRequested() : root.deleteRequested()
    }

    Flickable {
        id: scroll
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        contentWidth: width
        contentHeight: form.height + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        Column {
            id: form
            x: Theme.gutter
            y: 8
            width: Math.min(scroll.width - 2 * Theme.gutter, 560)
            spacing: 16

            Text {
                visible: root.errorText !== ""
                width: parent.width
                text: root.errorText
                wrapMode: Text.Wrap
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: Theme.danger
            }
            Text {
                visible: root.archived
                width: parent.width
                text: "Exercice archivé : il n'apparaît plus dans la bibliothèque ni dans les choix pour un plan."
                wrapMode: Text.Wrap
                font.pixelSize: 13
                color: Theme.warn
            }
            Text {
                visible: root.usage > 0
                width: parent.width
                text: "Utilisé dans " + root.usage + (root.usage > 1 ? " séances" : " séance")
                      + " : les modifications s'y appliquent aussi."
                wrapMode: Text.Wrap
                font.pixelSize: 13
                color: Theme.textMuted
            }
            FormField { id: titleField; width: parent.width; label: "Titre"; required: true; maxLength: 100 }
            ChipRow {
                id: category
                width: parent.width
                label: "Catégorie"
                allowClear: false
                options: root.categories
            }
            FormField {
                id: objective
                width: parent.width
                label: "Objectif"
                placeholder: "Ex. Améliorer la qualité de passe sous pression"
                maxLength: 200
            }
            Row {
                width: parent.width
                spacing: 12
                FormField {
                    id: duration
                    width: (parent.width - 24) / 3
                    label: "Durée (min)"
                    maxLength: 3
                    inputHints: Qt.ImhDigitsOnly
                }
                FormField {
                    id: playersMin
                    width: (parent.width - 24) / 3
                    label: "Joueurs min."
                    maxLength: 2
                    inputHints: Qt.ImhDigitsOnly
                }
                FormField {
                    id: playersMax
                    width: (parent.width - 24) / 3
                    label: "Joueurs max."
                    maxLength: 2
                    inputHints: Qt.ImhDigitsOnly
                }
            }
            FormField {
                id: equipment
                width: parent.width
                label: "Matériel"
                placeholder: "Ballons, plots, chasubles (séparés par des virgules)"
                multiline: true
                maxLength: 300
            }
            FormField {
                id: description
                width: parent.width
                label: "Description"
                placeholder: "Organisation, consignes, variantes…"
                multiline: true
                maxLength: 2000
            }
            AppButton {
                width: parent.width
                kind: "primary"
                text: "Enregistrer"
                iconName: "check"
                onClicked: root.save()
            }
        }
    }
}
