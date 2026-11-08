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
    @Slot(int, str, result="QVariantMap")
    def exportChallengeSheet(self, mid, url):
        B7.exported.append((mid, url))
        return {"ok": True, "error": "", "bytes": 20480, "fileName": "x.pdf"}

def make7():
    B7.check_result = {}; B7.exported = []; B7.checks = 0
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

# Tout est valide : nom de fichier proposé, export demandé après le choix du fichier.
B7.check_result = {"ok": True, "error": "", "count": 5, "problems": [], "teamName": "Senior A",
                   "fileName": "feuille-challenge-senior-a-2026-11-14.pdf"}
call(ed, "startChallengeSheet"); h.spin(400)
check(ed.property("sheetFileName") == "feuille-challenge-senior-a-2026-11-14.pdf", "nom de fichier proposé : feuille-challenge-<équipe>-<date>.pdf")
check(dlg.property("visible") is not True, "pas de fenêtre d'erreur quand tout est valide")
sd = named(h, "challengeSheetDialog")
check("feuille-challenge-senior-a-2026-11-14.pdf" in str(sd.property("currentFile")), "le sélecteur propose ce nom")
QMetaObject.invokeMethod(sd, "close") if sd.property("visible") else None
QMetaObject.invokeMethod(sd, "accepted"); h.spin(300)
check(len(B7.exported) == 1 and B7.exported[0][0] == ed.property("matchId"), f"export demandé pour ce match ({B7.exported})")

print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
