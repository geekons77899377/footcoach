import QtQuick
import QtQuick.Controls
import "SupportLink.js" as Link

// Formulaire d'assistance : ouvre le navigateur sur un ticket GitHub « SUP - … » pré-rempli (ou un e-mail). L'application
// n'envoie rien elle-même et ne contient aucun jeton : l'utilisateur relit et valide dans GitHub.
Item {
    id: root

    property string category: "bug"
    property bool showBack: true
    property string errorText: ""
    property bool dryRun: false             // tests : ne pas ouvrir le navigateur
    property string lastUrl: ""             // dernier lien produit
    readonly property string version: Qt.application.version
    readonly property string os: Qt.platform.os

    signal closeRequested()
    signal opened(string kind)              // "github" ou "mail"

    function openNew(cat) {
        category = cat;
        errorText = "";
        titleField.text = "";
        descField.text = "";
        stepsField.text = "";
        envSwitch.checked = true;
        scroll.contentY = 0;
    }

    function payload() {
        return { category: category, title: titleField.text, description: descField.text, steps: stepsField.text,
                 includeEnv: envSwitch.checked, version: version, os: os };
    }

    function validate() {
        if (titleField.text.trim() === "")
            return "Indiquez un titre.";
        if (descField.text.trim() === "")
            return "Décrivez le problème, l'idée ou la question.";
        return "";
    }

    function submit(kind) {
        errorText = validate();
        if (errorText !== "")
            return;
        var p = payload();
        lastUrl = kind === "mail" ? Link.mailUrl(p) : Link.issueUrl(p);
        if (!dryRun && !Qt.openUrlExternally(lastUrl)) {
            errorText = "Impossible d'ouvrir le navigateur ou l'application de messagerie sur cet appareil.";
            return;
        }
        opened(kind);
    }

    PageHeader {
        id: header
        width: parent.width
        leftInset: root.showBack ? 44 : 0
        title: Link.category(root.category).title
        subtitle: "Ticket d'assistance GitHub"
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
        contentHeight: form.height + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        Column {
            id: form
            x: Theme.gutter + 4
            y: 8
            width: Math.min(scroll.width - 2 * (Theme.gutter + 4), 640)
            spacing: 16

            Text {
                visible: root.errorText !== ""
                objectName: "supportError"
                width: parent.width
                text: root.errorText
                wrapMode: Text.Wrap
                font.pixelSize: 14
                font.weight: Font.DemiBold
                color: Theme.danger
            }
            ChipRow {
                id: cats
                objectName: "supportCategory"
                width: parent.width
                label: "Type de ticket"
                allowClear: false
                value: root.category
                options: Link.CATEGORIES
                onPicked: (id) => root.category = id
            }
            FormField { id: titleField; width: parent.width; label: "Titre"; required: true; maxLength: 100
                        placeholder: "Résumé en une ligne" }
            FormField { id: descField; width: parent.width; label: root.category === "bug" ? "Description du problème"
                                                                   : root.category === "enhancement" ? "Votre idée" : "Votre question"
                        required: true; multiline: true; maxLength: 1500 }
            FormField { id: stepsField; visible: root.category === "bug"; width: parent.width
                        label: "Étapes pour reproduire (facultatif)"; multiline: true; maxLength: 800 }

            Row {
                width: parent.width
                spacing: 12
                Switch {
                    id: envSwitch
                    objectName: "supportEnv"
                    checked: true
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    width: parent.width - envSwitch.width - 12
                    anchors.verticalCenter: parent.verticalCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: 13
                    color: Theme.textMuted
                    text: "Joindre la version de FootCoach (" + root.version + ") et le système (" + root.os + ")"
                }
            }

            Card {
                width: parent.width
                height: warn.height + 2 * padding
                padding: 14
                radius: Theme.radiusLg
                border.color: Theme.warn
                Text {
                    id: warn
                    width: parent.width
                    wrapMode: Text.Wrap
                    font.pixelSize: 13
                    color: Theme.text
                    text: "Le ticket sera visible publiquement sur GitHub. N'y mettez aucune donnée personnelle (nom ou numéro "
                          + "de joueur, téléphone, e-mail). Vous relirez le ticket dans votre navigateur avant de l'envoyer ; "
                          + "un compte GitHub est nécessaire."
                }
            }

            AppButton {
                objectName: "supportGithub"
                width: parent.width
                kind: "primary"
                text: "Créer le ticket sur GitHub"
                iconName: "check"
                onClicked: root.submit("github")
            }
            AppButton {
                objectName: "supportMail"
                width: parent.width
                text: "Envoyer par e-mail à la place"
                iconName: "mail"
                onClicked: root.submit("mail")
            }
        }
    }
}
