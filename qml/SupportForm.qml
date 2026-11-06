import QtQuick
import QtQuick.Controls
import "SupportLink.js" as Link

// Formulaire d'assistance : crée un ticket GitHub « SUP - … » directement depuis l'application (IssueReporter, sans
// navigateur ni compte GitHub), ou ouvre un e-mail pré-rempli. Rien n'est envoyé avant le toucher sur « Envoyer ».
Item {
    id: root

    property string category: "bug"
    property bool showBack: true
    property string errorText: ""
    property bool dryRun: false             // tests : ne pas ouvrir la messagerie
    property string lastUrl: ""             // dernier lien e-mail produit
    property string sentUrl: ""             // lien du ticket créé
    property bool sent: false               // dernier envoi réussi : écran de confirmation
    readonly property var reporter: typeof issueReporter !== "undefined" ? issueReporter : null
    readonly property bool canSend: reporter !== null && reporter.available
    readonly property bool busy: reporter !== null && reporter.busy
    readonly property string version: Qt.application.version
    readonly property string os: Qt.platform.os

    signal closeRequested()
    signal opened(string kind)              // "mail" (messagerie ouverte)

    function openNew(cat) {
        category = cat;
        errorText = "";
        sent = false;
        sentUrl = "";
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
        if (kind === "mail") {
            lastUrl = Link.mailUrl(p);
            if (!dryRun && !Qt.openUrlExternally(lastUrl)) {
                errorText = "Impossible d'ouvrir l'application de messagerie sur cet appareil.";
                return;
            }
            opened("mail");
            return;
        }
        if (!canSend) {
            errorText = "L'envoi direct n'est pas disponible dans cette version : utilisez l'e-mail.";
            return;
        }
        if (busy)
            return;
        reporter.send(Link.issueTitle(p.title), Link.issueBody(p), [Link.category(p.category).label]);
    }

    Connections {
        target: root.reporter
        ignoreUnknownSignals: true
        function onFinished(ok, message, url) {
            if (!root.visible)
                return;
            if (ok) {
                root.sentUrl = url;
                root.sent = true;
                root.errorText = "";
            } else {
                root.errorText = message;
            }
        }
    }

    PageHeader {
        id: header
        width: parent.width
        leftInset: root.showBack ? 44 : 0
        title: Link.category(root.category).title
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
            id: done
            objectName: "supportDone"
            visible: root.sent
            x: Theme.gutter + 4
            y: 8
            width: form.width
            spacing: 16
            Text {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: 16
                font.weight: Font.DemiBold
                color: Theme.text
                text: "Merci, votre message a bien été envoyé."
            }
            AppButton {
                objectName: "supportDoneClose"
                width: parent.width
                kind: "primary"
                text: "Fermer"
                onClicked: root.opened("sent")
            }
        }

        Column {
            id: form
            visible: !root.sent
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
                label: "Type"
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
                    text: "Votre message sera visible publiquement. N'y mettez aucune donnée personnelle (nom ou numéro de "
                          + "joueur, téléphone, e-mail)."
                }
            }

            AppButton {
                objectName: "supportSend"
                visible: root.canSend
                enabled: !root.busy
                width: parent.width
                kind: "primary"
                text: root.busy ? "Envoi en cours…" : "Envoyer le signalement"
                iconName: "check"
                onClicked: root.submit("send")
            }
            AppButton {
                objectName: "supportMail"
                width: parent.width
                kind: root.canSend ? "outline" : "primary"
                text: root.canSend ? "Envoyer par e-mail à la place" : "Envoyer par e-mail"
                iconName: "mail"
                onClicked: root.submit("mail")
            }
        }
    }
}
