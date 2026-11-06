import QtQuick
import "DateUtil.js" as DateUtil
import "Status.js" as Status

// Accueil : séance du jour, accès aux sections et derniers schémas. À partir de 760 px de large (tablette
// paysage, bureau), une colonne latérale ajoute les prochaines séances et la présence récente ; en dessous, tout
// reste en une seule colonne, comme avant (téléphone inchangé : la colonne latérale n'y apparaît pas).
Item {
    id: root

    property var boards: []
    property var upcoming: []                    // prochaines séances de l'équipe (14 jours)
    property real attendanceRate: -1             // taux de présence moyen sur 30 jours (0..1), -1 : aucune donnée
    property int attendanceSessions: 0
    readonly property bool twoCol: width >= 760 && height >= 520   // téléphone en paysage : colonne unique
    readonly property bool showSide: width >= 600 && height >= 520
    readonly property int pad: twoCol ? Theme.gutter + 4 : Theme.gutter   // aligné sur le titre de l'en-tête
    readonly property int sideW: width >= 1100 ? 340 : 300
    readonly property var months: ["JANV", "FÉVR", "MARS", "AVR", "MAI", "JUIN", "JUIL", "AOÛT", "SEPT", "OCT", "NOV", "DÉC"]
    signal navigate(string section, int boardId)

    function loadSide() {
        var team = AppState.teamId;
        var today = DateUtil.todayIso();
        var list = [];
        var rate = -1, count = 0;
        if (team > 0) {
            var all = backend.sessions(team, today, DateUtil.addDays(today, 14));
            var nowHm = Qt.formatTime(new Date(), "HH:mm");
            for (var i = 0; i < all.length; ++i) {
                var s = all[i];
                if (s.status === "cancelled" || (s.date === today && s.endTime < nowHm))
                    continue;
                list.push(s);
            }
            var st = backend.attendanceStats(team, DateUtil.addDays(today, -30), today);
            var sum = 0, n = 0;
            for (var j = 0; j < st.players.length; ++j) {
                if (st.players[j].rate >= 0) {
                    sum += st.players[j].rate;
                    n++;
                }
            }
            rate = n > 0 ? sum / n : -1;
            count = st.sessions.length;
        }
        upcoming = list.slice(0, 4);
        attendanceRate = rate;
        attendanceSessions = count;
    }

    function refresh() {
        switcher.load();
        boards = backend.boards();
        loadSide();
    }

    Component.onCompleted: refresh()
    onVisibleChanged: if (visible) refresh()

    PageHeader {
        id: header
        width: parent.width
        title: "Tableau de bord"
        subtitle: "Bonne séance, coach"
        TeamSwitcher {
            id: switcher
            onChanged: {
                live.reload();
                root.loadSide();
            }
        }
    }

    Flickable {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        contentWidth: width
        contentHeight: Math.max(content.y + content.height, root.showSide ? side.y + side.height : 0) + 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: content
            x: root.pad
            y: 16
            width: root.twoCol ? parent.width - 2 * root.pad - root.sideW - 24 : Math.min(parent.width - 2 * root.pad, 720)
            spacing: 18

            // Séance du jour : chronomètre et déroulé.
            LiveSessionCard {
                id: live
                objectName: "live"
                width: parent.width
                teamName: switcher.current === null ? "" : switcher.current.name
                onOpenExercise: (exerciseId) => root.navigate("exercises", exerciseId)
                onPlanSession: root.navigate("calendar", -1)
            }

            // Accès aux sections qui ne sont pas dans la barre du bas (téléphone).
            Row {
                width: parent.width
                spacing: 12
                AppButton {
                    objectName: "matchesButton"
                    width: (parent.width - 12) / 2
                    text: "Matchs"
                    iconName: "board"
                    onClicked: root.navigate("matches", -1)
                }
                AppButton {
                    objectName: "tacticsButton"
                    width: (parent.width - 12) / 2
                    text: "Tactique"
                    iconName: "board"
                    onClicked: root.navigate("tactics", -1)
                }
            }

            // Accès explicites aux données et aux paramètres (plus de feuille « Plus » sur téléphone).
            Row {
                width: parent.width
                spacing: 12
                AppButton {
                    objectName: "dataButton"
                    width: (parent.width - 12) / 2
                    text: "Données"
                    iconName: "copy"
                    onClicked: root.navigate("data", -1)
                }
                AppButton {
                    objectName: "settingsButton"
                    width: (parent.width - 12) / 2
                    text: "Paramètres"
                    iconName: "gear"
                    onClicked: root.navigate("settings", -1)
                }
            }

            // Derniers schémas.
            Text {
                visible: root.boards.length > 0
                text: "SCHÉMAS RÉCENTS"
                font.pixelSize: 12
                font.weight: Font.Bold
                font.letterSpacing: 1.4
                color: Theme.text
            }
            Repeater {
                model: root.boards.slice(0, 4)
                delegate: Card {
                    id: row
                    required property var modelData
                    width: content.width
                    height: 84
                    padding: 12
                    BoardThumb {
                        id: thumb
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        json: row.modelData.json || ""
                    }
                    Text {
                        anchors.left: thumb.right
                        anchors.leftMargin: 14
                        anchors.right: chevron.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: row.modelData.title
                        elide: Text.ElideRight
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Icon {
                        id: chevron
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        name: "chevron"
                        color: Theme.textMuted
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.navigate("tactics", row.modelData.id)
                    }
                }
            }
        }

        // Colonne latérale (grand écran) ou bloc sous la colonne principale (tablette portrait).
        Column {
            id: side
            visible: root.showSide
            x: root.twoCol ? content.x + content.width + 24 : root.pad
            y: root.twoCol ? 16 : content.y + content.height + 18
            width: root.twoCol ? root.sideW : content.width
            spacing: 18

            Card {
                id: upcomingCard
                objectName: "upcomingCard"
                width: parent.width
                height: upcomingCol.height + 2 * padding
                padding: 20
                radius: Theme.radiusLg
                color: Theme.surface
                Column {
                    id: upcomingCol
                    width: parent.width
                    spacing: 14
                    Item {
                        width: parent.width
                        height: 28
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Prochaines séances"
                            font.family: Theme.fontHeading
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Text {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: "CALENDRIER"
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            font.letterSpacing: 1.2
                            color: Theme.accent
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -12
                                onClicked: root.navigate("calendar", -1)
                            }
                        }
                    }
                    Rectangle { width: parent.width; height: 1; color: Theme.borderSubtle }
                    Text {
                        visible: root.upcoming.length === 0
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: AppState.teamId > 0 ? "Aucune séance prévue dans les 14 prochains jours."
                                                  : "Aucune équipe sélectionnée."
                        font.pixelSize: 14
                        color: Theme.textMuted
                    }
                    Repeater {
                        model: root.upcoming
                        delegate: Item {
                            id: srow
                            required property var modelData
                            readonly property var st: Status.session(modelData.status)
                            width: upcomingCol.width
                            height: 52
                            Rectangle {
                                id: badge
                                width: 48
                                height: 48
                                radius: Theme.radiusInput + 4
                                color: Theme.surfaceHi
                                border.width: 1
                                border.color: Theme.borderSubtle
                                Column {
                                    anchors.centerIn: parent
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: parseInt(srow.modelData.date.substring(8, 10), 10)
                                        font.family: Theme.fontHeading
                                        font.pixelSize: 18
                                        font.weight: Font.Bold
                                        color: Theme.info
                                    }
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: root.months[parseInt(srow.modelData.date.substring(5, 7), 10) - 1]
                                        font.pixelSize: 9
                                        font.weight: Font.Bold
                                        font.letterSpacing: 0.6
                                        color: Theme.textMuted
                                    }
                                }
                            }
                            Column {
                                anchors.left: badge.right
                                anchors.leftMargin: 12
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 3
                                Text {
                                    width: parent.width
                                    elide: Text.ElideRight
                                    text: srow.modelData.theme !== "" ? srow.modelData.theme : "Séance"
                                    font.pixelSize: 15
                                    font.weight: Font.Bold
                                    color: Theme.text
                                }
                                Text {
                                    width: parent.width
                                    elide: Text.ElideRight
                                    text: srow.modelData.time + " – " + srow.modelData.endTime
                                          + (srow.modelData.location !== "" ? " · " + srow.modelData.location : "")
                                    font.pixelSize: 12
                                    color: Theme.textMuted
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.navigate("calendar", -1)
                            }
                        }
                    }
                }
            }

            Card {
                id: presenceCard
                objectName: "presenceCard"
                width: parent.width
                height: presenceCol.height + 2 * padding
                padding: 20
                radius: Theme.radiusLg
                Column {
                    id: presenceCol
                    width: parent.width
                    spacing: 14
                    Text {
                        text: "Présence récente"
                        font.family: Theme.fontHeading
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Row {
                        width: parent.width
                        spacing: 12
                        Repeater {
                            model: [
                                { label: "PRÉSENCE · 30 J", value: root.attendanceRate < 0 ? "—" : Math.round(root.attendanceRate * 100) + " %",
                                  color: root.attendanceRate < 0 ? Theme.textMuted : Status.rateColor(root.attendanceRate) },
                                { label: "SÉANCES COMPTÉES", value: root.attendanceSessions + "", color: Theme.info }
                            ]
                            delegate: Rectangle {
                                required property var modelData
                                width: (presenceCol.width - 12) / 2
                                height: 96
                                radius: Theme.radius
                                color: Theme.bgDeep
                                border.width: 1
                                border.color: Theme.borderSubtle
                                Column {
                                    anchors.centerIn: parent
                                    spacing: 6
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: parent.parent.modelData.label
                                        font.pixelSize: 9
                                        font.weight: Font.Bold
                                        font.letterSpacing: 1
                                        color: Theme.textMuted
                                    }
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: parent.parent.modelData.value
                                        font.family: Theme.fontHeading
                                        font.pixelSize: 30
                                        font.weight: Font.ExtraBold
                                        color: parent.parent.modelData.color
                                    }
                                }
                            }
                        }
                    }
                    AppButton {
                        width: parent.width
                        text: "Voir le suivi"
                        onClicked: root.navigate("attendance", -1)
                    }
                }
            }
        }
    }
}
