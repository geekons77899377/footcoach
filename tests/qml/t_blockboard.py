"""Étape 2 : terrain tactique d'un bloc dans le plan de séance (PlanEditor.qml + BoardEditor.qml en mode bloc).

Le Backend est SIMULÉ (comme dans les autres tests de ce dossier) : il reproduit le contrat de Backend.cpp
(plan() : boardId / boardReadable / boardJson ; createBlockBoard ; board / saveBoard / deleteBoard ; boards() sans les
terrains de bloc). Ce test valide la logique QML ; le comportement du vrai Backend est testé par tests/tst_backend.cpp
(blockBoardThroughTheBackend) et tests/tst_board.cpp. Il ne remplace pas un essai sur appareil.
"""
exec(open('t_live.py').read().split("errs=[]")[0])
from PySide6.QtCore import Qt, Q_RETURN_ARG, Signal

errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)

PLAYERS_HALF = json.dumps({"version": 1, "pitch": "half", "items": [
    {"kind": "player", "x": 0.3, "y": 0.4, "label": "1"}, {"kind": "player", "x": 0.6, "y": 0.5, "label": "2"}], "arrows": []})

class B3(B2):
    dataReplaced = Signal()
    status = "planned"
    def __init__(self):
        super().__init__()
        self.blocks = {}          # itemId -> id du terrain
        self.calls = []
        self.reset_blocks()
    def reset_blocks(self):
        self.blocks = {101: self.add("Rondo 4v2 Pressing", PLAYERS_HALF), 102: self.add("Circuit de passes rapide", "{pas du json")}
    @Slot(int, str, str, result="QVariantList")
    def sessions(self, team, a, b):
        s = super().sessions(team, a, b)
        for x in s: x["status"] = self.status
        return s
    @Slot(int, result="QVariantMap")
    def plan(self, sid):
        rows = []
        for i, (t, c, d) in enumerate(EX[:4]):
            item = 100 + i
            bid = self.blocks.get(item, -1)
            j = self._boards[bid]["json"] if bid in self._boards else ""
            try:
                json.loads(j); ok = bid > 0
            except Exception:
                ok = False
            rows.append({"itemId": item, "position": i + 1, "start": "", "end": "", "durationMin": d, "notes": "",
                         "exerciseId": 10 + i, "title": t, "category": "technical", "categoryLabel": c, "objective": "",
                         "equipment": "", "description": "", "archived": False,
                         "boardId": bid if bid in self._boards else -1, "boardReadable": ok, "boardJson": j if ok else ""})
        return {"ok": True, "error": "", "entries": rows, "totalMin": 80, "sessionMin": 90, "equipment": []}
    @Slot(int, result="QVariantMap")
    def createBlockBoard(self, item):
        self.calls.append(("createBlockBoard", item))
        if self.status == "cancelled":
            return {"ok": False, "id": item, "error": "Cette séance est annulée."}
        if item in self.blocks and self.blocks[item] in self._boards:
            return {"ok": True, "id": item, "error": "", "boardId": self.blocks[item], "created": False}
        title = dict((100 + i, t) for i, (t, c, d) in enumerate(EX[:4]))[item]
        self.blocks[item] = self.add(title, json.dumps({"version": 1, "pitch": "full", "items": [], "arrows": []}))
        return {"ok": True, "id": item, "error": "", "boardId": self.blocks[item], "created": True}
    @Slot(result="QVariantList")
    def boards(self):
        return [b for b in super().boards() if b["id"] not in self.blocks.values()]

def run3(w=412, hh=892, status="planned"):
    B3.status = status
    b = B3(); b.keep = []
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "sessions"), Q_ARG("QVariant", -1)); h.spin(500)
    return h, b

def F(h, name):
    """Cherche un élément par objectName dans l'arbre VISUEL (les délégués d'un Repeater n'y sont pas rattachés en QObject)."""
    todo = [h.win.contentItem()]
    while todo:
        it = todo.pop()
        if it.objectName() == name: return it
        todo.extend(it.childItems())
    return None
def ctr(h, name):
    it = F(h, name); p = it.mapToScene(QPointF(it.width() / 2, it.height() / 2)); return p.x(), p.y()
def tap(h, name): x, y = ctr(h, name); h.click(x, y)
def V(x): return x.toVariant() if hasattr(x, "toVariant") else x
def find_obj(h, prop, val=None):
    for o in h.win.findChildren(QObject):
        if o.property(prop) is not None and (val is None or o.property(prop) == val):
            return o
def page(h): return [o for o in h.win.findChildren(QObject) if o.property("editing") is not None and o.property("scope") is not None and o.property("listWidth") == 400][0]
def plan(h): return find_obj(h, "planData")
def open_plan(h):
    s = {"id": 7, "teamId": 1, "date": datetime.datetime.now().strftime("%Y-%m-%d"), "time": "18:30", "endTime": "20:00",
         "durationMin": 90, "location": "", "theme": "Transitions", "notes": "", "status": B3.status, "present": -1, "total": -1}
    QMetaObject.invokeMethod(page(h), "openSession", Q_ARG("QVariant", s)); h.spin(500)
def back(h):
    r = QMetaObject.invokeMethod(h.win, "handleBack", Qt.DirectConnection, Q_RETURN_ARG("QVariant")); h.spin(350); return r
def sc(h): return F(h, "planScroll")
def scroll_to(h, name, margin=140):
    it = F(h, name); f = sc(h)
    y = it.mapToItem(f.property("contentItem"), QPointF(0, 0)).y()
    f.setProperty("contentY", max(0.0, min(y - margin, f.property("contentHeight") - f.property("height")))); h.spin(250)
def vis(h, name):
    o = F(h, name); return o is not None and bool(o.property("visible"))
def editor_of(h): return F(h, "boardLoader").property("item")
def tapname(h, name):
    scroll_to(h, name); tap(h, name)
def inside(h, name, w, hh):
    it = F(h, name); p = it.mapToScene(QPointF(0, 0)); return p.x() >= 0 and p.y() >= 0 and p.x() + it.width() <= w + 1 and p.y() + it.height() <= hh + 1 and it.width() > 40

# --- 1. États d'un bloc : sans terrain, avec terrain, terrain illisible -------------------------------------------
h, b = run3()
open_plan(h)
check(len(V(plan(h).property("planData"))["entries"]) == 4, "plan chargé : 4 blocs")
check(vis(h, "blockBoardEmpty_100") and not vis(h, "blockThumb_100"), "bloc sans terrain : état vide affiché, pas de miniature")
check(vis(h, "blockThumb_101") and not vis(h, "blockBoardEmpty_101"), "bloc avec terrain : miniature affichée")
check(vis(h, "blockBoardEmpty_102") and not vis(h, "blockThumb_102"), "terrain illisible : état d'alerte, pas de miniature")
check(F(h, "openBoard_100").property("text") == "Ajouter un terrain", "bloc sans terrain : bouton « Ajouter un terrain »")
check(F(h, "openBoard_101").property("text") == "Ouvrir le terrain", "bloc avec terrain : bouton « Ouvrir le terrain »")
check(b.calls == [], "aucun terrain créé à l'affichage (règle B)")
h.shot("blockboard_plan_portrait")

# --- 2. Ajouter un terrain : création à la demande, ouverture du bon schéma --------------------------------------
scroll_to(h, "openBoard_103"); y_before = sc(h).property("contentY")
check(y_before > 0, f"plan défilé avant l'ouverture (contentY={y_before:.0f})")
tap(h, "openBoard_103"); h.spin(500)
check(b.calls == [("createBlockBoard", 103)], "création appelée une seule fois, pour le bon bloc")
ed = editor_of(h)
check(plan(h).property("boardOpen") is True and ed is not None, "éditeur du terrain ouvert dans le plan")
new_id = b.blocks[103]
check(ed.property("boardId") == new_id, "l'éditeur ouvre le terrain créé pour ce bloc")
check(ed.property("blockMode") is True, "éditeur en mode bloc")
check(ed.property("contextTitle") == "Atelier 4 · Match à thème", "en-tête : « Atelier 4 · Match à thème »")
more = ed.findChild(QObject, "moreSheet"); ids = [m["id"] for m in V(more.property("model"))]
check("rename" not in ids and "duplicate" not in ids, f"menu du terrain : pas de Renommer ni Dupliquer ({ids})")
check("delete" in ids and [m["text"] for m in V(more.property("model")) if m["id"] == "delete"][0].startswith("Retirer"), "menu : « Retirer le terrain de ce bloc »")
check({"full", "half", "formation", "clear"} <= set(ids), "menu : terrain, formation et effacement conservés")
h.shot("blockboard_editor_portrait")

# --- 3. Modification, sauvegarde automatique, retour immédiat sans perte -----------------------------------------
bm = ed.findChild(QObject, "board")
saves0 = b.saves
QMetaObject.invokeMethod(bm, "addItem", Q_ARG("QVariant", "player"), Q_ARG("QVariant", 0.5), Q_ARG("QVariant", 0.5), Q_ARG("QVariant", "7")); h.spin(900)
n1 = len(json.loads(b._boards[new_id]["json"])["items"])
check(b.saves > saves0 and n1 == 1, f"sauvegarde automatique après 500 ms ({n1} élément)")
QMetaObject.invokeMethod(bm, "addItem", Q_ARG("QVariant", "opponent"), Q_ARG("QVariant", 0.7), Q_ARG("QVariant", 0.5), Q_ARG("QVariant", ""))
r = back(h)       # retour AVANT les 500 ms : l'enregistrement final doit tout de même avoir lieu
n2 = len(json.loads(b._boards[new_id]["json"])["items"])
check(r is True and plan(h).property("boardOpen") is False, "bouton retour : le terrain se ferme, on reste dans le plan")
check(n2 == 2, f"aucune perte au retour immédiat ({n2} éléments enregistrés)")
check(abs(sc(h).property("contentY") - y_before) < 3, f"position de défilement conservée ({sc(h).property('contentY'):.0f} / {y_before:.0f})")
check(vis(h, "blockThumb_103") and not vis(h, "blockBoardEmpty_103"), "miniature affichée au retour")
check(F(h, "openBoard_103").property("text") == "Ouvrir le terrain", "le bouton devient « Ouvrir le terrain »")
check(page(h).property("editing") is True, "le plan reste ouvert")
h.shot("blockboard_plan_after")

# --- 4. Réouverture du terrain puis du plan : persistance ------------------------------------------------------------
tap(h, "openBoard_103"); h.spin(500)
ed = editor_of(h)
check(ed.property("boardId") == new_id and b.calls.count(("createBlockBoard", 103)) == 1, "réouverture : même terrain, pas de nouvelle création")
shown = json.loads(QMetaObject.invokeMethod(ed.findChild(QObject, "board"), "toJson", Qt.DirectConnection, Q_RETURN_ARG("QVariant")))
check(len(shown["items"]) == 2, "terrain relu depuis la base et chargé dans l'éditeur avec ses 2 éléments")
check(back(h) is True and plan(h).property("boardOpen") is False, "retour au plan")
QMetaObject.invokeMethod(page(h), "closeEditor"); h.spin(300)
check(page(h).property("editing") is False, "plan fermé")
open_plan(h)
check(vis(h, "blockThumb_103"), "après fermeture et réouverture du plan : le terrain est retrouvé")
check(b.calls.count(("createBlockBoard", 103)) == 1, "aucune création superflue")

# --- 5. Ordre du bouton retour : plein écran, terrain, plan, accueil ----------------------------------------------
tapname(h, "openBoard_101"); h.spin(400)
ed = editor_of(h); check(ed.property("boardId") == b.blocks[101], "ouvre le terrain du bloc 2 (et pas un autre)")
ed.setProperty("fullscreen", True); h.spin(200)
check(back(h) is True and ed.property("fullscreen") is False and plan(h).property("boardOpen") is True, "retour 1 : quitte le plein écran, le terrain reste ouvert")
check(back(h) is True and plan(h).property("boardOpen") is False and page(h).property("editing") is True, "retour 2 : ferme le terrain, le plan reste")
check(back(h) is True and page(h).property("editing") is False, "retour 3 : ferme le plan")
check(back(h) is True and h.win.property("section") == "home", "retour 4 : accueil")

# --- 6. Retirer le terrain d'un bloc ------------------------------------------------------------------------------------------
QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "sessions"), Q_ARG("QVariant", -1)); h.spin(400)
open_plan(h)
tapname(h, "openBoard_101"); h.spin(400)
ed = editor_of(h)
QMetaObject.invokeMethod(ed, "deleteRequested"); h.spin(300)
btn = [o for o in h.win.findChildren(QObject) if o.property("text") == "Retirer" and o.property("visible")]
check(len(btn) >= 1, "confirmation « Retirer » affichée")
if btn:
    p = btn[0].mapToScene(QPointF(btn[0].width() / 2, btn[0].height() / 2)); h.click(p.x(), p.y()); h.spin(500)
check(plan(h).property("boardOpen") is False, "après confirmation : retour au plan")
check(101 not in b.blocks or b.blocks[101] not in b._boards, "terrain retiré de la base simulée")
check(vis(h, "blockBoardEmpty_101") and F(h, "openBoard_101").property("text") == "Ajouter un terrain", "le bloc 2 retrouve l'état « sans terrain »")
b.reset_blocks(); b.calls.clear()

# --- 7. Terrain illisible : ouvrable, non modifiable, retirable ----------------------------------------------------------
QMetaObject.invokeMethod(page(h), "closeEditor"); open_plan(h)
tapname(h, "openBoard_102"); h.spin(400)
ed = editor_of(h)
check(ed is not None and ed.property("editable") is False, "terrain illisible : ouvert en lecture seule (rien n'est écrasé)")
saves = b.saves; h.spin(800)
check(b.saves == saves and b._boards[b.blocks[102]]["json"] == "{pas du json", "contenu illisible intact")
back(h)

# --- 8. Séance annulée -----------------------------------------------------------------------------------------------------------------
h.shot("blockboard_before_cancel")
hc, bc = run3(status="cancelled"); open_plan(hc)
check(not vis(hc, "openBoard_100") and not vis(hc, "openBoard_101"), "séance annulée : aucun bouton d'ajout ni d'ouverture")
check(vis(hc, "blockThumb_101"), "séance annulée : la miniature reste visible")
scroll_to(hc, "blockThumb_101"); tap(hc, "blockThumb_101"); hc.spin(300)
check(plan(hc).property("boardOpen") is False and bc.calls == [], "séance annulée : toucher la miniature n'ouvre rien")

# --- 9. Données remplacées (import) pendant l'édition ---------------------------------------------------------------------------------
h9, b9 = run3(); open_plan(h9)
tapname(h9, "openBoard_101"); h9.spin(400)
e9 = editor_of(h9); bid = b9.blocks[101]
QMetaObject.invokeMethod(e9.findChild(QObject, "board"), "addItem", Q_ARG("QVariant", "ball"), Q_ARG("QVariant", 0.5), Q_ARG("QVariant", 0.5), Q_ARG("QVariant", ""))
before = b9._boards[bid]["json"]
b9.dataReplaced.emit(); h9.spin(900)
check(plan(h9).property("boardOpen") is False and page(h9).property("editing") is False, "import : le terrain et le plan se ferment")
check(b9._boards[bid]["json"] == before, "import : l'enregistrement en attente est abandonné (rien n'est écrit vers l'ancien terrain)")

# --- 10. Changer de séance avec un terrain ouvert (grand écran : la liste reste cliquable) ----------------------------------------
h10, b10 = run3(1280, 800); open_plan(h10)
tapname(h10, "openBoard_101"); h10.spin(400)
e10 = editor_of(h10); bid = b10.blocks[101]
QMetaObject.invokeMethod(e10.findChild(QObject, "board"), "addItem", Q_ARG("QVariant", "cone"), Q_ARG("QVariant", 0.2), Q_ARG("QVariant", 0.2), Q_ARG("QVariant", ""))
open_plan(h10)       # ré-ouvre une séance : le terrain ouvert doit être enregistré puis fermé
check(plan(h10).property("boardOpen") is False, "changement de séance : le terrain se ferme")
check(len(json.loads(b10._boards[bid]["json"])["items"]) == 3, "changement de séance : la modification en attente est enregistrée")

# --- 11. Cinq formats d'écran ---------------------------------------------------------------------------------------------------------
for name, w, hh in [("tel-portrait", 360, 640), ("tel-paysage", 844, 390), ("tab-portrait", 800, 1280), ("tab-paysage", 1280, 800), ("pc", 1600, 900)]:
    hf, bf = run3(w, hh); open_plan(hf)
    for nm in ("blockThumb_101", "openBoard_101", "blockBoardEmpty_100", "openBoard_100"):
        scroll_to(hf, nm, 100)
        check(inside(hf, nm, w, hh), f"{name}: {nm} entièrement visible dans la fenêtre")
    hf.shot(f"blockboard_plan_{name}")
    thumb = F(hf, "blockThumb_101"); check(thumb.width() >= 200 and thumb.height() >= 120, f"{name}: miniature lisible ({thumb.width():.0f}×{thumb.height():.0f})")
    tapname(hf, "openBoard_101"); hf.spin(450)
    e = editor_of(hf)
    pe = plan(hf)
    check(e is not None and abs(e.property("width") - pe.property("width")) < 1 and abs(e.property("height") - pe.property("height")) < 1 and e.property("height") > 300, f"{name}: l'éditeur occupe tout le plan ({e.property('width'):.0f}×{e.property('height'):.0f} dans {pe.property('width'):.0f}×{pe.property('height'):.0f})")
    hf.shot(f"blockboard_editor_{name}")
    check(back(hf) is True and plan(hf).property("boardOpen") is False, f"{name}: retour au plan")

print("avert:", [x for x in h.warnings if "Connections" not in x and "palette" not in x and "formationNames" not in x][:8])
print("ECHECS" if errs else "TOUT OK", errs)
