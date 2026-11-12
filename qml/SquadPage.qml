import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import "DateUtil.js" as DateUtil
import "Status.js" as Status

// Page « Effectif » : choix de l'équipe, recherche, filtre par poste, liste des joueurs et fiche. En grand écran, liste
// et fiche côte à côte (sans fiche ouverte : vue d'ensemble de l'effectif) ; sur tablette, grille de cartes joueurs ;
// sur téléphone, l'une après l'autre.
Item {
    id: page

    property var teams: []
    property int teamId: AppState.teamId           // partagé avec les autres pages
    onTeamIdChanged: {
        clearSelection();
        if (AppState.teamId !== teamId)
            AppState.teamId = teamId;
    }
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
    // Sélection multiple (appui long sur un joueur) : cases à cocher puis suppression groupée.
    property bool selecting: false
    property var selected: ({})           // id -> true
    readonly property int selectedCount: Object.keys(selected).length
    readonly property bool allShownSelected: {
        var list = shown;
        if (list.length === 0)
            return false;
        for (var i = 0; i < list.length; ++i)
            if (!selected[list[i].id])
                return false;
        return true;
    }
    readonly property bool split: width >= 900
    readonly property int listWidth: 380
    // Tablette (page trop étroite pour liste + fiche) sans fiche ouverte : grille de cartes joueurs.
    readonly property bool gridMode: width >= 600 && height >= 520 && !split && !editing
    readonly property int pad: gridMode ? Theme.gutter + 4 : Theme.gutter       // aligné sur le titre d'en-tête
    readonly property int gridGap: 14
    readonly property int gridCols: Math.max(1, Math.floor((width - 2 * pad + gridGap) / (250 + gridGap)))
    // Vue d'ensemble (calculée sur les joueurs actifs de l'équipe ; présence : backend.attendanceStats, 30 jours).
    property var overview: ({ count: 0, avgAge: -1, byPosition: ({}), rate: -1, top: [] })
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
        loadOverview();
    }

    function loadOverview() {
        var by = { GK: 0, DEF: 0, MID: 0, FWD: 0 };
        var n = 0, ageSum = 0, ageN = 0;
        for (var i = 0; i < players.length; ++i) {
            var p = players[i];
            if (!p.active)
                continue;
            n++;
            if (by[p.position] !== undefined)
                by[p.position]++;
            if (p.age >= 0) {
                ageSum += p.age;
                ageN++;
            }
        }
        var rate = -1, top = [];
        if (teamId > 0 && n > 0) {
            var today = DateUtil.todayIso();
            var st = backend.attendanceStats(teamId, DateUtil.addDays(today, -30), today);
            var sum = 0, m = 0, ranked = [];
            for (var j = 0; j < st.players.length; ++j) {
                if (st.players[j].rate >= 0) {
                    sum += st.players[j].rate;
                    m++;
                    ranked.push(st.players[j]);
                }
            }
            rate = m > 0 ? sum / m : -1;
            ranked.sort(function (a, b) { return b.rate - a.rate; });
            top = ranked.slice(0, 5);
        }
        overview = { count: n, avgAge: ageN > 0 ? Math.round(ageSum / ageN) : -1, byPosition: by, rate: rate, top: top };
    }

    function startSelection(p) {
        if (page.editing)
            page.closeForm();
        var m = {};
        m[p.id] = true;
        selected = m;
        selecting = true;
    }
    function toggleSelected(id) {
        var m = Object.assign({}, selected);
        if (m[id])
            delete m[id];
        else
            m[id] = true;
        selected = m;
    }
    function selectAllShown(on) {
        var m = Object.assign({}, selected);
        for (var i = 0; i < shown.length; ++i) {
            if (on)
                m[shown[i].id] = true;
            else
                delete m[shown[i].id];
        }
        selected = m;
    }
    function clearSelection() {
        selecting = false;
        selected = {};
    }
    function askDeleteSelected() {
        var n = selectedCount;
        if (n === 0)
            return;
        confirm.action = "deletePlayers";
        confirm.confirm(n > 1 ? "Supprimer " + n + " joueurs ?" : "Supprimer 1 joueur ?",
                        (n > 1 ? "Ces joueurs seront supprimés définitivement, avec leurs présences, leurs blessures et leurs photos."
                               : "Ce joueur sera supprimé définitivement, avec ses présences, ses blessures et sa photo.")
                        + "\n\nPour garder l'historique des présences, désactivez plutôt les joueurs qui ont quitté l'équipe.",
                        "Supprimer", true);
    }
    // Déplacement vers une autre équipe : choix de l'équipe, confirmation, compte rendu.
    readonly property var otherTeams: teams.filter(function (t) { return t.id !== teamId; })
    function askMoveSelected() {
        if (selectedCount === 0 || otherTeams.length === 0)
            return;
        var m = [];
        for (var i = 0; i < otherTeams.length; ++i)
            m.push({ id: "t" + otherTeams[i].id, icon: "users",
                     text: otherTeams[i].name + (otherTeams[i].clubName ? " · " + otherTeams[i].clubName : "") });
        moveSheet.model = m;
        moveSheet.open();
    }
    function teamName(id) {
        for (var i = 0; i < teams.length; ++i)
            if (teams[i].id === id)
                return teams[i].name;
        return "";
    }
    function confirmMove(target) {
        var n = selectedCount;
        var name = teamName(target);
        confirm.action = "movePlayers";
        confirm.moveTarget = target;
        confirm.confirm(n > 1 ? "Déplacer " + n + " joueurs ?" : "Déplacer 1 joueur ?",
                        (n > 1 ? "Ces joueurs passeront" : "Ce joueur passera") + " dans l'équipe « " + name + " ». "
                        + "Leurs présences, blessures, photos et contacts les suivent.\n\n"
                        + "Un numéro de maillot déjà pris dans « " + name + " » sera retiré. Un joueur qui y existe déjà "
                        + "(même nom ou même licence) ne sera pas déplacé.",
                        "Déplacer", false);
    }
    function moveSelected(target) {
        var ids = Object.keys(selected).map(function (k) { return parseInt(k); });
        var r = backend.movePlayers(ids, target);
        if (!r.ok) {
            toast.show(r.error);
            return;
        }
        clearSelection();
        refreshTeams();
        var name = teamName(target);
        var msg = r.moved === 0 ? "Aucun joueur déplacé"
                : (r.moved > 1 ? r.moved + " joueurs déplacés" : "1 joueur déplacé") + " vers " + name;
        if (r.numbersCleared > 0)
            msg += " · " + (r.numbersCleared > 1 ? r.numbersCleared + " numéros retirés" : "1 numéro retiré");
        if (r.skipped.length > 0) {
            info.confirm("Déplacement", msg + ".\n\nDéjà présents dans « " + name + " », non déplacés : "
                         + r.skipped.join(", ") + ".", "OK", false);
            info.cancelVisible = false;
        } else {
            toast.show(msg);
        }
    }

    function deleteSelected() {
        var ids = Object.keys(selected).map(function (k) { return parseInt(k); });
        var r = backend.deletePlayers(ids);
        if (r.ok) {
            clearSelection();
            refreshTeams();
            toast.show(r.removed > 1 ? r.removed + " joueurs supprimés" : "Joueur supprimé");
        } else {
            toast.show(r.error);
        }
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

    // Bouton retour du téléphone : ferme la fiche ouverte. Retourne true si la page a utilisé le geste.
    function goBack() {
        if (selecting) {
            clearSelection();
            return true;
        }
        if (!editing)
            return false;
        closeForm();
        return true;
    }

    function subtitleText() {
        if (!team)
            return "Aucune équipe";
        var n = players.length;
        var count = n === 0 ? "aucun joueur" : n === 1 ? "1 joueur" : n + " joueurs";
        return team.name + " · " + count;
    }

    Connections {
        target: backend
        function onDataReplaced() {
            page.closeForm();
            page.refreshTeams();
        }
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

    // Saisie du nom d'un club (création ou renommage).
    PromptDialog {
        id: clubPrompt
        objectName: "clubPrompt"
        property int clubId: -1                       // -1 : nouveau club
        onAccepted: (text) => {
            var r = backend.saveClub({ id: clubId, name: text });
            if (r.ok) {
                page.refreshTeams();
                toast.show(clubId > 0 ? "Club renommé" : "Club créé");
            } else {
                toast.show(r.error);
            }
        }
    }

    PromptDialog {
        id: confirm
        property string action: ""
        property int moveTarget: -1
        onAccepted: {
            if (action === "movePlayers") {
                page.moveSelected(moveTarget);
            } else if (action === "deletePlayers") {
                page.deleteSelected();
            } else if (action === "deletePlayer") {
                var r = backend.deletePlayer(page.currentId);
                if (r.ok) {
                    page.closeForm();
                    page.refreshTeams();
                } else {
                    toast.show(r.error);
                }
            } else if (action === "deleteClub") {
                var c = backend.deleteClub(page.team.clubId);
                if (c.ok) {
                    page.closeForm();
                    page.teamId = -1;
                    page.refreshTeams();
                } else {
                    toast.show(c.error);
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

    PromptDialog { id: info; objectName: "squadInfo" }
    ActionSheet {
        id: moveSheet
        objectName: "moveSheet"
        title: "Déplacer vers…"
        onChosen: (id) => page.confirmMove(parseInt(id.substring(1)))
    }

    PlayersImportDialog {
        id: importDialog
        objectName: "importDialog"
        onImported: (n) => {
            page.refreshTeams();
            toast.show(n + (n > 1 ? " joueurs ajoutés." : " joueur ajouté."));
        }
    }

    // Import en lot des photos : fichiers nommés d'après le numéro de licence (voir Backend::importPhotos).
    FileDialog {
        id: photosDialog
        objectName: "photosDialog"
        title: "Choisir les photos"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Images (*.jpg *.jpeg *.png *.webp *.bmp)", "Tous les fichiers (*)"]
        onAccepted: page.importPhotos(selectedFiles)
    }

    PhotosImportResult {
        id: photosResult
        objectName: "photosResult"
    }

    function importPhotos(files) {
        var urls = [];
        for (var i = 0; i < files.length; ++i)
            urls.push(files[i].toString());
        var r = backend.importPhotos(page.teamId, urls);
        if (!r.ok) {
            toast.show(r.error);
            return;
        }
        refreshTeams();
        photosResult.show(r);
    }

    ActionSheet {
        id: teamSheet
        title: "Équipe"
        function rebuild() {
            var m = [];
            var several = backend.clubs().length > 1;
            for (var i = 0; i < page.teams.length; ++i)
                m.push({ id: "t" + page.teams[i].id, text: page.teams[i].name
                         + (page.teams[i].category ? " · " + page.teams[i].category : "")
                         + (several ? " — " + page.teams[i].clubName : ""),
                         icon: "users", checked: page.teams[i].id === page.teamId });
            m.push({ id: "new", text: "Nouvelle équipe", icon: "plus" });
            if (page.team) {
                m.push({ id: "edit", text: "Modifier l'équipe", icon: "edit" });
                m.push({ id: "delete", text: "Supprimer l'équipe", icon: "trash", danger: true });
            }
            m.push({ id: "newClub", text: "Nouveau club", icon: "plus" });
            if (page.team) {
                m.push({ id: "renameClub", text: "Renommer le club « " + page.team.clubName + " »", icon: "edit" });
                if (several)
                    m.push({ id: "deleteClub", text: "Supprimer le club « " + page.team.clubName + " »", icon: "trash", danger: true });
            }
            model = m;
        }
        onChosen: (id) => {
            if (id === "new") {
                teamDialog.openFor(null, page.team ? page.team.clubId : -1);
            } else if (id === "newClub") {
                clubPrompt.clubId = -1;
                clubPrompt.ask("Nouveau club", "", false, 60);
            } else if (id === "renameClub") {
                clubPrompt.clubId = page.team.clubId;
                clubPrompt.ask("Renommer le club", page.team.clubName, false, 60);
            } else if (id === "deleteClub") {
                confirm.action = "deleteClub";
                confirm.confirm("Supprimer le club ?",
                                "Le club « " + page.team.clubName + " » sera supprimé avec toutes ses équipes, leurs joueurs, "
                                + "séances, matchs et présences. Cette action est définitive.", "Supprimer", true);
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
                objectName: "importButton"
                visible: page.team !== null
                iconName: "text"
                fillColor: Theme.surface
                accessibleName: "Importer une liste de joueurs"
                onClicked: importDialog.openFor(page.teamId)
            }
            IconButton {
                objectName: "photosButton"
                visible: page.team !== null
                iconName: "photo"
                fillColor: Theme.surface
                accessibleName: "Importer des photos"
                onClicked: photosDialog.open()
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

        // Barre de sélection : remplace l'en-tête pendant la sélection multiple.
        Rectangle {
            id: selectionBar
            objectName: "selectionBar"
            visible: page.selecting
            anchors.fill: listHeader
            z: 3
            color: Theme.bg
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.border
            }
            MouseArea { anchors.fill: parent }        // l'en-tête dessous ne reçoit rien
            IconButton {
                id: selClose
                anchors.left: parent.left
                anchors.leftMargin: Theme.gutter - 6
                anchors.verticalCenter: parent.verticalCenter
                iconName: "close"
                accessibleName: "Annuler la sélection"
                onClicked: page.clearSelection()
            }
            Column {
                anchors.left: selClose.right
                anchors.leftMargin: 8
                anchors.right: selAll.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                Text {
                    objectName: "selectionCount"
                    width: parent.width
                    elide: Text.ElideRight
                    text: page.selectedCount === 0 ? "Aucun joueur"
                          : page.selectedCount + (page.selectedCount > 1 ? " sélectionnés" : " sélectionné")
                    font.family: Theme.fontHeading
                    font.pixelSize: selectionBar.width < 480 ? 19 : 22
                    font.weight: Font.Bold
                    color: Theme.text
                }
                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: "Touchez pour cocher"
                    font.pixelSize: 12
                    color: Theme.textMuted
                }
            }
            Rectangle {
                id: selAll
                objectName: "selectAllButton"
                anchors.right: moveButton.visible ? moveButton.left : trashButton.left
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                height: 36
                width: selAllText.implicitWidth + 24
                radius: 18
                color: selAllArea.pressed ? Theme.surfaceHi : Theme.surface
                border.width: 1
                border.color: Theme.border
                Accessible.role: Accessible.Button
                Accessible.name: selAllText.text
                Text {
                    id: selAllText
                    anchors.centerIn: parent
                    text: page.allShownSelected ? "Aucun" : "Tous"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
                MouseArea {
                    id: selAllArea
                    anchors.fill: parent
                    onClicked: page.selectAllShown(!page.allShownSelected)
                }
            }
            IconButton {
                id: moveButton
                objectName: "moveSelectedButton"
                visible: page.otherTeams.length > 0
                anchors.right: trashButton.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                enabled: page.selectedCount > 0
                iconName: "move"
                fillColor: Theme.surface
                borderColor: Theme.border
                accessibleName: "Déplacer vers une autre équipe"
                onClicked: page.askMoveSelected()
            }
            IconButton {
                id: trashButton
                objectName: "deleteSelectedButton"
                anchors.right: parent.right
                anchors.rightMargin: Theme.gutter
                anchors.verticalCenter: parent.verticalCenter
                enabled: page.selectedCount > 0
                iconName: "trash"
                iconColor: Theme.text
                fillColor: Theme.danger
                diameter: 48
                glow: true
                accessibleName: "Supprimer les joueurs sélectionnés"
                onClicked: page.askDeleteSelected()
            }
        }

        Column {
            id: filters
            visible: page.team !== null
            anchors.top: listHeader.bottom
            anchors.left: parent.left
            anchors.leftMargin: page.pad
            anchors.right: parent.right
            anchors.rightMargin: page.pad
            anchors.topMargin: 4
            spacing: 10

            Rectangle {
                width: page.gridMode ? Math.min(parent.width, 480) : parent.width
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
            visible: !page.gridMode
            anchors.top: filters.visible ? filters.bottom : listHeader.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 10
            leftMargin: page.pad
            rightMargin: page.pad
            bottomMargin: 16
            spacing: 10
            clip: true
            model: page.team ? page.shown : []
            boundsBehavior: Flickable.StopAtBounds

            delegate: Card {
                id: row
                required property var modelData
                objectName: "playerRow"
                width: list.width - list.leftMargin - list.rightMargin
                height: 72
                padding: 12
                highlighted: (page.split && page.editing && page.currentId === modelData.id)
                             || (page.selecting && page.selected[modelData.id] === true)
                opacity: modelData.active ? 1 : 0.55

                PlayerAvatar {
                    id: badge
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    size: 46
                    playerId: row.modelData.id
                    hasPhoto: row.modelData.hasPhoto === true
                    number: row.modelData.jerseyNumber
                    maskColor: row.color
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
                    visible: !page.selecting
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    name: "chevron"
                    color: Theme.textDim
                    size: 20
                }
                SelectMark {
                    visible: page.selecting
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    checked: page.selected[row.modelData.id] === true
                }
                MouseArea {
                    objectName: "playerRowArea"
                    anchors.fill: parent
                    pressAndHoldInterval: 450
                    onClicked: page.selecting ? page.toggleSelected(row.modelData.id) : page.openPlayer(row.modelData)
                    onPressAndHold: page.selecting ? page.toggleSelected(row.modelData.id) : page.startSelection(row.modelData)
                }
            }

            footer: emptyState
        }

        // Grille de cartes joueurs (tablette) : mêmes données et filtres que la liste.
        GridView {
            id: grid
            objectName: "playerGrid"
            visible: page.gridMode
            anchors.top: filters.visible ? filters.bottom : listHeader.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 14
            leftMargin: page.pad
            rightMargin: page.pad - page.gridGap
            bottomMargin: 16
            clip: true
            model: page.team ? page.shown : []
            boundsBehavior: Flickable.StopAtBounds
            cellWidth: (width - leftMargin - rightMargin) / page.gridCols
            cellHeight: 96 + page.gridGap
            footer: emptyState

            delegate: Item {
                id: cell
                required property var modelData
                width: grid.cellWidth
                height: grid.cellHeight
                Card {
                    objectName: "playerCard"
                    width: cell.width - page.gridGap
                    height: 96
                    padding: 14
                    radius: Theme.radiusLg
                    highlighted: page.selecting && page.selected[cell.modelData.id] === true
                    opacity: cell.modelData.active ? 1 : 0.55
                    PlayerAvatar {
                        id: gbadge
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        size: 52
                        numberPixelSize: 20
                        numberFont: Theme.fontHeading
                        playerId: cell.modelData.id
                        hasPhoto: cell.modelData.hasPhoto === true
                        number: cell.modelData.jerseyNumber
                        maskColor: Theme.surface
                    }
                    Column {
                        anchors.left: gbadge.right
                        anchors.leftMargin: 14
                        anchors.right: gchevron.left
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3
                        Text {
                            width: parent.width
                            text: cell.modelData.fullName
                            elide: Text.ElideRight
                            font.family: Theme.fontHeading
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            color: Theme.textMuted
                            text: {
                                var parts = [];
                                if (cell.modelData.position)
                                    parts.push(page.positionLabels[cell.modelData.position].toUpperCase());
                                if (cell.modelData.age >= 0)
                                    parts.push(cell.modelData.age + " ANS");
                                if (!cell.modelData.active)
                                    parts.push("INACTIF");
                                return parts.length > 0 ? parts.join("  ·  ") : "—";
                            }
                        }
                    }
                    Icon {
                        id: gchevron
                        visible: !page.selecting
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        name: "chevron"
                        color: Theme.accent
                        size: 20
                    }
                    SelectMark {
                        visible: page.selecting
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        checked: page.selected[cell.modelData.id] === true
                    }
                    MouseArea {
                        anchors.fill: parent
                        pressAndHoldInterval: 450
                        onClicked: page.selecting ? page.toggleSelected(cell.modelData.id) : page.openPlayer(cell.modelData)
                        onPressAndHold: page.selecting ? page.toggleSelected(cell.modelData.id) : page.startSelection(cell.modelData)
                    }
                }
            }
        }

        Component {
            id: emptyState
            Item {
                width: page.gridMode ? page.width - 2 * page.pad : list.width - 2 * page.pad
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
                        AppButton {
                            objectName: "importEmptyButton"
                            visible: page.team !== null && page.players.length === 0
                            iconName: "text"
                            text: "Importer une liste"
                            onClicked: importDialog.openFor(page.teamId)
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

    // Grand écran sans fiche ouverte : vue d'ensemble de l'effectif (ou invite à créer joueur / équipe).
    Item {
        id: overviewPane
        objectName: "overviewPane"
        visible: page.split && !page.editing
        x: page.listWidth
        width: page.width - x
        height: page.height
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.border }
        Column {
            visible: page.team === null || page.overview.count === 0
            anchors.centerIn: parent
            spacing: 12
            Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "users"; color: Theme.textDim; size: 56 }
            Text { text: "Choisissez un joueur ou ajoutez-en un"; font.pixelSize: 16; color: Theme.textMuted }
        }
        Flickable {
            visible: page.team !== null && page.overview.count > 0
            anchors.fill: parent
            anchors.leftMargin: 1
            contentWidth: width
            contentHeight: ovCol.height + 48
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            Column {
                id: ovCol
                x: 24
                y: 20
                width: Math.min(parent.width - 48, 760)
                spacing: 18
                Column {
                    spacing: 2
                    Text {
                        text: "Vue d'ensemble"
                        font.family: Theme.fontHeading
                        font.pixelSize: 24
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        text: (page.team ? page.team.name : "") + " · joueurs actifs · choisissez un joueur pour ouvrir sa fiche"
                        font.pixelSize: 13
                        color: Theme.textMuted
                    }
                }
                Row {
                    spacing: 12
                    Repeater {
                        model: [
                            { label: "JOUEURS ACTIFS", value: page.overview.count + "", color: Theme.accent },
                            { label: "ÂGE MOYEN", value: page.overview.avgAge < 0 ? "—" : page.overview.avgAge + " ans", color: Theme.info },
                            { label: "PRÉSENCE · 30 J", value: page.overview.rate < 0 ? "—" : Math.round(page.overview.rate * 100) + " %",
                              color: page.overview.rate < 0 ? Theme.textMuted : Status.rateColor(page.overview.rate) }
                        ]
                        delegate: Rectangle {
                            required property var modelData
                            width: (ovCol.width - 24) / 3
                            height: 100
                            radius: Theme.radiusLg
                            color: Theme.surface
                            border.width: 1
                            border.color: Theme.border
                            Column {
                                anchors.centerIn: parent
                                spacing: 6
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: parent.parent.modelData.label
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    font.letterSpacing: 1
                                    color: Theme.textMuted
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: parent.parent.modelData.value
                                    font.family: Theme.fontHeading
                                    font.pixelSize: 32
                                    font.weight: Font.ExtraBold
                                    color: parent.parent.modelData.color
                                }
                            }
                        }
                    }
                }
                Card {
                    width: parent.width
                    height: posCol.height + 2 * padding
                    padding: 20
                    radius: Theme.radiusLg
                    Column {
                        id: posCol
                        width: parent.width
                        spacing: 14
                        Text {
                            text: "Répartition par poste"
                            font.family: Theme.fontHeading
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Repeater {
                            model: [{ id: "GK", text: "Gardiens" }, { id: "DEF", text: "Défenseurs" },
                                    { id: "MID", text: "Milieux" }, { id: "FWD", text: "Attaquants" }]
                            delegate: Item {
                                required property var modelData
                                readonly property int n: page.overview.byPosition[modelData.id] || 0
                                width: posCol.width
                                height: 24
                                Text {
                                    id: plabel
                                    width: 110
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: parent.modelData.text
                                    font.pixelSize: 14
                                    color: Theme.textMuted
                                }
                                Rectangle {
                                    anchors.left: plabel.right
                                    anchors.right: pcount.left
                                    anchors.rightMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    height: 8
                                    radius: 4
                                    color: Theme.bgDeep
                                    Rectangle {
                                        width: page.overview.count > 0 ? parent.width * parent.parent.n / page.overview.count : 0
                                        height: parent.height
                                        radius: 4
                                        color: Theme.accent
                                    }
                                }
                                Text {
                                    id: pcount
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: parent.n
                                    font.family: Theme.fontHeading
                                    font.pixelSize: 16
                                    font.weight: Font.Bold
                                    color: Theme.text
                                }
                            }
                        }
                    }
                }
                Card {
                    visible: page.overview.top.length > 0
                    width: parent.width
                    height: topCol.height + 2 * padding
                    padding: 20
                    radius: Theme.radiusLg
                    Column {
                        id: topCol
                        width: parent.width
                        spacing: 12
                        Text {
                            text: "Meilleure présence · 30 jours"
                            font.family: Theme.fontHeading
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Repeater {
                            model: page.overview.top
                            delegate: Item {
                                required property var modelData
                                width: topCol.width
                                height: 24
                                Text {
                                    id: tname
                                    width: 170
                                    anchors.verticalCenter: parent.verticalCenter
                                    elide: Text.ElideRight
                                    text: parent.modelData.name
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                    color: Theme.text
                                }
                                Rectangle {
                                    anchors.left: tname.right
                                    anchors.right: trate.left
                                    anchors.rightMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    height: 8
                                    radius: 4
                                    color: Theme.bgDeep
                                    Rectangle {
                                        width: parent.width * Math.max(0, parent.parent.modelData.rate)
                                        height: parent.height
                                        radius: 4
                                        color: Status.rateColor(parent.parent.modelData.rate)
                                    }
                                }
                                Text {
                                    id: trate
                                    width: 44
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    horizontalAlignment: Text.AlignRight
                                    text: Math.round(parent.modelData.rate * 100) + " %"
                                    font.pixelSize: 14
                                    font.weight: Font.Bold
                                    color: Status.rateColor(parent.modelData.rate)
                                }
                            }
                        }
                    }
                }
            }
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
