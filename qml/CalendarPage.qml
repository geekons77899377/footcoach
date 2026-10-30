import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil
import "Status.js" as Status

// Page « Calendrier » : mois, séances du jour / de la semaine / du mois, création (simple ou récurrente), modification,
// statut, suppression, appel. Les formulaires occupent toute la page (téléphone comme grand écran).
Item {
    id: page

    property string month: DateUtil.todayIso().substring(0, 7) + "-01"
    property string selected: DateUtil.todayIso()
    property string scope: "day"                 // day, week, month
    property var sessions: []
    property var marks: []
    property string mode: "browse"               // browse, session, recurrence, attendance
    readonly property bool split: width >= 640
    readonly property int teamId: AppState.teamId
    readonly property int leftWidth: width >= 900 ? 380 : 330     // colonne du mois en mode deux colonnes

    readonly property var scopes: [{ id: "day", text: "Jour" }, { id: "week", text: "Semaine" }, { id: "month", text: "Mois" }]

    function monthEnd(first) { return DateUtil.addDays(DateUtil.addMonths(first, 1), -1); }

    function rangeFrom() {
        if (scope === "week")
            return DateUtil.addDays(selected, -(DateUtil.weekday(selected) - 1));
        return scope === "month" ? selected.substring(0, 7) + "-01" : selected;
    }
    function rangeTo() {
        if (scope === "week")
            return DateUtil.addDays(rangeFrom(), 6);
        return scope === "month" ? monthEnd(selected.substring(0, 7) + "-01") : selected;
    }

    function refresh() {
        switcher.load();
        refreshData();
    }

    function refreshData() {
        if (teamId <= 0) {
            sessions = [];
            marks = [];
            return;
        }
        marks = backend.sessionDates(teamId, month, monthEnd(month));
        sessions = backend.sessions(teamId, rangeFrom(), rangeTo());
    }

    function select(day) {
        selected = day;
        month = day.substring(0, 7) + "-01";
        refresh();
    }

    function showMonth(first) {
        month = first;
        if (selected.substring(0, 7) !== first.substring(0, 7))
            selected = first;
        refresh();
    }

    function newSession() {
        sessionForm.openNew(teamId, selected);
        mode = "session";
    }
    function newRecurrence() {
        recurrenceForm.openNew(teamId, selected);
        mode = "recurrence";
    }
    function openAttendance(id) {
        var err = attendanceForm.openSession(id);
        if (err !== "") {
            toast.show(err);
            return;
        }
        mode = "attendance";
    }
    function changeStatus(id, status, force) {
        var r = backend.setSessionStatus(id, status, force === true);
        if (r.ok) {
            refresh();
        } else if (r.needsConfirm) {
            confirm.action = "status";
            confirm.targetId = id;
            confirm.targetStatus = status;
            confirm.confirm("Chevauchement", r.error + "\n\nChanger le statut quand même ?", "Changer", false);
        } else {
            toast.show(r.error);
        }
    }

    onTeamIdChanged: {
        mode = "browse";
        refreshData();
    }
    onScopeChanged: refreshData()
    Component.onCompleted: refresh()
    onVisibleChanged: if (visible) refresh()

    PromptDialog {
        id: confirm
        property string action: ""
        property int targetId: -1
        property string targetStatus: ""
        onAccepted: {
            if (action === "status") {
                page.changeStatus(targetId, targetStatus, true);
            } else if (action === "delete") {
                var r = backend.deleteSession(targetId);
                if (r.ok) {
                    page.mode = "browse";
                    page.refresh();
                } else {
                    toast.show(r.error);
                }
            }
        }
    }

    ActionSheet {
        id: rowSheet
        property var target: null
        title: target ? DateUtil.longLabel(target.date) + " · " + target.time : ""
        function openFor(s) {
            target = s;
            var m = [{ id: "attendance", text: "Faire l'appel", icon: "check" },
                     { id: "edit", text: "Modifier", icon: "edit" }];
            if (s.status !== "done")
                m.push({ id: "done", text: "Marquer réalisée", icon: "check" });
            if (s.status !== "planned")
                m.push({ id: "planned", text: "Remettre en prévue", icon: "calendar" });
            if (s.status !== "cancelled")
                m.push({ id: "cancelled", text: "Annuler la séance", icon: "minus" });
            m.push({ id: "delete", text: "Supprimer", icon: "trash", danger: true });
            model = m;
            open();
        }
        onChosen: (id) => {
            if (id === "attendance") {
                page.openAttendance(target.id);
            } else if (id === "edit") {
                sessionForm.openSession(target);
                page.mode = "session";
            } else if (id === "delete") {
                confirm.action = "delete";
                confirm.targetId = target.id;
                confirm.confirm("Supprimer la séance ?",
                                "La séance du " + DateUtil.display(target.date) + " à " + target.time
                                + " sera supprimée avec son appel et son plan d'exercices.", "Supprimer", true);
            } else {
                page.changeStatus(target.id, id, false);
            }
        }
    }

    // --- Cartes de séance -------------------------------------------------------------------------

    component SessionList: Column {
        id: sl
        spacing: 10
        Repeater {
            model: page.sessions
            delegate: Card {
                id: card
                required property var modelData
                width: sl.width
                height: 84
                padding: 12
                opacity: modelData.status === "cancelled" ? 0.6 : 1
                readonly property var st: Status.session(modelData.status)

                Column {
                    id: when
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 58
                    spacing: 2
                    Text {
                        text: card.modelData.time
                        font.pixelSize: 18
                        font.weight: Font.ExtraBold
                        color: Theme.text
                    }
                    Text {
                        text: "→ " + card.modelData.endTime
                        font.pixelSize: 12
                        color: Theme.textMuted
                    }
                }
                Rectangle { id: sep; anchors.left: when.right; anchors.leftMargin: 6; height: parent.height; width: 3; radius: 1.5; color: card.st.color }
                Column {
                    anchors.left: sep.right
                    anchors.leftMargin: 12
                    anchors.right: more.left
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3
                    Text {
                        width: parent.width
                        text: card.modelData.theme !== "" ? card.modelData.theme : "Séance"
                        elide: Text.ElideRight
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        font.pixelSize: 12
                        color: Theme.textMuted
                        text: (page.scope !== "day" ? DateUtil.longLabel(card.modelData.date) + " · " : "")
                              + (card.modelData.location !== "" ? card.modelData.location : card.modelData.durationMin + " min")
                    }
                    Row {
                        spacing: 8
                        Text {
                            text: card.st.label
                            font.pixelSize: 12
                            font.weight: Font.Bold
                            color: card.st.color
                        }
                        Text {
                            visible: card.modelData.total >= 0
                            text: "· " + card.modelData.present + "/" + card.modelData.total + " présents"
                            font.pixelSize: 12
                            color: Theme.textMuted
                        }
                    }
                }
                IconButton {
                    id: more
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    iconName: "more"
                    iconColor: Theme.textMuted
                    accessibleName: "Actions de la séance"
                    onClicked: rowSheet.openFor(card.modelData)
                }
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 52
                    onClicked: {
                        sessionForm.openSession(card.modelData);
                        page.mode = "session";
                    }
                }
            }
        }
        Card {
            visible: page.sessions.length === 0
            width: sl.width
            padding: 16
            Column {
                width: parent.width
                spacing: 10
                Text {
                    width: parent.width
                    text: page.scope === "day" ? "Aucune séance ce jour" : "Aucune séance sur cette période"
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: Theme.text
                }
                AppButton { kind: "primary"; text: "Planifier une séance"; iconName: "plus"; onClicked: page.newSession() }
            }
        }
    }

    // --- Navigation du mois -------------------------------------------------------------------------

    Item {
        id: browse
        anchors.fill: parent
        visible: page.mode === "browse"

        PageHeader {
            id: header
            width: parent.width
            title: "Calendrier"
            subtitle: switcher.current ? switcher.current.name : "Aucune équipe"
            TeamSwitcher { id: switcher; onChanged: page.refresh() }
            IconButton {
                visible: page.teamId > 0
                iconName: "repeat"
                fillColor: Theme.surface
                accessibleName: "Séances récurrentes"
                onClicked: page.newRecurrence()
            }
            IconButton {
                visible: page.teamId > 0
                iconName: "plus"
                iconColor: Theme.textOnAccent
                fillColor: Theme.accent
                diameter: 48
                glow: true
                accessibleName: "Nouvelle séance"
                onClicked: page.newSession()
            }
        }

        Card {
            visible: page.teamId <= 0
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Theme.gutter
            Column {
                width: parent.width
                spacing: 10
                Icon { name: "calendar"; color: Theme.accent; size: 32 }
                Text {
                    width: parent.width
                    text: "Créez d'abord une équipe"
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    color: Theme.text
                }
                Text {
                    width: parent.width
                    wrapMode: Text.Wrap
                    font.pixelSize: 14
                    color: Theme.textMuted
                    text: "Les séances appartiennent à une équipe : créez-la dans la page Effectif."
                }
            }
        }

        // Petit écran : tout dans une seule colonne défilante.
        Flickable {
            visible: page.teamId > 0 && !page.split
            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            contentWidth: width
            contentHeight: single.height + 32
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            Column {
                id: single
                x: Theme.gutter
                width: parent.width - 2 * Theme.gutter
                spacing: 12
                CalendarMonth {
                    width: parent.width
                    month: page.month
                    selected: page.selected
                    marks: page.marks
                    onPicked: (d) => page.select(d)
                    onMonthRequested: (m) => page.showMonth(m)
                }
                ChipRow {
                    width: parent.width
                    options: page.scopes
                    value: page.scope
                    allowClear: false
                    onPicked: (id) => page.scope = id
                }
                SessionList { width: parent.width }
            }
        }

        // Grand écran : mois à gauche, séances à droite.
        Item {
            visible: page.teamId > 0 && page.split
            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            Flickable {
                id: leftPane
                width: page.leftWidth
                height: parent.height
                contentWidth: width
                contentHeight: leftCol.height + 32
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                Column {
                    id: leftCol
                    x: Theme.gutter
                    width: parent.width - 2 * Theme.gutter
                    spacing: 12
                    CalendarMonth {
                        width: parent.width
                        month: page.month
                        selected: page.selected
                        marks: page.marks
                        onPicked: (d) => page.select(d)
                        onMonthRequested: (m) => page.showMonth(m)
                    }
                    ChipRow {
                        width: parent.width
                        options: page.scopes
                        value: page.scope
                        allowClear: false
                        onPicked: (id) => page.scope = id
                    }
                }
            }
            Rectangle { x: page.leftWidth; width: 1; height: parent.height; color: Theme.border }
            Flickable {
                x: page.leftWidth + 1
                width: parent.width - page.leftWidth - 1
                height: parent.height
                contentWidth: width
                contentHeight: rightCol.height + 40
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                SessionList {
                    id: rightCol
                    x: Theme.gutter
                    y: 8
                    width: Math.min(parent.width - 2 * Theme.gutter, 640)
                }
            }
        }
    }

    // --- Formulaires ----------------------------------------------------------------------------------

    SessionForm {
        id: sessionForm
        anchors.fill: parent
        visible: page.mode === "session"
        onCloseRequested: page.mode = "browse"
        onSaved: (day) => {
            page.mode = "browse";
            if (DateUtil.isIso(day))
                page.select(day);
            else
                page.refresh();
            toast.show("Séance enregistrée");
        }
        onDeleteRequested: {
            confirm.action = "delete";
            confirm.targetId = sessionForm.sessionId;
            confirm.confirm("Supprimer la séance ?", "La séance sera supprimée avec son appel et son plan d'exercices.",
                            "Supprimer", true);
        }
        onAttendanceRequested: page.openAttendance(sessionForm.sessionId)
    }

    RecurrenceForm {
        id: recurrenceForm
        anchors.fill: parent
        visible: page.mode === "recurrence"
        onCloseRequested: page.mode = "browse"
        onCreated: (count, firstDay) => {
            page.mode = "browse";
            if (DateUtil.isIso(firstDay))
                page.select(firstDay);
            else
                page.refresh();
            toast.show(count + (count > 1 ? " séances créées" : " séance créée"));
        }
    }

    AttendanceForm {
        id: attendanceForm
        anchors.fill: parent
        visible: page.mode === "attendance"
        onCloseRequested: page.mode = "browse"
        onSaved: {
            page.mode = "browse";
            page.refresh();
            toast.show("Appel enregistré");
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
        Timer { id: toastTimer; interval: 3500; onTriggered: toast.opacity = 0 }
    }
}
