import QtQuick
import QtQuick.Controls
import "DateUtil.js" as DateUtil

// Fiche d'un joueur : création ou modification. Les erreurs de validation viennent du backend.
Item {
    id: root

    property int teamId: -1
    property int playerId: -1             // -1 : nouveau joueur
    property bool active: true
    property bool showBack: true
    property string errorText: ""

    signal closeRequested()
    signal saved()
    signal deleteRequested()
    signal toggleRequested()

    readonly property var positions: [
        { id: "GK", text: "Gardien" }, { id: "DEF", text: "Défenseur" },
        { id: "MID", text: "Milieu" }, { id: "FWD", text: "Attaquant" }
    ]
    readonly property var feet: [
        { id: "R", text: "Droit" }, { id: "L", text: "Gauche" }, { id: "B", text: "Les deux" }
    ]

    property string category: ""           // catégorie de l'équipe (ex. U15, Seniors) : choisit l'année proposée

    // Année de naissance proposée par le sélecteur : milieu de la tranche d'âge de la catégorie, ou d'environ
    // 12 ans (entre U6 et seniors) si elle est inconnue.
    readonly property int suggestedYear: {
        var now = new Date().getFullYear();
        var m = /U\s*(\d+)/i.exec(category);
        if (m)
            return now - Math.max(4, parseInt(m[1], 10) - 1);
        if (/s[ée]nior|v[ée]t[ée]ran/i.test(category))
            return now - 25;
        return now - 12;
    }

    function parseDate(text) { return DateUtil.parse(text); }

    function isoFromDisplay(text) { return DateUtil.parse(text); }

    // Met en forme la saisie en JJ/MM/AAAA quand elle est valide.
    function normalizeBirth() {
        var iso = parseDate(birth.text);
        if (/^\d{4}-\d{2}-\d{2}$/.test(iso))
            birth.text = displayFromIso(iso);
    }

    function displayFromIso(iso) { return DateUtil.display(iso); }

    function openNew(team) {
        teamId = team;
        playerId = -1;
        active = true;
        errorText = "";
        firstName.text = "";
        lastName.text = "";
        jersey.text = "";
        position.value = "";
        foot.value = "";
        birth.text = "";
        phone.text = "";
        email.text = "";
        notes.text = "";
        scroll.contentY = 0;
    }

    function openPlayer(p) {
        teamId = p.teamId;
        playerId = p.id;
        active = p.active;
        errorText = "";
        firstName.text = p.firstName;
        lastName.text = p.lastName;
        jersey.text = p.jerseyNumber > 0 ? String(p.jerseyNumber) : "";
        position.value = p.position;
        foot.value = p.preferredFoot;
        birth.text = displayFromIso(p.birthDate);
        phone.text = p.phone;
        email.text = p.email;
        notes.text = p.notes;
        scroll.contentY = 0;
    }

    function save() {
        var n = jersey.text.trim() === "" ? 0 : parseInt(jersey.text, 10);
        if (isNaN(n)) {
            errorText = "Le numéro de maillot doit être un nombre.";
            return;
        }
        var r = backend.savePlayer({
            id: playerId, teamId: teamId, firstName: firstName.text, lastName: lastName.text,
            jerseyNumber: n, position: position.value, preferredFoot: foot.value,
            birthDate: isoFromDisplay(birth.text), phone: phone.text, email: email.text,
            notes: notes.text, active: active
        });
        if (r.ok) {
            errorText = "";
            playerId = r.id;
            saved();
        } else {
            errorText = r.error;
            scroll.contentY = 0;
        }
    }

    PageHeader {
        id: header
        width: parent.width
        title: root.playerId > 0 ? "Fiche joueur" : "Nouveau joueur"
        leftInset: root.showBack ? 40 : 0
        IconButton {
            visible: root.playerId > 0
            iconName: "more"
            accessibleName: "Actions du joueur"
            onClicked: moreSheet.open()
        }
        IconButton {
            iconName: "check"
            iconColor: Theme.textOnAccent
            fillColor: Theme.accent
            diameter: 48
            glow: true
            accessibleName: "Enregistrer"
            onClicked: root.save()
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

    DatePicker {
        id: datePicker
        onPicked: (iso) => birth.text = root.displayFromIso(iso)
    }

    ActionSheet {
        id: moreSheet
        title: "Joueur"
        model: [
            { id: "toggle", text: root.active ? "Désactiver le joueur" : "Réactiver le joueur", icon: "users" },
            { id: "delete", text: "Supprimer définitivement", icon: "trash", danger: true }
        ]
        onChosen: (id) => id === "toggle" ? root.toggleRequested() : root.deleteRequested()
    }

    Flickable {
        id: scroll
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        contentWidth: width
        contentHeight: form.height + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        Column {
            id: form
            x: Theme.gutter
            y: 8
            width: Math.min(scroll.width - 2 * Theme.gutter, 560)
            spacing: 16

            Text {
                visible: root.errorText !== ""
                width: parent.width
                text: root.errorText
                wrapMode: Text.Wrap
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: Theme.danger
            }
            Text {
                visible: !root.active
                width: parent.width
                text: "Joueur inactif : il n'apparaît plus dans les listes par défaut et son numéro est libre."
                wrapMode: Text.Wrap
                font.pixelSize: 13
                color: Theme.warn
            }
            Row {
                width: parent.width
                spacing: 12
                FormField { id: firstName; width: (parent.width - 12) / 2; label: "Prénom"; required: true; maxLength: 60 }
                FormField { id: lastName; width: (parent.width - 12) / 2; label: "Nom"; required: true; maxLength: 60 }
            }
            Row {
                width: parent.width
                spacing: 12
                FormField {
                    id: jersey
                    width: (parent.width - 12) / 2
                    label: "Numéro"
                    placeholder: "—"
                    maxLength: 2
                    inputHints: Qt.ImhDigitsOnly
                }
                FormField {
                    id: birth
                    width: (parent.width - 12) / 2
                    label: "Naissance"
                    placeholder: "JJ/MM/AAAA"
                    maxLength: 10
                    inputHints: Qt.ImhPreferNumbers
                    actionIcon: "calendar"
                    actionName: "Choisir la date de naissance"
                    onActionClicked: {
                        datePicker.openAt(root.parseDate(birth.text), root.suggestedYear);
                    }
                    onEditingFinished: root.normalizeBirth()
                }
            }
            ChipRow { id: position; width: parent.width; label: "Poste"; options: root.positions }
            ChipRow { id: foot; width: parent.width; label: "Pied fort"; options: root.feet }
            FormField { id: phone; width: parent.width; label: "Téléphone"; maxLength: 30; inputHints: Qt.ImhDialableCharactersOnly }
            FormField { id: email; width: parent.width; label: "E-mail"; maxLength: 120; inputHints: Qt.ImhEmailCharactersOnly }
            FormField { id: notes; width: parent.width; label: "Notes"; multiline: true; maxLength: 1000 }

            AppButton {
                width: parent.width
                kind: "primary"
                text: "Enregistrer"
                iconName: "check"
                onClicked: root.save()
            }
        }
    }
}
