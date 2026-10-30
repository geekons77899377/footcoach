# Journal des modifications

## 0.3.0 — 2026-10-30
- Interface refaite en QML (téléphone, tablette, PC ; portrait et paysage) : accueil, liste des schémas tactiques, éditeur de terrain avec zoom, pincement, flèches, annulation et sauvegarde automatique.
- Tailles de l'éditeur calculées selon la taille de l'écran.
- Correction : terrain dessiné trop grand sur les écrans haute densité (double mise à l'échelle du Canvas).
- Nouvelle couche `Backend` (QtCore/QtSql) exposée à QML, avec tests.
- L'ancienne interface Widgets devient la cible `FootCoachWidgets` (retirée quand toutes les pages seront portées).
- Pages Effectif, Calendrier, Présences, Exercices, Séances : « Bientôt disponible » dans la version QML.
