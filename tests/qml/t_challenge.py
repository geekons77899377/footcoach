"""Feuille de challenge 2/2 depuis un match : bouton, garde « modifications non enregistrées », liste des licences
manquantes, choix du fichier et export. Backend SIMULÉ : valide l'interface QML (MatchEditor, ChallengeProblems), pas la
génération du PDF ni la base (voir tests/tst_matches.cpp) ; pas d'essai du sélecteur de fichiers Android."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_match.py")).read().split("# --- 1. Créer un match")[0])

import shutil
def shot(h, tag):
    h.shot("x"); shutil.copy(os.path.join(OUT, "x.png"), os.path.join(OUT, tag + ".png"))

class B7(B6):
    check_result = {}
    exported = []
    checks = 0
    @Slot(int, result="QVariantMap")
    def challengeSheetCheck(self, mid):
        B7.checks += 1
        return dict(B7.check_result)
    share_ok = False
    shared = []
    @Slot(str, str, str, result="QVariantMap")
    def shareFile(self, url, mime, title):
        B7.shared.append((url, mime))
        return {"ok": B7.share_ok, "error": "" if B7.share_ok else "unsupported"}
    @Slot(str, result=str)
    def challengeSheetTempUrl(self, name):
        return "file:///tmp/cache/challenge/" + name
    @Slot(int, str, result="QVariantMap")
    def exportChallengeSheet(self, mid, url):
        B7.exported.append((mid, url))
        return {"ok": True, "error": "", "bytes": 20480, "fileName": "x.pdf"}

def make7():
    B7.check_result = {}; B7.exported = []; B7.checks = 0; B7.share_ok = False; B7.shared = []
    b = B7(); b.keep = []; b._club_init()
    h = Harness(412, 892, backend=b); h.spin(500)
    return h, b

h, b = make7()
go(h, "matches"); page = cls(h, "MatchesPage"); ed = cls(h, "MatchEditor")
call(page, "newMatch", D(4)); h.spin(500)
btn = named(h, "challengeSheetButton")
check(btn is not None and btn.property("enabled") is False, "match non enregistré : bouton « Créer la feuille » désactivé")
call(ed, "startChallengeSheet"); h.spin(200)
check(B7.checks == 0, "match non enregistré : aucun contrôle demandé au backend")

field(h, "opponentField").setProperty("text", "AS Voisins"); call(ed, "setRole", 1, "starter"); call(ed, "save"); h.spin(400)
check(btn.property("enabled") is True, "match enregistré : bouton activé")

# Modifications non enregistrées : pas de feuille (le PDF reflète la base).
field(h, "opponentField").setProperty("text", "AS Autres"); h.spin(200)
check(ed.property("dirty") is True and btn.property("enabled") is False, "modifications en attente : bouton désactivé")
call(ed, "startChallengeSheet"); h.spin(200)
check(B7.checks == 0, "modifications en attente : pas de contrôle")
call(ed, "save"); h.spin(300)

# Licence manquante / invalide : fenêtre avec les joueurs à corriger, pas de sélecteur de fichier.
B7.check_result = {"ok": False, "error": "2 joueurs n'ont pas de n° de licence valide.", "count": 2,
                   "problems": [{"name": "Trop Court", "message": "n° de licence invalide (10 chiffres attendus)"},
                                {"name": "Sans Licence", "message": "n° de licence manquant"}]}
call(ed, "startChallengeSheet"); h.spin(400)
dlg = named(h, "challengeProblems")
check(B7.checks == 1, "contrôle demandé pour le match enregistré")
check(dlg is not None and dlg.property("visible") is True, "fenêtre des problèmes affichée")
def collect(it, name, acc):
    for c in it.childItems():
        if c.objectName() == name: acc.append(c)
        collect(c, name, acc)
    return acc
content = dlg.property("contentItem")
m = collect(content, "challengeMessage", [])
pr = collect(content, "challengeProblem", [])
check(m and "2 joueurs" in m[0].property("text"), "message du backend affiché")
check(len(pr) == 2 and "Trop Court" in pr[0].property("text") and "10 chiffres" in pr[0].property("text")
      and "Sans Licence" in pr[1].property("text") and "manquant" in pr[1].property("text"), "chaque joueur à corriger est listé avec sa raison")
shot(h, "challenge_problems")
check(B7.exported == [], "aucun export tant que des licences sont à corriger")
call(dlg, "close"); h.spin(300)

# Plus de 12 convoqués : message seul.
B7.check_result = {"ok": False, "error": "La feuille a 12 lignes : 14 joueurs sont convoqués. Retirez-en 2 de la convocation.", "count": 14, "problems": []}
call(ed, "startChallengeSheet"); h.spin(400)
m = collect(dlg.property("contentItem"), "challengeMessage", [])
check(dlg.property("visible") is True and m and "12 lignes" in m[0].property("text")
      and len(collect(dlg.property("contentItem"), "challengeProblem", [])) == 0, "trop de convoqués : message sans liste")
call(dlg, "close"); h.spin(300)

# Tout est valide : la feuille est créée dans un fichier temporaire et ouverte aussitôt (pas de sélecteur d'abord).
B7.check_result = {"ok": True, "error": "", "count": 5, "problems": [], "teamName": "Senior A",
                   "fileName": "feuille-challenge-senior-a-2026-11-14.pdf"}
ed.setProperty("dryRun", True)                       # aucune application réelle n'est lancée
sd = named(h, "challengeSheetDialog")
call(ed, "startChallengeSheet"); h.spin(400)
want = "file:///tmp/cache/challenge/feuille-challenge-senior-a-2026-11-14.pdf"
check(len(B7.exported) == 1 and B7.exported[0] == (ed.property("matchId"), want), f"export vers le fichier temporaire ({B7.exported})")
check(ed.property("lastOpened") == want, f"le PDF créé est ouvert dans une application externe ({ed.property('lastOpened')})")
check(sd.property("visible") is not True, "pas de sélecteur de fichier quand l'ouverture réussit")
check(dlg.property("visible") is not True, "pas de fenêtre d'erreur quand tout est valide")
def toast_text(h):
    msgs = [str(o.property("message")) for o in h.win.findChildren(QObject)
            if o.metaObject().className().startswith("Toast") and o.property("message")]
    return " | ".join(msgs)
check("Enregistrez-la ou imprimez-la" in toast_text(h), f"message de réussite ({toast_text(h)})")

# Ouverture impossible : repli sur le sélecteur pour enregistrer.
ed.setProperty("dryRunOpens", False); h.spin(100)
call(ed, "startChallengeSheet"); h.spin(400)
check(len(B7.exported) == 2 and sd.property("visible") is True and "feuille-challenge-senior-a-2026-11-14.pdf" in str(sd.property("currentFile")),
      "ouverture impossible : le sélecteur propose d'enregistrer sous le même nom")
QMetaObject.invokeMethod(sd, "close") if sd.property("visible") else None

# Feuille de partage disponible (Android) : elle remplace l'ouverture externe et le sélecteur.
B7.share_ok = True; B7.shared = []; n = len(B7.exported)
ed.setProperty("dryRunOpens", True); ed.setProperty("lastOpened", "")
call(ed, "startChallengeSheet"); h.spin(400)
check(B7.shared == [(want, "application/pdf")], f"la feuille de partage reçoit le PDF créé ({B7.shared})")
check(len(B7.exported) == n + 1 and sd.property("visible") is not True, "partage réussi : pas de sélecteur de fichier")
check("où envoyer ou enregistrer" in toast_text(h), f"message de partage ({toast_text(h)})")

# Terrain du match ouvert par-dessus la fiche : un toucher sur le terrain ne doit jamais atteindre la fiche dessous
# (constat sur appareil, 14/11/2026 : la feuille de challenge s'ouvrait depuis le terrain).
h, b = make7()
B7.check_result = {"ok": True, "fileName": "feuille.pdf"}
go(h, "matches"); page = cls(h, "MatchesPage"); ed = cls(h, "MatchEditor")
call(page, "newMatch", D(4)); h.spin(500)
field(h, "opponentField").setProperty("text", "AS Voisins"); call(ed, "setRole", 1, "starter"); call(ed, "save"); h.spin(400)
btn = named(h, "challengeSheetButton")
# Amène le bouton au milieu de l'écran : on fait défiler la Flickable de la fiche qui le contient.
scroll = btn.parentItem()
while scroll is not None and not scroll.inherits("QQuickFlickable"):
    scroll = scroll.parentItem()
scroll = scroll.parentItem() if scroll is not None and scroll.objectName() == "" and not scroll.property("contentHeight") else scroll
p0 = btn.mapToItem(scroll.property("contentItem"), QPointF(0, 0))
scroll.setProperty("contentY", max(0, p0.y() - 400)); h.spin(300)
c = btn.mapToScene(QPointF(btn.width() / 2, btn.height() / 2))
n0 = B7.checks
h.click(c.x(), c.y()); h.spin(400)
check(B7.checks == n0 + 1, f"témoin : sans terrain ouvert, le toucher atteint bien le bouton ({c.x():.0f}, {c.y():.0f})")
# Le témoin a ouvert le choix du fichier (fenêtre modale) : on ferme toute fenêtre ouverte, sinon les touchers
# suivants iraient à son voile et non au terrain.
for o in h.win.findChildren(QObject):
    try:
        if o.inherits("QQuickPopup") and o.property("visible"):
            QMetaObject.invokeMethod(o, "close")
    except Exception: pass
h.spin(400)
call(ed, "openBoard"); h.spin(800)
check(ed.property("boardOpen") is True, "terrain du match ouvert")
pops = [o.objectName() or o.metaObject().className() for o in h.win.findChildren(QObject) if o.inherits("QQuickPopup") and o.property("visible")]
check(pops == [], f"aucune fenêtre ouverte par-dessus le terrain ({pops})")
n1 = B7.checks
# Toucher du doigt (et non clic de souris) : c'est le cas du téléphone.
from PySide6.QtCore import QPoint
touch = QTest.createTouchDevice()
def tap_finger(x, y):
    QTest.touchEvent(h.win, touch).press(0, QPoint(int(x), int(y)), h.win).commit(); h.spin(60)
    QTest.touchEvent(h.win, touch).release(0, QPoint(int(x), int(y)), h.win).commit(); h.spin(300)
for dy in (0, -60, 60, -120, 120):
    h.click(c.x(), c.y() + dy); h.spin(250)
    tap_finger(c.x(), c.y() + dy)
for dx in (-150, 150):
    h.click(c.x() + dx, c.y()); h.spin(250)
    tap_finger(c.x() + dx, c.y())
be = find(h, lambda o: o.metaObject().className().startswith("BoardEditor"))
for tool in ("multi", "select", "player", "opponent", "ball"):
    be.setProperty("tool", tool); h.spin(200)
    for dy in (0, -100, 100, 250, -250):
        tap_finger(c.x(), c.y() + dy)
        h.click(c.x(), c.y() + dy); h.spin(150)
    # glisser du doigt (entourer en mode multiple)
    QTest.touchEvent(h.win, touch).press(0, QPoint(int(c.x()) - 80, int(c.y()) - 80), h.win).commit(); h.spin(40)
    for k in range(1, 8):
        QTest.touchEvent(h.win, touch).move(0, QPoint(int(c.x()) - 80 + 20 * k, int(c.y()) - 80 + 20 * k), h.win).commit(); h.spin(30)
    QTest.touchEvent(h.win, touch).release(0, QPoint(int(c.x()) + 60, int(c.y()) + 60), h.win).commit(); h.spin(300)
be.setProperty("tool", "select"); h.spin(200)
check(B7.checks == n1, f"terrain ouvert : aucun toucher ne passe à la fiche dessous ({B7.checks - n1} feuille(s) demandée(s))")

print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
