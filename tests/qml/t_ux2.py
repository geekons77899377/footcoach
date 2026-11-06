"""L-UX2 : « retour » est cohérent partout : on revient à la page d'où l'on vient.

Backend SIMULÉ (comme les autres tests de ce dossier) : ce test valide la navigation QML (Main, AppState, pages), pas
la base SQLite, et ne remplace ni un essai sur appareil ni une validation Qt 6.9.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, Q_RETURN_ARG, QObject
errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)

def make():
    B2.mode = "now"; b = B2(); b.keep = []
    h = Harness(412, 892, backend=b)
    h.spin(500)
    return h, b
def section(h): return h.win.property("section")
def go(h, sec, item=-1, deep=False):
    QMetaObject.invokeMethod(h.win, "goFrom" if deep else "go", Q_ARG("QVariant", sec), Q_ARG("QVariant", item)); h.spin(500)
def back(h):
    r = QMetaObject.invokeMethod(h.win, "handleBack", Qt.DirectConnection, Q_RETURN_ARG("QVariant")); h.spin(500); return r
def find(h, pred):
    for o in h.win.findChildren(QObject):
        if pred(o): return o
def ex_page(h): return find(h, lambda o: o.metaObject().className().startswith("ExercisesPage"))
def tac_page(h): return find(h, lambda o: o.property("pendingDelete") is not None)
def ex_form(h): return find(h, lambda o: o.metaObject().indexOfSignal("archiveRequested()") >= 0)

# --- 1. Accueil -> exercice : retour = accueil (geste retour, puis flèche) ------------------------------------------------------
h, b = make()
check(section(h) == "home", "départ : Accueil")
go(h, "exercises", 11, deep=True)
check(section(h) == "exercises" and ex_page(h).property("editing") is True, "exercice ouvert depuis l'accueil")
back(h)
check(section(h) == "home", f"geste retour : retour à l'accueil, pas à la liste des exercices ({section(h)})")
go(h, "exercises", 11, deep=True)
QMetaObject.invokeMethod(ex_form(h), "closeRequested"); h.spin(500)
check(section(h) == "home", f"flèche retour de la fiche : retour à l'accueil ({section(h)})")

# --- 2. Depuis la bibliothèque : retour = liste ; puis accueil ----------------------------------------------------------------------
go(h, "exercises", -1)
e = ex_page(h)
QMetaObject.invokeMethod(e, "openExercise", Q_ARG("QVariant", dict(b.exercises(False)[0]))); h.spin(400)
check(e.property("editing") is True, "fiche ouverte depuis la liste")
back(h)
check(section(h) == "exercises" and e.property("editing") is False, "retour depuis la liste : on reste dans la bibliothèque")
back(h)
check(section(h) == "home", "second retour : accueil")

# --- 3. Origine périmée : un lien profond puis un autre onglet ne laisse rien de caché -------------------------------------------------
h, b = make()
go(h, "exercises", 11, deep=True)
go(h, "sessions")                     # onglet de la barre de navigation : l'origine est oubliée
go(h, "exercises", -1)
QMetaObject.invokeMethod(ex_page(h), "openExercise", Q_ARG("QVariant", dict(b.exercises(False)[0]))); h.spin(400)
back(h)
check(section(h) == "exercises", f"fiche ouverte normalement après un autre onglet : retour à la liste ({section(h)})")

# --- 4. Accueil -> schéma : retour = accueil ; depuis la liste : retour = liste ------------------------------------------------------
h, b = make()
A = b.add("Schéma A", PLAYERS_HALF) if "PLAYERS_HALF" in globals() else b.add("Schéma A", "{}")
go(h, "tactics", A, deep=True)
check(section(h) == "tactics" and tac_page(h).property("currentId") == A, "schéma ouvert depuis l'accueil")
back(h)
check(section(h) == "home", f"retour du schéma : accueil ({section(h)})")
go(h, "tactics", -1)
QMetaObject.invokeMethod(tac_page(h), "openBoard", Q_ARG("QVariant", A)); h.spin(400)
back(h)
check(section(h) == "tactics" and tac_page(h).property("currentId") == -1, "schéma ouvert depuis la liste : retour à la liste")

# --- 5. Paramètres -> Données : retour = Paramètres ------------------------------------------------------------------------------------
h, b = make()
go(h, "settings")
go(h, "data", -1, deep=True)
check(section(h) == "data", "Données ouvertes depuis Paramètres")
back(h)
check(section(h) == "settings", f"retour de Données : Paramètres ({section(h)})")
back(h)
check(section(h) == "home", "retour suivant : accueil")

# --- 6. Calendrier -> plan de séance : retour = Calendrier ; depuis Séances : retour = liste -------------------------------------------
h, b = make()
go(h, "sessions")
sp = find(h, lambda o: o.property("editing") is not None and o.property("scope") is not None and o.property("listWidth") == 400)
s = dict(b.sessions(1, "2000-01-01", "2100-01-01")[0]) if b.sessions(1, "2000-01-01", "2100-01-01") else None
if s:
    QMetaObject.invokeMethod(sp, "openSession", Q_ARG("QVariant", s)); h.spin(500)
    back(h)
    check(section(h) == "sessions" and sp.property("editing") is False, "plan ouvert depuis la liste des séances : retour à la liste")
else:
    check(False, "aucune séance simulée")

print("ECHECS" if errs else "TOUT OK", errs)
