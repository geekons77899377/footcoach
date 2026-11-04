pragma Singleton
import QtQuick

// Charte graphique : Design System FootCoach (Design/DESIGN.md) — bleu nuit #0b1120, vert « pitch » #2ecc71,
// cartes #151e32, bordures #222f47, rayons 24 / 32. Les teintes marquées « dérivé » n'y figurent pas : elles
// sont déduites pour les états (survol, sélection) ou les fonds d'icônes.
QtObject {
    // Fonds et surfaces
    readonly property color bg: "#0b1120"                  // background
    readonly property color bgDeep: "#080d19"              // dérivé : barres de navigation, un cran sous le fond
    readonly property color surface: "#151e32"             // surface : cartes, en-têtes, feuilles
    readonly property color surfaceHi: "#1a243b"           // surface.card : lignes de liste, éléments en relief
    readonly property color pitch: "#0d3822"               // surface.pitch : minuteur de séance, cartes d'exercice
    readonly property color border: "#222f47"              // border
    readonly property color borderSubtle: "#1f293d"        // border.subtle : séparateurs de lignes

    // Accent et états
    readonly property color accent: "#2ecc71"              // primary
    readonly property color accentHover: "#27ae60"         // primary.hover
    readonly property color accentSoft: "#1e3d33"          // dérivé : fond des pastilles vertes
    readonly property color textOnAccent: "#0b1120"        // primary.foreground
    readonly property color warn: "#f39c12"                // warning
    readonly property color danger: "#e74c3c"              // destructive
    readonly property color info: "#3498db"                // info
    readonly property color success: "#2ecc71"             // success
    readonly property color selection: "#facc15"           // dérivé : sélection dans l'éditeur de schémas

    // Texte
    readonly property color text: "#f8fafc"                // foreground
    readonly property color textMuted: "#8a99ad"           // muted.foreground
    readonly property color textDim: "#64748b"             // dérivé : texte désactivé (contraste faible, jamais pour une information)

    // Formes et mesures (grille de 4 px)
    readonly property int radius: 24                       // rounded-24px : cartes, boutons
    readonly property int radiusLg: 32                     // rounded-32px : grands conteneurs, feuilles
    readonly property int radiusSm: 14                     // conservé : puces, petits blocs
    readonly property int radiusInput: 8                   // rounded-lg : champs, badges
    readonly property int touch: 48                        // cible tactile minimale
    readonly property int gutter: 16

    // Typographie : Outfit (titres, chrono) embarquée sous qml/fonts ; texte courant : police du système (Inter
    // n'est pas embarquée, voir CHANGELOG). Repli sur la police du système si le fichier n'a pas pu être chargé.
    readonly property string fontHeading: headingLoader.status === FontLoader.Ready ? headingLoader.name : Qt.application.font.family

    readonly property FontLoader headingSemiBold: FontLoader { source: "fonts/Outfit-SemiBold.ttf" }
    readonly property FontLoader headingExtraBold: FontLoader { source: "fonts/Outfit-ExtraBold.ttf" }
    readonly property FontLoader headingLoader: FontLoader { source: "fonts/Outfit-Bold.ttf" }
}
