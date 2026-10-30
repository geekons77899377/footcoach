import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil
import "Status.js" as Status

// Plan d'une séance : exercices dans l'ordre, avec horaires calculés, durées, notes et matériel à prévoir.
Item {
    id: root

    property var session: null                // une séance de backend.sessions()
    property string teamName: ""
    property bool showBack: true
    property var planData: ({ entries: [], totalMin: 0, sessionMin: 0, equipment: [] })
    readonly property int diff: planData.totalMin - planData.sessionMin
    readonly property bool cancelled: session !== null && session.status === "cancelled"

    signal closeRequested()
    signal changed()                          // le plan a changé (la liste des séances se met à jour)

    function openSession(s, team) {
        session = s;
        teamName = team;
        reload();
        scroll.contentY = 0;
    }

    function reload() {
        if (!session)
            return;
        var r = backend.plan(session.id);
        if (r.ok)
            planData = r;
        else
            toast.show(r.error);
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

    ExercisePicker {
        id: picker
        onPicked: (e) => root.act(backend.addToPlan(root.session.id, e.id, 0))
    }
    PlanItemDialog {
        id: itemDialog
        onAccepted: (id, minutes, notes) => root.act(backend.updatePlanItem(id, minutes, notes))
    }

    Flickable {
        id: scroll
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

    Toast { id: toast }
}
