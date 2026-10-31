.pragma library

// Géométrie du tableau tactique, sans aucune dépendance à QML (testable avec Node).
//
// Repères :
//  - « normalisé » (nx, ny) : celui du JSON enregistré. nx = profondeur (terrain entier : 0 = but défendu,
//    1 = but visé ; demi-terrain : 0 = ligne médiane, 1 = but visé), ny = de la gauche (0) vers la droite (1)
//    du sens de jeu.
//  - « terrain » (xf, yf) en mètres : terrain entier de 105 × 68, xf le long du terrain, yf en travers.
//  - « écran » : pixels de la vue.
// Le terrain peut être affiché dans deux orientations : « horizontale » (on attaque vers la droite) ou
// « verticale » (on attaque vers le haut). On choisit celle qui donne le plus grand terrain dans la vue.

var FIELD_L = 105;
var FIELD_W = 68;
var HALF_L = 52.5;

function clamp(v, lo, hi) {
    return Math.max(lo, Math.min(hi, v));
}

// Dimensions affichées du terrain : { vertical, depth, width, s (px par mètre au zoom 1), pw, ph (px) }.
function frameFor(pitch, viewW, viewH, margin) {
    var depth = pitch === "half" ? HALF_L : FIELD_L;
    var aw = Math.max(1, viewW - 2 * margin);
    var ah = Math.max(1, viewH - 2 * margin);
    var sH = Math.min(aw / depth, ah / FIELD_W);     // orientation horizontale : largeur = profondeur
    var sV = Math.min(aw / FIELD_W, ah / depth);     // orientation verticale : hauteur = profondeur
    var vertical;
    if (Math.abs(sH - sV) < 1e-9)
        vertical = pitch === "half";                 // égalité : orientation habituelle (demi-terrain : but en haut)
    else
        vertical = sV > sH;
    var s = vertical ? sV : sH;
    return {
        vertical: vertical,
        depth: depth,
        width: FIELD_W,
        s: s,
        pw: (vertical ? FIELD_W : depth) * s,
        ph: (vertical ? depth : FIELD_W) * s
    };
}

// Normalisé -> position dans le rectangle affiché, de 0 à 1 (u vers la droite, v vers le bas).
function toUV(nx, ny, vertical) {
    return vertical ? { u: ny, v: 1 - nx } : { u: nx, v: ny };
}

function fromUV(u, v, vertical) {
    return vertical ? { x: 1 - v, y: u } : { x: u, y: v };
}

// Le JSON enregistre l'angle des buts dans le repère « habituel » du type de terrain (terrain entier : horizontal,
// demi-terrain : vertical). Quand on l'affiche dans l'autre orientation, tout le dessin tourne d'un quart de tour.
function orientationOffset(pitch, vertical) {
    var defaultVertical = pitch === "half";
    if (vertical === defaultVertical)
        return 0;
    return vertical ? -90 : 90;
}

function nextFreePlayerNumber(items) {
    var used = {};
    for (var i = 0; i < items.length; ++i) {
        if (items[i].kind !== "player")
            continue;
        var n = parseInt(items[i].label, 10);
        if (!isNaN(n))
            used[n] = true;
    }
    for (var k = 1; k <= 99; ++k) {
        if (!used[k])
            return k;
    }
    return 0;
}


// --- Alignement et rotation d'un groupe ------------------------------------------------------
// Les calculs se font en mètres dans le repère AFFICHÉ (x vers la droite, y vers le bas), pour que « à gauche »
// veuille dire à gauche à l'écran quelle que soit l'orientation du terrain, et que les rotations gardent les
// proportions du terrain (105 × 68 m).

function displaySize(pitch, vertical) {
    var depth = pitch === "half" ? HALF_L : FIELD_L;
    return vertical ? { w: FIELD_W, h: depth } : { w: depth, h: FIELD_W };
}

function toMetres(nx, ny, pitch, vertical) {
    var uv = toUV(nx, ny, vertical), d = displaySize(pitch, vertical);
    return { x: uv.u * d.w, y: uv.v * d.h };
}

function fromMetres(x, y, pitch, vertical) {
    var d = displaySize(pitch, vertical);
    return fromUV(x / d.w, y / d.h, vertical);          // { x: nx, y: ny }
}

// Alignement de points { x, y } : left, hcenter, right, top, vcenter, bottom ; hdist, vdist = répartition
// régulière entre les deux extrêmes (l'ordre des points est conservé). Renvoie de nouveaux points, dans le même ordre.
function alignPoints(pts, mode) {
    var n = pts.length, out = [], i;
    if (n === 0)
        return out;
    var minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
    for (i = 0; i < n; ++i) {
        minX = Math.min(minX, pts[i].x); maxX = Math.max(maxX, pts[i].x);
        minY = Math.min(minY, pts[i].y); maxY = Math.max(maxY, pts[i].y);
    }
    for (i = 0; i < n; ++i)
        out.push({ x: pts[i].x, y: pts[i].y });
    if (mode === "left") { for (i = 0; i < n; ++i) out[i].x = minX; }
    else if (mode === "right") { for (i = 0; i < n; ++i) out[i].x = maxX; }
    else if (mode === "hcenter") { for (i = 0; i < n; ++i) out[i].x = (minX + maxX) / 2; }
    else if (mode === "top") { for (i = 0; i < n; ++i) out[i].y = minY; }
    else if (mode === "bottom") { for (i = 0; i < n; ++i) out[i].y = maxY; }
    else if (mode === "vcenter") { for (i = 0; i < n; ++i) out[i].y = (minY + maxY) / 2; }
    else if (mode === "hdist" || mode === "vdist") {
        var key = mode === "hdist" ? "x" : "y";
        var order = [];
        for (i = 0; i < n; ++i) order.push(i);
        order.sort(function (a, b) { return pts[a][key] - pts[b][key] || a - b; });
        var lo = mode === "hdist" ? minX : minY, hi = mode === "hdist" ? maxX : maxY;
        for (i = 0; i < n; ++i)
            out[order[i]][key] = n > 1 ? lo + (hi - lo) * i / (n - 1) : lo;
    }
    return out;
}

// Rotation de points autour de (cx, cy), degrés, sens horaire à l'écran (y vers le bas).
function rotatePoints(pts, cx, cy, degrees) {
    var a = degrees * Math.PI / 180, c = Math.cos(a), s = Math.sin(a), out = [];
    for (var i = 0; i < pts.length; ++i) {
        var dx = pts[i].x - cx, dy = pts[i].y - cy;
        out.push({ x: cx + dx * c - dy * s, y: cy + dx * s + dy * c });
    }
    return out;
}

// Vrai si tous les points (mètres affichés) tiennent dans le terrain, à une tolérance près.
function insidePitch(pts, pitch, vertical) {
    var d = displaySize(pitch, vertical), eps = 1e-6;
    for (var i = 0; i < pts.length; ++i) {
        if (pts[i].x < -eps || pts[i].x > d.w + eps || pts[i].y < -eps || pts[i].y > d.h + eps)
            return false;
    }
    return true;
}

// --- Flèches ----------------------------------------------------------------------------------

// Tracé d'une flèche en pixels d'écran. style : run, pass, dribble, cross ; curve : courbure des centres.
// opts : { head (taille de la pointe), amp (amplitude de l'ondulation), period (période) }.
// Résultat : { line: [[x, y], ...], head: [[x, y], [x, y], [x, y]] } (vide si la flèche est trop courte).
function arrowGeometry(style, x1, y1, x2, y2, curve, opts) {
    var headMax = opts && opts.head ? opts.head : 18;
    var amp = opts && opts.amp ? opts.amp : 6;
    var period = opts && opts.period ? opts.period : 28;
    var dx = x2 - x1, dy = y2 - y1;
    var length = Math.sqrt(dx * dx + dy * dy);
    if (length < 1)
        return { line: [[x1, y1]], head: [] };

    var headLength = Math.min(headMax, length);
    var nx = dy / length, ny = -dx / length;           // à gauche du sens de la flèche (y vers le bas)
    var line = [[x1, y1]];
    var refX = x1, refY = y1;                           // la tête suit la direction depuis ce point

    if (style === "cross") {
        // Bézier quadratique : le point de contrôle est à deux fois la flèche voulue.
        var cx = (x1 + x2) / 2 + nx * (2 * curve * length);
        var cy = (y1 + y2) / 2 + ny * (2 * curve * length);
        for (var i = 1; i <= 48; ++i) {
            var t = i / 48, u = 1 - t;
            var px = u * u * x1 + 2 * u * t * cx + t * t * x2;
            var py = u * u * y1 + 2 * u * t * cy + t * t * y2;
            if (Math.hypot(px - x2, py - y2) < headLength * 0.6)
                break;
            line.push([px, py]);
        }
        refX = cx;
        refY = cy;
    } else {
        var body = length - headLength * 0.6;
        var ex = x1 + dx / length * body, ey = y1 + dy / length * body;
        if (style === "dribble") {
            var steps = Math.max(8, Math.floor(body / 2));
            for (var j = 1; j <= steps; ++j) {
                var tt = j / steps;
                var bx = x1 + (ex - x1) * tt, by = y1 + (ey - y1) * tt;
                var off = amp * Math.sin(tt * body / period * 2 * Math.PI) * Math.min(1, tt * body / (period / 2));
                line.push([bx + nx * off, by + ny * off]);
            }
        } else {
            line.push([ex, ey]);
        }
    }

    var angle = Math.atan2(y2 - refY, x2 - refX);
    var size = Math.min(headMax, length);
    var head = [
        [x2, y2],
        [x2 - Math.cos(angle - Math.PI / 7) * size, y2 - Math.sin(angle - Math.PI / 7) * size],
        [x2 - Math.cos(angle + Math.PI / 7) * size, y2 - Math.sin(angle + Math.PI / 7) * size]
    ];
    return { line: line, head: head };
}

function distToSegment(px, py, ax, ay, bx, by) {
    var vx = bx - ax, vy = by - ay;
    var len2 = vx * vx + vy * vy;
    var t = len2 > 0 ? clamp(((px - ax) * vx + (py - ay) * vy) / len2, 0, 1) : 0;
    return Math.hypot(px - (ax + t * vx), py - (ay + t * vy));
}

// Distance d'un point au tracé d'une flèche (pointe comprise).
function distToArrow(geom, px, py) {
    var best = Infinity;
    var pts = geom.line;
    for (var i = 1; i < pts.length; ++i)
        best = Math.min(best, distToSegment(px, py, pts[i - 1][0], pts[i - 1][1], pts[i][0], pts[i][1]));
    if (pts.length > 0 && geom.head.length > 0)
        best = Math.min(best, distToSegment(px, py, pts[pts.length - 1][0], pts[pts.length - 1][1],
                                            geom.head[0][0], geom.head[0][1]));
    return best;
}

// --- Terrain ----------------------------------------------------------------------------------

function arcPoints(cx, cy, r, a0, a1, n) {
    var pts = [];
    for (var i = 0; i <= n; ++i) {
        var a = (a0 + (a1 - a0) * i / n) * Math.PI / 180;
        pts.push([cx + r * Math.cos(a), cy + r * Math.sin(a)]);
    }
    return pts;
}

function rectPoints(x, y, w, h) {
    return [[x, y], [x + w, y], [x + w, y + h], [x, y + h], [x, y]];
}

// Traits du terrain entier, en mètres (repère terrain). Chaque trait : { pts, fill (bool) }.
function pitchLines() {
    var L = FIELD_L, W = FIELD_W, c = W / 2;
    var penD = 16.5, penH = 20.16, goalD = 5.5, goalH = 9.16, spot = 11, r = 9.15;
    var arcA = Math.acos((penD - spot) / r) * 180 / Math.PI;     // ≈ 53°
    var lines = [];
    lines.push({ pts: rectPoints(0, 0, L, W) });
    lines.push({ pts: [[L / 2, 0], [L / 2, W]] });
    lines.push({ pts: arcPoints(L / 2, c, r, 0, 360, 72) });
    lines.push({ pts: rectPoints(0, c - penH, penD, 2 * penH) });
    lines.push({ pts: rectPoints(L - penD, c - penH, penD, 2 * penH) });
    lines.push({ pts: rectPoints(0, c - goalH, goalD, 2 * goalH) });
    lines.push({ pts: rectPoints(L - goalD, c - goalH, goalD, 2 * goalH) });
    lines.push({ pts: arcPoints(spot, c, r, -arcA, arcA, 24) });
    lines.push({ pts: arcPoints(L - spot, c, r, 180 - arcA, 180 + arcA, 24) });
    lines.push({ pts: arcPoints(0, 0, 1, 0, 90, 8) });
    lines.push({ pts: arcPoints(L, 0, 1, 90, 180, 8) });
    lines.push({ pts: arcPoints(L, W, 1, 180, 270, 8) });
    lines.push({ pts: arcPoints(0, W, 1, 270, 360, 8) });
    // Buts (7,32 m de large, 2,4 m de profondeur) derrière les lignes de but.
    lines.push({ pts: rectPoints(-2.4, c - 3.66, 2.4, 7.32), fill: true });
    lines.push({ pts: rectPoints(L, c - 3.66, 2.4, 7.32), fill: true });
    // Points : centre et points de penalty.
    lines.push({ dot: [L / 2, c] });
    lines.push({ dot: [spot, c] });
    lines.push({ dot: [L - spot, c] });
    return lines;
}
