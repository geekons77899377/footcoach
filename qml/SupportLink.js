.pragma library

// Assistance : construit le lien qui ouvre le formulaire de nouveau ticket GitHub, déjà rempli, ou un e-mail.
// Rien n'est envoyé par l'application : l'utilisateur relit puis valide dans son navigateur (compte GitHub requis).
// Aucun jeton ni secret n'est embarqué.

var REPO = "geekons77899377/footcoach";
var CONTACT = "footcoach.app@gmail.com";
var PREFIX = "SUP - ";                  // préfixe du titre de tous les tickets d'assistance
var MAX_URL = 6500;                     // au-delà, GitHub ou le navigateur peuvent refuser le lien

var CATEGORIES = [
    { id: "bug", text: "Problème", title: "Signaler un problème", label: "bug" },
    { id: "enhancement", text: "Amélioration", title: "Proposer une amélioration", label: "enhancement" },
    { id: "question", text: "Question", title: "Poser une question", label: "question" }
];

function category(id) {
    for (var i = 0; i < CATEGORIES.length; ++i)
        if (CATEGORIES[i].id === id)
            return CATEGORIES[i];
    return CATEGORIES[0];
}

function clean(s) { return String(s === undefined || s === null ? "" : s).replace(/\r/g, "").trim(); }

// « SUP - <titre> », sur une seule ligne, 120 caractères au plus.
function issueTitle(title) {
    var t = clean(title).replace(/\s*\n\s*/g, " ");
    if (t.length > 120)
        t = t.substring(0, 117) + "...";
    return PREFIX + t;
}

// o : {category, description, steps, includeEnv, version, os}
function issueBody(o, description) {
    var cat = category(o.category);
    var b = "### Catégorie\n" + cat.text + "\n\n### Description\n" + (description !== undefined ? description : clean(o.description)) + "\n";
    if (cat.id === "bug" && clean(o.steps) !== "")
        b += "\n### Étapes pour reproduire\n" + clean(o.steps) + "\n";
    if (o.includeEnv)
        b += "\n### Environnement\n- FootCoach " + clean(o.version) + "\n- Système : " + clean(o.os) + "\n";
    b += "\n---\n*Ticket créé depuis l'application FootCoach (menu Paramètres).*\n";
    return b;
}

function issueUrl(o) {
    var cat = category(o.category);
    var title = issueTitle(o.title);
    var desc = clean(o.description);
    function build(d) {
        return "https://github.com/" + REPO + "/issues/new?title=" + encodeURIComponent(title)
               + "&body=" + encodeURIComponent(issueBody(o, d)) + "&labels=" + encodeURIComponent(cat.label);
    }
    var url = build(desc);
    while (url.length > MAX_URL && desc.length > 40) {      // description trop longue : on la tronque
        desc = desc.substring(0, Math.max(40, Math.floor(desc.length * 0.8)));
        url = build(desc + " […]");
    }
    return url;
}

function mailUrl(o) {
    var subject = issueTitle(o.title);
    return "mailto:" + CONTACT + "?subject=" + encodeURIComponent(subject) + "&body=" + encodeURIComponent(issueBody(o));
}

function contactUrl(version) {
    return "mailto:" + CONTACT + "?subject=" + encodeURIComponent(PREFIX + "Contact");
}

var PRIVACY_URL = "https://github.com/" + REPO + "/blob/main/docs/confidentialite.md";
