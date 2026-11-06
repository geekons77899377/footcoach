.pragma library

// Assistance : construit le titre et le corps du ticket « SUP - … » (envoyé par IssueReporter) et le lien e-mail.
// Aucun secret dans le QML : le jeton d'envoi est injecté à la compilation côté C++.

var REPO = "geekons77899377/footcoach";
var CONTACT = "footcoach.app@gmail.com";
var PREFIX = "SUP - ";                  // préfixe du titre de tous les tickets d'assistance

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

function mailUrl(o) {
    var subject = issueTitle(o.title);
    return "mailto:" + CONTACT + "?subject=" + encodeURIComponent(subject) + "&body=" + encodeURIComponent(issueBody(o));
}

function contactUrl(version) {
    return "mailto:" + CONTACT + "?subject=" + encodeURIComponent(PREFIX + "Contact");
}

