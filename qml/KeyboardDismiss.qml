import QtQuick

// Un toucher en dehors du champ de saisie qui a le focus le lui retire, et le clavier se ferme.
// Qt Quick ne le fait pas de lui-même. À placer dans un item (la fenêtre, le fond d'une boîte de dialogue).
// Prise passive : boutons, listes et champs situés dessous reçoivent le toucher normalement.
TapHandler {
    id: root

    onTapped: (point) => {
        const win = root.parent.Window.window;
        const focused = win ? win.activeFocusItem : null;
        if (!focused || !("inputMethodHints" in focused))
            return;                                         // le focus n'est pas dans un champ de saisie
        if (focused.contains(focused.mapFromItem(null, point.scenePosition)))
            return;                                         // toucher dans le champ lui-même
        win.contentItem.forceActiveFocus();
        Qt.inputMethod.hide();
    }
}
