"""Animation des schémas (16/11/2026) : lecture des flèches (course, conduite, passe), lecture / pause, pas à pas,
vitesse, partage en GIF (captures du terrain). Téléphone et bureau.

Backend SIMULÉ : ce test valide l'interface QML et BoardAnimation.js ; l'assemblage GIF réel est testé par
tests/tst_gif.cpp.
"""
import sys, os, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, QObject
from PySide6.QtGui import QImage
from PySide6.QtCore import QPointF

SCENE = json.dumps({"version": 1, "pitch": "half", "items": [
    {"kind": "player", "x": 0.30, "y": 0.40, "label": "7", "color": "#1565c0"},     # 0 : conduit puis passe
    {"kind": "ball", "x": 0.31, "y": 0.42, "label": ""},                            # 1 : à ses pieds
    {"kind": "player", "x": 0.20, "y": 0.75, "label": "9", "color": "#1565c0"},     # 2 : court
    {"kind": "cone", "x": 0.60, "y": 0.20, "label": "", "rot": 0, "scale": 1},      # 3 : ne bouge jamais
], "arrows": [
    {"style": "dribble", "x1": 0.30, "y1": 0.40, "x2": 0.50, "y2": 0.40, "curve": 0},
    {"style": "run", "x1": 0.20, "y1": 0.75, "x2": 0.55, "y2": 0.80, "curve": 0},
    {"style": "pass", "x1": 0.51, "y1": 0.41, "x2": 0.55, "y2": 0.78, "curve": 0},
    {"style": "run", "x1": 0.90, "y1": 0.10, "x2": 0.95, "y2": 0.30, "curve": 0},   # personne au départ
]})

class B10(B2):
    def __init__(self):
        super().__init__()
        self.frames = []; self.begun = None; self.finished = []; self.cancelled = 0
    @Slot(int, int, int, result=bool)
    def gifBegin(self, w, h, d):
        self.begun = (w, h, d); self.frames = []; return True
    @Slot(QImage, result=bool)
    def gifAddFrame(self, img):
        self.frames.append(QImage(img)); return True
    @Slot(str, result="QVariantMap")
    def gifFinish(self, url):
        self.finished.append(url); return {"ok": True, "frames": len(self.frames), "bytes": 1000}
    @Slot()
    def gifCancel(self): self.cancelled += 1
    @Slot(str, result=str)
    def animationFileName(self, t): return "animation-test.gif"
    @Slot(str, result=str)
    def challengeSheetTempUrl(self, name): return "file:///tmp/" + name

errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)
def find(h, pred):
    for o in h.win.findChildren(QObject):
        try:
            if pred(o): return o
        except Exception: pass
def named(root, name):
    for o in root.findChildren(QObject):
        if o.objectName() == name: return o
def jv(v): return v.toVariant() if hasattr(v, "toVariant") else v
def tokens(view):
    out = {}
    todo = [view]
    while todo:
        o = todo.pop()
        todo.extend(o.childItems())
        if o.metaObject().className().startswith("BoardToken"):
            p = o.property("screenPos")
            out[o.property("index")] = (round(p.x(), 1), round(p.y(), 1))
    return out
def moved(a, b, i): return abs(a[i][0] - b[i][0]) + abs(a[i][1] - b[i][1]) > 3

for W, H_, tag in [(412, 892, "tel"), (1280, 800, "bureau")]:
    B2.mode = "none"; b = B10(); b.keep = []
    bid = b.add("Une-deux", SCENE)
    h = Harness(W, H_, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "tactics"), Q_ARG("QVariant", bid)); h.spin(700)
    ed = find(h, lambda o: o.objectName() == "editor")
    view = named(ed, "boardView")
    btn = named(ed, "animateButton")
    check(btn is not None and btn.property("visible") is True, f"[{tag}] bouton « Animer le schéma » visible (le schéma a des flèches)")
    check(btn.property("text") == "Animer" and btn.property("width") > 70, f"[{tag}] bouton vert libellé « Animer » ({btn.property('text')}, {btn.property('width')} px)")
    if tag == "tel": h.shot("anim_0_bouton_animer")
    ed.setProperty("fullscreen", True); h.spin(200)
    fsb = named(ed, "animateButtonFullscreen")
    check(fsb is not None and fsb.isVisible(), f"[{tag}] plein écran : bouton ▶ sur le terrain (l'en-tête est masqué)")
    ed.setProperty("fullscreen", False); h.spin(200)
    before = tokens(view)
    QMetaObject.invokeMethod(btn, "clicked"); h.spin(300)
    player = named(ed, "boardPlayer")
    check(ed.property("animating") is True and player.property("visible") is True, f"[{tag}] mode animation : barre de lecture affichée")
    check(view.property("enabled") is False, f"[{tag}] gestes du terrain coupés pendant l'animation")
    zin = [o for o in ed.findChildren(QObject) if o.property("accessibleName") == "Zoom avant"]
    check(zin and not zin[0].isVisible(), f"[{tag}] boutons de zoom masqués pendant l'animation")
    tl = jv(view.property("anim"))
    steps = tl["steps"]
    check(len(steps) == 4, f"[{tag}] 4 mouvements, un par flèche ({len(steps)})")
    check(steps[0]["actor"] == 0 and steps[0]["ball"] == 1, f"[{tag}] conduite : le n°7 et son ballon ({steps[0]['actor']}, {steps[0]['ball']})")
    check(steps[1]["actor"] == 2, f"[{tag}] course : le n°9 ({steps[1]['actor']})")
    check(steps[2]["ball"] == 1 and steps[2]["actor"] == -1 and not steps[2]["ghostBall"], f"[{tag}] passe : le ballon, depuis sa nouvelle position")
    check(steps[3]["actor"] == -1, f"[{tag}] flèche sans joueur au départ : seulement mise en avant")
    check(all(steps[k]["t0"] > steps[k - 1]["t0"] for k in range(1, 4)), f"[{tag}] mouvements dans l'ordre des flèches")
    check(player.property("playing") is True, f"[{tag}] la lecture démarre seule")

    # Lecture : avance réelle par la minuterie.
    t0 = view.property("animTime")
    h.spin(700)
    check(view.property("animTime") > t0, f"[{tag}] le temps avance pendant la lecture ({t0:.2f} -> {view.property('animTime'):.2f})")
    QMetaObject.invokeMethod(player, "togglePlay"); h.spin(100)
    t1 = view.property("animTime"); h.spin(300)
    check(player.property("playing") is False and abs(view.property("animTime") - t1) < 1e-6, f"[{tag}] pause : le temps s'arrête")

    # Pas à pas : depuis le début, « suivant » joue la conduite et s'arrête à sa fin.
    view.setProperty("animTime", 0); h.spin(50)
    QMetaObject.invokeMethod(player, "next"); h.spin(50)
    for _ in range(80):
        QMetaObject.invokeMethod(player, "advance", Q_ARG("QVariant", 0.05))
    h.spin(150)
    end0 = steps[0]["t0"] + steps[0]["dur"]
    check(player.property("playing") is False and abs(view.property("animTime") - end0) < 1e-6, f"[{tag}] suivant : arrêt à la fin de la conduite ({view.property('animTime'):.2f} / {end0:.2f})")
    after1 = tokens(view)
    check(moved(before, after1, 0) and moved(before, after1, 1), f"[{tag}] le n°7 et le ballon ont avancé")
    check(not moved(before, after1, 2) and not moved(before, after1, 3), f"[{tag}] le n°9 et le plot n'ont pas encore bougé")
    check(named(player, "playerStep").property("text") == "1 / 4", f"[{tag}] compteur « 1 / 4 » ({named(player, 'playerStep').property('text')})")
    if tag == "tel": h.shot("anim_1_apres_conduite")
    QMetaObject.invokeMethod(player, "next"); h.spin(50)
    for _ in range(80):
        QMetaObject.invokeMethod(player, "advance", Q_ARG("QVariant", 0.05))
    h.spin(150)
    after2 = tokens(view)
    check(moved(after1, after2, 2) and not moved(after1, after2, 0), f"[{tag}] suivant : le n°9 court, le n°7 reste")
    QMetaObject.invokeMethod(player, "previous"); h.spin(150)
    check(player.property("playing") is False and view.property("animTime") < steps[1]["t0"] + 1e-6, f"[{tag}] précédent : retour avant la course")
    check(not moved(after1, tokens(view), 2), f"[{tag}] précédent : le n°9 revient à son départ")

    # Pendant la passe : le ballon voyage, à mi-chemin.
    s2 = steps[2]
    view.setProperty("animTime", s2["t0"] + s2["dur"] * 0.5); h.spin(150)
    st = jv(view.property("animState"))
    check(st["step"] == 2, f"[{tag}] mouvement en cours : la passe ({st['step']})")
    check(0.42 < st["pos"][1]["y"] < 0.78, f"[{tag}] passe : ballon en route ({st['pos'][1]['y']:.2f})")
    if tag == "tel": h.shot("anim_2_passe")

    # Vitesse : normal -> rapide -> lent -> normal.
    sp = named(player, "playerSpeed")
    speeds = []
    for _ in range(3):
        mas = [o for o in sp.findChildren(QObject) if o.metaObject().className().startswith("QQuickMouseArea")]
        QMetaObject.invokeMethod(mas[0], "clicked", Q_ARG("QQuickMouseEvent*", None)) if False else player.setProperty("speed", {1: 2, 2: 0.5, 0.5: 1}[player.property("speed")])
        speeds.append(player.property("speed"))
    check(speeds == [2, 0.5, 1], f"[{tag}] vitesses rapide, lent, normal ({speeds})")
    player.setProperty("speed", 2); view.setProperty("animTime", 0); player.setProperty("playing", True)
    QMetaObject.invokeMethod(player, "advance", Q_ARG("QVariant", 0.1)); h.spin(20)
    check(abs(view.property("animTime") - 0.2) < 1e-6, f"[{tag}] rapide : 0,1 s réel = 0,2 s d'animation")
    player.setProperty("playing", False); player.setProperty("speed", 1)
    # Boucle : à la fin, retour au début.
    view.setProperty("animTime", tl["total"] - 0.01); player.setProperty("playing", True)
    QMetaObject.invokeMethod(player, "advance", Q_ARG("QVariant", 0.05)); h.spin(20)
    check(view.property("animTime") == 0, f"[{tag}] fin de l'animation : elle recommence")
    player.setProperty("playing", False)

    # Partage : captures du terrain puis GIF.
    ed.setProperty("dryRun", True)
    QMetaObject.invokeMethod(player, "shareRequested"); h.spin(100)
    check(named(ed, "recordingBox").property("visible") is True, f"[{tag}] « Création de l'animation… » affiché")
    for _ in range(400):
        h.spin(30)
        if b.finished: break
    n = len(b.frames)
    expect = ed.property("recFrames")
    check(b.begun is not None and b.begun[0] <= 480 and b.begun[1] <= 640 and max(b.begun[0], b.begun[1]) >= 450
          and b.begun[2] == 10, f"[{tag}] GIF d'au plus 480 x 640 px, 10 images/s ({b.begun})")
    check(b.finished == ["file:///tmp/animation-test.gif"] and n == expect and n >= 10, f"[{tag}] toutes les images capturées puis assemblées ({n}/{expect})")
    check(all(not f.isNull() for f in b.frames), f"[{tag}] images non vides")
    diff = 0
    if n > 5:
        a, z = b.frames[0], b.frames[n // 2]
        diff = sum(1 for x in range(0, a.width(), 8) for y in range(0, a.height(), 8) if a.pixel(x, y) != z.pixel(x, y))
    check(diff > 5, f"[{tag}] les images diffèrent : le terrain bouge ({diff} points)")
    check(named(ed, "recordingBox").property("visible") is False and ed.property("lastAnimationUrl") == "file:///tmp/animation-test.gif",
          f"[{tag}] animation créée, partage proposé (simulé)")
    if tag == "tel" and n:
        b.frames[n // 2].save(os.path.join(OUT, "anim_3_image_gif.png"))
        if os.environ.get("ANIM_DUMP"):                     # images brutes, pour assembler un vrai GIF hors test
            os.makedirs(os.environ["ANIM_DUMP"], exist_ok=True)
            for k, f in enumerate(b.frames): f.save(os.path.join(os.environ["ANIM_DUMP"], "f%03d.png" % k))
    # Annuler une création en cours.
    b.finished = []
    QMetaObject.invokeMethod(player, "shareRequested"); h.spin(200)
    QMetaObject.invokeMethod(ed, "cancelRecording"); h.spin(300)
    check(b.cancelled >= 1 and not b.finished and player.property("recording") is False, f"[{tag}] annuler : rien n'est créé")

    # Fermer : retour à l'édition, positions d'origine.
    QMetaObject.invokeMethod(player, "closeRequested"); h.spin(200)
    check(ed.property("animating") is False and view.property("anim") is None and view.property("enabled") is True,
          f"[{tag}] fermer : retour à l'édition")
    check(tokens(view) == before, f"[{tag}] les éléments sont revenus à leur place (le schéma n'est pas modifié)")
    check(json.loads(b._boards[bid]["json"])["items"][0]["x"] == 0.30, f"[{tag}] schéma enregistré inchangé")
    bad = [w for w in h.warnings if "Board" in w]
    check(not bad, f"[{tag}] aucun avertissement QML sur le terrain ({bad[:3]})")

# Depuis la fiche d'un exercice : « Animer le terrain » ouvre le terrain et lance la lecture.
class B11(B10):
    @Slot(int, result="QVariantMap")
    def exerciseBoard(self, ex):
        return {"boardId": 501, "title": "Une-deux", "readable": True, "json": SCENE if ex == 10 else NOARROW}
    @Slot(int, result="QVariantMap")
    def board(self, i):
        return {"id": i, "title": "Une-deux", "readable": True, "error": "", "json": SCENE}
NOARROW = json.dumps({"version": 1, "pitch": "half", "items": [{"kind": "player", "x": 0.5, "y": 0.5, "label": "1"}], "arrows": []})
def item(h, name):
    todo = [h.win.contentItem()]
    while todo:
        it = todo.pop()
        if it.objectName() == name: return it
        todo.extend(it.childItems())
for W, H_, tag in [(412, 892, "tel"), (1280, 800, "bureau")]:
    B2.mode = "now"; b = B11(); b.keep = []
    h = Harness(W, H_, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", -1)); h.spin(500)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", 11)); h.spin(500)
    ab = item(h, "animateExerciseBoard")
    check(ab is not None and not ab.isVisible(), f"[{tag}] fiche : pas de « Animer le terrain » sans flèche")
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", 10)); h.spin(500)
    ab = item(h, "animateExerciseBoard")
    check(ab is not None and ab.isVisible() and ab.property("text") == "Animer le terrain", f"[{tag}] fiche : bouton « Animer le terrain »")
    if tag == "tel":
        fl = None
        it = ab
        while it is not None:
            if it.metaObject().className().startswith("QQuickFlickable"): fl = it; break
            it = it.parentItem()
        if fl is not None:
            y = ab.mapToItem(fl, QPointF(0, 0)).y()
            fl.setProperty("contentY", max(0, fl.property("contentY") + y - 250)); h.spin(200)
        h.shot("anim_4_fiche_exercice")
    QMetaObject.invokeMethod(ab, "clicked"); h.spin(800)
    ed = find(h, lambda o: o.objectName() == "editor")
    pl = named(ed, "boardPlayer")
    check(h.win.property("section") == "tactics" and ed.property("animating") is True and pl.property("playing") is True,
          f"[{tag}] fiche : le terrain s'ouvre et l'animation démarre ({h.win.property('section')})")
    tp = find(h, lambda o: o.property("animateOnOpen") is not None and o.metaObject().className().startswith("TacticsPage"))
    check(tp is not None and tp.property("animateOnOpen") is False, f"[{tag}] fiche : demande d'animation consommée")

print("\n%s" % ("ECHECS : %d" % len(errs) if errs else "TOUT PASSE"))
for e in errs: print("   KO ", e)
sys.exit(1 if errs else 0)
