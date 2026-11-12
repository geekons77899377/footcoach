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
| `t_blockboard.py` | terrain d'un bloc dans le plan : états (sans terrain, avec, illisible, séance annulée), création à la demande, ouverture du bon terrain, sauvegarde automatique et au retour, défilement conservé, retour Android, import pendant l'édition, 5 formats. **Backend simulé** : le C++ réel est testé par `tst_backend` et `tst_board` |
| `t_exboard.py` | fiche d'un exercice : section « Terrain » (miniature, ouverture dans Tactique, création à la demande), bureau et téléphone. **Backend simulé** : le C++ réel est testé par `tst_backend` |
| `t_category.py` | catégories d'exercices (ex. U10-U11) distinctes de l'« Activité » : filtres, choix et création dans la fiche, `categoryId` à l'enregistrement, bureau et téléphone. **Backend simulé** : le C++ réel est testé par `tst_backend` et `tst_dataformat` |
| `t_slots.py` | créneaux réguliers : liste, aperçu d'une modification en bloc, confirmation avant suppression, déprogrammation (à venir ou aussi passées sans appel), accès depuis le menu d'une séance, bouton retour, état vide, téléphone et bureau. **Backend simulé** : le C++ réel est testé par `tst_slots` |
| `t_select.py` | Effectif : sélection multiple par appui long (cases à cocher, « Tous » / « Aucun », poubelle, confirmation, retour et croix), téléphone, tablette et bureau ; Données : « Tout effacer » (avertissement avec le contenu actuel, mot EFFACER à taper). **Backend simulé** : le C++ réel est testé par `tst_reset` |
| `t_fields.py` | **chaque champ de saisie** avec le clavier simulé (fenêtre réduite) : champ visible, en-tête immobile |

`t_fields.py` compte aussi les champs déclarés dans `qml/` : si on en ajoute un sans l'ajouter au test, il échoue.
Les fichiers de travail vont dans `tests/qml/.work/` (ignoré par git).
