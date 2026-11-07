.pragma library

// Répartition des titulaires d'un match sur les postes d'une formation « 4-4-2 », « 4-2-3-1 »… selon leur poste (GK, DEF, MID, FWD).
// Les cases de Tactics::formation sont dans l'ordre : gardien, puis les lignes de la défense à l'attaque. La première ligne du
// nom est la défense, la dernière l'attaque, les autres le milieu. Seuls les titulaires sont placés (jamais les remplaçants).
//
// Plusieurs titulaires au même poste : on choisit d'après le pied fort (droitier à droite, gaucher à gauche, « les deux » partout,
// pied inconnu après), puis, à égalité, par prénom dans l'ordre alphabétique (accents et majuscules ignorés), puis par nom.
// Les cases sont remplies en commençant par les couloirs (les plus excentrées), où le pied fort compte, puis l'axe.

function slotRoles(name) {
    var parts = name.split("-").map(function (n) { return parseInt(n, 10); });
    var roles = ["GK"];
    for (var i = 0; i < parts.length; ++i) {
        var role = i === 0 ? "DEF" : (i === parts.length - 1 ? "FWD" : "MID");
        for (var j = 0; j < parts[i]; ++j)
            roles.push(role);
    }
    return roles;
}

function fold(s) {
    return String(s || "").normalize("NFD").replace(/[̀-ͯ]/g, "").toLowerCase().trim();
}

// Côté d'une case : y est la largeur du terrain (0 = haut). Équipe attaquant vers la droite : la droite du joueur est en bas (y grand).
function slotSide(y) {
    if (y > 0.56)
        return "R";
    if (y < 0.44)
        return "L";
    return "C";
}

// 0 = pied idéal pour le côté ; plus grand = moins bon. Au centre, le pied ne compte pas.
function footRank(foot, side) {
    if (side === "C")
        return 0;
    if (foot === side)
        return 0;
    if (foot === "B")
        return 1;
    if (!foot)
        return 2;
    return 3;
}

function before(a, b, side) {
    var fa = footRank(a.foot, side), fb = footRank(b.foot, side);
    if (fa !== fb)
        return fa < fb;
    var na = fold(a.firstName), nb = fold(b.firstName);
    if (na !== nb)
        return na < nb;
    var la = fold(a.lastName), lb = fold(b.lastName);
    if (la !== lb)
        return la < lb;
    return (a.index || 0) < (b.index || 0);
}

// starters : [{ label, position, foot ("R"|"L"|"B"|""), firstName, lastName }] ; slotYs : largeur (y) de chaque case, dans l'ordre.
// Renvoie { labels: [label | null par case], unplaced: nombre de titulaires sans case }.
function assign(name, starters, slotYs) {
    var roles = slotRoles(name);
    var list = starters.map(function (s, i) { s.index = i; return s; });
    var labels = [];
    var used = [];
    for (var k = 0; k < roles.length; ++k)
        labels.push(null);

    // Ordre de remplissage : couloirs d'abord (écart au centre décroissant), puis l'axe ; ordre de la formation à égalité.
    var order = [];
    for (var o = 0; o < roles.length; ++o)
        order.push(o);
    function wide(k) { return Math.abs((slotYs && slotYs[k] !== undefined ? slotYs[k] : 0.5) - 0.5); }
    order.sort(function (a, b) { return wide(b) - wide(a) || a - b; });

    function pick(k, accept) {
        var side = slotSide(slotYs && slotYs[k] !== undefined ? slotYs[k] : 0.5);
        var best = -1;
        for (var i = 0; i < list.length; ++i) {
            if (used[i] || !accept(list[i]))
                continue;
            if (best < 0 || before(list[i], list[best], side))
                best = i;
        }
        if (best >= 0) {
            labels[k] = list[best].label;
            used[best] = true;
        }
    }

    // 1) chaque case prend un titulaire de son poste ; 2) les cases restantes (hors but) prennent un titulaire sans poste ou d'un
    // autre poste, jamais un gardien dans le champ ; un joueur de champ ne va jamais au but : la case reste vide.
    for (var a = 0; a < order.length; ++a)
        pick(order[a], function (s) { return s.position === roles[order[a]]; });
    for (var b = 0; b < order.length; ++b) {
        var k2 = order[b];
        if (labels[k2] === null && roles[k2] !== "GK")
            pick(k2, function (s) { return s.position !== "GK"; });
    }
    var unplaced = 0;
    for (var u = 0; u < list.length; ++u)
        if (!used[u])
            ++unplaced;
    return { labels: labels, unplaced: unplaced };
}
