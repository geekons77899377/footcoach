# Tests QML hors appareil

Ces tests chargent les vrais fichiers `qml/` dans Qt Quick (PySide6, rendu « offscreen ») avec un `Backend` simulé.
Ils ne remplacent pas un essai sur téléphone : ils ne simulent ni le vrai clavier Android, ni le verrouillage de
l'écran, ni le bouton retour du système.

    pip install PySide6 pillow
    python3 tests/qml/run_all.py

| Fichier | Vérifie |
|---|---|
| `t_live.py` | séance du jour : choix selon l'heure, démarrer, pause, reprise, suivant, dépassement, fin |
| `t_live2.py` | séance du jour : sans séance, prochaine séance, 5 formats d'écran |
| `t_live3.py` | séance du jour : ouverture de la fiche de l'exercice, reprise après relance de l'application |
| `t_back.py` | bouton retour : fermeture des fiches, du plein écran, retour à l'accueil, sortie depuis l'accueil |
| `t_fields.py` | **chaque champ de saisie** avec le clavier simulé (fenêtre réduite) : champ visible, en-tête immobile |

`t_fields.py` compte aussi les champs déclarés dans `qml/` : si on en ajoute un sans l'ajouter au test, il échoue.
Les fichiers de travail vont dans `tests/qml/.work/` (ignoré par git).
