import QtQuick

// Choix unique présenté en pastilles. options : [{id, text}] ; value : id choisi ("" = aucun).
Column {
    id: root

    property string label: ""
    property var options: []
    property string value: ""
    property bool allowClear: true       // retoucher la pastille choisie l'efface
    signal picked(string id)

    spacing: 6

    Text {
        visible: root.label !== ""
        text: root.label
        font.pixelSize: 13
        font.weight: Font.DemiBold
        color: Theme.textMuted
    }
    Flow {
        width: parent.width
        spacing: 8
        Repeater {
            model: root.options
            delegate: Rectangle {
                id: chip
                required property var modelData
                readonly property bool on: root.value === modelData.id
                height: 40
                width: chipText.implicitWidth + 28
                radius: 20
                color: on ? Theme.accentSoft : Theme.surface
                border.width: on ? 2 : 1
                border.color: on ? Theme.accent : Theme.border
                Accessible.role: Accessible.Button
                Accessible.name: modelData.text
                Text {
                    id: chipText
                    anchors.centerIn: parent
                    text: chip.modelData.text
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: chip.on ? Theme.accent : Theme.text
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        var next = (chip.on && root.allowClear) ? "" : chip.modelData.id;
                        root.value = next;
                        root.picked(next);
                    }
                }
            }
        }
    }
}
