pragma Singleton
import QtQuick

// Charte graphique (thème sombre bleu nuit, accent vert). Fond, bordure et vert mesurés sur la maquette ;
// les autres teintes en sont déduites.
QtObject {
    readonly property color bg: "#0f172a"
    readonly property color bgDeep: "#0b1220"
    readonly property color surface: "#1a2433"
    readonly property color surfaceHi: "#243247"
    readonly property color border: "#334155"
    readonly property color accent: "#2ecc71"
    readonly property color accentSoft: "#1e3d33"
    readonly property color textOnAccent: "#06210f"
    readonly property color text: "#f8fafc"
    readonly property color textMuted: "#94a3b8"
    readonly property color textDim: "#64748b"
    readonly property color warn: "#f59e0b"
    readonly property color danger: "#ef4444"
    readonly property color selection: "#facc15"

    readonly property int radius: 22
    readonly property int radiusSm: 14
    readonly property int touch: 48
    readonly property int gutter: 16
}
