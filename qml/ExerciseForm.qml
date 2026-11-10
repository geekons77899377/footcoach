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
    property var activities: []            // échauffement, technique... [{id, text}]
    property var categories: []            // catégories définies par l'utilisateur (ex. U10-U11) : [{id, name, count}]

    property int boardId: 0                // terrain (schéma de référence) de l'exercice, 0 : aucun
    property string boardJson: ""

    signal closeRequested()
    signal openBoardRequested(int boardId)
    signal saved()
    signal categoryAdded(int id)          // la liste des catégories a changé : l'appelant la recharge
    signal archiveRequested()
    signal deleteRequested()

    function loadBoard() {
        var b = exerciseId > 0 ? backend.exerciseBoard(exerciseId) : ({});
        boardId = b.boardId > 0 ? b.boardId : 0;
        boardJson = b.json ? b.json : "";
    }

    function createBoard() {
        var r = backend.createExerciseBoard(exerciseId);
        if (!r.ok) {
            errorText = r.error;
            scroll.contentY = 0;
            return;
        }
        loadBoard();
        openBoardRequested(r.boardId);
    }

    // Pastilles de la rangée « Catégorie » : les catégories existantes puis « + Nouvelle ».
    readonly property var categoryOptions: {
        var out = [];
        for (var i = 0; i < categories.length; ++i)
            out.push({ id: String(categories[i].id), text: categories[i].name });
        out.push({ id: "new", text: "+ Nouvelle" });
        return out;
    }

    function createCategory(name) {
        var r = backend.addCategory(name);
        if (!r.ok) {
            errorText = r.error;
            scroll.contentY = 0;
            return;
        }
        errorText = "";
        categoryAdded(r.id);
        categoryRow.value = String(r.id);
    }

    function openNew() {
        exerciseId = -1;
        boardId = 0;
        boardJson = "";
        archived = false;
        usage = 0;
        errorText = "";
        titleField.text = "";
        activity.value = "technical";
        categoryRow.value = "";
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
        activity.value = e.category;
        categoryRow.value = e.categoryId > 0 ? String(e.categoryId) : "";
        objective.text = e.objective;
        duration.text = e.durationMin > 0 ? String(e.durationMin) : "";
        playersMin.text = e.playersMin > 0 ? String(e.playersMin) : "";
        playersMax.text = e.playersMax > 0 ? String(e.playersMax) : "";
        equipment.text = e.equipment;
        description.text = e.description;
        loadBoard();
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
            id: exerciseId, title: titleField.text,
            category: activity.value || "technical", categoryId: categoryRow.value === "" ? 0 : parseInt(categoryRow.value, 10),
            objective: objective.text, durationMin: d, playersMin: a, playersMax: b,
            equipment: equipment.text, description: description.text, archived: archived
        });
        if (r.ok) {
            errorText = "";
            exerciseId = r.id;
            loadBoard();
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

    EnsureVisible { flick: scroll }

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
                id: categoryRow
                width: parent.width
                label: "Catégorie"
                options: root.categoryOptions
                onPicked: (id) => {
                    if (id === "new") {
                        value = previous;          // « + Nouvelle » n'est pas une catégorie : on garde le choix actuel
                        newCategory.ask("Nouvelle catégorie", "", false, 40);
                    } else {
                        previous = id;
                    }
                }
                property string previous: ""
                onValueChanged: if (value !== "new") previous = value
            }
            ChipRow {
                id: activity
                width: parent.width
                label: "Activité"
                allowClear: false
                options: root.activities
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
            Column {
                visible: root.exerciseId > 0
                width: parent.width
                spacing: 8
                Text {
                    text: "Terrain"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: Theme.textMuted
                }
                BoardThumb {
                    objectName: "exerciseBoardThumb"
                    visible: root.boardId > 0
                    width: parent.width
                    height: Math.round(width * 0.62)
                    json: root.boardJson
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.openBoardRequested(root.boardId)
                    }
                }
                AppButton {
                    objectName: root.boardId > 0 ? "openExerciseBoard" : "createExerciseBoard"
                    width: parent.width
                    kind: "outline"
                    text: root.boardId > 0 ? "Ouvrir le terrain" : "Créer le terrain"
                    onClicked: root.boardId > 0 ? root.openBoardRequested(root.boardId) : root.createBoard()
                }
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

    PromptDialog {
        id: newCategory
        onAccepted: (text) => root.createCategory(text)
    }
}
