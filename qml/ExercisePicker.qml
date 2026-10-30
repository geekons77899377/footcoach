import QtQuick
import QtQuick.Controls
import "Status.js" as Status

// Choix d'un exercice de la bibliothèque (hors archivés) pour l'ajouter au plan : recherche et catégories.
Popup {
    id: root

    property var exercises: []
    property var categories: []
    property string query: ""
    property string categoryFilter: ""
    property real navInset: AppState.bottomInset
    signal picked(var exercise)

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

    function pick() {
        exercises = backend.exercises(false);
        categories = backend.exerciseCategories();
        query = "";
        categoryFilter = "";
        search.text = "";
        open();
    }

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 14
    width: Math.min(parent ? parent.width - 24 : 360, 520)
    height: parent ? Math.min(parent.height - navInset - 24, 640) : 480
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? parent.height - height - 12 - navInset : 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle { color: "#99000000" }
    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    contentItem: Item {
        Text {
            id: title
            width: parent.width
            topPadding: 2
            text: "AJOUTER UN EXERCICE"
            font.pixelSize: 12
            font.weight: Font.Bold
            font.letterSpacing: 1.2
            color: Theme.textMuted
        }
        Rectangle {
            id: searchBox
            anchors.top: title.bottom
            anchors.topMargin: 8
            width: parent.width
            height: 46
            radius: 23
            color: Theme.bg
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
                onTextChanged: root.query = text
                Keys.onReturnPressed: focus = false
            }
        }
        ChipRow {
            id: chips
            anchors.top: searchBox.bottom
            anchors.topMargin: 10
            width: parent.width
            value: root.categoryFilter
            options: root.categories
            onPicked: (id) => root.categoryFilter = id
        }
        ListView {
            id: list
            anchors.top: chips.bottom
            anchors.topMargin: 10
            anchors.bottom: parent.bottom
            width: parent.width
            clip: true
            spacing: 8
            model: root.shown
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                id: row
                required property var modelData
                width: list.width
                height: 64
                radius: 16
                color: area.pressed ? Theme.surfaceHi : Theme.bg
                border.width: 1
                border.color: Theme.border

                Rectangle {
                    id: dot
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    width: 10
                    height: 10
                    radius: 5
                    color: Status.categoryColor(row.modelData.category)
                }
                Column {
                    anchors.left: dot.right
                    anchors.leftMargin: 12
                    anchors.right: plus.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Text {
                        width: parent.width
                        text: row.modelData.title
                        elide: Text.ElideRight
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        font.pixelSize: 12
                        color: Theme.textMuted
                        text: row.modelData.categoryLabel
                              + (row.modelData.durationMin > 0 ? " · " + row.modelData.durationMin + " min" : "")
                    }
                }
                Icon {
                    id: plus
                    anchors.right: parent.right
                    anchors.rightMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    name: "plus"
                    color: Theme.accent
                    size: 22
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    onClicked: {
                        root.close();
                        root.picked(row.modelData);
                    }
                }
            }

            footer: Item {
                width: list.width
                height: visible ? 80 : 0
                visible: root.shown.length === 0
                Text {
                    anchors.centerIn: parent
                    text: root.exercises.length === 0 ? "Bibliothèque vide : créez d'abord des exercices."
                                                      : "Aucun exercice ne correspond."
                    font.pixelSize: 14
                    color: Theme.textMuted
                }
            }
        }
    }
}
