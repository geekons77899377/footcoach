import QtQuick
import QtQuick.Controls
import QtCore
import "LiveSession.js" as Live
import "DateUtil.js" as DateUtil

// « Séance du jour » de l'accueil : la séance choisie d'office selon l'heure du téléphone (ou à la main), son
// chronomètre par exercice et le déroulé. Le temps affiché se recalcule toujours à partir de l'horloge et de
// l'heure de départ de l'exercice : un écran verrouillé ou une application en arrière-plan ne le fausse pas.
Item {
    id: root

    property string teamName: ""
    property int teamId: AppState.teamId
    property var daySessions: []                 // séances d'hier, d'aujourd'hui et de demain
    property int manualId: -1                    // > 0 : séance choisie à la main
    property var entries: []                     // plan de la séance affichée (backend.plan)
    property int shownId: -1                     // séance dont le plan est chargé

    // État du chronomètre : idle | running | paused | finished.
    property string phase: "idle"
    property int exIndex: 0
    property real startedAt: 0                   // horloge au dernier départ / dernière reprise
    property real accMs: 0                       // temps déjà écoulé sur l'exercice avant la pause
    property var actuals: []                     // durées réelles des exercices terminés (ms)
    property real now: Date.now()
    property int lockedId: -1                    // séance verrouillée pendant qu'elle est en cours

    readonly property bool active: phase === "running" || phase === "paused"
    readonly property var auto: Live.pickAuto(daySessions, now)
    readonly property var session: {
        var id = lockedId > 0 ? lockedId : manualId;
        if (id > 0) {
            for (var i = 0; i < daySessions.length; ++i)
                if (daySessions[i].id === id)
                    return daySessions[i];
        }
        return auto.session;
    }
    readonly property bool isManual: lockedId <= 0 && manualId > 0 && session !== null && session.id === manualId
    readonly property var current: exIndex >= 0 && exIndex < entries.length ? entries[exIndex] : null
    readonly property real elapsedMs: Live.elapsed(accMs, startedAt, phase === "running", now)
    readonly property real leftMs: current ? Live.remaining(current.durationMin, elapsedMs) : 0
    readonly property bool overtime: current !== null && leftMs < 0

    signal openExercise(int exerciseId)
    signal planSession()

    implicitHeight: body.implicitHeight

    // --- Mémoire : reprise du chronomètre si l'application est relancée en cours de séance ------------------

    Settings {
        id: store
        category: "live"
        property int sessionId: -1
        property string day: ""
        property string phase: "idle"
        property int exIndex: 0
        property real startedAt: 0
        property real accMs: 0
        property string actuals: "[]"
    }

    function save() {
        store.sessionId = lockedId;
        store.day = DateUtil.todayIso();
        store.phase = phase;
        store.exIndex = exIndex;
        store.startedAt = startedAt;
        store.accMs = accMs;
        store.actuals = JSON.stringify(actuals);
    }

    function forget() {
        lockedId = -1;
        phase = "idle";
        exIndex = 0;
        accMs = 0;
        startedAt = 0;
        actuals = [];
        store.sessionId = -1;
        store.phase = "idle";
        keepAwake(false);
    }

    function keepAwake(on) {
        if (backend && backend.setKeepScreenOn)
            backend.setKeepScreenOn(on);
    }

    // --- Chargement --------------------------------------------------------------------------------------

    function reload() {
        now = Date.now();
        var today = DateUtil.todayIso();
        daySessions = teamId > 0 ? backend.sessions(teamId, DateUtil.addDays(today, -1), DateUtil.addDays(today, 1)) : [];
        // Reprise : même jour, séance toujours là, chronomètre commencé.
        if (!active && store.phase !== "idle" && store.day === today && store.sessionId > 0) {
            var found = false;
            for (var i = 0; i < daySessions.length; ++i)
                if (daySessions[i].id === store.sessionId)
                    found = true;
            if (found) {
                lockedId = store.sessionId;
                loadPlan(lockedId);
                phase = store.phase;
                exIndex = Math.min(store.exIndex, Math.max(0, entries.length - 1));
                startedAt = store.startedAt;
                accMs = store.accMs;
                try { actuals = JSON.parse(store.actuals); } catch (e) { actuals = []; }
                if (phase === "running")
                    keepAwake(true);
            } else {
                forget();
            }
        } else if (!active && store.phase !== "idle" && store.day !== today) {
            forget();
        }
        syncPlan();
    }

    function loadPlan(id) {
        shownId = id;
        var p = id > 0 ? backend.plan(id) : null;
        entries = p && p.ok ? p.entries : [];
    }

    function syncPlan() {
        var id = session ? session.id : -1;
        if (id !== shownId)
            loadPlan(id);
    }

    onSessionChanged: syncPlan()
    // Changement d'équipe : le chronomètre d'une autre équipe est abandonné. Le premier choix d'équipe au
    // lancement (de -1 à une équipe) ne doit pas effacer un chronomètre à reprendre.
    property int prevTeam: -1
    onTeamIdChanged: {
        if (prevTeam > 0 && prevTeam !== teamId)
            forget();
        prevTeam = teamId;
        manualId = -1;
        reload();
    }

    // --- Actions -----------------------------------------------------------------------------------------

    function start() {
        if (!session || entries.length === 0)
            return;
        lockedId = session.id;
        exIndex = 0;
        accMs = 0;
        actuals = [];
        now = Date.now();
        startedAt = now;
        phase = "running";
        keepAwake(true);
        save();
    }

    function pause() {
        if (phase !== "running")
            return;
        now = Date.now();
        accMs = elapsedMs;
        phase = "paused";
        save();
    }

    function resume() {
        if (phase !== "paused")
            return;
        now = Date.now();
        startedAt = now;
        phase = "running";
        save();
    }

    function toggle() {
        if (phase === "idle") start();
        else if (phase === "running") pause();
        else if (phase === "paused") resume();
    }

    function next() {
        if (!active)
            return;
        now = Date.now();
        var a = actuals.slice();
        a[exIndex] = elapsedMs;
        actuals = a;
        if (exIndex + 1 < entries.length) {
            exIndex = exIndex + 1;
            accMs = 0;
            startedAt = now;
        } else {
            phase = "finished";
            keepAwake(false);
        }
        save();
    }

    function chooseSession(id) {
        if (active)
            return;
        manualId = id;
        shownId = -1;
        syncPlan();
    }

    Component.onCompleted: reload()
    onVisibleChanged: if (visible) reload()
    Component.onDestruction: keepAwake(false)

    // Horloge : chaque quart de seconde en marche, chaque demi-minute sinon (pour le choix automatique).
    Timer {
        interval: root.phase === "running" ? 250 : 30000
        running: root.visible
        repeat: true
        onTriggered: root.now = Date.now()
    }

    // --- Affichage -------------------------------------------------------------------------------------------

    function stateOf(i) {
        if (phase === "finished" || (active && i < exIndex))
            return "done";
        if (active && i === exIndex)
            return "current";
        return "todo";
    }

    function dayLabel(s) {
        var today = DateUtil.todayIso();
        if (s.date === today) return "Aujourd'hui";
        if (s.date === DateUtil.addDays(today, -1)) return "Hier";
        if (s.date === DateUtil.addDays(today, 1)) return "Demain";
        return DateUtil.display(s.date);
    }

    function sessionTitle(s) {
        return s.theme && s.theme !== "" ? s.theme : "Séance d'entraînement";
    }

    function subtitleOf(s) {
        return (root.teamName !== "" ? root.teamName + " · " : "") + s.time + " → " + s.endTime;
    }

    Column {
        id: body
        width: parent.width
        spacing: 18

        Text {
            text: "SÉANCE DU JOUR"
            font.pixelSize: 12
            font.weight: Font.Bold
            font.letterSpacing: 1.4
            color: Theme.text
        }

        // Aucune séance à afficher.
        Card {
            visible: root.session === null
            width: parent.width
            padding: 18
            Column {
                width: parent.width
                spacing: 10
                Text {
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: root.teamId > 0 ? "Aucune séance aujourd'hui" : "Aucune équipe"
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    color: Theme.text
                }
                Text {
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: root.teamId > 0 ? "Planifiez une séance, ou choisissez-en une d'hier ou de demain."
                                          : "Créez une équipe pour préparer vos séances."
                    font.pixelSize: 14
                    color: Theme.textMuted
                }
                Row {
                    spacing: 10
                    AppButton {
                        visible: root.teamId > 0
                        kind: "primary"
                        text: "Planifier une séance"
                        iconName: "calendar"
                        onClicked: root.planSession()
                    }
                    AppButton {
                        visible: root.daySessions.length > 0
                        text: "Changer de séance"
                        onClicked: root.openPicker()
                    }
                }
            }
        }

        // Carte du chronomètre.
        Rectangle {
            id: timerCard
            visible: root.session !== null
            width: parent.width
            readonly property bool wide: width >= 560           // paysage / tablette : boutons à droite du chronomètre
            readonly property int sideW: wide ? 250 : 0
            height: wide ? Math.max(timerCol.implicitHeight, buttons.implicitHeight) + 36
                         : timerCol.implicitHeight + buttons.implicitHeight + 10 + 36
            radius: Theme.radius
            border.width: 1
            border.color: root.overtime ? Theme.warn : Theme.border
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0; color: "#143d2c" }
                GradientStop { position: 1; color: "#1a2433" }
            }

            Column {
                id: timerCol
                x: 18
                y: 18
                width: parent.width - 36 - (timerCard.wide ? timerCard.sideW + 18 : 0)
                spacing: 10

                Item {
                    width: parent.width
                    height: head.implicitHeight
                    Column {
                        id: head
                        anchors.left: parent.left
                        anchors.right: expandBtn.left
                        anchors.rightMargin: 8
                        spacing: 4
                        Text {
                            width: parent.width
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            text: root.session ? root.sessionTitle(root.session) : ""
                            font.pixelSize: 24
                            font.weight: Font.ExtraBold
                            color: Theme.text
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: root.session ? root.subtitleOf(root.session) : ""
                            font.pixelSize: 14
                            color: Theme.textMuted
                        }
                    }
                    IconButton {
                        id: expandBtn
                        anchors.right: parent.right
                        anchors.top: parent.top
                        visible: root.active
                        iconName: "expand"
                        accessibleName: "Chronomètre en plein écran"
                        fillColor: "#33000000"
                        onClicked: bigTimer.open()
                    }
                }

                // Choix de la séance : automatique par défaut, ou à la main.
                Item {
                    width: parent.width
                    height: Math.max(modeText.implicitHeight, 32)
                    Text {
                        id: modeText
                        anchors.left: parent.left
                        anchors.right: changeLink.left
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        elide: Text.ElideRight
                        text: root.isManual ? "CHOISIE À LA MAIN"
                              : root.lockedId > 0 ? "EN COURS"
                              : root.auto.mode === "now" ? "AUTOMATIQUE · SELON L'HEURE"
                              : "PROCHAINE SÉANCE · " + (root.session ? root.dayLabel(root.session).toUpperCase() : "")
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        font.letterSpacing: 1.2
                        color: Theme.accent
                    }
                    Text {
                        id: changeLink
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        visible: !root.active && root.daySessions.length > 0
                        text: "Changer de séance"
                        font.pixelSize: 13
                        font.underline: true
                        color: Theme.textMuted
                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -10
                            onClicked: root.openPicker()
                        }
                    }
                }

                Text {
                    visible: !root.active && root.auto.overlaps > 1 && !root.isManual
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: "Deux séances se chevauchent : vérifiez que c'est la bonne."
                    font.pixelSize: 13
                    color: Theme.warn
                }

                // Gros chronomètre.
                Text {
                    id: bigClock
                    width: parent.width
                    height: 108
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    fontSizeMode: Text.Fit
                    minimumPixelSize: 36
                    font.pixelSize: 96
                    font.weight: Font.Black
                    font.features: { "tnum": 1 }
                    text: root.phase === "finished" ? "Terminée"
                          : root.current ? Live.clock(root.active ? root.leftMs : root.current.durationMin * 60000)
                          : "--:--"
                    color: root.overtime ? Theme.warn : (root.phase === "paused" ? Theme.textMuted : Theme.text)
                    opacity: root.phase === "paused" ? (blink.on ? 1 : 0.45) : 1
                    Accessible.role: Accessible.StaticText
                    Accessible.name: "Chronomètre " + text
                }
                Timer {
                    id: blink
                    property bool on: true
                    interval: 600
                    running: root.phase === "paused" && root.visible
                    repeat: true
                    onTriggered: on = !on
                    onRunningChanged: if (!running) on = true
                }

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    text: root.phase === "finished" ? "Séance terminée · " + root.entries.length + " exercices"
                          : root.entries.length === 0 ? "Aucun exercice dans le plan de cette séance"
                          : root.active ? "Ex. " + (root.exIndex + 1) + " : " + (root.current ? root.current.title : "")
                                          + (root.phase === "paused" ? " · en pause" : "")
                          : "PROCHAIN : " + (root.current ? root.current.title.toUpperCase() : "")
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    font.letterSpacing: root.active ? 0 : 1
                    color: root.overtime ? Theme.warn : Theme.accent
                }
            }

            Column {
                id: buttons
                spacing: 10
                width: timerCard.wide ? timerCard.sideW : parent.width - 36
                x: timerCard.wide ? parent.width - width - 18 : 18
                y: timerCard.wide ? Math.max(18, (parent.height - implicitHeight) / 2)
                                  : 18 + timerCol.implicitHeight + 10
                AppButton {
                    objectName: "liveMain"
                    visible: root.phase !== "finished"
                    width: buttons.width
                    height: 60
                    kind: "primary"
                    enabled: root.entries.length > 0
                    text: root.phase === "idle" ? "COMMENCER" : root.phase === "running" ? "PAUSE" : "REPRENDRE"
                    iconName: root.phase === "running" ? "pause" : "play"
                    onClicked: root.toggle()
                }
                AppButton {
                    objectName: "liveNext"
                    visible: root.phase !== "finished"
                    width: buttons.width
                    height: 52
                    enabled: root.active
                    text: root.exIndex + 1 >= root.entries.length && root.active ? "Terminer la séance" : "Exercice suivant"
                    iconName: "skip"
                    onClicked: root.next()
                }
                AppButton {
                    visible: root.phase === "finished"
                    width: buttons.width
                    height: 52
                    text: "Fermer"
                    objectName: "liveClose"
                    onClicked: root.forget()
                }
            }
        }

        // Déroulé.
        Item {
            visible: root.session !== null && root.entries.length > 0
            width: parent.width
            height: 20
            Text {
                text: "PROGRAMME DE LA SÉANCE"
                font.pixelSize: 12
                font.weight: Font.Bold
                font.letterSpacing: 1.4
                color: Theme.text
            }
            Text {
                anchors.right: parent.right
                text: root.entries.length + (root.entries.length > 1 ? " EXERCICES" : " EXERCICE")
                font.pixelSize: 11
                font.weight: Font.Bold
                font.letterSpacing: 1
                color: Theme.accent
            }
        }

        Repeater {
            model: root.session !== null ? root.entries : []
            delegate: Rectangle {
                id: row
                required property var modelData
                required property int index
                readonly property string st: root.stateOf(index)
                width: body.width
                height: 84
                radius: Theme.radius
                color: st === "current" ? "#143d2c" : Theme.surface
                border.width: 1
                border.color: st === "current" ? Theme.accent : Theme.border
                opacity: st === "done" ? 0.7 : 1

                Rectangle {
                    id: badge
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    width: 50
                    height: 50
                    radius: Theme.radiusSm
                    color: row.st === "current" ? Theme.accent : Theme.bgDeep
                    border.width: 1
                    border.color: Theme.border
                    Icon {
                        anchors.centerIn: parent
                        visible: row.st !== "todo"
                        name: row.st === "done" ? "tick" : "stopwatch"
                        color: row.st === "done" ? Theme.accent : Theme.textOnAccent
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: row.st === "todo"
                        text: row.index + 1
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        color: Theme.textMuted
                    }
                }
                Column {
                    anchors.left: badge.right
                    anchors.leftMargin: 14
                    anchors.right: timeCol.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        text: row.modelData.title
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        color: row.st === "current" ? Theme.accent : Theme.text
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        text: row.st === "current" ? (root.phase === "paused" ? "EN PAUSE" : "EN COURS")
                              : (row.modelData.categoryLabel || "").toUpperCase()
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        font.letterSpacing: 1
                        color: row.st === "current" ? Theme.accent : Theme.textMuted
                    }
                }
                Column {
                    id: timeCol
                    anchors.right: chevron.left
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Text {
                        anchors.right: parent.right
                        text: row.st === "current" ? Live.clock(root.leftMs)
                              : row.st === "done" && root.actuals[row.index] !== undefined ? Live.actual(root.actuals[row.index])
                              : Live.minutes(row.modelData.durationMin)
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        font.features: { "tnum": 1 }
                        color: row.st === "current" ? (root.overtime ? Theme.warn : Theme.accent) : Theme.text
                    }
                    Text {
                        anchors.right: parent.right
                        visible: row.st === "done" && root.actuals[row.index] !== undefined
                        text: "prévu " + Live.minutes(row.modelData.durationMin)
                        font.pixelSize: 11
                        color: Theme.textMuted
                    }
                }
                Icon {
                    id: chevron
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    name: "chevron"
                    color: Theme.textMuted
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.openExercise(row.modelData.exerciseId)
                }
            }
        }
    }

    // --- Choix manuel de la séance -----------------------------------------------------------------------------

    function openPicker() {
        var m = [{ id: "auto", text: "Automatique · selon l'heure", icon: "clock", checked: manualId <= 0 }];
        for (var i = 0; i < daySessions.length; ++i) {
            var s = daySessions[i];
            if (s.status === "cancelled")
                continue;
            m.push({ id: "s" + s.id,
                     text: dayLabel(s) + " " + s.time + " · " + sessionTitle(s),
                     icon: "clipboard", checked: manualId === s.id });
        }
        picker.model = m;
        picker.open();
    }

    ActionSheet {
        id: picker
        title: "Séance à afficher"
        onChosen: (id) => root.chooseSession(id === "auto" ? -1 : parseInt(id.substring(1), 10))
    }

    // --- Chronomètre en plein écran (lisible de loin, au bord du terrain) ------------------------------------

    Popup {
        id: bigTimer
        parent: Overlay.overlay
        modal: true
        dim: false
        padding: 0
        x: 0
        y: 0
        width: parent ? parent.width : 400
        height: parent ? parent.height : 800
        closePolicy: Popup.CloseOnEscape
        background: Rectangle { color: "#0b1d16" }

        contentItem: Item {
            Column {
                anchors.centerIn: parent
                width: parent.width - 48
                spacing: 16
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: root.current ? "Ex. " + (root.exIndex + 1) + " : " + root.current.title : ""
                    font.pixelSize: 22
                    font.weight: Font.Bold
                    color: Theme.accent
                }
                Text {
                    width: parent.width
                    height: Math.max(120, bigTimer.height * 0.38)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    fontSizeMode: Text.Fit
                    minimumPixelSize: 40
                    font.pixelSize: 220
                    font.weight: Font.Black
                    font.features: { "tnum": 1 }
                    text: root.phase === "finished" ? "Terminée" : Live.clock(root.leftMs)
                    color: root.overtime ? Theme.warn : (root.phase === "paused" ? Theme.textMuted : Theme.text)
                }
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 12
                    AppButton {
                        width: Math.min(220, (bigTimer.width - 60) / 2)
                        height: 64
                        kind: "primary"
                        visible: root.active
                        text: root.phase === "running" ? "PAUSE" : "REPRENDRE"
                        iconName: root.phase === "running" ? "pause" : "play"
                        onClicked: root.toggle()
                    }
                    AppButton {
                        width: Math.min(220, (bigTimer.width - 60) / 2)
                        height: 64
                        visible: root.active
                        text: root.exIndex + 1 >= root.entries.length ? "Terminer" : "Suivant"
                        iconName: "skip"
                        onClicked: { root.next(); if (!root.active) bigTimer.close(); }
                    }
                }
            }
            IconButton {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 16
                iconName: "collapse"
                accessibleName: "Quitter le plein écran"
                fillColor: Theme.surface
                onClicked: bigTimer.close()
            }
        }
    }
}
