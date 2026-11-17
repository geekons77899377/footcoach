import QtQuick
import "BoardAnimation.js" as Anim

// Lecture animée d'un schéma (décision du 16/11/2026) : lecture / pause en boucle, mouvement précédent / suivant,
// vitesse (lent, normal, rapide), partage en animation GIF. Pilote view.anim et view.animTime du BoardView.
Rectangle {
    id: player
    objectName: "boardPlayer"

    required property var view                 // BoardView
    required property var board                // BoardModel
    property real u: 1
    property bool vertical: false              // barre verticale (paysage)
    property real speed: 1                     // 0.5, 1 ou 2
    property bool playing: false
    property real stopAt: -1                   // pas à pas : instant où s'arrêter (s), -1 : boucle
    property bool recording: false             // création du GIF en cours
    readonly property var tl: view.anim
    readonly property int stepCount: tl ? tl.steps.length : 0
    readonly property int currentStep: view.animState ? Math.max(view.animState.step >= 0 ? view.animState.step + 1
                                                                                    : view.animState.done, 0) : 0
    signal closeRequested()
    signal shareRequested()

    color: Theme.bgDeep

    // Construit la chronologie depuis le schéma affiché et revient au début.
    function start() {
        var items = [];
        for (var i = 0; i < board.items.count; ++i) {
            var it = board.items.get(i);
            items.push({ kind: it.kind, nx: it.nx, ny: it.ny });
        }
        board.clearSelection();
        view.anim = Anim.build(items, board.arrows, board.pitch);
        view.animTime = 0;
        stopAt = -1;
        playing = stepCount > 0;
    }
    function stop() {
        playing = false;
        view.anim = null;
        view.animTime = 0;
    }
    function togglePlay() {
        if (!tl)
            return;
        if (playing) {
            playing = false;
            return;
        }
        stopAt = -1;
        if (view.animTime >= tl.total - 0.01)
            view.animTime = 0;
        playing = true;
    }
    // Joue le mouvement suivant puis s'arrête.
    function next() {
        if (!tl)
            return;
        var k = Anim.nextStepFrom(tl, view.animTime);
        if (k >= stepCount) {
            view.animTime = 0;
            k = 0;
        }
        stopAt = Anim.stepEnd(tl, k);
        playing = true;
    }
    // Revient au début du mouvement en cours (ou du précédent), à l'arrêt.
    function previous() {
        if (!tl)
            return;
        playing = false;
        view.animTime = Anim.previousStart(tl, view.animTime);
    }
    // Avance le temps de dt secondes réelles (appelé par la minuterie ; public pour les tests).
    function advance(dt) {
        if (!tl || !playing)
            return;
        var t = view.animTime + dt * speed;
        if (stopAt >= 0 && t >= stopAt) {
            view.animTime = stopAt;
            stopAt = -1;
            playing = false;
            return;
        }
        view.animTime = t >= tl.total ? 0 : t;               // boucle
    }

    Timer {
        id: ticker
        interval: 33
        repeat: true
        running: player.playing && !player.recording && player.visible
        property double last: 0
        onRunningChanged: last = Date.now()
        onTriggered: {
            var now = Date.now();
            player.advance(Math.min(0.1, (now - last) / 1000));
            last = now;
        }
    }

    Rectangle {
        width: player.vertical ? 1 : parent.width
        height: player.vertical ? parent.height : 1
        x: player.vertical ? parent.width - 1 : 0
        color: Theme.border
    }

    Grid {
        anchors.centerIn: parent
        columns: player.vertical ? 1 : 7            // 7 commandes : une ligne en portrait, une colonne en paysage
        spacing: Math.round(6 * player.u)
        horizontalItemAlignment: Grid.AlignHCenter
        verticalItemAlignment: Grid.AlignVCenter

        IconButton {
            objectName: "playerClose"
            iconName: "close"
            accessibleName: "Fermer l'animation"
            onClicked: player.closeRequested()
        }
        IconButton {
            objectName: "playerPrevious"
            iconName: "skip"
            rotation: 180
            accessibleName: "Mouvement précédent"
            enabled: player.stepCount > 0
            onClicked: player.previous()
        }
        Rectangle {
            objectName: "playerPlay"
            width: Math.round(52 * player.u)
            height: width
            radius: width / 2
            color: Theme.accent
            Accessible.role: Accessible.Button
            Accessible.name: player.playing ? "Pause" : "Lecture"
            Icon {
                anchors.centerIn: parent
                name: player.playing ? "pause" : "play"
                color: "#ffffff"
                size: Math.round(26 * player.u)
            }
            MouseArea {
                anchors.fill: parent
                onClicked: player.togglePlay()
            }
        }
        IconButton {
            objectName: "playerNext"
            iconName: "skip"
            accessibleName: "Mouvement suivant"
            enabled: player.stepCount > 0
            onClicked: player.next()
        }
        Text {
            objectName: "playerStep"
            text: player.currentStep + " / " + player.stepCount
            font.pixelSize: Math.round(14 * player.u)
            font.weight: Font.DemiBold
            color: Theme.textMuted
        }
        Rectangle {
            objectName: "playerSpeed"
            width: speedText.implicitWidth + Math.round(20 * player.u)
            height: Math.round(36 * player.u)
            radius: height / 2
            color: Theme.surface
            border.width: 1
            border.color: Theme.border
            Accessible.role: Accessible.Button
            Accessible.name: "Vitesse " + speedText.text
            Text {
                id: speedText
                anchors.centerIn: parent
                text: player.speed === 0.5 ? "Lent" : player.speed === 2 ? "Rapide" : "Normal"
                font.pixelSize: Math.round(13 * player.u)
                font.weight: Font.DemiBold
                color: Theme.text
            }
            MouseArea {
                anchors.fill: parent
                onClicked: player.speed = player.speed === 1 ? 2 : player.speed === 2 ? 0.5 : 1
            }
        }
        IconButton {
            objectName: "playerShare"
            iconName: "share"
            accessibleName: "Partager l'animation"
            enabled: player.stepCount > 0 && !player.recording
            onClicked: player.shareRequested()
        }
    }
}
