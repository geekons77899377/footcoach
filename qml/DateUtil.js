.pragma library

// Outils de dates et d'heures pour les formulaires : saisie libre -> format de la base, et inversement.

var MONTHS = ["janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "octobre",
              "novembre", "décembre"];
var DAYS_SHORT = ["lun", "mar", "mer", "jeu", "ven", "sam", "dim"];
var DAYS_LONG = ["lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi", "dimanche"];

function pad(n) { return (n < 10 ? "0" : "") + n; }

function iso(d) { return d.getFullYear() + "-" + pad(d.getMonth() + 1) + "-" + pad(d.getDate()); }

function fromIso(s) {                       // "AAAA-MM-JJ" -> Date (midi, pour éviter les décalages d'heure d'été)
    return new Date(parseInt(s.substring(0, 4), 10), parseInt(s.substring(5, 7), 10) - 1, parseInt(s.substring(8, 10), 10), 12);
}

function todayIso() { return iso(new Date()); }

function addDays(s, n) { var d = fromIso(s); d.setDate(d.getDate() + n); return iso(d); }

function addMonths(s, n) {
    var d = fromIso(s);
    var day = d.getDate();
    d.setDate(1);
    d.setMonth(d.getMonth() + n);
    var last = new Date(d.getFullYear(), d.getMonth() + 1, 0).getDate();
    d.setDate(Math.min(day, last));
    return iso(d);
}

// Jour de la semaine, 1 = lundi … 7 = dimanche.
function weekday(s) { var w = fromIso(s).getDay(); return w === 0 ? 7 : w; }

// "AAAA-MM-JJ" -> "JJ/MM/AAAA" ("" si le format n'est pas bon).
function display(s) {
    return s && s.length === 10 ? s.substring(8, 10) + "/" + s.substring(5, 7) + "/" + s.substring(0, 4) : "";
}

// "mardi 3 novembre".
function longLabel(s) {
    var d = fromIso(s);
    return DAYS_LONG[weekday(s) - 1] + " " + d.getDate() + " " + MONTHS[d.getMonth()];
}

// Saisie libre : "02062016", "02/06/2016", "2.6.2016", "2-6-2016" -> "AAAA-MM-JJ". "" si vide ; le texte tel quel s'il est
// illisible ou impossible (31/02), pour que l'appelant (ou le backend) le refuse.
function parse(text) {
    var t = text.trim();
    if (t === "")
        return "";
    var d, mo, y, m;
    if (/^\d{8}$/.test(t)) {
        d = t.substring(0, 2); mo = t.substring(2, 4); y = t.substring(4, 8);
    } else if ((m = /^(\d{1,2})[\s\/.\-](\d{1,2})[\s\/.\-](\d{4})$/.exec(t))) {
        d = m[1]; mo = m[2]; y = m[3];
    } else {
        return t;
    }
    var dn = parseInt(d, 10), mn = parseInt(mo, 10), yn = parseInt(y, 10);
    var check = new Date(yn, mn - 1, dn);
    if (check.getFullYear() !== yn || check.getMonth() !== mn - 1 || check.getDate() !== dn)
        return t;
    return y + "-" + pad(mn) + "-" + pad(dn);
}

function isIso(s) { return /^\d{4}-\d{2}-\d{2}$/.test(s); }

// Heure : "1830", "18:30", "18h30", "18h", "8" -> "HH:mm" ; texte tel quel si illisible.
function parseTime(text) {
    var t = text.trim().toLowerCase();
    if (t === "")
        return "";
    var m = /^(\d{1,2})\s*[:h.]?\s*(\d{2})?$/.exec(t);
    if (!m && /^\d{3,4}$/.test(t))
        m = [t, t.length === 3 ? t.substring(0, 1) : t.substring(0, 2), t.substring(t.length - 2)];
    if (!m)
        return t;
    var h = parseInt(m[1], 10), mi = m[2] === undefined ? 0 : parseInt(m[2], 10);
    return h > 23 || mi > 59 ? t : pad(h) + ":" + pad(mi);
}

function isTime(s) { return /^\d{2}:\d{2}$/.test(s); }
