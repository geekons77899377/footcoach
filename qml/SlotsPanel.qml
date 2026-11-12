import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil

// Créneaux réguliers de l'équipe : chaque créneau a généré des séances ; on le modifie ou on le déprogramme d'un coup.
Item {
    id: root

    property int teamId: -1
    property bool showBack: true
    property var slots: []

    signal closeRequested()
    signal newRequested()
    signal editRequested(var slot)

    function refresh() { slots = teamId > 0 ? backend.trainingSlots(teamId) : []; }

    function daysText(days) {
        var out = [];
        for (var i = 0; i < days.length; ++i)
            out.push(DateUtil.DAYS_LONG[days[i] - 1]);
        if (out.length === 0)
            return "";
        var s = out.join(", ");
        return s.charAt(0).toUpperCase() + s.substring(1);
    }

    function countText(slot) {
        var n = slot.upcoming;
        var t = n === 0 ? "Aucune séance à venir" : n + (n > 1 ? " séances à venir" : " séance à venir");
        if (slot.withAttendance > 0)
            t += " · " + slot.withAttendance + (slot.withAttendance > 1 ? " avec appel" : " avec appel");
        return t;
    }

    onVisibleChanged: if (visible) refresh()

    PageHeader {
        id: header
        width: parent.width
        title: "Créneaux réguliers"
        subtitle: "Entraînements à jour et heure fixes"
        leftInset: root.showBack ? 40 : 0
        IconButton {
            iconName: "plus"
            iconColor: Theme.textOnAccent
            fillColor: Theme.accent
            diameter: 48
            glow: true
            accessibleName: "Nouveau créneau"
            onClicked: root.newRequested()
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
        contentHeight: list.height + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        Column {
            id: list
            x: Math.max(Theme.gutter, (scroll.width - width) / 2)
            y: 8
            width: Math.min(scroll.width - 2 * Theme.gutter, 560)
            spacing: 10

            Card {
                visible: root.slots.length === 0
                width: parent.width
                Column {
                    width: parent.width
                    spacing: 12
                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: "Aucun créneau régulier pour cette équipe."
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: "Un créneau crée d'un coup toutes les séances d'un entraînement à jour et heure fixes. "
                              + "Vous pourrez ensuite modifier l'heure, le lieu ou les jours de toutes les séances à venir, "
                              + "ou les déprogrammer en une fois."
                        font.pixelSize: 13
                        color: Theme.textMuted
                    }
                    AppButton {
                        width: parent.width
                        kind: "primary"
                        iconName: "repeat"
                        text: "Nouveau créneau"
                        onClicked: root.newRequested()
                    }
                }
            }

            Repeater {
                model: root.slots
                delegate: Card {
                    id: slotCard
                    objectName: "slotCard"
                    required property var modelData
                    width: list.width
                    padding: 14
                    Accessible.role: Accessible.Button
                    Accessible.name: root.daysText(modelData.weekdays) + " " + modelData.time
                    Column {
                        id: info
                        width: parent.width - 28
                        spacing: 4
                        Text {
                            width: parent.width
                            wrapMode: Text.Wrap
                            text: root.daysText(slotCard.modelData.weekdays) + " · " + slotCard.modelData.time
                                  + " – " + slotCard.modelData.endTime
                            font.pixelSize: 16
                            font.weight: Font.Bold
                            color: Theme.text
                        }
                        Text {
                            visible: text !== ""
                            width: parent.width
                            elide: Text.ElideRight
                            text: [slotCard.modelData.theme, slotCard.modelData.location].filter(function (x) { return x !== ""; }).join(" · ")
                            font.pixelSize: 13
                            color: Theme.text
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.Wrap
                            text: "Du " + DateUtil.display(slotCard.modelData.from) + " au " + DateUtil.display(slotCard.modelData.to)
                            font.pixelSize: 13
                            color: Theme.textMuted
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.Wrap
                            text: root.countText(slotCard.modelData)
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            color: slotCard.modelData.upcoming > 0 ? Theme.accent : Theme.textMuted
                        }
                    }
                    Icon {
                        anchors.right: parent.right
                        y: (info.height - height) / 2
                        name: "chevron"
                        size: 18
                        color: Theme.textMuted
                    }
                    MouseArea { anchors.fill: parent; onClicked: root.editRequested(slotCard.modelData) }
                }
            }
        }
    }
}
