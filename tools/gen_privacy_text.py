#!/usr/bin/env python3
"""Génère qml/PrivacyText.js à partir de docs/confidentialite.md (source unique de la politique de confidentialité).

La politique s'affiche dans l'application, sans navigateur. Le texte du dépôt reste la référence (c'est aussi lui que
Play Console demande sous forme d'URL publique) : relancer ce script après toute modification, `--check` vérifie que le
fichier généré est à jour (utilisé par tests/qml/t_privacy.py).
"""
import json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "docs", "confidentialite.md")
DST = os.path.join(ROOT, "qml", "PrivacyText.js")

def build():
    text = open(SRC, encoding="utf-8").read()
    # La note de brouillon (publication sur une page web) concerne le dépôt, pas l'écran de l'application.
    text = re.sub(r"\*Brouillon[^\n]*\n[^\n]*?(Dernière mise à jour[^\n]*)\n", r"*\1\n", text, count=1)
    return ("// GÉNÉRÉ par tools/gen_privacy_text.py depuis docs/confidentialite.md — ne pas modifier à la main.\n"
            ".pragma library\n\nvar TEXT = " + json.dumps(text.strip() + "\n", ensure_ascii=False) + ";\n")

if __name__ == "__main__":
    out = build()
    if "--check" in sys.argv:
        ok = os.path.exists(DST) and open(DST, encoding="utf-8").read() == out
        print("à jour" if ok else "PÉRIMÉ : relancer tools/gen_privacy_text.py")
        sys.exit(0 if ok else 1)
    open(DST, "w", encoding="utf-8").write(out)
    print("écrit", DST)
