# Fiche joueur : photo (affichage, ajout, retrait, erreur) et numéro de licence, sur Backend simulé.
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_squad.py")).read().split("# Tablettes : grille")[0])
import base64
from PySide6.QtCore import QBuffer, QIODevice
from PySide6.QtGui import QImage, QColor

def jpeg_url(color):
    img = QImage(40, 40, QImage.Format_RGB32); img.fill(QColor(color))
    buf = QBuffer(); buf.open(QIODevice.WriteOnly); img.save(buf, "JPEG")
    return "data:image/jpeg;base64," + base64.b64encode(bytes(buf.data())).decode()
RED, BLUE = jpeg_url("#cc2222"), jpeg_url("#2222cc")

class B5(B4):
    saved = []
    prep_ok = True
    @Slot(int, bool, result="QVariantList")
    def players(self, team, inactive):
        out = []
        for t in PL:
            if t[6] or inactive:
                p = pm(t)
                p["licence"] = "AB123" if t[0] == 2 else ""
                p["hasPhoto"] = t[0] == 2
                out.append(p)
        return out
    @Slot(int, result=str)
    def playerPhoto(self, pid): return RED if pid == 2 else ""
    @Slot(str, result="QVariantMap")
    def preparePhoto(self, url):
        if not self.prep_ok: return {"ok": False, "error": "Ce fichier n'est pas une image lisible."}
        return {"ok": True, "data": BLUE}
    calls = []
    @Slot(int, "QVariantList", result="QVariantMap")
    def importPhotos(self, team, urls):
        B5.calls.append((team, [str(u) for u in urls]))
        if not urls: return {"ok": False, "error": "Aucune photo choisie."}
        items = [{"file": "9603907336.jpg", "status": "added", "message": "Photo ajoutée.", "player": "Luca S."},
                 {"file": "9605678531.jpg", "status": "replaced", "message": "Photo remplacée.", "player": "Marco Reus"},
                 {"file": "photo1.jpg", "status": "unknown", "message": "Aucun joueur de l'équipe n'a ce numéro de licence.", "player": ""}]
        return {"ok": True, "error": "", "added": 1, "replaced": 1, "failed": 1, "items": items}
    @Slot("QVariantMap", result="QVariantMap")
    def savePlayer(self, p):
        B5.saved.append(dict(p)); return {"ok": True, "id": p.get("id", -1), "error": ""}

def form_of(h):
    it = F(h, "playerPhoto")
    while it is not None and it.property("playerId") is None: it = it.parentItem()
    return it
def open_player(index):
    B5.saved = []; B5.prep_ok = True
    b = B5(); b.keep = []
    h = Harness(1024, 768, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "squad"), Q_ARG("QVariant", -1)); h.spin(700)
    click_item(h, cards(h)[index]); h.spin(500)
    return h, form_of(h)
def call(h, f, name, *args):
    QMetaObject.invokeMethod(f, name, *[Q_ARG("QVariant", a) for a in args]); h.spin(300)
def btn(h, name): return F(h, name)

# Joueur avec photo et licence (Marco Reus, id 2).
h, f = open_player(1)
check(f is not None and f.property("playerId") == 2, "fiche du joueur 2 ouverte")
check(f.property("photoShown") == RED, "photo du joueur chargée depuis la base")
check(F(h, "playerPhoto").isVisible(), "photo visible")
check(btn(h, "photoRemove").isVisible(), "« Retirer la photo » proposé")
lic = [c for c in F(h, "playerPhoto").parentItem().parentItem().parentItem().childItems()]
txt = []
def walk(it):
    for c in it.childItems():
        v = c.property("text")
        if isinstance(v, str): txt.append(v)
        walk(c)
walk(f)
check("AB123" in txt, "numéro de licence affiché dans la fiche")
check("N° de licence" in txt, "libellé « N° de licence »")
shot(h, "photo_fiche")

# Enregistrer sans toucher à la photo : ni photo ni retrait envoyés.
call(h, f, "save")
s = B5.saved[-1]
check(s.get("photo") == "" and s.get("removePhoto") is False and s.get("licence") == "AB123", f"enregistrement sans changement ({s.get('photo')!r}, {s.get('removePhoto')}, {s.get('licence')})")

# Choisir une autre photo : aperçu immédiat, envoyée à l'enregistrement.
call(h, f, "pickPhoto", "file:///x.png")
check(f.property("photoShown") == BLUE and f.property("photoNew") == BLUE, "nouvelle photo affichée")
call(h, f, "save")
s = B5.saved[-1]
check(s.get("photo") == BLUE and s.get("removePhoto") is False, "nouvelle photo envoyée au backend")
check(f.property("photoNew") == "", "photo en attente vidée après enregistrement")

# Retirer la photo.
call(h, f, "clearPhoto")
check(f.property("photoShown") == "" and not F(h, "playerPhoto").isVisible(), "photo retirée de l'aperçu")
check(not btn(h, "photoRemove").isVisible(), "plus de bouton « Retirer » sans photo")
call(h, f, "save")
s = B5.saved[-1]
check(s.get("removePhoto") is True and s.get("photo") == "", "retrait envoyé au backend")

# Image illisible : message d'erreur, rien ne change.
B5.prep_ok = False
call(h, f, "pickPhoto", "file:///mauvais.txt")
check("image lisible" in f.property("errorText") and f.property("photoShown") == "", "image illisible : erreur affichée, pas de photo")

# Joueur sans photo : initiales, pas de bouton retirer ; nouveau joueur vierge.
h, f = open_player(0)
check(f.property("photoShown") == "" and not F(h, "playerPhoto").isVisible(), "joueur sans photo : pas d'image")
check(f.property("initials") == "LS", f"initiales L+S ({f.property('initials')})")
check(not btn(h, "photoRemove").isVisible(), "sans photo : pas de bouton « Retirer »")
call(h, f, "openNew", 1)
check(f.property("playerId") == -1 and f.property("photoShown") == "" and f.property("photoNew") == "", "nouveau joueur : photo vide")
shot(h, "photo_nouveau")
# Import en lot depuis l'Effectif : bouton, appel au backend avec les URL choisies, compte-rendu.
B5.saved = []; B5.prep_ok = True; B5.calls = []
b = B5(); b.keep = []
h = Harness(1024, 768, backend=b)
QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "squad"), Q_ARG("QVariant", -1)); h.spin(700)
check(F(h, "photosButton") is not None and F(h, "photosButton").isVisible(), "bouton « Importer des photos » présent dans l'Effectif")
res = [o for o in h.win.findChildren(QObject) if o.objectName() == "photosResult"][0]
page = res.parent()
QMetaObject.invokeMethod(page, "importPhotos", Q_ARG("QVariant", ["file:///a/9603907336.jpg", "file:///a/photo1.jpg"])); h.spin(500)
check(len(B5.calls) == 1 and B5.calls[0][1] == ["file:///a/9603907336.jpg", "file:///a/photo1.jpg"], f"backend appelé avec les fichiers choisis ({B5.calls})")
check(res.property("opened") is True or res.property("visible") is True, "compte-rendu affiché")
summ = [o for o in res.findChildren(QObject) if o.objectName() == "photosSummary"]
def collect(it, name, acc):
    for c in it.childItems():
        if c.objectName() == name: acc.append(c)
        collect(c, name, acc)
    return acc
content = res.property("contentItem")
sm = collect(content, "photosSummary", [])
check(sm and sm[0].property("text") == "1 photo ajoutée, 1 photo remplacée, 1 fichier ignoré.", f"résumé ({sm[0].property('text') if sm else None})")
pr = collect(content, "photosProblem", [])
check(len(pr) == 1 and "photo1.jpg" in pr[0].property("text") and "licence" in pr[0].property("text"), "fichier non rattaché listé")
shot(h, "photos_import_result")
QMetaObject.invokeMethod(res, "close"); h.spin(300)
QMetaObject.invokeMethod(page, "importPhotos", Q_ARG("QVariant", [])); h.spin(300)
check(res.property("visible") is not True, "aucun fichier : pas de compte-rendu (message d'erreur)")
print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
