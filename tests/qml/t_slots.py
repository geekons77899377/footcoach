# Créneaux réguliers : liste, création, modification en bloc (aperçu, confirmation), déprogrammation (choix de l'étendue),
# accès depuis une séance, bouton retour. Backend simulé : le C++ réel est testé par tst_slots.
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
_src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_calendar.py")).read()
exec(_src.split("for name, w, hh in")[0])
from PySide6.QtCore import QObject, Slot, QMetaObject, Q_ARG, QPointF

SLOTS = [{"id": 7, "teamId": 1, "weekdays": [2, 4], "time": "18:30", "endTime": "20:00", "durationMin": 90,
          "location": "Stade municipal", "theme": "Technique", "from": D(-30), "to": D(200),
          "total": 60, "upcoming": 52, "withAttendance": 3},
         {"id": 8, "teamId": 1, "weekdays": [3], "time": "14:00", "endTime": "15:30", "durationMin": 90,
          "location": "", "theme": "", "from": D(-10), "to": D(-1), "total": 2, "upcoming": 0, "withAttendance": 0}]
for s in ALL:
    s["slotId"] = 7 if s["id"] in (21, 24) else -1

class B6(B5):
    empty = False
    def __init__(self):
        super().__init__(); self.calls = []
    @Slot(int, result="QVariantList")
    def trainingSlots(self, team): return [] if B6.empty else SLOTS
    @Slot(int, result="QVariantMap")
    def trainingSlot(self, i):
        for s in SLOTS:
            if s["id"] == i: return dict(s, ok=True, error="")
        return {"ok": False, "error": "Créneau introuvable."}
    @Slot("QVariantMap", result="QVariantMap")
    def previewRecurrence(self, rule):
        n = len(rule.get("weekdays", [])) * 10
        return {"ok": n > 0, "count": n, "clashes": 0, "error": "" if n else "Cochez au moins un jour."}
    @Slot(int, "QVariantMap", result="QVariantMap")
    def previewSlotUpdate(self, i, rule):
        self.calls.append(("preview", i, rule.get("time")))
        return {"ok": True, "error": "", "updated": 48, "added": 1, "removed": 2, "removedWithPlan": 1, "kept": 3, "clashes": 0}
    @Slot(int, "QVariantMap", bool, result="QVariantMap")
    def updateSlot(self, i, rule, force):
        self.calls.append(("update", i, rule.get("time"), rule.get("weekdays"), force))
        return {"ok": True, "error": ""}
    @Slot(int, result="QVariantMap")
    def previewSlotDelete(self, i):
        return {"ok": True, "error": "", "upcoming": 52, "past": 5, "kept": 3, "withPlan": 1, "withPlanPast": 0}
    @Slot(int, bool, result="QVariantMap")
    def deleteSlot(self, i, past):
        self.calls.append(("delete", i, past))
        return {"ok": True, "error": "", "removed": 57 if past else 52, "kept": 3}

def make6(w, hh):
    b = B6(); b.keep = []
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "calendar"), Q_ARG("QVariant", -1)); h.spin(700)
    return h, b

def calendar(h):
    it = F(h, "monthPanel")
    while it is not None and it.property("recurrenceBack") is None: it = it.parentItem()
    return it

def objs(h, pred):
    out, todo = [], [h.win]
    while todo:
        o = todo.pop()
        if pred(o): out.append(o)
        todo.extend(o.children())
    return out

def open_dialog(h):
    ds = [o for o in objs(h, lambda o: o.property("okText") is not None and o.property("message") is not None)
          if o.property("opened") or o.property("visible")]
    return ds[0] if ds else None

def button(h, text):
    bs = [o for o in objs(h, lambda o: o.property("text") == text and o.property("kind") is not None) if o.property("visible")]
    return bs[0] if bs else None

def press(h, text):
    b = button(h, text)
    if b is None: return False
    p = b.mapToScene(QPointF(b.width() / 2, b.height() / 2))
    if 0 <= p.y() <= h.win.height() - 4:
        h.click(p.x(), p.y())
    else:                                   # sous le bas de l'écran (formulaire défilant) : signal direct
        QMetaObject.invokeMethod(b, "clicked"); h.spin(350)
    return True

def mode(h): return calendar(h).property("mode")
def accept(h, d):
    QMetaObject.invokeMethod(d, "accepted", Q_ARG("QString", "")); QMetaObject.invokeMethod(d, "close"); h.spin(500)
def slot_cards(h): return sorted(vis(h, "slotCard"), key=lambda c: round(c.mapToScene(QPointF(0, 0)).y()))
def jsval(v): return v.toVariant() if hasattr(v, "toVariant") else v

for name, w, hh in [("telephone", 412, 892), ("bureau", 1280, 800)]:
    h, b = make6(w, hh); cal = calendar(h)
    check(cal is not None, f"{name} : page calendrier trouvée")
    QMetaObject.invokeMethod(cal, "openSlots"); h.spin(500)
    check(mode(h) == "slots", f"{name} : écran des créneaux ouvert")
    cs = slot_cards(h)
    check(len(cs) == 2, f"{name} : 2 créneaux listés ({len(cs)})")
    t = texts(cs[0], []) if cs else []
    check(any("Mardi, jeudi · 18:30 – 20:00" in x for x in t), f"{name} : jours et horaires : {t[:1]}")
    check(any("52 séances à venir · 3 avec appel" in x for x in t), f"{name} : compteurs affichés")
    check(any("Aucune séance à venir" in x for x in texts(cs[1], [])) if len(cs) > 1 else False, f"{name} : créneau terminé")
    check(all(c.mapToScene(QPointF(c.width(), 0)).x() <= w - 8 for c in cs), f"{name} : cartes dans la fenêtre")
    shot(h, f"slots_{name}_liste")

    # Modification : aperçu, confirmation des suppressions, appel au backend.
    click_item(h, cs[0]); h.spin(500)
    check(mode(h) == "recurrence", f"{name} : clic sur un créneau -> formulaire")
    pv = vis(h, "slotPreview")
    ptxt = pv[0].property("text") if pv else ""
    check("48 séances modifiées · 1 séance ajoutée · 2 séances supprimées (dont 1 avec un plan d'exercices)" in ptxt, f"{name} : aperçu des changements : {ptxt!r}")
    check("3 séances gardées telles quelles" in ptxt, f"{name} : séances avec appel signalées")
    check(button(h, "Enregistrer les modifications") is not None and button(h, "Déprogrammer le créneau") is not None, f"{name} : boutons modifier et déprogrammer")
    shot(h, f"slots_{name}_modifier")
    press(h, "Enregistrer les modifications"); h.spin(300)
    d = open_dialog(h)
    check(d is not None and "2 séances à venir seront supprimées, dont 1 avec un plan d'exercices" in (d.property("message") or ""), f"{name} : confirmation avant suppression : {d.property('message') if d else None!r}")
    check(not any(c[0] == "update" for c in b.calls), f"{name} : rien d'écrit avant confirmation")
    shot(h, f"slots_{name}_confirmation")
    if d: accept(h, d)
    ups = [c for c in b.calls if c[0] == "update"]
    check(len(ups) == 1 and ups[0][1] == 7 and ups[0][2] == "18:30" and list(ups[0][3]) == [2, 4], f"{name} : updateSlot(7, règle) appelé : {ups}")
    check(mode(h) == "slots", f"{name} : retour à la liste des créneaux après modification")

    # Déprogrammation : choix de l'étendue puis confirmation.
    click_item(h, slot_cards(h)[0]); h.spin(500)
    press(h, "Déprogrammer le créneau"); h.spin(400)
    sheets = [o for o in objs(h, lambda o: o.property("title") == "Que faut-il supprimer ?")]
    model = jsval(sheets[0].property("model")) if sheets else []
    labels = [m["text"] for m in model] if model else []
    check(labels == ["Séances à venir (52)", "À venir et passées sans appel (57)"], f"{name} : choix de l'étendue : {labels}")
    shot(h, f"slots_{name}_etendue")
    if sheets: QMetaObject.invokeMethod(sheets[0], "chosen", Q_ARG("QString", "upcoming")); QMetaObject.invokeMethod(sheets[0], "close"); h.spin(400)
    d = open_dialog(h)
    msg = d.property("message") if d else ""
    check("52 séances seront supprimées, dont 1 avec un plan d'exercices." in msg and "8 séances resteront dans le calendrier" in msg, f"{name} : message de déprogrammation : {msg!r}")
    if d: accept(h, d)
    dels = [c for c in b.calls if c[0] == "delete"]
    check(dels == [("delete", 7, False)], f"{name} : deleteSlot(7, à venir seulement) : {dels}")
    check(mode(h) == "slots", f"{name} : retour à la liste après déprogrammation")

    # Bouton retour : formulaire -> liste -> calendrier.
    click_item(h, slot_cards(h)[0]); h.spin(400)
    QMetaObject.invokeMethod(cal, "goBack"); h.spin(300)
    check(mode(h) == "slots", f"{name} : retour depuis le formulaire -> liste")
    QMetaObject.invokeMethod(cal, "goBack"); h.spin(300)
    check(mode(h) == "browse", f"{name} : retour depuis la liste -> calendrier")

    # Nouveau créneau depuis la liste.
    QMetaObject.invokeMethod(cal, "openSlots"); h.spin(300)
    press(h, "Nouveau créneau") if button(h, "Nouveau créneau") else QMetaObject.invokeMethod(cal, "newRecurrence")
    h.spin(400)
    if mode(h) != "recurrence":
        QMetaObject.invokeMethod(cal, "newRecurrence"); h.spin(400)
    check(mode(h) == "recurrence" and button(h, "Créer les séances") is not None and button(h, "Déprogrammer le créneau") is None, f"{name} : nouveau créneau : création seule")
    QMetaObject.invokeMethod(cal, "goBack"); h.spin(300)
    check(mode(h) == "slots", f"{name} : retour du nouveau créneau -> liste")

# Accès depuis une séance du calendrier (menu de la séance, téléphone).
h, b = make6(412, 892); cal = calendar(h)
sheet = [o for o in objs(h, lambda o: "ActionSheet" in o.metaObject().className() and o.metaObject().indexOfMethod("openFor(QVariant)") >= 0)]
check(len(sheet) == 1, "menu de séance trouvé")
if sheet:
    QMetaObject.invokeMethod(sheet[0], "openFor", Q_ARG("QVariant", ALL[0])); h.spin(300)
    ids = [m["id"] for m in jsval(sheet[0].property("model"))]
    check("slot" in ids, f"séance d'un créneau : « Modifier le créneau régulier » proposé ({ids})")
    QMetaObject.invokeMethod(sheet[0], "chosen", Q_ARG("QString", "slot")); h.spin(500)
    check(mode(h) == "recurrence" and cal.property("recurrenceBack") == "browse", "ouvre le créneau, retour vers le calendrier")
    QMetaObject.invokeMethod(cal, "goBack"); h.spin(300)
    QMetaObject.invokeMethod(sheet[0], "openFor", Q_ARG("QVariant", ALL[1])); h.spin(300)
    check("slot" not in [m["id"] for m in jsval(sheet[0].property("model"))], "séance seule : pas d'entrée créneau")

# Équipe sans créneau : état vide.
B6.empty = True
h, b = make6(412, 892); cal = calendar(h)
QMetaObject.invokeMethod(cal, "openSlots"); h.spin(400)
t = texts(h.win.contentItem(), [])
check(any("Aucun créneau régulier" in x for x in t), "état vide affiché")
shot(h, "slots_telephone_vide")
warn = [x for x in h.warnings if "SlotsPanel" in x or "RecurrenceForm" in x]
check(not warn, f"aucun avertissement QML : {warn[:3]}")
print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
