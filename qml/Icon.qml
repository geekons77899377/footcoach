import QtQuick

// Icône vectorielle (SVG construit à la volée, donc de la couleur voulue et nette à toute taille).
Item {
    id: root

    property string name: ""
    property color color: "#94a3b8"
    property real size: 24
    property real strokeWidth: 2

    implicitWidth: size
    implicitHeight: size

    // Corps des icônes sur une grille de 24 × 24. "C" sera remplacé par la couleur.
    readonly property var glyphs: ({
        "home": '<path d="M3 11.5 12 4l9 7.5"/><path d="M5 10v10h5v-6h4v6h5V10"/>',
        "users": '<circle cx="9" cy="8" r="3.5"/><path d="M2.5 20c0-3.6 2.9-6 6.5-6s6.5 2.4 6.5 6"/><circle cx="17.5" cy="9" r="2.5"/><path d="M17 14.2c2.6.2 4.5 2 4.5 5.3"/>',
        "calendar": '<rect x="3.5" y="5" width="17" height="15.5" rx="3"/><path d="M3.5 10h17M8 3v4M16 3v4"/>',
        "check": '<rect x="5" y="4" width="14" height="17" rx="3"/><path d="M9 4.5h6M9 13l2.2 2.2L15.5 11"/>',
        "menu": '<path d="M4 7h16M4 12h16M4 17h16"/>',
        "plus": '<path d="M12 5v14M5 12h14"/>',
        "minus": '<path d="M5 12h14"/>',
        "search": '<circle cx="11" cy="11" r="6.5"/><path d="M16 16l4.5 4.5"/>',
        "back": '<path d="M15 5l-7 7 7 7"/>',
        "chevron": '<path d="M9 5l7 7-7 7"/>',
        "undo": '<path d="M9 7 4 12l5 5"/><path d="M4 12h10a5 5 0 0 1 0 10h-3"/>',
        "redo": '<path d="M15 7l5 5-5 5"/><path d="M20 12H10a5 5 0 0 0 0 10h3"/>',
        "trash": '<path d="M4 7h16M10 7V4h4v3M6 7l1 13h10l1-13M10 11v6M14 11v6"/>',
        "more": '<circle cx="5" cy="12" r="1.3" fill="C"/><circle cx="12" cy="12" r="1.3" fill="C"/><circle cx="19" cy="12" r="1.3" fill="C"/>',
        "zoomin": '<circle cx="11" cy="11" r="6.5"/><path d="M16 16l4.5 4.5M11 8v6M8 11h6"/>',
        "zoomout": '<circle cx="11" cy="11" r="6.5"/><path d="M16 16l4.5 4.5M8 11h6"/>',
        "align": '<path d="M4 4v16M8 7h12M8 12h8M8 17h10"/>',
        "grouprotate": '<rect x="4" y="13" width="7" height="7" rx="1"/><rect x="13" y="4" width="7" height="7" rx="1"/><path d="M13 17a4 4 0 0 0 4-4"/><path d="M17 17h-4v-4"/>',
        "alignleft": '<path d="M4 4v16M8 7h12M8 12h8M8 17h10"/>',
        "alignhcenter": '<path d="M12 3v18M5 7h14M8 12h8M6 17h12"/>',
        "alignright": '<path d="M20 4v16M4 7h12M8 12h8M6 17h10"/>',
        "aligntop": '<path d="M4 4h16M7 8v12M12 8v8M17 8v10"/>',
        "alignvcenter": '<path d="M3 12h18M7 5v14M12 8v8M17 6v12"/>',
        "alignbottom": '<path d="M4 20h16M7 4v12M12 8v8M17 6v10"/>',
        "alignhdist": '<path d="M4 4v16M20 4v16M12 8v8"/>',
        "alignvdist": '<path d="M4 4h16M4 20h16M8 12h8"/>',
        "expand": '<path d="M14 4h6v6M10 20H4v-6M20 4l-7 7M4 20l7-7"/>',
        "collapse": '<path d="M20 10h-6V4M4 14h6v6M14 10l7-7M3 21l7-7"/>',
        "fit": '<path d="M4 9V4h5M20 9V4h-5M4 15v5h5M20 15v5h-5"/>',
        "rotatecw": '<path d="M20 12a8 8 0 1 1-2.5-5.8"/><path d="M20 4v5h-5"/>',
        "rotateccw": '<path d="M4 12a8 8 0 1 0 2.5-5.8"/><path d="M4 4v5h5"/>',
        "palette": '<path d="M12 3a9 9 0 1 0 0 18c1.5 0 2-1 1.5-2.2-.6-1.3.3-2.8 1.8-2.8H18a3 3 0 0 0 3-3C21 7 17 3 12 3z"/><circle cx="7.5" cy="11" r="1"/><circle cx="10" cy="7" r="1"/><circle cx="15" cy="7.5" r="1"/>',
        "edit": '<path d="M4 20l1-4L16.5 4.5a2.1 2.1 0 0 1 3 3L8 19z"/>',
        "flip": '<path d="M7 4 3 8l4 4M3 8h14M17 20l4-4-4-4M21 16H7"/>',
        "board": '<rect x="3" y="5" width="18" height="14" rx="3"/><path d="M12 5v14"/><circle cx="12" cy="12" r="2.5"/>',
        "up": '<path d="M5 15l7-7 7 7"/>',
        "down": '<path d="M5 9l7 7 7-7"/>',
        "archive": '<rect x="3.5" y="4" width="17" height="5" rx="1.5"/><path d="M5 9v9.5A1.5 1.5 0 0 0 6.5 20h11a1.5 1.5 0 0 0 1.5-1.5V9M10 13h4"/>',
        "dumbbell": '<path d="M6 8v8M3 10v4M18 8v8M21 10v4M6 12h12"/>',
        "clipboard": '<rect x="5" y="4" width="14" height="17" rx="3"/><path d="M9 4.5h6M9 10h6M9 14h4"/>',
        "copy": '<rect x="8" y="8" width="12" height="12" rx="3"/><path d="M16 8V6a2 2 0 0 0-2-2H6a2 2 0 0 0-2 2v8a2 2 0 0 0 2 2h2"/>',
        "close": '<path d="M6 6l12 12M18 6 6 18"/>',
        "pitch": '<rect x="3" y="5" width="18" height="14" rx="2"/><path d="M12 5v14M3 10h3v4H3M21 10h-3v4h3"/>',
        // Outils du terrain : pictogrammes en couleur (indépendants de la couleur demandée).
        "t-select": '<path d="M6 3l13 7-6 2.2L10.5 19z" fill="C"/>',
        "clock": '<circle cx="12" cy="12" r="8.5"/><path d="M12 7.5V12l3 2"/>',
        "repeat": '<path d="M4 11V9a3 3 0 0 1 3-3h11M15 3l3 3-3 3M20 13v2a3 3 0 0 1-3 3H6M9 21l-3-3 3-3"/>',
        "t-multi": '<rect x="3.5" y="3.5" width="11" height="11" rx="2" stroke-dasharray="2.6 2.2"/><rect x="9.5" y="9.5" width="11" height="11" rx="2"/>',
        "t-player": '<circle cx="12" cy="12" r="8" fill="#1565c0" stroke="#ffffff" stroke-width="2"/>',
        "t-opponent": '<circle cx="12" cy="12" r="8" fill="#c62828" stroke="#ffffff" stroke-width="2"/>',
        "t-ball": '<circle cx="12" cy="12" r="6.5" fill="#ffffff" stroke="#111827" stroke-width="2"/>',
        "t-cone": '<path d="M12 4 20 19H4z" fill="#ff9800" stroke="#7a4a00" stroke-width="1.6"/>',
        "t-goal": '<rect x="9" y="3.5" width="6" height="17" rx="1" fill="#ffffff" fill-opacity="0.35" stroke="#ffffff" stroke-width="2"/>',
        "t-text": '<path d="M5 6V4.5h14V6M12 4.5v15M9 19.5h6"/>',
        "t-run": '<path d="M4 20 15 9" stroke-dasharray="2.6 2.6"/><path d="M20 4 13.6 6.2 17.8 10.4z" fill="C"/>',
        "t-pass": '<path d="M4 20 15 9"/><path d="M20 4 13.6 6.2 17.8 10.4z" fill="C"/>',
        "t-dribble": '<path d="M4 20c1-3 2.5-3.5 3.5-1.5s2.6 1.2 3.4-1.4 2.2-2.4 3.1-4.2"/><path d="M20 4 13.6 6.2 17.8 10.4z" fill="C"/>',
        "t-cross": '<path d="M4 20C5.5 11 10 6 16 6.4"/><path d="M21 6.8 15 3.6l-.4 6.4z" fill="C"/>'
    })

    function rgb(c) {
        return "rgb(" + Math.round(c.r * 255) + "," + Math.round(c.g * 255) + "," + Math.round(c.b * 255) + ")";
    }

    readonly property string svg: {
        var body = glyphs[name] || "";
        var col = rgb(color);
        body = body.replace(/fill="C"/g, 'fill="' + col + '"');
        return '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="' + col
               + '" stroke-opacity="' + color.a + '" stroke-width="' + strokeWidth
               + '" stroke-linecap="round" stroke-linejoin="round">' + body + '</svg>';
    }

    Image {
        anchors.fill: parent
        sourceSize: Qt.size(root.size * 3, root.size * 3)
        fillMode: Image.PreserveAspectFit
        smooth: true
        source: root.name !== "" ? "data:image/svg+xml;utf8," + encodeURIComponent(root.svg) : ""
    }
}
