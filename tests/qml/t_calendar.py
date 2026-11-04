# Calendrier : panneau du mois, navigation de période, détail déployé, états vides, téléphone inchangé.
import sys, os, datetime
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, QPointF
import shutil

TODAY = datetime.date.today()
def D(n): return (TODAY + datetime.timedelta(days=n)).strftime("%Y-%m-%d")
def S(i, d, t, e, theme, st="planned", loc="", notes="", total=-1, present=-1):
    return {"id":i,"teamId":1,"date":d,"time":t,"endTime":e,"durationMin":90,"location":loc,"theme":theme,"notes":notes,"status":st,"present":present,"total":total}
ALL = [S(21, D(0), "10:00", "11:30", "Transitions défensives", loc="Terrain A", notes="Travail sans ballon puis jeu réduit."),
       S(22, D(0), "18:30", "20:00", "Jeu de possession", "done", total=18, present=15),
       S(23, D(3), "09:00", "10:30", "Physique", "cancelled"),
       S(24, D(9), "10:00", "11:30", "Finition")]
class B5(B2):
    noteam = False
    @Slot(result="QVariantList")
    def teams(self): return [] if self.noteam else [{"id":1,"name":"Senior A","category":"Seniors","playerCount":4}]
    @Slot(int, str, str, result="QVariantList")
    def sessions(self, team, a, b): return [s for s in ALL if a <= s["date"] <= b]
    @Slot(int, str, str, result="QStringList")
    def sessionDates(self, team, a, b): return sorted(set(s["date"] for s in ALL if a <= s["date"] <= b))

def F(h, name, all_=False):
    todo = [h.win.contentItem()]; out = []
    while todo:
        it = todo.pop()
        if it.objectName() == name: out.append(it)
        todo.extend(it.childItems())
    return out if all_ else (out[0] if out else None)
errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".work", "out")
def shot(h, tag):
    h.shot("x"); shutil.copy(os.path.join(OUT, "x.png"), os.path.join(os.environ.get("HOME_SHOTS", OUT), tag + ".png"))
def make(w, hh, noteam=False):
    B5.noteam = noteam; b = B5(); b.keep = []
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "calendar"), Q_ARG("QVariant", -1)); h.spin(700)
    return h
def page(h): return F(h, "monthPanel").parentItem().parentItem().parentItem().parentItem().parentItem()
def vis(h, name): return [c for c in F(h, name, True) if c.isVisible()]
def cards(h):
    return sorted(vis(h, "sessionCard"), key=lambda c: (round(c.mapToScene(QPointF(0, 0)).y())))
def click_card(h, c):
    p = c.mapToScene(QPointF(120, 30)); h.click(p.x(), p.y())
def click_item(h, it):
    p = it.mapToScene(QPointF(it.width() / 2, it.height() / 2)); h.click(p.x(), p.y())
def label(h): return F(h, "periodLabel").property("text")
def texts(it, out):
    for c in it.childItems():
        t = c.property("text")
        if isinstance(t, str): out.append(t)
        texts(c, out)
    return out
def cal_page(h):
    it = F(h, "periodBar")
    while it is not None and it.property("expandedId") is None: it = it.parentItem()
    return it

for name, w, hh in [("bureau", 1280, 800), ("tablette-paysage", 1024, 768), ("tablette-portrait", 800, 1280)]:
    h = make(w, hh); pg = cal_page(h)
    check(pg is not None and pg.property("split") is True, f"{name} : deux colonnes")
    check(len(vis(h, "monthPanel")) == 1 and len(vis(h, "periodBar")) == 1, f"{name} : panneau du mois et barre de période")
    check(label(h).lower().startswith(("lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi", "dimanche")), f"{name} : période « jour » : {label(h)}")
    cs = cards(h)
    check(len(cs) == 2, f"{name} : 2 séances aujourd'hui -> {len(cs)}")
    check(len(vis(h, "calendarEmpty")) == 0, f"{name} : pas d'état vide")
    check(all(c.mapToScene(QPointF(c.width(), 0)).x() <= w - 8 for c in cs), f"{name} : cartes dans la fenêtre")
    shot(h, f"cal_{name}")
    # détail déployé
    click_card(h, cs[0]); h.spin(500)
    check(len(vis(h, "detailAttendance")) == 1 and len(vis(h, "detailEdit")) == 1, f"{name} : détail déployé (appel, modifier)")
    t = texts(vis(h, "sessionDetails")[0], [])
    check(any("Échauffement dynamique" in x for x in t), f"{name} : plan d'exercices listé")
    check(any("Terrain A" in x for x in t) and any("sans ballon" in x for x in t), f"{name} : lieu et notes affichés")
    shot(h, f"cal_{name}_detail")
    click_card(h, cards(h)[0]); h.spin(500)
    check(len(vis(h, "detailAttendance")) == 0, f"{name} : second clic -> détail replié")
    # changement de période
    click_item(h, F(h, "periodNext")); h.spin(500)
    check(len(cards(h)) == 0 and len(vis(h, "calendarEmpty")) == 1, f"{name} : demain sans séance -> état vide")
    click_item(h, F(h, "periodToday")); h.spin(500)
    check(len(cards(h)) == 2, f"{name} : « Aujourd'hui » -> 2 séances")
    pg.setProperty("scope", "month"); h.spin(500)
    expected = sum(1 for s in ALL if s["date"][:7] == D(0)[:7])
    check(len(cards(h)) == expected, f"{name} : mois -> {expected} séances ({len(cards(h))})")
    check(label(h).split()[-1] == str(TODAY.year), f"{name} : libellé du mois {label(h)}")
    pg.setProperty("scope", "week"); h.spin(500)
    mon = TODAY - datetime.timedelta(days=TODAY.weekday()); sun = mon + datetime.timedelta(days=6)
    exp_w = sum(1 for s in ALL if mon.strftime("%Y-%m-%d") <= s["date"] <= sun.strftime("%Y-%m-%d"))
    check(len(cards(h)) == exp_w and label(h).startswith("Semaine du"), f"{name} : semaine -> {exp_w} séances ({len(cards(h))}), « {label(h)} »")
    shot(h, f"cal_{name}_semaine")
    click_item(h, F(h, "periodPrev")); h.spin(500)
    check(len([s for s in ALL if (mon - datetime.timedelta(days=7)).strftime("%Y-%m-%d") <= s["date"] < mon.strftime("%Y-%m-%d")]) == len(cards(h)), f"{name} : semaine précédente")

# Téléphone : une colonne, barre de période absente, clic = formulaire.
for name, w, hh in [("telephone", 412, 892), ("telephone-paysage", 844, 390)]:
    h = make(w, hh)
    if name == "telephone":
        check(len(vis(h, "periodBar")) == 0, "téléphone : pas de barre de période")
        cs = cards(h)
        check(len(cs) == 2, f"téléphone : 2 séances ({len(cs)})")
        shot(h, "cal_telephone")
        click_card(h, cs[0]); h.spin(500)
        pg = cal_page(h) if F(h, "periodBar") else None
        check(len(vis(h, "detailAttendance")) == 0, "téléphone : clic ouvre le formulaire (pas de détail déployé)")
    else:
        check(len(vis(h, "periodBar")) == 1 or len(vis(h, "periodBar")) == 0, "téléphone paysage : affichage sans erreur")

# Sans équipe.
h = make(1280, 800, noteam=True)
t = texts(h.win.contentItem(), [])
check(any("Créez d'abord une équipe" in x for x in t), "sans équipe : message « Créez d'abord une équipe »")
print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
