import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil

// Création ou modification d'une équipe (nom, catégorie, saison).
Popup {
    id: root

    property int teamId: -1
    property string errorText: ""
    signal saved(int id)

    function openFor(team) {
        teamId = team ? team.id : -1;
        nameField.text = team ? team.name : "";
        categoryField.text = team ? team.category : "";
        // Nouvelle équipe : la saison en cours est proposée d'office (modifiable).
        seasonField.text = team ? team.season : DateUtil.seasonOf(DateUtil.todayIso());
        errorText = "";
        open();
    }

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 20
    width: Math.min(parent ? parent.width - 32 : 360, 420)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? Math.min(parent.height * 0.1, Math.max(12, parent.height - height - 12)) : 0
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle { color: "#99000000" }
    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }
    onOpened: nameField.focusField()

    contentItem: Column {
        spacing: 14
        Text {
            width: parent.width
            text: root.teamId > 0 ? "Modifier l'équipe" : "Nouvelle équipe"
            font.pixelSize: 20
            font.weight: Font.Bold
            color: Theme.text
        }
        FormField { id: nameField; width: parent.width; label: "Nom"; required: true; placeholder: "Ex. FC Exemple" }
        FormField { id: categoryField; width: parent.width; label: "Catégorie"; placeholder: "Ex. U15, Seniors" }
        FormField { id: seasonField; width: parent.width; label: "Saison"; placeholder: "Ex. 2026-2027" }
        ChipRow {
            id: seasonChips
            width: parent.width
            allowClear: false
            options: DateUtil.seasonSuggestions(DateUtil.todayIso())
            onPicked: (id) => seasonField.text = id
            // Binding (et non « value: … ») : ChipRow écrit lui-même dans value au toucher, ce qui casserait une liaison simple.
            Binding { target: seasonChips; property: "value"; value: seasonField.text }
        }
        Text {
            visible: root.errorText !== ""
            width: parent.width
            text: root.errorText
            wrapMode: Text.Wrap
            font.pixelSize: 14
            color: Theme.danger
        }
        Row {
            width: parent.width
            spacing: 10
            layoutDirection: Qt.RightToLeft
            AppButton {
                kind: "primary"
                text: "Enregistrer"
                onClicked: {
                    var r = backend.saveTeam({ id: root.teamId, name: nameField.text,
                                               category: categoryField.text, season: seasonField.text });
                    if (r.ok) {
                        root.close();
                        root.saved(r.id);
                    } else {
                        root.errorText = r.error;
                    }
                }
            }
            AppButton { kind: "ghost"; text: "Annuler"; onClicked: root.close() }
        }
    }
}
