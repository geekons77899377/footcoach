import QtQuick

// Clavier virtuel Android : après un toucher sur un champ, redemande l'affichage du clavier un instant plus tard.
// À placer DANS le champ de saisie (TextField, TextArea).
//
// Constat sur appareil : le 1er toucher donne le focus au champ (bordure active) sans afficher le clavier, le 2e
// l'affiche. Dans le code de Qt 6.9 (qquicktextedit.cpp), l'affichage est demandé à l'entrée du focus
// (handleFocusEvent) puis, si le champ a déjà le focus, à chaque appui (mousePressEvent) : seul le 2e cas aboutit.
// Hypothèse (non vérifiée sur appareil) : la 1re demande part avant que Android ait rattaché le nouveau champ à la
// saisie. Cette 2e demande, après le relâchement du doigt, est sans effet quand le clavier est déjà affiché.
// Prise passive : le champ reçoit le toucher normalement ; la souris n'ouvre pas de clavier virtuel.
TapHandler {
    id: root
    acceptedDevices: PointerDevice.TouchScreen | PointerDevice.Stylus

    onTapped: retry.restart()

    property Timer retry: Timer {
        interval: 150
        onTriggered: {
            const field = root.parent;
            if (field && field.activeFocus && !field.readOnly)
                Qt.inputMethod.show();
        }
    }
}
