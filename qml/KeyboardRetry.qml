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
//
// DIAGNOSTIC (14/11/2026, temporaire) : chaque étape est écrite dans le journal, « [toucher] clavier | champ : événement
// [état] », dans « Sortie de l'application » de Qt Creator ou `adb logcat | grep toucher`. L'état donne le focus du champ,
// si la fenêtre est active, si Qt croit le clavier visible et sa hauteur. À retirer une fois la cause trouvée.
TapHandler {
    id: root
    acceptedDevices: PointerDevice.TouchScreen | PointerDevice.Stylus

    property string tag: ""                      // nom du champ dans le journal

    function log(what) {
        const f = root.parent;
        const win = f && f.Window ? f.Window.window : null;
        AppState.trace("clavier", (tag !== "" ? tag : "champ") + " : " + what
                       + " [focus=" + (f ? f.activeFocus : "?")
                       + ", fenêtre active=" + (win ? win.active : "?")
                       + ", clavier visible=" + Qt.inputMethod.visible
                       + ", hauteur=" + Math.round(Qt.inputMethod.keyboardRectangle.height) + "]");
    }

    onPressedChanged: if (pressed) log("doigt posé")
    onTapped: { log("doigt levé"); retry.restart(); after.restart() }

    property Connections focusWatch: Connections {
        target: root.parent
        function onActiveFocusChanged() { root.log(root.parent.activeFocus ? "focus obtenu" : "focus perdu") }
    }

    property Timer retry: Timer {
        interval: 150
        onTriggered: {
            const field = root.parent;
            const ask = field && field.activeFocus && !field.readOnly;
            root.log(ask ? "+150 ms : Qt.inputMethod.show() demandé" : "+150 ms : rien à demander");
            if (ask)
                Qt.inputMethod.show();
        }
    }
    property Timer after: Timer {
        interval: 700
        onTriggered: root.log("+700 ms : état final")
    }
}
