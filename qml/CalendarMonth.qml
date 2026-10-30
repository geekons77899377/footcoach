import QtQuick
import "DateUtil.js" as DateUtil

// Mois affiché en grille (semaines du lundi au dimanche) : jours avec séances marqués d'un point, jour choisi en vert.
Item {
    id: root

    property string month: DateUtil.todayIso().substring(0, 7) + "-01"     // premier jour du mois affiché
    property string selected: DateUtil.todayIso()
    property var marks: []                                                   // jours (AAAA-MM-JJ) avec séance
    readonly property string today: DateUtil.todayIso()
    readonly property int cell: Math.max(34, Math.min(52, Math.floor(width / 7)))
    readonly property int offset: DateUtil.weekday(month) - 1               // cases vides avant le 1er
    readonly property int dayCount: new Date(DateUtil.fromIso(month).getFullYear(), DateUtil.fromIso(month).getMonth() + 1, 0).getDate()
    readonly property int rows: Math.ceil((offset + dayCount) / 7)

    signal picked(string day)
    signal monthRequested(string firstDay)

    implicitHeight: 52 + 28 + rows * cell + 8

    Row {
        id: head
        width: parent.width
        height: 52
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            iconName: "back"
            accessibleName: "Mois précédent"
            onClicked: root.monthRequested(DateUtil.addMonths(root.month, -1))
        }
        Text {
            width: parent.width - 2 * 44 - 2 * 8
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignHCenter
            text: {
                var d = DateUtil.fromIso(root.month);
                return DateUtil.MONTHS[d.getMonth()].charAt(0).toUpperCase() + DateUtil.MONTHS[d.getMonth()].slice(1) + " " + d.getFullYear();
            }
            font.pixelSize: 18
            font.weight: Font.Bold
            color: Theme.text
            MouseArea {
                anchors.fill: parent
                onClicked: root.picked(DateUtil.todayIso())        // toucher le titre : revenir à aujourd'hui
            }
        }
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            iconName: "chevron"
            accessibleName: "Mois suivant"
            onClicked: root.monthRequested(DateUtil.addMonths(root.month, 1))
        }
    }

    Row {
        id: weekdays
        anchors.top: head.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: root.cell * 7
        height: 28
        Repeater {
            model: DateUtil.DAYS_SHORT
            delegate: Text {
                required property string modelData
                width: root.cell
                height: 28
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: modelData
                font.pixelSize: 12
                font.weight: Font.DemiBold
                color: Theme.textMuted
            }
        }
    }

    Grid {
        anchors.top: weekdays.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        columns: 7
        Repeater {
            model: root.rows * 7
            delegate: Item {
                id: slot
                required property int index
                readonly property int dayNumber: index - root.offset + 1
                readonly property bool valid: dayNumber >= 1 && dayNumber <= root.dayCount
                readonly property string day: valid ? root.month.substring(0, 8) + (dayNumber < 10 ? "0" : "") + dayNumber : ""
                readonly property bool isSelected: valid && day === root.selected
                width: root.cell
                height: root.cell

                Rectangle {
                    visible: slot.valid
                    anchors.centerIn: parent
                    width: root.cell - 6
                    height: root.cell - 6
                    radius: width / 2
                    color: slot.isSelected ? Theme.accent : "transparent"
                    border.width: slot.valid && slot.day === root.today && !slot.isSelected ? 2 : 0
                    border.color: Theme.accent
                    Text {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -2
                        text: slot.dayNumber
                        font.pixelSize: 15
                        font.weight: slot.isSelected ? Font.ExtraBold : Font.DemiBold
                        color: slot.isSelected ? Theme.textOnAccent : Theme.text
                    }
                    Rectangle {
                        visible: root.marks.indexOf(slot.day) >= 0
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 5
                        width: 5
                        height: 5
                        radius: 2.5
                        color: slot.isSelected ? Theme.textOnAccent : Theme.accent
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    enabled: slot.valid
                    onClicked: root.picked(slot.day)
                }
            }
        }
    }
}
