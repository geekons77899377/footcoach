# Journal des modifications

## 0.8.2 — 2026-10-30
- Équipes : le champ « Saison » propose trois pastilles (saison précédente, en cours, suivante, au format 2026-2027) et, pour une nouvelle équipe, la saison en cours est préremplie. La saison sportive commence le 1er juillet. La saisie libre reste possible.
- Calcul vérifié ici (hors QML) pour plusieurs dates, dont les limites du 30 juin et du 1er juillet. Non vérifié : l'affichage dans Qt Quick (non disponible ici) et sur appareil.
- Aucun changement de schéma (4) ni de format de fichier (1).

## 0.8.1 — 2026-10-30
Aucun changement de format, de schéma ni d'interface (schéma de données 4, format de fichier 1).
- Tests de compatibilité du format `.footcoach` (`tests/tst_dataformat.cpp`) : structure de la version 1 figée, import de fichiers de référence des schémas 1 à 4, seize fichiers invalides refusés sans toucher aux données, sauvegarde automatique valide.
- Fichiers de référence dans `tests/fixtures/` : le schéma 4 est un export réel de la 0.8.0 ; les schémas 1 à 3 sont reconstitués (aucune version publiée n'a écrit de fichier avec ces schémas).
- Nouvelle documentation `docs/versionnage.md` : numéros de version, règles de migration et de compatibilité, exports partiels en version 2 (jamais lisibles comme remplacement complet par une ancienne version), inventaire des données personnelles pour l'option « sans données personnelles » (qui ne garantit pas l'anonymat).
- Constaté, non corrigé : un `schema` absent ou nul affiche le message « version plus récente ».
- Non fait : types d'export/import au choix (spécifiés dans la doc, pas implémentés).

## 0.8.0 — 2026-10-30
- Nouvelle page « Données » (menu Plus, ou barre latérale) : « Créer le fichier » enregistre toutes les données dans un fichier `.footcoach` (équipes, joueurs, séances, plans, exercices, présences, blessures, schémas) ; « Choisir un fichier » l'importe sur un autre appareil (téléphone ↔ tablette). Le fichier peut être envoyé par WhatsApp, e-mail ou Drive.
- L'import REMPLACE les données de l'appareil, après un récapitulatif (contenu du fichier et contenu actuel). Il est fait en une seule transaction : en cas de problème, rien n'est modifié. Les trois dernières copies de l'état précédent sont gardées dans le dossier privé de l'application (`sauvegardes`).
- Fichiers refusés : format inconnu, fichier trop gros (64 Mo), version plus récente que l'application, colonnes ou lignes inconnues ou mal formées, contenu incohérent.
- Après un import, les pages ouvertes (effectif, calendrier, séances, exercices, tactique) referment leurs fiches et se rechargent.
- Tests : aller-retour complet entre deux bases, fichiers abîmés, import incohérent annulé (12 tests dans le backend). Une classe `Database` accepte un nom de connexion, pour ouvrir deux bases à la fois.
- Non vérifié ici : les sélecteurs de fichier sur Android (enregistrement et ouverture, y compris les fichiers reçus par WhatsApp), et la compilation Qt Quick.

## 0.7.0 — 2026-10-30
- Exercices en QML : bibliothèque (recherche, filtre par catégorie, archivés), fiche d'un exercice (titre, catégorie, objectif, durée, joueurs, matériel, description), archivage, suppression refusée si l'exercice est dans un plan.
- Séances en QML : séances de l'équipe (à venir / passées) avec résumé du plan, plan de la séance : ajout d'un exercice de la bibliothèque, ordre (monter / descendre), durée et notes de la séance, horaires calculés, total comparé à la durée de la séance (avertissement s'il dépasse), matériel à prévoir sans doublon.
- Backend : API exercices et plan (`exercises`, `saveExercise`, `plan`, `addToPlan`…), 11 tests dont un nouveau couvrant exercices et plan.
- Pas encore fait : export PDF de la fiche de séance (sur Android, il demande l'enregistrement dans un dossier choisi ou le partage), liaison d'un schéma tactique à un exercice.
- Non vérifié ici : compilation Qt Quick et affichage sur appareil ; l'affichage a été contrôlé hors écran (téléphone et tablette) avec un backend simulé.
- Doc de publication : un seul ABI (arm64-v8a) suffit pour Google Play.

## 0.6.0 — 2026-10-30
Préparation de la publication sur Google Play (rien n'est publié).
- Icône : ballon rond vert (icône adaptative Android 8+ et icône classique), icône 512 × 512 pour la fiche (`store/icon-512.png`), source `store/icon.svg`, script `tools/make_icons.py`.
- Android : `targetSdk` 36 déclaré explicitement (exigé pour les nouvelles applications depuis le 31/08/2026), `minSdk` 23, `versionCode` 12 ; les ressources d'icône viennent du dossier `android/`.
- `.gitignore` : clés et mots de passe de signature (`*.jks`, `*.keystore`, `keystore.properties`…).
- Documentation : `docs/publication.md` (clé d'upload, build Release APK/AAB, fiche, test interne et fermé, accès à la production), `docs/confidentialite.md` (brouillon de politique de confidentialité), `store/fiche.md` (textes de la fiche).
- Non vérifié ici : compilation Android, affichage de l'icône, `targetSdkVersion` et permissions de l'APK produit.

## 0.5.3 — 2026-10-30
- Tactique, écran tactile : corrige le placement des joueurs et autres éléments. Chaque toucher déclenchait aussi le « clic droit » (retour à la sélection), si bien que l'outil était remis sur « sélection » avant la pose. Ce retour à la sélection est maintenant réservé à la souris.
- Vérifié ici avec un écran tactile simulé (QML hors écran) : avant, aucun élément posé ; après, un élément par toucher. Non vérifié sur l'émulateur ni sur un appareil.

## 0.5.2 — 2026-10-30
- Calendrier : mois et séances côte à côte dès 640 px de large (tablette 10 pouces en portrait), au lieu de 760 px.

## 0.5.1 — 2026-10-30
- Les feuilles du bas (équipe, actions d'une séance, « Plus »…) s'ouvrent toutes au-dessus de la barre de navigation et de la zone sûre, et défilent s'il y a beaucoup de choix (avant : la liste des équipes passait sous la barre du bas).

## 0.5.0 — 2026-10-30
- Calendrier en QML : mois avec points sur les jours de séance, liste Jour / Semaine / Mois, création et modification d'une séance (sélecteurs de date et d'heure, durée, lieu, thème, statut, notes), alerte en cas de chevauchement, changement de statut, suppression, séances récurrentes avec aperçu du nombre de séances.
- Appel : un statut par joueur (présent, absent, excusé, blessé), « Tous présents / absents », note par joueur, option « marquer la séance comme réalisée ».
- Suivi (présences) : taux par joueur sur Tout / 30 jours / 3 mois / 12 mois, détail séance par séance au toucher. Taux = présents ÷ (présents + absents).
- L'équipe choisie est la même dans Effectif, Calendrier et Suivi.
- Non vérifié ici : compilation Qt Quick et affichage sur appareil.
- Backend : séances (`sessions`, `sessionDates`, `saveSession`, `setSessionStatus`, `deleteSession`), séances récurrentes (`previewRecurrence`, `createRecurring`), appel (`attendanceSheet`, `saveAttendance`) et statistiques de présence (`attendanceStats`), avec test. Mêmes règles que la version Widgets : chevauchements signalés (confirmation), pas d'appel pour une séance annulée, joueurs inactifs gardés dans les appels existants.

## 0.4.3 — 2026-10-30
- Date de naissance : saisie libre (`02062016`, `02/06/2016`, `2.6.2016`…) remise en forme en JJ/MM/AAAA, et sélecteur de date par roues (bouton calendrier) qui s'ouvre sur une année adaptée à la catégorie de l'équipe (U15 : 2012 ; seniors : 2001 ; sinon environ 12 ans).
- Tactique : l'aide « Choisissez un outil… » ne s'affiche plus qu'une fois, disparaît après 8 secondes, au toucher ou dès qu'on choisit un outil.

## 0.4.2 — 2026-10-30
- Tactique : nouvel outil « Multi » (sélection multiple) : toucher ajoute ou retire un élément ou une flèche, glisser sur le terrain entoure une zone, bouton « Tout » ; les actions (couleur, rotation, taille, suppression) et le déplacement s'appliquent au groupe.
- Le menu « Plus » s'ouvre maintenant au-dessus de la barre de navigation du bas.

## 0.4.1 — 2026-10-30
- Correction : erreurs QML « Cannot call method 'palette' of null » à la fermeture (le backend était détruit avant le moteur QML). Une erreur d'ouverture de la base est aussi écrite dans la console.

## 0.4.0 — 2026-10-30
- Page Effectif en QML : choix de l'équipe (création, modification, suppression), recherche par nom ou numéro, filtre par poste, joueurs inactifs, fiche joueur (prénom, nom, numéro, poste, pied fort, naissance, téléphone, e-mail, notes), désactivation et suppression définitive. Liste et fiche côte à côte en grand écran.
- Backend : `teams`, `saveTeam`, `deleteTeam`, `players`, `savePlayer`, `setPlayerActive`, `deletePlayer`, avec les mêmes règles que la version Widgets (nom et prénom obligatoires, numéro unique parmi les joueurs actifs, contrôle de la date) et un test dédié.
- Non vérifié ici : compilation Qt Quick et affichage sur appareil (le clavier peut recouvrir le bas de la fiche).

## 0.3.0 — 2026-10-30
- Interface refaite en QML (téléphone, tablette, PC ; portrait et paysage) : accueil, liste des schémas tactiques, éditeur de terrain avec zoom, pincement, flèches, annulation et sauvegarde automatique.
- Tailles de l'éditeur calculées selon la taille de l'écran.
- Correction : terrain dessiné trop grand sur les écrans haute densité (double mise à l'échelle du Canvas).
- Nouvelle couche `Backend` (QtCore/QtSql) exposée à QML, avec tests.
- L'ancienne interface Widgets devient la cible `FootCoachWidgets` (retirée quand toutes les pages seront portées).
- Pages Effectif, Calendrier, Présences, Exercices, Séances : « Bientôt disponible » dans la version QML.
