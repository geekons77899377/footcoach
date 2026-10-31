import QtQuick

// Accueil : accès rapide à la boîte à outils du coach et aux derniers schémas.
Item {
    id: root

    property var boards: []
    signal navigate(string section, int boardId)

    function refresh() {
        switcher.load();
        boards = backend.boards();
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
            onChanged: live.reload()
        }
    }

    Flickable {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        contentWidth: width
        contentHeight: content.height + 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: content
            x: Theme.gutter
            y: 16
            width: Math.min(parent.width - 2 * Theme.gutter, 720)
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

            // Boîte à outils.
            Text {
                text: "BOÎTE À OUTILS DU COACH"
                font.pixelSize: 12
                font.weight: Font.Bold
                font.letterSpacing: 1.4
                color: Theme.text
            }
            Card {
                width: parent.width
                height: 150
                padding: 18
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -18
                    radius: Theme.radius
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: "#143d2c" }
                        GradientStop { position: 1; color: "#1a2433" }
                    }
                    border.width: 1
                    border.color: Theme.border
                }
                Column {
                    anchors.fill: parent
                    spacing: 6
                    Text {
                        text: "TABLEAU TACTIQUE"
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        font.letterSpacing: 1.2
                        color: Theme.accent
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: "Préparez formations, circuits et consignes sur le terrain"
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Item { width: 1; height: 6 }
                    AppButton {
                        kind: "primary"
                        text: "Ouvrir"
                        iconName: "board"
                        onClicked: root.navigate("tactics", -1)
                    }
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
    }
}
