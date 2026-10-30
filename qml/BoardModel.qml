import QtQuick
import "BoardGeometry.js" as Geo

// Contenu d'un schéma tactique en cours d'édition : éléments, flèches, sélection, annuler / rétablir.
// Le format JSON est exactement celui de Tactics::toJson (C++) ; le Backend le revalide avant enregistrement.
//
// Coordonnées normalisées : voir BoardGeometry.js. Chaque élément : kind (player, opponent, ball, cone, goal, text),
// nx, ny, label, rot (degrés), scl (taille relative), color ("" = couleur par défaut), sel (sélectionné).
QtObject {
    id: root

    property string pitch: "full"                 // "full" ou "half"
    readonly property ListModel items: ListModel {}
    property var arrows: []                       // [{style, x1, y1, x2, y2, curve, sel}]
    property int revision: 0                      // augmente à chaque changement visible (repeindre le terrain)

    // Résumé de la sélection (pour afficher les bonnes actions).
    property int selCount: 0
    property int selTransformable: 0              // buts, plots, notes
    property int selColorable: 0
    property int selCrosses: 0
    property int selLabelIndex: -1                // élément unique sélectionné qui porte un texte, sinon -1

    property bool canUndo: false
    property bool canRedo: false

    // Émis après chaque modification validée par l'utilisateur (enregistrement automatique).
    signal edited()

    readonly property real minScale: 0.3
    readonly property real maxScale: 4.0
    readonly property real defaultCurve: 0.25

    property var _undo: []
    property var _redo: []
    property string _committed: ""
    property var _moveStart: []

    function isRotatable(kind) { return kind === "goal" || kind === "cone" || kind === "text"; }
    function isColorable(kind) { return kind === "player" || kind === "opponent" || kind === "cone" || kind === "text"; }
    function hasLabel(kind) { return kind === "player" || kind === "opponent" || kind === "text"; }

    // --- Sérialisation ------------------------------------------------------------------------

    function _object() {
        var out = { version: 1, pitch: pitch, items: [], arrows: [] };
        for (var i = 0; i < items.count; ++i) {
            var it = items.get(i);
            var o = { kind: it.kind, x: it.nx, y: it.ny };
            if (it.label !== "")
                o.label = it.label;
            if (isRotatable(it.kind) && it.rot !== 0)
                o.rot = it.rot;
            if (isRotatable(it.kind) && it.scl !== 1)
                o.scale = it.scl;
            if (isColorable(it.kind) && it.color !== "")
                o.color = it.color;
            out.items.push(o);
        }
        for (var j = 0; j < arrows.length; ++j) {
            var a = arrows[j];
            var ao = { style: a.style, x1: a.x1, y1: a.y1, x2: a.x2, y2: a.y2 };
            if (a.style === "cross")
                ao.curve = a.curve;
            out.arrows.push(ao);
        }
        return out;
    }

    function toJson() {
        return JSON.stringify(_object());
    }

    function _num(v, fallback) {
        return typeof v === "number" && isFinite(v) ? v : fallback;
    }

    // Charge un JSON (déjà validé par le Backend). Renvoie false si le texte est inutilisable.
    function _fill(json) {
        var data;
        try {
            data = (json === "" || json === "{}") ? {} : JSON.parse(json);
        } catch (e) {
            return false;
        }
        items.clear();
        var list = [];
        pitch = data.pitch === "half" ? "half" : "full";
        var src = data.items || [];
        for (var i = 0; i < src.length; ++i) {
            var o = src[i];
            var kind = o.kind;
            if (["player", "opponent", "ball", "cone", "goal", "text"].indexOf(kind) < 0)
                continue;
            items.append({
                kind: kind,
                nx: Geo.clamp(_num(o.x, 0.5), 0, 1),
                ny: Geo.clamp(_num(o.y, 0.5), 0, 1),
                label: typeof o.label === "string" ? o.label : "",
                rot: isRotatable(kind) ? ((_num(o.rot, 0) % 360) + 360) % 360 : 0,
                scl: isRotatable(kind) ? Geo.clamp(_num(o.scale, 1), minScale, maxScale) : 1,
                color: isColorable(kind) && typeof o.color === "string" ? o.color : "",
                sel: false
            });
        }
        var arr = data.arrows || [];
        for (var j = 0; j < arr.length; ++j) {
            var a = arr[j];
            if (["run", "pass", "dribble", "cross"].indexOf(a.style) < 0)
                continue;
            list.push({
                style: a.style,
                x1: Geo.clamp(_num(a.x1, 0), 0, 1), y1: Geo.clamp(_num(a.y1, 0), 0, 1),
                x2: Geo.clamp(_num(a.x2, 0), 0, 1), y2: Geo.clamp(_num(a.y2, 0), 0, 1),
                curve: a.style === "cross" ? Geo.clamp(_num(a.curve, defaultCurve), -0.6, 0.6) : 0,
                sel: false
            });
        }
        arrows = list;
        return true;
    }

    // Remplace tout le contenu et repart d'une pile d'annulation vide.
    function load(json) {
        var ok = _fill(json);
        if (!ok) {
            items.clear();
            arrows = [];
            pitch = "full";
        }
        _undo = [];
        _redo = [];
        _committed = toJson();
        canUndo = false;
        canRedo = false;
        _selectionChanged();
        revision++;
        return ok;
    }

    // --- Annuler / rétablir ---------------------------------------------------------------------

    // Enregistre l'état courant comme une action annulable, si quelque chose a changé.
    function commit() {
        var s = toJson();
        if (s === _committed)
            return false;
        var u = _undo.slice();
        u.push(_committed);
        if (u.length > 100)
            u.shift();
        _undo = u;
        _redo = [];
        _committed = s;
        canUndo = true;
        canRedo = false;
        revision++;
        edited();
        return true;
    }

    function undo() {
        if (_undo.length === 0)
            return;
        var u = _undo.slice();
        var previous = u.pop();
        _redo = _redo.concat([_committed]);
        _undo = u;
        _fill(previous);
        _committed = previous;
        canUndo = _undo.length > 0;
        canRedo = true;
        _selectionChanged();
        revision++;
        edited();
    }

    function redo() {
        if (_redo.length === 0)
            return;
        var r = _redo.slice();
        var next = r.pop();
        _undo = _undo.concat([_committed]);
        _redo = r;
        _fill(next);
        _committed = next;
        canUndo = true;
        canRedo = _redo.length > 0;
        _selectionChanged();
        revision++;
        edited();
    }

    // --- Sélection ------------------------------------------------------------------------------

    function _selectionChanged() {
        var n = 0, tr = 0, co = 0, cr = 0, lab = -1, labCount = 0;
        for (var i = 0; i < items.count; ++i) {
            var it = items.get(i);
            if (!it.sel)
                continue;
            n++;
            if (isRotatable(it.kind)) tr++;
            if (isColorable(it.kind)) co++;
            if (hasLabel(it.kind)) { lab = i; labCount++; }
        }
        for (var j = 0; j < arrows.length; ++j) {
            if (!arrows[j].sel)
                continue;
            n++;
            if (arrows[j].style === "cross")
                cr++;
        }
        selCount = n;
        selTransformable = tr;
        selColorable = co;
        selCrosses = cr;
        selLabelIndex = (n === 1 && labCount === 1) ? lab : -1;
        revision++;
    }

    function clearSelection() {
        var changed = false;
        for (var i = 0; i < items.count; ++i) {
            if (items.get(i).sel) {
                items.setProperty(i, "sel", false);
                changed = true;
            }
        }
        if (arrows.some(function (a) { return a.sel; })) {
            arrows = arrows.map(function (a) { a.sel = false; return a; });
            changed = true;
        }
        if (changed)
            _selectionChanged();
    }

    function selectItem(index, additive) {
        if (!additive)
            clearSelection();
        items.setProperty(index, "sel", additive ? !items.get(index).sel : true);
        _selectionChanged();
    }

    function selectArrow(index, additive) {
        if (!additive)
            clearSelection();
        var list = arrows.slice();
        list[index].sel = additive ? !list[index].sel : true;
        arrows = list;
        _selectionChanged();
    }

    // --- Actions ------------------------------------------------------------------------------

    function addItem(kind, nx, ny, label) {
        var rot = 0;
        if (kind === "goal" && pitch === "half")
            rot = 90;                       // en demi-terrain, la ligne de but est horizontale (repère habituel)
        clearSelection();
        items.append({ kind: kind, nx: Geo.clamp(nx, 0, 1), ny: Geo.clamp(ny, 0, 1), label: label || "",
                       rot: rot, scl: 1, color: "", sel: false });
        commit();
        return items.count - 1;
    }

    function nextPlayerNumber() {
        var list = [];
        for (var i = 0; i < items.count; ++i)
            list.push({ kind: items.get(i).kind, label: items.get(i).label });
        return Geo.nextFreePlayerNumber(list);
    }

    function addArrow(style, x1, y1, x2, y2) {
        clearSelection();
        var list = arrows.slice();
        list.push({ style: style, x1: x1, y1: y1, x2: x2, y2: y2,
                    curve: style === "cross" ? defaultCurve : 0, sel: false });
        arrows = list;
        commit();
    }

    // Déplacement : beginMove() mémorise les positions, moveBy() suit le doigt, commit() valide.
    function beginMove() {
        var start = [];
        for (var i = 0; i < items.count; ++i) {
            if (items.get(i).sel)
                start.push({ i: i, nx: items.get(i).nx, ny: items.get(i).ny });
        }
        _moveStart = start;
    }

    function moveBy(dnx, dny) {
        for (var k = 0; k < _moveStart.length; ++k) {
            var s = _moveStart[k];
            items.setProperty(s.i, "nx", Geo.clamp(s.nx + dnx, 0, 1));
            items.setProperty(s.i, "ny", Geo.clamp(s.ny + dny, 0, 1));
        }
    }

    function deleteSelected() {
        if (selCount === 0)
            return;
        for (var i = items.count - 1; i >= 0; --i) {
            if (items.get(i).sel)
                items.remove(i);
        }
        arrows = arrows.filter(function (a) { return !a.sel; });
        _selectionChanged();
        commit();
    }

    function rotateSelected(degrees) {
        for (var i = 0; i < items.count; ++i) {
            var it = items.get(i);
            if (it.sel && isRotatable(it.kind))
                items.setProperty(i, "rot", (((it.rot + degrees) % 360) + 360) % 360);
        }
        commit();
    }

    function scaleSelected(factor) {
        for (var i = 0; i < items.count; ++i) {
            var it = items.get(i);
            if (it.sel && isRotatable(it.kind))
                items.setProperty(i, "scl", Geo.clamp(Math.round(it.scl * factor * 100) / 100, minScale, maxScale));
        }
        commit();
    }

    function colorSelected(color) {
        for (var i = 0; i < items.count; ++i) {
            var it = items.get(i);
            if (it.sel && isColorable(it.kind))
                items.setProperty(i, "color", color);
        }
        commit();
    }

    function flipSelectedCurves() {
        arrows = arrows.map(function (a) {
            if (a.sel && a.style === "cross")
                a.curve = -a.curve;
            return a;
        });
        commit();
    }

    function setLabel(index, text) {
        if (index < 0 || index >= items.count)
            return;
        var kind = items.get(index).kind;
        var t = text.trim();
        if (kind === "text") {
            t = t.substring(0, 300);
            if (t === "")
                return;
        } else {
            t = t.substring(0, 3);
        }
        items.setProperty(index, "label", t);
        commit();
    }

    function clearAll() {
        items.clear();
        arrows = [];
        _selectionChanged();
        commit();
    }

    function setPitch(p) {
        if (pitch === p)
            return;
        pitch = p;
        commit();
    }

    // Remplace les joueurs (et les adversaires si demandé) par une formation ; terrain entier seulement.
    // formationJson : {"items": [...]} fourni par Backend.formationJson.
    function applyFormation(formationJson, withOpponents) {
        if (pitch !== "full")
            return false;
        var data;
        try {
            data = JSON.parse(formationJson);
        } catch (e) {
            return false;
        }
        clearSelection();
        for (var i = items.count - 1; i >= 0; --i) {
            var k = items.get(i).kind;
            if (k === "player" || (withOpponents && k === "opponent"))
                items.remove(i);
        }
        var list = data.items || [];
        for (var j = 0; j < list.length; ++j) {
            items.append({ kind: list[j].kind, nx: list[j].x, ny: list[j].y, label: list[j].label || "",
                           rot: 0, scl: 1, color: "", sel: false });
        }
        commit();
        return true;
    }
}
