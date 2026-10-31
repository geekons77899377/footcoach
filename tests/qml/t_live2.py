exec(open('t_live.py').read().split("errs=[]")[0])
errs=[]
def check(c,msg):
    print(("OK  " if c else "KO  ")+msg)
    if not c: errs.append(msg)
for mode in ["none","next"]:
    h,b=run(mode=mode)
    check(P(h,"phase")=="idle",f"[{mode}] idle")
    if mode=="none":
        check(card(h).property("session") is None,"[none] pas de séance")
    else:
        check(card(h).property("session") is not None and card(h).property("auto").toVariant()["mode"]=="next","[next] prochaine séance proposée")
    h.shot(f"live_{mode}")
h,b=run(mode="now")
for name,w,hh in [("tel-petit",360,640),("tel-paysage",844,390),("tab-portrait",800,1280),("tab-paysage",1280,800),("pc",1280,800)]:
    h.resize(w,hh); h.spin(300)
    tap(h,"liveMain"); h.spin(300)
    h.shot(f"live_fmt_{name}")
    x,y=ctr(h,"liveMain"); check(0<x<w and 0<y<hh,f"{name}: bouton principal visible ({x:.0f},{y:.0f})")
    tap(h,"liveMain"); tap(h,"liveMain"); h.spin(200)
    h.win.findChild(QObject,"live").setProperty("phase","idle")
# plein écran
h2,b2=run(mode="now"); tap(h2,"liveMain"); h2.spin(300)
QMetaObject.invokeMethod(h2.win.findChild(QObject,"live"),"openPicker")
h2.spin(400); h2.shot("live_picker")
print("ECHECS" if errs else "TOUT OK",errs)
