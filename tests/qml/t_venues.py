"""Schéma 11 : stades des matchs. Paramètres > Stades des matchs (département, mise à jour depuis Data ES, stade du club,
terrains indisponibles, lieux saisis à la main) et lieu du match dans la fiche (proposition automatique, choix dans la
liste, saisie libre).

Backend SIMULÉ : la réponse de Data ES est l'extrait réel tests/fixtures/data-es-oise-extrait.json, sans réseau.
Le C++ réel (base, mise à jour, export) est testé par tests/tst_venues.cpp ; ni le vrai téléphone ni Qt 6.9 ne sont
couverts ici.
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

class B7(B2):
    @Slot(int, bool, result="QVariantList")
    def players(self, team, inactive): return []
    @Slot(result="QVariantList")
    def teams(self):
        return [{"id": 1, "clubId": 1, "clubName": "Mon club", "name": "U11", "category": "U11", "playerCount": 0}]
    @Slot(int, str, str, result="QVariantList")
    def sessions(self, team, a, b): return []
    @Slot(int, str, str, result="QStringList")
    def sessionDates(self, team, a, b): return []

def make(w=412, hh=892):
    B7.mode = "none"; b = B7(); b.keep = []
    b._club_init(); b._venue_init()
    h = Harness(w, hh, backend=b)
    h.spin(500)
    return h, b
def go(h, sec): QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", sec), Q_ARG("QVariant", -1)); h.spin(500)
def back(h):
    r = QMetaObject.invokeMethod(h.win, "handleBack", Qt.DirectConnection, Q_RETURN_ARG("QVariant")); h.spin(400); return r
def find(h, pred):
    for o in h.win.findChildren(QObject):
        try:
            if pred(o): return o
        except Exception: pass
def cls(h, prefix): return find(h, lambda o: o.metaObject().className().startswith(prefix))
def named(h, name): return find(h, lambda o: o.objectName() == name)
def call(o, m, *a): QMetaObject.invokeMethod(o, m, *[Q_ARG("QVariant", x) for x in a])
def sig(o, m, text): QMetaObject.invokeMethod(o, m, Q_ARG("QString", text))
def jv(v): return v.toVariant() if hasattr(v, "toVariant") else v
def wait_refresh(h, b, ms=3000):
    end = time.time() + ms / 1000
    while b.venuesBusy and time.time() < end: h.spin(50)
    h.spin(300)

def open_venues(h):
    go(h, "settings")
    sp = cls(h, "SettingsPage")
    call(sp, "openVenues"); h.spin(500)
    return sp, named(h, "venuesPage")

# --- 1. Paramètres : entrée « Stades des matchs », liste vide --------------------------------------------------------
h, b = make()
go(h, "settings")
row = named(h, "rowVenues")
check(row is not None and row.property("visible") is True, "Paramètres : rubrique « Stades des matchs »")
check("Charger" in row.property("subtitle"), f"liste vide : invitation à la charger ({row.property('subtitle')})")
sp, vp = open_venues(h)
check(sp.property("mode") == "venues" and vp.property("visible") is True, "la page des stades s'ouvre")
check(named(h, "venuesEmpty").property("visible") is True, "liste vide : message pour la charger")
h.shot("venues_1_vide_telephone")

# --- 2. Sans département : pas d'appel ; département invalide refusé ----------------------------------------------------
call(vp, "startRefresh"); h.spin(300)
check(getattr(b, "refresh_calls", 0) == 0, "sans département : aucune mise à jour lancée")
dep = named(h, "departmentField")
dep.setProperty("text", "20"); call(vp, "saveDepartment"); h.spin(200)
check(b._dept.get(1, "") == "", "département 20 refusé (la Corse est 2A ou 2B)")

# --- 3. Mise à jour depuis Data ES (simulée) -------------------------------------------------------------------------
dep.setProperty("text", "60")
call(vp, "startRefresh"); h.spin(100)
check(b._dept.get(1) == "60", "le département est enregistré avant la mise à jour")
check(b.venuesBusy is True and named(h, "refreshButton").property("text") == "Téléchargement…", "téléchargement en cours affiché")
check(named(h, "cancelRefresh").property("visible") is True, "bouton Annuler visible pendant le téléchargement")
h.shot("venues_2_telechargement")
wait_refresh(h, b)
res = named(h, "refreshResult").property("text")
check("6 terrains reçus" in res and "6 nouveaux" in res, f"bilan affiché ({res})")
check(len(jv(vp.property("shown"))) == 6, "6 stades dans la liste")
check("Dernière mise à jour" in named(h, "venueSummary").property("text"), "date de la dernière mise à jour affichée")
h.shot("venues_3_liste_telephone")

# recherche sans accents ni majuscules
vp.property("searchField").setProperty("text", "nanteuil honneur"); h.spin(200)
shown = jv(vp.property("shown"))
check(len(shown) == 1 and "honneur" in shown[0]["field"].lower(), f"recherche « nanteuil honneur » : 1 stade ({[s['field'] for s in shown]})")
vp.property("searchField").setProperty("text", "lagny"); h.spin(200)
lagny = jv(vp.property("shown"))[0]
vp.property("searchField").setProperty("text", ""); h.spin(200)

# --- 4. Stade du club, terrain indisponible (feuille d'actions) ------------------------------------------------------
acts = named(h, "venueActions")
vp.setProperty("actionVenue", lagny); sig(acts, "chosen", "home"); h.spin(300)
check("Lagny-le-Sec" in named(h, "homeLabel").property("text"), f"stade du club choisi ({named(h, 'homeLabel').property('text')})")
silly = [v for v in jv(vp.property("allVenues")) if v["city"] == "Silly-le-Long"][0]
vp.setProperty("actionVenue", silly); sig(acts, "chosen", "unavailable"); h.spin(300)
check(len(jv(vp.property("shown"))) == 5, "terrain marqué indisponible : plus dans « Proposés »")
named(h, "venueFilter").setProperty("value", "unavailable"); sig(named(h, "venueFilter"), "picked", "unavailable"); h.spin(200)
shown = jv(vp.property("shown"))
check(len(shown) == 1 and shown[0]["city"] == "Silly-le-Long", "« Non proposés » : le terrain indisponible")
h.shot("venues_4_non_proposes")
sig(named(h, "venueFilter"), "picked", "available"); h.spin(200)

# --- 5. Mise à jour en échec : rien ne change ; terrain retiré de Data ES -------------------------------------------------
b.venue_refresh_answer = "error"
call(vp, "startRefresh"); wait_refresh(h, b)
check("Impossible de joindre" in named(h, "refreshResult").property("text") and len(b._venues) == 6,
      "sans connexion : message clair, liste inchangée")
b.venue_refresh_answer = [r for r in DATA_ES if r["new_name"] != "Le Plessis-Belleville"]
call(vp, "startRefresh"); wait_refresh(h, b)
res = named(h, "refreshResult").property("text")
check("1 retiré de la liste officielle" in res, f"terrain disparu de Data ES signalé ({res})")
check(len(b._venues) == 6 and len(jv(vp.property("shown"))) == 4, "rien n'est supprimé ; le terrain retiré n'est plus proposé")
check("Silly-le-Long" not in [v["city"] for v in jv(vp.property("shown"))], "la mise à jour ne revient pas sur « indisponible »")
# annulation
b.venue_refresh_delay = 2000
call(vp, "startRefresh"); h.spin(100); call(named(h, "cancelRefresh"), "clicked"); h.spin(300)
check(b.venuesBusy is False and "annulée" in named(h, "refreshResult").property("text"), "téléchargement annulé")
b.venue_refresh_delay = 300

# --- 6. Lieu saisi à la main --------------------------------------------------------------------------------------------
form = named(h, "manualVenueForm")
call(form, "openNew"); h.spin(300)
named(h, "venueNameField").setProperty("text", "Gymnase Lucien")
named(h, "venueAddressField").setProperty("text", "2 rue des Écoles")
named(h, "venueCpField").setProperty("text", "60330")
named(h, "venueCityField").setProperty("text", "Lagny-le-Sec")
h.shot("venues_5_lieu_a_la_main")
call(named(h, "saveVenueButton"), "clicked"); h.spin(300)
manual = [v for v in b._venues.values() if v["source"] == "manual"]
check(len(manual) == 1 and form.property("visible") is False, "lieu saisi à la main enregistré")
check(back(h) is True and sp.property("mode") == "browse", "retour : page Paramètres")
check("stades proposés" in named(h, "rowVenues").property("subtitle"), f"résumé dans Paramètres ({named(h, 'rowVenues').property('subtitle')})")

# --- 7. Fiche du match : lieu proposé, choisi, tapé à la main ------------------------------------------------------------
go(h, "matches")
page = cls(h, "MatchesPage"); ed = cls(h, "MatchEditor")
call(page, "newMatch", D(4)); h.spin(500)
loc = named(h, "locationField"); hint = named(h, "venueHint")
check("Lagny-le-Sec" in loc.property("text") and ed.property("venueId") == lagny["id"], f"domicile : stade du club proposé ({loc.property('text')})")
check(hint.property("visible") is True and hint.property("text") == "Stade du club", "origine affichée sous le champ")
check(ed.property("dirty") is False, "la proposition ne compte pas comme une modification")
h.shot("venues_6_match_domicile")

named(h, "opponentField").setProperty("text", "US Nanteuil")
sig(named(h, "placeChips"), "picked", "away"); named(h, "placeChips").setProperty("value", "away"); h.spin(300)
check(loc.property("text") == "" and ed.property("venueId") == -1, "extérieur, adversaire inconnu : le stade du club est retiré")

sheet = named(h, "venueSheet")
call(loc, "actionClicked"); h.spin(400)
check(sheet.property("visible") is True and len(jv(sheet.property("allVenues"))) == 5, "le bouton du champ ouvre la liste des stades proposés")
named(h, "venueSearch").setProperty("text", "honneur"); h.spin(200)
pick = jv(sheet.property("shown"))
check(len(pick) == 1, "recherche dans la liste")
h.shot("venues_7_choix_stade")
QMetaObject.invokeMethod(sheet, "picked", Q_ARG("QVariant", pick[0])); call(sheet, "close"); h.spin(300)
check(ed.property("venueId") == pick[0]["id"] and "Nanteuil-le-Haudouin" in loc.property("text"), "stade choisi : lieu rempli")
call(ed, "save"); h.spin(300)
saved = b.last_match_saved
check(saved["venueId"] == pick[0]["id"] and saved["homeAway"] == "away", "le lieu choisi est envoyé à l'enregistrement")

# lieu modifié à la main : le lien avec le stade est retiré
loc.setProperty("text", loc.property("text") + " (entrée parking)"); h.spin(100)
check(ed.property("venueId") == -1 and hint.property("visible") is False, "texte modifié à la main : plus de lien avec le stade")

# nouveau match contre le même adversaire : son stade est proposé
call(ed, "save"); h.spin(300)
check(b.last_match_saved["venueId"] == -1, "texte modifié puis enregistré : le match n'a plus de stade lié")
call(page, "newMatch", D(20)); h.spin(500)
check(ed.property("isNew") is True and ed.property("dirty") is False, "nouveau match ouvert")
sig(named(h, "placeChips"), "picked", "away"); named(h, "placeChips").setProperty("value", "away"); h.spin(300)
named(h, "opponentField").setProperty("text", "U.S. NANTEUIL"); call(named(h, "opponentField"), "editingFinished"); h.spin(300)
check(ed.property("venueId") == pick[0]["id"] and hint.property("text") == "Stade retenu pour cet adversaire",
      f"même adversaire (écrit autrement) : son stade est proposé ({ed.property('venueId')}, {hint.property('text')})")
h.shot("venues_8_match_adversaire_connu")
# un lieu tapé à la main n'est jamais remplacé par une proposition
loc.setProperty("text", "Terrain du lycée"); sig(named(h, "placeChips"), "picked", "home"); named(h, "placeChips").setProperty("value", "home"); h.spin(300)
check(loc.property("text") == "Terrain du lycée" and ed.property("venueId") == -1, "lieu tapé à la main : pas remplacé par le stade du club")

# --- 8. Tablette et bureau : la page des stades -----------------------------------------------------------------------
for w, hh, tag in [(800, 1280, "tablette"), (1280, 800, "bureau")]:
    h2, b2 = make(w, hh)
    b2._dept[1] = "60"; b2._apply_data_es(1, DATA_ES)
    sp2, vp2 = open_venues(h2)
    check(len(jv(vp2.property("shown"))) == 6, f"{tag} : liste affichée")
    h2.shot("venues_9_" + tag)

print("\n%s" % ("ÉCHECS : %d" % len(errs) if errs else "TOUT PASSE"))
for e in errs: print("   KO ", e)
sys.exit(1 if errs else 0)
