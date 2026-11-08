import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import "DateUtil.js" as DateUtil

// Fiche d'un joueur : création ou modification. Les erreurs de validation viennent du backend.
Item {
    id: root

    property int teamId: -1
    property int playerId: -1             // -1 : nouveau joueur
    property bool active: true
    property bool showBack: true
    property string errorText: ""
    // Photo : « photoShown » est ce qu'on voit (data URL JPEG) ; « photoNew » n'est enregistrée qu'avec le joueur.
    property string photoShown: ""
    property string photoNew: ""
    property bool photoRemoved: false

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

    // Initiales affichées quand il n'y a pas de photo.
    readonly property string initials: {
        var a = firstName.text.trim(), b = lastName.text.trim();
        var t = (a ? a.charAt(0) : "") + (b ? b.charAt(0) : "");
        return t.toUpperCase();
    }

    // Lit l'image choisie, la réduit côté application (480 px, JPEG) et l'affiche ; rien n'est enregistré avant « Enregistrer ».
    function pickPhoto(url) {
        var r = backend.preparePhoto(url);
        if (r.ok) {
            photoNew = r.data;
            photoShown = r.data;
            photoRemoved = false;
            errorText = "";
        } else {
            errorText = r.error;
            scroll.contentY = 0;
        }
    }

    function clearPhoto() {
        photoNew = "";
        photoShown = "";
        photoRemoved = true;
    }

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
        licence.text = "";
        photoShown = "";
        photoNew = "";
        photoRemoved = false;
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
        licence.text = p.licence ? p.licence : "";
        photoNew = "";
        photoRemoved = false;
        photoShown = p.hasPhoto ? backend.playerPhoto(p.id) : "";
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
            notes: notes.text, active: active, licence: licence.text,
            photo: photoNew, removePhoto: photoRemoved
        });
        if (r.ok) {
            errorText = "";
            playerId = r.id;
            photoNew = "";
            photoRemoved = false;
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

    FileDialog {
        id: photoDialog
        title: "Choisir une photo"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Images (*.jpg *.jpeg *.png *.webp *.heic *.bmp)", "Tous les fichiers (*)"]
        onAccepted: root.pickPhoto(selectedFile.toString())
    }

    EnsureVisible { flick: scroll }

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
                id: photoRow
                width: parent.width
                spacing: 16

                Rectangle {
                    id: avatar
                    width: 96
                    height: 96
                    radius: width / 2
                    color: Theme.surfaceHi
                    border.width: 1
                    border.color: Theme.border
                    clip: true
                    Accessible.role: Accessible.Graphic
                    Accessible.name: root.photoShown !== "" ? "Photo du joueur" : "Pas de photo"

                    Image {
                        id: avatarImage
                        objectName: "playerPhoto"
                        anchors.fill: parent
                        visible: root.photoShown !== ""
                        source: root.photoShown
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: false
                        smooth: true
                    }
                    // Masque circulaire : « clip » ignore le rayon d'un Rectangle, les coins sont recouverts par le fond.
                    Canvas {
                        anchors.fill: parent
                        visible: root.photoShown !== ""
                        onPaint: {
                            var c = getContext("2d");
                            c.reset();
                            c.fillStyle = Theme.bg;
                            c.beginPath();
                            c.rect(0, 0, width, height);
                            c.arc(width / 2, height / 2, width / 2, 0, Math.PI * 2, true);
                            c.fill();
                        }
                        Component.onCompleted: requestPaint()
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: root.photoShown === ""
                        text: root.initials !== "" ? root.initials : "?"
                        font.pixelSize: 32
                        font.weight: Font.DemiBold
                        color: Theme.textMuted
                    }
                }

                Column {
                    anchors.verticalCenter: avatar.verticalCenter
                    width: parent.width - avatar.width - parent.spacing
                    spacing: 8
                    AppButton {
                        objectName: "photoChoose"
                        width: parent.width
                        text: root.photoShown !== "" ? "Changer la photo" : "Ajouter une photo"
                        iconName: "plus"
                        onClicked: photoDialog.open()
                    }
                    AppButton {
                        objectName: "photoRemove"
                        width: parent.width
                        visible: root.photoShown !== ""
                        kind: "ghost"
                        text: "Retirer la photo"
                        iconName: "trash"
                        onClicked: root.clearPhoto()
                    }
                }
            }
            Row {
                width: parent.width
                spacing: 12
                FormField { id: firstName; width: (parent.width - 12) / 2; label: "Prénom"; required: true; maxLength: 60 }
                FormField { id: lastName; width: (parent.width - 12) / 2; label: "Nom"; maxLength: 60 }
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
            FormField {
                id: licence
                width: parent.width
                label: "N° de licence"
                placeholder: "Facultatif"
                maxLength: 20
                inputHints: Qt.ImhNoPredictiveText | Qt.ImhUppercaseOnly
            }
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
