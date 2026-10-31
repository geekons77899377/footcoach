exec(open('t_live.py').read().split("errs=[]")[0])
errs=[]
def check(c,msg):
    print(("OK  " if c else "KO  ")+msg)
    if not c: errs.append(msg)
import os,shutil
from PySide6.QtCore import QSettings
QSettings().clear()
# 1. appui sur une ligne -> fiche exercice
h,b=run(mode="now")
tap(h,"liveMain"); h.spin(300)
rows=[o for o in h.win.findChildren(QObject) if o.metaObject().className().startswith("QQuickMouseArea") ]
card_=card(h)
card_.openExercise.emit(11); h.spin(500)
check(h.win.property("section")=="exercises","ouvre la page Exercices")
ex=[o for o in h.win.findChildren(QObject) if o.property("editing") is not None and o.property("currentId") is not None]
check(any(o.property("editing") is True and o.property("currentId")==11 for o in ex),"fiche de l'exercice 11 ouverte")
# 2. reprise après relance (même fichier de réglages)
QMetaObject.invokeMethod(h.win,"go",Q_ARG("QVariant","home"),Q_ARG("QVariant",-1)); h.spin(400)
tap(h,"liveNext"); h.spin(300)
check(P(h,"exIndex")==1 and P(h,"phase")=="running","avant relance : exercice 2 en cours")
card(h).setProperty("startedAt", card(h).property("startedAt")-300000); h.spin(200)
QMetaObject.invokeMethod(card(h),"save"); h.spin(900)
del h
h2,b2=run(mode="now")
check(P(h2,"phase")=="running" and P(h2,"exIndex")==1,"relance : reprend à l'exercice 2")
check(b2.keep[-1:]==[True],"relance : écran de nouveau maintenu")
check(P(h2,"elapsedMs")>=299000,f"relance : temps écoulé conservé ({P(h2,'elapsedMs')/1000:.0f} s)")
# 3. équipe différente / autre jour -> oublié
QMetaObject.invokeMethod(card(h2),"forget"); h2.spin(900)
h3,b3=run(mode="now")
check(P(h3,"phase")=="idle","après Fermer : relance à zéro")
print("ECHECS" if errs else "TOUT OK",errs)
