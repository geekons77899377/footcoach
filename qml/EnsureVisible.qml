import QtQuick

// Garde visible le champ qui a le focus dans une liste défilante. Sur Android le clavier réduit la fenêtre ; sans
// cela, un champ placé en bas du formulaire (notes, description, e-mail) reste caché derrière le clavier, car une
// liste défilante ne se déplace pas toute seule vers le champ touché.
// Usage : EnsureVisible { flick: scroll } à côté de la Flickable. Le champ est cherché parmi les enfants de la
// colonne du formulaire (le bloc entier, libellé compris, est amené à l'écran).
Item {
    id: root

    required property Flickable flick
    property int margin: 16
    visible: false

    function ensure() {
        var win = Window.window;
        var it = win ? win.activeFocusItem : null;
        if (!it || !flick || !flick.visible)
            return;
        var content = flick.contentItem;
        var p = it, inside = false;
        while (p) {
            if (p === content) { inside = true; break; }
            p = p.parent;
        }
        if (!inside || it === content)
            return;
        // Bloc à montrer : l'ancêtre dont le grand-parent est le contenu (enfant direct de la colonne du formulaire).
        var block = it;
        while (block.parent && block.parent.parent && block.parent.parent !== content)
            block = block.parent;
        var top = block.mapToItem(content, 0, 0).y;
        var bottom = top + block.height;
        // originY : 0 pour une Flickable, négatif pour une ListView dont l'en-tête (header) est au-dessus du contenu.
        var minY = flick.originY;
        var maxY = Math.max(minY, minY + flick.contentHeight - flick.height);
        var y = flick.contentY;
        if (bottom + margin > y + flick.height)
            y = bottom + margin - flick.height;
        if (top - margin < y)
            y = top - margin;
        y = Math.max(minY, Math.min(maxY, y));
        if (Math.abs(y - flick.contentY) > 0.5)
            flick.contentY = y;
    }

    // Court délai : le temps que la fenêtre soit réduite et la mise en page refaite.
    Timer { id: later; interval: 40; onTriggered: root.ensure() }
    Connections {
        target: root.Window.window
        function onActiveFocusItemChanged() { later.restart(); }
    }
    Connections {
        target: root.flick
        function onHeightChanged() { later.restart(); }
        function onContentHeightChanged() { later.restart(); }
    }
}
