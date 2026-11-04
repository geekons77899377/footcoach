# FootCoach — SRS
## Software Requirements Specification

**Version du document :** 1.0  
**Date :** 4 novembre 2026  
**Statut :** exigences fonctionnelles et non fonctionnelles consolidées. Les statuts indiquent le niveau de décision, pas l'état d'implémentation.

---

## 1. Conventions

- **CONFIRMÉE** : exigence exprimée ou décision explicite de l'utilisateur.
- **PROPOSITION** : solution recommandée, à faire confirmer.
- **À VÉRIFIER** : état ou comportement actuel non démontré.
- **NON DÉCIDÉE** : arbitrage utilisateur manquant.
- **À TESTER** : critère de validation à exécuter sur le produit réel.

Priorité : P0 = intégrité/sécurité/navigation fondamentale ; P1 = cœur fonctionnel ; P2 = évolution.

## 2. Exigences fonctionnelles

### 2.1 Contexte club / équipe

**FR-CTX-001 — Multi-clubs**  
Statut : CONFIRMÉE — P1  
Le logiciel doit permettre à un entraîneur de gérer plusieurs clubs dans une installation.

**FR-CTX-002 — Équipes par club**  
Statut : CONFIRMÉE — P1  
Un club regroupe plusieurs équipes.

**FR-CTX-003 — Sélection du club**  
Statut : CONFIRMÉE — P1  
L'utilisateur peut changer le club sélectionné.

**FR-CTX-004 — Sélection de l'équipe**  
Statut : CONFIRMÉE — P1  
L'utilisateur peut changer l'équipe sélectionnée dans le contexte du club.

**FR-CTX-005 — Isolation du contexte**  
Statut : PROPOSITION À CONFIRMER — P0  
Les listes et données affichées doivent être filtrées par club/équipe pertinents. Les règles exactes de rattachement des joueurs, séances, matchs et calendriers doivent être définies dans le modèle de données.

### 2.2 Navigation

**FR-NAV-001 — Navigation métier**  
Statut : CONFIRMÉE — P0  
La hiérarchie fonctionnelle suit : Club sélectionné → Équipe sélectionnée → Accueil / Séances / Matchs / Équipe / Calendrier / Suivi.

**FR-NAV-002 — Retour à l'écran précédent**  
Statut : CONFIRMÉE — P0  
Le retour doit restaurer l'écran réellement précédent, pas une destination fixe.

**FR-NAV-003 — Restauration du contexte**  
Statut : CONFIRMÉE — P0  
Le retour doit préserver les identifiants et états pertinents : sélection, position de défilement, minuterie et modifications en cours, selon le contexte.

**FR-NAV-004 — Cohérence Android**  
Statut : CONFIRMÉE — P0  
Le retour système Android et le retour de l'interface suivent la même pile de navigation.

**FR-NAV-005 — Visibilité du contexte**  
Statut : PROPOSITION — P1  
Le club et l'équipe courants doivent rester visibles ou immédiatement identifiables.

### 2.3 Accueil

**FR-HOME-001 — Tableau de bord contextualisé**  
Statut : CONFIRMÉE dans le principe — P1  
L'accueil présente les informations utiles au club/à l'équipe sélectionnés.

**FR-HOME-002 — Changer d'équipe depuis l'accueil**  
Statut : CONFIRMÉE — P1  
Un bouton de l'accueil permet de changer d'équipe.

**FR-HOME-003 — Fermer une carte de séance**  
Statut : CONFIRMÉE — P1  
L'action « Fermer » sur la carte d'une séance retire uniquement cette carte de l'accueil. Elle ne supprime ni la séance ni ses données. La séance reste accessible depuis la gestion des séances.

### 2.4 Séances d'entraînement

**FR-SES-001 — Séances indépendantes des matchs**  
Statut : CONFIRMÉE — P0  
Les séances d'entraînement sont un domaine fonctionnel distinct des matchs.

**FR-SES-002 — Liste, création, historique**  
Statut : CONFIRMÉE — P1  
La rubrique Séances permet de consulter la liste, créer une séance et consulter l'historique.

**FR-SES-003 — Composition d'une séance**  
Statut : CONFIRMÉE — P1  
Une séance contient une liste ordonnée d'exercices.

**FR-SES-004 — Données d'un exercice**  
Statut : CONFIRMÉE — P1  
Un exercice comprend titre, catégorie, objectif, durée, minimum/maximum de joueurs, matériel et description.

**FR-SES-005 — Terrain tactique facultatif**  
Statut : CONFIRMÉE — P1  
Un exercice peut exister sans représentation tactique.

**FR-SES-006 — Terrain par bloc**  
Statut : CONFIRMÉE dans la conception des étapes 1–2 — À VÉRIFIER techniquement — P1  
Chaque bloc/exercice d'un plan de séance peut posséder son propre tableau tactique. La règle cible est au plus un tableau par bloc.

**FR-SES-007 — Aperçu et édition**  
Statut : CONFIRMÉE — P1  
Le plan de séance affiche un aperçu tactique réduit ; l'édition s'effectue dans une vue plus grande.

**FR-SES-008 — Invariance des données à l'affichage**  
Statut : CONFIRMÉE — P0  
Changer la taille ou le mode d'affichage du terrain ne modifie ni objets ni positions ni flèches/tracés ni propriétés tactiques.

**FR-SES-009 — Retrait du terrain**  
Statut : CONFIRMÉE — P1  
Retirer le terrain d'un bloc ne doit pas supprimer l'exercice lui-même.

### 2.5 Bibliothèque d'exercices

**FR-EX-001 — Exercices prédéfinis**  
Statut : CONFIRMÉE — P1  
La bibliothèque contient initialement des exercices prédéfinis.

**FR-EX-002 — Personnalisation**  
Statut : CONFIRMÉE — P1  
L'utilisateur peut créer et personnaliser des exercices.

**FR-EX-003 — Accès par bouton**  
Statut : CONFIRMÉE — P1  
La bibliothèque est accessible par un bouton. Son emplacement exact dans la navigation finale reste à confirmer.

**FR-EX-004 — Sémantique modèle/instance**  
Statut : NON DÉCIDÉE — P1  
Définir si l'insertion d'un exercice dans une séance crée un instantané indépendant ou une référence au modèle de bibliothèque. Ne pas coder de propagation implicite des modifications avant décision.

### 2.6 Terrain / tableau tactique

**FR-TAC-001 — Composant partagé**  
Statut : CONFIRMÉE — P1  
Le moteur tactique est réutilisable dans les contextes exercice/atelier et match, sans fusionner les entités métier de ces contextes.

**FR-TAC-002 — Copier/coller**  
Statut : CONFIRMÉE — P1  
Copier un ou plusieurs objets sélectionnés et coller des copies en conservant type, apparence, orientation, dimensions et propriétés. Décaler légèrement les copies pour qu'elles soient visibles et sélectionnables. Autoriser les collages successifs et les raccourcis clavier standards (Ctrl+C/Ctrl+V et équivalents plateforme). Les originaux restent inchangés.

**FR-TAC-003 — Suppression indépendante du type**  
Statut : CONFIRMÉE — P1  
L'outil de suppression ne doit pas disparaître selon la catégorie des objets sélectionnés. Sa disponibilité dépend du contexte d'édition et de la sélection, pas du type (joueur, adversaire, but, ballon, etc.).

### 2.7 Matchs

**FR-MAT-001 — Domaine indépendant**  
Statut : CONFIRMÉE — P0  
Les matchs sont indépendants des séances d'entraînement.

**FR-MAT-002 — Navigation directe**  
Statut : CONFIRMÉE — P1  
Matchs est une rubrique métier de premier niveau dans la hiérarchie de l'équipe.

**FR-MAT-003 — Liste, préparation, historique**  
Statut : CONFIRMÉE au niveau navigation — P1  
La rubrique contient liste des matchs, préparation d'un match et historique.

**FR-MAT-004 — Terrain tactique**  
Statut : CONFIRMÉE dans le principe — P1  
Le terrain tactique peut être utilisé pour placer les joueurs et décrire l'organisation tactique d'un match. Les écrans détaillés et le modèle de données match restent à spécifier.

### 2.8 Équipe et joueurs

**FR-TEAM-001 — Effectif**  
Statut : CONFIRMÉE — P1  
La rubrique Équipe donne accès à l'effectif, aux fiches joueurs et à la catégorie.

**FR-TEAM-002 — Réutilisation des joueurs entre équipes/clubs**  
Statut : NON DÉCIDÉE — P1  
Définir l'identité globale d'un joueur et ses affiliations à plusieurs équipes/clubs. Éviter les doublons sans présumer du modèle.

### 2.9 Calendrier et suivi

**FR-CAL-001 — Calendrier**  
Statut : CONFIRMÉE au niveau navigation — P1  
Une rubrique Calendrier est prévue. Les événements, règles de récurrence et interactions exactes restent à spécifier.

**FR-TRK-001 — Suivi**  
Statut : CONFIRMÉE au niveau navigation — P1  
Une rubrique Suivi est prévue. Le détail des indicateurs et rapports reste à spécifier.

### 2.10 Minuteries d'exercices

**FR-TMR-001 — Minuteries indépendantes**  
Statut : CONFIRMÉE — P1  
Chaque exercice peut démarrer indépendamment ou simultanément avec d'autres.

**FR-TMR-002 — Pause/reprise indépendante**  
Statut : CONFIRMÉE — P1  
Chaque minuterie peut être mise en pause/reprise indépendamment. Mettre en pause A ne doit pas affecter B. L'état écoulé est conservé.

**FR-TMR-003 — « Tout démarrer »**  
Statut : NON DÉCIDÉE  
La règle exacte de cette action n'est pas arbitrée. Ne pas supposer qu'elle démarre uniquement les minuteries jamais lancées sans validation.

### 2.11 Assistance et signalement

**FR-SUP-001 — Aide et assistance**  
Statut : CONFIRMÉE dans le principe — P2  
Prévoir Signaler un problème, Proposer une amélioration et Contacter le support.

**FR-SUP-002 — Ticket**  
Statut : PROPOSITION détaillée à valider — P2  
Formulaire : catégorie, titre, description, étapes de reproduction, capture facultative.

**FR-SUP-003 — Métadonnées techniques**  
Statut : PROPOSITION — P2  
Collecte utile : version FootCoach, Qt, OS/version, schéma DB, identifiant d'installation aléatoire, diagnostics filtrés des données personnelles.

**FR-SUP-004 — Backend GitHub sécurisé**  
Statut : PROPOSITION — P2  
Si GitHub Issues est utilisé, ne jamais embarquer de jeton GitHub personnel dans l'application. Prévoir un service intermédiaire et une GitHub App aux permissions minimales, limitation de débit et protection anti-abus.

**FR-SUP-005 — Capture d'écran**  
Statut : CONFIRMÉE — P2  
Aucune capture automatique ou en arrière-plan. Capture uniquement après action explicite, revue et confirmation de l'utilisateur ; prévoir la protection des données personnelles/joueurs.

**FR-SUP-006 — Suivi de ticket**  
Statut : PROPOSITION demandée — P2  
Email facultatif. Sans email, proposer un identifiant de ticket et un secret affiché une seule fois, conservé de manière sécurisée côté service. Ne pas utiliser l'email comme identifiant principal.

### 2.12 Sauvegarde et restauration

**FR-DATA-001 — Sauvegarde/restauration**  
Statut : EXIGENCE STRATÉGIQUE — P0  
Prévoir une sauvegarde/restauration fiable, versionnée, incluant base SQLite et fichiers associés, contrôles d'intégrité et migrations compatibles.

**FR-DATA-002 — Fréquence**  
Statut : NON DÉCIDÉE  
Manuelle, automatique ou les deux.

**FR-DATA-003 — Transfert Desktop/Android**  
Statut : NON DÉCIDÉE  
Fichier de sauvegarde, USB, ou les deux.

## 3. Exigences non fonctionnelles

**NFR-001 — Intégrité des données (P0)**  
Aucune opération de navigation, fermeture, suppression ou changement de contexte ne doit provoquer de perte silencieuse de données.

**NFR-002 — Persistance locale (P0)**  
Les données métier sont stockées localement dans SQLite selon la direction produit retenue.

**NFR-003 — Offline (P1)**  
Les fonctions cœur doivent être utilisables sans connexion réseau.

**NFR-004 — Portabilité (P1)**  
Desktop et Android partagent autant que possible le cœur métier. Les chemins de fichiers et API spécifiques sont isolés.

**NFR-005 — Ergonomie multi-écran (P1)**  
Interface adaptée Desktop, tablette et mobile. La parité porte sur les capacités et les règles, pas sur une disposition strictement identique.

**NFR-006 — Performance (P1)**  
Listes, aperçus tactiques, navigation et sauvegarde doivent rester fluides sur les appareils Android cibles et Desktop ; seuils chiffrés à définir après mesure.

**NFR-007 — Robustesse des erreurs (P0)**  
Les échecs de sauvegarde, chargement, suppression ou migration doivent être visibles et ne doivent pas fermer/effacer l'accès aux données sans décision explicite.

**NFR-008 — Sécurité locale (P1)**  
Ne pas exposer de secrets GitHub dans le binaire. Protéger les éventuels secrets de suivi de tickets.

**NFR-009 — Testabilité (P0)**  
Séparer tests unitaires C++/repositories, tests d'intégration Qt/SQLite et tests QML réellement exécutés avec le backend de production.

## 4. Critères d'acceptation transverses

- Un entraîneur peut sélectionner un club puis une équipe et les rubriques présentent le contexte correspondant.
- Un match n'apparaît pas comme une séance et n'est pas stocké comme un exercice.
- Une séance contient des exercices ordonnés ; un exercice peut ne pas avoir de terrain.
- Un terrain d'atelier est attaché au bon bloc et son aperçu n'altère pas ses données.
- Le retour restaure l'écran précédent et son contexte.
- Fermer une carte d'accueil ne supprime pas la séance.
- Les minuteries d'exercices sont indépendantes.
- Les opérations de sauvegarde et de migration sont vérifiées sur base réelle.
- Les parcours critiques sont testés sur Desktop et Android réels, pas seulement dans un harness simulé.

## 5. Questions bloquantes ou ouvertes

1. Sauvegarde : manuelle, automatique, ou les deux ?
2. Plusieurs profils d'entraîneur sur un même appareil, ou un seul profil ?
3. Transfert Desktop ↔ Android : fichier, USB, ou les deux ?
4. Sémantique bibliothèque → séance : copie/instantané ou référence ?
5. Identité et affiliation des joueurs à plusieurs équipes/clubs ?
6. Sens précis de « Tout démarrer » pour les minuteries ?
7. Place exacte de Bibliothèque, Paramètres et Assistance dans la navigation détaillée ?
8. Modèle fonctionnel et données détaillées des matchs ?
9. Règles détaillées du calendrier et du suivi ?
