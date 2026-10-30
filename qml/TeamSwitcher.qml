import QtQuick

// Bouton « équipe » d'un en-tête : ouvre la liste des équipes ; le choix est partagé par toutes les pages (AppState).
Item {
    id: root

    property var teams: []                 // [{id, name, category, ...}] (backend.teams())
    readonly property var current: {
        for (var i = 0; i < teams.length; ++i)
            if (teams[i].id === AppState.teamId)
                return teams[i];
        return null;
    }
    signal changed()

    // À appeler après avoir relu les équipes : garde une équipe valide sélectionnée.
    function load() {
        teams = backend.teams();
        var ok = false;
        for (var i = 0; i < teams.length; ++i)
            if (teams[i].id === AppState.teamId)
                ok = true;
        if (!ok)
            AppState.teamId = teams.length > 0 ? teams[0].id : -1;
    }

    implicitWidth: button.implicitWidth
    implicitHeight: button.implicitHeight

    IconButton {
        id: button
        iconName: "users"
        accessibleName: "Changer d'équipe"
        fillColor: Theme.surface
        onClicked: {
            var m = [];
            for (var i = 0; i < root.teams.length; ++i)
                m.push({ id: "t" + root.teams[i].id,
                         text: root.teams[i].name + (root.teams[i].category ? " · " + root.teams[i].category : ""),
                         icon: "users", checked: root.teams[i].id === AppState.teamId });
            sheet.model = m;
            sheet.open();
        }
    }
    ActionSheet {
        id: sheet
        title: "Équipe"
        onChosen: (id) => {
            AppState.teamId = parseInt(id.substring(1), 10);
            root.changed();
        }
    }
}
