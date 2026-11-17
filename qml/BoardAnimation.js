.pragma library

// Animation d'un schéma à partir de ses flèches (décision du 16/11/2026) : chaque flèche devient un mouvement, dans
// l'ordre où les flèches ont été tracées.
//  * Course : le joueur (ou l'adversaire) le plus proche du départ de la flèche la suit.
//  * Conduite : le joueur le plus proche la suit avec le ballon qui est à ses pieds (ballon dessiné s'il n'y en a pas).
//  * Passe, centre : le ballon le plus proche du départ la suit (ballon dessiné s'il n'y en a pas) ; le centre suit
//    la même courbe que la flèche.
//  * « Le plus proche » : à moins de SEARCH_M mètres du départ, en tenant compte des mouvements précédents. Une flèche
//    sans élément à déplacer est seulement mise en avant pendant son tour.
// Positions normalisées comme le schéma : x = profondeur (0 ligne médiane ou but gauche), y = largeur (0 à gauche).

var FIELD_L = 105;
var FIELD_W = 68;
var HALF_L = 52.5;

var SPEED = { run: 5.5, dribble: 4, pass: 13, cross: 15 };    // mètres par seconde, à la vitesse normale
var MIN_DUR = 0.6;          // durée minimale d'un mouvement (s)
var MAX_DUR = 4.0;
var GAP = 0.25;             // pause entre deux mouvements
var LEAD = 0.6;             // pause avant le premier mouvement
var HOLD = 1.2;             // pause sur la position finale, avant de recommencer
var SEARCH_M = 4.5;         // rayon de recherche de l'élément qui suit une flèche (mètres)
var BALL_FOOT_M = 0.9;      // ballon devant les pieds du joueur pendant une conduite (mètres)

function depthOf(pitch) { return pitch === "half" ? HALF_L : FIELD_L; }

// Normalisé -> mètres et retour.
function toM(p, depth) { return { x: p.x * depth, y: p.y * FIELD_W }; }
function toN(m, depth) {
    return { x: Math.max(0, Math.min(1, m.x / depth)), y: Math.max(0, Math.min(1, m.y / FIELD_W)) };
}

function dist(a, b) { return Math.sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }

// Élément le plus proche de « at » (mètres) parmi les genres acceptés, à moins de SEARCH_M ; -1 sinon.
function nearest(pos, items, kinds, at, depth, exclude) {
    var best = -1, bestD = SEARCH_M;
    for (var i = 0; i < items.length; ++i) {
        if (kinds.indexOf(items[i].kind) < 0 || i === exclude)
            continue;
        var d = dist(toM(pos[i], depth), at);
        if (d <= bestD) {
            best = i;
            bestD = d;
        }
    }
    return best;
}

// Point de la trajectoire (mètres) au fraction p (0 à 1) : droite, ou courbe du centre (même formule que la flèche).
function pathPoint(step, p) {
    var a = step.fromM, b = step.toM;
    if (step.style === "cross" && step.curve !== 0) {
        var dx = b.x - a.x, dy = b.y - a.y;
        var len = Math.sqrt(dx * dx + dy * dy);
        if (len > 0) {
            var nx = dy / len, ny = -dx / len;
            var cx = (a.x + b.x) / 2 + nx * (2 * step.curve * len);
            var cy = (a.y + b.y) / 2 + ny * (2 * step.curve * len);
            var u = 1 - p;
            return { x: u * u * a.x + 2 * u * p * cx + p * p * b.x, y: u * u * a.y + 2 * u * p * cy + p * p * b.y };
        }
    }
    return { x: a.x + (b.x - a.x) * p, y: a.y + (b.y - a.y) * p };
}

// Accélération et freinage doux pour les joueurs, départ vif pour le ballon.
function ease(style, p) {
    if (style === "pass" || style === "cross")
        return p * (2 - p);
    return p * p * (3 - 2 * p);
}

// Ballon devant les pieds : décalé dans le sens du déplacement.
function footBall(actorM, step) {
    var dx = step.toM.x - step.fromM.x, dy = step.toM.y - step.fromM.y;
    var len = Math.sqrt(dx * dx + dy * dy);
    if (len < 0.01)
        return actorM;
    return { x: actorM.x + dx / len * BALL_FOOT_M, y: actorM.y + dy / len * BALL_FOOT_M };
}

// Chronologie de l'animation. items : [{kind, nx, ny}], arrows : [{style, x1, y1, x2, y2, curve}].
// Résultat : {pitch, depth, start: [{x, y}], steps: [...], total} ; steps vide si le schéma n'a pas de flèche.
function build(items, arrows, pitch) {
    var depth = depthOf(pitch);
    var pos = [];
    for (var i = 0; i < items.length; ++i)
        pos.push({ x: items[i].nx, y: items[i].ny });
    var start = pos.map(function (p) { return { x: p.x, y: p.y }; });
    var steps = [];
    var t = LEAD;
    for (var j = 0; j < arrows.length; ++j) {
        var a = arrows[j];
        var fromM = toM({ x: a.x1, y: a.y1 }, depth), toMm = toM({ x: a.x2, y: a.y2 }, depth);
        var step = { arrow: j, style: a.style, curve: a.curve || 0, fromM: fromM, toM: toMm,
                     actor: -1, ball: -1, ghostBall: false };
        if (a.style === "run" || a.style === "dribble")
            step.actor = nearest(pos, items, ["player", "opponent"], fromM, depth, -1);
        if (a.style === "dribble") {
            var near = step.actor >= 0 ? toM(pos[step.actor], depth) : fromM;
            step.ball = nearest(pos, items, ["ball"], near, depth, -1);
            step.ghostBall = step.ball < 0;
        } else if (a.style === "pass" || a.style === "cross") {
            step.ball = nearest(pos, items, ["ball"], fromM, depth, -1);
            step.ghostBall = step.ball < 0;
        }
        // La trajectoire part de l'élément (pas exactement du départ tracé), pour qu'il ne saute pas.
        if (step.actor >= 0)
            step.fromM = toM(pos[step.actor], depth);
        else if (step.ball >= 0 && step.style !== "dribble")
            step.fromM = toM(pos[step.ball], depth);
        var len = dist(step.fromM, step.toM);
        var speed = SPEED[a.style] || SPEED.run;
        step.dur = Math.max(MIN_DUR, Math.min(MAX_DUR, len / speed));
        step.t0 = t;
        t += step.dur + GAP;
        // Positions après ce mouvement.
        if (step.actor >= 0)
            pos[step.actor] = toN(step.toM, depth);
        if (step.ball >= 0)
            pos[step.ball] = toN(step.style === "dribble" ? footBall(step.toM, step) : step.toM, depth);
        steps.push(step);
    }
    var total = steps.length > 0 ? t - GAP + HOLD : 0;
    return { pitch: pitch, depth: depth, start: start, steps: steps, total: total };
}

// État à l'instant t (secondes) : {pos: [{x, y}] normalisées, step (indice du mouvement en cours, -1 entre deux),
// done (nombre de mouvements terminés), ghost: {x, y} ou null (ballon dessiné), progress (0 à 1 du mouvement)}.
function stateAt(tl, t) {
    var depth = tl.depth;
    var pos = tl.start.map(function (p) { return { x: p.x, y: p.y }; });
    var current = -1, done = 0, ghost = null, progress = 0;
    for (var k = 0; k < tl.steps.length; ++k) {
        var s = tl.steps[k];
        var end = s.t0 + s.dur;
        var p;
        if (t >= end) {
            p = 1;
            done = k + 1;
        } else if (t > s.t0) {
            p = (t - s.t0) / s.dur;
            current = k;
            progress = p;
        } else {
            break;
        }
        var m = pathPoint(s, ease(s.style, p));
        if (s.actor >= 0)
            pos[s.actor] = toN(m, depth);
        var ballM = s.style === "dribble" ? footBall(m, s) : m;
        if (s.ball >= 0)
            pos[s.ball] = toN(ballM, depth);
        if (s.ghostBall && p < 1)
            ghost = toN(ballM, depth);
    }
    return { pos: pos, step: current, done: done, ghost: ghost, progress: progress };
}

// Début et fin d'un mouvement (secondes).
function stepStart(tl, k) { return k < 0 ? 0 : k >= tl.steps.length ? tl.total : tl.steps[k].t0; }
function stepEnd(tl, k) { return k < 0 ? 0 : k >= tl.steps.length ? tl.total : tl.steps[k].t0 + tl.steps[k].dur; }

// Mouvement suivant à jouer depuis t : indice du premier mouvement qui n'est pas terminé, steps.length s'il n'y en a plus.
function nextStepFrom(tl, t) {
    for (var k = 0; k < tl.steps.length; ++k) {
        if (t < tl.steps[k].t0 + tl.steps[k].dur - 1e-6)
            return k;
    }
    return tl.steps.length;
}

// Début du mouvement précédent : celui en cours s'il a commencé depuis plus d'un instant, sinon celui d'avant.
function previousStart(tl, t) {
    for (var k = tl.steps.length - 1; k >= 0; --k) {
        if (t > tl.steps[k].t0 + 0.05)
            return tl.steps[k].t0 - (k === 0 ? LEAD : GAP) * 0.5;
    }
    return 0;
}
