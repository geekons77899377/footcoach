# Accueil : composition deux colonnes (bureau/tablette paysage), colonne unique (téléphone), données latérales.
import sys, os, datetime
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG
import shutil

def F(h, name):
    todo = [h.win.contentItem()]
    while todo:
        it = todo.pop()
        if it.objectName() == name: return it
        todo.extend(it.childItems())
    return None

D = lambda n: (now + datetime.timedelta(days=n)).strftime("%Y-%m-%d")
class B3(B2):
    side = "data"
    @Slot(int, str, str, result="QVariantList")
    def sessions(self, team, a, b):
        base = super().sessions(team, a, b) if self.side == "data" else []
        if self.side != "data": return []
        base = [dict(s) for s in base]
        base.append({"id":8,"teamId":1,"date":D(2),"time":"18:30","endTime":"20:00","durationMin":90,"location":"Terrain B","theme":"Jeu de possession","notes":"","status":"planned","present":-1,"total":-1})
        base.append({"id":9,"teamId":1,"date":D(4),"time":"09:00","endTime":"10:30","durationMin":90,"location":"","theme":"","notes":"","status":"cancelled","present":-1,"total":-1})
        base.append({"id":10,"teamId":1,"date":D(6),"time":"10:00","endTime":"11:30","durationMin":90,"location":"","theme":"Physique","notes":"","status":"planned","present":-1,"total":-1})
        return base
    @Slot(int, str, str, result="QVariantMap")
    def attendanceStats(self, team, a, b):
        if self.side != "data": return {"sessions": [], "players": []}
        return {"sessions": [{"id":1,"date":D(-3)},{"id":2,"date":D(-8)}],
                "players": [{"id":1,"name":"A","rate":0.9},{"id":2,"name":"B","rate":0.7},{"id":3,"name":"C","rate":-1}]}

errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".work", "out")
def make(w, hh, side="data"):
    B3.side = side
    b = B3(); b.keep = []; B2.mode = "now"
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "home"), Q_ARG("QVariant", -1)); h.spin(600)
    return h, b
def shot(h, tag):
    h.shot("x"); shutil.copy(os.path.join(OUT, "x.png"), os.path.join(os.environ.get("HOME_SHOTS", OUT), tag + ".png"))

# Bureau 1280x800 : deux colonnes.
h, b = make(1280, 800)
up, pr, lv = F(h, "upcomingCard"), F(h, "presenceCard"), F(h, "live")
check(up is not None and pr is not None, "bureau : cartes latérales présentes")
check(up.property("visible") and pr.property("visible"), "bureau : cartes visibles")
pu = up.mapToScene(QPointF(0, 0)); pl = lv.mapToScene(QPointF(0, 0))
check(pu.x() > pl.x() + lv.width(), f"bureau : colonne latérale à droite de la séance du jour ({pu.x():.0f} > {pl.x()+lv.width():.0f})")
check(abs(pu.y() - pl.y()) < 2, "bureau : colonnes alignées en haut")
check(pu.x() + up.width() <= 1280 - 16, "bureau : colonne latérale dans la fenêtre")
shot(h, "home_1280")

# Contenu : 2 séances à venir (l'annulée est exclue), 2 tuiles de présence.
txt = []
def texts(it):
    for c in it.childItems():
        t = c.property("text")
        if isinstance(t, str): txt.append(t)
        texts(c)
texts(up); texts(pr)
check("Jeu de possession" in txt and "Physique" in txt, "séances à venir affichées")
check(not any("09:00" in t for t in txt), "séance annulée exclue")
check("80 %" in txt, f"taux moyen = (0,9+0,7)/2 = 80 % (lignes: {[t for t in txt if '%' in t]})")
check("2" in txt, "nombre de séances comptées = 2")

# Tablette portrait 800x1280 : une colonne, bloc latéral dessous.
h, b = make(800, 1280)
up, lv = F(h, "upcomingCard"), F(h, "live")
check(up.property("visible"), "tablette portrait : bloc visible")
pu = up.mapToScene(QPointF(0, 0)); pl = lv.mapToScene(QPointF(0, 0))
check(pu.y() > pl.y() + lv.height() - 1 or pu.x() > pl.x() + lv.width(), "tablette portrait : sous ou à côté de la séance")
shot(h, "home_800")

# Tablette paysage 1024x768 : deux colonnes.
h, b = make(1024, 768)
up, lv = F(h, "upcomingCard"), F(h, "live")
pu = up.mapToScene(QPointF(0, 0)); pl = lv.mapToScene(QPointF(0, 0))
check(pu.x() > pl.x() + lv.width(), "tablette paysage : deux colonnes")
shot(h, "home_1024")

# Téléphone 412x892 : colonne unique, aucune carte latérale.
h, b = make(412, 892)
up = F(h, "upcomingCard")
check(up is None or not up.property("visible"), "téléphone : colonne latérale masquée")
shot(h, "home_412")

# Téléphone paysage 844x390 : colonne unique, pas de carte latérale.
h, b = make(844, 390)
up = F(h, "upcomingCard")
check(up is None or not up.property("visible"), "téléphone paysage : colonne latérale masquée")

# Sans séance ni données : états vides.
h, b = make(1280, 800, side="none")
up = F(h, "upcomingCard"); pr = F(h, "presenceCard")
txt = []; texts(up); texts(pr)
check("—" in txt, "état vide : présence « — »")
check(any("Aucune séance" in t for t in txt), "état vide : message sans séance")

print(f"\n{len(errs)} échec(s)")
sys.exit(1 if errs else 0)
