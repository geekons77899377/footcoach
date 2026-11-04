"""Chaque champ de saisie de l'application, avec le clavier simulé (fenêtre réduite comme avec adjustResize)."""
exec(open('t_live.py').read().split("errs=[]")[0])
from PySide6.QtCore import Qt
import sys
W,H,KB=412,892,380
fails=[]; total=0
h,b=run(W,H,mode="now")
def objs():
    out=[]
    for o in h.win.findChildren(QObject):
        try:
            o.metaObject(); o.property("visible"); o.inherits("QQuickItem"); out.append(o)
        except Exception: pass
    return out
def sec(s): QMetaObject.invokeMethod(h.win,"go",Q_ARG("QVariant",s),Q_ARG("QVariant",-1)); h.spin(400)
def kb(on):
    h.win.setProperty("keyboardUp",on); h.resize(W,H-KB if on else H); h.spin(250)
def inputs():
    out=[]
    for o in objs():
        if o.objectName()=="warmField":
            continue   # champ invisible du préchauffage du clavier (Main.qml), pas un champ de l'utilisateur
        if (o.inherits("QQuickTextInput") or o.inherits("QQuickTextEdit")) and o.property("visible"):
            out.append(o)
    return out
def headers():
    return [(o,o.mapToScene(QPointF(0,0)).y()) for o in objs() if o.metaObject().className().startswith("PageHeader") and o.property("visible")]
def fieldname(o):
    p=o
    for _ in range(6):
        p=p.parent()
        if p is None: break
        if p.property("label"): return p.property("label")
        if p.property("placeholderText"): return p.property("placeholderText")
    return o.property("objectName") or o.metaObject().className()
def test_form(name, opener):
    global total
    kb(False); opener(); h.spin(400)
    base=headers(); fields=inputs()
    print(f"-- {name} : {len(fields)} champ(s)")
    for f in fields:
        total+=1
        kb(True)
        f.forceActiveFocus(); h.spin(450)
        hh=min(f.height(),50)
        p=f.mapToScene(QPointF(0,0)); top=p.y(); bottom=top+hh
        hdr=headers()
        hdr_ok=all(abs(y0-y1)<1 for (_,y0),(_,y1) in zip(base,hdr)) if base and len(base)==len(hdr) else True
        hdr_bottom=max([o.mapToScene(QPointF(0,o.height())).y() for o,_ in hdr] or [0])
        vis = top>=hdr_bottom-2 and bottom<=H-KB+1
        lab=fieldname(f)
        status="OK " if (vis and hdr_ok) else "KO "
        if not (vis and hdr_ok): fails.append(f"{name}/{lab}")
        print(f"   {status}{lab!s:28s} haut={top:.0f} bas={bottom:.0f} (visible entre {hdr_bottom:.0f} et {H-KB}) en-tête fixe={hdr_ok}")
    kb(False)
def find(prop,val=None,cls=None):
    for o in objs():
        if o.property(prop) is not None and (val is None or o.property(prop)==val): return o
def target(cls_prefix, prop):
    for o in h.win.findChildren(QObject):
        try:
            if o.metaObject().className().startswith(cls_prefix) and o.property(prop) is not None: return o
        except Exception: pass
def opener(sectionid, cls, prop, method):
    def f():
        sec(sectionid); t=target(cls,prop); QMetaObject.invokeMethod(t,method)
    return f
test_form("Séance", opener("calendar","CalendarPage","month","newSession"))
test_form("Récurrence", opener("calendar","CalendarPage","month","newRecurrence"))
test_form("Exercice", opener("exercises","ExercisesPage","categoryFilter","newExercise"))
test_form("Joueur", opener("squad","SquadPage","currentId","newPlayer"))
test_form("Ticket d'assistance", lambda: (sec("settings"), QMetaObject.invokeMethod(target("SettingsPage","mode"),"openSupport",Q_ARG("QVariant","bug"))))

# --- fenêtres (popups) et champs de recherche ---------------------------------------------------------------
def popup_inputs(p):
    out=[]
    for o in p.findChildren(QObject):
        try:
            if (o.inherits("QQuickTextInput") or o.inherits("QQuickTextEdit")) and o.property("visible"): out.append(o)
        except Exception: pass
    return out
def test_popup(name, cls, call, which=0):
    global total
    kb(False)
    ps=[o for o in objs() if o.metaObject().className().startswith(cls)]
    if not ps: fails.append(name+" (introuvable)"); print("   KO ",name,"introuvable"); return
    p=ps[which]; call(p); h.spin(500); fs=popup_inputs(p)
    print(f"-- {name} : {len(fs)} champ(s)")
    for f in fs:
        total+=1; kb(True); f.forceActiveFocus(); h.spin(450)
        pos=f.mapToScene(QPointF(0,0)); top=pos.y(); bottom=top+min(f.height(),50)
        ok = top>=0 and bottom<=H-KB+1 and f.property("activeFocus")
        lab=fieldname(f)
        print(f"   {'OK ' if ok else 'KO '}{lab!s:28s} haut={top:.0f} bas={bottom:.0f} (visible entre 0 et {H-KB})")
        if not ok: fails.append(f"{name}/{lab}")
    kb(False); QMetaObject.invokeMethod(p,"close"); h.spin(300)
sec("squad")
test_popup("Équipe (nouvelle)","TeamDialog",lambda p: QMetaObject.invokeMethod(p,"openFor",Q_ARG("QVariant",None)))
sec("sessions")
test_popup("Plan : durée et notes d'un exercice","PlanItemDialog",lambda p: QMetaObject.invokeMethod(p,"openFor",Q_ARG("QVariant",{"itemId":1,"title":"Rondo","durationMin":15,"notes":""})))
sec("exercises")
test_popup("Saisie texte (une ligne)","PromptDialog",lambda p: QMetaObject.invokeMethod(p,"ask",Q_ARG("QVariant","Renommer"),Q_ARG("QVariant","abc"),Q_ARG("QVariant",False),Q_ARG("QVariant",100)))
test_popup("Saisie texte (plusieurs lignes)","PromptDialog",lambda p: QMetaObject.invokeMethod(p,"ask",Q_ARG("QVariant","Notes"),Q_ARG("QVariant","abc"),Q_ARG("QVariant",True),Q_ARG("QVariant",1000)))
sec("sessions")
test_popup("Sélecteur d'exercices (recherche)","ExercisePicker",lambda p: QMetaObject.invokeMethod(p,"pick"))
# champs de recherche des listes
for nm,s_ in [("Recherche exercices","exercises"),("Recherche effectif","squad")]:
    kb(False); sec(s_); h.spin(300)
    pg=target("ExercisesPage" if s_=="exercises" else "SquadPage","currentId")
    pg.setProperty("editing",False); h.spin(300)
    fs=[o for o in inputs()]
    print(f"-- {nm} : {len(fs)} champ(s)")
    for f in fs:
        total+=1; kb(True); f.forceActiveFocus(); h.spin(400)
        pos=f.mapToScene(QPointF(0,0)); ok= pos.y()>=0 and pos.y()+min(f.height(),50)<=H-KB+1
        print(f"   {'OK ' if ok else 'KO '}{fieldname(f)!s:28s} haut={pos.y():.0f}")
        if not ok: fails.append(f"{nm}/{fieldname(f)}")
    kb(False)

# --- complétude : tous les champs déclarés dans qml/ ont été testés -------------------------------------------
import glob, re, os
declared = 0
for p in glob.glob(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "qml", "*.qml")):
    if p.endswith("FormField.qml"): continue
    src = open(p, encoding="utf-8").read()
    declared += len(re.findall(r"^\s*FormField\s*\{", src, re.M)) + len(re.findall(r"^\s*FormField\s*\{|\bFormField\s*\{", src)) * 0
    declared += len(re.findall(r"\b(?:TextField|TextArea)\s*\{", src))
# PromptDialog est testé deux fois (une ligne, plusieurs lignes) : un seul champ déclaré
tested_unique = total - 1
print(f"champs déclarés dans qml/ : {declared} ; champs testés (sans doublon) : {tested_unique}")
if declared != tested_unique:
    fails.append(f"complétude : {declared} déclarés, {tested_unique} testés")
print(f"\n{total} champs testés, {len(fails)} échec(s):",fails)
sys.exit(1 if fails else 0)
