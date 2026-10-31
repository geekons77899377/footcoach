import QtQuick

// À placer dans le `background` d'une boîte de dialogue modale : ferme le clavier quand on touche n'importe où
// dans la fenêtre, y compris sur la zone assombrie autour (qui ne reçoit pas d'événements elle-même).
Item {
    id: root
    required property var popup
    x: -popup.x
    y: -popup.y
    width: popup.parent ? popup.parent.width : 0
    height: popup.parent ? popup.parent.height : 0
    KeyboardDismiss {}
}
