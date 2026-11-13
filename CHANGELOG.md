# Journal des modifications

## Non publié — formations à 8 et adversaires en même nombre
- **Feuille « Formation » du terrain** : choix du format **À 11** ou **À 8**. À 8 (gardien + 7) : **3-3-1, 2-3-2, 3-2-2, 2-4-1**. Pour un match, le format est d'office « À 8 » quand 8 titulaires au plus sont convoqués, sinon « À 11 » ; on le change d'un appui.
- **Adversaires** : l'interrupteur devient « Avec adversaires (autant de joueurs) » : ils sont toujours aussi nombreux que l'équipe (8 contre 8, 11 contre 11), placés avec la même formation en miroir. Avant, 11 adversaires étaient posés même pour une équipe à 8.
- `Tactics::formationNames(int perSide)` et `Backend::formationNamesFor(int)` ; le placement des titulaires par poste (`SquadFormation.js`) fonctionne tel quel avec ces noms.
- Pas de réglage « format » enregistré par équipe ou par match (le choix n'est pas mémorisé) ; les noms de formation à 8 sont des systèmes courants, non vérifiés dans un document de la FFF.
- Tests : `tst_board::eightASideFormationsHaveEightPlayersAndEqualOpponents`, `tst_backend::formationsAndPalette` étendu, `t_squadboard.py` étendu (backend **simulé**).

## Non publié — feuille de partage pour la feuille de challenge
- **Match > « Créer la feuille »** : sur Android, le PDF est d'abord proposé via la **feuille de partage** du système (imprimer, enregistrer, envoyer) au lieu de l'ouverture directe d'un fichier du cache, que beaucoup d'appareils refusent (le sélecteur « Enregistrer » s'affichait alors). Si le partage est impossible, l'ancien parcours reste (ouverture externe, puis sélecteur de dossier).
- `Backend::shareFile(url, mime, titre)` (partage d'un fichier local) ; `ShareBridge.partager` reçoit le type MIME ; `footcoach_exports_paths.xml` expose aussi le cache (droit donné URI par URI) ; `<queries>` pour `application/pdf`.
- Non vérifié sur appareil ni avec le NDK : le code Android n'est ni compilé ni exécuté ici. Test `t_challenge.py` étendu (backend **simulé**).

## Non publié — déplacer des joueurs vers une autre équipe
- **Effectif, mode sélection** : nouveau bouton **« Déplacer »** (flèche) entre « Tous » et la poubelle, visible s'il existe une autre équipe. On choisit l'équipe d'arrivée (nom et club), on confirme, puis un compte rendu s'affiche.
- Règles (`Backend::movePlayers`) : présences, blessures, photo, contacts et convocations passées suivent le joueur. Un **numéro de maillot** déjà pris dans l'équipe d'arrivée par un joueur actif est retiré. Un joueur qui y existe déjà (**même nom** ou **même licence**, casse ignorée) n'est pas déplacé : il est nommé dans le compte rendu. Tout ou rien pour les autres.
- `PromptDialog` : propriété `cancelVisible` pour un simple message avec un bouton OK.
- Correction : les icônes des boutons « Importer une liste de joueurs » et « Importer des photos » de l'Effectif étaient vides (glyphes `text` et `photo` absents de `Icon.qml`). Ajout aussi du glyphe `move`.
- Tests : `tst_reset::movePlayersToAnotherTeam` (numéro gardé ou retiré, nom ou licence en double, historique conservé, cas d'erreur) ; `t_select.py` étendu (déplacement, compte rendu, bouton masqué avec une seule équipe ; backend **simulé**).

## Non publié — suppression groupée dans l'Effectif et « Tout effacer »
- **Effectif, sélection multiple** : un **appui long** sur un joueur (liste ou cartes) passe en mode sélection. Chaque joueur a une case à cocher ronde ; toucher un joueur le coche ou le décoche (sans ouvrir sa fiche). L'en-tête devient une barre « N sélectionnés » avec une croix (annuler), **« Tous » / « Aucun »** (joueurs affichés, donc selon les filtres et « Afficher les joueurs inactifs ») et une **poubelle rouge**. Confirmation avant suppression, qui rappelle de plutôt désactiver un joueur parti pour garder l'historique. Le bouton retour d'Android annule la sélection ; changer d'équipe aussi.
- Suppression **tout ou rien** (`Backend::deletePlayers`) : présences, blessures, photos, convocations et contacts des joueurs supprimés partent avec eux.
- **Données > « Tout effacer »** : supprime toutes les données de l'appareil et revient à l'état d'installation (club « Mon club », bibliothèque d'exercices et exemples U10-U11 avec leurs terrains, mêmes identifiants qu'une base neuve). Double confirmation : un avertissement qui affiche le contenu actuel et conseille « Envoyer mes données » d'abord, puis le mot **EFFACER** à taper (majuscules et espaces ignorés). Une copie de l'état précédent est écrite avant dans le dossier interne des sauvegardes (`avant-effacement-…-NNN.footcoach`, 3 gardées) ; elle n'est pas accessible depuis l'interface.
- Les données de départ sont reprises des migrations elles-mêmes (`Database::seedStatements` : leurs INSERT et UPDATE, dans l'ordre), pour qu'une future migration de données soit prise en compte sans rien dupliquer.
- Tests : C++ `tst_reset` (retour à une base neuve comparée table par table, copie de sécurité importable, rotation sans écrasement, suppression groupée) ; QML `t_select.py` (86 vérifications, téléphone, tablette, bureau, backend **simulé**). **Non vérifié :** appui long au doigt sur appareil, Qt 6.9 / Windows / Android.

## Non publié — créneaux réguliers : modifier ou déprogrammer des entraînements en bloc (schéma 10)
- **Calendrier > bouton « Créneaux réguliers »** (icône répétition) : liste des créneaux de l'équipe (jours, horaire, thème, lieu, période, nombre de séances à venir et avec appel), avec **« Nouveau créneau »**. Le formulaire « Séances récurrentes » devient « Nouveau créneau régulier » : il crée les séances comme avant et enregistre en plus le créneau.
- **Modifier un créneau** : la nouvelle règle (jours, heure, durée, lieu, thème, période) s'applique aux **séances à venir** du créneau (aujourd'hui compris). Jour gardé : séance mise à jour, plan d'exercices conservé ; jour retiré : séance supprimée ; jour nouveau : séance créée. Un **aperçu** donne le nombre de séances modifiées, ajoutées, supprimées (dont avec un plan) et gardées ; une **confirmation** est demandée si des séances vont être supprimées, et en cas de chevauchement avec une autre séance de l'équipe.
- **Respect des séances déjà travaillées** : une séance avec un **appel enregistré** n'est jamais modifiée ni supprimée ; un champ (heure, durée, lieu, thème) **modifié à la main** sur une séance n'est pas écrasé ; les séances passées ne changent pas.
- **Déprogrammer un créneau** : supprime d'un coup ses séances à venir sans appel ; s'il reste des séances passées sans appel, on choisit « Séances à venir » ou « À venir et passées sans appel » (cas d'une erreur de saisie). Les séances gardées restent dans le calendrier comme séances seules.
- Menu d'une séance générée par un créneau : nouvelle entrée **« Modifier le créneau régulier »**.
- Migration `010_slot_presence_contact.sql` (schéma 9 → 10) : table `training_slot`, colonne `training_session.slot_id` ; colonne `match_player.presence` (`unknown` par défaut, puis `present`, `absent`, `excused`, `injured`) et table `player_contact` (2 contacts par joueur : libellé, téléphone, e-mail), préparées pour la relance des parents avant un match (**pas encore d'écran**). Les colonnes existantes `player.phone` / `player.email` sont recopiées dans le contact 1.
- Enregistrer les convoqués d'un match **ne remet plus à zéro** la présence des joueurs toujours convoqués (seuls les joueurs retirés sont effacés).
- Export/import `.footcoach` : tables `player_contact` et `training_slot`, colonnes `slot_id` et `presence` ; un export porte `schema = 10` et est refusé par une application plus ancienne. Un fichier plus ancien s'importe ; son téléphone et son e-mail de joueur deviennent le contact 1.
- Limite : les séances créées en série **avant** cette version ne sont rattachées à aucun créneau et restent gérées une par une.
- Tests : C++ `tst_slots` (13 cas : création, aperçu, modification à venir seulement, champs modifiés à la main, changement de jours, séances avec appel, chevauchements, déprogrammation avec ou sans le passé, présence conservée, migration 010, export/import) ; `tst_dataformat` mis à jour (schéma 10) ; QML `t_slots.py` (52 vérifications, backend **simulé**). Vérifié avec Qt 6.4 (C++) et PySide6 6.12 (QML) sous Linux. **Non vérifié :** compilation Qt 6.9 / Windows / Android, rendu sur appareil.

## Non publié — suppression du préchauffage du clavier (0.9.8)
- Retrait du contournement ajouté en 0.9.8 : ouverture puis fermeture du clavier 1,5 s après le lancement sur Android, sur un champ invisible. Il n'avait rien corrigé et faisait apparaître brièvement le clavier à chaque lancement. Disparaissent aussi le champ invisible, les deux minuteries et la carte « Clavier (essai) » de la page Données.
- Le problème d'origine (premier clavier affiché sans ses touches après le lancement) n'est donc pas résolu ; sa cause n'a pas été trouvée. Le réglage `keyboard/warmup` éventuellement enregistré sur l'appareil devient sans effet.

## Non publié — révision entre parenthèses dans le champ « Version »
- Paramètres > Version affiche désormais « FootCoach 0.10.0 (2eb040d) » : les 7 premiers caractères du SHA-1, « , modifié » si des fichiers suivis n'étaient pas commités à la compilation. Le SHA-1 complet est en sous-titre de la même ligne. La ligne séparée « Révision (SHA-1) » disparaît.

## Non publié — SHA-1 du commit affiché avec la version
- Paramètres > Informations : nouvelle ligne **« Révision (SHA-1) »** avec le SHA-1 complet (40 caractères) du commit compilé ; « modifications non commitées » est ajouté si des fichiers suivis étaient modifiés au moment de la compilation. « inconnu » si git n'est pas disponible ou hors dépôt.
- Le SHA est recalculé à **chaque compilation** (`cmake/GitRevision.cmake`, cible `footcoach_git_revision`), pas seulement à la configuration, et l'en-tête généré n'est réécrit que s'il change. La version (`0.10.0`) est inchangée.
- Test : `t_support.py` (SHA affiché, interface, backend simulé). Le script CMake est vérifié seul (SHA, indicateur de modifications, absence de git, pas de réécriture inutile). **Non vérifié :** le câblage CMake complet et la compilation de `main_qml.cpp` (cible Qt 6.9 non compilable ici), l'affichage sur appareil.

## Non publié — correction : le terrain préconfiguré suit l'exercice dans le plan de séance
- Ajouter à une séance un exercice qui a un terrain préconfiguré (par exemple un des exemples U10-U11) **recopie ce terrain dans le bloc** ; avant, le bloc n'avait aucun terrain et il fallait en créer un vide à la main. Le terrain du bloc est une **copie** : le modifier ne change pas l'exercice, et inversement. Un exercice sans terrain de référence se comporte comme avant.
- Les blocs déjà présents dans des séances (ajoutés avant la correction) ne sont pas modifiés automatiquement : « Ajouter un terrain » sur le bloc recopie désormais le terrain de l'exercice au lieu de créer un terrain vide.
- Un terrain de référence illisible n'est jamais recopié (le bloc reçoit alors un terrain vide).
- Test : `addingAnExerciseToThePlanCopiesItsReferenceBoard` (backend réel, base neuve avec les exemples). Non vérifié sur appareil.

## Non publié — catégories d'exercices (« U10-U11 » et suivantes) et « Activité » (schéma 9)
- Nouvelle notion de **Catégorie** d'exercice, définie par l'utilisateur : **U10-U11** est la première ; on en crée d'autres depuis la fiche d'un exercice (pastille **« + Nouvelle »**). L'ancienne « Catégorie » (échauffement, technique, tactique, physique, jeu, retour au calme) s'appelle désormais **« Activité »** (écrans QML et ancienne interface Widgets). Un exercice a une activité (obligatoire) et peut avoir une catégorie (facultative).
- Bibliothèque et choix d'exercice d'une séance : filtres **Catégorie** (affiché seulement s'il y en a) et **Activité**, combinables ; le nom de la catégorie figure dans la ligne de l'exercice.
- Migration `009_exercise_category.sql` (schéma 8 → 9) : table `exercise_category`, colonne `exercise.category_id` (suppression d'une catégorie : les exercices restent, sans catégorie). Les **27 exemples** passent dans la catégorie U10-U11 et **perdent leur préfixe « U10-U11 · »** (titres d'exercice et de terrain). Un exercice créé par l'utilisateur dont le titre commence par « U10-U11 · » subit le même traitement.
- Export/import `.footcoach` : la table `exercise_category` s'ajoute (avant `exercise`) ; un export porte `schema = 9` et est refusé par une application plus ancienne. Les fichiers des schémas 1 à 7 restent importables (sans catégorie). Un fichier fait au schéma 8 (avant cette mise à jour) s'importe, mais ses exemples gardent le préfixe « U10-U11 · » dans leur titre et n'ont pas de catégorie.
- Backend : `categories()`, `addCategory`, `renameCategory`, `deleteCategory` ; `saveExercise` accepte `categoryId`. **Pas encore d'écran** pour renommer ou supprimer une catégorie (le backend est prêt).
- Tests : C++ (`tst_backend` : migration, création/renommage/suppression, unicité sans tenir compte de la casse, activité indépendante ; `tst_dataformat` : migration d'une base du schéma 4, aller-retour export/import, fichier du schéma 7) ; `t_category.py` (interface, backend **simulé**). Non vérifié : rendu sur appareil, Qt 6.9.

## Non publié — le terrain d'un exercice est visible et ouvrable depuis sa fiche
- Fiche d'un exercice : nouvelle section **« Terrain »** avec la miniature du schéma de référence et le bouton **« Ouvrir le terrain »** (ouvre le schéma dans la section Tactique ; « retour » revient à l'exercice). Sans terrain : bouton **« Créer le terrain »** (schéma vide, au titre de l'exercice, lié à l'exercice). Les 27 exemples U10-U11 y sont donc exploitables.
- Backend : `exerciseBoard(exerciseId)` et `createExerciseBoard(exerciseId)` (un seul schéma de référence par exercice). Tests : C++ (`tst_backend`) et `t_exboard.py` (backend **simulé**). Non vérifié : rendu et navigation sur appareil, et que les modifications faites dans l'éditeur sont retrouvées au retour sur la fiche.
- Limite connue : les schémas de référence des exercices figurent aussi dans la bibliothèque de la page Tactique.

## Non publié — exemples d'exercices U10-U11 avec placement sur le terrain (schéma 8)
- Migration `008_seed_u10_u11_exercises.sql` (schéma 7 → 8, **INSERT seulement** : aucune donnée existante modifiée) : **27 exercices** préfixés « U10-U11 · », chacun avec **son schéma de terrain de référence** (demi-terrain, joueurs, adversaires, plots, ballons, flèches).
- **24 viennent du GIFE U10/U11 de la FFF** (jeux et situations de référence, thèmes 1 à 24, pages 110 à 133 du PDF) : objectif, espace, effectif, durée, consignes, buts, variantes et points de vigilance **reformulés** ; la page source est indiquée dans chaque description. **3 viennent de la note de l'éducateur du 10/11** (déménageurs avec passe, attaque contre défense 3 contre 2, match à 8 avec contrainte), en formulation de travail : détails manquants à confirmer.
- **Placement** : seuls **9 schémas du GIFE sont figés** dans le PDF (thèmes 4 à 8 et 18 à 21) : placement repris **à l'œil** de l'image, sans les mouvements animés. Les **15 autres** (thèmes 1 à 3, 9 à 17, 22 à 24) sont des schémas **reconstitués d'après le texte** (le schéma du PDF est une animation non lisible) ; chaque description le dit. Effectifs réduits : schémas illustratifs.
- Modifiables, archivables et supprimables comme les exercices de départ. Les bases existantes les reçoivent à la mise à jour.
- **Conséquence du numéro de schéma** : un export `.footcoach` fait après cette mise à jour porte `schema = 8` et sera refusé par une application plus ancienne (message « plus récente »). Les fichiers des schémas 1 à 7 restent importables.
- Tests : C++ (27 exemples présents, chacun avec une fiche de référence lisible et dans les bornes, une seule fiche par exercice, migration depuis un schéma 4 avec données conservées). Les anciens tests partent d'une base sans exemples. Non vérifié : affichage dans l'application sur appareil, conformité du placement avec les animations du GIFE.

## Non publié — la feuille de challenge s'ouvre d'abord, l'utilisateur l'enregistre ensuite
- « Créer la feuille » ne demande plus de dossier : le PDF est créé dans le **cache de l'application** (`challengeSheetTempUrl`, nom sans chemin) puis **ouvert aussitôt** (`Qt.openUrlExternally`) dans l'application de l'appareil (lecteur PDF) ; l'utilisateur y imprime ou l'enregistre. Message : « Feuille ouverte. Enregistrez-la ou imprimez-la depuis l'application qui s'ouvre. »
- **Repli** : si aucune application n'a pu l'ouvrir, le sélecteur « Enregistrer sous » s'ouvre avec le même nom (« Feuille enregistrée. »).
- Test : `t_challenge.py` (simulé, sans lancer d'application réelle). **Non vérifié sur Android** : si l'ouverture d'un fichier du cache (`file://`) par une application externe est acceptée (Android peut la refuser sans FileProvider ; le code de Qt n'a pas pu être relu dans cette session). Si elle échoue, le repli s'affiche ; à défaut il faudra passer par un FileProvider.

## Non publié — feuille de challenge 2/2 (PDF noir et blanc) depuis un match
- Fiche d'un match : bloc **« Feuille de challenge »** avec le bouton « Créer la feuille (PDF) ». Possible **seulement depuis un match enregistré** (bouton désactivé tant qu'il y a des modifications non enregistrées : le PDF reflète la base).
- Contenu : page **A4 paysage** reprenant la feuille 2/2 du district (4 blocs « Équipe 1 à 4 »). **Seul le bloc Équipe 1 est rempli** : nom de l'équipe et **joueurs convoqués du match** (titulaires et remplaçants), triés par nom puis prénom : n°, nom, prénom, n° de licence. Colonne « Blessure », responsable et n° de licence du responsable : vides, à compléter à la main. Pas de logo ni de motif décoratif.
- **Noir et blanc** : traits noirs fins sur fond blanc, aucun aplat, aucune couleur ni nuance de gris (contrôlé : écart RGB nul sur le rendu). Les noms longs réduisent la police (jusqu'à 6,5 pt) avant d'être tronqués.
- **Règles de refus** (rien n'est écrit) : aucun convoqué ; plus de 12 convoqués (la feuille a 12 lignes ; le message dit combien en retirer) ; un convoqué sans **n° de licence de 10 chiffres** (manquant ou invalide) — la fenêtre liste chaque joueur et la raison. Hypothèse : une licence = exactement 10 chiffres (`ChallengeSheet::kLicenceDigits`), comme les numéros vus sur la FFF.
- Enregistrement via le sélecteur de fichiers (nom proposé `feuille-challenge-<équipe>-<date>.pdf`), puis impression depuis le lecteur PDF de l'appareil. Pas d'impression directe (Qt n'en fournit pas sous Android).
- Tests : C++ (`tst_matches` : validité de la licence, refus, ordre alphabétique, PDF d'une page, quatre blocs ; l'ordre et le texte sont relus avec `pdftotext` quand il est installé) et `t_challenge.py` (interface, backend **simulé**). Non vérifié : écriture réelle sur Android (`content://`), nom proposé par le sélecteur, rendu sur une vraie imprimante noir et blanc.

## Non publié — photo dans la liste des joueurs
- Dans l'Effectif (liste téléphone et grille tablette), la **photo du joueur remplace le numéro dans la pastille ronde** ; le numéro reste lisible dans une petite pastille en bas à droite. Sans photo : pastille avec le numéro, comme avant. Composant `PlayerAvatar.qml`.
- La photo est lue à l'affichage de chaque carte visible (`playerPhoto`), pas stockée en mémoire pour toute l'équipe. Convocations et autres écrans inchangés.
- Test : `t_photo.py` (backend **simulé**, grille et liste). Non vérifié sur appareil : fluidité du défilement avec beaucoup de photos.

## Non publié — import de photos en lot (Effectif)
- Bouton **photo** dans l'en-tête de l'Effectif : on choisit plusieurs images ; chaque fichier **`<n° de licence>.jpg`** (ou .png, .jpeg, .webp, .bmp ; casse ignorée) est rattaché au joueur **de l'équipe affichée** qui porte cette licence. Chaque image est réduite comme une photo de la fiche (480 px, JPEG).
- **Photo déjà présente : remplacée.** Chaque fichier est indépendant (pas de « tout ou rien »). Compte-rendu : photos ajoutées, remplacées, fichiers ignorés avec la raison (licence inconnue dans l'équipe, image illisible, deux fichiers pour la même licence : le premier est utilisé). Maximum 400 fichiers par import.
- Prérequis : le joueur doit **déjà avoir son n° de licence** (fiche joueur). L'import de liste de joueurs ne le prend pas encore.
- Tests : C++ `importPhotosByLicence` et `t_photo.py` (backend **simulé**). Non vérifié sur appareil : sur Android, le nom affiché d'un fichier choisi (`content://`) doit être lu correctement par `QFileInfo` ; sinon les fichiers sont signalés « licence inconnue » sans rien modifier.

## Non publié — photo et numéro de licence d'un joueur (schéma 7)
- La **fiche joueur** propose une **photo** (bouton « Ajouter / Changer la photo », « Retirer la photo ») : choix d'une image dans les fichiers (aucune permission, pas de prise de vue). L'image est **réduite à 480 px en JPEG** (orientation EXIF respectée, fond blanc si transparente) et **stockée dans la base**. Sans photo, les initiales s'affichent. La photo n'apparaît que dans la fiche (pas dans les listes, cartes ni convocations).
- **N° de licence** facultatif : lettres et chiffres, 20 au plus, enregistré en majuscules, **unique dans l'équipe** (casse ignorée, joueurs inactifs compris) ; une autre équipe peut avoir la même. Message clair en cas de doublon ; garde-fou en base (index unique, valable aussi à l'import).
- Migration `007_player_photo_licence.sql` (schéma 6 → 7, données conservées). Export/import `.footcoach` : colonnes `licence` et `photo` (octets en base64) ; les fichiers des schémas 1 à 6 restent importables ; un fichier du schéma 7 est refusé par une application plus ancienne (message « plus récente »). Nouveau fichier de référence `tests/fixtures/schema7.footcoach`.
- Confidentialité : texte in-app et `docs/confidentialite.md` mis à jour (photo enregistrée réduite, incluse dans les exports, jamais envoyée ; aucune permission ajoutée).
- Tests : C++ (licence, photo, mauvaise entrée, aller-retour du schéma 7, doublon de licence refusé à l'import) et `t_photo.py` (backend **simulé**). Non vérifié sur appareil : sélecteur de fichiers Android (`content://`) et lecture réelle de l'image.
- Limites : la liste d'import de joueurs ne prend pas encore de licence ; le plafond d'une photo est 400 Ko après réduction.

## Non publié — formation d'un match placée selon les postes
- Choisir une formation sur le terrain d'un match place les **titulaires** selon leur **poste** (gardien, défenseur, milieu, attaquant) : lignes d'après le nom de la formation (4-2-3-1 = 4 défenseurs, 5 milieux, 1 attaquant). **Les remplaçants ne sont jamais placés.**
- **Plusieurs titulaires au même poste** : choix d'après le **pied fort** (droitier à droite, gaucher à gauche, « les deux » partout mais après le pied adapté, pied inconnu ensuite ; l'axe compte comme un côté à partir de 0,44/0,56 de largeur), puis à égalité par **prénom alphabétique** (accents et casse ignorés), puis nom. Couloirs remplis avant l'axe.
- Titulaire sans poste ou en surnombre : il prend une case restante hors du but ; jamais un gardien dans le champ ni un joueur de champ au but (case vide). Message si des titulaires restent sans place.
- `SquadFormation.js`. Test : `t_squadboard.py` (backend **simulé**). Non vérifié sur appareil.

## Non publié — barre d'actions du terrain
- La barre d'actions de la sélection (modifier, couleur, supprimer, pivoter…) **disparaît dès qu'on choisit un outil de dessin** (joueur, adversaire, ballon, flèche, note…) ; la sélection est levée. Elle reste avec « Sélection » et « Multi ».
- Elle est **déplaçable** par une poignée (trois paires de points, à gauche) et ne sort jamais de la vue ; elle revient en bas au centre à la réouverture du terrain.
- Test : `t_ctxbar.py` (glissé simulé par souris ; le geste tactile réel n'est pas vérifié sur appareil).

## Non publié — terrain d'un match : joueurs de la convocation
- Sur le terrain tactique d'un match, **« Joueur » ouvre la liste des convoqués** (titulaires d'abord, avec leur rôle) au lieu d'ajouter un rond anonyme ; le joueur choisi est posé avec son nom sous le rond (24 caractères : prénom + initiale du nom si trop long). Un même joueur peut être posé plusieurs fois. Aucun convoqué : message, rien n'est posé.
- **Formation** sur un match : les postes se remplissent avec les titulaires convoqués, dans l'ordre de la liste ; les postes sans titulaire restent vides.
- La liste suit l'état courant de la fiche (convocations modifiées, même non enregistrées). Les schémas libres et les blocs de séance sont inchangés. Le texte posé est un libellé : renommer un joueur plus tard ne met pas le schéma à jour.
- **Plus de texte libre pour un joueur de match** : modifier un rond (double-toucher ou crayon) ouvre la liste « Remplacer par » (convoqués) au lieu d'une saisie. Les adversaires et les notes restent libres. Limites : un schéma créé avant ce changement garde ses ronds numérotés ; retirer un joueur des convocations ne le retire pas du schéma.
- Test : `t_squadboard.py` (backend **simulé**). Non vérifié sur appareil.

## Non publié — texte sous le joueur (schéma tactique)
- Le texte d'un joueur ou d'un adversaire s'affiche **sous le rond** (le rond reste vide) et peut compter **24 caractères** (3 auparavant) ; il passe sur deux lignes si besoin. Les notes texte (300 caractères) sont inchangées. Les anciens schémas (numéros) s'affichent sous le rond, sans migration : le texte est déjà enregistré dans le schéma.
- Test : `t_caption.py` (backend **simulé**). Non vérifié sur appareil.

## Non publié — import de joueurs dans « Effectif »
- **Import d'une liste collée** (bouton dans l'en-tête de l'Effectif et dans l'état vide) : un joueur par ligne, **prénom seul obligatoire** ; le reste est facultatif, séparé par « ; » ou une tabulation (collage depuis un tableur) : Prénom ; Nom ; Numéro ; Poste ; Naissance (JJ/MM/AAAA). Ligne d'en-tête « Prénom ; Nom … » ignorée, 300 joueurs au plus.
- **Unicité** : prénom + nom unique dans l'équipe, sans tenir compte des accents ni des majuscules, joueurs inactifs compris ; numéro unique parmi les joueurs actifs. Règle appliquée aussi à la fiche joueur. Hypothèse retenue (à confirmer) : deux joueurs de même prénom sont permis si leurs noms diffèrent.
- **Tout ou rien** : à la moindre ligne refusée, rien n'est créé ; les lignes à corriger sont listées avec leur numéro (8 au plus, puis un résumé).
- **Le nom devient facultatif** dans la fiche joueur et en base (chaîne vide ; aucune migration : la colonne l'acceptait déjà).
- `Backend::importPlayers`, `PlayerRepository::nameTaken`. Tests : `tst_backend::playerNameOptionalAndUnique`, `importPlayersList` ; `t_import.py` (backend **simulé**) ; `t_fields.py` couvre le champ de saisie au-dessus du clavier. **Non vérifié sur appareil.**

## Non publié — signalements sans navigateur, confidentialité dans l'application
- **Confidentialité** : la politique s'affiche dans l'application (`PrivacyPage`, texte généré depuis `docs/confidentialite.md` par `tools/gen_privacy_text.py`, contrôlé par `t_privacy.py`) ; plus aucune redirection vers GitHub.
- **Signalements** : « Envoyer le signalement » crée le ticket « SUP - … » directement par l'API GitHub (`IssueReporter`, QtNetwork), sans navigateur ni compte GitHub. Jeton « Issues : écriture » seul, injecté à la compilation (`FOOTCOACH_BUG_TOKEN`), jamais dans le dépôt ; sans jeton, seul l'e-mail est proposé. États : envoi en cours (bouton bloqué), confirmation, échec avec saisie conservée.
- **Android** : permission `INTERNET` ajoutée au manifeste (seule permission). `docs/confidentialite.md` et `docs/publication.md` mis à jour (données déclarées, Play Console).
- Tests : `tst_issuereporter` (faux serveur HTTP local : succès 201, en-têtes et corps, erreurs 401/422/500 sans fuite du jeton, serveur injoignable, double envoi, sans jeton) ; `t_support.py` adapté (reporter **simulé**). **Non vérifié** : envoi réel vers GitHub, Qt 6.9, Android.
- **Android, HTTPS** : constaté sur appareil (logcat) « No functional TLS backend was found » : Qt n'embarque pas OpenSSL. Ajout de `libssl_3.so` / `libcrypto_3.so` (arm64-v8a, KDAB android_openssl, Apache 2.0, segments alignés 16 Ko) dans `android/openssl/`, déclarées via `QT_ANDROID_EXTRA_LIBS`. Le message d'échec indique maintenant le code d'erreur et l'absence de TLS. **Non vérifié** : compilation Android et envoi réel après correction.
- Risque accepté : le jeton est extractible de l'APK ; portée limitée aux tickets, révocable.

## 0.10.0 — 2026-11-03 (branche v1, non publiée) — schéma 5 : un terrain par bloc de séance
**Étape 2 : le terrain dans le plan de séance**
- Chaque bloc du plan affiche la miniature de son terrain, ou un état vide « Pas encore de terrain pour cet atelier » avec le bouton **Ajouter un terrain**. « Ouvrir le terrain » ouvre l'éditeur tactique existant par-dessus le plan ; le retour (bouton de l'en-tête ou retour d'Android) enregistre, remet à jour la miniature et **conserve la position de défilement**. Le terrain n'est créé que sur demande (terrain vide, au nom de l'exercice).
- Dans ce contexte, l'éditeur n'offre plus « Renommer » ni « Dupliquer » ; « Supprimer le schéma » devient « Retirer le terrain de ce bloc ». Séance annulée : miniature visible, ni ajout ni ouverture. Terrain au contenu illisible : ouvert en lecture seule, jamais écrasé, retirable.
- Nouvelle fonction `Backend::createBlockBoard(itemId)` (idempotente) : seule évolution du backend, nécessaire parce qu'aucune fonction ne créait un terrain lié à un bloc. `PlanRepository::sessionOf`. Aucun changement de la structure de base.
- `BoardThumb.qml` : traits et pastilles proportionnels à la largeur (inchangé à 72 px, lisible en grand).
- Tests : `tst_backend::blockBoardThroughTheBackend`, `importReferenceFiles` étendu (fichiers 1 à 5), `tests/qml/t_blockboard.py` (Backend **simulé**, 5 formats d'écran). **Non vérifié sur téléphone ou tablette.**

**Étape 1 : socle (schéma 5)**
- **Schéma de données 5** (`sql/005_block_board.sql`) : `tactical_board.session_exercise_id` (clé étrangère vers `session_exercise`, `ON DELETE CASCADE`) et son index. Colonne facultative : les données existantes ne changent pas. Le format d'échange `.footcoach` reste en version 1 ; fichier de référence `schema5.footcoach` ajouté (les 1 à 4 sont inchangés).
- Cœur C++ : `BoardRecord.sessionExerciseId`, `BoardRepository::get / forBlock / forBlocksOfSession / referenceForExercise`, `PlanEntry.boardId`, `Backend::plan()` renvoie `boardId`, `boardReadable`, `boardJson` pour chaque bloc.
- Règles appliquées dans le dépôt : un terrain de bloc n'est ni un schéma de référence ni lié à une séance ; un seul terrain par bloc ; un seul schéma de référence par exercice (contrôlés à l'ajout ou au changement de lien seulement, pour ne pas bloquer d'éventuels doublons existants).
- `BoardRepository::forSession` inclut les terrains des blocs, sans doublon. `Backend::boards()` (bibliothèque) n'affiche pas les terrains de bloc. Dupliquer un schéma produit un schéma libre.
- `BoardRepository::update` renvoie désormais une erreur si le schéma n'existe pas (avant : succès silencieux).
- **Non vérifié** : l'application complète sur appareil ; la cascade a été testée par les tests C++ sur SQLite 3.45.1 de l'environnement de développement, pas sur le SQLite d'Android. Les écrans Widgets (`src/ui`) n'ont pas été adaptés.

## 0.9.8 — 2026-10-31 (branche v1)
- **Retour en arrière sur le clavier** : les essais des versions 0.9.5 (mode « Fixe » `adjustNothing` avec marge gérée par l'application, interrupteur Données > Clavier) et 0.9.7 (redessin de la vue Android) sont annulés par `git revert` : ils n'ont pas supprimé le défaut du premier appui dans un champ (constaté sur appareil, journaux adb de la 0.9.5 et de la 0.9.7). Le manifeste redevient `adjustResize`, comme en 0.9.4.
- **Contournement, non vérifié** : au lancement (Android seulement), l'application ouvre puis referme une fois le clavier sur un champ invisible, puisque le deuxième champ s'affiche correctement. Cause du défaut non trouvée : le journal montre que l'animation d'apparition du clavier ne se termine pas au premier affichage. Un bref clignotement du clavier au démarrage est possible. Désactivable dans Données > Clavier (essai) si ça gêne.
- Le numéro de version reste affiché sous le titre de la page Données.

## 0.9.6 — 2026-10-31 (branche v1)
- Le numéro de version est affiché sous le titre de la page Données (« version 0.9.6 »), pour savoir sans ambiguïté quelle version est installée sur l'appareil. Constaté : l'essai du clavier de la 0.9.5 a été fait sur une 0.9.4 non mise à jour ; les essais du premier appui dans un champ restent donc à refaire sur une version dont le numéro est affiché.

## 0.9.4 — 2026-10-31 (branche v1)
- **Champs de saisie et clavier** : un champ placé en bas d'un formulaire (Notes de la séance, de la récurrence et du joueur, Description de l'exercice, E-mail du joueur) restait caché derrière le clavier, car une liste défilante ne se déplace pas toute seule vers le champ touché. Nouveau composant `EnsureVisible` : le champ qui prend le focus est ramené à l'écran, y compris quand la fenêtre est réduite par le clavier. Appliqué aux fiches Exercice, Joueur, Séance et Récurrence.
- La barre du bas est masquée pendant la saisie via la propriété `keyboardUp` de la fenêtre (liée à `Qt.inputMethod.visible`).
- **Tests** : nouveau banc de tests QML hors écran dans le dépôt (`tests/qml/`, `python3 tests/qml/run_all.py`) : séance du jour (choix selon l'heure, chronomètre, reprise après relance), bouton retour (13 cas), et **chaque champ de saisie de l'application** avec clavier simulé (33 champs, dont fiches, fenêtres de dialogue, recherches ; le test échoue si on ajoute un champ sans l'ajouter au test). Tests de la logique JavaScript de la séance du jour ajoutés à `ctest` (`livesession_js`, avec Node.js). Avant le correctif, ces tests donnaient 5 échecs (champs du bas cachés) ; après, 0.
- **Non vérifié sur téléphone** : le clavier simulé est une fenêtre réduite ; le vrai clavier Android, ses animations et le comportement de Qt sur votre téléphone restent à confirmer.

## 0.9.3 — 2026-10-31 (branche v1)
- La barre de navigation du bas est masquée tant que le clavier est ouvert (téléphone en portrait), pour laisser sa place au formulaire ; elle revient à la fermeture du clavier. Constaté sur appareil en 0.9.2 : le réglage `adjustResize` fonctionne (l'en-tête reste en place), mais la barre restait au-dessus du clavier.
- Non vérifié sur téléphone (le banc d'essai ne simule pas le clavier) : le masquage et le retour de la barre, et l'absence de saut d'affichage à la fermeture du clavier.

## 0.9.2 — 2026-10-31 (branche v1)
- **Bouton retour d'Android** : il ferme d'abord ce qui est ouvert (fiche d'exercice ou de joueur, plan de séance, formulaire du calendrier, fiche dépliée du suivi, plein écran puis schéma du terrain), puis ramène à l'accueil ; l'application ne se ferme que depuis l'accueil. Une fenêtre ouverte (feuille de choix, dialogue, chronomètre en plein écran) se ferme d'abord, par le comportement de Qt.
- **Clavier** : le manifeste Android déclare maintenant `windowSoftInputMode="adjustResize"` : la fenêtre se réduit au lieu d'être poussée vers le haut quand le clavier s'ouvre dans un champ de notes (constaté sur appareil : l'en-tête passait sous la barre d'état).
- Manifeste : `android:enableOnBackInvokedCallback="false"` sur `<application>`, pour que le retour passe par la fenêtre de Qt (cet attribut a été signalé comme nécessaire sur Android 13 et plus dans un fil du forum Qt ; Android 16 pourrait l'ignorer pour une cible API 36).
- Vérifié hors appareil : l'enchaînement des retours sur les 7 pages (13 cas). **Non vérifié sur téléphone** : le bouton retour lui-même, le geste de retour par balayage, et surtout le clavier (le banc d'essai ne simule pas le clavier Android).

## 0.9.1 — 2026-10-31 (branche v1)
- Accueil, carte « Aucune séance aujourd'hui » : les boutons « Planifier une séance » et « Changer de séance » passent à la ligne quand l'écran est étroit ; sur téléphone, le second dépassait de l'écran (constaté sur appareil). Vérifié hors appareil sur 412, 360 et 320 de large.
- Non corrigé : défilement vers le haut de la fenêtre quand on touche un champ de notes sur Android (cause à confirmer sur appareil, voir les échanges).

## 0.9.0 — 2026-10-31 (branche v1)
- **Accueil : « Séance du jour »** avec chronomètre par exercice.
  - La séance affichée est choisie d'office selon l'heure du téléphone (celle dont la plage horaire contient l'heure actuelle) ; sinon la prochaine séance du jour. « Changer de séance » permet de choisir à la main (hier, aujourd'hui, demain) et de revenir à l'automatique. Si deux séances se chevauchent, un avertissement s'affiche.
  - Grand chronomètre en **décompte** (durée prévue de l'exercice), boutons Commencer / Pause / Reprendre et Exercice suivant / Terminer la séance. Au-delà de la durée prévue, il continue en orange (+mm:ss).
  - Déroulé de la séance : exercices à venir, en cours (mis en avant avec son chronomètre), terminés (durée réelle et durée prévue). Appuyer sur un exercice ouvre sa fiche dans Exercices pour le redéfinir.
  - Plein écran du chronomètre (lisible de loin), et deux colonnes en paysage et sur tablette.
  - Le temps se recalcule toujours à partir de l'heure de départ : un écran verrouillé ou une application en arrière-plan ne fausse pas le chronomètre. L'état est conservé si l'application est relancée le même jour (réglages de l'application, aucun changement de schéma).
  - L'écran reste allumé tant que le chronomètre tourne (Android : drapeau de fenêtre, sans permission).
- Schéma de données : inchangé (4). Format `.footcoach` : inchangé (1). Identifiant : inchangé.
- Vérifié hors appareil : choix de la séance selon l'heure, décompte, pause, reprise, dépassement, fin de séance, reprise après relance, ouverture de l'exercice, 7 formats d'écran. **Non vérifié sur téléphone** : écran maintenu allumé (appel Android via JNI), comportement écran verrouillé, lisibilité en plein soleil. Il n'y a pas d'alerte sonore ni de notification quand l'application est en arrière-plan.

## 0.8.12 — 2026-10-31
- Android : `minSdk` passé de 23 à 28 (Android 9). Qt 6.9 ne prend en charge qu'Android 9 et au-delà ; l'application n'est plus proposée sur Android 6 à 8, où elle n'était pas garantie. Aucun changement de code, de schéma (4) ni de format de fichier (1).
- À vérifier au prochain build : `aapt2 dump badging app.apk` doit afficher `sdkVersion:'28'`.

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
