exec(open('t_live.py').read().split("errs=[]")[0])
from PySide6.QtCore import Qt, Q_RETURN_ARG
errs=[]
def check(c,msg):
    print(("OK  " if c else "KO  ")+msg)
    if not c: errs.append(msg)
h,b=run(mode="now")
def back():
    r=QMetaObject.invokeMethod(h.win,"handleBack",Qt.DirectConnection,Q_RETURN_ARG("QVariant")); h.spin(300); return r
def sec(): return h.win.property("section")
def go(s,i=-1): QMetaObject.invokeMethod(h.win,"go",Q_ARG("QVariant",s),Q_ARG("QVariant",i)); h.spin(400)
def find(cls_prop):
    return [o for o in h.win.findChildren(QObject) if o.property(cls_prop) is not None]
check(back() is False and sec()=="home","accueil : rien à fermer -> l'application peut se fermer")
go("calendar"); check(back() is True and sec()=="home","calendrier (liste) -> retour accueil")
go("calendar"); cal=[o for o in find("mode") if o.property("scope") is not None][0]
cal.setProperty("mode","session"); h.spin(200)
check(back() is True and cal.property("mode")=="browse" and sec()=="calendar","calendrier : formulaire ouvert -> fermé, reste dans le calendrier")
check(back() is True and sec()=="home","calendrier : deuxième retour -> accueil")
go("exercises"); ex=[o for o in find("editing") if o.property("categoryFilter") is not None][0]
ex.setProperty("editing",True); h.spin(200)
check(back() is True and ex.property("editing") is False and sec()=="exercises","exercices : fiche ouverte -> fermée")
check(back() is True and sec()=="home","exercices : retour -> accueil")
go("sessions"); se=[o for o in find("editing") if o.property("scope") is not None and o.property("listWidth")==400][0]
se.setProperty("editing",True); h.spin(200)
check(back() is True and se.property("editing") is False and sec()=="sessions","séances : plan ouvert -> fermé")
go("squad"); sq=[o for o in find("editing") if o.property("currentId") is not None and o.property("listWidth") is None or False]
go("attendance"); at=[o for o in find("openIndex")][0]
at.setProperty("openIndex",2); h.spin(200)
check(back() is True and at.property("openIndex")==-1 and sec()=="attendance","suivi : fiche dépliée -> repliée")
# tactique : schéma ouvert, plein écran
bid=b.add("Pressing",json.dumps({"version":1,"pitch":"full","items":[{"kind":"player","x":0.3,"y":0.4,"label":"1"}],"arrows":[]}))
go("tactics",bid); tp=h.find("") if False else None
tac=[o for o in find("currentId") if o.property("currentId")==bid][0]
ed=[o for o in find("fullscreen")][0]
ed.setProperty("fullscreen",True); h.spin(200)
check(tac.property("currentId")==bid,"schéma ouvert")
check(back() is True and ed.property("fullscreen") is False and tac.property("currentId")==bid,"tactique : plein écran -> quitté, schéma reste ouvert")
check(back() is True and tac.property("currentId")==-1 and sec()=="tactics","tactique : schéma fermé")
check(back() is True and sec()=="home","tactique : retour -> accueil")
go("data"); check(back() is True and sec()=="home","données -> accueil")
print("ECHECS" if errs else "TOUT OK",errs)
