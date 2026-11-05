# Registre des décisions — FootCoach v3

Commit de référence : `624997e52bce151851d37eb4c746a13cb5cfb6a1`. Date : 2026-11-05.

Statuts : **TRANCHÉE** = validée par l'utilisateur ; **NON DÉCIDÉE** = arbitrage nécessaire ; **PROPOSITION** = arbitrage ouvert, à valider ; **PARTIELLEMENT TRANCHÉE** = confirmation écrite attendue.

## Convention de preuve

- Fait vérifié : contrôle exécuté ou reproductible, avec le commit indiqué (suite QML 377 sur 624997e, git diff, SHA-256 du PDF).
- Constat issu du dépôt : lu dans un fichier du dépôt, non exécuté (Main.qml height 72, Theme.qml ligne 41, DESIGN.md).
- Hypothèse ou non vérifié : non prouvé ici (build Qt 6.9, Android, backend C++ réel, ctest sur 624997e, rapprochement PDF/PNG).
- Décision en attente : NON DÉCIDÉE, PROPOSITION ou PARTIELLEMENT TRANCHÉE. Seule la mention TRANCHÉE est une décision validée.

| ID | Sujet | Lot | Statut | Dépendances |
|---|---|---|---|---|
| D1 | Modèle club : table club, team.club_id, migration 006 | L4 | NON DÉCIDÉE (modèle à valider) | Aucune ; avant L4 |
| D2 | Modèle match : champs, rattachement club/équipe, terrain match | L5 | NON DÉCIDÉE (champs à définir) | D1 ; D3 ; L4 |
| D3 | Navigation : mécanisme de pile explicite | L3 | NON DÉCIDÉE (mécanisme) — principe validé | Aucune ; avant L3 |
| D4 | Sort de la table injury (absente de la SRS ; donnée de santé potentielle) | L11 | NON DÉCIDÉE | Aucune ; avant L11 |
| D5a | Sauvegarde : mécanisme (manuel, automatique, les deux) et fréquence | L10 | NON DÉCIDÉE | Aucune ; avant L10 |
| D5b | Transfert Desktop ↔ Android : fichier, USB, ou les deux | L10 | NON DÉCIDÉE | Aucune ; avant L10 |
| D6 | Sens de « Tout démarrer » pour les minuteries | L9 | NON DÉCIDÉE | Aucune ; avant L9 |
| D7 | Identité des joueurs présents dans plusieurs équipes ou clubs | L11 | NON DÉCIDÉE | D1 ; avant L11 |
| D8 | Bibliothèque vers séance : copie instantanée ou référence | L6 | NON DÉCIDÉE (impact schéma) | Aucune ; avant L6 |
| D9 | Copier/coller tactique : raccourci, périmètre | L6 | NON DÉCIDÉE | Aucune ; avant L6 |
| D10 | Assistance : backend, capture d'écran, suivi de ticket | L12 | NON DÉCIDÉE (FR-SUP-004 : lien pré-rempli retenu de fait) | Aucune ; avant L12 |
| D11 | Seuils de performance et de robustesse | L14 | NON DÉCIDÉE | Mesures de L14 |
| D12 | Libellés de navigation mobile | L15 (amendement) | TRANCHÉE le 2026-11-05 | — |
| D13 | Stratégie de publication : fusion v1 → main, tag, backport 0.8.13, Play | PUB | NON DÉCIDÉE | L0, L1, L2, L10, L14, L15 |
| D14 | Profils d'entraîneur : un seul, ou plusieurs par appareil | Hors lot (proposition) | NON DÉCIDÉE — PROPOSITION DE TRAITEMENT | — |
| D15 | Référence écrans : Design/ux et Design/DESIGN.md (désigné VDS dans la mission) | L15 | NON DÉCIDÉE (rapprochement écran par écran à faire) | Avant L15 |
| D16 | Commit ou abandon du correctif t_live.py | PT-0 | NON DÉCIDÉE | Avant L9 |
| D17 | Règles détaillées du calendrier et du suivi | L7 ; L8 | NON DÉCIDÉE | Avant L7 et L8 |
| D18 | Import avec liens invalides : refus total ou réparation | L0 | NON DÉCIDÉE | Avant L0 |
| D19 | Contenu des paramètres (réglages et informations) | L13 | NON DÉCIDÉE | Avant L13 |
| D20 | Emplacement de Paramètres et d'Assistance (SRS §5 q7) | L13 ; L15 | PARTIELLEMENT TRANCHÉE — EN ATTENTE de la confirmation écrite de l'utilisateur | Avant L13 |
| D21 | Accès à Matchs sur mobile (absent de la barre à ce stade) | L5 | PROPOSITION — arbitrage ouvert | D2 ; L5 |
| D22 | Statut « blessé » des présences (AUD-04) | L8 | PROPOSITION — arbitrage ouvert | D17 ; avant L8 |
| D23 | Hauteur de la barre mobile : DESIGN.md 64 px, code 72 px | L15 | PROPOSITION — arbitrage ouvert | — |
| D24 | Police du texte courant : Inter (DESIGN.md) ou police système (code) | L15 | PROPOSITION — arbitrage ouvert | — |

## Détail

### D1 — Modèle club : table club, team.club_id, migration 006

- **Formulation** : Club → Équipes → Activités : le club est un niveau de hiérarchie distinct de l'équipe.
- **Sources** : SSS §4.1 ; SRS FR-CTX-001 à 005 ; SDD 2.2c
- **Lot impacté** : L4
- **Statut** : NON DÉCIDÉE (modèle à valider)
- **Dépendances** : Aucune ; avant L4
- **Conséquences fonctionnelles** : Sélection club puis équipe ; isolation des données
- **Conséquences techniques** : Migration 006 ; tous les repositories filtrent par club
- **Déjà acquis** : Principe club > équipe validé (SSS §4)
- **Proposition** : Proposition : table club, colonne team.club_id, migration avec sauvegarde préalable.

### D2 — Modèle match : champs, rattachement club/équipe, terrain match

- **Formulation** : Un match est indépendant de la séance et n'est jamais un exercice.
- **Sources** : SSS §4.2 ; SRS FR-MAT-001 à 004 ; SRS §5 q8
- **Lot impacté** : L5
- **Statut** : NON DÉCIDÉE (champs à définir)
- **Dépendances** : D1 ; D3 ; L4
- **Conséquences fonctionnelles** : Liste, préparation, historique des matchs
- **Conséquences techniques** : Migration 007 ; repository match
- **Déjà acquis** : Indépendance match/séance validée (SSS §4.2)
- **Proposition** : —

### D3 — Navigation : mécanisme de pile explicite

- **Formulation** : Le retour restaure l'écran précédent, pas une destination fixe.
- **Sources** : SRS FR-NAV-002 à 004 ; SDD 3.3
- **Lot impacté** : L3
- **Statut** : NON DÉCIDÉE (mécanisme) — principe validé
- **Dépendances** : Aucune ; avant L3
- **Conséquences fonctionnelles** : Retour et restauration de contexte
- **Conséquences techniques** : Pile QML dans Main.qml ; retour Android = retour UI
- **Déjà acquis** : Principe validé (SRS FR-NAV-002)
- **Proposition** : Proposition : pile QML portée par Main.qml.

### D4 — Sort de la table injury (absente de la SRS ; donnée de santé potentielle)

- **Formulation** : Aucune exigence ne décrit cette table aujourd'hui.
- **Sources** : AUD-03 ; consigne lot 4 (pas de champs biologiques)
- **Lot impacté** : L11
- **Statut** : NON DÉCIDÉE
- **Dépendances** : Aucune ; avant L11
- **Conséquences fonctionnelles** : Aucune pour l'instant
- **Conséquences techniques** : Table présente, cascade seulement
- **Déjà acquis** : —
- **Proposition** : Proposition : retirer la table ou la rattacher à une exigence explicite, sans donnée de santé détaillée.

### D5a — Sauvegarde : mécanisme (manuel, automatique, les deux) et fréquence

- **Formulation** : Question ouverte SRS §5 q1.
- **Sources** : SRS §5 q1 ; FR-DATA-002
- **Lot impacté** : L10
- **Statut** : NON DÉCIDÉE
- **Dépendances** : Aucune ; avant L10
- **Conséquences fonctionnelles** : Rappel, sauvegarde automatique ou manuelle
- **Conséquences techniques** : Fichier .footcoach existant ; rappel à concevoir
- **Déjà acquis** : Export/import manuel implémenté
- **Proposition** : —

### D5b — Transfert Desktop ↔ Android : fichier, USB, ou les deux

- **Formulation** : Question ouverte SRS §5 q3.
- **Sources** : SRS §5 q3 ; FR-DATA-003 ; SSS-02e
- **Lot impacté** : L10
- **Statut** : NON DÉCIDÉE
- **Dépendances** : Aucune ; avant L10
- **Conséquences fonctionnelles** : Procédure de transfert
- **Conséquences techniques** : Canal non implémenté (fichier manuel seulement)
- **Déjà acquis** : —
- **Proposition** : —

### D6 — Sens de « Tout démarrer » pour les minuteries

- **Formulation** : Question ouverte SRS §5 q6.
- **Sources** : FR-TMR-003 ; SRS §5 q6
- **Lot impacté** : L9
- **Statut** : NON DÉCIDÉE
- **Dépendances** : Aucune ; avant L9
- **Conséquences fonctionnelles** : Démarrage simultané ou non des minuteries
- **Conséquences techniques** : Absent
- **Déjà acquis** : Minuteries indépendantes validées (FR-TMR-001/002)
- **Proposition** : —

### D7 — Identité des joueurs présents dans plusieurs équipes ou clubs

- **Formulation** : Question ouverte SRS §5 q5.
- **Sources** : FR-TEAM-002 ; SRS §5 q5
- **Lot impacté** : L11
- **Statut** : NON DÉCIDÉE
- **Dépendances** : D1 ; avant L11
- **Conséquences fonctionnelles** : Réutilisation de la fiche joueur
- **Conséquences techniques** : Non modélisé
- **Déjà acquis** : —
- **Proposition** : —

### D8 — Bibliothèque vers séance : copie instantanée ou référence

- **Formulation** : Question ouverte SRS §5 q4 ; FR-EX-004.
- **Sources** : FR-EX-004 ; SRS §5 q4
- **Lot impacté** : L6
- **Statut** : NON DÉCIDÉE (impact schéma)
- **Dépendances** : Aucune ; avant L6
- **Conséquences fonctionnelles** : Modification d'un exercice se propage ou non
- **Conséquences techniques** : Migration si référence ; L0 et L4 à coordonner
- **Déjà acquis** : Ne pas coder de propagation implicite (SRS FR-EX-004)
- **Proposition** : —

### D9 — Copier/coller tactique : raccourci, périmètre

- **Formulation** : FR-TAC-002 : copie d'éléments.
- **Sources** : FR-TAC-002 ; SDD P1
- **Lot impacté** : L6
- **Statut** : NON DÉCIDÉE
- **Dépendances** : Aucune ; avant L6
- **Conséquences fonctionnelles** : Copie multi-objets
- **Conséquences techniques** : Absent (Ctrl+C = inverser les courbes)
- **Déjà acquis** : —
- **Proposition** : Proposition : Ctrl+C / Ctrl+V sur la sélection courante.

### D10 — Assistance : backend, capture d'écran, suivi de ticket

- **Formulation** : FR-SUP-004 à 006.
- **Sources** : SRS §2.11
- **Lot impacté** : L12
- **Statut** : NON DÉCIDÉE (FR-SUP-004 : lien pré-rempli retenu de fait)
- **Dépendances** : Aucune ; avant L12
- **Conséquences fonctionnelles** : Canal de signalement
- **Conséquences techniques** : Lien GitHub pré-rempli ; pas de backend
- **Déjà acquis** : Lien pré-rempli existant, sans jeton
- **Proposition** : Proposition : consigner le lien comme solution retenue ; capture et suivi hors périmètre initial.

### D11 — Seuils de performance et de robustesse

- **Formulation** : NFR-006 : seuils à définir après mesure.
- **Sources** : NFR-006 ; NFR-007
- **Lot impacté** : L14
- **Statut** : NON DÉCIDÉE
- **Dépendances** : Mesures de L14
- **Conséquences fonctionnelles** : —
- **Conséquences techniques** : Aucune mesure
- **Déjà acquis** : —
- **Proposition** : —

### D12 — Libellés de navigation mobile

- **Formulation** : Six entrées en français : Accueil, Séances, Exercices, Calendrier, Effectif, Suivi. Pas d'entrée « Plus ». Pas d'entrée Matchs à ce stade.
- **Sources** : Décision utilisateur du 2026-11-05 ; SSS §5 et §6 (libellés anglais à amender)
- **Lot impacté** : L15 (amendement)
- **Statut** : TRANCHÉE le 2026-11-05
- **Dépendances** : —
- **Conséquences fonctionnelles** : Navigation mobile livrée (lot 1, commit 624997e)
- **Conséquences techniques** : Vérifié en harnais (t_nav) ; à revérifier sur backend réel
- **Déjà acquis** : Décision définitive
- **Proposition** : —

### D13 — Stratégie de publication : fusion v1 → main, tag, backport 0.8.13, Play

- **Formulation** : v1 = main + 21 commits ; main = 0.8.12 ; tag v0.10.0-sch5 non posé.
- **Sources** : AUD-09 ; SDD 4 et 9
- **Lot impacté** : PUB
- **Statut** : NON DÉCIDÉE
- **Dépendances** : L0, L1, L2, L10, L14, L15
- **Conséquences fonctionnelles** : —
- **Conséquences techniques** : Remote : main 065c407, v1 624997e, 4 tags v0.8.x
- **Déjà acquis** : —
- **Proposition** : —

### D14 — Profils d'entraîneur : un seul, ou plusieurs par appareil

- **Formulation** : Question ouverte SRS §5 q2.
- **Sources** : SRS §5 q2 ; SSS §8 (gestion multi-utilisateurs simultanée exclue)
- **Lot impacté** : Hors lot (proposition)
- **Statut** : NON DÉCIDÉE — PROPOSITION DE TRAITEMENT
- **Dépendances** : —
- **Conséquences fonctionnelles** : —
- **Conséquences techniques** : Aucun profil modélisé
- **Déjà acquis** : Pas de gestion multi-utilisateurs simultanée (SSS §8)
- **Proposition** : Proposition : hors périmètre initial (un seul profil local). Justification : SSS §8 exclut la gestion multi-utilisateurs ; la SSS ne dit pas explicitement si plusieurs profils locaux sont exclus. À confirmer.

### D15 — Référence écrans : Design/ux et Design/DESIGN.md (désigné VDS dans la mission)

- **Formulation** : Référence visuelle des écrans.
- **Sources** : SSS §6 ; Design/DESIGN.md
- **Lot impacté** : L15
- **Statut** : NON DÉCIDÉE (rapprochement écran par écran à faire)
- **Dépendances** : Avant L15
- **Conséquences fonctionnelles** : —
- **Conséquences techniques** : Design/DESIGN.md présent ; PDF UXPilot rangé dans Design/ (identique au fichier fourni, SHA-256 1184ee63…892d)
- **Déjà acquis** : Design/ux (25 PNG) présent
- **Proposition** : Proposition : retenir Design/ux et Design/DESIGN.md comme références ; le PDF UXPilot est désormais dans Design/ sous le nom cité par la SSS §6.

### D16 — Commit ou abandon du correctif t_live.py

- **Formulation** : Modification non commitée, test sensible au temps.
- **Sources** : AUD-11
- **Lot impacté** : PT-0
- **Statut** : NON DÉCIDÉE
- **Dépendances** : Avant L9
- **Conséquences fonctionnelles** : —
- **Conséquences techniques** : tests/qml/t_live.py modifié, non commité
- **Déjà acquis** : —
- **Proposition** : Proposition : commiter séparément le correctif après revue, puis stabiliser le test (horloge simulée).

### D17 — Règles détaillées du calendrier et du suivi

- **Formulation** : Question ouverte SRS §5 q9.
- **Sources** : SRS §5 q9
- **Lot impacté** : L7 ; L8
- **Statut** : NON DÉCIDÉE
- **Dépendances** : Avant L7 et L8
- **Conséquences fonctionnelles** : Règles de saisie et d'affichage
- **Conséquences techniques** : —
- **Déjà acquis** : —
- **Proposition** : —

### D18 — Import avec liens invalides : refus total ou réparation

- **Formulation** : Un import ne doit jamais écrire un lien invalide.
- **Sources** : SDD 5.3 ; AUD-02
- **Lot impacté** : L0
- **Statut** : NON DÉCIDÉE
- **Dépendances** : Avant L0
- **Conséquences fonctionnelles** : Message d'erreur et issue de l'import
- **Conséquences techniques** : checkLinks() non appelé à l'import
- **Déjà acquis** : Import transactionnel avec sauvegarde préalable (constaté)
- **Proposition** : Proposition : refus total sans écriture, plus message explicite.

### D19 — Contenu des paramètres (réglages et informations)

- **Formulation** : Contenu à définir.
- **Sources** : SSS-07.10
- **Lot impacté** : L13
- **Statut** : NON DÉCIDÉE
- **Dépendances** : Avant L13
- **Conséquences fonctionnelles** : —
- **Conséquences techniques** : Version et politique de confidentialité présentes
- **Déjà acquis** : —
- **Proposition** : —

### D20 — Emplacement de Paramètres et d'Assistance (SRS §5 q7)

- **Formulation** : Paramètres atteint depuis l'accueil ; Assistance dans Paramètres.
- **Sources** : SRS §5 q7 ; lot 1
- **Lot impacté** : L13 ; L15
- **Statut** : PARTIELLEMENT TRANCHÉE — EN ATTENTE de la confirmation écrite de l'utilisateur
- **Dépendances** : Avant L13
- **Conséquences fonctionnelles** : Accès Paramètres depuis l'accueil (constaté)
- **Conséquences techniques** : SettingsPage.qml accessible depuis HomePage
- **Déjà acquis** : Paramètres et Assistance hors barre mobile (lot 1)
- **Proposition** : Proposition : confirmer par écrit cet emplacement et l'inscrire dans la SRS.

### D21 — Accès à Matchs sur mobile (absent de la barre à ce stade)

- **Formulation** : Aucune entrée Matchs dans la barre tant que le domaine n'existe pas.
- **Sources** : Décision du 2026-11-05 ; SRS FR-MAT-002 (rubrique de premier niveau)
- **Lot impacté** : L5
- **Statut** : PROPOSITION — arbitrage ouvert
- **Dépendances** : D2 ; L5
- **Conséquences fonctionnelles** : Entrée Matchs à définir
- **Conséquences techniques** : —
- **Déjà acquis** : —
- **Proposition** : Proposition : entrée Matchs dans l'accueil à l'introduction de L5, la barre restant à six entrées ; à arbitrer.

### D22 — Statut « blessé » des présences (AUD-04)

- **Formulation** : Statut injured présent dans le code, absent de la SRS.
- **Sources** : AUD-04 ; AttendanceRepository
- **Lot impacté** : L8
- **Statut** : PROPOSITION — arbitrage ouvert
- **Dépendances** : D17 ; avant L8
- **Conséquences fonctionnelles** : —
- **Conséquences techniques** : Statut injured compté dans les statistiques
- **Déjà acquis** : —
- **Proposition** : Proposition : rattacher le statut à FR-TRK-001 dans la SRS, ou le retirer.

### D23 — Hauteur de la barre mobile : DESIGN.md 64 px, code 72 px

- **Formulation** : Divergence documentaire.
- **Sources** : Design/DESIGN.md (ligne 152) ; Main.qml (height 72)
- **Lot impacté** : L15
- **Statut** : PROPOSITION — arbitrage ouvert
- **Dépendances** : —
- **Conséquences fonctionnelles** : —
- **Conséquences techniques** : —
- **Déjà acquis** : —
- **Proposition** : Proposition : aligner DESIGN.md sur 72 px (valeur du code validée visuellement).

### D24 — Police du texte courant : Inter (DESIGN.md) ou police système (code)

- **Formulation** : DESIGN.md prescrit Inter ; Inter n'est pas intégrée.
- **Sources** : Design/DESIGN.md (ligne 136) ; Theme.qml ; reporté par l'utilisateur
- **Lot impacté** : L15
- **Statut** : PROPOSITION — arbitrage ouvert
- **Dépendances** : —
- **Conséquences fonctionnelles** : Rendu typographique
- **Conséquences techniques** : Outfit embarquée ; Inter absente
- **Déjà acquis** : Intégration d'Inter reportée (décision utilisateur)
- **Proposition** : Proposition : amender DESIGN.md pour indiquer la police système, ou intégrer Inter lors de L15.
