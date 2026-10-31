import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtCore
import "Keyboard.js" as Kbd

// Fenêtre principale. Trois dispositions de navigation selon la place disponible :
//  - « bottom » (téléphone en portrait) : barre en bas, comme dans la maquette ;
//  - « rail » (téléphone en paysage, tablette en portrait) : colonne d'icônes à gauche ;
//  - « sidebar » (tablette en paysage, PC) : menu latéral avec les libellés.
ApplicationWindow {
    id: win

    visible: true
    // Téléphone/tablette : la fenêtre est imposée par le système. PC : fenêtre de bureau, bornée par l'écran
    // disponible (sinon on obtenait la mise en page « téléphone » sur Windows).
    readonly property bool desktop: Qt.platform.os !== "android" && Qt.platform.os !== "ios"
    width: desktop ? Math.min(1280, Math.max(minimumWidth, Screen.desktopAvailableWidth - 80)) : 412
    height: desktop ? Math.min(820, Math.max(minimumHeight, Screen.desktopAvailableHeight - 80)) : 892
    minimumWidth: 320
    minimumHeight: 300
    title: "FootCoach"
    color: Theme.bg

    Material.theme: Material.Dark
    Material.accent: Theme.accent
    Material.background: Theme.bg

    // Clavier à l'écran. Deux façons de lui laisser la place, au choix dans Données > Clavier (essai) :
    //  - "fixed" (défaut) : la fenêtre Android ne bouge pas (adjustNothing) et l'application remonte elle-même son contenu
    //    de la hauteur du clavier. Évite le clignotement noir d'Android, qui recrée sa surface quand la fenêtre change
    //    de taille (observé : premier appui dans un champ = écran noir, deuxième appui = correct) ;
    //  - "resize" : Android réduit la fenêtre (adjustResize).
    // Propriétés simples (sans liaison forcée) pour pouvoir les simuler dans les tests.
    property bool keyboardUp: Qt.inputMethod.visible
    property string keyboardMode: AppState.keyboardMode
    property real keyboardHeight: keyboardMode === "fixed"
                                  ? Kbd.height(Qt.inputMethod.keyboardRectangle, win.height, Screen.devicePixelRatio, keyboardUp)
                                  : 0
    Settings {
        id: kbdPrefs
        category: "keyboard"
        property string mode: "fixed"
    }
    Connections {
        target: AppState
        function onKeyboardModeChanged() { kbdPrefs.mode = AppState.keyboardMode; }
    }
    // Essai 0.9.7 : au premier affichage du clavier, son animation Android ne se terminait pas (journal adb, 0.9.5).
    Timer { id: nudge1; interval: 100; onTriggered: if (backend && backend.nudgeKeyboardAnimation) backend.nudgeKeyboardAnimation() }
    Timer { id: nudge2; interval: 400; onTriggered: if (backend && backend.nudgeKeyboardAnimation) backend.nudgeKeyboardAnimation() }
    onKeyboardUpChanged: if (keyboardUp) { nudge1.restart(); nudge2.restart(); }
    onKeyboardModeChanged: applyKeyboardMode()
    Component.onCompleted: {
        if (kbdPrefs.mode === "resize" || kbdPrefs.mode === "fixed")
            AppState.keyboardMode = kbdPrefs.mode;
        applyKeyboardMode();
    }
    function applyKeyboardMode() {
        if (backend && backend.setSoftInputResize)
            backend.setSoftInputResize(keyboardMode === "resize");
    }

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
        { id: "tactics", text: "Tactique", icon: "board" },
        { id: "data", text: "Données", icon: "copy" }
    ]
    readonly property var primaryIds: ["home", "squad", "calendar", "attendance"]
    readonly property var moreIds: ["exercises", "sessions", "tactics", "data"]

    function indexOf(id) {
        for (var i = 0; i < sections.length; ++i) {
            if (sections[i].id === id)
                return i;
        }
        return 0;
    }

    // Bouton retour du téléphone : d'abord la fiche ou le plein écran de la page courante, puis retour à l'accueil.
    // Retourne true si le geste a été utilisé ; sinon (accueil, rien à fermer) l'application peut se fermer.
    function handleBack() {
        var page = ({ squad: squadPage, calendar: calendarPage, attendance: attendancePage,
                      exercises: exercisesPage, sessions: sessionsPage, tactics: tactics })[section];
        if (page && page.goBack())
            return true;
        if (section !== "home") {
            go("home", -1);
            return true;
        }
        return false;
    }

    // Android : le bouton retour demande la fermeture de la fenêtre ; on la refuse tant qu'il y a quelque chose à fermer.
    onClosing: (close) => {
        if (Qt.platform.os === "android" && handleBack())
            close.accepted = false;
    }

    function go(id, boardId) {
        section = id;
        if (id === "tactics" && boardId > 0)
            tactics.openBoard(boardId);
        else if (id === "exercises" && boardId > 0)
            exercisesPage.openById(boardId);
    }

    // Les feuilles du bas s'ouvrent au-dessus de la barre de navigation et de la zone sûre.
    Binding {
        target: AppState
        property: "bottomInset"
        value: win.keyboardHeight > 0 ? win.keyboardHeight
               : (bottomBar.visible ? bottomBar.height : 0) + win.SafeArea.margins.bottom
    }

    ActionSheet {
        id: moreSheet
        title: "Plus"
        model: [
            { id: "exercises", text: "Exercices", icon: "dumbbell", checked: win.section === "exercises" },
            { id: "sessions", text: "Séances", icon: "clipboard", checked: win.section === "sessions" },
            { id: "tactics", text: "Tactique", icon: "board", checked: win.section === "tactics" },
            { id: "data", text: "Données", icon: "copy", checked: win.section === "data" }
        ]
        onChosen: (id) => win.go(id, -1)
    }

    // Toucher en dehors d'un champ de saisie ferme le clavier. Couche transparente au-dessus des pages (et de leurs
    // listes défilantes, qui sinon interceptent le toucher) mais sous les boîtes de dialogue, qui ont la leur.
    Item {
        anchors.fill: parent
        z: 99999
        KeyboardDismiss {}
    }

    // Tout le contenu reste dans la zone sûre (barres système, encoches) quand la fenêtre les recouvre.
    Item {
        id: shell
        anchors.fill: parent
        anchors.topMargin: parent.SafeArea.margins.top
        anchors.bottomMargin: win.keyboardHeight > 0 ? win.keyboardHeight : parent.SafeArea.margins.bottom
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

            // Les entrées défilent quand la fenêtre est trop basse (téléphone en paysage : 8 entrées ne tiennent pas).
            Flickable {
                id: navScroll
                anchors.top: brand.bottom
                anchors.topMargin: 24
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: win.navMode === "sidebar" ? 12 : 8
                anchors.rightMargin: win.navMode === "sidebar" ? 12 : 8
                contentWidth: width
                contentHeight: navColumn.implicitHeight + 8
                clip: true
                boundsBehavior: Flickable.StopAtBounds
            Column {
                id: navColumn
                width: navScroll.width
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
            SquadPage { id: squadPage }
            CalendarPage { id: calendarPage }
            AttendancePage { id: attendancePage }
            ExercisesPage { id: exercisesPage }
            SessionsPage { id: sessionsPage }
            TacticsPage { id: tactics }
            DataPage { onImported: win.go("home", -1) }
        }

        // --- Barre du bas (téléphone en portrait) ------------------------------------------------

        Rectangle {
            id: bottomBar
            // Masquée pendant la saisie : le clavier réduit déjà la fenêtre, la barre ne doit pas lui prendre la place.
            visible: win.navMode === "bottom" && !win.immersive && !win.keyboardUp
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
