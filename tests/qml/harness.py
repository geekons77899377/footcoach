"""Banc d'essai QML hors écran : charge les vrais fichiers qml/ avec un Backend simulé (mêmes méthodes que Backend.cpp)."""
import json, os, shutil, sys, time
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
os.environ.setdefault("QT_QUICK_BACKEND", "software")
os.environ.setdefault("QT_QUICK_CONTROLS_STYLE", "Material")
from PySide6.QtCore import QObject, Slot, Property, Signal, QUrl, Qt, QPointF, QTimer
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtQuick import QQuickWindow
from PySide6.QtTest import QTest

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.environ.get("FC_QML", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "qml"))
WORK = os.path.join(HERE, ".work")
RUN = os.path.join(WORK, "run")
OUT = os.path.join(WORK, "out")
FORM = json.load(open(os.path.join(HERE, "formations.json")))

def prepare():
    if os.path.isdir(RUN):
        shutil.rmtree(RUN)
    os.makedirs(RUN)
    for f in os.listdir(SRC):
        p = os.path.join(SRC, f)
        if os.path.isdir(p):
            shutil.copytree(p, os.path.join(RUN, f))      # qml/fonts
        else:
            shutil.copy(p, RUN)
    with open(os.path.join(RUN, "qmldir"), "w") as q:
        q.write("singleton Theme 1.0 Theme.qml\nsingleton AppState 1.0 AppState.qml\n")
    os.makedirs(OUT, exist_ok=True)

class Backend(QObject):
    lastErrorChanged = Signal()
    dataReplaced = Signal()
    def __init__(self):
        super().__init__()
        self._boards = {}
        self._next = 1
        self._err = ""
        self.saves = 0
    @Property(str, notify=lastErrorChanged)
    def lastError(self): return self._err
    def add(self, title, js="{}"):
        i = self._next; self._next += 1
        self._boards[i] = {"id": i, "title": title, "json": js, "updatedAt": "2026-10-29 10:00:00"}
        return i
    @Slot(result="QVariantList")
    def boards(self):
        out = []
        for b in sorted(self._boards.values(), key=lambda b: b["title"].lower()):
            try:
                d = json.loads(b["json"]); ok = True
                if d == {} : n = 0
                else: n = len(d.get("items", []))
            except Exception:
                ok = False; n = 0
            out.append({"id": b["id"], "title": b["title"], "updatedAt": b["updatedAt"], "readable": ok,
                        "itemCount": n, "json": b["json"] if ok else "", "exerciseId": -1, "sessionId": -1})
        return out
    @Slot(int, result="QVariantMap")
    def board(self, i):
        b = self._boards.get(i)
        if not b: return {}
        try:
            json.loads(b["json"]); ok = True; err = ""
        except Exception as e:
            ok = False; err = str(e)
        return {"id": i, "title": b["title"], "readable": ok, "error": err, "json": b["json"] if ok else ""}
    @Slot(result="QVariantList")
    def categories(self): return []
    @Slot(str, result="QVariantMap")
    def addCategory(self, name): return {"ok": False, "error": "Non simulé."}
    @Slot(int, result="QVariantMap")
    def exerciseBoard(self, ex): return {"boardId": 0}
    @Slot(int, result="QVariantMap")
    def createExerciseBoard(self, ex): return {"ok": False, "error": "Non simulé."}
    @Slot(str, result=int)
    def createBoard(self, title): return self.add(title.strip() or "Nouveau schéma")
    @Slot(int, result=int)
    def duplicateBoard(self, i):
        b = self._boards.get(i)
        return self.add(b["title"] + " (copie)", b["json"]) if b else -1
    @Slot(int, str, result=bool)
    def renameBoard(self, i, t):
        if not t.strip() or i not in self._boards: return False
        self._boards[i]["title"] = t.strip(); return True
    @Slot(int, result=bool)
    def deleteBoard(self, i): return self._boards.pop(i, None) is not None
    @Slot(int, str, result=bool)
    def saveBoard(self, i, js):
        if i not in self._boards: return False
        try: json.loads(js)
        except Exception:
            self._err = "json"; self.lastErrorChanged.emit(); return False
        self._boards[i]["json"] = js; self.saves += 1; return True

    @Slot(int, bool, result="QVariantList")
    def players(self, team, inactive):
        return []      # les tests qui ont besoin d'un effectif redéfinissent cette méthode
    # --- Clubs et matchs (simulés : mêmes clés que src/app/BackendMatches.cpp et Backend.cpp) -----------------
    # Les tests qui définissent leurs propres équipes (teams) n'ont rien à changer : un club « Mon club » existe d'office.
    def _club_init(self):
        if not hasattr(self, "_clubs"):
            self._clubs = [{"id": 1, "name": "Mon club"}]
            self._matches = {}
            self._match_players = {}
            self._match_board = {}
            self._next_match = 1
            self._next_club = 2
    @Slot(result="QVariantList")
    def clubs(self):
        self._club_init()
        return [dict(c, teamCount=1) for c in self._clubs]
    @Slot("QVariantMap", result="QVariantMap")
    def saveClub(self, c):
        self._club_init()
        name = str(c.get("name", "")).strip()
        if not name: return {"ok": False, "id": -1, "error": "Le nom du club est obligatoire."}
        cid = int(c.get("id", -1))
        if cid > 0:
            for x in self._clubs:
                if x["id"] == cid: x["name"] = name; return {"ok": True, "id": cid, "error": ""}
            return {"ok": False, "id": -1, "error": "Club introuvable."}
        cid = self._next_club; self._next_club += 1
        self._clubs.append({"id": cid, "name": name})
        return {"ok": True, "id": cid, "error": ""}
    @Slot(int, result="QVariantMap")
    def deleteClub(self, cid):
        self._club_init()
        if len(self._clubs) <= 1: return {"ok": False, "id": cid, "error": "Impossible de supprimer le dernier club."}
        self._clubs = [c for c in self._clubs if c["id"] != cid]
        return {"ok": True, "id": cid, "error": ""}
    def _match_map(self, m):
        mid = m["id"]
        return dict(m, convoked=len(self._match_players.get(mid, [])), hasBoard=mid in self._match_board)
    @Slot(int, str, str, result="QVariantList")
    def matches(self, team, a, b):
        self._club_init()
        return [self._match_map(m) for m in sorted(self._matches.values(), key=lambda m: (m["date"], m["time"]))
                if m["teamId"] == team and a <= m["date"] <= b]
    @Slot(int, str, str, result="QStringList")
    def matchDates(self, team, a, b):
        self._club_init()
        return sorted(set(m["date"] for m in self._matches.values() if m["teamId"] == team and a <= m["date"] <= b))
    @Slot(int, result="QVariantMap")
    def match(self, mid):
        self._club_init()
        m = self._matches.get(mid)
        if not m: return {"ok": False, "id": mid, "error": "Match introuvable."}
        return {"ok": True, "id": mid, "error": "", "match": self._match_map(m),
                "players": [{"playerId": p, "name": "Joueur %d" % p, "jerseyNumber": p, "role": r}
                            for p, r in self._match_players.get(mid, [])],
                "boardId": self._match_board.get(mid, -1)}
    @Slot("QVariantMap", result="QVariantMap")
    def saveMatch(self, m):
        self._club_init()
        self.match_saves = getattr(self, "match_saves", 0) + 1
        if not str(m.get("opponent", "")).strip(): return {"ok": False, "id": -1, "error": "L'adversaire est obligatoire."}
        if not m.get("date") or not m.get("time"): return {"ok": False, "id": -1, "error": "La date et l'heure du match sont invalides."}
        sf, sa = int(m.get("scoreFor", -1)), int(m.get("scoreAgainst", -1))
        if (sf >= 0) != (sa >= 0): return {"ok": False, "id": -1, "error": "Le score est incomplet : renseignez les deux scores ou aucun."}
        mid = int(m.get("id", -1))
        if mid <= 0:
            mid = self._next_match; self._next_match += 1
        self._matches[mid] = {"id": mid, "teamId": int(m["teamId"]), "date": m["date"], "time": m["time"],
                              "opponent": m["opponent"].strip(), "location": m.get("location", ""), "homeAway": m.get("homeAway", "home"),
                              "status": m.get("status", "planned"), "scoreFor": sf, "scoreAgainst": sa, "notes": m.get("notes", "")}
        return {"ok": True, "id": mid, "error": ""}
    @Slot(int, result="QVariantMap")
    def deleteMatch(self, mid):
        self._club_init()
        if mid not in self._matches: return {"ok": False, "id": mid, "error": "Match introuvable."}
        del self._matches[mid]; self._match_players.pop(mid, None)
        b = self._match_board.pop(mid, None)
        if b: self._boards.pop(b, None)
        return {"ok": True, "id": mid, "error": ""}
    @Slot(int, "QVariantList", result="QVariantMap")
    def setMatchPlayers(self, mid, entries):
        self._club_init()
        if mid not in self._matches: return {"ok": False, "id": mid, "error": "Match introuvable."}
        self.last_convoked = [(int(e["playerId"]), e["role"]) for e in entries]
        self._match_players[mid] = list(self.last_convoked)
        return {"ok": True, "id": mid, "error": ""}
    @Slot(int, result="QVariantMap")
    def createMatchBoard(self, mid):
        self._club_init()
        if mid not in self._matches: return {"ok": False, "id": mid, "error": "Match introuvable."}
        created = mid not in self._match_board
        if created: self._match_board[mid] = self.add("Match contre " + self._matches[mid]["opponent"])
        return {"ok": True, "id": mid, "error": "", "boardId": self._match_board[mid], "created": created}
    @Slot(result="QStringList")
    def formationNames(self): return FORM["_names"]
    @Slot(str, bool, result=str)
    def formationJson(self, n, opp):
        f = FORM.get(n); return (f["opp"] if opp else f["plain"]) if f else ""
    @Slot(result="QVariantList")
    def palette(self): return FORM["_palette"]

class IssueReporter(QObject):
    """Simule IssueReporter (C++) : le vrai envoi réseau est testé par tst_issuereporter (faux serveur HTTP)."""
    busyChanged = Signal()
    finished = Signal(bool, str, str)
    def __init__(self, available=True):
        super().__init__(); self._available = available; self._busy = False; self.sent = []; self.outcome = ("ok", "")
    @Property(bool, constant=True)
    def available(self): return self._available
    @Property(bool, notify=busyChanged)
    def busy(self): return self._busy
    @Slot(str, str, "QStringList")
    def send(self, title, body, labels):
        self.sent.append((title, body, list(labels)))
        self._busy = True; self.busyChanged.emit()
        if self.outcome[0] == "hold":
            return                      # le test appelle release()
        self.release()
    def release(self, ok=None, msg=None):
        kind, text = self.outcome
        ok = (kind == "ok") if ok is None else ok
        self._busy = False; self.busyChanged.emit()
        self.finished.emit(ok, msg or ("Signalement envoyé, merci !" if ok else (text or "Erreur simulée.")),
                           "https://github.com/geekons77899377/footcoach/issues/42" if ok else "")

from PySide6.QtCore import QCoreApplication, QSettings
QCoreApplication.setOrganizationName("FootCoachTest"); QCoreApplication.setApplicationName("FootCoachTest")
QSettings.setPath(QSettings.NativeFormat, QSettings.UserScope, os.path.join(WORK, "settings")); QSettings.setPath(QSettings.IniFormat, QSettings.UserScope, os.path.join(WORK, "settings"))
class Harness:
    def __init__(self, width=412, height=892, backend=None, reporter=None):
        prepare()
        self.app = QGuiApplication.instance() or QGuiApplication(sys.argv)
        self.backend = backend or Backend()
        self.engine = QQmlApplicationEngine()
        self.warnings = []
        self.engine.warnings.connect(lambda ws: self.warnings.extend(w.toString() for w in ws))
        ctx = self.engine.rootContext()
        ctx.setContextProperty("backend", self.backend)
        self.reporter = reporter or IssueReporter()
        ctx.setContextProperty("issueReporter", self.reporter)
        ctx.setContextProperty("startupError", "")
        self.engine.addImportPath(RUN)
        self.engine.load(QUrl.fromLocalFile(os.path.join(RUN, "Main.qml")))
        roots = self.engine.rootObjects()
        if not roots:
            raise SystemExit("QML non chargé :\n" + "\n".join(self.warnings))
        self.win = roots[0]
        self.resize(width, height)
    def spin(self, ms=120):
        end = time.time() + ms / 1000
        while time.time() < end:
            self.app.processEvents(); QTest.qWait(10)
    def resize(self, w, h):
        self.win.setWidth(w); self.win.setHeight(h); self.spin(200)
    def shot(self, name):
        self.spin(150)
        img = self.win.grabWindow()
        path = os.path.join(OUT, name + ".png"); img.save(path); return path
    def find(self, name):
        return self.win.findChild(QObject, name)
    # entrées souris
    def click(self, x, y):
        QTest.mouseClick(self.win, Qt.LeftButton, Qt.NoModifier, QPointF(x, y).toPoint()); self.spin(350)
    def drag(self, x1, y1, x2, y2, steps=8):
        QTest.mousePress(self.win, Qt.LeftButton, Qt.NoModifier, QPointF(x1, y1).toPoint()); self.spin(20)
        for i in range(1, steps + 1):
            x = x1 + (x2 - x1) * i / steps; y = y1 + (y2 - y1) * i / steps
            QTest.mouseMove(self.win, QPointF(x, y).toPoint()); self.spin(15)
        QTest.mouseRelease(self.win, Qt.LeftButton, Qt.NoModifier, QPointF(x2, y2).toPoint()); self.spin(250)
