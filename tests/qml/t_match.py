"""L5/L7 : matchs (liste, fiche, convoqués, schéma tactique, garde de sortie) et matchs dans le calendrier.

Backend SIMULÉ (comme les autres tests de ce dossier) : ce test valide l'interface QML (MatchesPage, MatchEditor,
CalendarPage, Main), pas la base SQLite (voir tests/tst_matches.cpp) ; il ne remplace ni un essai sur appareil ni une
validation Qt 6.9.
"""
import sys, os, datetime
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, Q_RETURN_ARG, QObject, Qt
errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)

TODAY = datetime.date.today()
def D(n): return (TODAY + datetime.timedelta(days=n)).strftime("%Y-%m-%d")
def disp(iso): return iso[8:10] + "/" + iso[5:7] + "/" + iso[0:4]

class B6(B2):
    @Slot(int, bool, result="QVariantList")
    def players(self, team, inactive):
        return [{"id": i, "teamId": 1, "firstName": "P%d" % i, "lastName": "N%d" % i, "fullName": "P%d N%d" % (i, i),
                 "jerseyNumber": i, "active": True, "position": "", "age": -1} for i in (1, 2, 3)]
    @Slot(result="QVariantList")
    def teams(self):
        return [{"id": 1, "clubId": 1, "clubName": "Mon club", "name": "Senior A", "category": "Seniors", "playerCount": 3}]
    @Slot(int, str, str, result="QVariantList")
    def sessions(self, team, a, b): return []
    @Slot(int, str, str, result="QStringList")
    def sessionDates(self, team, a, b): return []

def make():
    B6.mode = "none"; b = B6(); b.keep = []
    b._club_init()
    h = Harness(412, 892, backend=b)
    h.spin(500)
    return h, b
def go(h, sec, item=-1, deep=False):
    QMetaObject.invokeMethod(h.win, "goFrom" if deep else "go", Q_ARG("QVariant", sec), Q_ARG("QVariant", item)); h.spin(500)
def back(h):
    r = QMetaObject.invokeMethod(h.win, "handleBack", Qt.DirectConnection, Q_RETURN_ARG("QVariant")); h.spin(500); return r
def section(h): return h.win.property("section")
def find(h, pred):
    for o in h.win.findChildren(QObject):
        try:
            if pred(o): return o
        except Exception: pass
def cls(h, prefix): return find(h, lambda o: o.metaObject().className().startswith(prefix))
def named(h, name): return find(h, lambda o: o.objectName() == name)
def call(o, m, *a): QMetaObject.invokeMethod(o, m, *[Q_ARG("QVariant", x) for x in a])
def field(h, name): return named(h, name)
def items(h, name, visible=True):
    # Les éléments créés par un Repeater ne sont pas des enfants QObject : on parcourt l'arbre visuel.
    todo = [h.win.contentItem()]; out = []
    while todo:
        it = todo.pop()
        if it.objectName() == name and (it.isVisible() or not visible): out.append(it)
        todo.extend(it.childItems())
    return out
def jv(v): return v.toVariant() if hasattr(v, "toVariant") else v

# --- 1. Créer un match : champs, convoqués, enregistrement ------------------------------------------------------------
h, b = make()
go(h, "matches")
page = cls(h, "MatchesPage"); ed = cls(h, "MatchEditor")
check(section(h) == "matches", "section Matchs atteinte")
check(page.property("editing") is False and named(h, "matchEmpty") is not None, "liste vide : message d'état vide")
call(page, "newMatch", D(5)); h.spin(500)
check(page.property("editing") is True and ed.property("isNew") is True, "« + » ouvre la fiche d'un nouveau match")
check(ed.property("dirty") is False, "fiche neuve : aucune modification en attente")

# 1a. enregistrement refusé : adversaire vide, rien n'est créé
call(ed, "save")
check(ed.property("matchId") == -1 and len(b._matches) == 0, "adversaire vide : rien n'est enregistré")
check(ed.property("errorText") != "", f"l'erreur est affichée ({ed.property('errorText')})")

# 1b. saisie + convocation
field(h, "opponentField").setProperty("text", "FC Rivière")
call(ed, "setRole", 1, "starter"); call(ed, "setRole", 2, "substitute"); call(ed, "setRole", 3, "starter"); call(ed, "setRole", 3, "")
h.spin(200)
check(ed.property("starters") == 1 and ed.property("substitutes") == 1, "convoqués : 1 titulaire, 1 remplaçant (3 retiré)")
check(ed.property("dirty") is True, "modifications en attente détectées")
call(ed, "save"); h.spin(300)
check(ed.property("matchId") > 0 and len(b._matches) == 1, "match enregistré")
m = list(b._matches.values())[0]
check(m["opponent"] == "FC Rivière" and m["date"] == D(5) and m["time"] == "15:00" and m["homeAway"] == "home",
      f"valeurs enregistrées ({m['opponent']}, {m['date']}, {m['time']}, {m['homeAway']})")
check(sorted(b.last_convoked) == [(1, "starter"), (2, "substitute")], f"convoqués envoyés au backend ({b.last_convoked})")
check(ed.property("dirty") is False, "après enregistrement : plus de modification en attente")

# 1c. score incomplet refusé par le backend, le match n'est pas modifié
field(h, "scoreFor").setProperty("text", "2"); h.spin(100)
n = b.match_saves
call(ed, "save")
check(b._matches[ed.property("matchId")]["scoreFor"] == -1 and ed.property("errorText") != "", "score incomplet : refusé, ancien score conservé")
field(h, "scoreAgainst").setProperty("text", "1"); h.spin(100)
call(ed, "save")
check(b._matches[ed.property("matchId")]["scoreFor"] == 2 and b._matches[ed.property("matchId")]["scoreAgainst"] == 1, "score complet enregistré")

# --- 2. Liste : le match y figure, retour vers la liste ---------------------------------------------------------------------
call(page, "refreshRows"); h.spin(200)
check(len(page.property("rows")) == 1, "le match est dans « À venir »")
back(h)
check(page.property("editing") is False and section(h) == "matches", "retour : liste des matchs")

# --- 3. Garde de sortie : modifications non enregistrées ne sont pas perdues en silence -----------------------------------------
call(page, "openMatch", ed.property("matchId")); h.spin(500)
field(h, "opponentField").setProperty("text", "Autre nom"); h.spin(200)
check(ed.property("dirty") is True and page.property("editing") is True, "fiche modifiée")
go(h, "home")
check(section(h) == "matches", "changer de section avec des modifications : on reste sur la fiche (confirmation)")
confirm = find(h, lambda o: o.property("pending") is not None and o.metaObject().className().startswith("PromptDialog"))
check(confirm is not None and confirm.property("visible") is True, "la confirmation « Quitter sans enregistrer ? » est affichée")
r = QMetaObject.invokeMethod(page, "hasUnsaved", Qt.DirectConnection, Q_RETURN_ARG("QVariant"))
check(r is True, "fermeture de la fenêtre : la page signale des modifications non enregistrées")
QMetaObject.invokeMethod(confirm, "accepted", Q_ARG("QString", "")); h.spin(500)
check(section(h) == "home", "après confirmation : on quitte")
check(b._matches[ed.property("matchId")]["opponent"] == "FC Rivière", "le match enregistré n'a pas été modifié par l'abandon")

# --- 4. Schéma tactique : refusé tant que le match n'est pas enregistré, puis créé à la demande, jamais en double ------------------------
h, b = make()
go(h, "matches"); page = cls(h, "MatchesPage"); ed = cls(h, "MatchEditor")
call(page, "newMatch", D(2)); h.spin(400)
call(ed, "openBoard"); h.spin(200)
check(ed.property("boardOpen") is False and len(b._boards) == 0, "match non enregistré : pas de schéma créé")
field(h, "opponentField").setProperty("text", "AS Voisins"); call(ed, "save"); h.spin(300)
mid = ed.property("matchId")
call(ed, "openBoard"); h.spin(600)
check(ed.property("boardOpen") is True and len(b._boards) == 1, "schéma créé à la demande et ouvert")
bid = ed.property("openBoardId")
check(b._match_board.get(mid) == bid, "le schéma est celui du match")
back(h)                                  # geste retour : ferme le schéma, pas la fiche
check(ed.property("boardOpen") is False and page.property("editing") is True, "retour : le schéma se ferme, la fiche reste")
call(ed, "openBoard"); h.spin(500)
check(len(b._boards) == 1 and ed.property("openBoardId") == bid, "ré-ouverture : même schéma, pas de doublon")
back(h)

# --- 5. Suppression : en échec, la fiche reste ; en succès, match et schéma disparaissent ------------------------------------------
ed.setProperty("matchId", 9999); call(ed, "remove"); h.spin(300)
check(ed.property("matchId") == 9999 and page.property("editing") is True, "suppression d'un match inexistant : échec signalé, la fiche reste")
ed.setProperty("matchId", mid)
call(ed, "remove"); h.spin(500)
check(len(b._matches) == 0 and len(b._boards) == 0, "suppression : le match et son schéma disparaissent")
check(page.property("editing") is False, "retour à la liste après suppression")

# --- 6. Calendrier : pastille de match, carte distincte d'une séance, ouverture du match ------------------------------------------
h, b = make()
r = b.saveMatch({"teamId": 1, "date": D(0), "time": "16:00", "opponent": "US Nord", "homeAway": "away", "status": "planned", "scoreFor": -1, "scoreAgainst": -1})
mid = r["id"]
go(h, "calendar"); h.spin(500)
cal = cls(h, "CalendarPage")
check(len(jv(cal.property("matches"))) == 1 and len(jv(cal.property("sessions"))) == 0, "calendrier : 1 match, 0 séance (distincts)")
check(len(items(h, "matchCard")) == 1, "carte « Match · US Nord » affichée")
check(len(items(h, "calendarEmpty")) == 0, "pas d'état vide quand il y a un match")
dots = items(h, "matchDot")
check(len(dots) == 1, f"une pastille de match dans le mois ({len(dots)})")
sdots = items(h, "sessionDot")
check(len(sdots) == 0, "aucune pastille de séance")
check(str(dots[0].property("color").name()).lower() == "#f59e0b", "pastille de match d'une couleur distincte des séances")
card = items(h, "matchCard")[0]
x, y = h_center = (card.mapToScene(QPointF(card.width() / 2, card.height() / 2)).x(), card.mapToScene(QPointF(card.width() / 2, card.height() / 2)).y())
h.click(x, y); h.spin(700)
check(section(h) == "matches" and cls(h, "MatchEditor").property("matchId") == mid, "toucher le match : la fiche du match s'ouvre")
back(h)
check(section(h) == "calendar", f"retour depuis le match ouvert du calendrier : calendrier ({section(h)})")

# 6b. changement de période : un match d'un autre jour n'apparaît pas
r2 = b.saveMatch({"teamId": 1, "date": D(40), "time": "10:00", "opponent": "Hors période", "scoreFor": -1, "scoreAgainst": -1})
cal = cls(h, "CalendarPage"); call(cal, "refreshData"); h.spin(200)
check(len(jv(cal.property("matches"))) == 1, "période « jour » : le match du jour 40 n'est pas listé")
cal.setProperty("scope", "month"); h.spin(300)
check(len(jv(cal.property("matches"))) in (1, 2), "période « mois » : liste recalculée")

# --- 7. Import : les données remplacées ferment la fiche ouverte ---------------------------------------------------------------------
h, b = make()
go(h, "matches"); page = cls(h, "MatchesPage"); ed = cls(h, "MatchEditor")
call(page, "newMatch", D(1)); h.spin(300)
field(h, "opponentField").setProperty("text", "Brouillon"); h.spin(100)
b.dataReplaced.emit()
h.spin(400)
check(page.property("editing") is False, "données remplacées : la fiche est fermée sans confirmation (rien à enregistrer vers l'ancienne base)")

print("\n%s" % ("ECHECS : %d" % len(errs) if errs else "TOUT PASSE"))
for e in errs: print("   KO ", e)
sys.exit(1 if errs else 0)
