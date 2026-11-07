import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil
import "Status.js" as Status

// Fiche d'un match, sur un seul écran : informations (date, heure, adversaire, lieu, domicile/extérieur, statut, score,
// notes), joueurs convoqués (titulaire ou remplaçant) et schéma tactique du match. Le schéma s'ouvre par-dessus la
// fiche (BoardEditor) ; il n'est créé qu'à la demande, une fois le match enregistré.
Item {
    id: root

    property int matchId: -1
    property int teamId: -1
    property string teamName: ""
    property bool showBack: true
    property string errorText: ""
    property var roster: []                    // joueurs actifs de l'équipe (backend.players)
    property var roles: ({})                   // playerId -> "starter" | "substitute" ; absent = non convoqué
    property int boardId: -1
    property string savedState: ""               // état enregistré, pour savoir s'il reste des modifications

    property bool boardOpen: false
    property int openBoardId: -1
    readonly property bool isNew: matchId <= 0
    readonly property bool dirty: formState() !== savedState
    // Convoqués du match (état courant de la fiche), titulaires d'abord : seuls joueurs posables sur son terrain.
    readonly property var convoked: {
        var starters = [], subs = [];
        for (var i = 0; i < roster.length; ++i) {
            var r = roles[roster[i].id];
            var e = { id: roster[i].id, name: roster[i].fullName, firstName: roster[i].firstName,
                      lastName: roster[i].lastName, position: roster[i].position || "",
                      foot: roster[i].preferredFoot || "", role: r };
            if (r === "starter")
                starters.push(e);
            else if (r === "substitute")
                subs.push(e);
        }
        return starters.concat(subs);
    }
    readonly property int starters: countRole("starter")
    readonly property int substitutes: countRole("substitute")

    signal closeRequested()
    signal changed()                           // le match a été enregistré ou supprimé : la liste se met à jour

    readonly property var statuses: [{ id: "planned", text: "Prévu" }, { id: "played", text: "Joué" },
                                     { id: "cancelled", text: "Annulé" }]
    readonly property var places: [{ id: "home", text: "Domicile" }, { id: "away", text: "Extérieur" }]

    function countRole(role) {
        var n = 0;
        for (var k in roles)
            if (roles[k] === role)
                ++n;
        return n;
    }

    function formState() {
        return JSON.stringify([dateField.text, timeField.text, opponent.text, location.text, place.value, status.value,
                               scoreFor.text, scoreAgainst.text, notes.text, roles]);
    }

    function loadRoster() {
        roster = teamId > 0 ? backend.players(teamId, false) : [];
    }

    function openNew(team, name, day) {
        if (!leave(false))
            return;
        matchId = -1;
        teamId = team;
        teamName = name;
        errorText = "";
        boardId = -1;
        dateField.text = DateUtil.display(day);
        timeField.text = "15:00";
        opponent.text = "";
        location.text = "";
        place.value = "home";
        status.value = "planned";
        scoreFor.text = "";
        scoreAgainst.text = "";
        notes.text = "";
        roles = ({});
        loadRoster();
        savedState = formState();
        scroll.contentY = 0;
    }

    function openMatch(id, name) {
        if (!leave(false))
            return;
        var r = backend.match(id);
        if (!r.ok) {
            toast.show(r.error);
            return;
        }
        var m = r.match;
        matchId = id;
        teamId = m.teamId;
        teamName = name;
        errorText = "";
        boardId = r.boardId;
        dateField.text = DateUtil.display(m.date);
        timeField.text = m.time;
        opponent.text = m.opponent;
        location.text = m.location;
        place.value = m.homeAway;
        status.value = m.status;
        scoreFor.text = m.scoreFor >= 0 ? String(m.scoreFor) : "";
        scoreAgainst.text = m.scoreAgainst >= 0 ? String(m.scoreAgainst) : "";
        notes.text = m.notes;
        var map = ({});
        for (var i = 0; i < r.players.length; ++i)
            map[r.players[i].playerId] = r.players[i].role;
        roles = map;
        loadRoster();
        savedState = formState();
        scroll.contentY = 0;
    }

    function setRole(playerId, role) {
        var map = ({});
        for (var k in roles)
            map[k] = roles[k];
        if (role === "")
            delete map[playerId];
        else
            map[playerId] = role;
        roles = map;
    }

    // Enregistre le match puis ses convoqués. Faux (rien d'autre n'est fait) si le match est refusé.
    function save() {
        var day = DateUtil.parse(dateField.text);
        var time = DateUtil.parseTime(timeField.text);
        var sf = scoreFor.text.trim() === "" ? -1 : parseInt(scoreFor.text, 10);
        var sa = scoreAgainst.text.trim() === "" ? -1 : parseInt(scoreAgainst.text, 10);
        if ((scoreFor.text.trim() !== "" && isNaN(sf)) || (scoreAgainst.text.trim() !== "" && isNaN(sa))) {
            errorText = "Le score doit être un nombre.";
            scroll.contentY = 0;
            return false;
        }
        var r = backend.saveMatch({ id: matchId, teamId: teamId, date: day, time: time, opponent: opponent.text,
                                    location: location.text, homeAway: place.value || "home",
                                    status: status.value || "planned", scoreFor: sf, scoreAgainst: sa,
                                    notes: notes.text });
        if (!r.ok) {
            errorText = r.error;
            scroll.contentY = 0;
            return false;
        }
        var entries = [];
        for (var k in roles)
            entries.push({ playerId: parseInt(k, 10), role: roles[k] });
        var p = backend.setMatchPlayers(r.id, entries);
        matchId = r.id;                                   // le match existe désormais, même si les convoqués échouent
        if (!p.ok) {
            errorText = "Match enregistré, mais pas les convoqués : " + p.error;
            scroll.contentY = 0;
            changed();
            return false;
        }
        errorText = "";
        savedState = formState();
        changed();
        toast.show("Match enregistré");
        return true;
    }

    function remove() {
        var r = backend.deleteMatch(matchId);
        if (!r.ok) {
            toast.show(r.error);
            return;
        }
        leave(true);
        matchId = -1;
        savedState = formState();
        changed();
        closeRequested();
    }

    // --- Garde de sortie : terrain ouvert puis modifications non enregistrées -------------------------

    // Exécute action une fois le terrain ouvert enregistré et les modifications de la fiche enregistrées ou abandonnées
    // (confirmation). Si l'enregistrement du terrain échoue, on reste sur le terrain.
    function guardLeave(action) {
        var afterBoard = function () {
            if (!dirty) {
                action();
                return;
            }
            leaveConfirm.pending = action;
            leaveConfirm.confirm("Quitter sans enregistrer ?",
                                 "Les modifications de ce match ne sont pas enregistrées.", "Quitter", true);
        };
        if (boardOpen && boardLoader.item)
            boardLoader.item.leaveGuarded(afterBoard);
        else
            afterBoard();
    }

    function showSaveFailed(action) {
        if (boardOpen && boardLoader.item)
            boardLoader.item.showSaveFailed(action);
    }

    // Vrai si rien n'est en attente pour le terrain ; la fiche elle-même passe par guardLeave().
    function flushBoard() {
        return !boardOpen || !boardLoader.item || boardLoader.item.flush();
    }

    // Vrai si la fenêtre peut se fermer sans perdre de modifications de la fiche.
    function hasUnsaved() { return dirty; }

    function leave(discard) {
        if (!boardOpen)
            return true;
        if (boardLoader.item) {
            if (discard === true)
                boardLoader.item.discard();
            else if (!boardLoader.item.flush())
                return false;
        }
        boardOpen = false;
        openBoardId = -1;
        return true;
    }

    function goBack() {
        if (boardOpen) {
            if (boardLoader.item && boardLoader.item.fullscreen)
                boardLoader.item.fullscreen = false;
            else
                closeBoard();
            return true;
        }
        return false;
    }

    // --- Schéma tactique -----------------------------------------------------------------------------

    function openBoard() {
        if (isNew) {
            errorText = "Enregistrez d'abord le match pour lui ajouter un schéma tactique.";
            scroll.contentY = 0;
            return;
        }
        var id = boardId;
        if (!(id > 0)) {
            var r = backend.createMatchBoard(matchId);
            if (!r.ok) {
                toast.show(r.error);
                return;
            }
            id = r.boardId;
            boardId = id;
        }
        openBoardId = id;
        boardOpen = true;
    }

    function closeBoard() {
        if (!boardOpen)
            return;
        var close = function () {
            boardOpen = false;
            openBoardId = -1;
            var r = backend.match(matchId);              // le schéma a pu être retiré
            boardId = r.ok ? r.boardId : -1;
        };
        if (boardLoader.item)
            boardLoader.item.leaveGuarded(close);
        else
            close();
    }

    // Fiche jamais ouverte : rien à enregistrer (sinon la garde de sortie la croirait modifiée).
    Component.onCompleted: savedState = formState()

    PromptDialog {
        id: leaveConfirm
        property var pending: null
        onAccepted: {
            var action = pending;
            pending = null;
            if (action) {
                savedState = formState();         // abandon voulu
                action();
            }
        }
    }
    PromptDialog {
        id: deleteConfirm
        onAccepted: root.remove()
    }
    PromptDialog {
        id: removeBoardConfirm
        objectName: "removeMatchBoardConfirm"
        onAccepted: {
            var id = root.openBoardId;
            if (!(id > 0) || !backend.deleteBoard(id)) {
                // Suppression en échec : le schéma reste ouvert, enregistrable, et l'erreur est affichée.
                toast.show("Impossible de retirer le schéma : " + backend.lastError);
                return;
            }
            if (boardLoader.item)
                boardLoader.item.discard();           // plus d'enregistrement vers un schéma supprimé
            root.boardOpen = false;
            root.openBoardId = -1;
            root.boardId = -1;
        }
    }
    DatePicker {
        id: datePicker
        title: "Date du match"
        maxYear: new Date().getFullYear() + 2
        yearCount: 6
        onPicked: (iso) => dateField.text = DateUtil.display(iso)
    }
    TimePicker {
        id: timePicker
        title: "Heure du coup d'envoi"
        onPicked: (t) => timeField.text = t
    }
    ActionSheet {
        id: moreSheet
        title: "Match"
        model: [{ id: "delete", text: "Supprimer le match", icon: "trash", danger: true }]
        onChosen: deleteConfirm.confirm("Supprimer le match ?",
            "Le match contre « " + opponent.text + " », ses convocations et son schéma tactique seront supprimés. "
            + "Cette action est définitive.", "Supprimer", true)
    }

    PageHeader {
        id: header
        width: parent.width
        title: root.isNew ? "Nouveau match" : "Match"
        subtitle: root.teamName
        leftInset: root.showBack ? 40 : 0
        IconButton {
            visible: !root.isNew
            iconName: "more"
            accessibleName: "Actions du match"
            onClicked: moreSheet.open()
        }
        IconButton {
            objectName: "saveMatch"
            iconName: "check"
            iconColor: Theme.textOnAccent
            fillColor: Theme.accent
            diameter: 48
            glow: true
            accessibleName: "Enregistrer le match"
            onClicked: root.save()
        }
    }
    IconButton {
        visible: root.showBack
        anchors.left: parent.left
        anchors.leftMargin: 2
        anchors.verticalCenter: header.verticalCenter
        z: 2
        iconName: "back"
        accessibleName: "Retour"
        onClicked: root.closeRequested()
    }

    EnsureVisible { flick: scroll }

    Flickable {
        id: scroll
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        contentWidth: width
        contentHeight: form.height + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        Column {
            id: form
            x: Math.max(Theme.gutter, (scroll.width - width) / 2)
            y: 8
            width: Math.min(scroll.width - 2 * Theme.gutter, 560)
            spacing: 16

            Text {
                visible: root.errorText !== ""
                objectName: "matchError"
                width: parent.width
                text: root.errorText
                wrapMode: Text.Wrap
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: Theme.danger
            }
            FormField { id: opponent; objectName: "opponentField"; width: parent.width; label: "Adversaire"; required: true; placeholder: "Ex. FC Rivière"; maxLength: 80 }
            Row {
                width: parent.width
                spacing: 12
                FormField {
                    id: dateField
                    width: (parent.width - 12) * 0.6
                    label: "Date"
                    required: true
                    placeholder: "JJ/MM/AAAA"
                    maxLength: 10
                    inputHints: Qt.ImhPreferNumbers
                    actionIcon: "calendar"
                    actionName: "Choisir la date"
                    onActionClicked: datePicker.openAt(DateUtil.parse(dateField.text), new Date().getFullYear())
                    onEditingFinished: {
                        var iso = DateUtil.parse(dateField.text);
                        if (DateUtil.isIso(iso))
                            dateField.text = DateUtil.display(iso);
                    }
                }
                FormField {
                    id: timeField
                    width: (parent.width - 12) * 0.4
                    label: "Coup d'envoi"
                    required: true
                    placeholder: "HH:MM"
                    maxLength: 5
                    inputHints: Qt.ImhPreferNumbers
                    actionIcon: "clock"
                    actionName: "Choisir l'heure"
                    onActionClicked: timePicker.openAt(DateUtil.parseTime(timeField.text))
                    onEditingFinished: {
                        var t = DateUtil.parseTime(timeField.text);
                        if (DateUtil.isTime(t))
                            timeField.text = t;
                    }
                }
            }
            ChipRow { id: place; width: parent.width; label: "Lieu du match"; options: root.places; allowClear: false; value: "home" }
            FormField { id: location; width: parent.width; label: "Stade ou adresse"; placeholder: "Ex. Stade municipal"; maxLength: 120 }
            ChipRow { id: status; width: parent.width; label: "Statut"; options: root.statuses; allowClear: false; value: "planned" }
            Row {
                width: parent.width
                spacing: 12
                FormField { id: scoreFor; objectName: "scoreFor"; width: (parent.width - 12) / 2; label: "Buts marqués"; placeholder: "—"; maxLength: 2; inputHints: Qt.ImhDigitsOnly }
                FormField { id: scoreAgainst; objectName: "scoreAgainst"; width: (parent.width - 12) / 2; label: "Buts encaissés"; placeholder: "—"; maxLength: 2; inputHints: Qt.ImhDigitsOnly }
            }
            FormField { id: notes; width: parent.width; label: "Notes"; multiline: true; maxLength: 1000 }

            // --- Convoqués ---
            Card {
                width: parent.width
                Column {
                    width: parent.width
                    spacing: 10
                    Text {
                        width: parent.width
                        text: "Joueurs convoqués"
                        font.pixelSize: 17
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        objectName: "convokedSummary"
                        width: parent.width
                        text: root.starters + " titulaire(s) · " + root.substitutes + " remplaçant(s)"
                        font.pixelSize: 13
                        color: Theme.textMuted
                    }
                    Text {
                        visible: root.roster.length === 0
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: "Cette équipe n'a aucun joueur actif : ajoutez-en dans Effectif."
                        font.pixelSize: 14
                        color: Theme.textMuted
                    }
                    Repeater {
                        model: root.roster
                        delegate: Item {
                            id: prow
                            required property var modelData
                            readonly property string role: root.roles[modelData.id] || ""
                            width: form.width - 32
                            height: 52
                            Text {
                                anchors.left: parent.left
                                anchors.right: roleRow.left
                                anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                text: (prow.modelData.jerseyNumber > 0 ? prow.modelData.jerseyNumber + " · " : "") + prow.modelData.fullName
                                elide: Text.ElideRight
                                font.pixelSize: 15
                                font.weight: prow.role !== "" ? Font.Bold : Font.Normal
                                color: prow.role !== "" ? Theme.text : Theme.textMuted
                            }
                            Row {
                                id: roleRow
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 6
                                Repeater {
                                    model: [{ id: "starter", text: "Tit." }, { id: "substitute", text: "Remp." }]
                                    delegate: Rectangle {
                                        required property var modelData
                                        objectName: "role_" + prow.modelData.id + "_" + modelData.id
                                        readonly property bool on: prow.role === modelData.id
                                        width: 56
                                        height: 40
                                        radius: 20
                                        color: on ? Theme.accentSoft : Theme.surface
                                        border.width: on ? 2 : 1
                                        border.color: on ? Theme.accent : Theme.border
                                        Accessible.role: Accessible.Button
                                        Accessible.name: prow.modelData.fullName + " : " + (modelData.id === "starter" ? "titulaire" : "remplaçant")
                                        Text {
                                            anchors.centerIn: parent
                                            text: parent.modelData.text
                                            font.pixelSize: 13
                                            font.weight: Font.DemiBold
                                            color: parent.on ? Theme.text : Theme.textMuted
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: root.setRole(prow.modelData.id, parent.on ? "" : parent.modelData.id)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // --- Schéma tactique ---
            Card {
                width: parent.width
                Column {
                    width: parent.width
                    spacing: 10
                    Text {
                        width: parent.width
                        text: "Schéma tactique"
                        font.pixelSize: 17
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: root.isNew ? "Enregistrez le match pour lui ajouter un schéma tactique."
                              : root.boardId > 0 ? "Ce match a un schéma tactique." : "Aucun schéma pour ce match."
                        font.pixelSize: 14
                        color: Theme.textMuted
                    }
                    AppButton {
                        objectName: "openMatchBoard"
                        width: parent.width
                        kind: "outline"
                        iconName: "board"
                        enabled: !root.isNew
                        text: root.boardId > 0 ? "Ouvrir le schéma" : "Créer le schéma"
                        onClicked: root.openBoard()
                    }
                }
            }

            AppButton {
                width: parent.width
                kind: "primary"
                text: "Enregistrer"
                iconName: "check"
                onClicked: root.save()
            }
        }
    }

    // Éditeur du schéma du match, par-dessus la fiche (qui reste chargée dessous : saisie et défilement conservés).
    Rectangle {
        id: boardLayer
        objectName: "matchBoardLayer"
        anchors.fill: parent
        visible: root.boardOpen
        z: 10
        color: Theme.bg
        Loader {
            id: boardLoader
            objectName: "matchBoardLoader"
            anchors.fill: parent
            active: root.boardOpen
            sourceComponent: BoardEditor {
                blockMode: true
                showBack: true
                squad: root.convoked
                contextTitle: "Match contre " + opponent.text
                onCloseRequested: root.closeBoard()
                onDeleteRequested: removeBoardConfirm.confirm("Retirer le schéma ?",
                    "Le schéma tactique de ce match sera supprimé. Le match et ses convocations ne sont pas touchés.",
                    "Retirer", true)
            }
            onLoaded: item.open(root.openBoardId)
        }
    }

    Toast { id: toast }
}
