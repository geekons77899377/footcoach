import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil
import "Status.js" as Status

// Page « Séances » : séances de l'équipe (à venir / passées) avec le résumé de leur plan, et le plan de la séance
// choisie. En grand écran, liste et plan côte à côte ; sur téléphone, l'un après l'autre.
Item {
    id: page

    property string scope: "upcoming"          // upcoming | past
    property var rows: []                      // séances + résumé du plan
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
            var list;
            if (scope === "upcoming") {
                list = backend.sessions(teamId, today, DateUtil.addDays(today, 365));
            } else {
                list = backend.sessions(teamId, DateUtil.addDays(today, -365), DateUtil.addDays(today, -1));
                list.reverse();
            }
            for (var i = 0; i < list.length; ++i) {
                var s = list[i];
                var p = backend.plan(s.id);
                s.exerciseCount = p.ok ? p.entries.length : 0;
                s.planMin = p.ok ? p.totalMin : 0;
                out.push(s);
            }
        }
        rows = out;
    }

    function openSession(s) {
        currentId = s.id;
        editor.openSession(s, switcher.current ? switcher.current.name : "");
        editing = true;
    }

    function closeEditor() {
        editing = false;
        currentId = -1;
    }

    Connections {
        target: backend
        function onDataReplaced() {
            page.closeEditor();
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
            title: "Séances"
            subtitle: switcher.current === null ? "Aucune équipe"
                      : switcher.current.name + " · " + page.rows.length
                        + (page.rows.length > 1 ? " séances" : " séance")
            TeamSwitcher {
                id: switcher
                onChanged: { page.closeEditor(); page.refreshRows(); }
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
            options: [{ id: "upcoming", text: "À venir" }, { id: "past", text: "Passées" }]
            onPicked: (id) => page.scope = id
        }

        ListView {
            id: list
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
                readonly property var st: Status.session(modelData.status)
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
                    Text { text: "→ " + row.modelData.endTime; font.pixelSize: 12; color: Theme.textMuted }
                }
                Rectangle {
                    id: sep
                    anchors.left: when.right
                    anchors.leftMargin: 6
                    height: parent.height
                    width: 3
                    radius: 1.5
                    color: row.st.color
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
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        text: row.modelData.theme !== "" ? row.modelData.theme : "Sans thème"
                        elide: Text.ElideRight
                        font.pixelSize: 13
                        color: Theme.textMuted
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        readonly property int diff: row.modelData.planMin - row.modelData.durationMin
                        color: row.modelData.exerciseCount === 0 ? Theme.textDim
                               : diff > 0 ? Theme.warn : Theme.accent
                        text: row.modelData.exerciseCount === 0 ? "Plan vide"
                              : row.modelData.exerciseCount + (row.modelData.exerciseCount > 1 ? " exercices" : " exercice")
                                + " · " + row.modelData.planMin + " / " + row.modelData.durationMin + " min"
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
                    onClicked: page.openSession(row.modelData)
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
                        Icon { name: "clipboard"; color: Theme.accent; size: 32 }
                        Text {
                            width: parent.width
                            text: page.teamId <= 0 ? "Aucune équipe"
                                  : page.scope === "upcoming" ? "Aucune séance à venir" : "Aucune séance passée"
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.Wrap
                            font.pixelSize: 14
                            color: Theme.textMuted
                            text: page.teamId <= 0 ? "Créez une équipe dans Effectif, puis planifiez vos séances dans Calendrier."
                                  : "Planifiez une séance dans Calendrier, puis composez son plan ici."
                        }
                    }
                }
            }
        }
    }

    PlanEditor {
        id: editor
        x: page.split ? page.listWidth : 0
        width: page.width - x
        height: page.height
        visible: page.editing
        showBack: !page.split
        onCloseRequested: page.closeEditor()
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
            Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "clipboard"; color: Theme.textDim; size: 56 }
            Text { text: "Choisissez une séance pour composer son plan"; font.pixelSize: 16; color: Theme.textMuted }
        }
    }
}
