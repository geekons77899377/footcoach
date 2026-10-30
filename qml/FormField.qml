import QtQuick
import QtQuick.Controls

// Champ de formulaire : libellé au-dessus, saisie arrondie (une ou plusieurs lignes).
Column {
    id: root

    property string label: ""
    property alias text: field.text
    property string placeholder: ""
    property bool multiline: false
    property int maxLength: 100
    property int inputHints: Qt.ImhNone
    property bool required: false
    property string actionIcon: ""          // bouton rond à droite du champ (ex. calendrier)
    property string actionName: ""
    signal actionClicked()
    signal editingFinished()

    function focusField() { field.forceActiveFocus(); }

    spacing: 6

    Text {
        text: root.label + (root.required ? " *" : "")
        font.pixelSize: 13
        font.weight: Font.DemiBold
        color: Theme.textMuted
    }
    Rectangle {
        width: parent.width
        height: root.multiline ? 104 : 50
        radius: Theme.radiusSm
        color: Theme.bg
        border.width: 1
        border.color: field.activeFocus ? Theme.accent : Theme.border

        IconButton {
            visible: root.actionIcon !== ""
            z: 2
            anchors.right: parent.right
            anchors.rightMargin: 3
            anchors.verticalCenter: parent.verticalCenter
            diameter: 42
            iconName: root.actionIcon
            iconColor: Theme.accent
            accessibleName: root.actionName
            onClicked: root.actionClicked()
        }
        Flickable {
            anchors.fill: parent
            anchors.margins: 2
            clip: true
            contentWidth: width
            contentHeight: field.implicitHeight
            TextArea.flickable: TextArea {
                id: field
                color: Theme.text
                placeholderText: root.placeholder
                placeholderTextColor: Theme.textDim
                font.pixelSize: 16
                wrapMode: root.multiline ? TextEdit.Wrap : TextEdit.NoWrap
                selectByMouse: true
                background: null
                leftPadding: 12
                rightPadding: root.actionIcon !== "" ? 52 : 12
                verticalAlignment: root.multiline ? TextEdit.AlignTop : TextEdit.AlignVCenter
                inputMethodHints: root.inputHints
                onTextChanged: {
                    if (text.length > root.maxLength)
                        text = text.substring(0, root.maxLength);
                }
                onActiveFocusChanged: if (!activeFocus) root.editingFinished()
                Keys.onReturnPressed: (event) => {
                    if (root.multiline)
                        event.accepted = false;
                    else
                        field.focus = false;
                }
            }
        }
    }
}
