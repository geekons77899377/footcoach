"""Schéma 12 : éducateurs du club et éducateur responsable d'une équipe (imprimé sur la feuille de challenge).
Liste du club (ajout, modification, suppression avec confirmation), choix du responsable dans « Modifier l'équipe »,
entrée « Éducateurs du club » dans le menu de l'Effectif. Téléphone et bureau.

Backend SIMULÉ : ce test valide l'interface QML, pas la base SQLite (voir tests/tst_matches.cpp).
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, QObject
errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)

class B8(B2):
    def __init__(self):
        super().__init__()
        self._club_init(); self._edu_init()
        self._clubs[0]["name"] = "US Lagny-le-Sec"
        self.teams_ = [{"id": 1, "clubId": 1, "name": "U11", "category": "U11", "season": "2026-2027", "playerCount": 0, "educatorId": -1}]
        self.saved_teams = []
    def _names(self):
        clubs = {c["id"]: c["name"] for c in self._clubs}
        eds = {e["id"]: (e["lastName"].upper() + " " + e["firstName"]) for e in self._educators}
        return [dict(t, clubName=clubs.get(t["clubId"], ""), educatorName=eds.get(t["educatorId"], "")) for t in self.teams_]
    @Slot(result="QVariantList")
    def teams(self): return self._names()
    @Slot("QVariantMap", result="QVariantMap")
    def saveTeam(self, t):
        self.saved_teams.append(dict(t))
        for x in self.teams_:
            if x["id"] == int(t.get("id", -1)):
                x.update(name=t["name"], category=t.get("category", ""), season=t.get("season", ""),
                         educatorId=int(t.get("educatorId", -1)))
                return {"ok": True, "id": x["id"], "error": ""}
        return {"ok": False, "id": -1, "error": "?"}
    @Slot(int, bool, result="QVariantList")
    def players(self, team, inactive): return []
    @Slot(int, str, str, result="QStringList")
    def sessionDates(self, team, a, b): return []
    @Slot(int, str, str, result="QVariantList")
    def sessions(self, team, a, b): return []

def find(h, pred):
    for o in h.win.findChildren(QObject):
        try:
            if pred(o): return o
        except Exception: pass
def named(root, name):
    for o in root.findChildren(QObject):
        if o.objectName() == name: return o
def jv(v): return v.toVariant() if hasattr(v, "toVariant") else v
def button(root, text):
    for o in root.findChildren(QObject):
        if o.metaObject().className().startswith("AppButton") and o.property("text") == text and o.property("visible"): return o

for W, H_, tag in [(412, 892, "tel"), (1280, 800, "bureau")]:
    B8.mode = "none"; b = B8(); b.keep = []
    h = Harness(W, H_, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "squad"), Q_ARG("QVariant", -1)); h.spin(600)
    page = find(h, lambda o: o.metaObject().className().startswith("SquadPage"))
    team = jv(page.property("team"))

    # 1. Équipe sans éducateur dans le club : pas de choix, bouton d'ajout.
    dlg = find(h, lambda o: o.metaObject().className().startswith("TeamDialog"))
    QMetaObject.invokeMethod(dlg, "openFor", Q_ARG("QVariant", team), Q_ARG("QVariant", -1)); h.spin(400)
    chips = named(dlg, "educatorChips")
    check(chips.property("visible") is False, f"[{tag}] aucun éducateur : pas de choix de responsable")
    check(button(dlg, "Ajouter un éducateur responsable") is not None, f"[{tag}] bouton « Ajouter un éducateur responsable »")
    if tag == "tel": h.shot("edu_1_equipe_sans_educateur")

    # 2. Liste du club : nom obligatoire, ajout, modification.
    QMetaObject.invokeMethod(button(dlg, "Ajouter un éducateur responsable"), "clicked"); h.spin(400)
    eds = [o for o in dlg.findChildren(QObject) if o.objectName() == "educatorsDialog"][0]
    check(eds.property("visible") is True and eds.property("clubName") == "US Lagny-le-Sec", f"[{tag}] la liste des éducateurs du club s'ouvre")
    named(eds, "educatorFirstName").setProperty("text", "Karim")
    QMetaObject.invokeMethod(named(eds, "educatorSave"), "clicked"); h.spin(300)
    check("obligatoires" in eds.property("errorText") and not b._educators, f"[{tag}] nom manquant : refusé avec message")
    named(eds, "educatorLastName").setProperty("text", "Benali")
    named(eds, "educatorLicence").setProperty("text", "1234"); h.spin(100)
    check(named(eds, "educatorLicenceWarning").property("visible") is True, f"[{tag}] licence sans 10 chiffres : avertissement affiché")
    named(eds, "educatorLicence").setProperty("text", "23 1807 3452"); h.spin(100)
    check(named(eds, "educatorLicenceWarning").property("visible") is False, f"[{tag}] licence de 10 chiffres : pas d'avertissement")
    QMetaObject.invokeMethod(named(eds, "educatorSave"), "clicked"); h.spin(300)
    named(eds, "educatorFirstName").setProperty("text", "Paul")
    named(eds, "educatorLastName").setProperty("text", "Adam")
    QMetaObject.invokeMethod(named(eds, "educatorSave"), "clicked"); h.spin(300)
    lst = jv(eds.property("educators"))
    check([e["fullName"] for e in lst] == ["ADAM Paul", "BENALI Karim"], f"[{tag}] 2 éducateurs ajoutés, triés par nom ({[e['fullName'] for e in lst]})")
    check(eds.property("errorText") == "" and named(eds, "educatorFirstName").property("text") == "", f"[{tag}] formulaire vidé après l'ajout")
    if tag == "tel": h.shot("edu_2_liste_du_club")
    QMetaObject.invokeMethod(eds, "edit", Q_ARG("QVariant", lst[0])); h.spin(200)
    check(eds.property("editingId") == lst[0]["id"] and button(eds, "Enregistrer") is not None, f"[{tag}] toucher un éducateur le met en modification")
    named(eds, "educatorFirstName").setProperty("text", "Paulo")
    QMetaObject.invokeMethod(button(eds, "Enregistrer"), "clicked"); h.spin(300)
    check(any(e["firstName"] == "Paulo" for e in b._educators) and eds.property("editingId") == -1, f"[{tag}] modification enregistrée")
    QMetaObject.invokeMethod(eds, "close"); h.spin(300)

    # 3. Choix du responsable dans la fiche de l'équipe, enregistré avec l'équipe.
    check(chips.property("visible") is True and len(jv(chips.property("options"))) == 2, f"[{tag}] la fiche propose les 2 éducateurs du club")
    karim = [e for e in b._educators if e["lastName"] == "Benali"][0]["id"]
    QMetaObject.invokeMethod(chips, "picked", Q_ARG("QString", str(karim))); h.spin(200)
    if tag == "tel": h.shot("edu_3_choix_du_responsable")
    QMetaObject.invokeMethod(button(dlg, "Enregistrer"), "clicked"); h.spin(500)
    check(b.saved_teams and int(b.saved_teams[-1]["educatorId"]) == karim, f"[{tag}] l'équipe est enregistrée avec son responsable ({b.saved_teams[-1:]})")
    team = [t for t in jv(page.property("teams")) if t["id"] == 1][0]
    QMetaObject.invokeMethod(dlg, "openFor", Q_ARG("QVariant", team), Q_ARG("QVariant", -1)); h.spin(300)
    check(chips.property("value") == str(karim), f"[{tag}] rouverte, la fiche montre le responsable choisi")
    QMetaObject.invokeMethod(chips, "picked", Q_ARG("QString", "")); h.spin(150)
    QMetaObject.invokeMethod(button(dlg, "Enregistrer"), "clicked"); h.spin(400)
    check(int(b.saved_teams[-1]["educatorId"]) == -1, f"[{tag}] retoucher la pastille retire le responsable")

    # 4. Menu de l'Effectif : « Éducateurs du club », suppression avec confirmation.
    sh = find(h, lambda o: o.metaObject().className().startswith("ActionSheet") and o.metaObject().indexOfMethod("rebuild()") >= 0)
    QMetaObject.invokeMethod(sh, "rebuild"); h.spin(100)
    texts = [m["text"] for m in jv(sh.property("model"))]
    check("Éducateurs du club « US Lagny-le-Sec »" in texts, f"[{tag}] entrée « Éducateurs du club » dans le menu ({texts})")
    QMetaObject.invokeMethod(sh, "chosen", Q_ARG("QString", "educators")); h.spin(400)
    eds2 = [o for o in page.findChildren(QObject) if o.objectName() == "educatorsDialog" and o.property("visible")]
    check(len(eds2) == 1, f"[{tag}] le menu ouvre la liste des éducateurs")
    if eds2:
        e2 = eds2[0]
        conf = named(e2, "educatorConfirm")
        conf.setProperty("educatorId", karim)
        QMetaObject.invokeMethod(conf, "confirm", Q_ARG("QVariant", "Supprimer l'éducateur ?"), Q_ARG("QVariant", "x"),
                                 Q_ARG("QVariant", "Supprimer"), Q_ARG("QVariant", True)); h.spin(300)
        check(len(b._educators) == 2, f"[{tag}] rien n'est supprimé avant la confirmation")
        QMetaObject.invokeMethod(conf, "accepted", Q_ARG("QString", "")); h.spin(400)
        check([e["lastName"] for e in b._educators] == ["Adam"] and len(jv(e2.property("educators"))) == 1, f"[{tag}] suppression confirmée : l'éducateur disparaît de la liste")
    bad = [w for w in h.warnings if "Educator" in w or "TeamDialog" in w]
    check(not bad, f"[{tag}] aucun avertissement QML sur les fiches modifiées ({bad[:3]})")

print("\n%s" % ("ECHECS : %d" % len(errs) if errs else "TOUT PASSE"))
for e in errs: print("   KO ", e)
sys.exit(1 if errs else 0)
