# Paramètres et assistance : navigation, formulaire de ticket « SUP - … », liens produits, retour, absence de secret.
import sys, os, re, glob
from urllib.parse import unquote, urlparse, parse_qs
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, QPointF
import shutil

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
def make(w, hh):
    b = B2(); b.keep = []; B2.mode = "now"
    from PySide6.QtCore import QCoreApplication
    if QCoreApplication.instance(): QCoreApplication.setApplicationVersion("0.10.0")   # le vrai main.cpp le fixe (FOOTCOACH_VERSION)
    h = Harness(w, hh, backend=b)
    QCoreApplication.setApplicationVersion("0.10.0")
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "settings"), Q_ARG("QVariant", -1)); h.spin(600)
    return h
def spage(h):
    it = F(h, "supportCard")
    while it is not None and it.property("dryRun") is None: it = it.parentItem()
    return it
def fld(h, label):
    todo = [h.win.contentItem()]
    while todo:
        it = todo.pop()
        if it.property("label") == label and it.property("multiline") is not None: return it
        todo.extend(it.childItems())
def click_item(h, it):
    p = it.mapToScene(QPointF(it.width() / 2, it.height() / 2)); h.click(p.x(), p.y())
def vis(h, n): return [c for c in F(h, n, True) if c.isVisible()]
def texts(it, out):
    for c in it.childItems():
        t = c.property("text")
        if isinstance(t, str): out.append(t)
        texts(c, out)
    return out

# --- Bureau
h = make(1280, 800); pg = spage(h); pg.setProperty("dryRun", True)
check(pg is not None and pg.isVisible(), "bureau : page Paramètres affichée")
t = texts(h.win.contentItem(), [])
check("Paramètres" in t, "bureau : entrée « Paramètres » dans le menu latéral")
check(all(len(vis(h, n)) == 1 for n in ("rowBug", "rowIdea", "rowContact", "rowVersion", "rowPrivacy", "rowData")), "bureau : les 6 lignes sont présentes")
check("FootCoach 0.10.0" in t, f"bureau : version affichée ({[x for x in t if x.startswith('FootCoach ')]})")
check("0123456789abcdef0123456789abcdef01234567" in t, "bureau : SHA-1 complet affiché dans Paramètres")
check(len(vis(h, "rowRevision")) == 1, "bureau : ligne « Révision (SHA-1) » présente")
shot(h, "set_bureau")
click_item(h, F(h, "rowBug")); h.spin(500)
check(pg.property("mode") == "support" and F(h, "supportForm").isVisible(), "bureau : « Signaler un problème » ouvre le formulaire")
check(F(h, "supportForm").property("category") == "bug", "bureau : catégorie « problème »")
form = F(h, "supportForm")
click_item(h, F(h, "supportSend")); h.spin(300)
check(F(h, "supportError").isVisible() and F(h, "supportError").property("text") == "Indiquez un titre.", "formulaire vide -> « Indiquez un titre. »")
check(not h.reporter.sent, "formulaire vide : rien n'est envoyé")
ft = fld(h, "Titre")
ft.setProperty("text", "Le chrono s'arrête"); h.spin(100)
click_item(h, F(h, "supportSend")); h.spin(300)
check(F(h, "supportError").property("text").startswith("Décrivez"), "titre seul -> « Décrivez … »")
fd = fld(h, "Description du problème")
fd.setProperty("text", "Quand j'écran se verrouille & que je reviens, 100% faux.\nDeuxième ligne."); h.spin(100)
fs = fld(h, "Étapes pour reproduire (facultatif)"); fs.setProperty("text", "1. Démarrer\n2. Verrouiller"); h.spin(100)
click_item(h, F(h, "supportSend")); h.spin(500)
check(len(h.reporter.sent) == 1, "un seul envoi demandé")
title, body, labels = h.reporter.sent[0]
check(title == "SUP - Le chrono s'arrête", f"titre préfixé « SUP - » : {title}")
check(labels == ["bug"], f"label bug : {labels}")
check("### Description" in body and "100% faux" in body and "&" in body and "Deuxième ligne." in body, "corps : description intacte (& % retours)")
check("### Étapes pour reproduire" in body and "2. Verrouiller" in body, "corps : étapes pour reproduire")
check("### Environnement" in body and "FootCoach" in body and "Système :" in body, "corps : environnement joint (consentement activé)")
check(form.property("sent") is True and F(h, "supportDone").isVisible() and not F(h, "supportSend").isVisible(), "succès : écran de confirmation affiché")
check("github" not in "".join(texts(h.win.contentItem(), [])).lower(), "aucune mention de GitHub dans l'interface")
click_item(h, F(h, "supportDoneClose")); h.spin(400)
check(pg.property("mode") == "browse", "après envoi : retour à la page Paramètres")

# sans environnement + amélioration
QMetaObject.invokeMethod(pg, "openSupport", Q_ARG("QVariant", "enhancement")); h.spin(400)
form = F(h, "supportForm")
check(not any(c.property("label") == "Étapes pour reproduire (facultatif)" and c.isVisible() for c in [fld(h, "Étapes pour reproduire (facultatif)")] if c), "amélioration : champ « étapes » masqué")
fld(h, "Titre").setProperty("text", "Mode sombre auto")
fld(h, "Votre idée").setProperty("text", "Suivre le réglage du système")
sw = F(h, "supportEnv"); sw.setProperty("checked", False); h.spin(100)
click_item(h, F(h, "supportSend")); h.spin(400)
title, body, labels = h.reporter.sent[-1]
check(labels == ["enhancement"] and "### Environnement" not in body and "Étapes" not in body, "amélioration : label, sans environnement ni étapes")
check(form.property("sent") is True, "amélioration : confirmation")

# échec puis nouvel essai ; envoi en cours (bouton bloqué)
QMetaObject.invokeMethod(pg, "openSupport", Q_ARG("QVariant", "bug")); h.spin(300)
check(form.property("sent") is False and F(h, "supportSend").isVisible(), "réouverture : formulaire vide, plus de confirmation")
fld(h, "Titre").setProperty("text", "Échec réseau"); fld(h, "Description du problème").setProperty("text", "x"); h.spin(100)
h.reporter.outcome = ("fail", "Impossible de joindre le service. Vérifiez votre connexion et réessayez.")
n = len(h.reporter.sent)
click_item(h, F(h, "supportSend")); h.spin(300)
check(len(h.reporter.sent) == n + 1 and form.property("sent") is False, "échec : pas de confirmation")
check(F(h, "supportError").isVisible() and "Impossible de joindre" in F(h, "supportError").property("text"), "échec : message affiché")
check(fld(h, "Titre").property("text") == "Échec réseau", "échec : la saisie est conservée")
h.reporter.outcome = ("hold", "")
click_item(h, F(h, "supportSend")); h.spin(300)
check(form.property("busy") is True and not F(h, "supportSend").property("enabled") and "en cours" in F(h, "supportSend").property("text"), "envoi en cours : bouton désactivé")
click_item(h, F(h, "supportSend")); h.spin(200)
check(len(h.reporter.sent) == n + 2, "envoi en cours : un second toucher n'envoie rien")
h.reporter.outcome = ("ok", ""); h.reporter.release(); h.spin(300)
check(form.property("sent") is True, "envoi libéré : confirmation")
QMetaObject.invokeMethod(pg, "goBack"); h.spin(200)

# jeton absent : seul l'e-mail est proposé
from PySide6.QtCore import QCoreApplication
b0 = B2(); b0.keep = []; B2.mode = "now"
QCoreApplication.setApplicationVersion("0.10.0")
h0 = Harness(412, 892, backend=b0, reporter=IssueReporter(available=False))
QMetaObject.invokeMethod(h0.win, "go", Q_ARG("QVariant", "settings"), Q_ARG("QVariant", -1)); h0.spin(500)
pg0 = spage(h0); pg0.setProperty("dryRun", True)
QMetaObject.invokeMethod(pg0, "openSupport", Q_ARG("QVariant", "bug")); h0.spin(300)
check(not F(h0, "supportSend").isVisible() and F(h0, "supportMail").isVisible(), "sans jeton : pas de bouton d'envoi, e-mail proposé")
fld(h0, "Titre").setProperty("text", "t"); fld(h0, "Description du problème").setProperty("text", "d"); h0.spin(100)
click_item(h0, F(h0, "supportMail")); h0.spin(300)
check(F(h0, "supportForm").property("lastUrl").startswith("mailto:") and not h0.reporter.sent, "sans jeton : l'e-mail se prépare, rien n'est envoyé")

# e-mail
QMetaObject.invokeMethod(pg, "openSupport", Q_ARG("QVariant", "question")); h.spin(300)
fld(h, "Titre").setProperty("text", "Comment exporter ?")
fld(h, "Votre question").setProperty("text", "Où trouver le fichier ?"); h.spin(100)
click_item(h, F(h, "supportMail")); h.spin(400)
url = F(h, "supportForm").property("lastUrl")
check(url.startswith("mailto:footcoach.app@gmail.com?subject=SUP%20-%20Comment%20exporter"), f"e-mail pré-rempli vers le contact ({url[:60]}…)")

# retour : formulaire -> page -> accueil
QMetaObject.invokeMethod(pg, "openSupport", Q_ARG("QVariant", "bug")); h.spin(300)
ok = pg.property("mode") == "support"
QMetaObject.invokeMethod(pg, "goBack"); h.spin(200)
check(ok and pg.property("mode") == "browse", "retour : ferme le formulaire")

# --- Navigation « Données » depuis la page
click_item(h, F(h, "rowData")); h.spin(500)
check(h.win.property("section") == "data", "ligne « Données » -> page Données")

# --- Téléphone : accès par « Plus », retour
h = make(412, 892); pg = spage(h); pg.setProperty("dryRun", True)
check(pg.isVisible() and h.win.property("section") == "settings", "téléphone : page Paramètres")
shot(h, "set_telephone")
click_item(h, F(h, "rowIdea")); h.spin(500)
check(pg.property("mode") == "support", "téléphone : formulaire ouvert")
shot(h, "set_telephone_form")
ok = QMetaObject.invokeMethod(h.win, "handleBack"); h.spin(300)
check(pg.property("mode") == "browse" and h.win.property("section") == "settings", "téléphone : retour ferme le formulaire, reste dans Paramètres")
QMetaObject.invokeMethod(h.win, "handleBack"); h.spin(300)
check(h.win.property("section") == "home", "téléphone : second retour -> accueil")

# --- Tablette
h = make(1024, 768); pg = spage(h)
check(pg.isVisible() and F(h, "supportCard").mapToScene(QPointF(F(h, "supportCard").width(), 0)).x() <= 1024 - 8, "tablette paysage : carte dans la fenêtre")
shot(h, "set_tablette")

# --- Aucun secret embarqué
bad = []
for f in glob.glob(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "qml", "*")):
    if os.path.isdir(f): continue
    try: src = open(f, encoding="utf-8").read()
    except Exception: continue
    if re.search(r"ghp_[A-Za-z0-9]{20}|github_pat_|Authorization|api\.github\.com|Bearer |openUrlExternally\(Link\.issue", src): bad.append(os.path.basename(f))
check(not bad, f"aucun jeton ni appel d'API GitHub dans qml/ ({bad})")
print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
