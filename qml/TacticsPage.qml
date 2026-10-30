import QtQuick
import QtQuick.Controls

// Page « Tactique » : liste des schémas puis éditeur. En grand écran, les deux côte à côte ; sur téléphone,
// l'un après l'autre (le bouton retour revient à la liste).
Item {
    id: page

    property var boards: []
    property int currentId: -1
    readonly property bool split: width >= 900
    readonly property int listWidth: 360

    function refresh() {
        boards = backend.boards();
    }

    function openBoard(id) {
        refresh();
        currentId = id;
        editor.open(id);
    }

    function createBoard() {
        var id = backend.createBoard("");
        if (id > 0) {
            refresh();
            openBoard(id);
        } else {
            errorToast.show("Impossible de créer le schéma : " + backend.lastError);
        }
    }

    function formatDate(sqlUtc) {
        if (!sqlUtc || sqlUtc.length < 10)
            return "";
        return sqlUtc.substring(8, 10) + "/" + sqlUtc.substring(5, 7) + "/" + sqlUtc.substring(0, 4);
    }

    function closeEditor() {
        editor.flush();
        currentId = -1;
        refresh();
    }

    Connections {
        target: backend
        function onDataReplaced() {
            editor.discard();
            page.currentId = -1;
            page.refresh();
        }
    }

    Component.onCompleted: refresh()
    // La liste peut avoir changé depuis une autre page (accueil, import…) : on la relit à chaque affichage.
    onVisibleChanged: if (visible) refresh()

    // Confirmation de suppression.
    property int pendingDelete: -1
    PromptDialog {
        id: confirm
        onAccepted: {
            if (page.pendingDelete > 0 && backend.deleteBoard(page.pendingDelete)) {
                if (page.currentId === page.pendingDelete)
                    page.currentId = -1;
                page.refresh();
            }
            page.pendingDelete = -1;
        }
    }

    PromptDialog {
        id: renameDialog
        property int targetId: -1
        onAccepted: (text) => {
            if (text.trim() !== "" && backend.renameBoard(targetId, text))
                page.refresh();
        }
    }

    ActionSheet {
        id: rowSheet
        property int targetId: -1
        property string targetTitle: ""
        title: targetTitle
        model: [
            { id: "rename", text: "Renommer", icon: "edit" },
            { id: "duplicate", text: "Dupliquer", icon: "copy" },
            { id: "delete", text: "Supprimer", icon: "trash", danger: true }
        ]
        onChosen: (id) => {
            if (id === "rename") {
                renameDialog.targetId = targetId;
                renameDialog.ask("Titre du schéma", targetTitle, false, 80);
            } else if (id === "duplicate") {
                var copy = backend.duplicateBoard(targetId);
                page.refresh();
                if (copy > 0)
                    page.openBoard(copy);
            } else if (id === "delete") {
                page.pendingDelete = targetId;
                confirm.confirm("Supprimer le schéma ?", "« " + targetTitle + " » sera supprimé définitivement.",
                                "Supprimer", true);
            }
        }
    }

    // --- Liste ------------------------------------------------------------------------------------

    Item {
        id: listPane
        x: 0
        width: page.split ? page.listWidth : page.width
        height: page.height
        visible: page.split || page.currentId < 0

        PageHeader {
            id: listHeader
            width: parent.width
            title: "Tactique"
            subtitle: page.boards.length === 0 ? "Aucun schéma"
                      : page.boards.length === 1 ? "1 schéma" : page.boards.length + " schémas"
            IconButton {
                iconName: "plus"
                iconColor: Theme.textOnAccent
                fillColor: Theme.accent
                diameter: 48
                glow: true
                accessibleName: "Nouveau schéma"
                onClicked: page.createBoard()
            }
        }

        ListView {
            id: list
            anchors.top: listHeader.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 8
            leftMargin: Theme.gutter
            rightMargin: Theme.gutter
            bottomMargin: 16
            spacing: 12
            clip: true
            model: page.boards
            boundsBehavior: Flickable.StopAtBounds

            delegate: Card {
                id: row
                required property var modelData
                readonly property bool current: page.split && page.currentId === modelData.id
                width: list.width - list.leftMargin - list.rightMargin
                height: 88
                padding: 12
                highlighted: current

                BoardThumb {
                    id: thumb
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    json: row.modelData.json || ""
                }
                Column {
                    anchors.left: thumb.right
                    anchors.leftMargin: 14
                    anchors.right: moreButton.left
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
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
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: row.modelData.readable ? Theme.textMuted : Theme.warn
                        text: row.modelData.readable
                              ? (row.modelData.itemCount + (row.modelData.itemCount > 1 ? " éléments" : " élément")
                                 + " · modifié le " + page.formatDate(row.modelData.updatedAt))
                              : "Contenu illisible"
                    }
                }
                IconButton {
                    id: moreButton
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    iconName: "more"
                    iconColor: Theme.textMuted
                    accessibleName: "Actions du schéma " + row.modelData.title
                    onClicked: {
                        rowSheet.targetId = row.modelData.id;
                        rowSheet.targetTitle = row.modelData.title;
                        rowSheet.open();
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 52
                    onClicked: page.openBoard(row.modelData.id)
                }
            }

            footer: Item {
                width: list.width - 2 * Theme.gutter       // le ListView applique déjà les marges latérales
                height: page.boards.length === 0 ? 220 : 0
                visible: page.boards.length === 0
                Card {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    Column {
                        width: parent.width
                        spacing: 12
                        Icon {
                            name: "board"
                            color: Theme.accent
                            size: 32
                        }
                        Text {
                            width: parent.width
                            text: "Votre premier schéma"
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.Wrap
                            font.pixelSize: 14
                            color: Theme.textMuted
                            text: "Placez joueurs, plots et flèches sur un terrain entier ou un demi-terrain, "
                                  + "avec zoom pour être précis."
                        }
                        AppButton {
                            kind: "primary"
                            text: "Créer un schéma"
                            iconName: "plus"
                            onClicked: page.createBoard()
                        }
                    }
                }
            }
        }
    }

    // --- Éditeur ----------------------------------------------------------------------------------

    BoardEditor {
        id: editor
        x: page.split ? page.listWidth : 0
        width: page.width - x
        height: page.height
        visible: page.currentId >= 0
        showBack: !page.split
        onCloseRequested: page.closeEditor()
        onChanged: page.refresh()
        onDuplicateRequested: {
            var copy = backend.duplicateBoard(page.currentId);
            page.refresh();
            if (copy > 0)
                page.openBoard(copy);
        }
        onDeleteRequested: {
            page.pendingDelete = page.currentId;
            confirm.confirm("Supprimer le schéma ?", "« " + editor.title + " » sera supprimé définitivement.",
                            "Supprimer", true);
        }
    }

    // Grand écran sans schéma ouvert.
    Item {
        visible: page.split && page.currentId < 0
        x: page.listWidth
        width: page.width - x
        height: page.height
        Rectangle {
            anchors.left: parent.left
            width: 1
            height: parent.height
            color: Theme.border
        }
        Column {
            anchors.centerIn: parent
            spacing: 12
            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: "board"
                color: Theme.textDim
                size: 56
            }
            Text {
                text: "Choisissez un schéma ou créez-en un"
                font.pixelSize: 16
                color: Theme.textMuted
            }
        }
    }

    Rectangle {
        id: errorToast
        property string message: ""
        function show(text) {
            message = text;
            opacity = 1;
            errorTimer.restart();
        }
        opacity: 0
        visible: opacity > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        width: Math.min(parent.width - 32, errorText.implicitWidth + 36)
        height: errorText.implicitHeight + 24
        radius: 16
        color: Theme.surfaceHi
        border.width: 1
        border.color: Theme.danger
        Text {
            id: errorText
            anchors.centerIn: parent
            width: parent.width - 28
            wrapMode: Text.Wrap
            text: errorToast.message
            font.pixelSize: 14
            color: Theme.text
        }
        Timer {
            id: errorTimer
            interval: 4000
            onTriggered: errorToast.opacity = 0
        }
    }
}
