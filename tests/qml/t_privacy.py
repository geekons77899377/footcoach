"""Politique de confidentialité lue dans l'application (aucun navigateur, aucune connexion réseau).

Backend SIMULÉ. Vérifie l'interface QML ; l'ouverture réelle sur Android/Windows reste à essayer sur appareil.
"""
import sys, os, re, subprocess, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_support.py")).read().split("h = make(")[0])
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

for (w, hh) in ((412, 892), (1200, 800)):
    tag = f"[{w}px]"
    h = make(w, hh)
    sp = spage(h)
    check(sp.property("mode") == "browse", f"{tag} Paramètres : liste des réglages")
    row = F(h, "rowPrivacy")
    click_item(h, row); h.spin(500)
    check(sp.property("mode") == "privacy", f"{tag} toucher « Politique de confidentialité » ouvre la page dans l'application")
    page = F(h, "privacyPage"); txt = F(h, "privacyText")
    check(page is not None and page.isVisible() and txt is not None and txt.isVisible(), f"{tag} la page est affichée")
    body = txt.property("text")
    check("Politique de confidentialité" in body and "uniquement **sur votre appareil**" in body.replace("Uniquement", "uniquement"),
          f"{tag} le texte de la politique est présent")
    check("Brouillon" not in body and "Play Console" not in body, f"{tag} la note de brouillon du dépôt n'est pas affichée")
    check(txt.height() > 300, f"{tag} texte complet ({txt.height():.0f} px de haut)")
    # Aucun lien web n'est proposé comme action : la page n'a pas de MouseArea de lien.
    check("github.com" not in str(sp.property("mode")), f"{tag} aucune URL de navigateur utilisée")
    from PySide6.QtCore import QMetaObject, Qt, Q_RETURN_ARG
    r = QMetaObject.invokeMethod(sp, "goBack", Qt.DirectConnection, Q_RETURN_ARG("QVariant")); h.spin(300)
    check(r is True and sp.property("mode") == "browse", f"{tag} retour : liste des réglages")

# Le texte embarqué suit docs/confidentialite.md (source unique)
r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "gen_privacy_text.py"), "--check"], capture_output=True, text=True)
check(r.returncode == 0, f"qml/PrivacyText.js à jour avec docs/confidentialite.md ({r.stdout.strip()})")
src = open(os.path.join(ROOT, "qml", "SupportLink.js"), encoding="utf-8").read()
check("PRIVACY_URL" not in src, "plus d'URL de politique vers GitHub dans l'application")
check("openUrlExternally(Link.PRIVACY" not in open(os.path.join(ROOT, "qml", "SettingsPage.qml"), encoding="utf-8").read(), "la page Paramètres n'ouvre plus le navigateur pour la politique")

print("\n%s" % ("ECHECS : %d" % len(errs) if errs else "TOUT PASSE"))
for e in errs: print("   KO ", e)
sys.exit(1 if errs else 0)
