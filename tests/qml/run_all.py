#!/usr/bin/env python3
"""Lance les tests QML hors écran (PySide6) : choix et chronomètre de la séance, bouton retour, champs de saisie avec
clavier simulé. Un test réussit si son code de sortie est 0 et qu'il n'affiche ni « KO » ni « ECHECS ».
Usage : python3 tests/qml/run_all.py   (PySide6 et un affichage « offscreen » suffisent, pas d'appareil)"""
import os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
TESTS = ["t_live.py", "t_live2.py", "t_live3.py", "t_back.py", "t_fields.py", "t_blockboard.py", "t_home.py", "t_exgrid.py", "t_squad.py", "t_calendar.py", "t_support.py", "t_nav.py", "t_l0a.py", "t_l0c.py", "t_ux1.py", "t_ux2.py"]
env = dict(os.environ, QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software")
bad = []
for t in TESTS:
    r = subprocess.run([sys.executable, t], cwd=HERE, env=env, capture_output=True, text=True)
    lines = r.stdout.splitlines()
    ok = [l for l in lines if l.strip().startswith("OK ")]
    ko = [l for l in lines if l.startswith("KO ") or "ECHECS" in l or l.startswith("   KO ")]
    status = "OK " if r.returncode == 0 and not ko else "KO "
    print(f"{status}{t:14s} {len(ok)} vérifications OK, {len(ko)} échec(s)")
    for l in ko:
        print("     ", l.strip())
    if status == "KO ":
        bad.append(t)
        if r.returncode != 0:
            print((r.stderr or "")[-600:])
print("\nTOUT PASSE" if not bad else f"\nÉCHECS : {bad}")
sys.exit(1 if bad else 0)
