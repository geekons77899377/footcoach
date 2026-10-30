# Journal des modifications

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
