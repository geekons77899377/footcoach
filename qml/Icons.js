.pragma library

// Icônes dessinées en traits sur une grille 24 × 24 (aucune image à embarquer).
function line(c, x1, y1, x2, y2) { c.beginPath(); c.moveTo(x1, y1); c.lineTo(x2, y2); c.stroke(); }
function circle(c, x, y, r, fill) { c.beginPath(); c.arc(x, y, r, 0, Math.PI * 2); if (fill) c.fill(); else c.stroke(); }
function poly(c, pts, close, fill) {
    c.beginPath(); c.moveTo(pts[0][0], pts[0][1]);
    for (var i = 1; i < pts.length; ++i) c.lineTo(pts[i][0], pts[i][1]);
    if (close) c.closePath();
    if (fill) c.fill(); else c.stroke();
}
function rrect(c, x, y, w, h, r) {
    c.beginPath(); c.moveTo(x + r, y); c.lineTo(x + w - r, y); c.quadraticCurveTo(x + w, y, x + w, y + r);
    c.lineTo(x + w, y + h - r); c.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
    c.lineTo(x + r, y + h); c.quadraticCurveTo(x, y + h, x, y + h - r);
    c.lineTo(x, y + r); c.quadraticCurveTo(x, y, x + r, y); c.closePath(); c.stroke();
}
function head(c, x, y, angle, size) {
    c.beginPath(); c.moveTo(x, y);
    c.lineTo(x - Math.cos(angle - 0.45) * size, y - Math.sin(angle - 0.45) * size);
    c.lineTo(x - Math.cos(angle + 0.45) * size, y - Math.sin(angle + 0.45) * size);
    c.closePath(); c.fill();
}
function undo(c) {
    poly(c, [[9, 4], [4, 9], [9, 14]], false);
    c.beginPath(); c.moveTo(4, 9); c.lineTo(14, 9); c.arc(14, 14.5, 5.5, -Math.PI / 2, Math.PI / 2); c.lineTo(9, 20); c.stroke();
}
function rotate(c, flip) {
    c.save();
    if (flip) { c.translate(24, 0); c.scale(-1, 1); }
    c.beginPath(); c.arc(12, 13, 7, Math.PI * 1.15, Math.PI * 0.85, false); c.stroke();   // presque un tour complet
    poly(c, [[4.5, 4.5], [4.5, 9.5], [9.5, 9.5]], false);
    c.restore();
}
function draw(c, name) {
    switch (name) {
    case "home": poly(c, [[3, 11], [12, 3], [21, 11]]); poly(c, [[5, 9.5], [5, 21], [19, 21], [19, 9.5]]);
        poly(c, [[10, 21], [10, 14], [14, 14], [14, 21]]); break;
    case "users": circle(c, 9, 8, 3.5);
        c.beginPath(); c.moveTo(3, 20); c.bezierCurveTo(3, 15.5, 6, 14, 9, 14); c.bezierCurveTo(12, 14, 15, 15.5, 15, 20); c.stroke();
        circle(c, 17, 9, 2.6);
        c.beginPath(); c.moveTo(16.5, 14.2); c.bezierCurveTo(19.5, 14.2, 21.5, 15.8, 21.5, 19.5); c.stroke(); break;
    case "calendar": rrect(c, 3, 5, 18, 16, 2.5); line(c, 3, 10, 21, 10); line(c, 8, 3, 8, 7); line(c, 16, 3, 16, 7); break;
    case "check": rrect(c, 5, 4.5, 14, 17, 2.5); rrect(c, 9, 2.5, 6, 4, 1.5); poly(c, [[9, 13.5], [11.2, 15.7], [15.5, 11]]); break;
    case "menu": line(c, 4, 7, 20, 7); line(c, 4, 12, 20, 12); line(c, 4, 17, 20, 17); break;
    case "plus": line(c, 12, 5, 12, 19); line(c, 5, 12, 19, 12); break;
    case "minus": line(c, 5, 12, 19, 12); break;
    case "close": line(c, 6, 6, 18, 18); line(c, 18, 6, 6, 18); break;
    case "tick": poly(c, [[5, 12.5], [10, 17.5], [19, 7]]); break;
    case "search": circle(c, 11, 11, 6.5); line(c, 16, 16, 21, 21); break;
    case "back": poly(c, [[15, 5], [8, 12], [15, 19]]); break;
    case "chevron": poly(c, [[9, 5], [16, 12], [9, 19]]); break;
    case "undo": undo(c); break;
    case "redo": c.save(); c.translate(24, 0); c.scale(-1, 1); undo(c); c.restore(); break;
    case "trash": line(c, 4, 7, 20, 7); poly(c, [[9, 7], [9, 4], [15, 4], [15, 7]]);
        poly(c, [[6, 7], [7, 20], [17, 20], [18, 7]]); line(c, 10, 11, 10, 16); line(c, 14, 11, 14, 16); break;
    case "rotateL": rotate(c, false); break;
    case "rotateR": rotate(c, true); break;
    case "zoomIn": circle(c, 10.5, 10.5, 6.5); line(c, 15.5, 15.5, 21, 21); line(c, 10.5, 7.5, 10.5, 13.5); line(c, 7.5, 10.5, 13.5, 10.5); break;
    case "zoomOut": circle(c, 10.5, 10.5, 6.5); line(c, 15.5, 15.5, 21, 21); line(c, 7.5, 10.5, 13.5, 10.5); break;
    case "fit": poly(c, [[4, 9], [4, 4], [9, 4]]); poly(c, [[15, 4], [20, 4], [20, 9]]);
        poly(c, [[20, 15], [20, 20], [15, 20]]); poly(c, [[9, 20], [4, 20], [4, 15]]); break;
    case "dots": circle(c, 12, 5, 1.7, true); circle(c, 12, 12, 1.7, true); circle(c, 12, 19, 1.7, true); break;
    case "edit": poly(c, [[4, 20], [4.8, 15.8], [16, 4.6], [19.4, 8], [8.2, 19.2], [4, 20]]); line(c, 14, 6.6, 17.4, 10); break;
    case "copy": rrect(c, 8, 8, 12, 12, 2.5); poly(c, [[16, 8], [16, 6.5], [13.5, 4], [6.5, 4], [4, 6.5], [4, 13.5], [6.5, 16], [8, 16]]); break;
    case "flip": poly(c, [[7, 4], [3, 8], [7, 12]]); line(c, 3, 8, 21, 8); poly(c, [[17, 12], [21, 16], [17, 20]]); line(c, 3, 16, 21, 16); break;
    case "palette": circle(c, 12, 12, 8.5); circle(c, 8.5, 10, 1.3, true); circle(c, 12, 7.5, 1.3, true); circle(c, 15.5, 10, 1.3, true); break;
    case "board": rrect(c, 3, 5, 18, 14, 2); line(c, 12, 5, 12, 19); circle(c, 12, 12, 2.8); break;
    case "exercise": poly(c, [[12, 4], [19, 20], [5, 20]], true); line(c, 8.2, 14, 15.8, 14); break;
    case "session": circle(c, 12, 12, 8.5); poly(c, [[12, 7], [12, 12], [15.5, 14]]); break;
    case "text": line(c, 6, 6, 18, 6); line(c, 12, 6, 12, 19); line(c, 9, 19, 15, 19); break;
    case "orient": poly(c, [[4, 12], [4, 7], [9, 7]]); c.beginPath(); c.arc(12, 12, 7.5, Math.PI * 1.1, Math.PI * 1.9, false); c.stroke(); break;
    // Outils du terrain
    case "select": poly(c, [[6, 3.5], [18.5, 12.5], [12.2, 13.6], [9, 19.5]], true); break;
    case "player": circle(c, 12, 12, 7.5); circle(c, 12, 12, 2.2, true); break;
    case "opponent": circle(c, 12, 12, 7.5); line(c, 8.5, 8.5, 15.5, 15.5); line(c, 15.5, 8.5, 8.5, 15.5); break;
    case "ball": circle(c, 12, 12, 7); poly(c, [[12, 8.2], [15.4, 10.7], [14.1, 14.7], [9.9, 14.7], [8.6, 10.7]], true); break;
    case "cone": poly(c, [[12, 4], [19.5, 19], [4.5, 19]], true); line(c, 8.4, 13, 15.6, 13); break;
    case "goal": rrect(c, 8.5, 3.5, 7, 17, 1.2); line(c, 8.5, 8, 15.5, 8); line(c, 8.5, 13, 15.5, 13); break;
    case "run": c.setLineDash([2.6, 2.4]); line(c, 3.5, 19.5, 16, 7); c.setLineDash([]); head(c, 20.5, 3.5, -Math.PI / 4, 6.5); break;
    case "pass": line(c, 3.5, 19.5, 16.5, 6.5); head(c, 20.5, 3.5, -Math.PI / 4, 6.5); break;
    case "cross": c.beginPath(); c.moveTo(3.5, 20); c.quadraticCurveTo(3.5, 5, 17, 5.8); c.stroke(); head(c, 21, 6, 0.1, 6.5); break;
    case "dribble": c.beginPath(); c.moveTo(3, 19);
        for (var i = 1; i <= 20; ++i) { var t = i / 20; c.lineTo(3 + 13 * t + Math.cos(-Math.PI / 4 + Math.PI / 2) * Math.sin(t * 6 * Math.PI) * 2.0,
                                                            19 - 13 * t + Math.sin(-Math.PI / 4 + Math.PI / 2) * Math.sin(t * 6 * Math.PI) * 2.0); }
        c.stroke(); head(c, 20.5, 3.5, -Math.PI / 4, 6.5); break;
    }
}
