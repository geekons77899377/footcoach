"""L-UX1 : « Modifier la séance » montre tout sur un seul écran : informations modifiables + plan d'exercices.

Backend SIMULÉ (comme les autres tests de ce dossier) : ce test valide le câblage QML (Calendrier -> section Séances,
SessionFields dans PlanEditor), pas l'écriture SQLite ; il ne remplace ni un essai sur appareil ni une validation Qt 6.9.
"""
exec(open('t_l0a.py').read().split("# === SCENA" + "RIOS ===")[0])

class B7(B4):
    def __init__(self):
        super().__init__()
        self.saved = []; self.added = []
        self.sess = {"id": 7, "teamId": 2, "date": datetime.datetime.now().strftime("%Y-%m-%d"), "time": "18:30",
                     "endTime": "20:00", "durationMin": 90, "location": "Stade", "theme": "Transitions", "notes": "N1",
                     "status": "planned", "present": -1, "total": -1}
    @Slot(result="QVariantList")
    def teams(self):
        return [{"id": 1, "name": "U15", "category": ""}, {"id": 2, "name": "U17", "category": ""}]
    @Slot(int, str, str, result="QVariantList")
    def sessions(self, team, a, b):
        return [dict(self.sess)] if team == self.sess["teamId"] else []
    @Slot("QVariantMap", bool, result="QVariantMap")
    def saveSession(self, s, force):
        self.saved.append(dict(s)); self.sess.update(s)
        return {"ok": True, "id": s["id"] if s["id"] > 0 else 99, "error": "", "needsConfirm": False}
    @Slot(int, int, int, result="QVariantMap")
    def addToPlan(self, sid, ex, dur):
        self.added.append((sid, ex)); return {"ok": True, "error": ""}

def runux():
    B7.status = "planned"
    b = B7(); b.keep = []
    h = Harness(412, 892, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "calendar"), Q_ARG("QVariant", -1)); h.spin(500)
    return h, b
def calendar_page(h): return [o for o in h.win.findChildren(QObject) if o.metaObject().indexOfMethod("editSession(QVariant)") >= 0][0]
def infos(h): return F_in(h.win.contentItem(), "sessionInfosCard") if False else [o for o in h.win.findChildren(QObject) if o.objectName() == "sessionInfosCard"][0]

# --- 1. « Modifier » ouvre un écran unique : informations + plan -----------------------------------------------------------
h, b = runux()
check(h.win.property("section") == "calendar", "départ : Calendrier")
QMetaObject.invokeMethod(calendar_page(h), "editSession", Q_ARG("QVariant", dict(b.sess))); h.spin(700)
check(h.win.property("section") == "sessions" and page(h).property("editing") is True,
      "Modifier : on arrive sur l'écran de la séance (section Séances), plan ouvert")
check(bool(infos(h).property("visible")), "les informations de la séance sont affichées au-dessus du plan")
check(len(plan(h).property("planData")["entries"]) > 0, "le plan d'exercices est affiché sur le même écran")
fields = [o for o in infos(h).findChildren(QObject) if o.property("sessionId") is not None][0]
check(fields.property("sessionId") == 7 and fields.property("teamId") == 2, "les champs sont chargés avec la séance choisie")
th = [o for o in infos(h).findChildren(QObject) if o.property("label") == "Thème"][0]
check(th.property("text") == "Transitions", f"thème chargé ({th.property('text')!r})")

# --- 2. Ajouter un exercice directement depuis cet écran ---------------------------------------------------------------------
add_btn = [o for o in h.win.findChildren(QObject) if o.property("accessibleName") == "Ajouter un exercice"]
check(len(add_btn) == 1 and bool(add_btn[0].property("visible")), "« Ajouter un exercice » est disponible directement")
QMetaObject.invokeMethod(add_btn[0], "clicked"); h.spin(500)
pk = [o for o in h.win.findChildren(QObject) if o.metaObject().indexOfMethod("pick()") >= 0][0]
check(bool(pk.property("opened")), "le sélecteur d'exercices s'ouvre")

# --- 3. Modifier les informations ici même : enregistrées, l'en-tête et la liste suivent ----------------------------------------
th.setProperty("text", "Jeu de possession")
QMetaObject.invokeMethod(fields, "save", Q_ARG("QVariant", False)); h.spin(500)
check(len(b.saved) == 1 and b.saved[0]["theme"] == "Jeu de possession" and b.saved[0]["id"] == 7,
      "le thème modifié est enregistré pour la bonne séance")
ed = plan(h)
def jv(v): return v.toVariant() if hasattr(v, "toVariant") else v
check(jv(ed.property("session"))["theme"] == "Jeu de possession", "l'écran affiche la séance enregistrée")
rows = jv(page(h).property("rows"))
check(len(rows) == 1 and rows[0]["theme"] == "Jeu de possession", "la liste des séances est mise à jour")

# --- 4. L'équipe de la séance est sélectionnée, et le retour revient à la liste -----------------------------------------------
sw = [o for o in page(h).findChildren(QObject) if o.property("teams") is not None and o.property("current") is not None][0]
check(jv(sw.property("current"))["id"] == 2, "l'équipe de la séance est sélectionnée")
back(h)
check(page(h).property("editing") is False and h.win.property("section") == "calendar", "retour : on revient au Calendrier, d'où l'on venait")

# --- 5. Le menu « Modifier » d'une séance du Calendrier suit le même chemin ------------------------------------------------------
h, b = runux()
rs = [o for o in h.win.findChildren(QObject) if o.property("target") is not None or (o.property("title") is not None and o.metaObject().indexOfMethod("openFor(QVariant)") >= 0)]
rs = [o for o in rs if o.metaObject().indexOfMethod("openFor(QVariant)") >= 0][0]
QMetaObject.invokeMethod(rs, "openFor", Q_ARG("QVariant", dict(b.sess))); h.spin(400)
QMetaObject.invokeMethod(rs, "chosen", Q_ARG("QString", "edit")); h.spin(700)
check(h.win.property("section") == "sessions" and page(h).property("editing") is True,
      "menu « Modifier » du Calendrier : écran unique (informations + plan)")
QMetaObject.invokeMethod(rs, "close"); h.spin(400)
h.shot("ux1_modifier_telephone")
h.resize(412, 1900); h.spin(500); h.shot("ux1_modifier_complet")

print("ECHECS" if errs else "TOUT OK", errs)
