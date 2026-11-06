import QtQuick
import QtQuick.Controls
import QtCore

// Éditeur d'un schéma tactique : en-tête, terrain, barre d'outils (en bas en portrait, à gauche en paysage)
// et barre d'actions sur la sélection. Enregistrement automatique après chaque modification.
Item {
    id: editor
    objectName: "editor"

    property int boardId: -1
    property string title: ""
    property bool editable: true
    property string loadError: ""
    property string tool: "select"
    property bool showBack: true
    // Terrain propre à un bloc de séance (ouvert depuis le plan) : pas de renommage ni de duplication (le titre est
    // celui de l'atelier, une copie ferait un schéma libre) ; « supprimer » devient « retirer du bloc ».
    property bool blockMode: false
    // Terrain d'un match : null = joueurs libres (ronds numérotés) ; sinon liste des convoqués [{id, name, role}] (titulaires
    // d'abord) : « Joueur » propose cette liste et la formation se remplit avec les titulaires.
    property var squad: null
    readonly property bool squadMode: squad !== null
    property string contextTitle: ""                 // titre affiché en en-tête à la place de celui du schéma
    // Plein écran du terrain : l'en-tête et la barre d'outils se masquent ; les outils restent accessibles par un bouton.
    property bool fullscreen: false
    readonly property bool landscape: width > height
    readonly property bool shortLandscape: landscape && height < 500       // téléphone couché : outils plus serrés
    // Toutes les tailles suivent la plus petite dimension de l'écran (1 ≈ téléphone de 400 dp de large).
    readonly property real u: Math.max(0.8, Math.min(1.3, Math.min(width, height) / 400))
    readonly property int toolSize: Math.round((shortLandscape ? 52 : 58) * u)    // côté d'un outil
    readonly property int stripSize: toolSize + Math.round(16 * u)               // épaisseur de la barre d'outils
    readonly property int stripUsed: fullscreen ? 0 : stripSize                  // place prise par la barre d'outils
    readonly property int zoomButton: Math.round(40 * u)

    signal closeRequested()
    signal changed()                      // le contenu ou le titre a changé : rafraîchir la liste
    signal duplicateRequested()
    signal deleteRequested()

    readonly property var tools: [
        { id: "select", text: "Sélection", icon: "t-select" },
        { id: "multi", text: "Multi", icon: "t-multi" },
        { id: "player", text: "Joueur", icon: "t-player" },
        { id: "opponent", text: "Adversaire", icon: "t-opponent" },
        { id: "ball", text: "Ballon", icon: "t-ball" },
        { id: "cone", text: "Plot", icon: "t-cone" },
        { id: "goal", text: "But", icon: "t-goal" },
        { id: "text", text: "Note", icon: "t-text" },
        { id: "run", text: "Course", icon: "t-run" },
        { id: "pass", text: "Passe", icon: "t-pass" },
        { id: "cross", text: "Centre", icon: "t-cross" },
        { id: "dribble", text: "Conduite", icon: "t-dribble" }
    ]

    // --- Chargement et enregistrement ---------------------------------------------------------

    // Des modifications ne sont pas encore enregistrées (en attente du minuteur, ou enregistrement en échec). Tant
    // qu'elles le sont, flush() réessaie et les sorties passent par leaveGuarded().
    property bool dirty: false
    // Sortie en attente de l'issue du dialogue d'échec (voir leaveGuarded()).
    property var exitAction: null

    BoardModel {
        id: board
        objectName: "board"
        onEdited: {
            editor.dirty = true;
            saveTimer.restart();
        }
    }

    Timer {
        id: saveTimer
        interval: 500
        onTriggered: editor.save()
    }

    // Charge un schéma. Faux (sans rien charger) si le schéma ouvert a des modifications qui n'ont pas pu être
    // enregistrées : les appelants passent d'abord par leaveGuarded().
    function open(id) {
        if (!flush())
            return false;
        boardId = id;
        tool = "select";
        fullscreen = false;
        var b = backend.board(id);
        title = b.title || "";
        editable = b.readable === true;
        loadError = b.error || "";
        board.load(editable ? b.json : "");
        dirty = false;
        view.fit();
        return true;
    }

    // Vrai si le contenu est enregistré (ou s'il n'y a rien à enregistrer). Faux : l'enregistrement a échoué, les
    // modifications restent en attente (dirty) et l'erreur est affichée.
    function save() {
        saveTimer.stop();
        if (!editable || boardId < 0) {
            dirty = false;
            return true;
        }
        if (backend.saveBoard(boardId, board.toJson())) {
            dirty = false;
            changed();
            return true;
        }
        toast.show("Enregistrement impossible : " + backend.lastError);
        return false;
    }

    // Les données de l'appareil ont été remplacées : abandonne l'enregistrement en attente du schéma ouvert.
    function discard() {
        saveTimer.stop();
        dirty = false;
        boardId = -1;
    }

    // Enregistre ce qui est en attente. Faux si l'enregistrement a échoué : l'appelant ne doit pas quitter l'écran.
    function flush() {
        if (!dirty && !saveTimer.running)
            return true;
        return save();
    }

    // Quitte le terrain en exécutant action, sauf si l'enregistrement échoue : on reste alors sur l'écran, l'erreur
    // est affichée et l'utilisateur choisit de réessayer ou de quitter sans enregistrer (après confirmation).
    function leaveGuarded(action) {
        if (flush()) {
            exitAction = null;
            action();
            return;
        }
        showSaveFailed(action);
    }

    // Ouvre le dialogue d'échec pour une sortie dont l'enregistrement vient d'échouer (sans le retenter : l'appelant
    // l'a déjà fait, par exemple flush() à la fermeture de la fenêtre). action ne s'exécute que si l'utilisateur
    // réessaie avec succès ou confirme de quitter sans enregistrer ; « Rester » et Échap l'abandonnent.
    function showSaveFailed(action) {
        exitAction = action;
        saveFailed.message = backend.lastError;
        saveFailed.open();
    }

    SaveFailedDialog {
        id: saveFailed
        onRetry: {
            var a = editor.exitAction;
            editor.exitAction = null;
            editor.leaveGuarded(a);
        }
        onLeaveWithoutSaving: {
            discardConfirm.pendingAction = editor.exitAction;   // propre à cette confirmation : Annuler = rester
            editor.exitAction = null;
            discardConfirm.confirm("Quitter sans enregistrer ?",
                "Les dernières modifications de ce terrain seront perdues.", "Quitter sans enregistrer", true);
        }
        onStay: editor.exitAction = null
    }

    // Seconde confirmation : perdre des modifications n'est jamais le résultat d'un seul toucher.
    PromptDialog {
        id: discardConfirm
        objectName: "discardConfirm"
        property var pendingAction: null
        onAccepted: {
            var a = pendingAction;
            pendingAction = null;
            saveTimer.stop();
            editor.dirty = false;                  // choix explicite de l'utilisateur : rien n'est perdu en silence
            if (a)
                a();
        }
    }

    // --- Dialogues ------------------------------------------------------------------------------

    property string purpose: ""
    property var pending: null

    PromptDialog {
        id: prompt
        onAccepted: (text) => {
            if (editor.purpose === "rename") {
                if (text.trim() !== "" && backend.renameBoard(editor.boardId, text)) {
                    editor.title = text.trim();
                    editor.changed();
                }
            } else if (editor.purpose === "note") {
                if (text.trim() !== "")
                    board.addItem("text", editor.pending.x, editor.pending.y, text.trim());
            } else if (editor.purpose === "label") {
                board.setLabel(editor.pending, text);
            } else if (editor.purpose === "clear") {
                board.clearAll();
            }
        }
    }

    function editLabel(index) {
        var it = board.items.get(index);
        purpose = "label";
        pending = index;
        if (it.kind === "text")
            prompt.ask("Note", it.label, true, 300);
        else
            prompt.ask("Texte sous le joueur (" + board.maxPlayerLabel + " caractères maximum)", it.label, false, board.maxPlayerLabel);
    }

    function currentToolIcon() {
        for (var i = 0; i < tools.length; ++i)
            if (tools[i].id === tool)
                return tools[i].icon;
        return "t-select";
    }
    function currentToolText() {
        for (var i = 0; i < tools.length; ++i)
            if (tools[i].id === tool)
                return tools[i].text;
        return "";
    }

    // Choix de l'outil quand la barre d'outils est masquée (plein écran).
    ActionSheet {
        id: toolSheet
        title: "Outil"
        model: editor.tools.map(function (t) { return { id: t.id, text: t.text, icon: t.icon, checked: editor.tool === t.id }; })
        onChosen: (id) => editor.tool = id
    }

    // Rotation du groupe autour de son centre.
    ActionSheet {
        id: groupRotateSheet
        title: "Faire tourner le groupe"
        model: [
            { id: "90", text: "Quart de tour à droite (90°)", icon: "rotatecw" },
            { id: "-90", text: "Quart de tour à gauche (90°)", icon: "rotateccw" },
            { id: "45", text: "45° à droite", icon: "rotatecw" },
            { id: "-45", text: "45° à gauche", icon: "rotateccw" },
            { id: "180", text: "Demi-tour (180°)", icon: "rotatecw" }
        ]
        onChosen: (id) => {
            if (!board.rotateGroup(parseInt(id, 10), view.vertical))
                toast.show("Rotation impossible : des éléments sortiraient du terrain.");
        }
    }

    ActionSheet {
        id: moreSheet
        objectName: "moreSheet"
        title: "Schéma"
        model: [
            { id: "full", text: "Terrain entier", icon: "pitch", checked: board.pitch === "full" },
            { id: "half", text: "Demi-terrain", icon: "pitch", checked: board.pitch === "half" },
            { id: "formation", text: "Formation…", icon: "users" },
            { id: "rename", text: "Renommer", icon: "edit" },
            { id: "duplicate", text: "Dupliquer", icon: "copy" },
            { id: "clear", text: "Tout effacer", icon: "close", danger: true },
            { id: "delete", text: editor.blockMode ? "Retirer le terrain de ce bloc" : "Supprimer le schéma",
              icon: "trash", danger: true }
        ].filter(function (m) { return !editor.blockMode || (m.id !== "rename" && m.id !== "duplicate"); })
        onChosen: (id) => {
            if (id === "full" || id === "half") {
                board.setPitch(id);
            } else if (id === "formation") {
                if (board.pitch !== "full")
                    toast.show("Les formations se placent sur le terrain entier.");
                else
                    formationSheet.open();
            } else if (id === "rename") {
                editor.purpose = "rename";
                prompt.ask("Titre du schéma", editor.title, false, 80);
            } else if (id === "duplicate") {
                editor.leaveGuarded(function () { editor.duplicateRequested(); });
            } else if (id === "clear") {
                editor.purpose = "clear";
                prompt.confirm("Tout effacer ?", "Tous les éléments de ce schéma seront retirés (vous pourrez annuler).",
                               "Effacer", true);
            } else if (id === "delete") {
                editor.deleteRequested();
            }
        }
    }

    // Joueurs du match : nom affiché sous le rond (24 caractères au plus : prénom + initiale du nom si trop long).
    function squadLabel(p) {
        var n = p.name;
        if (n.length > board.maxPlayerLabel && p.firstName)
            n = p.firstName + (p.lastName ? " " + p.lastName.charAt(0) + "." : "");
        return n.substring(0, board.maxPlayerLabel).trim();
    }

    function askSquadPlayer(nx, ny) {
        if (squad.length === 0) {
            toast.show("Aucun joueur convoqué : choisissez-les dans la fiche du match.");
            return;
        }
        pending = Qt.point(nx, ny);
        var m = [];
        for (var i = 0; i < squad.length; ++i)
            m.push({ id: String(i), text: squad[i].name + (squad[i].role === "starter" ? " · Titulaire" : " · Remplaçant") });
        squadSheet.model = m;
        squadSheet.open();
    }

    ActionSheet {
        id: squadSheet
        objectName: "squadSheet"
        title: "Joueur à placer"
        onChosen: (id) => board.addItem("player", editor.pending.x, editor.pending.y, editor.squadLabel(editor.squad[parseInt(id, 10)]))
    }

    FormationSheet {
        id: formationSheet
        names: backend.formationNames()
        onPicked: (name, withOpp) => {
            if (!editor.squadMode) {
                board.applyFormation(backend.formationJson(name, withOpp), withOpp);
                return;
            }
            var starters = [];
            for (var i = 0; i < editor.squad.length; ++i)
                if (editor.squad[i].role === "starter")
                    starters.push(editor.squadLabel(editor.squad[i]));
            if (starters.length === 0)
                toast.show("Aucun titulaire convoqué : choisissez-les dans la fiche du match.");
            else
                board.applyFormation(backend.formationJson(name, withOpp), withOpp, starters);
        }
    }

    ColorSheet {
        id: colorSheet
        palette: backend.palette()
        onPicked: (color) => board.colorSelected(color)
    }

    // --- En-tête --------------------------------------------------------------------------------

    Item {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        visible: !editor.fullscreen

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            IconButton {
                visible: editor.showBack
                iconName: "back"
                accessibleName: "Retour à la liste"
                // Les pages hôtes ferment le terrain par leaveGuarded() : l'écran n'est quitté que si tout est enregistré.
                onClicked: editor.closeRequested()
            }
        }

        Text {
            id: titleText
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: editor.showBack ? 58 : 20
            anchors.right: headerButtons.left
            anchors.rightMargin: 8
            text: editor.contextTitle !== "" ? editor.contextTitle : editor.title
            elide: Text.ElideRight
            font.pixelSize: 20
            font.weight: Font.Bold
            color: Theme.text
            MouseArea {
                anchors.fill: parent
                enabled: editor.editable && !editor.blockMode
                onClicked: {
                    editor.purpose = "rename";
                    prompt.ask("Titre du schéma", editor.title, false, 80);
                }
            }
        }

        Row {
            id: headerButtons
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0
            IconButton {
                iconName: "undo"
                accessibleName: "Annuler"
                enabled: editor.editable && board.canUndo
                onClicked: board.undo()
            }
            IconButton {
                iconName: "redo"
                accessibleName: "Rétablir"
                enabled: editor.editable && board.canRedo
                onClicked: board.redo()
            }
            IconButton {
                iconName: "more"
                accessibleName: "Plus d'actions"
                enabled: editor.editable
                onClicked: moreSheet.open()
            }
        }
    }

    // --- Corps : outils + terrain ----------------------------------------------------------------

    Item {
        id: body
        anchors.top: editor.fullscreen ? parent.top : header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        BoardView {
            id: view
            objectName: "boardView"
            x: editor.landscape ? editor.stripUsed : 0
            y: 0
            width: body.width - (editor.landscape ? editor.stripUsed : 0)
            height: body.height - (editor.landscape ? 0 : editor.stripUsed)
            board: board
            tool: editor.tool
            enabled: editor.editable

            onSelectToolRequested: editor.tool = "select"
            onLabelEditRequested: (index) => editor.editLabel(index)
            onPlaceRequested: (nx, ny) => {
                if (editor.tool === "player" && editor.squadMode) {
                    editor.askSquadPlayer(nx, ny);
                } else if (editor.tool === "player") {
                    var n = board.nextPlayerNumber();
                    board.addItem("player", nx, ny, n > 0 ? String(n) : "");
                } else if (editor.tool === "text") {
                    editor.purpose = "note";
                    editor.pending = Qt.point(nx, ny);
                    prompt.ask("Note", "", true, 300);
                } else {
                    board.addItem(editor.tool, nx, ny, "");
                }
            }
        }

        // Aide : affichée une seule fois (quelques secondes), tant que le schéma est vide ; disparaît dès qu'on
        // choisit un outil, qu'on la touche ou qu'on place quelque chose.
        Settings {
            id: prefs
            category: "board"
            property bool hintSeen: false
        }
        Timer {
            id: hintTimer
            interval: 8000
            running: hintBox.visible
            onTriggered: prefs.hintSeen = true
        }
        Connections {
            target: editor
            function onToolChanged() { if (editor.tool !== "select") prefs.hintSeen = true; }
        }
        Rectangle {
            id: hintBox
            visible: editor.editable && !prefs.hintSeen && board.items.count === 0 && board.arrows.length === 0
            MouseArea { anchors.fill: parent; onClicked: prefs.hintSeen = true }
            anchors.left: view.left
            anchors.leftMargin: editor.fullscreen ? editor.zoomButton + 24 : 12
            anchors.top: view.top
            anchors.topMargin: 12
            width: Math.min(view.width - editor.zoomButton - 36 - (editor.fullscreen ? editor.zoomButton + 12 : 0), 360)
            height: hint.implicitHeight + 24
            radius: 16
            color: "#d90f172a"
            border.width: 1
            border.color: Theme.border
            Text {
                id: hint
                anchors.centerIn: parent
                width: parent.width - 28
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                font.pixelSize: Math.round(13 * editor.u)
                color: Theme.text
                text: "Choisissez un outil puis touchez le terrain. Deux doigts pour zoomer et déplacer : "
                      + "zoomez avant de placer pour être précis."
            }
        }

        // Zoom.
        Column {
            anchors.right: view.right
            anchors.rightMargin: 12
            anchors.top: view.top
            anchors.topMargin: 12
            spacing: 8
            visible: editor.editable
            IconButton {
                diameter: editor.zoomButton
                iconName: "zoomin"
                accessibleName: "Zoom avant"
                fillColor: "#d90f172a"
                borderColor: Theme.border
                enabled: view.zoom < view.maxZoom - 0.01
                onClicked: view.zoomIn()
            }
            IconButton {
                diameter: editor.zoomButton
                iconName: "zoomout"
                accessibleName: "Zoom arrière"
                fillColor: "#d90f172a"
                borderColor: Theme.border
                enabled: view.zoom > 1.01
                onClicked: view.zoomOut()
            }
            IconButton {
                diameter: editor.zoomButton
                iconName: "fit"
                accessibleName: "Voir tout le terrain"
                fillColor: "#d90f172a"
                borderColor: Theme.border
                enabled: view.zoom > 1.01
                onClicked: view.fit()
            }
            IconButton {
                diameter: editor.zoomButton
                iconName: editor.fullscreen ? "collapse" : "expand"
                accessibleName: editor.fullscreen ? "Quitter le plein écran" : "Terrain en plein écran"
                fillColor: "#d90f172a"
                borderColor: Theme.border
                onClicked: editor.fullscreen = !editor.fullscreen
            }
        }

        // En plein écran : outils, annuler et rétablir restent à portée, en haut à gauche.
        Column {
            visible: editor.editable && editor.fullscreen
            anchors.left: view.left
            anchors.leftMargin: 12
            anchors.top: view.top
            anchors.topMargin: 12
            spacing: 8
            IconButton {
                diameter: editor.zoomButton
                iconName: editor.currentToolIcon()
                iconColor: Theme.accent
                accessibleName: "Outil : " + editor.currentToolText()
                fillColor: "#d90f172a"
                borderColor: Theme.accent
                onClicked: toolSheet.open()
            }
            IconButton {
                diameter: editor.zoomButton
                iconName: "undo"
                accessibleName: "Annuler"
                fillColor: "#d90f172a"
                borderColor: Theme.border
                enabled: board.canUndo
                onClicked: board.undo()
            }
            IconButton {
                diameter: editor.zoomButton
                iconName: "redo"
                accessibleName: "Rétablir"
                fillColor: "#d90f172a"
                borderColor: Theme.border
                enabled: board.canRedo
                onClicked: board.redo()
            }
        }

        // Aide de la sélection multiple.
        Rectangle {
            visible: editor.editable && editor.tool === "multi"
            anchors.left: view.left
            anchors.leftMargin: editor.fullscreen ? editor.zoomButton + 24 : 12
            anchors.top: view.top
            anchors.topMargin: 12
            width: Math.min(view.width - editor.zoomButton - 36 - (editor.fullscreen ? editor.zoomButton + 12 : 0), 360)
            height: multiHint.implicitHeight + 20
            radius: 16
            color: "#d90f172a"
            border.width: 1
            border.color: Theme.border
            Row {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                Text {
                    id: multiHint
                    width: parent.width - allButton.width - 10
                    anchors.verticalCenter: parent.verticalCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Math.round(13 * editor.u)
                    color: Theme.text
                    text: "Touchez pour ajouter ou retirer · glissez sur le terrain pour entourer"
                }
                AppButton {
                    id: allButton
                    anchors.verticalCenter: parent.verticalCenter
                    kind: "outline"
                    text: "Tout"
                    implicitHeight: 40
                    onClicked: board.selectAll()
                }
            }
        }

        // Actions sur la sélection.
        Rectangle {
            id: contextBar
            visible: editor.editable && board.selCount > 0
            anchors.horizontalCenter: view.horizontalCenter
            anchors.bottom: view.bottom
            anchors.bottomMargin: 12
            width: Math.min(ctxRow.implicitWidth + 20, view.width - 24)
            height: 60
            radius: 30
            color: "#f20f172a"
            border.width: 1
            border.color: Theme.border

            Flickable {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                contentWidth: ctxRow.implicitWidth
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                Row {
                    id: ctxRow
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    IconButton {
                        visible: board.selLabelIndex >= 0
                        iconName: "edit"
                        accessibleName: "Modifier le texte"
                        onClicked: editor.editLabel(board.selLabelIndex)
                    }
                    // Deux boutons, seulement quand plusieurs éléments sont sélectionnés (par rapport à l'écran).
                    IconButton {
                        objectName: "alignHorizontal"
                        visible: board.selItems > 1
                        iconName: "alignvcenter"
                        accessibleName: "Aligner horizontalement"
                        onClicked: board.alignSelected("vcenter", view.vertical)
                    }
                    IconButton {
                        objectName: "alignVertical"
                        visible: board.selItems > 1
                        iconName: "alignhcenter"
                        accessibleName: "Aligner verticalement"
                        onClicked: board.alignSelected("hcenter", view.vertical)
                    }
                    IconButton {
                        visible: board.selCount > 1
                        iconName: "grouprotate"
                        accessibleName: "Faire tourner le groupe"
                        onClicked: groupRotateSheet.open()
                    }
                    IconButton {
                        visible: board.selTransformable > 0
                        iconName: "rotateccw"
                        accessibleName: "Pivoter à gauche"
                        onClicked: board.rotateSelected(-45)
                    }
                    IconButton {
                        visible: board.selTransformable > 0
                        iconName: "rotatecw"
                        accessibleName: "Pivoter à droite"
                        onClicked: board.rotateSelected(45)
                    }
                    IconButton {
                        visible: board.selTransformable > 0
                        iconName: "minus"
                        accessibleName: "Réduire"
                        onClicked: board.scaleSelected(1 / 1.25)
                    }
                    IconButton {
                        visible: board.selTransformable > 0
                        iconName: "plus"
                        accessibleName: "Agrandir"
                        onClicked: board.scaleSelected(1.25)
                    }
                    IconButton {
                        visible: board.selColorable > 0
                        iconName: "palette"
                        accessibleName: "Couleur"
                        onClicked: colorSheet.open()
                    }
                    IconButton {
                        visible: board.selCrosses > 0
                        iconName: "flip"
                        accessibleName: "Inverser la courbure"
                        onClicked: board.flipSelectedCurves()
                    }
                    IconButton {
                        iconName: "trash"
                        iconColor: Theme.danger
                        accessibleName: "Supprimer la sélection"
                        onClicked: board.deleteSelected()
                    }
                }
            }
        }

        // Barre d'outils : horizontale en portrait (en bas), verticale en paysage (à gauche).
        Rectangle {
            id: strip
            x: 0
            y: editor.landscape ? 0 : body.height - editor.stripSize
            width: editor.landscape ? editor.stripSize : body.width
            height: editor.landscape ? body.height : editor.stripSize
            color: Theme.bgDeep
            visible: editor.editable && !editor.fullscreen

            Rectangle {
                width: editor.landscape ? 1 : parent.width
                height: editor.landscape ? parent.height : 1
                x: editor.landscape ? parent.width - 1 : 0
                color: Theme.border
            }

            ListView {
                id: toolList
                anchors.fill: parent
                anchors.margins: 8
                orientation: editor.landscape ? ListView.Vertical : ListView.Horizontal
                spacing: 8
                clip: true
                model: editor.tools
                boundsBehavior: Flickable.StopAtBounds
                delegate: Rectangle {
                    id: toolItem
                    required property var modelData
                    readonly property bool active: editor.tool === modelData.id
                    width: editor.landscape ? toolList.width : editor.toolSize + Math.round(8 * editor.u)
                    height: editor.landscape ? editor.toolSize : toolList.height
                    radius: Math.round(14 * editor.u)
                    color: active ? Theme.accentSoft : Theme.surface
                    border.width: active ? 2 : 1
                    border.color: active ? Theme.accent : Theme.border
                    Accessible.role: Accessible.Button
                    Accessible.name: modelData.text
                    Column {
                        anchors.centerIn: parent
                        spacing: 2
                        Icon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            name: toolItem.modelData.icon
                            color: toolItem.active ? Theme.accent : Theme.text
                            size: Math.round(24 * editor.u)
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: toolItem.modelData.text
                            font.pixelSize: Math.round(10 * editor.u)
                            font.weight: Font.DemiBold
                            color: toolItem.active ? Theme.accent : Theme.textMuted
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: editor.tool = toolItem.modelData.id
                    }
                }
            }
        }

        // Schéma illisible : jamais modifié.
        Rectangle {
            visible: !editor.editable
            anchors.fill: parent
            color: Theme.bg
            Card {
                anchors.centerIn: parent
                width: Math.min(parent.width - 32, 420)
                Column {
                    width: parent.width
                    spacing: 10
                    Text {
                        width: parent.width
                        text: "Schéma illisible"
                        font.pixelSize: 20
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 15
                        color: Theme.textMuted
                        text: "Le contenu de ce schéma ne peut pas être lu, il n'est donc pas modifiable (ce qu'il contient "
                              + "reste intact).\n" + editor.loadError
                    }
                    AppButton {
                        text: editor.blockMode ? "Retirer le terrain de ce bloc" : "Supprimer ce schéma"
                        kind: "danger"
                        onClicked: editor.deleteRequested()
                    }
                }
            }
        }
    }

    // Message bref en bas de l'écran.
    Rectangle {
        id: toast
        property string message: ""
        function show(text) {
            message = text;
            opacity = 1;
            toastTimer.restart();
        }
        z: 100
        opacity: 0
        visible: opacity > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: editor.landscape ? 20 : editor.stripUsed + 20
        width: Math.min(parent.width - 32, toastText.implicitWidth + 36)
        height: toastText.implicitHeight + 24
        radius: 16
        color: Theme.surfaceHi
        border.width: 1
        border.color: Theme.border
        Behavior on opacity { NumberAnimation { duration: 180 } }
        Text {
            id: toastText
            anchors.centerIn: parent
            width: parent.width - 28
            text: toast.message
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
            color: Theme.text
        }
        Timer {
            id: toastTimer
            interval: 3200
            onTriggered: toast.opacity = 0
        }
    }

    // Dernier recours seulement : une destruction ne peut pas être bloquée et l'erreur ne serait plus visible. Les
    // sorties (retour, autre terrain, navigation, fermeture de la fenêtre) passent par leaveGuarded().
    Component.onDestruction: flush()
}
