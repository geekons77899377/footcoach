.pragma library

// Statuts d'appel (présent, absent, excusé, blessé) et de séance (prévue, réalisée, annulée) : libellés et couleurs.
var ATTENDANCE = {
    present: { label: "Présent", letter: "P", color: "#2ecc71" },
    absent: { label: "Absent", letter: "A", color: "#f26456" },
    excused: { label: "Excusé", letter: "E", color: "#f59e0b" },
    injured: { label: "Blessé", letter: "B", color: "#a78bfa" }
};
var ATTENDANCE_ORDER = ["present", "absent", "excused", "injured"];

var SESSION = {
    planned: { label: "Prévue", color: "#60a5fa" },
    done: { label: "Réalisée", color: "#2ecc71" },
    cancelled: { label: "Annulée", color: "#f26456" }
};

// Matchs : prévu, joué, annulé. La couleur d'un match dans le calendrier (MATCH_COLOR) diffère de celle des séances.
var MATCH = {
    planned: { label: "Prévu", color: "#60a5fa" },
    played: { label: "Joué", color: "#2ecc71" },
    cancelled: { label: "Annulé", color: "#f26456" }
};
var MATCH_COLOR = "#f59e0b";

function match(code) { return MATCH[code] || { label: code, color: "#8190a5" }; }
function attendance(code) { return ATTENDANCE[code] || { label: "", letter: "·", color: "#8190a5" }; }
function session(code) { return SESSION[code] || { label: code, color: "#8190a5" }; }

// Couleur d'un taux de présence (0..1).
function rateColor(rate) {
    if (rate < 0) return "#8190a5";
    if (rate >= 0.8) return "#2ecc71";
    if (rate >= 0.6) return "#f59e0b";
    return "#f26456";
}

// Catégories d'exercice : couleur d'accompagnement.
var CATEGORY = {
    warmup: "#f59e0b", technical: "#60a5fa", tactical: "#a78bfa",
    physical: "#f26456", game: "#2ecc71", cooldown: "#38bdf8"
};
function categoryColor(code) { return CATEGORY[code] || "#8190a5"; }
