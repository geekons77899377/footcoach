import QtQuick
import QtQuick.Controls

// Paramètres > Stades des matchs (schéma 11) : département du club, mise à jour de la liste depuis Data ES
// (recensement des équipements sportifs du ministère des Sports), stade du club, terrains indisponibles et lieux
// saisis à la main. La mise à jour ne supprime rien : un terrain retiré de Data ES ou hors service n'est plus proposé
// pour les nouveaux matchs, mais les matchs déjà prévus le gardent.
Item {
    id: page

    property bool showBack: true
    property int clubId: -1
    property var clubs: []
    property var summary: ({})
    property var allVenues: []
    property string filter: "available"       // available, unavailable, all
    property string query: ""
    property string lastResult: ""            // bilan de la dernière mise à jour (texte affiché)
    property var actionVenue: null            // lieu dont la feuille d'actions est ouverte
    // Champs de l'en-tête de la liste (déclarés dans le composant header, inaccessibles par leur id).
    property Item departmentField: null
    property Item searchField: null
    signal closeRequested()

    readonly property bool busy: backend.venuesBusy
    readonly property var filters: [{ id: "available", text: "Proposés" }, { id: "unavailable", text: "Non proposés" },
                                    { id: "all", text: "Tous" }]
    readonly property var shown: {
        var q = fold(query.trim()), words = q === "" ? [] : q.split(/\s+/), out = [];
        for (var i = 0; i < allVenues.length; ++i) {
            var v = allVenues[i];
            if (filter === "available" && !v.selectable)
                continue;
            if (filter === "unavailable" && v.selectable)
                continue;
            var hay = fold([v.city, v.postcode, v.name, v.field, v.address].join(" ")), ok = true;
            for (var w = 0; w < words.length && ok; ++w)
                ok = hay.indexOf(words[w]) >= 0;
            if (ok)
                out.push(v);
        }
        return out;
    }

    function fold(s) {
        return String(s || "").normalize("NFD").replace(/[̀-ͯ]/g, "").toLowerCase();
    }

    // Ouvre la page sur le club de l'équipe courante (ou le premier club).
    function openFor(teamId) {
        clubs = backend.clubs();
        var c = teamId > 0 ? backend.clubOfTeam(teamId) : -1;
        if (c <= 0 && clubs.length > 0)
            c = clubs[0].id;
        lastResult = "";
        query = "";
        if (searchField)
            searchField.text = "";
        filter = "available";
        selectClub(c);
    }

    function selectClub(id) {
        clubId = id;
        reload();
        if (departmentField)
            departmentField.text = summary.department || "";
    }

    function reload() {
        summary = clubId > 0 ? backend.venueSummary(clubId) : ({});
        allVenues = clubId > 0 ? backend.venues(clubId, true) : [];
    }

    function statusText(v) {
        if (v.unavailable)
            return "Marqué indisponible";
        if (v.esStatus === "removed")
            return "Retiré de la liste officielle";
        if (v.esStatus === "closed")
            return "Hors service (liste officielle)";
        return "";
    }

    function saveDepartment() {
        var d = departmentField ? departmentField.text.trim().toUpperCase() : (summary.department || "");
        if (d === (summary.department || ""))
            return true;
        var r = backend.setClubDepartment(clubId, d);
        if (!r.ok) {
            toast.show(r.error);
            return false;
        }
        departmentField.text = d;
        reload();
        return true;
    }

    function startRefresh() {
        if (!saveDepartment())
            return;
        if ((summary.department || "") === "") {
            toast.show("Indiquez d'abord le département du club (ex. 60).");
            return;
        }
        lastResult = "";
        if (!backend.refreshVenues(clubId))
            toast.show(backend.lastError);
    }

    // « 1 terrain reçu », « 6 terrains reçus » : n suivi du singulier ou du pluriel.
    function plural(n, one, many) {
        return n + " " + (n > 1 ? many : one);
    }

    function describe(r) {
        if (!r.ok)
            return r.error;
        var parts = [plural(r.total, "terrain reçu", "terrains reçus")];
        if (r.added > 0)
            parts.push(plural(r.added, "nouveau", "nouveaux"));
        if (r.reopened > 0)
            parts.push(plural(r.reopened, "de nouveau en service", "de nouveau en service"));
        if (r.closed > 0)
            parts.push(plural(r.closed, "hors service", "hors service"));
        if (r.removed > 0)
            parts.push(plural(r.removed, "retiré de la liste officielle", "retirés de la liste officielle"));
        return "Liste mise à jour : " + parts.join(", ") + ".";
    }

    Connections {
        target: backend
        function onVenuesRefreshed(r) {
            if (r.clubId !== page.clubId)
                return;
            page.lastResult = page.describe(r);
            page.reload();
        }
    }

    Rectangle { anchors.fill: parent; color: Theme.bg }

    PageHeader {
        id: header
        width: parent.width
        title: "Stades des matchs"
        subtitle: page.summary.clubName || ""
        leftInset: page.showBack ? 52 : 0
    }
    IconButton {
        visible: page.showBack
        objectName: "venuesBack"
        x: Theme.gutter - 4
        anchors.verticalCenter: header.verticalCenter
        iconName: "back"
        accessibleName: "Retour"
        onClicked: page.closeRequested()
    }

    ListView {
        id: list
        objectName: "venuesList"
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        width: Math.min(parent.width - 2 * (Theme.gutter + 4), 720)
        anchors.horizontalCenter: parent.horizontalCenter
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: page.shown
        bottomMargin: 24 + AppState.bottomInset

        header: Column {
            width: list.width
            spacing: 14
            topPadding: 16
            bottomPadding: 8

            ChipRow {
                objectName: "venueClubChips"
                visible: page.clubs.length > 1
                width: parent.width
                label: "Club"
                allowClear: false
                options: page.clubs.map(function (c) { return { id: String(c.id), text: c.name }; })
                value: String(page.clubId)
                onPicked: (id) => page.selectClub(parseInt(id, 10))
            }

            Card {
                objectName: "refreshCard"
                width: parent.width
                Column {
                    width: parent.width
                    spacing: 10
                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: "Les terrains de football du département viennent du recensement des équipements sportifs "
                              + "du ministère des Sports (Data ES). Seul le numéro du département est envoyé."
                        font.pixelSize: 13
                        color: Theme.textMuted
                    }
                    FormField {
                        id: department
                        objectName: "departmentField"
                        width: 140
                        label: "Département"
                        placeholder: "Ex. 60"
                        maxLength: 3
                        inputHints: Qt.ImhNoPredictiveText | Qt.ImhPreferUppercase
                        onEditingFinished: page.saveDepartment()
                        Component.onCompleted: { page.departmentField = department; text = page.summary.department || ""; }
                    }
                    AppButton {
                        objectName: "refreshButton"
                        width: parent.width
                        kind: "primary"
                        iconName: page.busy ? "" : "rotateR"
                        text: page.busy ? "Téléchargement…" : "Mettre à jour les stades"
                        enabled: !page.busy && page.clubId > 0
                        onClicked: page.startRefresh()
                    }
                    AppButton {
                        objectName: "cancelRefresh"
                        visible: page.busy
                        kind: "outline"
                        text: "Annuler"
                        onClicked: backend.cancelVenueRefresh()
                    }
                    Text {
                        objectName: "refreshResult"
                        visible: page.lastResult !== ""
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: page.lastResult
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: Theme.accent
                    }
                    Text {
                        objectName: "venueSummary"
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 13
                        color: Theme.textMuted
                        text: {
                            var s = page.summary;
                            if (!s.ok)
                                return "";
                            if (s.total === 0)
                                return "Aucun stade pour l'instant.";
                            var t = page.plural(s.selectable, "proposé", "proposés") + " sur " + s.total;
                            if (s.unavailable > 0) t += " · " + page.plural(s.unavailable, "marqué indisponible", "marqués indisponibles");
                            if (s.removed > 0) t += " · " + page.plural(s.removed, "retiré", "retirés");
                            if (s.closed > 0) t += " · " + s.closed + " hors service";
                            if (s.lastRefresh)
                                t += "\nDernière mise à jour : " + s.lastRefresh.substring(8, 10) + "/" + s.lastRefresh.substring(5, 7)
                                     + "/" + s.lastRefresh.substring(0, 4) + " à " + s.lastRefresh.substring(11, 16);
                            return t;
                        }
                    }
                }
            }

            Card {
                objectName: "homeCard"
                width: parent.width
                Column {
                    width: parent.width
                    spacing: 4
                    Text { text: "Stade du club (matchs à domicile)"; font.pixelSize: 13; font.weight: Font.DemiBold; color: Theme.textMuted }
                    Text {
                        objectName: "homeLabel"
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: page.summary.homeLabel ? page.summary.homeLabel : "Aucun : touchez un stade de la liste pour le choisir."
                        font.pixelSize: 15
                        font.weight: page.summary.homeLabel ? Font.Bold : Font.Normal
                        color: page.summary.homeLabel ? Theme.text : Theme.textMuted
                    }
                }
            }

            AppButton {
                objectName: "addManualVenue"
                kind: "outline"
                iconName: "plus"
                text: "Ajouter un lieu à la main"
                onClicked: manualForm.openNew()
            }

            Rectangle {
                width: parent.width
                height: 48
                radius: Theme.radiusSm
                color: Theme.surface
                border.width: 1
                border.color: search.activeFocus ? Theme.accent : Theme.border
                Icon {
                    id: searchIcon
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    name: "search"
                    color: Theme.textMuted
                    size: 20
                }
                TextField {
                    KeyboardRetry {}
                    id: search
                    objectName: "venuesSearch"
                    anchors.left: searchIcon.right
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 4
                    color: Theme.text
                    placeholderText: "Rechercher : commune, stade, rue…"
                    placeholderTextColor: Theme.textDim
                    background: null
                    font.pixelSize: 16
                    inputMethodHints: Qt.ImhNoPredictiveText
                    onTextChanged: page.query = text
                    Component.onCompleted: page.searchField = search
                }
            }

            ChipRow {
                objectName: "venueFilter"
                width: parent.width
                allowClear: false
                options: page.filters
                value: page.filter
                onPicked: (id) => page.filter = id
            }

            Text {
                objectName: "venuesEmpty"
                visible: page.shown.length === 0
                width: parent.width
                wrapMode: Text.Wrap
                text: page.allVenues.length === 0 ? "La liste est vide : indiquez le département puis touchez « Mettre à jour les stades »."
                                                  : "Aucun stade dans cette liste."
                font.pixelSize: 14
                color: Theme.textMuted
            }
        }

        delegate: Rectangle {
            id: row
            required property var modelData
            width: list.width
            height: 74
            radius: 14
            color: tap.pressed ? Theme.surfaceHi : "transparent"
            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: row.modelData.city + " · " + row.modelData.name
                    font.pixelSize: 15
                    font.weight: Font.Bold
                    color: row.modelData.selectable ? Theme.text : Theme.textMuted
                }
                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: [row.modelData.field, row.modelData.address].filter(function (s) { return s && s !== ""; }).join(" · ")
                    font.pixelSize: 13
                    color: Theme.textMuted
                }
                Text {
                    readonly property string status: page.statusText(row.modelData)
                    width: parent.width
                    elide: Text.ElideRight
                    text: [row.modelData.isHome ? "Stade du club" : "", status,
                           row.modelData.source === "manual" ? "Saisi à la main" : ""]
                          .filter(function (s) { return s !== ""; }).join(" · ")
                    visible: text !== ""
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    color: status !== "" ? Theme.warn : Theme.accent
                }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.borderSubtle }
            MouseArea {
                id: tap
                anchors.fill: parent
                onClicked: {
                    page.actionVenue = row.modelData;
                    var v = row.modelData, m = [];
                    if (!v.isHome)
                        m.push({ id: "home", text: "Choisir comme stade du club", icon: "home" });
                    else
                        m.push({ id: "nohome", text: "Ne plus en faire le stade du club", icon: "close" });
                    m.push(v.unavailable ? { id: "available", text: "Remettre disponible", icon: "tick" }
                                         : { id: "unavailable", text: "Marquer indisponible", icon: "close" });
                    if (v.source === "manual") {
                        m.push({ id: "edit", text: "Modifier", icon: "edit" });
                        m.push({ id: "delete", text: "Supprimer", icon: "trash", danger: true });
                    }
                    actions.title = v.city + " · " + v.name;
                    actions.model = m;
                    actions.open();
                }
            }
        }
    }

    EnsureVisible { flick: list }

    ActionSheet {
        id: actions
        objectName: "venueActions"
        onChosen: (id) => {
            close();
            var v = page.actionVenue, r = null;
            if (!v)
                return;
            if (id === "home")
                r = backend.setHomeVenue(page.clubId, v.id);
            else if (id === "nohome")
                r = backend.setHomeVenue(page.clubId, -1);
            else if (id === "unavailable" || id === "available")
                r = backend.setVenueUnavailable(v.id, id === "unavailable");
            else if (id === "edit")
                manualForm.openEdit(v);
            else if (id === "delete")
                confirmDelete.confirm("Supprimer ce lieu ?", "Les matchs déjà prévus à cet endroit gardent son adresse.",
                                      "Supprimer", true);
            if (r && !r.ok)
                toast.show(r.error);
            page.reload();
        }
    }

    PromptDialog {
        id: confirmDelete
        objectName: "confirmVenueDelete"
        onAccepted: {
            var r = backend.deleteVenue(page.actionVenue.id);
            if (!r.ok)
                toast.show(r.error);
            page.reload();
        }
    }

    // Lieu saisi à la main : nom obligatoire, adresse, code postal, commune.
    Popup {
        id: manualForm
        objectName: "manualVenueForm"
        property int editId: -1
        function openNew() { editId = -1; fName.text = ""; fField.text = ""; fAddress.text = ""; fCp.text = ""; fCity.text = ""; formError.text = ""; open(); }
        function openEdit(v) {
            editId = v.id; fName.text = v.name; fField.text = v.field; fAddress.text = v.address; fCp.text = v.postcode;
            fCity.text = v.city; formError.text = ""; open();
        }
        parent: Overlay.overlay
        modal: true
        dim: true
        padding: 16
        width: Math.min(parent ? parent.width - 24 : 360, 520)
        x: parent ? (parent.width - width) / 2 : 0
        y: parent ? Math.max(12, (parent.height - height) / 2 - AppState.bottomInset / 2) : 0
        closePolicy: Popup.CloseOnEscape
        Overlay.modal: Rectangle { color: "#99000000" }
        background: Rectangle { radius: 26; color: Theme.surface; border.width: 1; border.color: Theme.border }
        contentItem: Column {
            spacing: 10
            Text {
                text: manualForm.editId > 0 ? "Modifier le lieu" : "Nouveau lieu"
                font.pixelSize: 18
                font.weight: Font.Bold
                color: Theme.text
            }
            Text { id: formError; visible: text !== ""; width: parent.width; wrapMode: Text.Wrap; color: Theme.danger; font.pixelSize: 13 }
            FormField { id: fName; objectName: "venueNameField"; width: parent.width; label: "Nom du stade"; required: true; maxLength: 80; placeholder: "Ex. Stade municipal" }
            FormField { id: fField; objectName: "venueFieldField"; width: parent.width; label: "Terrain"; maxLength: 80; placeholder: "Ex. Terrain synthétique" }
            FormField { id: fAddress; objectName: "venueAddressField"; width: parent.width; label: "Adresse"; maxLength: 120 }
            Row {
                width: parent.width
                spacing: 12
                FormField { id: fCp; objectName: "venueCpField"; width: 110; label: "Code postal"; maxLength: 5; inputHints: Qt.ImhDigitsOnly }
                FormField { id: fCity; objectName: "venueCityField"; width: parent.width - 122; label: "Commune"; maxLength: 80 }
            }
            Row {
                spacing: 10
                anchors.right: parent.right
                AppButton { text: "Annuler"; kind: "outline"; onClicked: manualForm.close() }
                AppButton {
                    objectName: "saveVenueButton"
                    text: "Enregistrer"
                    kind: "primary"
                    onClicked: {
                        var r = backend.saveVenue({ id: manualForm.editId, clubId: page.clubId, name: fName.text,
                                                    field: fField.text, address: fAddress.text, postcode: fCp.text,
                                                    city: fCity.text });
                        if (!r.ok) {
                            formError.text = r.error;
                            return;
                        }
                        manualForm.close();
                        page.reload();
                        toast.show("Lieu enregistré");
                    }
                }
            }
        }
    }

    Toast { id: toast }
}
