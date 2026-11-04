# FootCoach — SDD
## Software Design Description

**Version du document :** 1.0  
**Date :** 4 novembre 2026  
**Statut :** conception cible et état technique connu, à confronter au dépôt actuel.

---

## 1. Objet

Décrire l'architecture technique de référence et les précautions de reprise. Ce document ne remplace pas l'inspection du code. Les éléments explicitement signalés « à vérifier » ne doivent pas être traités comme des faits établis.

## 2. Architecture cible

### 2.1 Technologies et plateformes

Direction retenue :
- Qt 6.9 ;
- C++ pour domaine, services applicatifs et accès aux données ;
- QML pour l'interface ;
- SQLite pour le stockage local ;
- Desktop Windows et Android ;
- base de code partagée, avec adaptation des interfaces selon la plateforme.

### 2.2 Découpage logique recommandé

```text
FootCoach
├── UI (QML)
│   ├── Navigation / Shell
│   ├── Accueil
│   ├── Clubs / Équipes
│   ├── Séances
│   ├── Matchs
│   ├── Effectif / Joueurs
│   ├── Calendrier / Suivi
│   ├── Bibliothèque d'exercices
│   └── Composants tactiques
│
├── Application (C++)
│   ├── Services de cas d'utilisation
│   ├── Gestion du contexte club/équipe
│   ├── Navigation / état de session
│   ├── Services séance et match
│   ├── Minuteries
│   ├── Sauvegarde / restauration
│   └── Assistance / diagnostics
│
├── Domain (C++)
│   ├── Club / Équipe / Joueur
│   ├── Séance / Exercice
│   ├── Match
│   ├── Terrain tactique
│   └── Présence / Événement
│
├── Persistence
│   ├── Repositories
│   ├── SQLite
│   └── Migrations
│
└── Platform
    ├── Desktop
    └── Android
```

Ce découpage est une cible conceptuelle ; conserver les conventions et la structure réelles du dépôt tant qu'une refonte n'est pas décidée.

## 3. Règles d'architecture métier

### 3.1 Séparer les agrégats métier

- Club regroupe les équipes.
- Équipe est le contexte courant de travail.
- Séance regroupe des blocs/exercices.
- Match est un agrégat distinct de Séance.
- Exercice de bibliothèque et exercice inséré dans une séance doivent avoir une sémantique explicite (copie ou référence : décision ouverte).
- Le moteur tactique est partagé, mais le rattachement du tableau au bloc d'une séance et au match doit rester explicite.

### 3.2 Terrain tactique partagé

Le moteur de rendu/édition peut être commun. Le contexte métier ne doit pas l'être implicitement :
- tableau d'atelier : lié à un bloc `session_exercise` ;
- tableau de référence d'exercice : bibliothèque, si le modèle est retenu ;
- tableau de match : lié au match, modèle à spécifier ;
- tableau libre : uniquement si le produit le conserve comme fonctionnalité.

Ne jamais mélanger les tableaux de bibliothèque et ceux des blocs de séance dans une liste générique sans distinguer leur type/contexte.

### 3.3 Navigation

La navigation doit utiliser une pile ou un mécanisme équivalent, pas des destinations de retour codées en dur. Une entrée de navigation doit pouvoir conserver :
- écran précédent ;
- identifiants de club/équipe/séance/exercice/match ;
- position de défilement ;
- état d'édition et données non encore persistées ;
- état des minuteries ;
- destination de retour et motif de fermeture.

Le retour Android et le retour UI doivent passer par la même logique centrale. Les changements de contexte (club/équipe) doivent être traités explicitement pour éviter de restaurer un écran avec des identifiants incompatibles.

## 4. État technique connu au 4 novembre 2026

### 4.1 Git

État rapporté et précédemment confirmé par les commandes de l'utilisateur :
- dépôt GitHub : `geekons77899377/footcoach` ;
- branche : `v1` ;
- commit courant : `9944ed19fbbe154a43139fa73b22232707564fec` (`9944ed1`) ;
- message : `0.10.0 : schéma 5, un terrain tactique par bloc de séance (étapes 1 et 2)` ;
- parent : `7ac3063` ;
- commit poussé sur `origin/v1` selon confirmation utilisateur ;
- `main` non modifiée ;
- aucun tag créé/poussé ;
- tag `v0.10.0-sch5` envisagé mais en attente d'autorisation explicite.

Règles de livraison :
- commit = état sauvegardé/versionné, pas preuve de qualité ;
- tag = version formellement identifiée ;
- validation = conformité démontrée ;
- push = publication distante.
Ces opérations sont indépendantes. Aucun tag ou push sans autorisation explicite de l'utilisateur.

### 4.2 Environnement local Windows/WSL

Chemin WSL rapporté : `/mnt/d/Work/builds/footcoach`  
Chemin Windows : `D:\Work\builds\footcoach`  
Branche locale rapportée : `v1`.

Un stash local a été créé avant le pull :
`stash@{0}` — `sauvegarde adaptations locales build Windows`.

Ne pas faire `stash pop`, `stash apply` ou `stash drop` sans revue préalable. Contenu rapporté :
- CMake : `QT_DISCARD_FILE_CONTENTS TRUE` appliqué aux chemins QML, et reformatage `qt_add_qml_module` ;
- AndroidManifest : reformatage XML uniquement ;
- `main.cpp` : `qputenv("QML_DISK_CACHE", "0");` ;
- fichier non suivi `create_footcoach_issues.py`.

Le pull `--ff-only origin v1` a été confirmé et le HEAD exact était `9944ed19...`. Aucun build Windows/Android réel n'a été confirmé dans ce contexte.

## 5. Évolution technique connue : schéma 5 / tableaux par bloc

### 5.1 Migration

Fichier rapporté : `sql/005_block_board.sql`.

Évolution décrite :
- ajout de `tactical_board.session_exercise_id` comme clé étrangère vers `session_exercise` ;
- `ON DELETE CASCADE` ;
- index sur le lien.

**À vérifier dans le dépôt :** présence exacte de la migration, contraintes, index, gestion des données existantes et comportement d'import/export.

### 5.2 Modèle et repositories rapportés

- `BoardRecord.sessionExerciseId`.
- `BoardRepository` : méthodes `get`, `forBlock`, `forBlocksOfSession`, `referenceForExercise`, options de liste.
- `PlanEntry.boardId`.
- `Backend::plan()` expose `boardId`, `boardReadable`, `boardJson`.
- `Backend::boards()` exclut les tableaux de blocs de la bibliothèque.
- `duplicateBoard` crée un tableau libre en supprimant les liens exercice/bloc.
- `update()` signale une erreur si le tableau n'existe plus.
- `checkLinks()` valide côté repository l'unicité d'un tableau par bloc et d'un tableau de référence par exercice.

**Ces détails proviennent des diffs et comptes rendus précédemment examinés, pas d'un audit direct du dépôt complet dans cette session.**

### 5.3 Risques d'intégrité à auditer

1. **Unicité** : l'unicité décrite dans `checkLinks()` serait applicative ; aucune contrainte SQL `UNIQUE` n'a été confirmée. Vérifier les chemins d'écriture alternatifs.
2. **Import** : déterminer si l'import de données peut écrire directement en SQL sans passer par `checkLinks()`. Ne pas conclure sans lire le code.
3. **Doublons** : une sous-requête de `PlanRepository::entries()` aurait sélectionné le premier tableau par ID (`ORDER BY id LIMIT 1`). Vérifier si des doublons peuvent être créés et s'ils sont masqués.
4. **Suppression en échec** : dans un diff QML examiné, l'éditeur semblait se fermer aussi après échec de suppression. Vérifier le code actuel et conserver l'accès à une nouvelle tentative en cas d'échec.
5. **Flush avant fermeture** : vérifier le retour de `BoardEditor.flush()` et empêcher la fermeture/navigation si la sauvegarde échoue.
6. **Loader QML** : vérifier la syntaxe et le chargement réel des composants dynamiques sur la version Qt cible.
7. **Import/export** : tester un round-trip avec un tableau valide et les relations de bloc, pas seulement un JSON malformé.
8. **Suppression de bloc** : confirmer le cascade delete et les cas d'erreur.

### 5.4 Étape 2 : comportement QML rapporté

- `PlanEditor.qml` affiche les blocs avec titre, durée et aperçu tactique.
- Réutilisation de `BoardThumb.qml`.
- `BoardEditor.qml` s'ouvre au-dessus du plan ; autosave/retour et conservation du scroll sont visés.
- Création du tableau à la demande par `createBlockBoard(itemId)`, idempotente selon l'implémentation rapportée.
- Mode bloc : masquer Renommer/Dupliquer ; action de suppression présentée comme retrait du terrain du bloc.
- Tableau illisible : lecture seule, pas d'écrasement automatique, possibilité de retrait.
- `SessionsPage.qml` gère le retour avec priorité fullscreen, tableau, plan, accueil selon le compte rendu.
- Les changements de données et de contexte déclenchent des chemins de fermeture/rechargement à auditer.

### 5.5 Niveau de tests rapporté

Claude avait annoncé :
- `ctest` : 8/8 ;
- harness QML : 82 vérifications, 5 tailles d'écran ;
- tests C++ de création idempotente, bloc inexistant, séance annulée, persistance et cascade.

Limites :
- le harness QML utilise un Backend Python simulé ;
- ces tests ne prouvent pas l'intégration réelle QML ↔ C++ ↔ SQLite ;
- pas de validation sur appareil Android réel ni build Desktop/Android complet confirmés ici ;
- les résultats n'ont pas été relancés indépendamment dans cette session.

Conclusion : étapes 1 et 2 sont commitées/poussées selon l'état rapporté, mais leur validation complète reste ouverte.

## 6. Priorités techniques de reprise

### P0 — avant toute nouvelle fonctionnalité
1. Lire l'état réel du dépôt et vérifier branche/HEAD/status.
2. Auditer les étapes 1–2, en priorité unicité, import, suppression en échec et flush/fermeture.
3. Compiler réellement avec Qt 6.9 sur Desktop.
4. Exécuter CTest et les tests QML.
5. Vérifier l'intégration avec le vrai Backend C++/SQLite.
6. Vérifier le parcours sur Android, au minimum build + installation + persistance + navigation.
7. Ne pas démarrer l'étape 3 tant que les étapes 1–2 ne sont pas validées.

### P1 — stabilisation UX et métier
- navigation contextuelle avec pile ;
- suppression générique dans le tableau tactique ;
- copier/coller multi-objets ;
- aperçu tactique réduit et lisible dans le plan ;
- fermeture/masquage d'une carte de séance sur l'accueil sans suppression ;
- minuteries indépendantes ;
- bibliothèque et sémantique copie/référence ;
- cohérence du contexte club/équipe.

### P2 — extensions
- matchs complets ;
- sauvegarde/restauration ;
- assistance et tickets ;
- équipe/joueurs, calendrier et suivi avancés.

L'ordre final doit être ajusté en fonction des dépendances et de l'état réel du code.

## 7. Roadmap fonctionnelle envisagée

1. Stabiliser fondation et navigation.
2. Fiabiliser persistance locale, migrations, sauvegarde/restauration et chemins de fichiers.
3. Finaliser les séances et la bibliothèque d'exercices.
4. Développer le domaine Matchs indépendamment.
5. Développer équipes/joueurs.
6. Développer calendrier et suivi.
7. Développer analyse/performance avancées.

Cette roadmap est une proposition de séquencement, pas une autorisation de commencer automatiquement chaque phase.

## 8. Règles d'implémentation pour Claude AI

- Inspecter le dépôt avant toute modification ; ne pas se fier uniquement à ce document.
- Lire les instructions du dépôt (`AGENTS.md`, `CLAUDE.md`, README, conventions) si présentes.
- Faire un état initial : branche, HEAD, `git status`, stash existant.
- Ne jamais écraser, supprimer ou appliquer le stash `stash@{0}` sans accord.
- Ne pas faire de refonte opportuniste ni corriger des éléments hors périmètre.
- Avant chaque modification : annoncer fichiers concernés, cause et critères d'acceptation.
- Préférer des changements petits, cohérents et testables.
- Tests réels d'abord ; distinguer tests simulés et intégration réelle.
- Signaler les hypothèses, risques et résultats non reproductibles.
- Ne jamais déclarer une fonctionnalité « validée » sur la seule base d'un commit ou d'un test simulé.
- Commit local possible comme checkpoint si demandé/autorisé dans le cadre de travail ; pas de tag ni push sans autorisation explicite.
- Ne pas commencer une étape dépendante avant validation de ses prérequis.
- Ne pas modifier le schéma DB sans migration, stratégie de compatibilité et tests de migration.
- Préserver les données utilisateur ; aucune suppression destructive silencieuse.

## 9. Première mission recommandée à Claude

**Audit ciblé, sans nouvelle fonctionnalité : valider les étapes 1 et 2 du tableau tactique par bloc.**

Livrables attendus :
1. état Git réel et inventaire des modifications ;
2. lecture des fichiers concernés ;
3. vérification de l'unicité du tableau par bloc sur tous les chemins d'écriture, y compris import ;
4. vérification des erreurs de suppression et de flush avant fermeture ;
5. validation du chargement QML dynamique ;
6. build Qt 6.9 Desktop ;
7. CTest + tests QML ;
8. intégration réelle Backend C++/SQLite ;
9. build et test Android si environnement disponible ;
10. tableau des résultats : PASS / FAIL / BLOCKED / NOT TESTED, avec preuves et commandes ;
11. corrections minimales strictement liées aux défauts démontrés.

Ne pas implémenter l'étape 3, ne pas élargir le périmètre et ne pas taguer/pousser.

## 10. Prompt court à donner à Claude

> Lis intégralement `SSS_FootCoach.md`, `SRS_FootCoach.md` et `SDD_FootCoach.md`. Ils constituent le dossier de reprise, mais inspecte le dépôt pour confirmer tout état technique. Commence par un état Git non destructif (branche, HEAD, status, stash). Ne touche pas au stash `stash@{0}`. Ta première mission est exclusivement l'audit et la validation des étapes 1–2 du tableau tactique par bloc : intégrité/unicité y compris import, erreurs de suppression, flush avant fermeture, chargement QML, build Qt 6.9, CTest, tests QML et intégration réelle C++/SQLite, Android si possible. Fais les corrections minimales uniquement si un défaut est démontré. N'implémente pas l'étape 3 et ne traite aucun backlog non lié. Ne fais aucun tag ni push. Fournis un rapport PASS/FAIL/BLOCKED/NOT TESTED avec preuves, fichiers modifiés, commandes et risques. Distingue systématiquement confirmé, supposé, vérifié et validé. Si une décision métier manque, pose une seule question ciblée et attends.
