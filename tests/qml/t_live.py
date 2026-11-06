from harness import *
from PySide6.QtCore import QMetaObject, Q_ARG, QPointF
import datetime, json, sys
now = datetime.datetime.now()
def hhmm(d): return d.strftime("%H:%M")
class B2(Backend):
    mode = "now"
    @Slot(result="QVariantList")
    def teams(self): return [{"id":1,"name":"Senior A","category":"Seniors"}]
    @Slot(result="QVariantList")
    def exerciseCategories(self): return []
    @Slot(bool, result="QVariantList")
    def exercises(self, a): return [{"id":i,"title":t,"category":"technical","categoryLabel":c,"objective":"","durationMin":d,"playersMin":0,"playersMax":0,"equipment":"","description":"","archived":False,"usage":1} for i,(t,c,d) in enumerate(EX, 10)]
    @Slot(int,str,str,result="QVariantList")
    def sessions(self, team, a, b):
        if self.mode == "none": return []
        # La date vient de l'horaire calculé (sinon, après minuit, la séance « prochaine » tombe dans le passé).
        st = now + datetime.timedelta(hours=2) if self.mode == "next" else now - datetime.timedelta(minutes=10)
        day = st.strftime("%Y-%m-%d")
        return [{"id":7,"teamId":1,"date":day,"time":hhmm(st),"endTime":hhmm(st+datetime.timedelta(minutes=90)),"durationMin":90,"location":"","theme":"Transitions défensives","notes":"","status":"planned","present":-1,"total":-1}]
    @Slot(int,result="QVariantMap")
    def plan(self, sid):
        return {"ok":True,"entries":[{"itemId":i,"exerciseId":10+i,"title":t,"categoryLabel":c,"durationMin":d,"start":"","end":""} for i,(t,c,d) in enumerate(EX)]}
    @Slot(bool)
    def setKeepScreenOn(self, on): self.keep.append(on)
EX=[("Échauffement dynamique","Activation & mobilité",15),("Rondo 4v2 Pressing","Technique & réaction",20),("Circuit de passes rapide","Coordination",20),("Match à thème","Jeu",25)]
B2.keep=[]
def run(w=412,hh=892,mode="now",tag=""):
    B2.mode=mode
    b=B2(); b.keep=[]
    h=Harness(w,hh,backend=b)
    QMetaObject.invokeMethod(h.win,"go",Q_ARG("QVariant","home"),Q_ARG("QVariant",-1)); h.spin(500)
    return h,b
def ctr(h,name):
    it=h.find(name); p=it.mapToScene(QPointF(it.width()/2,it.height()/2)); return p.x(),p.y()
def tap(h,name): x,y=ctr(h,name); h.click(x,y)
def card(h): return h.find("live")
def P(h,n): return card(h).property(n)
errs=[]
def check(c,msg):
    print(("OK  " if c else "KO  ")+msg)
    if not c: errs.append(msg)

h,b=run()
check(P(h,"phase")=="idle","idle au départ")
check(card(h).property("session") is not None,"séance auto trouvée (en cours)")
check(P(h,"entries").__len__()==4,"4 exercices chargés")
h.shot("live_idle")
tap(h,"liveMain"); h.spin(600)
check(P(h,"phase")=="running","COMMENCER démarre")
check(b.keep[-1:]==[True],"écran maintenu allumé")
h.spin(2200); left=P(h,"leftMs")
check(15*60000-left>=2000 and 15*60000-left<4000,f"décompte avance ({(15*60000-left)/1000:.1f} s écoulées)")
h.shot("live_running")
tap(h,"liveMain"); h.spin(300)
check(P(h,"phase")=="paused","pause")
a=P(h,"elapsedMs"); h.spin(1200); check(abs(P(h,"elapsedMs")-a)<50,"figé en pause")
tap(h,"liveMain"); h.spin(300)
check(P(h,"phase")=="running","reprise")
tap(h,"liveNext"); h.spin(400)
check(P(h,"exIndex")==1,"exercice suivant")
check(abs(P(h,"accMs"))<1,"chrono repart à zéro")
# dépassement : exercice 2 prévu 20 min -> simuler 21 min écoulées
card(h).setProperty("startedAt", card(h).property("now")-21*60000); h.spin(500)
check(P(h,"overtime") is True,"dépassement détecté (orange)")
h.shot("live_overtime")
tap(h,"liveNext"); tap(h,"liveNext"); h.spin(300)
check(P(h,"exIndex")==3,"arrivé au dernier exercice")
h.shot("live_last")
tap(h,"liveNext"); h.spin(400)
check(P(h,"phase")=="finished","séance terminée")
check(b.keep[-1]==False,"écran libéré à la fin")
check(len(P(h,"actuals").toVariant())==4,"4 durées réelles")
h.shot("live_done")
tap(h,"liveClose"); h.spin(300)
check(P(h,"phase")=="idle","Fermer remet à zéro")
print("avert:",[x for x in h.warnings if "Connections" not in x and "palette" not in x and "formationNames" not in x][:6])
print("ECHECS" if errs else "TOUT OK",errs)
