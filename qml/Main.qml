import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtCore

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

    // Clavier à l'écran ouvert (Android : la fenêtre est alors réduite). Propriété simple pour pouvoir la simuler en test.
    property bool keyboardUp: Qt.inputMethod.visible

    // Contournement (0.9.8), Android seulement : au premier affichage du clavier après le lancement, ses touches ne
    // s'affichent pas (le deuxième champ touché est correct). Cause non trouvée ; on ouvre donc puis on referme le
    // clavier une fois, sur un champ invisible, peu après le lancement. Désactivable dans Données > Clavier (essai).
    Settings {
        id: kbdPrefs
        category: "keyboard"
        property bool warmup: true
    }
    function warmUpKeyboard() {
        warmField.forceActiveFocus();
        Qt.inputMethod.show();
        warmEnd.restart();
    }
    TextInput {
        id: warmField
        objectName: "warmField"
        width: 2; height: 2; opacity: 0
        z: -1
    }
    Timer {
        id: warmStart
        interval: 1500
        running: Qt.platform.os === "android" && kbdPrefs.warmup
        onTriggered: win.warmUpKeyboard()
    }
    Timer {
        id: warmEnd
        interval: 700
        onTriggered: {
            Qt.inputMethod.hide();
            warmField.focus = false;
        }
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
        { id: "matches", text: "Matchs", icon: "board" },
        { id: "tactics", text: "Tactique", icon: "board" },
        { id: "data", text: "Données", icon: "copy" },
        { id: "settings", text: "Paramètres", icon: "gear" }
    ]
    // Barre mobile : six entrées, sans feuille « Plus ». Tactique, Données et Paramètres sont atteints depuis l'accueil.
    readonly property var primaryIds: ["home", "sessions", "exercises", "calendar", "squad", "attendance"]

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
                      exercises: exercisesPage, sessions: sessionsPage, matches: matchesPage, tactics: tactics,
                      settings: settingsPage })[section];
        if (page && page.goBack())
            return true;
        if (AppState.leaveItem(section))        // ouverte depuis une autre page : retour à cette page
            return true;
        if (section !== "home") {
            go("home", -1);
            return true;
        }
        return false;
    }

    // Android : le bouton retour demande la fermeture de la fenêtre ; on la refuse tant qu'il y a quelque chose à fermer.
    // Fermeture de la fenêtre (Windows comme Android) : refusée tant qu'un terrain ne peut pas être enregistré ;
    // le dialogue propose de réessayer ou de quitter sans enregistrer, puis la fermeture reprend (forceClose).
    property bool forceClose: false
    onClosing: (close) => {
        if (Qt.platform.os === "android" && handleBack()) {
            close.accepted = false;
            return;
        }
        if (forceClose)
            return;
        // Fiche de match avec des modifications non enregistrées : la fenêtre ne se ferme pas en silence.
        if (matchesPage.hasUnsaved()) {
            close.accepted = false;
            matchesPage.guardLeave(function () {
                win.forceClose = true;
                win.close();
            });
            return;
        }
        var pages = [tactics, sessionsPage, matchesPage];
        for (var i = 0; i < pages.length; ++i) {
            if (!pages[i].flushBoard()) {
                close.accepted = false;
                pages[i].showSaveFailed(function () {     // flushBoard() vient d'échouer : pas de seconde tentative
                    win.forceClose = true;
                    win.close();
                });
                return;
            }
        }
    }

    // Quitter la section courante : un terrain dont l'enregistrement échoue garde l'utilisateur sur son écran.
    function go(id, boardId) {
        var page = ({ sessions: sessionsPage, matches: matchesPage, tactics: tactics })[section];
        if (id !== section && page)
            page.guardLeave(function () { enter(id, boardId); });
        else
            enter(id, boardId);
    }

    // Ouvre une séance existante : informations et plan sur le même écran (section Séances, équipe de la séance).
    function openSessionPlan(s) {
        AppState.teamId = s.teamId;              // avant l'entrée dans la page : sa liste se charge pour la bonne équipe
        var from = section;
        go("sessions", -1);
        if (section === "sessions") {
            sessionsPage.openSession(s);
            if (from !== "sessions") {
                AppState.returnTo = from;            // retour : la page d'où l'on vient (Calendrier)
                AppState.returnFor = "sessions";
            }
        }
    }

    // Ouvre un match existant (depuis le calendrier) : section Matchs, équipe du match.
    function openMatch(m) {
        AppState.teamId = m.teamId;
        var from = section;
        go("matches", -1);
        if (section === "matches") {
            matchesPage.showMatch(m);
            if (from !== "matches") {
                AppState.returnTo = from;
                AppState.returnFor = "matches";
            }
        }
    }

    // Navigation « en profondeur » (liens de l'accueil, de Paramètres, d'un exercice...) : l'origine est retenue pour que
    // « retour » y revienne. Les onglets et la barre de navigation n'en retiennent aucune.
    function goFrom(id, itemId) {
        var from = section;
        go(id, itemId);
        if (section === id && from !== id) {
            AppState.returnTo = from;
            AppState.returnFor = id;
        }
    }

    onSectionChanged: if (section !== AppState.returnFor) AppState.clearReturn()

    Connections {
        target: AppState
        function onReturnRequested(target) { win.go(target, -1); }
    }

    function enter(id, boardId) {
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
        value: (bottomBar.visible ? bottomBar.height : 0) + win.SafeArea.margins.bottom
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
                    font.family: Theme.fontHeading
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
                onNavigate: (section, boardId) => win.goFrom(section, boardId)
            }
            SquadPage { id: squadPage }
            CalendarPage { id: calendarPage; onPlanRequested: (s) => win.openSessionPlan(s); onMatchRequested: (m) => win.openMatch(m) }
            AttendancePage { id: attendancePage }
            ExercisesPage { id: exercisesPage; onOpenBoard: (id) => win.goFrom("tactics", id) }
            SessionsPage { id: sessionsPage }
            MatchesPage { id: matchesPage }
            TacticsPage { id: tactics }
            DataPage { onImported: win.go("home", -1) }
            SettingsPage { id: settingsPage; onNavigate: (section) => win.goFrom(section, -1) }
        }

        // --- Barre du bas (téléphone en portrait) ------------------------------------------------

        Rectangle {
            id: bottomBar
            objectName: "bottomBar"
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
                    model: win.primaryIds
                    delegate: Item {
                        id: tab
                        required property string modelData
                        readonly property var def: win.sections[win.indexOf(modelData)]
                        readonly property bool active: win.section === modelData
                        objectName: "navTab_" + modelData
                        width: parent.width / win.primaryIds.length
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
                                objectName: "navLabel_" + tab.modelData
                                width: tab.width - 8
                                horizontalAlignment: Text.AlignHCenter
                                text: tab.def.short || tab.def.text
                                // Taille uniforme pour toutes les entrées (pas de réduction individuelle) : à 360 px,
                                // chaque entrée fait 60 px ; le test vérifie que chaque libellé tient dans sa largeur.
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                maximumLineCount: 1
                                color: tab.active ? Theme.accent : Theme.textMuted
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: win.go(tab.modelData, -1)
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
