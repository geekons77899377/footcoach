# Feuille de route FootCoach — v3

Commit de référence : `624997e52bce151851d37eb4c746a13cb5cfb6a1` (branche v1). Date : 2026-11-05. Document de planification ; aucun développement engagé.

## Convention de preuve

- Fait vérifié : contrôle exécuté ou reproductible, avec le commit indiqué (suite QML 377 sur 624997e, git diff, SHA-256 du PDF).
- Constat issu du dépôt : lu dans un fichier du dépôt, non exécuté (Main.qml height 72, Theme.qml ligne 41, DESIGN.md).
- Hypothèse ou non vérifié : non prouvé ici (build Qt 6.9, Android, backend C++ réel, ctest sur 624997e, rapprochement PDF/PNG).
- Décision en attente : NON DÉCIDÉE, PROPOSITION ou PARTIELLEMENT TRANCHÉE. Seule la mention TRANCHÉE est une décision validée.

## Légende

- **Prérequis technique** : conditions d'exécution et de validation.
- **Correction indispensable** : défaut vérifié touchant l'intégrité ou la navigation.
- **Développement fonctionnel** : exigence à construire.
- **Validation de livraison** : preuve exigée avant publication.
- **Dépend strictement de** : bloque le démarrage. **Dépend recommandé de** : bloque la validation seulement.

> **L16** : périmètre à définir (SDD §7 point 7). Aucune exigence n'est rattachée ; aucun critère n'est inventé.

## Ordre recommandé

1. **PT-0** (hygiène) et **D16** : immédiat, sans risque.
2. **L1** (environnement Qt 6.9) en parallèle du **développement** de L0 et de L3.
3. **L0** : validation après L1 ; **L3** : après D3, en parallèle de L0.
4. **L2** : après L1 ; sert de preuve pour L3 à L8.
5. **L4** : après D1, L0 et L1 (migration).
6. **L5**, **L6**, **L7**, **L8**, **L9**, **L11**, **L12**, **L13** : selon leurs décisions et leurs dépendances (voir tableau).
7. **L10** : après L0, L4 et D5a/D5b.
8. **L14** puis **L15**, puis **PUB** après D13.

## Prérequis technique

### PT-0 — Hygiène du dépôt : statut de t_live.py et stabilité du test sensible au temps

- **Périmètre (exigences)** : AUD-08, AUD-11, AUD-12 ; SDD 5.5
- **Décisions préalables** : D16
- **Dépend strictement de** : —
- **Dépend recommandé de** : —
- **Parallélisable avec** : Tous
- **Prérequis** : Décision D16
- **Critères d'acceptation** : Arbre de travail sans modification inattendue ; t_live.py vert sur 3 passages consécutifs
- **Tests requis** : run_all.py ; t_live.py ×3
- **Preuves attendues** : Journal de 3 passages
- **Bloque** : L9 (stabilité) ; mesures de non-régression
- **État actuel** : Modification non commitée de 6 lignes dans t_live.py ; décision en attente

### L1 — Build et tests réels : Qt 6.9 Windows, Android, ctest complet sur la version cible

- **Périmètre (exigences)** : SDD P0 3 et 6 ; NFR-004 ; AUD-05 (L1a Qt 6.9 disponible, L1b build Windows, L1c build Android, L1d ctest sur 6.9)
- **Décisions préalables** : —
- **Dépend strictement de** : Environnement Qt 6.9 et SDK Android (hors dépôt)
- **Dépend recommandé de** : —
- **Parallélisable avec** : L0 (développement), L3, L12, L13
- **Prérequis** : Qt 6.9 ; SDK Android ; machine Windows
- **Critères d'acceptation** : L1a–L1d réussis ; 8/8 ctest sur Qt 6.9 ; APK généré
- **Tests requis** : ctest ; build CMake Windows ; build Android
- **Preuves attendues** : Journal de build daté ; journal ctest
- **Bloque** : L0 (validation), L2, L10, L14, PUB
- **État actuel** : Non réalisé : Qt 6.4.2 seul disponible dans l'environnement d'audit

### L2 — Intégration réelle QML ↔ C++ ↔ SQLite : cinq parcours représentatifs

- **Périmètre (exigences)** : NFR-009 ; SDD P0 5 ; SSS §4 critères d'acceptation
- **Décisions préalables** : —
- **Dépend strictement de** : L1
- **Dépend recommandé de** : —
- **Parallélisable avec** : L0 (développement), L9, L12, L13
- **Prérequis** : Build exécutable (L1) ; base temporaire pilotable
- **Critères d'acceptation** : P1 création séance avec exercices, sauvegarde, relance ; P2 export puis import (round-trip) ; P3 édition du tableau tactique, fermeture, relance ; P4 saisie des présences et statistiques ; P5 retour avec contexte (après L3)
- **Tests requis** : Nouveau dossier tests/integration (à créer)
- **Preuves attendues** : Rapport des cinq parcours sur backend C++ réel
- **Bloque** : Validation réelle de L3 à L8 ; PUB
- **État actuel** : Non démarré. Seul le harnais simulé existe.

## Correction indispensable

### L0 — Intégrité : fermeture bloquée si la sauvegarde échoue ; import contrôlé ; suppression en échec préservée

- **Périmètre (exigences)** : AUD-01, AUD-02, AUD-07 ; NFR-001, NFR-007 ; SDD 5.3 ; FR-DATA-001
- **Décisions préalables** : D18
- **Dépend strictement de** : D18 (règle d'import) ; L1 pour la validation (le développement peut commencer avant)
- **Dépend recommandé de** : —
- **Parallélisable avec** : L3, L8, L9, L12, L13
- **Prérequis** : D18 arbitrée
- **Critères d'acceptation** : (1) import avec liens invalides refusé sans écriture ; (2) fermeture bloquée si save() échoue ; (3) suppression en échec : accès conservé ; (4) tst_dataformat étendu
- **Tests requis** : tst_dataformat ; test QML du flush en échec ; suppression en échec
- **Preuves attendues** : ctest 6.9 ; run_all.py
- **Bloque** : L4 (ordre des migrations) ; L10
- **État actuel** : Non commencé. Défauts constatés : BoardEditor.qml (flush sans contrôle), BackendData.cpp (checkLinks absent)

### L3 — Navigation par pile : retour à l'écran précédent et restauration du contexte

- **Périmètre (exigences)** : FR-NAV-002, FR-NAV-003, FR-NAV-004 ; SDD 3.3 ; SSS-05.4 à 05.6 ; FR-HOME-001 à 003 (retour)
- **Décisions préalables** : D3
- **Dépend strictement de** : D3 (mécanisme)
- **Dépend recommandé de** : L2 (validation réelle)
- **Parallélisable avec** : L0, L8, L9, L12, L13
- **Prérequis** : D3 arbitrée
- **Critères d'acceptation** : Retour exact à l'écran précédent ; retour Android = retour UI ; sélection, défilement et minuteries conservés ; aucune destination codée en dur
- **Tests requis** : t_back étendu ; t_nav ; t_live
- **Preuves attendues** : Suite QML ; parcours P5 (L2)
- **Bloque** : L5, L6 (entrées et retours), L15
- **État actuel** : Non commencé. handleBack() renvoie à l'accueil par défaut : confirmé dans Main.qml

## Développement fonctionnel

### L4 — Clubs et équipes : modèle, sélection, isolation des données

- **Périmètre (exigences)** : FR-CTX-001 à 005 ; FR-NAV-005 ; SSS-04.1, 05.1, 05.2, 07.1 ; SDD 2.2a, 2.2c, 3.3c
- **Décisions préalables** : D1
- **Dépend strictement de** : D1 ; L0 (règles d'import et de migration) ; L1 (migration validée sur 6.9)
- **Dépend recommandé de** : L2 ; L3 (contexte dans la pile)
- **Parallélisable avec** : L8, L9, L12, L13 ; pas en parallèle de L0 (même zone SQL)
- **Prérequis** : D1 arbitrée ; sauvegarde des bases de test
- **Critères d'acceptation** : Migration 005 → 006 sans perte sur les fixtures ; listes filtrées par club et équipe ; sélection club et équipe depuis l'accueil
- **Tests requis** : tst_repositories ; nouveau fixture schéma 6 ; t_home, t_squad étendus
- **Preuves attendues** : ctest 6.9 ; suite QML
- **Bloque** : L5, L7, L10, L11
- **État actuel** : Absent : team sans club_id (schéma 005)

### L5 — Domaine Matchs : modèle, liste, préparation, terrain match

- **Périmètre (exigences)** : FR-MAT-001 à 004 ; FR-SES-001 ; SSS-04.2, 04.5, 05.3, 07.6 ; ECR-09 ; D21
- **Décisions préalables** : D2 ; D21
- **Dépend strictement de** : D2 ; L3 (entrée de pile) ; L4 (rattachement)
- **Dépend recommandé de** : L2
- **Parallélisable avec** : L6, L8, L9, L12, L13 (L6 partage le moteur tactique : coordonner)
- **Prérequis** : D2 arbitrée ; maquette ECR-09 à produire (absente)
- **Critères d'acceptation** : Match distinct d'une séance et d'un exercice (test de non-confusion) ; navigation vers Matchs ; préparation et terrain match
- **Tests requis** : Tests C++ match ; QML match ; parcours calendrier
- **Preuves attendues** : ctest ; suite QML ; P-match (L2)
- **Bloque** : L7 (distinction calendrier)
- **État actuel** : Absent : aucune table, aucun écran

### L6 — Séances et bibliothèque : historique, invariance, copier/coller tactique, suppression typée

- **Périmètre (exigences)** : FR-SES-002 à 009 ; FR-EX-001, 002, 004 ; FR-TAC-001 à 003 ; ECR-03, 05, 06 ; SSS-04.3 à 04.5 ; SDD 3.2, 5.4
- **Décisions préalables** : D8 ; D9
- **Dépend strictement de** : D8 ; D9
- **Dépend recommandé de** : L3
- **Parallélisable avec** : L8, L9, L12, L13 ; L5 (domaines distincts)
- **Prérequis** : D8 et D9 arbitrées ; si D8 = référence, migration planifiée après L4
- **Critères d'acceptation** : Historique accessible ; invariance FR-SES-008 prouvée par test ; copier/coller selon D9 ; suppression typée
- **Tests requis** : t_blockboard ; ctest board ; tests copier/coller
- **Preuves attendues** : ctest 6.9 ; suite QML
- **Bloque** : L15
- **État actuel** : Partiel : séances, exercices, blocs et suppression de terrain présents ; historique, copier/coller absents

### L7 — Calendrier : séances et matchs distincts ; règles détaillées

- **Périmètre (exigences)** : FR-CAL-001 ; SSS-07.7 ; ECR-04 ; SRS §5 q9
- **Décisions préalables** : D17
- **Dépend strictement de** : D17 ; L4 (filtre club)
- **Dépend recommandé de** : L5 (partie matchs)
- **Parallélisable avec** : L8, L9, L11, L12, L13
- **Prérequis** : D17 arbitrée
- **Critères d'acceptation** : Séances et matchs distincts (après L5) ; états vides et changements de période testés
- **Tests requis** : t_calendar étendu
- **Preuves attendues** : Suite QML
- **Bloque** : L15
- **État actuel** : Partiel : calendrier de séances présent, sans match

### L8 — Suivi et présences : tests QML de la page, règles de saisie, statut « blessé »

- **Périmètre (exigences)** : FR-TRK-001 ; SSS-07.8 ; ECR-07 ; AUD-04 ; D22
- **Décisions préalables** : D17 ; D22
- **Dépend strictement de** : D17 ; L1 (ctest attendance sur 6.9)
- **Dépend recommandé de** : L2
- **Parallélisable avec** : Presque tous ; L4 si le filtre club touche les présences
- **Prérequis** : D17 et D22 arbitrées
- **Critères d'acceptation** : t_attendance.py vert ; ctest attendance vert ; règles de saisie appliquées
- **Tests requis** : Nouveau t_attendance.py ; ctest attendance
- **Preuves attendues** : Suite QML ; ctest 6.9
- **Bloque** : PUB
- **État actuel** : Partiel : page et repository présents ; aucun test QML

### L9 — Chronomètres : « Tout démarrer » selon D6 ; stabilité du test temporel

- **Périmètre (exigences)** : FR-TMR-001 à 003 ; FR-HOME-003
- **Décisions préalables** : D6
- **Dépend strictement de** : D6 ; PT-0
- **Dépend recommandé de** : L2
- **Parallélisable avec** : Tous
- **Prérequis** : D6 arbitrée ; PT-0 réalisé
- **Critères d'acceptation** : Décompte stable sur 3 passages ; pause et reprise ; « Tout démarrer » selon D6
- **Tests requis** : t_live, t_live2, t_live3 ; ctest livesession_js
- **Preuves attendues** : Suite QML ; ctest
- **Bloque** : —
- **État actuel** : Partiel : minuteries indépendantes, pause et reprise présentes ; « Tout démarrer » absent

### L10 — Données : mécanisme et fréquence de sauvegarde, canal de transfert, round-trip réel

- **Périmètre (exigences)** : FR-DATA-001 à 003 ; SSS-02e, 07.11 ; SRS §5 q1, q3 ; AUD-02
- **Décisions préalables** : D5a ; D5b
- **Dépend strictement de** : L0 ; L4 (format d'export incluant le club) ; D5a ; D5b ; L1 (tests Windows et Android)
- **Dépend recommandé de** : L2
- **Parallélisable avec** : L8, L9, L11, L12, L13
- **Prérequis** : D5a et D5b arbitrées
- **Critères d'acceptation** : Round-trip sur base réelle, Windows et Android ; mécanisme et fréquence appliqués ; liens invalides refusés (L0)
- **Tests requis** : tst_dataformat ; parcours P2 (L2) ; test sur appareil
- **Preuves attendues** : Journaux ; rapport d'appareil
- **Bloque** : PUB
- **État actuel** : Partiel : export et import manuels présents (migration 005 testée)

### L11 — Effectif : traitement de la table injury ; joueurs multi-équipes

- **Périmètre (exigences)** : FR-TEAM-001, 002 ; SSS-07.2 ; ECR-02 ; AUD-03 ; D4 ; D7
- **Décisions préalables** : D4 ; D7
- **Dépend strictement de** : D4 ; D7 ; L4
- **Dépend recommandé de** : L2
- **Parallélisable avec** : L6, L8, L9, L12, L13
- **Prérequis** : D4 et D7 arbitrées
- **Critères d'acceptation** : Décision D4 appliquée (retrait, migration ou rattachement) ; multi-équipes selon D7 ; tests
- **Tests requis** : tst_repositories ; t_squad
- **Preuves attendues** : ctest 6.9 ; suite QML
- **Bloque** : —
- **État actuel** : Partiel : effectif présent ; table injury orpheline ; multi-équipes absent

### L12 — Assistance : FR-SUP-004 à 006 selon décisions

- **Périmètre (exigences)** : FR-SUP-001 à 006 ; NFR-008 ; SSS-07.9 ; D10
- **Décisions préalables** : D10
- **Dépend strictement de** : D10
- **Dépend recommandé de** : —
- **Parallélisable avec** : Tous
- **Prérequis** : D10 arbitrée
- **Critères d'acceptation** : SRS mise à jour (statuts FR-SUP-004 à 006) ; t_support étendu ; aucun secret dans le binaire (test existant)
- **Tests requis** : t_support
- **Preuves attendues** : Suite QML ; mise à jour SRS
- **Bloque** : —
- **État actuel** : Partiel : formulaire et lien pré-rempli présents ; capture et suivi absents

### L13 — Paramètres et informations : contenu définitif ; emplacement confirmé

- **Périmètre (exigences)** : SSS-07.10 ; ECR-08 ; SRS §5 q7 ; D19 ; D20
- **Décisions préalables** : D19 ; D20
- **Dépend strictement de** : D19 ; D20
- **Dépend recommandé de** : —
- **Parallélisable avec** : Tous
- **Prérequis** : D19 et D20 arbitrées
- **Critères d'acceptation** : Contenu validé ; emplacement confirmé par écrit ; captures bureau et téléphone
- **Tests requis** : t_support étendu ; revue visuelle
- **Preuves attendues** : Captures ; confirmation écrite
- **Bloque** : L15
- **État actuel** : Partiel : Paramètres et Assistance présents, contenu à définir

## Validation de livraison

### L14 — Performance : seuils mesurés sur Android cible et Desktop

- **Périmètre (exigences)** : NFR-006 ; NFR-005
- **Décisions préalables** : D11
- **Dépend strictement de** : L1 ; D11 ; appareil Android cible disponible ; L2 à L8 stabilisés
- **Dépend recommandé de** : —
- **Parallélisable avec** : —
- **Prérequis** : Appareil Android ; seuils D11
- **Critères d'acceptation** : Seuils atteints ; mesures reproductibles (au moins trois mesures)
- **Tests requis** : Mesures chronométrées sur parcours P1 à P4
- **Preuves attendues** : Rapport de mesures
- **Bloque** : PUB
- **État actuel** : Non mesuré

### L15 — Validation visuelle des lots livrés (bureau, tablette, téléphone) ; amendement des documents

- **Périmètre (exigences)** : NFR-005 ; SSS §5 et §6 ; ECR-01 à 10 ; SRS FR-EX-003 ; SDD 5.5 ; DESIGN.md (D23, D24)
- **Décisions préalables** : D15 ; D12 (amendement) ; D20 (confirmation)
- **Dépend strictement de** : D15 ; livraisons fonctionnelles retenues ; amendements documentaires validés
- **Dépend recommandé de** : L14
- **Parallélisable avec** : —
- **Prérequis** : Lots livrés ; captures à 360 et 412 px
- **Critères d'acceptation** : Procès-verbal de validation écrit ; amendements SSS, SRS et DESIGN.md faits
- **Tests requis** : Captures ; contrôles de largeur
- **Preuves attendues** : Procès-verbal ; captures
- **Bloque** : PUB
- **État actuel** : Lot 1 et lots 2 à 5 : captures produites, validation écrite pour lot 1 seulement

### PUB — Publication : fusion v1 → main, tag, backport 0.8.13, fiche Play

- **Périmètre (exigences)** : D13 ; AUD-09
- **Décisions préalables** : D13
- **Dépend strictement de** : D13 ; L0 ; L1 ; L2 ; L10 ; L14 ; L15
- **Dépend recommandé de** : —
- **Parallélisable avec** : —
- **Prérequis** : D13 arbitrée
- **Critères d'acceptation** : Build release signé ; fusion selon D13 ; tag posé seulement si D13 le prévoit
- **Tests requis** : Build release
- **Preuves attendues** : Artefact signé ; historique Git
- **Bloque** : —
- **État actuel** : Non démarré

## Hors périmètre actuel (P3)

### L16 — Analyse et performance avancées

- **Périmètre (exigences)** : SDD §7 point 7
- **Décisions préalables** : —
- **Dépend strictement de** : Toutes les livraisons P0 à P2
- **Dépend recommandé de** : —
- **Parallélisable avec** : —
- **Prérequis** : Périmètre à définir
- **Critères d'acceptation** : À définir
- **Tests requis** : À définir
- **Preuves attendues** : À définir
- **Bloque** : —
- **État actuel** : Non défini

## Évolutions envisagées — à cadrer (hors lots engagés)

Non arbitrées. Ne figurent dans aucune exigence validée ni dans un lot.

Décision D14 (profils d'entraîneur) : traitée hors lots, en proposition ; voir le registre des décisions.

### Vue globale des exercices sur un terrain

**Statut** : Non arbitré. Aucun lot, aucune exigence validée.

Questions à résoudre avant de l'ajouter à la SSS, la SRS et la SDD :

- Quelle représentation pour un exercice sans schéma tactique (liste dans la vue, emplacement réservé) ?
- La durée totale affichée est-elle la somme des durées de la séance, ou une durée de déroulé ?
- Ordre et numérotation des exercices sur le terrain : ordre de la séance ?
- Terrain unique : superposition, zones ou vignettes par exercice ?
- Technique : agrégation en lecture seule de session_exercise et tactical_board, sans nouvelle table ?
- Impacts : SSS, SRS, SDD, maquette à produire, lot à créer après L6.

## Contrôles de la feuille de route

- Lots : 19 ; décisions référencées : 25.
- Dépendances : aucune erreur.
- Cycles de dépendances : aucun.
