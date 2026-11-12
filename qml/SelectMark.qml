import QtQuick

// Case à cocher ronde d'une ligne en mode sélection (Effectif).
Rectangle {
    id: root
    property bool checked: false

    width: 28
    height: 28
    radius: 14
    color: checked ? Theme.accent : "transparent"
    border.width: checked ? 0 : 2
    border.color: Theme.textMuted
    Accessible.role: Accessible.CheckBox
    Accessible.checked: checked

    Icon {
        visible: root.checked
        anchors.centerIn: parent
        name: "tick"
        color: Theme.textOnAccent
        size: 18
    }
}
