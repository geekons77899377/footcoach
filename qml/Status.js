.pragma library

// Statuts d'appel (présent, absent, excusé, blessé) et de séance (prévue, réalisée, annulée) : libellés et couleurs.
var ATTENDANCE = {
    present: { label: "Présent", letter: "P", color: "#2ecc71" },
    absent: { label: "Absent", letter: "A", color: "#ef4444" },
    excused: { label: "Excusé", letter: "E", color: "#f59e0b" },
    injured: { label: "Blessé", letter: "B", color: "#a78bfa" }
};
var ATTENDANCE_ORDER = ["present", "absent", "excused", "injured"];

var SESSION = {
    planned: { label: "Prévue", color: "#60a5fa" },
    done: { label: "Réalisée", color: "#2ecc71" },
    cancelled: { label: "Annulée", color: "#ef4444" }
};

function attendance(code) { return ATTENDANCE[code] || { label: "", letter: "·", color: "#64748b" }; }
function session(code) { return SESSION[code] || { label: code, color: "#64748b" }; }

// Couleur d'un taux de présence (0..1).
function rateColor(rate) {
    if (rate < 0) return "#64748b";
    if (rate >= 0.8) return "#2ecc71";
    if (rate >= 0.6) return "#f59e0b";
    return "#ef4444";
}

// Catégories d'exercice : couleur d'accompagnement.
var CATEGORY = {
    warmup: "#f59e0b", technical: "#60a5fa", tactical: "#a78bfa",
    physical: "#ef4444", game: "#2ecc71", cooldown: "#38bdf8"
};
function categoryColor(code) { return CATEGORY[code] || "#64748b"; }
