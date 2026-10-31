.pragma library

// Logique pure de la « séance du jour » : choix de la séance selon l'heure du téléphone et calcul des chronomètres.
// Aucun état ici : tout se déduit de l'heure et de quelques horodatages, donc un verrouillage d'écran ou une
// application mise en arrière-plan ne fausse jamais un chronomètre.

// Début de la séance en millisecondes (date "AAAA-MM-JJ" + heure "HH:mm" du téléphone), NaN si incomplet.
function startMs(session) {
    var d = session.date, t = session.time;
    if (!d || !t)
        return NaN;
    return new Date(parseInt(d.substring(0, 4), 10), parseInt(d.substring(5, 7), 10) - 1,
                    parseInt(d.substring(8, 10), 10), parseInt(t.substring(0, 2), 10),
                    parseInt(t.substring(3, 5), 10), 0, 0).getTime();
}

function endMs(session) {
    var s = startMs(session);
    if (isNaN(s))
        return NaN;
    return s + (session.durationMin > 0 ? session.durationMin : 90) * 60000;
}

// Séance à afficher d'office. sessions : séances du jour (backend.sessions). Retour :
// { session, mode: "now" | "next" | "none", overlaps: nombre de séances en cours en même temps }.
// "now" : l'heure du téléphone est dans la plage de la séance (la plus récemment commencée si plusieurs) ;
// "next" : sinon la prochaine séance du jour ; sinon la dernière terminée, en mode "none" avec session = null.
function pickAuto(sessions, nowMs) {
    var live = [], upcoming = [];
    for (var i = 0; i < sessions.length; ++i) {
        var s = sessions[i];
        if (s.status === "cancelled")
            continue;
        var a = startMs(s), b = endMs(s);
        if (isNaN(a))
            continue;
        if (nowMs >= a && nowMs < b)
            live.push({ s: s, a: a });
        else if (a > nowMs)
            upcoming.push({ s: s, a: a });
    }
    if (live.length > 0) {
        live.sort(function (x, y) { return y.a - x.a; });
        return { session: live[0].s, mode: "now", overlaps: live.length };
    }
    if (upcoming.length > 0) {
        upcoming.sort(function (x, y) { return x.a - y.a; });
        return { session: upcoming[0].s, mode: "next", overlaps: 0 };
    }
    return { session: null, mode: "none", overlaps: 0 };
}

// "mm:ss" ; au-delà de zéro (temps dépassé) : "+mm:ss". À partir d'une heure : "h:mm:ss".
function clock(ms) {
    var neg = ms < 0;
    var total = Math.floor(Math.abs(ms) / 1000);
    var h = Math.floor(total / 3600), m = Math.floor((total % 3600) / 60), s = total % 60;
    var core = h > 0 ? h + ":" + (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s
                     : (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s;
    return (neg ? "+" : "") + core;
}

// Temps restant (peut être négatif) de l'exercice en cours : durée prévue - temps écoulé.
function remaining(plannedMin, elapsedMs) {
    return plannedMin * 60000 - elapsedMs;
}

// Temps écoulé d'un exercice : cumul avant la pause + durée depuis la dernière reprise (si en marche).
function elapsed(accMs, startedAt, running, nowMs) {
    var e = accMs + (running ? Math.max(0, nowMs - startedAt) : 0);
    return e < 0 ? 0 : e;
}

// "20'" pour une durée en minutes.
function minutes(n) { return n + "'"; }

// Durée réelle d'un exercice terminé, "12'" (arrondie à la minute la plus proche, 1 minute au moins).
function actual(ms) { return Math.max(1, Math.round(ms / 60000)) + "'"; }
