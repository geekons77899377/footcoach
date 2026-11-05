import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil
import "Status.js" as Status

// Plan d'une séance : exercices dans l'ordre, avec horaires calculés, durées, notes et matériel à prévoir.
// Chaque bloc peut avoir son propre terrain tactique : miniature dans la carte, éditeur (BoardEditor) ouvert par-dessus
// le plan, qui reste en place dessous (position de défilement conservée). Le terrain n'est créé qu'à la demande.
Item {
    id: root

    property var session: null                // une séance de backend.sessions()
    property string teamName: ""
    property bool showBack: true
    property var planData: ({ entries: [], totalMin: 0, sessionMin: 0, equipment: [] })
    readonly property int diff: planData.totalMin - planData.sessionMin
    readonly property bool cancelled: session !== null && session.status === "cancelled"

    readonly property bool landscape: width > height

    // Terrain d'un bloc ouvert dans l'éditeur.
    property bool boardOpen: false
    property int openBoardId: -1                // terrain ouvert
    property string openBoardTitle: ""          // « Atelier 2 · Rondo 4v2 »

    signal closeRequested()
    signal changed()                          // le plan a changé (la liste des séances se met à jour)

    function openSession(s, team) {
        if (!leave(false))                 // un terrain encore ouvert est enregistré avant de changer de séance
            return;
        session = s;
        teamName = team;
        infos.openSession(s);
        reload(false);
        scroll.contentY = 0;
    }

    // keepScroll : la liste est reconstruite, on remet le défilement où il était.
    function reload(keepScroll) {
        if (!session)
            return;
        var r = backend.plan(session.id);
        if (!r.ok) {
            toast.show(r.error);
            return;
        }
        var y = scroll.contentY;
        planData = r;
        if (keepScroll !== false)
            Qt.callLater(function () {
                scroll.contentY = Math.max(0, Math.min(y, scroll.contentHeight - scroll.height));
            });
    }

    // --- Terrain d'un bloc ------------------------------------------------------------------------

    // Rapport largeur/hauteur de la miniature : demi-terrain 68 × 52,5 m, terrain entier 105 × 68 m.
    function pitchRatio(json) {
        try {
            return JSON.parse(json).pitch === "half" ? 1.3 : 1.55;
        } catch (e) {
            return 1.55;
        }
    }

    // Ouvre le terrain du bloc ; le crée d'abord (terrain vide) s'il n'en a pas encore : seulement sur demande.
    function openBlockBoard(entry) {
        if (cancelled)
            return;
        var id = entry.boardId;
        if (!(id > 0)) {
            var r = backend.createBlockBoard(entry.itemId);
            if (!r.ok) {
                toast.show(r.error);
                return;
            }
            id = r.boardId;
        }
        openBoardId = id;
        openBoardTitle = "Atelier " + entry.position + " · " + entry.title;
        boardOpen = true;
    }

    // Retour au plan : enregistre, relit le plan (miniatures à jour) et garde la position de défilement.
    function closeBoard() {
        if (!boardOpen)
            return;
        guardLeave(function () {
            boardOpen = false;
            openBoardId = -1;
            reload();
            changed();
        });
    }

    // Exécute action (sortie du plan ou de la page) une fois le terrain ouvert enregistré ; si l'enregistrement échoue,
    // on reste sur le terrain (dialogue : réessayer ou quitter sans enregistrer).
    function guardLeave(action) {
        if (boardOpen && boardLoader.item)
            boardLoader.item.leaveGuarded(action);
        else
            action();
    }

    // Dialogue d'échec pour une sortie dont l'enregistrement vient d'échouer (voir BoardEditor.showSaveFailed()).
    function showSaveFailed(action) {
        if (boardOpen && boardLoader.item)
            boardLoader.item.showSaveFailed(action);
    }

    // Vrai si rien n'est en attente d'enregistrement (réessaie l'enregistrement le cas échéant).
    function flushBoard() {
        return !boardOpen || !boardLoader.item || boardLoader.item.flush();
    }

    // Quitte le terrain sans relire le plan. discard : abandonne l'enregistrement en attente (données remplacées).
    // Faux (on reste sur le terrain) si l'enregistrement en attente échoue : les appelants passent d'abord par guardLeave().
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

    // Bouton retour du téléphone : quitte le plein écran du terrain, puis le terrain. Vrai si le geste a servi.
    function goBack() {
        if (!boardOpen)
            return false;
        if (boardLoader.item && boardLoader.item.fullscreen)
            boardLoader.item.fullscreen = false;
        else
            closeBoard();
        return true;
    }

    function act(r) {
        if (r.ok) {
            reload();
            changed();
        } else {
            toast.show(r.error);
        }
    }

    function summary() {
        if (planData.entries.length === 0)
            return "Aucun exercice";
        var t = planData.totalMin + " min planifiées sur " + planData.sessionMin + " min";
        if (diff > 0)
            return t + " : dépasse de " + diff + " min";
        if (diff < 0)
            return t + " : il reste " + (-diff) + " min";
        return t + " : la séance est complète";
    }

    PageHeader {
        id: header
        width: parent.width
        title: "Plan de séance"
        subtitle: root.session ? DateUtil.longLabel(root.session.date) + " · " + root.session.time
                                 + (root.teamName !== "" ? " · " + root.teamName : "") : ""
        leftInset: root.showBack ? 40 : 0
        IconButton {
            visible: !root.cancelled
            iconName: "plus"
            iconColor: Theme.textOnAccent
            fillColor: Theme.accent
            diameter: 48
            glow: true
            accessibleName: "Ajouter un exercice"
            onClicked: picker.pick()
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

    ExercisePicker {
        id: picker
        onPicked: (e) => root.act(backend.addToPlan(root.session.id, e.id, 0))
    }
    PromptDialog {
        id: removeBoardConfirm
        objectName: "removeBoardConfirm"
        onAccepted: {
            var id = root.openBoardId;
            if (!(id > 0) || !backend.deleteBoard(id)) {
                // Suppression en échec : le terrain reste ouvert, enregistrable, et l'erreur est affichée.
                toast.show("Impossible de retirer le terrain : " + backend.lastError);
                return;
            }
            if (boardLoader.item)
                boardLoader.item.discard();           // plus d'enregistrement vers un terrain supprimé
            root.boardOpen = false;
            root.openBoardId = -1;
            root.reload();
            root.changed();
        }
    }
    PlanItemDialog {
        id: itemDialog
        onAccepted: (id, minutes, notes) => root.act(backend.updatePlanItem(id, minutes, notes))
    }

    Flickable {
        id: scroll
        objectName: "planScroll"
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        contentWidth: width
        contentHeight: body.height + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        Column {
            id: body
            x: Theme.gutter
            y: 12
            width: Math.min(scroll.width - 2 * Theme.gutter, 640)
            spacing: 12

            // Informations de la séance (date, heure, durée, lieu, thème, statut, notes) : modifiables ici, au-dessus du
            // plan, pour tout voir et tout changer sur le même écran.
            Card {
                objectName: "sessionInfosCard"
                width: parent.width
                SessionFields {
                    id: infos
                    width: parent.width
                    spacing: 14
                    onSaved: (day) => {
                        // La séance affichée suit les valeurs enregistrées (en-tête, durée prévue, annulation).
                        root.session = Object.assign({}, root.session, infos.snapshot());
                        root.reload();
                        root.changed();
                        toast.show("Séance enregistrée");
                    }
                }
            }

            // Durée planifiée par rapport à la séance.
            Card {
                width: parent.width
                Column {
                    width: parent.width
                    spacing: 8
                    Text {
                        width: parent.width
                        text: root.session && root.session.theme !== "" ? root.session.theme : "Séance"
                        elide: Text.ElideRight
                        font.pixelSize: 17
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Rectangle {
                        width: parent.width
                        height: 8
                        radius: 4
                        color: Theme.bg
                        Rectangle {
                            width: root.planData.sessionMin > 0
                                   ? parent.width * Math.min(1, root.planData.totalMin / root.planData.sessionMin) : 0
                            height: parent.height
                            radius: 4
                            color: root.diff > 0 ? Theme.warn : Theme.accent
                        }
                    }
                    Text {
                        width: parent.width
                        text: root.summary()
                        wrapMode: Text.Wrap
                        font.pixelSize: 13
                        font.weight: root.diff > 0 ? Font.DemiBold : Font.Normal
                        color: root.diff > 0 ? Theme.warn : Theme.textMuted
                    }
                    Text {
                        visible: root.cancelled
                        width: parent.width
                        text: "Séance annulée : le plan n'est plus modifiable."
                        font.pixelSize: 13
                        color: Theme.danger
                    }
                }
            }

            // Exercices du plan.
            Repeater {
                model: root.planData.entries
                delegate: Card {
                    id: card
                    required property var modelData
                    required property int index
                    width: body.width
                    padding: 14

                    Column {
                        width: parent.width
                        spacing: 10
                        Row {
                            width: parent.width
                            spacing: 12
                            Rectangle {
                                id: num
                                width: 36
                                height: 36
                                radius: 18
                                color: Theme.accentSoft
                                border.width: 1
                                border.color: Status.categoryColor(card.modelData.category)
                                Text {
                                    anchors.centerIn: parent
                                    text: card.modelData.position
                                    font.pixelSize: 16
                                    font.weight: Font.ExtraBold
                                    color: Theme.accent
                                }
                            }
                            Column {
                                width: parent.width - num.width - 12
                                spacing: 3
                                Text {
                                    width: parent.width
                                    text: card.modelData.title
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 17
                                    font.weight: Font.Bold
                                    color: Theme.text
                                }
                                Text {
                                    width: parent.width
                                    wrapMode: Text.Wrap
                                    font.pixelSize: 13
                                    color: Theme.textMuted
                                    text: (card.modelData.start !== ""
                                           ? card.modelData.start + " – " + card.modelData.end + " · " : "")
                                          + card.modelData.durationMin + " min · " + card.modelData.categoryLabel
                                          + (card.modelData.archived ? " · archivé" : "")
                                }
                            }
                        }
                        Text {
                            visible: card.modelData.objective !== ""
                            width: parent.width
                            text: card.modelData.objective
                            wrapMode: Text.Wrap
                            font.pixelSize: 14
                            color: Theme.text
                        }
                        Text {
                            visible: card.modelData.notes !== ""
                            width: parent.width
                            text: card.modelData.notes
                            wrapMode: Text.Wrap
                            font.pixelSize: 14
                            font.italic: true
                            color: Theme.textMuted
                        }
                        // Terrain tactique du bloc : miniature, état vide ou terrain illisible.
                        Column {
                            id: boardZone
                            objectName: "blockBoard_" + card.modelData.itemId
                            readonly property bool has: card.modelData.boardId > 0
                            readonly property bool readable: card.modelData.boardReadable === true
                            readonly property real thumbWidth: Math.min(width, root.height < 500 ? 280 : 380)
                            width: parent.width
                            spacing: 8

                            BoardThumb {
                                id: thumb
                                objectName: "blockThumb_" + card.modelData.itemId
                                visible: boardZone.has && boardZone.readable
                                width: boardZone.thumbWidth
                                height: Math.round(width / root.pitchRatio(json))
                                json: card.modelData.boardJson || ""
                                MouseArea {
                                    anchors.fill: parent
                                    enabled: !root.cancelled
                                    onClicked: root.openBlockBoard(card.modelData)
                                }
                            }
                            Rectangle {
                                objectName: "blockBoardEmpty_" + card.modelData.itemId
                                visible: !boardZone.has || !boardZone.readable
                                width: boardZone.thumbWidth
                                height: 84
                                radius: 12
                                color: Theme.bg
                                border.width: 1
                                border.color: boardZone.has ? Theme.warn : Theme.border
                                Column {
                                    anchors.centerIn: parent
                                    width: parent.width - 24
                                    spacing: 6
                                    Icon {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        name: "board"
                                        size: 26
                                        color: boardZone.has ? Theme.warn : Theme.textDim
                                    }
                                    Text {
                                        width: parent.width
                                        horizontalAlignment: Text.AlignHCenter
                                        wrapMode: Text.Wrap
                                        font.pixelSize: 13
                                        color: boardZone.has ? Theme.warn : Theme.textMuted
                                        text: boardZone.has ? "Contenu du terrain illisible"
                                              : root.cancelled ? "Pas de terrain pour cet atelier"
                                              : "Pas encore de terrain pour cet atelier"
                                    }
                                }
                            }
                            AppButton {
                                objectName: "openBoard_" + card.modelData.itemId
                                visible: !root.cancelled
                                width: boardZone.thumbWidth
                                kind: "outline"
                                iconName: boardZone.has ? "edit" : "plus"
                                text: boardZone.has ? "Ouvrir le terrain" : "Ajouter un terrain"
                                Accessible.name: (boardZone.has ? "Ouvrir le terrain de " : "Ajouter un terrain à ") + card.modelData.title
                                onClicked: root.openBlockBoard(card.modelData)
                            }
                        }
                        Item {
                            visible: !root.cancelled
                            width: parent.width
                            height: actionRow.height
                        Row {
                            id: actionRow
                            anchors.right: parent.right
                            spacing: 6
                            IconButton {
                                iconName: "up"
                                enabled: card.index > 0
                                accessibleName: "Monter"
                                fillColor: Theme.bg
                                onClicked: root.act(backend.movePlanItem(card.modelData.itemId, -1))
                            }
                            IconButton {
                                iconName: "down"
                                enabled: card.index < root.planData.entries.length - 1
                                accessibleName: "Descendre"
                                fillColor: Theme.bg
                                onClicked: root.act(backend.movePlanItem(card.modelData.itemId, 1))
                            }
                            IconButton {
                                iconName: "edit"
                                accessibleName: "Modifier la durée et les notes"
                                fillColor: Theme.bg
                                onClicked: itemDialog.openFor(card.modelData)
                            }
                            IconButton {
                                iconName: "trash"
                                iconColor: Theme.danger
                                accessibleName: "Retirer du plan"
                                fillColor: Theme.bg
                                onClicked: root.act(backend.removePlanItem(card.modelData.itemId))
                            }
                        }
                        }
                    }
                }
            }

            AppButton {
                visible: !root.cancelled
                width: parent.width
                kind: root.planData.entries.length === 0 ? "primary" : "outline"
                iconName: "plus"
                text: "Ajouter un exercice"
                onClicked: picker.pick()
            }

            // Matériel à prévoir.
            Card {
                visible: root.planData.equipment.length > 0
                width: parent.width
                Column {
                    width: parent.width
                    spacing: 6
                    Text {
                        text: "MATÉRIEL À PRÉVOIR"
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        font.letterSpacing: 1.2
                        color: Theme.textMuted
                    }
                    Text {
                        width: parent.width
                        text: root.planData.equipment.join(", ")
                        wrapMode: Text.Wrap
                        font.pixelSize: 15
                        color: Theme.text
                    }
                }
            }
        }
    }

    // Éditeur du terrain d'un bloc, par-dessus le plan (qui reste chargé dessous : défilement conservé).
    Rectangle {
        id: boardLayer
        objectName: "boardLayer"
        anchors.fill: parent
        visible: root.boardOpen
        z: 10
        color: Theme.bg
        Loader {
            id: boardLoader
            objectName: "boardLoader"
            anchors.fill: parent
            active: root.boardOpen
            sourceComponent: BoardEditor {
                blockMode: true
                showBack: true
                contextTitle: root.openBoardTitle
                onCloseRequested: root.closeBoard()
                onDeleteRequested: removeBoardConfirm.confirm("Retirer le terrain ?",
                    "Le terrain de « " + root.openBoardTitle + " » sera supprimé. L'exercice et son schéma de référence "
                    + "ne sont pas touchés.", "Retirer", true)
            }
            onLoaded: item.open(root.openBoardId)
        }
    }

    Toast { id: toast }
}
