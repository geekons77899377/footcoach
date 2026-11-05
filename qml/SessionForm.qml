import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil

// Fiche d'une séance : création ou modification (date, heure, durée, lieu, thème, statut, notes).
Item {
    id: root

    property bool showBack: true

    signal closeRequested()
    signal saved(string day)           // relayé depuis les champs
    signal deleteRequested()
    signal attendanceRequested()

    property alias teamId: fields.teamId
    property alias sessionId: fields.sessionId
    property alias errorText: fields.errorText
    property alias durationMin: fields.durationMin

    function openNew(team, day) {
        fields.openNew(team, day);
        scroll.contentY = 0;
    }

    function openSession(s) {
        fields.openSession(s);
        scroll.contentY = 0;
    }

    function save(force) { fields.save(force); }

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

    EnsureVisible { flick: scroll }

    Flickable {
        id: scroll
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        contentWidth: width
        contentHeight: fields.height + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        SessionFields {
            id: fields
            x: Math.max(Theme.gutter, (scroll.width - width) / 2)
            y: 8
            width: Math.min(scroll.width - 2 * Theme.gutter, 560)
            onSaved: (day) => root.saved(day)
            onFailed: scroll.contentY = 0
        }
    }
}
