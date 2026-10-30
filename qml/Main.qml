import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Fenêtre principale. Trois dispositions de navigation selon la place disponible :
//  - « bottom » (téléphone en portrait) : barre en bas, comme dans la maquette ;
//  - « rail » (téléphone en paysage, tablette en portrait) : colonne d'icônes à gauche ;
//  - « sidebar » (tablette en paysage, PC) : menu latéral avec les libellés.
ApplicationWindow {
    id: win

    visible: true
    width: 412
    height: 892
    minimumWidth: 320
    minimumHeight: 300
    title: "FootCoach"
    color: Theme.bg

    Material.theme: Material.Dark
    Material.accent: Theme.accent
    Material.background: Theme.bg

    readonly property string navMode: width < 600 ? "bottom" : (width >= 840 && height >= 520 ? "sidebar" : "rail")
    property string section: "home"

    // Éditeur du terrain ouvert sur un petit écran : la navigation se masque pour laisser toute la place au
    // terrain (le bouton retour de l'éditeur ramène à la liste).
    readonly property bool immersive: section === "tactics" && tactics.currentId >= 0
                                      && navMode !== "sidebar" && width < 900

    readonly property var sections: [
        { id: "home", text: "Accueil", icon: "home" },
        { id: "squad", text: "Effectif", icon: "users" },
        { id: "calendar", text: "Calendrier", icon: "calendar" },
        { id: "attendance", text: "Présences", short: "Suivi", icon: "check" },
        { id: "exercises", text: "Exercices", icon: "dumbbell" },
        { id: "sessions", text: "Séances", icon: "clipboard" },
        { id: "tactics", text: "Tactique", icon: "board" }
    ]
    readonly property var primaryIds: ["home", "squad", "calendar", "attendance"]
    readonly property var moreIds: ["exercises", "sessions", "tactics"]

    function indexOf(id) {
        for (var i = 0; i < sections.length; ++i) {
            if (sections[i].id === id)
                return i;
        }
        return 0;
    }

    function go(id, boardId) {
        section = id;
        if (id === "tactics" && boardId > 0)
            tactics.openBoard(boardId);
    }

    ActionSheet {
        id: moreSheet
        title: "Plus"
        // Au-dessus de la barre du bas (et de la zone sûre en bas de l'écran).
        navInset: bottomBar.visible ? bottomBar.height + win.SafeArea.margins.bottom : 0
        model: [
            { id: "exercises", text: "Exercices", icon: "dumbbell", checked: win.section === "exercises" },
            { id: "sessions", text: "Séances", icon: "clipboard", checked: win.section === "sessions" },
            { id: "tactics", text: "Tactique", icon: "board", checked: win.section === "tactics" }
        ]
        onChosen: (id) => win.go(id, -1)
    }

    // Tout le contenu reste dans la zone sûre (barres système, encoches) quand la fenêtre les recouvre.
    Item {
        id: shell
        anchors.fill: parent
        anchors.topMargin: parent.SafeArea.margins.top
        anchors.bottomMargin: parent.SafeArea.margins.bottom
        anchors.leftMargin: parent.SafeArea.margins.left
        anchors.rightMargin: parent.SafeArea.margins.right

        // --- Menu latéral (grand écran) et colonne d'icônes ------------------------------------------

        Rectangle {
            id: side
            visible: win.navMode !== "bottom" && !win.immersive
            width: win.navMode === "sidebar" ? 248 : 84
            height: parent.height
            color: Theme.bgDeep

            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Theme.border
            }

            Row {
                id: brand
                anchors.top: parent.top
                anchors.topMargin: 22
                anchors.horizontalCenter: win.navMode === "rail" ? parent.horizontalCenter : undefined
                anchors.left: win.navMode === "sidebar" ? parent.left : undefined
                anchors.leftMargin: 20
                spacing: 12
                Rectangle {
                    width: 40
                    height: 40
                    radius: 12
                    color: Theme.accent
                    Icon {
                        anchors.centerIn: parent
                        name: "pitch"
                        color: Theme.textOnAccent
                        size: 24
                    }
                }
                Text {
                    visible: win.navMode === "sidebar"
                    anchors.verticalCenter: parent.verticalCenter
                    text: "FootCoach"
                    font.pixelSize: 20
                    font.weight: Font.ExtraBold
                    color: Theme.text
                }
            }

            Column {
                anchors.top: brand.bottom
                anchors.topMargin: 24
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: win.navMode === "sidebar" ? 12 : 8
                anchors.rightMargin: win.navMode === "sidebar" ? 12 : 8
                spacing: 6
                Repeater {
                    model: win.sections
                    delegate: Rectangle {
                        id: item
                        required property var modelData
                        readonly property bool active: win.section === modelData.id
                        width: parent.width
                        height: win.navMode === "sidebar" ? 52 : 60
                        radius: 14
                        color: active ? Theme.accentSoft : (area.pressed ? Theme.surface : "transparent")
                        Accessible.role: Accessible.Button
                        Accessible.name: modelData.text

                        Row {
                            visible: win.navMode === "sidebar"
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 16
                            spacing: 14
                            Icon {
                                name: item.modelData.icon
                                color: item.active ? Theme.accent : Theme.textMuted
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: item.modelData.text
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                                color: item.active ? Theme.accent : Theme.text
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        Column {
                            visible: win.navMode === "rail"
                            anchors.centerIn: parent
                            spacing: 3
                            Icon {
                                anchors.horizontalCenter: parent.horizontalCenter
                                name: item.modelData.icon
                                color: item.active ? Theme.accent : Theme.textMuted
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: (item.modelData.short || item.modelData.text).toUpperCase()
                                font.pixelSize: 9
                                font.weight: Font.Bold
                                font.letterSpacing: 0.6
                                color: item.active ? Theme.accent : Theme.textMuted
                            }
                        }
                        MouseArea {
                            id: area
                            anchors.fill: parent
                            onClicked: win.go(item.modelData.id, -1)
                        }
                    }
                }
            }
        }

        // --- Pages ----------------------------------------------------------------------------------

        StackLayout {
            id: pages
            x: side.visible ? side.width : 0
            width: parent.width - x
            height: parent.height - (bottomBar.visible ? bottomBar.height : 0)
            currentIndex: win.indexOf(win.section)

            HomePage {
                onNavigate: (section, boardId) => win.go(section, boardId)
            }
            SquadPage { }
            CalendarPage { }
            AttendancePage { }
            PlaceholderPage { title: "Exercices"; iconName: "dumbbell" }
            PlaceholderPage { title: "Séances"; iconName: "clipboard" }
            TacticsPage { id: tactics }
        }

        // --- Barre du bas (téléphone en portrait) ------------------------------------------------

        Rectangle {
            id: bottomBar
            visible: win.navMode === "bottom" && !win.immersive
            anchors.bottom: parent.bottom
            width: parent.width
            height: 72
            color: Theme.bgDeep

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.border
            }

            Row {
                anchors.fill: parent
                Repeater {
                    model: win.primaryIds.concat(["more"])
                    delegate: Item {
                        id: tab
                        required property string modelData
                        readonly property bool isMore: modelData === "more"
                        readonly property var def: isMore ? { text: "Plus", icon: "menu" } : win.sections[win.indexOf(modelData)]
                        readonly property bool active: isMore ? win.moreIds.indexOf(win.section) >= 0 : win.section === modelData
                        width: parent.width / 5
                        height: parent.height
                        Accessible.role: Accessible.Button
                        Accessible.name: def.text

                        Column {
                            anchors.centerIn: parent
                            spacing: 5
                            Icon {
                                anchors.horizontalCenter: parent.horizontalCenter
                                name: tab.def.icon
                                color: tab.active ? Theme.accent : Theme.textMuted
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: (tab.def.short || tab.def.text).toUpperCase()
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                color: tab.active ? Theme.accent : Theme.textMuted
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: tab.isMore ? moreSheet.open() : win.go(tab.modelData, -1)
                        }
                    }
                }
            }
        }
    }

    // Échec d'ouverture de la base de données.
    Rectangle {
        visible: startupError !== ""
        anchors.fill: parent
        color: Theme.bg
        Card {
            anchors.centerIn: parent
            width: Math.min(parent.width - 32, 460)
            Column {
                width: parent.width
                spacing: 10
                Text {
                    text: "Impossible d'ouvrir la base de données"
                    width: parent.width
                    wrapMode: Text.Wrap
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    color: Theme.text
                }
                Text {
                    text: startupError
                    width: parent.width
                    wrapMode: Text.Wrap
                    font.pixelSize: 14
                    color: Theme.textMuted
                }
            }
        }
    }
}
