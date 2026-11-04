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
    @Slot(result="QStringList")
    def formationNames(self): return FORM["_names"]
    @Slot(str, bool, result=str)
    def formationJson(self, n, opp):
        f = FORM.get(n); return (f["opp"] if opp else f["plain"]) if f else ""
    @Slot(result="QVariantList")
    def palette(self): return FORM["_palette"]

from PySide6.QtCore import QCoreApplication, QSettings
QCoreApplication.setOrganizationName("FootCoachTest"); QCoreApplication.setApplicationName("FootCoachTest")
QSettings.setPath(QSettings.NativeFormat, QSettings.UserScope, os.path.join(WORK, "settings")); QSettings.setPath(QSettings.IniFormat, QSettings.UserScope, os.path.join(WORK, "settings"))
class Harness:
    def __init__(self, width=412, height=892, backend=None):
        prepare()
        self.app = QGuiApplication.instance() or QGuiApplication(sys.argv)
        self.backend = backend or Backend()
        self.engine = QQmlApplicationEngine()
        self.warnings = []
        self.engine.warnings.connect(lambda ws: self.warnings.extend(w.toString() for w in ws))
        ctx = self.engine.rootContext()
        ctx.setContextProperty("backend", self.backend)
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
