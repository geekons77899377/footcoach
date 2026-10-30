import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil

// Fiche d'une séance : création ou modification (date, heure, durée, lieu, thème, statut, notes).
Item {
    id: root

    property int teamId: -1
    property int sessionId: -1
    property string errorText: ""
    property bool showBack: true
    property int durationMin: 90

    signal closeRequested()
    signal saved(string day)
    signal deleteRequested()
    signal attendanceRequested()

    readonly property var statuses: [{ id: "planned", text: "Prévue" }, { id: "done", text: "Réalisée" },
                                     { id: "cancelled", text: "Annulée" }]

    function openNew(team, day) {
        teamId = team;
        sessionId = -1;
        errorText = "";
        dateField.text = DateUtil.display(day);
        timeField.text = "18:30";
        durationMin = 90;
        location.text = "";
        theme.text = "";
        status.value = "planned";
        notes.text = "";
        scroll.contentY = 0;
    }

    function openSession(s) {
        teamId = s.teamId;
        sessionId = s.id;
        errorText = "";
        dateField.text = DateUtil.display(s.date);
        timeField.text = s.time;
        durationMin = s.durationMin;
        location.text = s.location;
        theme.text = s.theme;
        status.value = s.status;
        notes.text = s.notes;
        scroll.contentY = 0;
    }

    function save(force) {
        var day = DateUtil.parse(dateField.text);
        var time = DateUtil.parseTime(timeField.text);
        var r = backend.saveSession({
            id: sessionId, teamId: teamId, date: day, time: time, durationMin: durationMin,
            location: location.text, theme: theme.text, status: status.value || "planned", notes: notes.text
        }, force === true);
        if (r.ok) {
            errorText = "";
            sessionId = r.id;
            saved(day);
        } else if (r.needsConfirm) {
            confirm.confirm("Chevauchement", r.error + "\n\nL'enregistrer quand même ?", "Enregistrer", false);
        } else {
            errorText = r.error;
            scroll.contentY = 0;
        }
    }

    PromptDialog {
        id: confirm
        onAccepted: root.save(true)
    }
    DatePicker {
        id: datePicker
        title: "Date de la séance"
        maxYear: new Date().getFullYear() + 2
        yearCount: 6
        onPicked: (iso) => dateField.text = DateUtil.display(iso)
    }
    TimePicker {
        id: timePicker
        title: "Heure de début"
        onPicked: (t) => timeField.text = t
    }
    ActionSheet {
        id: moreSheet
        title: "Séance"
        model: [
            { id: "attendance", text: "Faire l'appel", icon: "check" },
            { id: "delete", text: "Supprimer la séance", icon: "trash", danger: true }
        ]
        onChosen: (id) => id === "attendance" ? root.attendanceRequested() : root.deleteRequested()
    }

    PageHeader {
        id: header
        width: parent.width
        title: root.sessionId > 0 ? "Séance" : "Nouvelle séance"
        leftInset: root.showBack ? 40 : 0
        IconButton {
            visible: root.sessionId > 0
            iconName: "more"
            accessibleName: "Actions de la séance"
            onClicked: moreSheet.open()
        }
        IconButton {
            iconName: "check"
            iconColor: Theme.textOnAccent
            fillColor: Theme.accent
            diameter: 48
            glow: true
            accessibleName: "Enregistrer"
            onClicked: root.save(false)
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
                    label: "Heure"
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
            Column {
                width: parent.width
                spacing: 6
                Text {
                    text: "Durée"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: Theme.textMuted
                }
                Row {
                    spacing: 12
                    IconButton {
                        iconName: "minus"
                        fillColor: Theme.surface
                        accessibleName: "Durée moins 15 minutes"
                        enabled: root.durationMin > 15
                        onClicked: root.durationMin = Math.max(15, root.durationMin - 15)
                    }
                    Text {
                        width: 90
                        height: 44
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        text: root.durationMin + " min"
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    IconButton {
                        iconName: "plus"
                        fillColor: Theme.surface
                        accessibleName: "Durée plus 15 minutes"
                        enabled: root.durationMin < 300
                        onClicked: root.durationMin = Math.min(300, root.durationMin + 15)
                    }
                }
            }
            FormField { id: theme; width: parent.width; label: "Thème"; placeholder: "Ex. Jeu de possession, finition"; maxLength: 120 }
            FormField { id: location; width: parent.width; label: "Lieu"; placeholder: "Ex. Stade municipal, terrain 2"; maxLength: 120 }
            ChipRow { id: status; width: parent.width; label: "Statut"; options: root.statuses; allowClear: false; value: "planned" }
            FormField { id: notes; width: parent.width; label: "Notes"; multiline: true; maxLength: 1000 }

            AppButton {
                width: parent.width
                kind: "primary"
                text: "Enregistrer"
                iconName: "check"
                onClicked: root.save(false)
            }
        }
    }
}
