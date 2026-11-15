import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil

// Création ou modification d'une équipe (nom, catégorie, saison, éducateur responsable).
Popup {
    id: root

    property int teamId: -1
    property int clubId: -1                // club de la nouvelle équipe (celui d'une équipe existante ne change pas)
    property var clubs: []
    property var educators: []             // éducateurs du club de l'équipe (schéma 12)
    property int educatorId: -1            // responsable choisi, -1 : aucun
    property string errorText: ""
    signal saved(int id)

    function openFor(team, defaultClubId) {
        clubs = backend.clubs();
        clubId = team ? team.clubId : (defaultClubId > 0 ? defaultClubId : (clubs.length > 0 ? clubs[0].id : -1));
        teamId = team ? team.id : -1;
        nameField.text = team ? team.name : "";
        categoryField.text = team ? team.category : "";
        // Nouvelle équipe : la saison en cours est proposée d'office (modifiable).
        seasonField.text = team ? team.season : DateUtil.seasonOf(DateUtil.todayIso());
        educatorId = team && team.educatorId !== undefined ? team.educatorId : -1;
        reloadEducators();
        errorText = "";
        open();
    }
    function reloadEducators() {
        educators = clubId > 0 ? backend.educators(clubId) : [];
        var found = false;
        for (var i = 0; i < educators.length; ++i)
            found = found || educators[i].id === educatorId;
        if (!found)
            educatorId = -1;               // supprimé entre-temps, ou autre club
    }
    function clubNameOf(id) {
        for (var i = 0; i < clubs.length; ++i)
            if (clubs[i].id === id)
                return clubs[i].name;
        return "";
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
        KeyboardDismissAll { popup: root }   // y compris la zone assombrie autour
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
        ChipRow {
            id: clubChips
            objectName: "clubChips"
            visible: root.teamId <= 0 && root.clubs.length > 1
            width: parent.width
            label: "Club"
            allowClear: false
            options: {
                var o = [];
                for (var i = 0; i < root.clubs.length; ++i)
                    o.push({ id: String(root.clubs[i].id), text: root.clubs[i].name });
                return o;
            }
            onPicked: (id) => {
                root.clubId = parseInt(id, 10);
                root.reloadEducators();
            }
            Binding { target: clubChips; property: "value"; value: String(root.clubId) }
        }
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
        // Éducateur responsable : imprimé dans « Nom Responsable » de la feuille de challenge.
        ChipRow {
            id: educatorChips
            objectName: "educatorChips"
            visible: root.educators.length > 0
            width: parent.width
            label: "Éducateur responsable"
            options: {
                var o = [];
                for (var i = 0; i < root.educators.length; ++i)
                    o.push({ id: String(root.educators[i].id), text: root.educators[i].fullName });
                return o;
            }
            onPicked: (id) => root.educatorId = id === "" ? -1 : parseInt(id, 10)
            Binding { target: educatorChips; property: "value"; value: root.educatorId > 0 ? String(root.educatorId) : "" }
        }
        AppButton {
            objectName: "manageEducators"
            kind: "outline"
            text: root.educators.length > 0 ? "Gérer les éducateurs du club" : "Ajouter un éducateur responsable"
            onClicked: educatorsDialog.openFor(root.clubId, root.clubNameOf(root.clubId))
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
                    var r = backend.saveTeam({ id: root.teamId, clubId: root.clubId, name: nameField.text,
                                               category: categoryField.text, season: seasonField.text,
                                               educatorId: root.educatorId });
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

    EducatorsDialog {
        id: educatorsDialog
        onChanged: root.reloadEducators()
    }
}
