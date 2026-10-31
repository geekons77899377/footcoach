# Journal des modifications

## 0.8.11 — 2026-10-31
- Préparation Google Play (aucun changement de code ni de schéma) :
  - Contact public : `footcoach.app@gmail.com` (fiche et politique de confidentialité).
  - Description courte de la fiche ramenée à 77 caractères (limite 80).
  - Politique de confidentialité : le téléphone et l'adresse e-mail des joueurs sont maintenant listés parmi les données traitées (ils étaient saisis par l'application mais seulement évoqués dans la section Export).
  - `minSdk` laissé à 23 pour l'instant, en attente de décision (Qt 6.9 ne prend officiellement en charge qu'Android 9, API 28, et au-delà).

## 0.8.10 — 2026-10-31
- Android : ajout de `android/AndroidManifest.xml` (remplace le modèle généré par Qt).
  - **Icône :** `android:icon` et `android:roundIcon` pointent maintenant vers l'icône de l'application. Avant, le manifeste généré n'en déclarait aucune, d'où l'icône Android par défaut (constaté dans le manifeste généré par la 0.8.9).
  - **Permissions :** plus aucune. Qt ajoutait `INTERNET`, `ACCESS_NETWORK_STATE` et `WRITE_EXTERNAL_STORAGE` (relevées dans le manifeste généré), alors que l'application n'utilise ni le réseau ni le stockage partagé (l'export et l'import passent par le sélecteur de fichiers du système). Ceci met le manifeste en accord avec la politique de confidentialité.
  - **Sauvegarde automatique Android désactivée** (`allowBackup="false"`) : les données des joueurs restent sur l'appareil. Le transfert d'un appareil à l'autre passe par l'export / import de fichier.
- Non vérifié ici (pas d'Android ni de Qt Quick) : la compilation et l'installation. À contrôler sur l'appareil : icône dans le lanceur, application qui démarre, export / import de fichier, et dans le manifeste généré : `android:icon` présent, aucune `uses-permission`. Si un champ `%%INSERT_...%%` était mal orthographié, la compilation échouerait avec une erreur visible.
- `docs/publication.md` : valeurs mises à jour (versionCode, permissions, manifeste).
- Aucun changement d'identifiant (`org.footcoach.app`), de schéma (4) ni de format de fichier (1).

## 0.8.9 — 2026-10-31
- Nouvelle icône : un joueur qui frappe le ballon vers un but, sur fond vert (concept « vue de côté », redessiné en vectoriel, source unique `store/icon.svg`). Elle remplace le ballon vert sur fond bleu nuit.
- Android : icône adaptative en deux couches (fond pelouse + premier plan joueur, but et ballon cadré dans la zone sûre de 66 dp) et icône classique en carré arrondi. Google Play : `store/icon-512.png` (512 × 512, opaque).
- Windows : nouvelle icône d'exécutable (`windows/footcoach.ico`, 16 à 256 px, via `windows/footcoach.rc`). Première fois que l'exécutable Windows a une icône propre.
- `tools/make_icons.py` régénère tout depuis `store/icon.svg`.
- Vérifié ici : génération des fichiers, tailles et transparence, superposition fond + premier plan avec masque rond et zone sûre, lisibilité à 48 px. Non vérifié : affichage sur le Redmi et la tablette, compilation de la ressource Windows (MSVC), fiche Play (rien n'est envoyé à Google Play).
- Aucun changement d'identifiant (`org.footcoach.app`), de schéma (4) ni de format de fichier (1).

## 0.8.8 — 2026-10-31
- **Correction PC : la souris ne fonctionnait plus.** Depuis la 0.8.3, la couche transparente qui ferme le clavier (`KeyboardDismiss`) interceptait les clics de souris avant les boutons : menu, boutons, cartes ne répondaient plus à la souris. Cette couche ne réagit maintenant qu'à l'écran tactile et au stylet (la souris n'ouvre pas de clavier virtuel). Sur téléphone et tablette, le comportement du clavier reste celui de la 0.8.3/0.8.5.
- Vérifié ici hors écran : avant correction, clic souris sur les entrées du menu latéral → aucune réaction ; après correction → la section change à chaque clic. Clic souris sur le bouton plein écran de l'éditeur : fonctionne. Fermeture du clavier au toucher (champ Notes, dialogue d'équipe, zone assombrie) : inchangée. Non vérifié : sous Windows réel.
- Mon banc d'essai voyait déjà un clic souris sans effet lors de l'essai du plein écran (0.8.4) ; je l'ai pris pour une limite de l'outil au lieu d'y voir ce défaut. Les essais de souris sont maintenant faits systématiquement.
- Aucun changement de schéma (4) ni de format de fichier (1).

## 0.8.7 — 2026-10-31
- Navigation latérale : quand la fenêtre est trop basse (téléphone en paysage : 390 px de haut pour 8 entrées), la colonne d'icônes défile maintenant. Avant, Séances, Tactique et Données étaient hors de l'écran et inatteignables, ce qui correspond très probablement au « on ne peut pas scroller en paysage » signalé.
- Vérifié ici hors écran sur 7 formats (téléphone 412×892, petit téléphone 360×640, téléphone paysage 844×390, tablette 800×1280 et 1280×800, PC 1280×800 et 1000×600), toutes sections plus l'éditeur tactique ; mode de navigation attendu pour chaque format (bas, colonne d'icônes, menu latéral). Chemin d'ouverture Android simulé : fenêtre 412×892 inchangée. Non vérifié : appareils réels.
- Aucun changement de schéma (4) ni de format de fichier (1).

## 0.8.6 — 2026-10-31
- Version PC : la fenêtre s'ouvrait à la taille d'un téléphone (412 × 892), donc avec la mise en page « téléphone » (barre de navigation en bas). Sur PC, la fenêtre s'ouvre maintenant en 1280 × 820 au plus, réduite si l'écran est plus petit, ce qui donne le menu latéral et la liste à côté de l'éditeur. Android et iOS ne changent pas.
- Cause de mon côté : mes essais précédents simulaient seulement des tailles de téléphone et fixaient la taille de la fenêtre, donc ils ne passaient jamais par la taille d'ouverture réelle d'une fenêtre de PC.
- Vérifié ici hors écran à 1280 × 800 : toutes les sections et l'éditeur tactique en mode « menu latéral ». Non vérifié : la fenêtre réelle sous Windows (taille d'ouverture réellement obtenue, écrans à forte mise à l'échelle).
- Aucun changement de schéma (4) ni de format de fichier (1).

## 0.8.5 — 2026-10-31
- Éditeur tactique, sélection multiple : deux nouveaux boutons dans la barre d'actions (visibles quand plusieurs éléments sont sélectionnés).
  - **Aligner** : à gauche, centré horizontalement, à droite, en haut, centré verticalement, en bas, et répartir horizontalement ou verticalement (3 éléments minimum, espacement régulier entre les deux extrêmes, ordre conservé). « Gauche », « haut », etc. sont ceux de l'écran, quelle que soit l'orientation du terrain. Les flèches sélectionnées ne bougent pas avec l'alignement.
  - **Faire tourner le groupe** : 90° à droite ou à gauche, 45° à droite ou à gauche, demi-tour, autour du centre de la sélection. Les positions des éléments et les extrémités des flèches sélectionnées tournent ; les buts, plots et notes tournent aussi sur eux-mêmes. Les calculs tiennent compte des proportions du terrain (105 × 68 m). Si un élément sortirait du terrain, rien n'est modifié et un message l'indique.
  - Les boutons « Pivoter » existants continuent de faire tourner chaque élément sur lui-même. Annuler et rétablir fonctionnent pour les deux actions.
- Clavier : dans les boîtes de dialogue (notes d'un exercice du plan, texte d'un schéma, équipe, choix d'un exercice), toucher la zone assombrie autour de la boîte ferme maintenant le clavier (limite signalée en 0.8.3). Dans les fiches joueur et séance, le champ Notes se comportait déjà correctement dans mes essais.
- Vérifié ici hors écran (Qt 6.12 via PySide6, orientations paysage et portrait) : chaque alignement, la répartition, la rotation (4 × 90° ramène exactement au départ, résultat identique dans les deux orientations), annuler, et le clavier dans la boîte d'équipe. Non vérifié : appareil réel et clavier virtuel Android.
- Aucun changement de schéma (4) ni de format de fichier (1).

## 0.8.4 — 2026-10-31
- Éditeur tactique : nouveau bouton « Terrain en plein écran » (colonne de boutons à droite du terrain, sous le zoom). Il masque l'en-tête et la barre d'outils pour donner tout l'écran au terrain, surtout utile sur téléphone en paysage. Le même bouton (« Quitter le plein écran ») revient à l'affichage normal ; le plein écran est aussi quitté en ouvrant un autre schéma.
- En plein écran, trois boutons flottants en haut à gauche gardent l'essentiel à portée : l'outil actif (ouvre la liste des outils, qui défile), annuler et rétablir. Les aides de la sélection multiple et des outils se décalent pour ne pas les recouvrir.
- Plein écran = l'application occupe tout l'espace de l'éditeur ; la barre d'état et de navigation du système ne sont pas masquées (non fait volontairement, non vérifiable ici).
- Vérifié ici hors écran (Qt 6.12 via PySide6) en 844×390 (paysage) et 412×892 (portrait) : bascule, retour, redimensionnement du terrain, ouverture de la liste des outils. Les interactions de l'éditeur (placement, déplacement, flèches, zoom, pincement) donnent les mêmes résultats qu'avant. Non vérifié : rendu sur appareil.
- Non traité : le bouton Retour d'Android ne quitte pas encore le plein écran (aucune gestion du bouton Retour n'existe encore dans l'application).
- Aucun changement de schéma (4) ni de format de fichier (1).

## 0.8.3 — 2026-10-31
- Clavier : toucher en dehors d'un champ de saisie retire maintenant le focus au champ et ferme le clavier (signalé sur le Redmi : le clavier restait ouvert). Une couche transparente au-dessus des pages (`KeyboardDismiss.qml`) le fait pour les pages, formulaires et listes défilantes ; les boîtes de dialogue qui ont un champ (équipe, saisie de texte, durée d'un exercice, choix d'un exercice) ont la leur.
- Toucher un bouton, un autre champ ou le champ déjà actif se comporte comme avant (le clavier reste ouvert pour un champ).
- Limite connue : dans une boîte de dialogue qui ne se ferme pas en touchant à l'extérieur, toucher la zone assombrie autour ne ferme pas le clavier (toucher dans la boîte le ferme).
- Vérifié ici hors écran avec de vrais événements tactiles simulés (Qt 6.12 via PySide6) : avant, le champ gardait le focus après un toucher ailleurs ; après, il le perd. Le placement, le déplacement, les flèches, le zoom et le pincement de l'éditeur tactique donnent les mêmes résultats qu'avant. Non vérifié : le comportement du clavier virtuel réel sur Android (Qt 6.9.3), qui reste à confirmer sur appareil.
- Documentation : règles de tags (`vX.Y.Z` après retour de test, `vX.Y.Z-schN` à chaque changement de schéma) dans `docs/versionnage.md`.
- Aucun changement de schéma (4) ni de format de fichier (1).

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
