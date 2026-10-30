import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil
import "Status.js" as Status

// Appel d'une séance : un statut par joueur (présent, absent, excusé, blessé), note facultative par joueur.
Item {
    id: root

    property int sessionId: -1
    property var session: ({})
    property var rows: []                  // [{playerId, name, jerseyNumber, active, status, note}]
    property bool markDone: true
    property string errorText: ""
    property bool showBack: true
    readonly property int presentCount: {
        var n = 0;
        for (var i = 0; i < rows.length; ++i)
            if (rows[i].status === "present")
                ++n;
        return n;
    }

    signal closeRequested()
    signal saved()

    // Retourne un message d'erreur ("" si tout va bien).
    function openSession(id) {
        var r = backend.attendanceSheet(id);
        if (!r.ok)
            return r.error;
        sessionId = id;
        session = r.session;
        rows = r.players;
        markDone = r.session.status === "planned";
        errorText = "";
        return "";
    }

    function setStatus(i, status) {
        var list = rows.slice();
        list[i] = Object.assign({}, list[i], { status: status });
        rows = list;
    }

    function setAll(status) {
        rows = rows.map(function (r) { return Object.assign({}, r, { status: status }); });
    }

    function save() {
        var entries = rows.map(function (r) { return { playerId: r.playerId, status: r.status, note: r.note }; });
        var r = backend.saveAttendance(sessionId, entries, markDone);
        if (r.ok) {
            errorText = "";
            saved();
        } else {
            errorText = r.error;
        }
    }

    property int noteIndex: -1
    PromptDialog {
        id: noteDialog
        onAccepted: (text) => {
            if (root.noteIndex < 0)
                return;
            var list = root.rows.slice();
            list[root.noteIndex] = Object.assign({}, list[root.noteIndex], { note: text.trim() });
            root.rows = list;
        }
    }

    PageHeader {
        id: header
        width: parent.width
        title: "Appel"
        subtitle: root.session.date ? DateUtil.longLabel(root.session.date) + " · " + root.session.time : ""
        leftInset: root.showBack ? 40 : 0
        IconButton {
            iconName: "check"
            iconColor: Theme.textOnAccent
            fillColor: Theme.accent
            diameter: 48
            glow: true
            accessibleName: "Enregistrer l'appel"
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

    Column {
        id: top
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.leftMargin: Theme.gutter
        anchors.right: parent.right
        anchors.rightMargin: Theme.gutter
        anchors.topMargin: 4
        spacing: 10

        Text {
            width: parent.width
            text: root.presentCount + " présent(s) sur " + root.rows.length
            font.pixelSize: 15
            font.weight: Font.Bold
            color: Theme.text
            elide: Text.ElideRight
        }
        Row {
            spacing: 10
            AppButton { kind: "outline"; text: "Tous présents"; implicitHeight: 40; onClicked: root.setAll("present") }
            AppButton { kind: "outline"; text: "Tous absents"; implicitHeight: 40; onClicked: root.setAll("absent") }
        }
        Row {
            spacing: 14
            Repeater {
                model: Status.ATTENDANCE_ORDER
                delegate: Row {
                    required property string modelData
                    spacing: 5
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: Status.attendance(modelData).color
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: Status.attendance(modelData).label
                        font.pixelSize: 12
                        color: Theme.textMuted
                    }
                }
            }
        }
        Text {
            visible: root.errorText !== ""
            width: parent.width
            text: root.errorText
            wrapMode: Text.Wrap
            font.pixelSize: 14
            font.weight: Font.DemiBold
            color: Theme.danger
        }
    }

    ListView {
        id: list
        anchors.top: top.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 8
        leftMargin: Theme.gutter
        rightMargin: Theme.gutter
        bottomMargin: 8
        spacing: 8
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.rows

        delegate: Card {
            id: row
            required property var modelData
            required property int index
            width: list.width - list.leftMargin - list.rightMargin
            height: 64
            padding: 10
            opacity: modelData.active ? 1 : 0.65

            Rectangle {
                id: badge
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: 34
                height: 34
                radius: 17
                color: Theme.accentSoft
                Text {
                    anchors.centerIn: parent
                    text: row.modelData.jerseyNumber > 0 ? row.modelData.jerseyNumber : "–"
                    font.pixelSize: 14
                    font.weight: Font.ExtraBold
                    color: Theme.accent
                }
            }
            Item {
                anchors.left: badge.right
                anchors.leftMargin: 10
                anchors.right: buttons.left
                anchors.rightMargin: 8
                height: parent.height
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    spacing: 1
                    Text {
                        width: parent.width
                        text: row.modelData.name + (row.modelData.active ? "" : " (inactif)")
                        elide: Text.ElideRight
                        font.pixelSize: 15
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        visible: row.modelData.note !== ""
                        text: row.modelData.note
                        elide: Text.ElideRight
                        font.pixelSize: 12
                        font.italic: true
                        color: Theme.textMuted
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.noteIndex = row.index;
                        noteDialog.ask("Note — " + row.modelData.name, row.modelData.note, true, 200);
                    }
                }
            }
            Row {
                id: buttons
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6
                Repeater {
                    model: Status.ATTENDANCE_ORDER
                    delegate: Rectangle {
                        id: btn
                        required property string modelData
                        readonly property bool on: row.modelData.status === modelData
                        readonly property var st: Status.attendance(modelData)
                        width: 40
                        height: 40
                        radius: 12
                        color: on ? st.color : "transparent"
                        border.width: 2
                        border.color: on ? st.color : Theme.border
                        Accessible.role: Accessible.RadioButton
                        Accessible.name: st.label + " : " + row.modelData.name
                        Accessible.checked: on
                        Text {
                            anchors.centerIn: parent
                            text: btn.st.letter
                            font.pixelSize: 16
                            font.weight: Font.ExtraBold
                            color: btn.on ? "#0b1220" : btn.st.color
                        }
                        MouseArea { anchors.fill: parent; onClicked: root.setStatus(row.index, btn.modelData) }
                    }
                }
            }
        }
    }

    Item {
        id: footer
        anchors.bottom: parent.bottom
        width: parent.width
        height: (markRow.visible ? 56 : 0) + 64
        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.border }
        Row {
            id: markRow
            visible: root.session.status === "planned"
            x: Theme.gutter
            y: 6
            height: 48
            spacing: 10
            Switch {
                checked: root.markDone
                onToggled: root.markDone = checked
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: "Marquer la séance comme réalisée"
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: 14
                color: Theme.textMuted
            }
        }
        AppButton {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            anchors.left: parent.left
            anchors.leftMargin: Theme.gutter
            anchors.right: parent.right
            anchors.rightMargin: Theme.gutter
            kind: "primary"
            text: "Enregistrer l'appel"
            iconName: "check"
            onClicked: root.save()
        }
    }
}
