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

# Extrait de Data ES (6 terrains réels, mêmes clés que la réponse lue par VenueRepository::applyDataEs).
DATA_ES = json.load(open(os.path.join(HERE, "..", "fixtures", "data-es-oise-extrait.json"), encoding="utf-8"))

def _venue_label(v):
    head = v["name"]
    f = v.get("field") or ""
    if f and f.lower() != "terrain de football" and f.lower() != head.lower():
        head += " (%s)" % f
    town = ("%s %s" % (v.get("postcode") or "", v.get("city") or "")).strip()
    return ", ".join([x for x in (head, v.get("address") or "", town) if x])

def _tidy(s):
    s = " ".join((s or "").split())
    if not s or s != s.upper(): return s
    small = {"de", "du", "des", "la", "le", "les", "et", "en", "sur", "aux", "au"}
    out = []
    for i, w in enumerate(s.lower().split(" ")):
        if i > 0 and w in small: out.append(w); continue
        w = list(w)
        for k in range(len(w)):
            if k == 0 or w[k - 1] in "-'": w[k] = w[k].upper()
        out.append("".join(w))
    return " ".join(out)

class Backend(QObject):
    lastErrorChanged = Signal()
    dataReplaced = Signal()
    venuesBusyChanged = Signal()
    venuesRefreshed = Signal("QVariantMap")
    def __init__(self):
        super().__init__()
        self._boards = {}
        self._next = 1
        self._err = ""
        self.saves = 0
    @Property(str, notify=lastErrorChanged)
    def lastError(self): return self._err
    # --- Lieux de match (schéma 11, simulés : mêmes clés que src/app/BackendVenues.cpp) ---------------------------
    def _venue_init(self):
        if not hasattr(self, "_venues"):
            self._venues = {}            # id -> lieu
            self._next_venue = 1
            self._dept = {}              # clubId -> département
            self._opp_venue = {}         # (clubId, clé) -> venueId
            self._venue_busy = False
            self.venue_refresh_answer = DATA_ES     # réponse simulée de Data ES (liste), ou "error"
            self.venue_refresh_delay = 300
    def _venue_out(self, v):
        d = dict(v)
        d["label"] = _venue_label(v)
        d["selectable"] = not v["unavailable"] and v["esStatus"] == "ok"
        return d
    @Property(bool, notify=venuesBusyChanged)
    def venuesBusy(self):
        self._venue_init(); return self._venue_busy
    @Slot(int, result=int)
    def clubOfTeam(self, team):
        for t in self.teams() if hasattr(self, "teams") else []:
            if t.get("id") == team: return int(t.get("clubId", 1))
        return 1 if team > 0 else -1
    @Slot(int, result="QVariantMap")
    def venueSummary(self, club):
        self._venue_init(); self._club_init()
        c = next((x for x in self._clubs if x["id"] == club), None)
        if not c: return {"ok": False, "error": "Club introuvable.", "id": -1}
        vs = [self._venue_out(v) for v in self._venues.values() if v["clubId"] == club]
        home = next((v for v in vs if v["isHome"]), None)
        return {"ok": True, "error": "", "id": club, "clubId": club, "clubName": c["name"], "department": self._dept.get(club, ""),
                "lastRefresh": getattr(self, "_last_refresh", {}).get(club, ""), "total": len(vs),
                "selectable": sum(1 for v in vs if v["selectable"]), "unavailable": sum(1 for v in vs if v["unavailable"]),
                "removed": sum(1 for v in vs if v["esStatus"] == "removed"), "closed": sum(1 for v in vs if v["esStatus"] == "closed"),
                "manual": sum(1 for v in vs if v["source"] == "manual"),
                "homeVenueId": home["id"] if home else -1, "homeLabel": home["label"] if home else ""}
    @Slot(int, str, result="QVariantMap")
    def setClubDepartment(self, club, d):
        self._venue_init()
        import re
        d = d.strip().upper()
        if d and not re.match(r"^(0[1-9]|1[0-9]|[3-8][0-9]|9[0-5]|2[1-9]|2A|2B|97[1-6])$", d):
            return {"ok": False, "error": "Numéro de département inconnu : tapez par exemple 60, 2A ou 974.", "id": -1}
        self._dept[club] = d
        return {"ok": True, "error": "", "id": club}
    @Slot(int, bool, result="QVariantList")
    def venues(self, club, include_all):
        self._venue_init()
        vs = [self._venue_out(v) for v in self._venues.values() if v["clubId"] == club]
        if not include_all: vs = [v for v in vs if v["selectable"]]
        return sorted(vs, key=lambda v: (not v["isHome"], v["city"].lower(), v["name"].lower(), (v["field"] or "").lower(), v["id"]))
    def _add_venue(self, club, **kw):
        self._venue_init()
        i = self._next_venue; self._next_venue += 1
        v = {"id": i, "clubId": club, "name": "", "field": "", "address": "", "postcode": "", "city": "", "source": "manual",
             "sourceId": "", "esStatus": "ok", "unavailable": False, "isHome": False, "lat": None, "lon": None}
        v.update(kw); self._venues[i] = v
        return i
    @Slot("QVariantMap", result="QVariantMap")
    def saveVenue(self, m):
        self._venue_init()
        name = str(m.get("name", "")).strip()
        if not name: return {"ok": False, "error": "Le nom du lieu est obligatoire.", "id": -1}
        vid = int(m.get("id", -1))
        vals = {k: str(m.get(k, "")).strip() for k in ("field", "address", "postcode", "city")}
        if vid > 0:
            v = self._venues.get(vid)
            if not v or v["source"] != "manual": return {"ok": False, "error": "Un terrain de la liste officielle ne se modifie pas.", "id": -1}
            v.update(vals); v["name"] = name
        else:
            vid = self._add_venue(int(m.get("clubId", -1)), name=name, **vals)
        return {"ok": True, "error": "", "id": vid, "venue": self._venue_out(self._venues[vid])}
    @Slot(int, result="QVariantMap")
    def deleteVenue(self, vid):
        self._venue_init()
        v = self._venues.get(vid)
        if not v or v["source"] != "manual": return {"ok": False, "error": "Un terrain de la liste officielle ne se supprime pas.", "id": vid}
        del self._venues[vid]
        for m in getattr(self, "_matches", {}).values():
            if m.get("venueId") == vid: m["venueId"] = -1
        return {"ok": True, "error": "", "id": vid}
    @Slot(int, bool, result="QVariantMap")
    def setVenueUnavailable(self, vid, u):
        self._venue_init()
        if vid not in self._venues: return {"ok": False, "error": "Lieu introuvable.", "id": vid}
        self._venues[vid]["unavailable"] = bool(u)
        return {"ok": True, "error": "", "id": vid}
    @Slot(int, int, result="QVariantMap")
    def setHomeVenue(self, club, vid):
        self._venue_init()
        for v in self._venues.values():
            if v["clubId"] == club: v["isHome"] = (v["id"] == vid)
        return {"ok": True, "error": "", "id": vid}
    def _apply_data_es(self, club, recs):
        known = {v["sourceId"]: v for v in self._venues.values() if v["clubId"] == club and v["source"] == "data_es"}
        st = {"total": 0, "added": 0, "updated": 0, "closed": 0, "removed": 0, "reopened": 0}
        seen = set()
        if not recs:
            return {"ok": False, "error": "Data ES n'a renvoyé aucun terrain de football pour ce département : la liste n'a pas été modifiée.", "clubId": club}
        for r in recs:
            sid = r["equip_numero"]; seen.add(sid)
            closed = (r.get("inst_hs_bool") or "").lower() == "oui"
            vals = dict(name=_tidy(r.get("inst_nom")), field=_tidy(r.get("equip_nom")), address=_tidy(r.get("inst_adresse")),
                        postcode=r.get("inst_cp") or "", city=r.get("new_name") or "", esStatus="closed" if closed else "ok")
            if sid in known:
                before = known[sid]["esStatus"]; known[sid].update(vals)
                st["closed" if closed else ("reopened" if before != "ok" else "updated")] += 1
            else:
                self._add_venue(club, source="data_es", sourceId=sid, **vals)
                st["closed" if closed else "added"] += 1
        for sid, v in known.items():
            if sid not in seen and v["esStatus"] != "removed":
                v["esStatus"] = "removed"; st["removed"] += 1
        st["total"] = len(seen)
        if not hasattr(self, "_last_refresh"): self._last_refresh = {}
        self._last_refresh[club] = "2026-11-13 21:40"
        return dict(st, ok=True, error="", clubId=club)
    @Slot(int, result=bool)
    def refreshVenues(self, club):
        self._venue_init()
        if self._venue_busy:
            self._err = "Une mise à jour des stades est déjà en cours."; self.lastErrorChanged.emit(); return False
        if not self._dept.get(club):
            self._err = "Indiquez d'abord le département du club."; self.lastErrorChanged.emit(); return False
        self.refresh_calls = getattr(self, "refresh_calls", 0) + 1
        self._venue_busy = True; self.venuesBusyChanged.emit()
        self._refresh_club = club
        self._refresh_timer = QTimer(self); self._refresh_timer.setSingleShot(True)
        self._refresh_timer.timeout.connect(self._finish_refresh)
        self._refresh_timer.start(self.venue_refresh_delay)
        return True
    def _finish_refresh(self, cancelled=False):
        club = self._refresh_club
        if cancelled:
            r = {"ok": False, "error": "Mise à jour annulée : la liste n'a pas été modifiée.", "clubId": club, "cancelled": True}
        elif self.venue_refresh_answer == "error":
            r = {"ok": False, "error": "Impossible de joindre Data ES (vérifiez la connexion Internet) : la liste n'a pas été modifiée.", "clubId": club}
        else:
            r = self._apply_data_es(club, self.venue_refresh_answer)
        self._venue_busy = False; self.venuesBusyChanged.emit()
        self.venuesRefreshed.emit(r)
    @Slot()
    def cancelVenueRefresh(self):
        self._venue_init()
        if self._venue_busy:
            self._refresh_timer.stop(); self._finish_refresh(cancelled=True)
    @Slot(int, str, result="QVariantMap")
    def openItinerary(self, vid, address):
        self._venue_init()
        v = self._venues.get(vid)
        q = (v["name"] + " - " + v["city"]) if v else address.strip()
        if not q: return {"ok": False, "error": "Indiquez d'abord le stade ou l'adresse du match.", "uri": ""}
        self.itinerary_calls = getattr(self, "itinerary_calls", []) + [(vid, address)]
        return {"ok": False, "error": "L'itinéraire s'ouvre sur le téléphone (Waze, Google Maps…).", "uri": "geo:0,0?q=" + q}
    @staticmethod
    def _opp_key(s):
        import unicodedata, re
        s = "".join(c for c in unicodedata.normalize("NFD", s) if unicodedata.category(c) != "Mn").lower()
        words = re.sub(r"[^0-9a-z]+", " ", s).split()
        out = []; last_init = False
        for w in words:
            init = len(w) == 1 and w.isalpha()
            if init and last_init: out[-1] += w
            else: out.append(w)
            last_init = init
        return " ".join(out)
    @Slot(int, str, str, result="QVariantMap")
    def suggestedVenue(self, team, home_away, opponent):
        self._venue_init()
        club = self.clubOfTeam(team)
        if home_away == "home":
            v = next((v for v in self._venues.values() if v["clubId"] == club and v["isHome"]), None)
        else:
            vid = self._opp_venue.get((club, self._opp_key(opponent)), -1)
            v = self._venues.get(vid)
        if not v: return {}
        v = self._venue_out(v)
        return v if v["selectable"] else {}
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
        return dict(m, venueId=m.get("venueId", -1), convoked=len(self._match_players.get(mid, [])), hasBoard=mid in self._match_board)
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
                              "status": m.get("status", "planned"), "scoreFor": sf, "scoreAgainst": sa, "notes": m.get("notes", ""),
                              "venueId": int(m.get("venueId", -1))}
        vid = int(m.get("venueId", -1))
        if vid > 0:
            self._venue_init()
            v = self._venues.get(vid)
            if not self._matches[mid]["location"] and v: self._matches[mid]["location"] = _venue_label(v)
            if m.get("homeAway") == "away":
                self._opp_venue[(self.clubOfTeam(int(m["teamId"])), self._opp_key(m["opponent"]))] = vid
        self.last_match_saved = dict(self._matches[mid])
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
    @Slot(int, result="QStringList")
    def formationNamesFor(self, n): return FORM["_names8"] if n == 8 else FORM["_names"] if n == 11 else []
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
        ctx.setContextProperty("buildInfo", {"version": "0.0.0", "sha1": "0123456789abcdef0123456789abcdef01234567", "shortSha": "0123456", "dirty": False})
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
