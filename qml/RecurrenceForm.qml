import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil

// Séances récurrentes : jours de la semaine, heure, durée, période ; aperçu du nombre de séances créées.
Item {
    id: root

    property int teamId: -1
    property bool showBack: true
    property var weekdays: []             // 1 = lundi … 7 = dimanche
    property int durationMin: 90
    property string errorText: ""
    property var preview: ({ ok: false, count: 0, clashes: 0, error: "" })

    signal closeRequested()
    signal created(int count, string firstDay)

    function openNew(team, day) {
        teamId = team;
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

    function refreshPreview() { preview = backend.previewRecurrence(rule()); }

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
            confirm.confirm("Chevauchements", r.error + "\n\nCréer quand même toutes les séances ?", "Créer", false);
        } else {
            errorText = r.error;
            scroll.contentY = 0;
        }
    }

    onDurationMinChanged: refreshPreview()

    PromptDialog {
        id: confirm
        onAccepted: root.create(true)
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
        title: "Séances récurrentes"
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
                Text {
                    width: parent.width
                    wrapMode: Text.Wrap
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: root.preview.ok ? Theme.text : Theme.warn
                    text: root.preview.ok
                          ? root.preview.count + (root.preview.count > 1 ? " séances seront créées" : " séance sera créée")
                            + (root.preview.clashes > 0 ? " (dont " + root.preview.clashes + " en chevauchement)" : "")
                          : root.preview.error
                }
            }
            AppButton {
                width: parent.width
                kind: "primary"
                text: "Créer les séances"
                iconName: "repeat"
                enabled: root.preview.ok
                onClicked: root.create(false)
            }
        }
    }
}
