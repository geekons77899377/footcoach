import QtQuick
import QtQuick.Controls

// Page « Effectif » : choix de l'équipe, recherche, filtre par poste, liste des joueurs et fiche. En grand écran, liste
// et fiche côte à côte ; sur téléphone, l'une après l'autre.
Item {
    id: page

    property var teams: []
    property int teamId: AppState.teamId           // partagé avec les autres pages
    onTeamIdChanged: if (AppState.teamId !== teamId) AppState.teamId = teamId
    Connections {
        target: AppState
        function onTeamIdChanged() {
            if (page.teamId !== AppState.teamId) {
                page.teamId = AppState.teamId;
                page.closeForm();
                page.refreshPlayers();
            }
        }
    }
    property var players: []
    property string query: ""
    property string positionFilter: ""
    property bool showInactive: false
    property int currentId: -1            // joueur ouvert (-1 : aucun)
    property bool editing: false          // fiche ouverte (joueur existant ou nouveau)
    readonly property bool split: width >= 900
    readonly property int listWidth: 380
    readonly property var team: {
        for (var i = 0; i < teams.length; ++i)
            if (teams[i].id === teamId)
                return teams[i];
        return null;
    }
    readonly property var positionLabels: ({ GK: "Gardien", DEF: "Défenseur", MID: "Milieu", FWD: "Attaquant" })
    readonly property var shown: {
        var q = query.trim().toLowerCase();
        var out = [];
        for (var i = 0; i < players.length; ++i) {
            var p = players[i];
            if (positionFilter !== "" && p.position !== positionFilter)
                continue;
            if (q !== "" && (p.fullName.toLowerCase().indexOf(q) < 0 && String(p.jerseyNumber) !== q))
                continue;
            out.push(p);
        }
        return out;
    }

    function refreshTeams() {
        teams = backend.teams();
        var found = false;
        for (var i = 0; i < teams.length; ++i)
            if (teams[i].id === teamId)
                found = true;
        if (!found)
            teamId = teams.length > 0 ? teams[0].id : -1;
        refreshPlayers();
    }

    function refreshPlayers() {
        players = teamId > 0 ? backend.players(teamId, showInactive) : [];
    }

    function openPlayer(p) {
        currentId = p.id;
        form.openPlayer(p);
        editing = true;
    }

    function newPlayer() {
        currentId = -1;
        form.openNew(teamId);
        editing = true;
    }

    function closeForm() {
        editing = false;
        currentId = -1;
    }

    function subtitleText() {
        if (!team)
            return "Aucune équipe";
        var n = players.length;
        var count = n === 0 ? "aucun joueur" : n === 1 ? "1 joueur" : n + " joueurs";
        return team.name + " · " + count;
    }

    onShowInactiveChanged: refreshPlayers()
    Component.onCompleted: refreshTeams()
    onVisibleChanged: if (visible) refreshTeams()

    TeamDialog {
        id: teamDialog
        onSaved: (id) => {
            page.teamId = id;
            page.refreshTeams();
        }
    }

    PromptDialog {
        id: confirm
        property string action: ""
        onAccepted: {
            if (action === "deletePlayer") {
                var r = backend.deletePlayer(page.currentId);
                if (r.ok) {
                    page.closeForm();
                    page.refreshTeams();
                } else {
                    toast.show(r.error);
                }
            } else if (action === "deleteTeam") {
                var t = backend.deleteTeam(page.teamId);
                if (t.ok) {
                    page.closeForm();
                    page.teamId = -1;
                    page.refreshTeams();
                } else {
                    toast.show(t.error);
                }
            }
        }
    }

    ActionSheet {
        id: teamSheet
        title: "Équipe"
        function rebuild() {
            var m = [];
            for (var i = 0; i < page.teams.length; ++i)
                m.push({ id: "t" + page.teams[i].id, text: page.teams[i].name
                         + (page.teams[i].category ? " · " + page.teams[i].category : ""),
                         icon: "users", checked: page.teams[i].id === page.teamId });
            m.push({ id: "new", text: "Nouvelle équipe", icon: "plus" });
            if (page.team) {
                m.push({ id: "edit", text: "Modifier l'équipe", icon: "edit" });
                m.push({ id: "delete", text: "Supprimer l'équipe", icon: "trash", danger: true });
            }
            model = m;
        }
        onChosen: (id) => {
            if (id === "new") {
                teamDialog.openFor(null);
            } else if (id === "edit") {
                teamDialog.openFor(page.team);
            } else if (id === "delete") {
                confirm.action = "deleteTeam";
                confirm.confirm("Supprimer l'équipe ?",
                                "« " + page.team.name + " » sera supprimée avec ses "
                                + page.team.playerCount + " joueur(s), ses séances et ses présences. "
                                + "Cette action est définitive.", "Supprimer", true);
            } else if (id.charAt(0) === "t") {
                page.teamId = parseInt(id.substring(1), 10);
                page.closeForm();
                page.refreshPlayers();
            }
        }
    }

    // --- Liste ------------------------------------------------------------------------------------

    Item {
        id: listPane
        width: page.split ? page.listWidth : page.width
        height: page.height
        visible: page.split || !page.editing

        PageHeader {
            id: listHeader
            width: parent.width
            title: "Effectif"
            subtitle: page.subtitleText()
            IconButton {
                iconName: "users"
                accessibleName: "Changer d'équipe"
                fillColor: Theme.surface
                onClicked: {
                    teamSheet.rebuild();
                    teamSheet.open();
                }
            }
            IconButton {
                visible: page.team !== null
                iconName: "plus"
                iconColor: Theme.textOnAccent
                fillColor: Theme.accent
                diameter: 48
                glow: true
                accessibleName: "Nouveau joueur"
                onClicked: page.newPlayer()
            }
        }

        Column {
            id: filters
            visible: page.team !== null
            anchors.top: listHeader.bottom
            anchors.left: parent.left
            anchors.leftMargin: Theme.gutter
            anchors.right: parent.right
            anchors.rightMargin: Theme.gutter
            anchors.topMargin: 4
            spacing: 10

            Rectangle {
                width: parent.width
                height: 48
                radius: 24
                color: Theme.surface
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
                    placeholderText: "Nom, prénom ou numéro"
                    placeholderTextColor: Theme.textDim
                    color: Theme.text
                    font.pixelSize: 15
                    background: null
                    onTextChanged: page.query = text
                    Keys.onReturnPressed: focus = false
                }
            }
            ChipRow {
                width: parent.width
                value: page.positionFilter
                options: [{ id: "GK", text: "Gardiens" }, { id: "DEF", text: "Défenseurs" },
                          { id: "MID", text: "Milieux" }, { id: "FWD", text: "Attaquants" }]
                onPicked: (id) => page.positionFilter = id
            }
            Row {
                spacing: 10
                Switch {
                    id: inactiveSwitch
                    checked: page.showInactive
                    onToggled: page.showInactive = checked
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "Afficher les joueurs inactifs"
                    font.pixelSize: 14
                    color: Theme.textMuted
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        ListView {
            id: list
            anchors.top: filters.visible ? filters.bottom : listHeader.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 10
            leftMargin: Theme.gutter
            rightMargin: Theme.gutter
            bottomMargin: 16
            spacing: 10
            clip: true
            model: page.team ? page.shown : []
            boundsBehavior: Flickable.StopAtBounds

            delegate: Card {
                id: row
                required property var modelData
                width: list.width - list.leftMargin - list.rightMargin
                height: 72
                padding: 12
                highlighted: page.split && page.editing && page.currentId === modelData.id
                opacity: modelData.active ? 1 : 0.55

                Rectangle {
                    id: badge
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 46
                    height: 46
                    radius: 23
                    color: Theme.accentSoft
                    border.width: 1
                    border.color: Theme.accent
                    Text {
                        anchors.centerIn: parent
                        text: row.modelData.jerseyNumber > 0 ? row.modelData.jerseyNumber : "–"
                        font.pixelSize: 18
                        font.weight: Font.ExtraBold
                        color: Theme.accent
                    }
                }
                Column {
                    anchors.left: badge.right
                    anchors.leftMargin: 14
                    anchors.right: chevron.left
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3
                    Text {
                        width: parent.width
                        text: row.modelData.fullName
                        elide: Text.ElideRight
                        font.pixelSize: 17
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        font.pixelSize: 13
                        color: Theme.textMuted
                        text: {
                            var parts = [];
                            if (row.modelData.position)
                                parts.push(page.positionLabels[row.modelData.position]);
                            if (row.modelData.age >= 0)
                                parts.push(row.modelData.age + " ans");
                            if (!row.modelData.active)
                                parts.push("inactif");
                            return parts.length > 0 ? parts.join(" · ") : "—";
                        }
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
                    onClicked: page.openPlayer(row.modelData)
                }
            }

            footer: Item {
                width: list.width - 2 * Theme.gutter
                height: visible ? 200 : 0
                visible: page.team === null || (page.shown.length === 0)
                Card {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    Column {
                        width: parent.width
                        spacing: 12
                        Icon { name: "users"; color: Theme.accent; size: 32 }
                        Text {
                            width: parent.width
                            text: page.team === null ? "Créez votre équipe"
                                  : page.players.length === 0 ? "Aucun joueur" : "Aucun résultat"
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.Wrap
                            font.pixelSize: 14
                            color: Theme.textMuted
                            text: page.team === null ? "Une équipe regroupe vos joueurs, vos séances et vos présences."
                                  : page.players.length === 0 ? "Ajoutez vos premiers joueurs à l'équipe."
                                  : "Aucun joueur ne correspond à la recherche ou au filtre."
                        }
                        AppButton {
                            visible: page.team === null || page.players.length === 0
                            kind: "primary"
                            iconName: "plus"
                            text: page.team === null ? "Créer une équipe" : "Ajouter un joueur"
                            onClicked: page.team === null ? teamDialog.openFor(null) : page.newPlayer()
                        }
                    }
                }
            }
        }
    }

    // --- Fiche ------------------------------------------------------------------------------------

    PlayerForm {
        id: form
        category: page.team ? page.team.category : ""
        x: page.split ? page.listWidth : 0
        width: page.width - x
        height: page.height
        visible: page.editing
        showBack: !page.split
        onCloseRequested: page.closeForm()
        onSaved: {
            page.currentId = form.playerId;
            page.refreshTeams();
            if (!page.split)
                page.closeForm();
            toast.show("Joueur enregistré");
        }
        onToggleRequested: {
            var r = backend.setPlayerActive(page.currentId, !form.active);
            if (r.ok) {
                page.refreshTeams();
                page.closeForm();
            } else {
                toast.show(r.error);
            }
        }
        onDeleteRequested: {
            confirm.action = "deletePlayer";
            confirm.confirm("Supprimer le joueur ?",
                            "Le joueur, ses présences et ses blessures seront supprimés définitivement. "
                            + "Pour le garder dans l'historique, désactivez-le plutôt.", "Supprimer", true);
        }
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
            Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "users"; color: Theme.textDim; size: 56 }
            Text { text: "Choisissez un joueur ou ajoutez-en un"; font.pixelSize: 16; color: Theme.textMuted }
        }
    }

    Rectangle {
        id: toast
        property string message: ""
        function show(text) {
            message = text;
            opacity = 1;
            toastTimer.restart();
        }
        opacity: 0
        visible: opacity > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        width: Math.min(parent.width - 32, toastText.implicitWidth + 36)
        height: toastText.implicitHeight + 24
        radius: 16
        color: Theme.surfaceHi
        border.width: 1
        border.color: Theme.border
        z: 10
        Text {
            id: toastText
            anchors.centerIn: parent
            width: parent.width - 28
            wrapMode: Text.Wrap
            text: toast.message
            font.pixelSize: 14
            color: Theme.text
        }
        Timer { id: toastTimer; interval: 3000; onTriggered: toast.opacity = 0 }
    }
}
