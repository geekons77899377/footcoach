# FootCoach

Application Qt Quick / QML (Windows et Android) pour organiser les entraînements d'une équipe de football à 11.

## Prérequis

- Windows, Qt 6.9 ou plus (modules Qt Quick, Qt Quick Controls, Qt SQL avec le plugin SQLite ; Qt Widgets pour l'ancienne version)
- CMake 3.21+, MSVC ou MinGW (kit Qt correspondant)

## Compilation

Avec Qt Creator : ouvrir `CMakeLists.txt`, choisir le kit Qt 6.9+, compiler. La cible `FootCoach` est l'application QML (tactique et effectif ; les autres pages affichent « Bientôt disponible »). La cible `FootCoachWidgets` est l'ancienne version complète en Widgets, conservée jusqu'à la fin du portage.

En ligne de commande (adapter le chemin du kit Qt) :

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.8.x/msvc2022_64
cmake --build build
```

## Android (application QML, téléphone et tablette, portrait et paysage)

Prérequis : Qt 6.9.3 avec le composant Android, JDK 17 ou plus, SDK et NDK Android (voir l'assistant Android de Qt Creator).

1. Qt Creator : ouvrir `CMakeLists.txt`, choisir le kit **Android Qt 6.9.3 Clang arm64-v8a**, configurer.
2. Téléphone en USB (débogage USB activé), le choisir comme appareil, puis **Exécuter** : Qt Creator construit l'APK de
   débogage, l'installe et le lance.
3. APK signé à distribuer : *Projets > Construire > Étapes de construction > Build Android APK*, cocher « Sign package »
   avec votre keystore, compiler en **Release**. L'APK (`android-build-release…apk`) se trouve dans le dossier de build.
4. Les tests (`tst_*`) ne sont pas construits pour Android. La base est dans le dossier privé de l'application :
   **la désinstaller efface les données** (pas encore de sauvegarde).
5. Identité du paquet : `org.footcoach.app`. Ne pas la changer une fois l'application installée. Le code de version
   (`QT_ANDROID_VERSION_CODE` dans `CMakeLists.txt`) doit augmenter à chaque version à installer par-dessus la précédente.

Disposition téléphone : active sur Android ; sur PC, lancer avec la variable d'environnement `FOOTCOACH_COMPACT=1`
(barre « Menu » en haut au lieu de la liste à gauche, boutons agrandis).

## Déploiement (Windows)

```
windeployqt --qmldir qml build/FootCoach.exe
```

## Données

La base SQLite est créée au premier lancement dans le dossier de données de l'application
(`QStandardPaths::AppDataLocation`, sous `%APPDATA%`). Le chemin et la version du schéma
sont affichés dans la barre d'état.

## Migrations

Les scripts `sql/NNN_nom.sql` sont embarqués dans l'exécutable et appliqués dans l'ordre.
La version est stockée dans `PRAGMA user_version`. Pour ajouter une migration : créer le fichier
`sql/003_xxx.sql` et l'ajouter à la liste `SQL_MIGRATIONS` (CMakeLists.txt).
Règles : pas de `TRIGGER`, aucun `;` ni `--` dans un littéral de chaîne.

## Tests

Les tests Qt Test (`tests/tst_repositories.cpp`, base SQLite en mémoire) sont compilés par défaut
(`-DFOOTCOACH_BUILD_TESTS=OFF` pour les désactiver). Lancement : `ctest --test-dir build --output-on-failure`
ou exécuter `tst_repositories` depuis Qt Creator.

## Structure

```
sql/              migrations SQLite (001 schéma, 002 numéro de maillot unique, 003 archivage des exercices, 004 exercices de départ)
src/core/         accès base de données, modèles métier, repositories
src/ui/           fenêtres, dialogues, pages Effectif, Calendrier, Présences, Exercices, Séances et Tactique
tests/            tests Qt Test
```

## Règles métier du tableau tactique

- Page **Tactique** : liste des schémas à gauche, terrain à droite. Chaque schéma a un titre et peut être lié à un
  **exercice** et/ou à une **séance** (bouton « Propriétés… »). Supprimer l'exercice ou la séance ne supprime pas le schéma.
- Éléments : joueurs (numérotés automatiquement), adversaires, ballons, plots, buts mobiles ; flèches de course (pointillé),
  de passe (trait plein), de conduite (ondulé) et de **centre** (courbe ; « Inverser la courbure », touche C, change le côté de la courbure d'un centre sélectionné). Double-clic sur un joueur : changer son numéro (3 caractères maximum).
  Les **notes texte** (outil « Texte », plusieurs lignes possibles, 300 caractères maximum) se modifient par double-clic.
  « Couleur » change la couleur des joueurs, adversaires, plots et notes sélectionnés (palette de 7 couleurs).
  « Agrandir » / « Réduire » (touches + et -) redimensionnent les buts, plots et notes par pas de 25 % (de 30 % à 400 %).
  Les buts et les plots **pivotent** par pas de 45° : sélectionner, puis « Pivoter à droite » (touche R) ou « Pivoter à gauche » (Maj+R).
  En demi-terrain, un but posé est d'emblée horizontal.
- Formations 4-4-2, 4-3-3, 3-5-2 et 4-2-3-1, avec adversaires en miroir en option (terrain entier seulement).
  « Placer la formation » remplace les joueurs de l'équipe ; les autres éléments sont conservés.
- Terrain entier ou demi-terrain (but visé en haut). Changer de type garde les positions relatives.
- **Annuler / Rétablir** (Ctrl+Z / Ctrl+Y) pour toutes les actions ; l'historique repart de zéro quand on change de schéma.
- **Enregistrement automatique** après chaque action : le contenu est sérialisé en JSON dans `tactical_board.scene_json`
  (coordonnées normalisées de 0 à 1, version 1). Un schéma dont le JSON est illisible n'est jamais écrasé.
- Export PNG (2000 px de large). La fiche PDF d'une séance joint, dans une section « Schémas » sur une nouvelle page,
  les schémas liés à la séance et ceux liés aux exercices de son plan.

## Règles métier des exercices et du plan de séance

- La bibliothèque contient 17 exercices génériques au premier lancement (migration 004) : ils sont modifiables,
  duplicables, archivables et supprimables comme les vôtres.
- Un exercice **utilisé dans un plan** ne peut pas être supprimé (clé étrangère) : l'application propose de l'**archiver**.
  Il disparaît de la bibliothèque et du sélecteur, mais reste visible dans les plans existants. « Afficher les archivés » le retrouve.
- Le plan d'une séance (page Séances) est une liste ordonnée : ajout depuis la bibliothèque, monter / descendre, retirer,
  durée et note propres à cette séance. Les horaires sont calculés en enchaînant les durées à partir de l'heure de début.
- Si le total dépasse la durée de la séance, un **avertissement** rouge s'affiche (aucun blocage).
- « Exporter en PDF » produit une fiche A4 : en-tête, plan avec horaires, objectifs, consignes, matériel à prévoir, total.
  Limite connue : le moteur PDF de Qt peut couper une ligne d'exercice entre deux pages.

## Règles métier des présences

- « Faire l'appel » (page Calendrier) enregistre un statut par joueur : présent, absent, excusé, blessé.
  Enregistrer l'appel d'une séance « prévue » la passe en « réalisée » (case à décocher si besoin).
- Seules les séances **réalisées** comptent dans les statistiques.
- **Taux = présents ÷ (présents + absents)** : les excusés et les blessés ne comptent pas.
- Un joueur sans statut enregistré compte comme **absent**, mais seulement s'il est actif et si la séance a eu lieu
  à partir du jour où il a été ajouté à l'effectif (un joueur arrivé en cours de saison n'est pas pénalisé pour le passé).
  Un statut saisi explicitement compte toujours.
- La page Présences affiche la synthèse (classée par taux) et la grille joueurs × séances ; période au choix.

## Règles métier du calendrier

- Une séance = équipe + date/heure + durée (15 à 300 min) + lieu, thème, notes, statut (prévue / réalisée / annulée).
- « Séances récurrentes » génère des séances **indépendantes** (jours de la semaine + heure, entre deux dates,
  200 séances maximum) : aucune règle de série n'est conservée, chaque séance se modifie ou s'annule seule.
- Deux séances d'une même équipe qui se chevauchent déclenchent un avertissement avec confirmation
  (les séances annulées ne comptent pas ; finir pile quand l'autre commence n'est pas un chevauchement).
- Supprimer une séance efface ses présences et son plan d'exercices ; pour garder l'historique, utiliser le statut « Annulée ».

## Règles métier de l'effectif

- Un numéro de maillot est unique parmi les joueurs **actifs** d'une équipe (index partiel, migration 002).
- « Désactiver » conserve le joueur et son historique ; « Supprimer définitivement » efface aussi
  ses présences et blessures (confirmation obligatoire). Supprimer une équipe supprime ses joueurs et séances.
