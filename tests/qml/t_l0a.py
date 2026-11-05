"""L0-a : sortie contrôlée d'un terrain quand l'enregistrement échoue (AUD-01, décision « Option A »).

Le Backend est SIMULÉ (comme dans les autres tests de ce dossier) : saveBoard() peut échouer à la demande. Ce test valide
la logique QML de BoardEditor.leaveGuarded() et de ses hôtes (PlanEditor, TacticsPage, Main) ; l'échec d'écriture réel
de SQLite n'est pas exercé ici et ce test ne remplace ni un essai sur appareil ni une validation Qt 6.9.

Règles vérifiées : en cas d'échec on reste sur l'écran, l'erreur est affichée, l'utilisateur peut réessayer ou quitter
sans enregistrer (confirmation explicite) ; aucune perte silencieuse.
"""
exec(open('t_blockboard.py').read().split("# --- 1.")[0])

class B4(B3):
    fail_save = False
    fail_msg = "Disque plein"
    attempts = 0
    @Slot(int, str, result=bool)
    def saveBoard(self, i, js):
        if self.fail_save:
            self.attempts += 1
            self._err = self.fail_msg; self.lastErrorChanged.emit()
            return False
        return super().saveBoard(i, js)

def run4(w=412, hh=892):
    B4.status = "planned"
    b = B4(); b.keep = []
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "sessions"), Q_ARG("QVariant", -1)); h.spin(500)
    return h, b

def popup(h, name):
    for o in h.win.findChildren(QObject):
        if o.objectName() == name: return o
def is_open(h, name):
    p = popup(h, name); return p is not None and bool(p.property("opened"))
def n_items(b, bid): return len(json.loads(b._boards[bid]["json"])["items"])
def find_text(root, text):
    todo = [root]
    while todo:
        it = todo.pop()
        if it.property("text") == text and it.property("kind") is not None: return it
        todo.extend(it.childItems())
def tap_it(h, it):
    p = it.mapToScene(QPointF(it.width() / 2, it.height() / 2)); h.click(p.x(), p.y())
def tap_popup(h, popup_name, objname=None, text=None):
    p = popup(h, popup_name); root = p.property("contentItem")
    it = F_in(root, objname) if objname else find_text(root, text)
    tap_it(h, it); h.spin(350)
def F_in(root, name):
    todo = [root]
    while todo:
        it = todo.pop()
        if it.objectName() == name: return it
        todo.extend(it.childItems())
def add(ed, h, kind="player", x=0.5):
    QMetaObject.invokeMethod(ed.findChild(QObject, "board"), "addItem", Q_ARG("QVariant", kind), Q_ARG("QVariant", x),
                             Q_ARG("QVariant", 0.5), Q_ARG("QVariant", "9"))
def open_block(h, b, item=101):
    open_plan(h); tapname(h, f"openBoard_{item}"); h.spin(400)
    return editor_of(h), b.blocks[item]

# === SCENARIOS ===
# --- 1. Cas nominal : enregistrement réussi, aucune boîte de dialogue ---------------------------------------------------
h, b = run4(); ed, bid = open_block(h, b)
n0 = n_items(b, bid)
add(ed, h); check(ed.property("dirty") is True, "après une modification : des changements sont en attente")
h.spin(900)
check(ed.property("dirty") is False and n_items(b, bid) == n0 + 1, "enregistrement automatique réussi : plus rien en attente")
add(ed, h, "opponent", 0.7)
r = back(h)
check(r is True and plan(h).property("boardOpen") is False and not is_open(h, "saveFailedDialog"),
      "retour immédiat avec enregistrement réussi : le terrain se ferme sans dialogue")
check(n_items(b, bid) == n0 + 2, "aucune perte au retour immédiat")

# --- 2. Échec : on reste sur l'écran, l'erreur est affichée, réessayer ------------------------------------------------
h, b = run4(); ed, bid = open_block(h, b)
n0 = n_items(b, bid); b.fail_save = True
add(ed, h); h.spin(900)
check(ed.property("dirty") is True and n_items(b, bid) == n0, "échec de l'enregistrement automatique : modifications en attente, base inchangée")
r = back(h)
check(r is True and plan(h).property("boardOpen") is True, "retour pendant l'échec : le terrain reste ouvert")
check(is_open(h, "saveFailedDialog"), "le dialogue d'échec est affiché")
msg = F_in(popup(h, "saveFailedDialog").property("contentItem"), "saveFailedMessage").property("text")
check("Disque plein" in msg, f"l'erreur est affichée dans le dialogue ({msg!r})")
a0 = b.attempts
tap_popup(h, "saveFailedDialog", objname="saveRetryButton")
check(b.attempts > a0 and plan(h).property("boardOpen") is True and is_open(h, "saveFailedDialog"),
      "Réessayer : nouvel essai, toujours en échec -> on reste, le dialogue revient")
b.fail_save = False
tap_popup(h, "saveFailedDialog", objname="saveRetryButton")
check(plan(h).property("boardOpen") is False and n_items(b, bid) == n0 + 1,
      "Réessayer après rétablissement : modifications enregistrées, le terrain se ferme")

# --- 3. Quitter sans enregistrer : confirmation explicite ---------------------------------------------------------------
h, b = run4(); ed, bid = open_block(h, b)
n0 = n_items(b, bid); b.fail_save = True
add(ed, h); h.spin(900)
back(h)
tap_popup(h, "saveFailedDialog", objname="saveDiscardButton")
check(is_open(h, "discardConfirm") and plan(h).property("boardOpen") is True,
      "Quitter sans enregistrer : une confirmation est demandée, le terrain reste ouvert")
tap_popup(h, "discardConfirm", text="Annuler")
check(plan(h).property("boardOpen") is True and ed.property("dirty") is True,
      "Annuler la confirmation : rien n'est perdu, les modifications restent en attente")
back(h)
tap_popup(h, "saveFailedDialog", objname="saveDiscardButton")
tap_popup(h, "discardConfirm", text="Quitter sans enregistrer")
check(plan(h).property("boardOpen") is False and n_items(b, bid) == n0,
      "Quitter sans enregistrer, confirmé : le terrain se ferme, la base garde la dernière version enregistrée")

# --- 4. Rester sur le terrain ---------------------------------------------------------------------------------------------
h, b = run4(); ed, bid = open_block(h, b)
n0 = n_items(b, bid); b.fail_save = True
add(ed, h); h.spin(900); back(h)
tap_popup(h, "saveFailedDialog", objname="saveStayButton")
shown = json.loads(QMetaObject.invokeMethod(ed.findChild(QObject, "board"), "toJson", Qt.DirectConnection, Q_RETURN_ARG("QVariant")))
check(plan(h).property("boardOpen") is True and len(shown["items"]) == n0 + 1 and ed.property("dirty") is True,
      "Rester : le terrain garde les modifications non enregistrées")
b.fail_save = False
back(h)
check(plan(h).property("boardOpen") is False and n_items(b, bid) == n0 + 1,
      "retour une fois le problème résolu : enregistré puis fermé, sans dialogue")

# --- 5. Autres chemins de sortie : changement de section, fermeture du plan, fermeture de la fenêtre ---------------------
h, b = run4(); ed, bid = open_block(h, b)
n0 = n_items(b, bid); b.fail_save = True
add(ed, h); h.spin(900)
QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "home"), Q_ARG("QVariant", -1)); h.spin(300)
check(h.win.property("section") == "sessions" and is_open(h, "saveFailedDialog"),
      "navigation vers une autre section : refusée pendant l'échec, dialogue affiché")
b.fail_save = False
tap_popup(h, "saveFailedDialog", objname="saveRetryButton")
check(h.win.property("section") == "home" and n_items(b, bid) == n0 + 1,
      "après Réessayer : enregistré, puis la navigation reprend")

h, b = run4(); ed, bid = open_block(h, b)
n0 = n_items(b, bid); b.fail_save = True
add(ed, h); h.spin(900)
QMetaObject.invokeMethod(page(h), "closeEditor"); h.spin(300)
check(page(h).property("editing") is True and plan(h).property("boardOpen") is True and is_open(h, "saveFailedDialog"),
      "fermeture du plan depuis la page : refusée pendant l'échec")
tap_popup(h, "saveFailedDialog", objname="saveStayButton")

h, b = run4(); ed, bid = open_block(h, b)
n0 = n_items(b, bid); b.fail_save = True
add(ed, h); h.spin(900)
h.win.close(); h.spin(300)
check(h.win.isVisible() and is_open(h, "saveFailedDialog"),
      "fermeture de la fenêtre : refusée pendant l'échec, dialogue affiché (Component.onDestruction n'est pas utilisé)")
b.fail_save = False
tap_popup(h, "saveFailedDialog", objname="saveRetryButton")
check(n_items(b, bid) == n0 + 1 and not h.win.isVisible(),
      "Réessayer : enregistré, puis la fenêtre se ferme")

# --- 6. Page Tactique : ouvrir un autre schéma, dupliquer ---------------------------------------------------------------------
h, b = run4()
A = b.add("Schéma A", PLAYERS_HALF); Bd = b.add("Schéma B", PLAYERS_HALF)
QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "tactics"), Q_ARG("QVariant", A)); h.spin(500)
tp = find_obj(h, "pendingDelete"); ed = tp.findChild(QObject, "editor")
check(tp.property("currentId") == A, "page Tactique : schéma A ouvert")
b.fail_save = True
add(ed, h); h.spin(900)
QMetaObject.invokeMethod(tp, "openBoard", Q_ARG("QVariant", Bd)); h.spin(300)
check(tp.property("currentId") == A and is_open(h, "saveFailedDialog"), "ouvrir un autre schéma : refusé pendant l'échec")
tap_popup(h, "saveFailedDialog", objname="saveStayButton")
n_boards = len(b._boards)
QMetaObject.invokeMethod(ed.findChild(QObject, "moreSheet"), "chosen", Q_ARG("QString", "duplicate")); h.spin(300)
check(len(b._boards) == n_boards and is_open(h, "saveFailedDialog"), "dupliquer : refusé pendant l'échec (aucune copie créée)")
b.fail_save = False
tap_popup(h, "saveFailedDialog", objname="saveRetryButton")
check(len(b._boards) == n_boards + 1 and n_items(b, A) == 3, "Réessayer : enregistré, puis la copie est créée")


# --- 7. Fermeture de la fenêtre : UNE seule tentative d'enregistrement avant le dialogue -------------------------------
h, b = run4(); ed, bid = open_block(h, b)
n0 = n_items(b, bid); b.fail_save = True
add(ed, h); h.spin(900)
a0 = b.attempts
h.win.close(); h.spin(300)
check(is_open(h, "saveFailedDialog") and b.attempts == a0 + 1,
      f"fermeture de la fenêtre en échec : une seule tentative d'enregistrement ({b.attempts - a0})")
tap_popup(h, "saveFailedDialog", objname="saveStayButton")
check(h.win.isVisible() and plan(h).property("boardOpen") is True, "Rester : la fenêtre reste ouverte")
b.fail_save = False
h.win.close(); h.spin(300)
check(not h.win.isVisible() and n_items(b, bid) == n0 + 1, "problème résolu : un nouvel essai enregistre puis la fenêtre se ferme")

# --- 8. Échap = « Rester sur le terrain » : aucune sortie obsolète n'est exécutée plus tard -------------------------------
h, b = run4(); ed, bid = open_block(h, b)
n0 = n_items(b, bid); b.fail_save = True
add(ed, h); h.spin(900)
back(h)
check(is_open(h, "saveFailedDialog"), "échec : dialogue affiché")
QMetaObject.invokeMethod(popup(h, "saveFailedDialog"), "close"); h.spin(500)   # fermeture sans choix, comme Échap
check(not is_open(h, "saveFailedDialog") and plan(h).property("boardOpen") is True and ed.property("dirty") is True,
      "Échap : dialogue fermé, on reste sur le terrain, modifications conservées")
check(ed.property("exitAction") is None, "Échap : la sortie en attente est abandonnée (exitAction vidé)")
b.fail_save = False
h.spin(900)
check(plan(h).property("boardOpen") is True, "problème résolu ensuite : rien ne se ferme tout seul (pas de sortie obsolète)")
ed.setProperty("dirty", True)
QMetaObject.invokeMethod(ed, "flush", Qt.DirectConnection, Q_RETURN_ARG("QVariant")); h.spin(300)
check(plan(h).property("boardOpen") is True and n_items(b, bid) == n0 + 1,
      "un enregistrement réussi plus tard enregistre sans exécuter l'ancienne sortie")

h, b = run4(); ed, bid = open_block(h, b)
b.fail_save = True
add(ed, h); h.spin(900); back(h)
tap_popup(h, "saveFailedDialog", objname="saveDiscardButton")
check(is_open(h, "discardConfirm"), "Quitter sans enregistrer : confirmation demandée")
QMetaObject.invokeMethod(popup(h, "discardConfirm"), "close"); h.spin(500)
check(not is_open(h, "discardConfirm") and plan(h).property("boardOpen") is True and ed.property("dirty") is True and ed.property("exitAction") is None,
      "Échap sur la confirmation : on reste sur le terrain, aucune sortie en attente")
b.fail_save = False
back(h)
check(plan(h).property("boardOpen") is False and not is_open(h, "discardConfirm"),
      "retour ensuite : enregistré puis fermé, sans réutiliser l'ancienne confirmation")

# --- 9. Changement d'équipe / de période pendant un échec : on reste dans le contexte du terrain ----------------------------
class B6(B4):
    @Slot(result="QVariantList")
    def teams(self):
        return [{"id": 1, "name": "U15", "category": ""}, {"id": 2, "name": "U17", "category": ""}]
def team_sheet(h):
    for o in page(h).findChildren(QObject):
        if o.property("title") == "Équipe" and o.property("model") is not None: return o
def current_team(h):
    sw = [o for o in page(h).findChildren(QObject) if o.property("teams") is not None and o.property("current") is not None]
    cur = sw[0].property("current") if sw else None
    if cur is None: return None
    cur = cur.toVariant() if hasattr(cur, "toVariant") else cur
    return cur.get("id") if isinstance(cur, dict) else None

B6.status = "planned"
b = B6(); b.keep = []
h = Harness(412, 892, backend=b)
QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "sessions"), Q_ARG("QVariant", -1)); h.spin(500)
check(current_team(h) == 1, f"équipe de départ : 1 ({current_team(h)})")
ed, bid = open_block(h, b)
n0 = n_items(b, bid); b.fail_save = True
add(ed, h); h.spin(900)
QMetaObject.invokeMethod(team_sheet(h), "chosen", Q_ARG("QString", "t2")); h.spin(400)
check(current_team(h) == 1, "changement d'équipe refusé pendant l'échec : l'équipe n'a pas changé")
check(plan(h).property("boardOpen") is True and is_open(h, "saveFailedDialog"), "le terrain reste ouvert, dialogue affiché")
tap_popup(h, "saveFailedDialog", objname="saveStayButton")
check(current_team(h) == 1 and plan(h).property("boardOpen") is True and ed.property("dirty") is True,
      "Rester : équipe inchangée, terrain ouvert, modifications en attente (enregistrables dans le bon contexte)")
b.fail_save = False
h.spin(900)
# la sauvegarde se fait sur le terrain d'origine (identifiant du terrain), l'équipe n'y intervient pas
QMetaObject.invokeMethod(ed, "flush", Qt.DirectConnection, Q_RETURN_ARG("QVariant")); h.spin(300)
check(n_items(b, bid) == n0 + 1, "le terrain est enregistré sur le bon bloc")
QMetaObject.invokeMethod(team_sheet(h), "chosen", Q_ARG("QString", "t2")); h.spin(500)
check(current_team(h) == 2 and plan(h).property("boardOpen") is False, "une fois enregistré : l'équipe change et le plan se ferme")

b = B6(); b.keep = []
h = Harness(412, 892, backend=b)
QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "sessions"), Q_ARG("QVariant", -1)); h.spin(500)
ed, bid = open_block(h, b)
b.fail_save = True
add(ed, h); h.spin(900)
pg = page(h)
sc_obj = [o for o in page(h).findChildren(QObject) if o.property("allowClear") is False and o.property("options") is not None]
QMetaObject.invokeMethod(sc_obj[0], "picked", Q_ARG("QString", "past")); h.spin(400)
check(pg.property("scope") == "upcoming" and plan(h).property("boardOpen") is True and is_open(h, "saveFailedDialog"),
      "changement de période refusé pendant l'échec : la liste ne change pas sous le terrain ouvert")
tap_popup(h, "saveFailedDialog", objname="saveStayButton")

print("ECHECS" if errs else "TOUT OK", errs)
