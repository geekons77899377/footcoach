import QtQuick
import QtQuick.Controls
import "Status.js" as Status

// Page « Exercices » : bibliothèque (recherche, catégories, archivés) et fiche d'exercice. En grand écran, liste et
// fiche côte à côte ; sur téléphone, l'une après l'autre.
Item {
    id: page

    property var categories: []
    property var exercises: []
    property string query: ""
    property string categoryFilter: ""
    property bool showArchived: false
    property int currentId: -1
    property bool editing: false
    readonly property bool split: width >= 900
    readonly property int listWidth: 380
    // Aucun exercice ouvert et écran assez grand : bibliothèque en grille de cartes (bureau, tablette).
    readonly property bool gridMode: width >= 600 && height >= 520 && !editing
    readonly property int pad: gridMode ? Theme.gutter + 4 : Theme.gutter     // aligné sur le titre de l'en-tête
    readonly property int gridGap: 16
    readonly property int gridCols: Math.max(1, Math.floor((width - 2 * pad + gridGap) / (260 + gridGap)))   // cartes d'au moins 260 px
    readonly property var shown: {
        var q = query.trim().toLowerCase();
        var out = [];
        for (var i = 0; i < exercises.length; ++i) {
            var e = exercises[i];
            if (categoryFilter !== "" && e.category !== categoryFilter)
                continue;
            if (q !== "" && e.title.toLowerCase().indexOf(q) < 0 && e.objective.toLowerCase().indexOf(q) < 0)
                continue;
            out.push(e);
        }
        return out;
    }

    function refresh() {
        categories = backend.exerciseCategories();
        exercises = backend.exercises(showArchived);
    }

    function openExercise(e) {
        AppState.clearReturn();               // ouverture depuis la liste : « retour » revient à la liste
        currentId = e.id;
        form.openExercise(e);
        editing = true;
    }

    // Ouvre la fiche d'un exercice de la bibliothèque (depuis la séance du jour).
    function openById(id) {
        refresh();
        for (var i = 0; i < exercises.length; ++i) {
            if (exercises[i].id === id) {
                openExercise(exercises[i]);
                return;
            }
        }
        showArchived = true;                  // exercice archivé : il reste dans d'anciens plans
        refresh();
        for (var j = 0; j < exercises.length; ++j) {
            if (exercises[j].id === id) {
                openExercise(exercises[j]);
                break;
            }
        }
        showArchived = false;
    }

    // Bouton retour du téléphone : ferme la fiche ouverte. Retourne true si la page a utilisé le geste.
    function goBack() {
        if (!editing)
            return false;
        backFromForm();
        return true;
    }

    // Retour volontaire (flèche ou geste) : ferme la fiche, puis revient à la page d'origine si l'on vient d'ailleurs.
    function backFromForm() {
        closeForm();
        AppState.leaveItem("exercises");
    }

    function newExercise() {
        currentId = -1;
        form.openNew();
        editing = true;
    }

    function closeForm() {
        editing = false;
        currentId = -1;
    }

    function players(e) {
        if (e.playersMin > 0 && e.playersMax > 0)
            return e.playersMin === e.playersMax ? e.playersMin + " joueurs" : e.playersMin + "–" + e.playersMax + " joueurs";
        if (e.playersMin > 0)
            return "dès " + e.playersMin + " joueurs";
        if (e.playersMax > 0)
            return "jusqu'à " + e.playersMax + " joueurs";
        return "";
    }

    Connections {
        target: backend
        function onDataReplaced() {
            page.closeForm();
            page.refresh();
        }
    }

    onShowArchivedChanged: refresh()
    Component.onCompleted: refresh()
    onVisibleChanged: if (visible) refresh()

    PromptDialog {
        id: confirm
        onAccepted: {
            var r = backend.deleteExercise(page.currentId);
            if (r.ok) {
                page.closeForm();
                page.refresh();
                toast.show("Exercice supprimé");
            } else {
                toast.show(r.error);
            }
        }
    }

    // --- Liste ------------------------------------------------------------------------------------

    Item {
        id: listPane
        width: page.gridMode || !page.split ? page.width : page.listWidth
        height: page.height
        visible: page.split || !page.editing

        PageHeader {
            id: listHeader
            width: parent.width
            title: "Exercices"
            subtitle: page.exercises.length === 0 ? "Bibliothèque vide"
                      : page.exercises.length + (page.exercises.length > 1 ? " exercices" : " exercice")
            IconButton {
                iconName: "plus"
                iconColor: Theme.textOnAccent
                fillColor: Theme.accent
                diameter: 48
                glow: true
                accessibleName: "Nouvel exercice"
                onClicked: page.newExercise()
            }
        }

        Column {
            id: filters
            anchors.top: listHeader.bottom
            anchors.left: parent.left
            anchors.leftMargin: page.pad
            anchors.right: parent.right
            anchors.rightMargin: page.pad
            anchors.topMargin: 4
            spacing: 10

            Rectangle {
                width: page.gridMode ? Math.min(parent.width, 480) : parent.width
                height: 48
                radius: 24
                color: Theme.surface
                border.width: 1
                border.color: search.activeFocus ? Theme.accent : Theme.border
                Icon {
                    id: searchIcon
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    name: "search"
                    color: Theme.textMuted
                    size: 20
                }
                TextField {
                    id: search
                    anchors.left: searchIcon.right
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    placeholderText: "Titre ou objectif"
                    placeholderTextColor: Theme.textDim
                    color: Theme.text
                    font.pixelSize: 15
                    background: null
                    onTextChanged: page.query = text
                    Keys.onReturnPressed: focus = false
                }
            }
            ChipRow {
                width: parent.width
                value: page.categoryFilter
                options: page.categories
                onPicked: (id) => page.categoryFilter = id
            }
            Row {
                spacing: 10
                Switch {
                    checked: page.showArchived
                    onToggled: page.showArchived = checked
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "Afficher les exercices archivés"
                    font.pixelSize: 14
                    color: Theme.textMuted
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        ListView {
            id: list
            visible: !page.gridMode
            anchors.top: filters.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 10
            leftMargin: page.pad
            rightMargin: page.pad
            bottomMargin: 16
            spacing: 10
            clip: true
            model: page.shown
            boundsBehavior: Flickable.StopAtBounds

            delegate: Card {
                id: row
                required property var modelData
                width: list.width - list.leftMargin - list.rightMargin
                height: 78
                padding: 12
                highlighted: page.split && page.editing && page.currentId === modelData.id
                opacity: modelData.archived ? 0.55 : 1

                Rectangle {
                    id: badge
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 48
                    height: 48
                    radius: 14
                    color: Qt.rgba(Qt.color(Status.categoryColor(row.modelData.category)).r,
                                   Qt.color(Status.categoryColor(row.modelData.category)).g,
                                   Qt.color(Status.categoryColor(row.modelData.category)).b, 0.18)
                    Icon {
                        anchors.centerIn: parent
                        name: "dumbbell"
                        color: Status.categoryColor(row.modelData.category)
                        size: 26
                    }
                }
                Column {
                    anchors.left: badge.right
                    anchors.leftMargin: 14
                    anchors.right: chevron.left
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3
                    Text {
                        width: parent.width
                        text: row.modelData.title
                        elide: Text.ElideRight
                        font.pixelSize: 17
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        font.pixelSize: 13
                        color: Theme.textMuted
                        text: {
                            var parts = [row.modelData.categoryLabel];
                            if (row.modelData.durationMin > 0)
                                parts.push(row.modelData.durationMin + " min");
                            var p = page.players(row.modelData);
                            if (p !== "")
                                parts.push(p);
                            if (row.modelData.archived)
                                parts.push("archivé");
                            return parts.join(" · ");
                        }
                    }
                }
                Icon {
                    id: chevron
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    name: "chevron"
                    color: Theme.textDim
                    size: 20
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: page.openExercise(row.modelData)
                }
            }

            footer: emptyState
        }

        // Grille de cartes (grand écran, aucun exercice ouvert) : mêmes données et même filtre que la liste.
        GridView {
            id: grid
            objectName: "exerciseGrid"
            visible: page.gridMode
            anchors.top: filters.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 14
            leftMargin: page.pad
            rightMargin: page.pad - page.gridGap        // le dernier écart de chaque ligne est dans la cellule
            bottomMargin: 16
            clip: true
            model: page.shown
            boundsBehavior: Flickable.StopAtBounds
            cellWidth: (width - leftMargin - rightMargin) / page.gridCols
            cellHeight: 270 + page.gridGap
            footer: emptyState

            delegate: Item {
                id: cell
                required property var modelData
                readonly property color tint: Status.categoryColor(modelData.category)
                width: grid.cellWidth
                height: grid.cellHeight
                Card {
                    id: tile
                    objectName: "exerciseCard"
                    width: cell.width - page.gridGap
                    height: 270
                    padding: 0
                    radius: Theme.radiusLg
                    opacity: cell.modelData.archived ? 0.55 : 1

                    // Tête : pastille de catégorie sur un dégradé teinté.
                    Rectangle {
                        id: top
                        x: 1
                        y: 1
                        width: parent.width - 2
                        height: 92
                        radius: tile.radius - 1             // suit les coins arrondis de la carte
                        gradient: Gradient {
                            GradientStop { position: 0; color: Qt.rgba(cell.tint.r, cell.tint.g, cell.tint.b, 0.16) }
                            GradientStop { position: 1; color: "transparent" }
                        }
                        Rectangle {
                            anchors.centerIn: parent
                            width: 56
                            height: 56
                            radius: 28
                            color: Theme.bgDeep
                            border.width: 1
                            border.color: Theme.border
                            Icon {
                                anchors.centerIn: parent
                                name: "dumbbell"
                                color: cell.tint
                                size: 26
                            }
                        }
                    }
                    Column {
                        anchors.top: top.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        spacing: 6
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            font.letterSpacing: 1
                            color: cell.tint
                            text: (cell.modelData.categoryLabel + "").toUpperCase()
                                  + (cell.modelData.durationMin > 0 ? "  ·  " + cell.modelData.durationMin + " MIN" : "")
                                  + (cell.modelData.archived ? "  ·  ARCHIVÉ" : "")
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.Wrap
                            font.family: Theme.fontHeading
                            font.pixelSize: 19
                            font.weight: Font.Bold
                            color: Theme.text
                            text: cell.modelData.title
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.Wrap
                            font.pixelSize: 13
                            color: Theme.textMuted
                            text: cell.modelData.objective
                        }
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        anchors.bottomMargin: 46
                        height: 1
                        color: Theme.borderSubtle
                    }
                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 20
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 14
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: Theme.textMuted
                        text: page.players(cell.modelData) !== "" ? page.players(cell.modelData) : "Tous effectifs"
                    }
                    Icon {
                        anchors.right: parent.right
                        anchors.rightMargin: 18
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 12
                        name: "chevron"
                        color: Theme.accent
                        size: 20
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: page.openExercise(cell.modelData)
                    }
                }
            }
        }

        Component {
            id: emptyState
            Item {
                width: page.gridMode ? page.width - 2 * page.pad : list.width - 2 * page.pad
                height: visible ? 160 : 0
                visible: page.shown.length === 0
                Card {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    Column {
                        width: parent.width
                        spacing: 12
                        Icon { name: "dumbbell"; color: Theme.accent; size: 32 }
                        Text {
                            width: parent.width
                            text: page.exercises.length === 0 ? "Aucun exercice" : "Aucun résultat"
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.Wrap
                            font.pixelSize: 14
                            color: Theme.textMuted
                            text: page.exercises.length === 0 ? "Ajoutez vos exercices pour composer les séances."
                                  : "Aucun exercice ne correspond à la recherche ou au filtre."
                        }
                    }
                }
            }
        }
    }

    // --- Fiche ------------------------------------------------------------------------------------

    ExerciseForm {
        id: form
        categories: page.categories
        x: page.split ? page.listWidth : 0
        width: page.width - x
        height: page.height
        visible: page.editing
        showBack: !page.split
        onCloseRequested: page.backFromForm()
        onSaved: {
            page.currentId = form.exerciseId;
            page.refresh();
            if (!page.split)
                page.closeForm();
            toast.show("Exercice enregistré");
        }
        onArchiveRequested: {
            var r = backend.setExerciseArchived(page.currentId, !form.archived);
            if (r.ok) {
                page.refresh();
                page.closeForm();
                toast.show(form.archived ? "Exercice remis dans la bibliothèque" : "Exercice archivé");
            } else {
                toast.show(r.error);
            }
        }
        onDeleteRequested: {
            if (form.usage > 0) {
                toast.show("Utilisé dans " + form.usage + " séance(s) : archivez-le plutôt que de le supprimer.");
                return;
            }
            confirm.confirm("Supprimer l'exercice ?", "L'exercice sera supprimé définitivement.", "Supprimer", true);
        }
    }

    Item {
        visible: page.split && !page.editing && !page.gridMode
        x: page.listWidth
        width: page.width - x
        height: page.height
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.border }
        Column {
            anchors.centerIn: parent
            spacing: 12
            Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "dumbbell"; color: Theme.textDim; size: 56 }
            Text { text: "Choisissez un exercice ou ajoutez-en un"; font.pixelSize: 16; color: Theme.textMuted }
        }
    }

    Toast { id: toast }
}
