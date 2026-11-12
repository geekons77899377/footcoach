import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil

// Créneau régulier : jours de la semaine, heure, durée, période. Création : aperçu du nombre de séances créées.
// Modification (slotId > 0) : aperçu des séances à venir modifiées, ajoutées, supprimées ; déprogrammation en bloc.
Item {
    id: root

    property int teamId: -1
    property int slotId: -1               // > 0 : modification d'un créneau existant
    readonly property bool editing: slotId > 0
    property bool showBack: true
    property var weekdays: []             // 1 = lundi … 7 = dimanche
    property int durationMin: 90
    property string errorText: ""
    property var preview: ({ ok: false, count: 0, clashes: 0, error: "" })
    property var deletion: null           // aperçu de la déprogrammation en cours

    signal closeRequested()
    signal created(int count, string firstDay)
    signal slotChanged(string message)

    function openEdit(slot) {
        teamId = slot.teamId;
        slotId = slot.id;
        weekdays = slot.weekdays.slice();
        durationMin = slot.durationMin;
        errorText = "";
        fromField.text = DateUtil.display(slot.from);
        toField.text = DateUtil.display(slot.to);
        timeField.text = slot.time;
        theme.text = slot.theme;
        location.text = slot.location;
        scroll.contentY = 0;
        refreshPreview();
    }

    function plural(n, one, many) { return n + " " + (n > 1 ? many : one); }

    // Texte de l'aperçu d'une modification.
    function changeText(p) {
        var parts = [];
        if (p.updated > 0) parts.push(plural(p.updated, "séance modifiée", "séances modifiées"));
        if (p.added > 0) parts.push(plural(p.added, "séance ajoutée", "séances ajoutées"));
        if (p.removed > 0) parts.push(plural(p.removed, "séance supprimée", "séances supprimées")
                                      + (p.removedWithPlan > 0 ? " (dont " + p.removedWithPlan + " avec un plan d'exercices)" : ""));
        var t = parts.length === 0 ? "Aucune séance à venir ne change." : parts.join(" · ");
        if (p.kept > 0)
            t += "\n" + plural(p.kept, "séance gardée telle quelle (appel déjà fait)", "séances gardées telles quelles (appel déjà fait)");
        if (p.clashes > 0)
            t += "\n" + plural(p.clashes, "séance chevauchera", "séances chevaucheront") + " une autre séance de l'équipe.";
        return t;
    }

    function openNew(team, day) {
        teamId = team;
        slotId = -1;
        weekdays = [];
        durationMin = 90;
        errorText = "";
        fromField.text = DateUtil.display(day);
        toField.text = DateUtil.display(DateUtil.addMonths(day, 3));
        timeField.text = "18:30";
        theme.text = "";
        location.text = "";
        scroll.contentY = 0;
        refreshPreview();
    }

    function rule() {
        return { teamId: teamId, from: DateUtil.parse(fromField.text), to: DateUtil.parse(toField.text),
                 weekdays: weekdays, time: DateUtil.parseTime(timeField.text), durationMin: durationMin,
                 location: location.text, theme: theme.text };
    }

    function refreshPreview() { preview = editing ? backend.previewSlotUpdate(slotId, rule()) : backend.previewRecurrence(rule()); }

    function toggleDay(d) {
        var list = weekdays.slice();
        var i = list.indexOf(d);
        if (i >= 0)
            list.splice(i, 1);
        else
            list.push(d);
        weekdays = list;
        refreshPreview();
    }

    function create(force) {
        var r = backend.createRecurring(rule(), force === true);
        if (r.ok) {
            created(r.count, DateUtil.parse(fromField.text));
        } else if (r.needsConfirm) {
            confirm.action = "create";
            confirm.confirm("Chevauchements", r.error + "\n\nCréer quand même toutes les séances ?", "Créer", false);
        } else {
            errorText = r.error;
            scroll.contentY = 0;
        }
    }

    function save(force, removalConfirmed) {
        // Des séances vont disparaître : on demande d'abord confirmation.
        if (!removalConfirmed && preview.ok && preview.removed > 0) {
            confirm.action = "removal";
            confirm.confirm("Modifier le créneau ?",
                            plural(preview.removed, "séance à venir sera supprimée", "séances à venir seront supprimées")
                            + (preview.removedWithPlan > 0 ? ", dont " + preview.removedWithPlan + " avec un plan d'exercices" : "")
                            + ".", "Modifier", true);
            return;
        }
        var r = backend.updateSlot(slotId, rule(), force === true);
        if (r.ok) {
            slotChanged("Créneau modifié");
        } else if (r.needsConfirm) {
            confirm.action = "clash";
            confirm.confirm("Chevauchements", r.error + "\n\nModifier quand même ?", "Modifier", false);
        } else {
            errorText = r.error;
            scroll.contentY = 0;
        }
    }

    // Déprogrammation : choix de l'étendue s'il reste des séances passées sans appel, puis confirmation.
    function askDelete() {
        var d = backend.previewSlotDelete(slotId);
        if (!d.ok) {
            errorText = d.error;
            return;
        }
        deletion = d;
        if (d.past > 0) {
            scopeSheet.model = [
                { id: "upcoming", text: "Séances à venir (" + d.upcoming + ")", icon: "calendar" },
                { id: "all", text: "À venir et passées sans appel (" + (d.upcoming + d.past) + ")", icon: "trash", danger: true }];
            scopeSheet.open();
        } else {
            confirmDelete(false);
        }
    }

    function confirmDelete(includePast) {
        var d = deletion;
        var n = d.upcoming + (includePast ? d.past : 0);
        var kept = d.kept + (includePast ? 0 : d.past);
        var withPlan = d.withPlan + (includePast ? d.withPlanPast : 0);
        var msg = n === 0 ? "Aucune séance ne sera supprimée."
                          : plural(n, "séance sera supprimée", "séances seront supprimées")
                            + (withPlan > 0 ? ", dont " + withPlan + " avec un plan d'exercices" : "") + ".";
        if (kept > 0)
            msg += "\n" + plural(kept, "séance restera dans le calendrier", "séances resteront dans le calendrier")
                   + " comme séance seule.";
        confirm.action = includePast ? "deleteAll" : "deleteUpcoming";
        confirm.confirm("Déprogrammer le créneau ?", msg, "Déprogrammer", true);
    }

    function doDelete(includePast) {
        var r = backend.deleteSlot(slotId, includePast);
        if (r.ok)
            slotChanged(r.removed === 0 ? "Créneau déprogrammé"
                                        : "Créneau déprogrammé : " + plural(r.removed, "séance supprimée", "séances supprimées"));
        else
            errorText = r.error;
    }

    onDurationMinChanged: refreshPreview()

    PromptDialog {
        id: confirm
        property string action: "create"
        onAccepted: {
            if (action === "removal") root.save(false, true);
            else if (action === "clash") root.save(true, true);
            else if (action === "deleteUpcoming") root.doDelete(false);
            else if (action === "deleteAll") root.doDelete(true);
            else root.create(true);
        }
    }
    ActionSheet {
        id: scopeSheet
        title: "Que faut-il supprimer ?"
        onChosen: (id) => root.confirmDelete(id === "all")
    }
    DatePicker {
        id: datePicker
        property var target: null
        maxYear: new Date().getFullYear() + 2
        yearCount: 6
        onPicked: (iso) => { if (target) target.text = DateUtil.display(iso); root.refreshPreview(); }
    }
    TimePicker {
        id: timePicker
        onPicked: (t) => { timeField.text = t; root.refreshPreview(); }
    }

    PageHeader {
        id: header
        width: parent.width
        title: root.editing ? "Modifier le créneau" : "Nouveau créneau régulier"
        leftInset: root.showBack ? 40 : 0
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
                width: parent.width
                text: root.errorText
                wrapMode: Text.Wrap
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: Theme.danger
            }

            Column {
                width: parent.width
                spacing: 6
                Text {
                    text: "Jours *"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: Theme.textMuted
                }
                Row {
                    spacing: 6
                    Repeater {
                        model: 7
                        delegate: Rectangle {
                            id: dayChip
                            required property int index
                            readonly property bool on: root.weekdays.indexOf(index + 1) >= 0
                            width: Math.floor((form.width - 36) / 7)
                            height: 46
                            radius: 14
                            color: on ? Theme.accentSoft : Theme.surface
                            border.width: on ? 2 : 1
                            border.color: on ? Theme.accent : Theme.border
                            Accessible.role: Accessible.CheckBox
                            Accessible.name: DateUtil.DAYS_LONG[index]
                            Accessible.checked: on
                            Text {
                                anchors.centerIn: parent
                                text: DateUtil.DAYS_SHORT[dayChip.index]
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: dayChip.on ? Theme.accent : Theme.text
                            }
                            MouseArea { anchors.fill: parent; onClicked: root.toggleDay(dayChip.index + 1) }
                        }
                    }
                }
            }
            Row {
                width: parent.width
                spacing: 12
                FormField {
                    id: timeField
                    width: (parent.width - 12) / 2
                    label: "Heure"
                    required: true
                    maxLength: 5
                    inputHints: Qt.ImhPreferNumbers
                    actionIcon: "clock"
                    actionName: "Choisir l'heure"
                    onActionClicked: timePicker.openAt(DateUtil.parseTime(timeField.text))
                    onEditingFinished: {
                        var t = DateUtil.parseTime(timeField.text);
                        if (DateUtil.isTime(t))
                            timeField.text = t;
                        root.refreshPreview();
                    }
                }
                Column {
                    width: (parent.width - 12) / 2
                    spacing: 6
                    Text {
                        text: "Durée"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: Theme.textMuted
                    }
                    Row {
                        spacing: 4
                        IconButton {
                            iconName: "minus"
                            fillColor: Theme.surface
                            accessibleName: "Durée moins 15 minutes"
                            onClicked: root.durationMin = Math.max(15, root.durationMin - 15)
                        }
                        Text {
                            width: 58
                            height: 44
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: root.durationMin + " min"
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        IconButton {
                            iconName: "plus"
                            fillColor: Theme.surface
                            accessibleName: "Durée plus 15 minutes"
                            onClicked: root.durationMin = Math.min(300, root.durationMin + 15)
                        }
                    }
                }
            }
            Row {
                width: parent.width
                spacing: 12
                FormField {
                    id: fromField
                    width: (parent.width - 12) / 2
                    label: "Du"
                    required: true
                    maxLength: 10
                    inputHints: Qt.ImhPreferNumbers
                    actionIcon: "calendar"
                    actionName: "Choisir la date de début"
                    onActionClicked: { datePicker.target = fromField; datePicker.title = "Premier jour"; datePicker.openAt(DateUtil.parse(fromField.text), new Date().getFullYear()); }
                    onEditingFinished: {
                        var iso = DateUtil.parse(fromField.text);
                        if (DateUtil.isIso(iso))
                            fromField.text = DateUtil.display(iso);
                        root.refreshPreview();
                    }
                }
                FormField {
                    id: toField
                    width: (parent.width - 12) / 2
                    label: "Au (inclus)"
                    required: true
                    maxLength: 10
                    inputHints: Qt.ImhPreferNumbers
                    actionIcon: "calendar"
                    actionName: "Choisir la date de fin"
                    onActionClicked: { datePicker.target = toField; datePicker.title = "Dernier jour"; datePicker.openAt(DateUtil.parse(toField.text), new Date().getFullYear()); }
                    onEditingFinished: {
                        var iso = DateUtil.parse(toField.text);
                        if (DateUtil.isIso(iso))
                            toField.text = DateUtil.display(iso);
                        root.refreshPreview();
                    }
                }
            }
            FormField { id: theme; width: parent.width; label: "Thème"; maxLength: 120 }
            FormField { id: location; width: parent.width; label: "Lieu"; maxLength: 120 }

            Card {
                width: parent.width
                padding: 14
                Column {
                    width: parent.width
                    spacing: 6
                    Text {
                        objectName: "slotPreview"
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: root.preview.ok ? Theme.text : Theme.warn
                        text: !root.preview.ok ? root.preview.error
                              : root.editing ? root.changeText(root.preview)
                              : root.preview.count + (root.preview.count > 1 ? " séances seront créées" : " séance sera créée")
                                + (root.preview.clashes > 0 ? " (dont " + root.preview.clashes + " en chevauchement)" : "")
                    }
                    Text {
                        visible: root.editing
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 12
                        color: Theme.textMuted
                        text: "Seules les séances à venir changent. Une séance avec un appel n'est jamais modifiée, "
                              + "et un champ modifié à la main sur une séance est conservé."
                    }
                }
            }
            AppButton {
                width: parent.width
                kind: "primary"
                text: root.editing ? "Enregistrer les modifications" : "Créer les séances"
                iconName: root.editing ? "check" : "repeat"
                enabled: root.preview.ok
                onClicked: root.editing ? root.save(false, false) : root.create(false)
            }
            AppButton {
                visible: root.editing
                width: parent.width
                kind: "danger"
                text: "Déprogrammer le créneau"
                iconName: "trash"
                onClicked: root.askDelete()
            }
        }
    }
}
