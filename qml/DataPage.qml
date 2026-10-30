import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

// Page « Données » : envoyer toutes les données dans un fichier .footcoach (à transmettre par WhatsApp, e-mail…) et
// les recevoir sur un autre appareil (téléphone ↔ tablette). L'import REMPLACE les données de l'appareil.
Item {
    id: page

    property string pendingFile: ""
    signal imported()

    function when(iso) {
        if (!iso || iso.length < 16)
            return "date inconnue";
        return iso.substring(8, 10) + "/" + iso.substring(5, 7) + "/" + iso.substring(0, 4) + " à " + iso.substring(11, 16);
    }

    function describe(c) {
        return c.teams + (c.teams > 1 ? " équipes" : " équipe") + ", " + c.players + (c.players > 1 ? " joueurs" : " joueur")
               + ", " + c.sessions + (c.sessions > 1 ? " séances" : " séance") + ", "
               + c.exercises + (c.exercises > 1 ? " exercices" : " exercice") + ", "
               + c.boards + (c.boards > 1 ? " schémas" : " schéma");
    }

    FileDialog {
        id: saveDialog
        title: "Enregistrer le fichier FootCoach"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Fichier FootCoach (*.footcoach)", "Tous les fichiers (*)"]
        defaultSuffix: "footcoach"
        onAccepted: {
            var r = backend.exportDataFile(selectedFile.toString());
            toast.show(r.ok ? "Fichier créé (" + Math.max(1, Math.round(r.bytes / 1024)) + " Ko). Envoyez-le à votre autre appareil."
                            : r.error);
        }
    }

    FileDialog {
        id: openDialog
        title: "Choisir un fichier FootCoach"
        fileMode: FileDialog.OpenFile
        // « Tous les fichiers » d'abord : un fichier reçu par WhatsApp peut perdre son extension.
        nameFilters: ["Tous les fichiers (*)", "Fichier FootCoach (*.footcoach)"]
        onAccepted: {
            var info = backend.inspectDataFile(selectedFile.toString());
            if (!info.ok) {
                toast.show(info.error);
                return;
            }
            page.pendingFile = selectedFile.toString();
            confirm.confirm("Remplacer les données de cet appareil ?",
                            "Fichier exporté le " + page.when(info.exportedAt) + " (FootCoach " + info.appVersion + ") :\n"
                            + page.describe(info.counts) + ".\n\nCet appareil contient actuellement :\n"
                            + page.describe(info.current) + ".\n\n"
                            + "Tout sera remplacé par le contenu du fichier. Une copie de vos données actuelles est gardée "
                            + "automatiquement.", "Remplacer", true);
        }
    }

    PromptDialog {
        id: confirm
        onAccepted: {
            var r = backend.importDataFile(page.pendingFile);
            if (r.ok) {
                AppState.teamId = -1;
                toast.show("Données importées");
                page.imported();
            } else {
                toast.show(r.error);
            }
        }
    }

    PageHeader {
        id: header
        width: parent.width
        title: "Données"
        subtitle: "Téléphone ↔ tablette"
    }

    Flickable {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        contentWidth: width
        contentHeight: body.height + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        Column {
            id: body
            x: Theme.gutter
            y: 12
            width: Math.min(parent.width - 2 * Theme.gutter, 560)
            spacing: 14

            Card {
                width: parent.width
                Column {
                    width: parent.width
                    spacing: 10
                    Icon { name: "copy"; color: Theme.accent; size: 28 }
                    Text {
                        width: parent.width
                        text: "Envoyer mes données"
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 14
                        color: Theme.textMuted
                        text: "Crée un fichier avec toutes vos données (équipes, joueurs, séances, plans, exercices, "
                              + "présences, schémas). Envoyez-le ensuite à votre autre appareil par WhatsApp, e-mail ou "
                              + "Drive. Il contient des noms et dates de naissance : ne le partagez qu'avec vous-même."
                    }
                    AppButton {
                        kind: "primary"
                        iconName: "check"
                        text: "Créer le fichier"
                        onClicked: {
                            saveDialog.currentFile = "file:///" + backend.suggestedExportName();
                            saveDialog.open();
                        }
                    }
                }
            }

            Card {
                width: parent.width
                Column {
                    width: parent.width
                    spacing: 10
                    Icon { name: "undo"; color: Theme.accent; size: 28 }
                    Text {
                        width: parent.width
                        text: "Recevoir des données"
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: 14
                        color: Theme.textMuted
                        text: "Enregistrez d'abord le fichier reçu dans le téléphone ou la tablette (par exemple depuis "
                              + "WhatsApp), puis choisissez-le ici. Les données de cet appareil sont remplacées par "
                              + "celles du fichier ; avant cela, vous voyez ce qu'il contient et la différence avec "
                              + "l'existant."
                    }
                    AppButton {
                        kind: "outline"
                        iconName: "plus"
                        text: "Choisir un fichier"
                        onClicked: openDialog.open()
                    }
                }
            }
        }
    }

    Toast { id: toast }
}
