"""L0-c : suppression d'un terrain en échec (AUD-07) : l'accès au terrain est conservé et l'erreur est visible.

Le Backend est SIMULÉ : deleteBoard() peut échouer à la demande. Ce test valide la logique QML de PlanEditor et de
TacticsPage ; l'échec réel de SQLite n'est pas exercé ici (voir tests/tst_board.cpp pour le dépôt de données) et ce test ne
remplace ni un essai sur appareil ni une validation Qt 6.9.
"""
exec(open('t_l0a.py').read().split("# === SCENA" + "RIOS ===")[0])

class B5(B4):
    fail_delete = False
    @Slot(int, result=bool)
    def deleteBoard(self, i):
        if self.fail_delete:
            self._err = "Base verrouillée"; self.lastErrorChanged.emit()
            return False
        return super().deleteBoard(i)

def run5(w=412, hh=892):
    B5.status = "planned"
    b = B5(); b.keep = []
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "sessions"), Q_ARG("QVariant", -1)); h.spin(500)
    return h, b

def toast_text(h, start):
    for o in h.win.findChildren(QObject):
        m = o.property("message")
        if isinstance(m, str) and m.startswith(start) and bool(o.property("visible")):
            return m
def dialog_with_title(h, title):
    for o in h.win.findChildren(QObject):
        if o.property("title") == title and o.property("okText") is not None: return o

# --- 1. Terrain d'un bloc (PlanEditor) : échec de suppression -----------------------------------------------------------------
h, b = run5(); ed, bid = open_block(h, b)
n0 = n_items(b, bid); b.fail_delete = True
QMetaObject.invokeMethod(ed.findChild(QObject, "moreSheet"), "chosen", Q_ARG("QString", "delete")); h.spin(300)
check(is_open(h, "removeBoardConfirm"), "« Retirer le terrain » : confirmation demandée")
tap_popup(h, "removeBoardConfirm", text="Retirer")
check(plan(h).property("boardOpen") is True and plan(h).property("openBoardId") == bid,
      "suppression en échec : le terrain reste ouvert (accès conservé)")
check(editor_of(h) is ed and ed.property("boardId") == bid, "l'éditeur garde son terrain (pas d'identifiant perdu)")
check(bid in b._boards, "le terrain existe toujours dans la base")
t = toast_text(h, "Impossible de retirer le terrain")
check(t is not None and "Base verrouillée" in t, f"l'erreur est affichée ({t!r})")
add(ed, h); h.spin(900)
check(n_items(b, bid) == n0 + 1 and ed.property("dirty") is False,
      "après l'échec, le terrain reste modifiable et s'enregistre toujours")

# --- 2. Même terrain : la suppression réussit une fois le problème levé --------------------------------------------------------
b.fail_delete = False
QMetaObject.invokeMethod(ed.findChild(QObject, "moreSheet"), "chosen", Q_ARG("QString", "delete")); h.spin(300)
tap_popup(h, "removeBoardConfirm", text="Retirer")
check(plan(h).property("boardOpen") is False and bid not in b._boards,
      "suppression réussie : le terrain est retiré et le plan est affiché")
check(vis(h, "blockBoardEmpty_101"), "le bloc n'a plus de terrain (état vide)")

# --- 3. Page Tactique : échec de suppression d'un schéma ------------------------------------------------------------------------
h, b = run5()
A = b.add("Schéma A", PLAYERS_HALF); Bd = b.add("Schéma B", PLAYERS_HALF)
QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "tactics"), Q_ARG("QVariant", A)); h.spin(500)
tp = find_obj(h, "pendingDelete"); ed = tp.findChild(QObject, "editor")
b.fail_delete = True
QMetaObject.invokeMethod(ed.findChild(QObject, "moreSheet"), "chosen", Q_ARG("QString", "delete")); h.spin(300)
dlg = dialog_with_title(h, "Supprimer le schéma ?")
check(dlg is not None and bool(dlg.property("opened")), "« Supprimer le schéma ? » : confirmation demandée")
tap_it(h, find_text(dlg.property("contentItem"), "Supprimer")); h.spin(400)
check(tp.property("currentId") == A and A in b._boards, "suppression en échec : le schéma reste ouvert et existe toujours")
t = toast_text(h, "Impossible de supprimer le schéma")
check(t is not None and "Base verrouillée" in t, f"l'erreur est affichée, plus d'échec silencieux ({t!r})")
add(ed, h); h.spin(900)
check(n_items(b, A) == 3 and ed.property("dirty") is False, "le schéma reste modifiable et s'enregistre toujours")

# --- 4. Suppression réussie avec des modifications en attente : aucune garde résiduelle -------------------------------------------
b.fail_delete = False; b.fail_save = True
add(ed, h); h.spin(900)
check(ed.property("dirty") is True, "modification non enregistrable en attente avant la suppression")
QMetaObject.invokeMethod(ed.findChild(QObject, "moreSheet"), "chosen", Q_ARG("QString", "delete")); h.spin(300)
dlg = dialog_with_title(h, "Supprimer le schéma ?")
tap_it(h, find_text(dlg.property("contentItem"), "Supprimer")); h.spin(400)
check(tp.property("currentId") == -1 and A not in b._boards, "suppression réussie : le schéma est supprimé")
check(ed.property("dirty") is False, "les modifications destinées au schéma supprimé ne bloquent plus rien")
QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "home"), Q_ARG("QVariant", -1)); h.spin(300)
check(h.win.property("section") == "home" and not is_open(h, "saveFailedDialog"), "la navigation n'est pas bloquée par un schéma supprimé")

print("ECHECS" if errs else "TOUT OK", errs)
