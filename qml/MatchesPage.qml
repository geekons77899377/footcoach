import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil
import "Status.js" as Status

// Page « Matchs » : matchs de l'équipe (à venir / passés) et fiche du match choisi (informations, convoqués, schéma
// tactique). En grand écran, liste et fiche côte à côte ; sur téléphone, l'une après l'autre.
Item {
    id: page

    property string scope: "upcoming"          // upcoming | past
    property var rows: []
    property int currentId: -1
    property bool editing: false
    readonly property bool split: width >= 900
    readonly property int listWidth: 400
    readonly property int teamId: AppState.teamId

    function refresh() {
        switcher.load();
        refreshRows();
    }

    function refreshRows() {
        var out = [];
        if (teamId > 0) {
            var today = DateUtil.todayIso();
            if (scope === "upcoming") {
                out = backend.matches(teamId, today, DateUtil.addDays(today, 365));
            } else {
                out = backend.matches(teamId, DateUtil.addDays(today, -365), DateUtil.addDays(today, -1));
                out.reverse();
            }
        }
        rows = out;
    }

    function teamName() { return switcher.current ? switcher.current.name : ""; }

    function openMatch(id) {
        editor.guardLeave(function () {
            AppState.clearReturn();
            currentId = id;
            editor.openMatch(id, teamName());
            editing = true;
        });
    }

    // Ouvre un match depuis une autre page (calendrier) : l'équipe du match devient l'équipe courante.
    function showMatch(m) {
        AppState.teamId = m.teamId;
        switcher.load();
        openMatch(m.id);
    }

    function newMatch(day) {
        editor.guardLeave(function () {
            AppState.clearReturn();
            currentId = -1;
            editor.openNew(teamId, teamName(), day || DateUtil.todayIso());
            editing = true;
        });
    }

    function closeEditor() { closeAndReturn(false); }

    function closeAndReturn(back) {
        editor.guardLeave(function () {
            editor.leave(false);
            editing = false;
            currentId = -1;
            if (back)
                AppState.leaveItem("matches");
        });
    }

    function guardLeave(action) { editor.guardLeave(action); }
    function flushBoard() { return editor.flushBoard(); }
    function showSaveFailed(action) { editor.showSaveFailed(action); }
    // Fermeture de la fenêtre : vrai s'il reste des modifications de la fiche non enregistrées.
    function hasUnsaved() { return editing && editor.hasUnsaved(); }

    function backFromEditor() { closeAndReturn(true); }

    function goBack() {
        if (!editing)
            return false;
        if (editor.goBack())
            return true;
        backFromEditor();
        return true;
    }

    Connections {
        target: backend
        function onDataReplaced() {
            editor.leave(true);
            editor.savedState = editor.formState();      // les données ont été remplacées : rien à conserver
            page.editing = false;
            page.currentId = -1;
            page.refresh();
        }
    }

    onScopeChanged: { closeEditor(); refreshRows(); }
    Component.onCompleted: refresh()
    onVisibleChanged: if (visible) refresh()

    Item {
        id: listPane
        width: page.split ? page.listWidth : page.width
        height: page.height
        visible: page.split || !page.editing

        PageHeader {
            id: listHeader
            width: parent.width
            title: "Matchs"
            subtitle: switcher.current === null ? "Aucune équipe"
                      : switcher.current.name + " · " + page.rows.length
                        + (page.rows.length > 1 ? " matchs" : " match")
            TeamSwitcher {
                id: switcher
                beforeChange: (apply) => page.guardLeave(apply)
                onChanged: { page.closeEditor(); page.refreshRows(); }
            }
            IconButton {
                objectName: "newMatch"
                visible: page.teamId > 0
                iconName: "plus"
                iconColor: Theme.textOnAccent
                fillColor: Theme.accent
                diameter: 48
                glow: true
                accessibleName: "Nouveau match"
                onClicked: page.newMatch("")
            }
        }

        ChipRow {
            id: scopeChips
            visible: page.teamId > 0
            anchors.top: listHeader.bottom
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.leftMargin: Theme.gutter
            anchors.right: parent.right
            anchors.rightMargin: Theme.gutter
            allowClear: false
            value: page.scope
            options: [{ id: "upcoming", text: "À venir" }, { id: "past", text: "Passés" }]
            onPicked: (id) => page.guardLeave(function () { page.scope = id; })
        }

        ListView {
            id: list
            objectName: "matchList"
            anchors.top: scopeChips.visible ? scopeChips.bottom : listHeader.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 10
            leftMargin: Theme.gutter
            rightMargin: Theme.gutter
            bottomMargin: 16
            spacing: 10
            clip: true
            model: page.rows
            boundsBehavior: Flickable.StopAtBounds

            delegate: Card {
                id: row
                required property var modelData
                readonly property var st: Status.match(modelData.status)
                width: list.width - list.leftMargin - list.rightMargin
                height: 92
                padding: 12
                highlighted: page.split && page.editing && page.currentId === modelData.id
                opacity: modelData.status === "cancelled" ? 0.6 : 1

                Column {
                    id: when
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 58
                    spacing: 2
                    Text { text: row.modelData.time; font.pixelSize: 18; font.weight: Font.ExtraBold; color: Theme.text }
                    Text { text: row.modelData.homeAway === "home" ? "Dom." : "Ext."; font.pixelSize: 12; color: Theme.textMuted }
                }
                Rectangle {
                    id: sep
                    anchors.left: when.right
                    anchors.leftMargin: 6
                    height: parent.height
                    width: 3
                    radius: 1.5
                    color: Status.MATCH_COLOR
                }
                Column {
                    anchors.left: sep.right
                    anchors.leftMargin: 12
                    anchors.right: chevron.left
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3
                    Text {
                        width: parent.width
                        text: DateUtil.longLabel(row.modelData.date)
                        elide: Text.ElideRight
                        font.pixelSize: 14
                        color: Theme.textMuted
                    }
                    Text {
                        width: parent.width
                        text: "contre " + row.modelData.opponent
                        elide: Text.ElideRight
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: row.st.color
                        text: row.st.label
                              + (row.modelData.scoreFor >= 0 ? " · " + row.modelData.scoreFor + " – " + row.modelData.scoreAgainst : "")
                              + " · " + row.modelData.convoked + (row.modelData.convoked > 1 ? " convoqués" : " convoqué")
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
                    onClicked: page.openMatch(row.modelData.id)
                }
            }

            footer: Item {
                width: list.width - 2 * Theme.gutter
                height: visible ? 160 : 0
                visible: page.rows.length === 0
                Card {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    Column {
                        width: parent.width
                        spacing: 12
                        Icon { name: "board"; color: Theme.accent; size: 32 }
                        Text {
                            objectName: "matchEmpty"
                            width: parent.width
                            text: page.teamId <= 0 ? "Aucune équipe"
                                  : page.scope === "upcoming" ? "Aucun match à venir" : "Aucun match passé"
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.Wrap
                            font.pixelSize: 14
                            color: Theme.textMuted
                            text: page.teamId <= 0 ? "Créez une équipe dans Effectif, puis ajoutez vos matchs ici."
                                  : "Ajoutez un match avec le bouton +, puis convoquez vos joueurs."
                        }
                    }
                }
            }
        }
    }

    MatchEditor {
        id: editor
        x: page.split ? page.listWidth : 0
        width: page.width - x
        height: page.height
        visible: page.editing
        showBack: !page.split
        onCloseRequested: page.backFromEditor()
        onChanged: page.refreshRows()
    }

    Item {
        visible: page.split && !page.editing
        x: page.listWidth
        width: page.width - x
        height: page.height
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.border }
        Column {
            anchors.centerIn: parent
            spacing: 12
            Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "board"; color: Theme.textDim; size: 56 }
            Text { text: "Choisissez un match ou ajoutez-en un"; font.pixelSize: 16; color: Theme.textMuted }
        }
    }
}
