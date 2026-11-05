import QtQuick
import QtQuick.Controls

// Échec d'enregistrement d'un terrain au moment de le quitter : on reste sur l'écran, l'erreur est affichée et
// l'utilisateur choisit de réessayer ou de quitter sans enregistrer (cette seconde issue est confirmée à part).
Popup {
    id: root
    objectName: "saveFailedDialog"

    property string message: ""
    signal retry()
    signal leaveWithoutSaving()
    signal stay()

    // Une fermeture sans choix (Échap) vaut « Rester sur le terrain » : la sortie en attente est abandonnée.
    property bool decided: false
    onAboutToShow: decided = false
    onClosed: if (!decided) root.stay()

    parent: Overlay.overlay
    modal: true
    dim: true
    padding: 20
    width: Math.min(parent ? parent.width - 32 : 360, 420)
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? Math.min(parent.height * 0.18, Math.max(12, parent.height - height - 12)) : 0
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle { color: "#99000000" }

    background: Rectangle {
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    contentItem: Column {
        spacing: 14

        Text {
            width: parent.width
            text: "Enregistrement impossible"
            font.pixelSize: 20
            font.weight: Font.Bold
            color: Theme.text
            wrapMode: Text.Wrap
        }
        Text {
            objectName: "saveFailedMessage"
            width: parent.width
            text: root.message + "\nVos dernières modifications de ce terrain ne sont pas enregistrées."
            font.pixelSize: 15
            color: Theme.textMuted
            wrapMode: Text.Wrap
        }
        Column {
            width: parent.width
            spacing: 10
            AppButton {
                objectName: "saveRetryButton"
                width: parent.width
                kind: "primary"
                text: "Réessayer"
                onClicked: {
                    root.decided = true;
                    root.close();
                    root.retry();
                }
            }
            AppButton {
                objectName: "saveDiscardButton"
                width: parent.width
                kind: "danger"
                text: "Quitter sans enregistrer"
                onClicked: {
                    root.decided = true;
                    root.close();
                    root.leaveWithoutSaving();
                }
            }
            AppButton {
                objectName: "saveStayButton"
                width: parent.width
                kind: "ghost"
                text: "Rester sur le terrain"
                onClicked: {
                    root.decided = true;
                    root.close();
                    root.stay();
                }
            }
        }
    }
}
