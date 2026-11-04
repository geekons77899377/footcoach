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
    property int expandedId: -1                  // séance dont le détail est déployé (grand écran)
    readonly property int pad: Theme.gutter + 4  // aligné sur le titre d'en-tête

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

    // Libellé de la période affichée (jour, semaine ou mois) et pas d'une période à l'autre.
    function periodLabel() {
        var months = DateUtil.MONTHS;
        if (scope === "day")
            return DateUtil.longLabel(selected);
        if (scope === "month") {
            var d = DateUtil.fromIso(selected);
            return months[d.getMonth()].charAt(0).toUpperCase() + months[d.getMonth()].slice(1) + " " + d.getFullYear();
        }
        var a = DateUtil.fromIso(rangeFrom()), b = DateUtil.fromIso(rangeTo());
        return "Semaine du " + a.getDate() + (a.getMonth() !== b.getMonth() ? " " + months[a.getMonth()] : "")
               + " au " + b.getDate() + " " + months[b.getMonth()];
    }
    function stepPeriod(dir) {
        if (scope === "day")
            select(DateUtil.addDays(selected, dir));
        else if (scope === "week")
            select(DateUtil.addDays(selected, 7 * dir));
        else
            select(DateUtil.addMonths(selected.substring(0, 7) + "-01", dir));
    }
    function summaryText() {
        var n = sessions.length, planned = 0, done = 0;
        for (var i = 0; i < n; ++i) {
            if (sessions[i].status === "planned") planned++;
            else if (sessions[i].status === "done") done++;
        }
        if (n === 0)
            return "Aucune séance";
        return n + (n > 1 ? " séances" : " séance") + (planned > 0 ? " · " + planned + " prévue" + (planned > 1 ? "s" : "") : "")
               + (done > 0 ? " · " + done + " réalisée" + (done > 1 ? "s" : "") : "");
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

    // Bouton retour du téléphone : ferme le formulaire ouvert (séance, récurrence, appel).
    function goBack() {
        if (mode === "browse")
            return false;
        mode = "browse";
        return true;
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
    onScopeChanged: {
        expandedId = -1;
        refreshData();
    }
    Connections {
        target: backend
        function onDataReplaced() {
            page.mode = "browse";
            page.refresh();
        }
    }

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
            delegate: Column {
                id: entry
                required property var modelData
                required property int index
                readonly property bool grouped: page.split && page.scope !== "day"
                readonly property bool newDay: grouped && (index === 0 || page.sessions[index - 1].date !== modelData.date)
                width: sl.width
                spacing: 8
                Text {
                    visible: entry.newDay
                    width: parent.width
                    topPadding: entry.index === 0 ? 0 : 8
                    text: DateUtil.longLabel(entry.modelData.date).toUpperCase()
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    font.letterSpacing: 1.2
                    color: entry.modelData.date === DateUtil.todayIso() ? Theme.accent : Theme.textMuted
                }
                Card {
                    id: card
                    objectName: "sessionCard"
                    readonly property var st: Status.session(entry.modelData.status)
                    readonly property bool expanded: page.split && page.expandedId === entry.modelData.id
                    width: parent.width
                    height: 84 + (expanded ? details.height + 12 : 0)
                    padding: 12
                    radius: Theme.radiusLg
                    highlighted: expanded
                    opacity: entry.modelData.status === "cancelled" ? 0.6 : 1
                    Behavior on height { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

                    Item {
                        id: head
                        width: parent.width
                        height: 60
                        Column {
                            id: when
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 58
                            spacing: 2
                            Text {
                                text: entry.modelData.time
                                font.family: Theme.fontHeading
                                font.pixelSize: 19
                                font.weight: Font.ExtraBold
                                color: Theme.text
                            }
                            Text {
                                text: "→ " + entry.modelData.endTime
                                font.pixelSize: 12
                                color: Theme.textMuted
                            }
                        }
                        Rectangle { id: sep; anchors.left: when.right; anchors.leftMargin: 6; height: parent.height; width: 4; radius: 2; color: card.st.color }
                        Column {
                            anchors.left: sep.right
                            anchors.leftMargin: 12
                            anchors.right: more.left
                            anchors.rightMargin: 6
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 3
                            Text {
                                width: parent.width
                                text: entry.modelData.theme !== "" ? entry.modelData.theme : "Séance"
                                elide: Text.ElideRight
                                font.family: Theme.fontHeading
                                font.pixelSize: 17
                                font.weight: Font.Bold
                                color: Theme.text
                            }
                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                font.pixelSize: 12
                                color: Theme.textMuted
                                text: (page.scope !== "day" && !entry.grouped ? DateUtil.longLabel(entry.modelData.date) + " · " : "")
                                      + (entry.modelData.location !== "" ? entry.modelData.location : entry.modelData.durationMin + " min")
                            }
                            Row {
                                spacing: 8
                                Text {
                                    text: card.st.label.toUpperCase()
                                    font.pixelSize: 11
                                    font.weight: Font.Bold
                                    font.letterSpacing: 0.8
                                    color: card.st.color
                                }
                                Text {
                                    visible: entry.modelData.total >= 0
                                    text: "· " + entry.modelData.present + "/" + entry.modelData.total + " présents"
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
                            onClicked: rowSheet.openFor(entry.modelData)
                        }
                        MouseArea {
                            anchors.fill: parent
                            anchors.rightMargin: 52
                            onClicked: {
                                if (page.split) {
                                    page.expandedId = card.expanded ? -1 : entry.modelData.id;
                                } else {
                                    sessionForm.openSession(entry.modelData);
                                    page.mode = "session";
                                }
                            }
                        }
                    }

                    // Détail (grand écran) : informations de la séance, plan d'exercices et actions.
                    Loader {
                        id: details
                        objectName: "sessionDetails"
                        active: card.expanded
                        visible: card.expanded
                        anchors.top: head.bottom
                        anchors.topMargin: 12
                        width: parent.width
                        height: item ? item.implicitHeight : 0
                        sourceComponent: Column {
                            id: dcol
                            width: card.width - 24
                            spacing: 10
                            readonly property var entries: {
                                var r = backend.plan(entry.modelData.id);
                                return r && r.entries ? r.entries : [];
                            }
                            Rectangle { width: parent.width; height: 1; color: Theme.borderSubtle }
                            Text {
                                width: parent.width
                                text: DateUtil.longLabel(entry.modelData.date) + " · " + entry.modelData.time + " – " + entry.modelData.endTime
                                      + " · " + entry.modelData.durationMin + " min"
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                color: Theme.text
                            }
                            Text {
                                visible: entry.modelData.location !== ""
                                width: parent.width
                                text: "Lieu : " + entry.modelData.location
                                font.pixelSize: 13
                                color: Theme.textMuted
                            }
                            Text {
                                visible: entry.modelData.notes !== ""
                                width: parent.width
                                wrapMode: Text.Wrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                                text: entry.modelData.notes
                                font.pixelSize: 13
                                color: Theme.textMuted
                            }
                            Text {
                                text: "PLAN DE SÉANCE"
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 1.2
                                color: Theme.textMuted
                            }
                            Text {
                                visible: dcol.entries.length === 0
                                width: parent.width
                                text: "Aucun exercice planifié."
                                font.pixelSize: 13
                                color: Theme.textMuted
                            }
                            Repeater {
                                model: dcol.entries.slice(0, 6)
                                delegate: Item {
                                    required property var modelData
                                    required property int index
                                    width: dcol.width
                                    height: 22
                                    Text {
                                        anchors.left: parent.left
                                        anchors.right: dur.left
                                        anchors.rightMargin: 8
                                        anchors.verticalCenter: parent.verticalCenter
                                        elide: Text.ElideRight
                                        text: (parent.index + 1) + ".  " + parent.modelData.title
                                        font.pixelSize: 13
                                        color: Theme.text
                                    }
                                    Text {
                                        id: dur
                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: parent.modelData.durationMin + "'"
                                        font.pixelSize: 13
                                        font.weight: Font.Bold
                                        color: Theme.textMuted
                                    }
                                }
                            }
                            Text {
                                visible: dcol.entries.length > 6
                                text: "+ " + (dcol.entries.length - 6) + " autre(s) exercice(s)"
                                font.pixelSize: 12
                                color: Theme.textMuted
                            }
                            Row {
                                spacing: 10
                                AppButton {
                                    objectName: "detailAttendance"
                                    kind: "primary"
                                    text: "Faire l'appel"
                                    iconName: "check"
                                    onClicked: page.openAttendance(entry.modelData.id)
                                }
                                AppButton {
                                    objectName: "detailEdit"
                                    text: "Modifier"
                                    iconName: "edit"
                                    onClicked: {
                                        sessionForm.openSession(entry.modelData);
                                        page.mode = "session";
                                    }
                                }
                            }
                            Item { width: 1; height: 2 }
                        }
                    }
                }
            }
        }
        Card {
            objectName: "calendarEmpty"
            visible: page.sessions.length === 0
            width: sl.width
            padding: 20
            radius: Theme.radiusLg
            Column {
                width: parent.width
                spacing: 10
                Icon { name: "calendar"; color: Theme.accent; size: 28 }
                Text {
                    width: parent.width
                    text: page.scope === "day" ? "Aucune séance ce jour" : "Aucune séance sur cette période"
                    font.family: Theme.fontHeading
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    color: Theme.text
                }
                Text {
                    width: parent.width
                    wrapMode: Text.Wrap
                    font.pixelSize: 13
                    color: Theme.textMuted
                    text: page.periodLabel() + " : rien de planifié pour l'équipe."
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

        // Grand écran : mois à gauche (dans un panneau), séances à droite avec navigation de période.
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
                    x: page.pad
                    y: 4
                    width: parent.width - page.pad - Theme.gutter
                    spacing: 12
                    Card {
                        objectName: "monthPanel"
                        width: parent.width
                        height: wideMonth.implicitHeight + 2 * padding
                        padding: 12
                        radius: Theme.radiusLg
                        CalendarMonth {
                            id: wideMonth
                            width: parent.width
                            month: page.month
                            selected: page.selected
                            marks: page.marks
                            onPicked: (d) => page.select(d)
                            onMonthRequested: (m) => page.showMonth(m)
                        }
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
            Item {
                id: rightPane
                x: page.leftWidth + 1
                width: parent.width - page.leftWidth - 1
                height: parent.height

                // Période : précédente / suivante, libellé, retour à aujourd'hui.
                Item {
                    id: periodBar
                    objectName: "periodBar"
                    readonly property bool narrow: width < 560          // tablette portrait : deux lignes
                    x: Theme.gutter
                    y: 4
                    width: Math.min(parent.width - 2 * Theme.gutter, 760)
                    height: narrow ? 100 : 64
                    IconButton {
                        id: prevBtn
                        objectName: "periodPrev"
                        y: periodBar.narrow ? 0 : (parent.height - height) / 2
                        iconName: "back"
                        accessibleName: "Période précédente"
                        onClicked: page.stepPeriod(-1)
                    }
                    IconButton {
                        id: nextBtn
                        objectName: "periodNext"
                        anchors.left: prevBtn.right
                        anchors.leftMargin: 4
                        y: prevBtn.y
                        iconName: "chevron"
                        accessibleName: "Période suivante"
                        onClicked: page.stepPeriod(1)
                    }
                    Column {
                        x: periodBar.narrow ? 4 : nextBtn.x + nextBtn.width + 12
                        y: periodBar.narrow ? 52 : (parent.height - height) / 2
                        width: periodBar.narrow ? parent.width - 4 : todayBtn.x - x - 8
                        spacing: 2
                        Text {
                            objectName: "periodLabel"
                            width: parent.width
                            elide: Text.ElideRight
                            text: page.periodLabel()
                            font.family: Theme.fontHeading
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: page.summaryText()
                            font.pixelSize: 13
                            color: Theme.textMuted
                        }
                    }
                    AppButton {
                        id: todayBtn
                        objectName: "periodToday"
                        x: parent.width - width
                        y: periodBar.narrow ? (prevBtn.height - height) / 2 : (parent.height - height) / 2
                        text: "Aujourd'hui"
                        onClicked: page.select(DateUtil.todayIso())
                    }
                }
                Flickable {
                    anchors.top: periodBar.bottom
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    contentWidth: width
                    contentHeight: rightCol.height + 40
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    SessionList {
                        id: rightCol
                        x: Theme.gutter
                        y: 4
                        width: Math.min(parent.width - 2 * Theme.gutter, 760)
                    }
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
