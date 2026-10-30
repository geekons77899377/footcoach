import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil
import "Status.js" as Status

// Page « Suivi » : taux de présence par joueur sur une période, avec le détail séance par séance.
// Taux = présents ÷ (présents + absents) : excusés et blessés ne comptent pas contre le joueur.
Item {
    id: page

    property string period: "all"
    property var stats: ({ sessions: [], players: [] })
    property int openIndex: -1
    readonly property int teamId: AppState.teamId
    readonly property var periods: [{ id: "all", text: "Tout" }, { id: "30", text: "30 jours" },
                                    { id: "90", text: "3 mois" }, { id: "365", text: "12 mois" }]

    function refresh() {
        switcher.load();
        refreshData();
    }

    function refreshData() {
        openIndex = -1;
        if (teamId <= 0) {
            stats = { sessions: [], players: [] };
            return;
        }
        var to = DateUtil.todayIso();
        var from = period === "all" ? "2000-01-01" : DateUtil.addDays(to, -parseInt(period, 10));
        stats = backend.attendanceStats(teamId, from, to);
    }

    onTeamIdChanged: refreshData()
    onPeriodChanged: refreshData()
    Component.onCompleted: refresh()
    onVisibleChanged: if (visible) refresh()

    PageHeader {
        id: header
        width: parent.width
        title: "Suivi"
        subtitle: !switcher.current ? "Aucune équipe"
                  : switcher.current.name + " · " + page.stats.sessions.length
                    + (page.stats.sessions.length > 1 ? " séances réalisées" : " séance réalisée")
        TeamSwitcher { id: switcher; onChanged: page.refresh() }
    }

    ListView {
        id: list
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        leftMargin: Theme.gutter
        rightMargin: Theme.gutter
        bottomMargin: 16
        spacing: 10
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: page.stats.players

        header: Item {
            width: list.width - 2 * Theme.gutter
            height: chips.height + 12
            ChipRow {
                id: chips
                y: 4
                width: parent.width
                options: page.periods
                value: page.period
                allowClear: false
                onPicked: (id) => page.period = id
            }
        }

        delegate: Card {
            id: card
            required property var modelData
            required property int index
            readonly property bool open: page.openIndex === index
            width: Math.min(list.width - 2 * Theme.gutter, 720)
            padding: 12
            implicitHeight: head.height + (open ? cells.height + 12 : 0) + 24
            Behavior on implicitHeight { NumberAnimation { duration: 120 } }
            clip: true

            Item {
                id: head
                width: parent.width
                height: 48
                Rectangle {
                    id: badge
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 40
                    height: 40
                    radius: 20
                    color: Theme.accentSoft
                    Text {
                        anchors.centerIn: parent
                        text: card.modelData.jerseyNumber > 0 ? card.modelData.jerseyNumber : "–"
                        font.pixelSize: 16
                        font.weight: Font.ExtraBold
                        color: Theme.accent
                    }
                }
                Column {
                    anchors.left: badge.right
                    anchors.leftMargin: 12
                    anchors.right: rate.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3
                    Text {
                        width: parent.width
                        text: card.modelData.name + (card.modelData.active ? "" : " (inactif)")
                        elide: Text.ElideRight
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        font.pixelSize: 12
                        color: Theme.textMuted
                        text: card.modelData.counted === 0 ? "Aucune séance comptée"
                              : card.modelData.present + " présent · " + card.modelData.absent + " absent"
                                + (card.modelData.excused > 0 ? " · " + card.modelData.excused + " excusé" : "")
                                + (card.modelData.injured > 0 ? " · " + card.modelData.injured + " blessé" : "")
                    }
                }
                Text {
                    id: rate
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: card.modelData.rate < 0 ? "—" : Math.round(card.modelData.rate * 100) + " %"
                    font.pixelSize: 20
                    font.weight: Font.ExtraBold
                    color: Status.rateColor(card.modelData.rate)
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: page.openIndex = card.open ? -1 : card.index
                }
            }
            Flow {
                id: cells
                visible: card.open
                anchors.top: head.bottom
                anchors.topMargin: 10
                width: parent.width
                spacing: 6
                Repeater {
                    model: card.open ? card.modelData.cells : []
                    delegate: Rectangle {
                        id: cell
                        required property string modelData
                        required property int index
                        readonly property var st: Status.attendance(modelData)
                        width: 54
                        height: 40
                        radius: 10
                        color: modelData === "" ? "transparent" : Qt.rgba(Qt.color(st.color).r, Qt.color(st.color).g, Qt.color(st.color).b, 0.18)
                        border.width: 1
                        border.color: modelData === "" ? Theme.border : st.color
                        Column {
                            anchors.centerIn: parent
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: DateUtil.display(page.stats.sessions[cell.index].date).substring(0, 5)
                                font.pixelSize: 11
                                color: Theme.textMuted
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: cell.st.letter
                                font.pixelSize: 14
                                font.weight: Font.ExtraBold
                                color: cell.st.color
                            }
                        }
                        Accessible.name: DateUtil.display(page.stats.sessions[index].date) + " : " + st.label
                    }
                }
            }
        }

        footer: Item {
            width: list.width - 2 * Theme.gutter
            height: visible ? 220 : 0
            visible: page.teamId <= 0 || page.stats.players.length === 0
            Card {
                width: parent.width
                Column {
                    width: parent.width
                    spacing: 10
                    Icon { name: "check"; color: Theme.accent; size: 32 }
                    Text {
                        width: parent.width
                        text: page.teamId <= 0 ? "Créez d'abord une équipe" : "Pas encore de statistiques"
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 14
                        color: Theme.textMuted
                        text: page.teamId <= 0 ? "Les présences suivent les joueurs d'une équipe (page Effectif)."
                              : "Faites l'appel depuis le Calendrier : une séance compte une fois passée en « Réalisée ». "
                                + "Taux = présents ÷ (présents + absents) ; les excusés et les blessés ne comptent pas."
                    }
                }
            }
        }
    }
}
