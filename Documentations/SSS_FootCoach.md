# FootCoach — SSS
## System / Software Scope Specification

**Version du document :** 1.0  
**Date :** 4 novembre 2026  
**Statut :** synthèse de reprise — décisions métier confirmées et éléments à vérifier explicitement séparés.

---

## 1. Objet du document

Ce document donne à Claude AI le contexte système et produit nécessaire pour reprendre FootCoach sans réinterpréter les décisions déjà prises. Il décrit le produit, son périmètre, ses utilisateurs, son environnement et ses limites.

Il ne constitue pas une preuve que les fonctionnalités décrites sont déjà implémentées ou validées.

## 2. Vision du produit

FootCoach est un logiciel de préparation, d'organisation et de suivi des activités d'un entraîneur de football.

L'objectif est de proposer des capacités ambitieuses et professionnelles, accessibles aux entraîneurs amateurs, semi-professionnels et professionnels, avec une utilisation individuelle.

### Plateformes et fonctionnement retenus

- Desktop et Android.
- Pas de version iOS prévue dans le périmètre actuel.
- Application utilisable localement, y compris hors connexion.
- Stockage local des données.
- Une installation correspond à un environnement de travail indépendant.
- Pas de compte distant, de synchronisation cloud ni de collaboration temps réel dans le périmètre initial.
- Transfert manuel des données entre Desktop et Android au moyen d'une sauvegarde/restauration : mécanisme exact à décider.
- Base technique cible : Qt 6.9, C++ pour la logique métier, QML pour l'interface, SQLite pour la persistance.

Ces choix constituent la direction produit retenue. Leur conformité au dépôt actuel doit être auditée.

## 3. Utilisateurs et contexte d'utilisation

Utilisateur principal : entraîneur de football individuel qui peut gérer plusieurs clubs et plusieurs équipes.

Profils d'usage visés :
- entraîneur amateur ;
- entraîneur semi-professionnel ;
- entraîneur professionnel.

L'application n'est pas conçue initialement comme un outil collaboratif multi-utilisateurs.

## 4. Modèle métier de référence

### 4.1 Hiérarchie

Le modèle de navigation et de travail validé par l'utilisateur est :

```text
FootCoach/
└── Club sélectionné/
    ├── Changer de club
    └── Équipe sélectionnée/
        ├── Changer d'équipe
        ├── Accueil
        ├── Séances d'entraînement/
        │   ├── Liste des séances
        │   ├── Créer une séance
        │   └── Historique
        ├── Matchs/
        │   ├── Liste des matchs
        │   ├── Préparer un match
        │   └── Historique
        ├── Équipe/
        │   ├── Effectif
        │   ├── Fiches joueurs
        │   └── Catégorie
        ├── Calendrier/
        └── Suivi/
```

Cette arborescence est une hiérarchie fonctionnelle, pas une obligation de reproduire les mêmes dossiers dans le code.

### 4.2 Sens des concepts

- **Club** : structure regroupant plusieurs équipes. Une installation peut gérer plusieurs clubs.
- **Équipe** : groupe régulier de joueurs rattaché à un club, par exemple U15, U17 ou Seniors.
- **Séance d'entraînement** : activité d'entraînement organisée pour une équipe, constituée d'une liste ordonnée d'exercices.
- **Match** : objet métier indépendant d'une séance d'entraînement. Il est associé à une équipe et à un adversaire. Un match n'est jamais un exercice ni un type de séance.
- **Exercice** : unité de travail réutilisable dans la bibliothèque et insérable dans une séance.
- **Terrain/tableau tactique** : outil visuel réutilisable dans plusieurs contextes, notamment un atelier d'exercice et la préparation d'un match. Le moteur graphique peut être partagé ; les données métier des contextes restent distinctes.

### 4.3 Séance et exercice

Une séance est une liste d'exercices. Un exercice comprend au minimum :
- titre ;
- catégorie ;
- objectif ;
- durée ;
- nombre minimum et maximum de joueurs ;
- matériel ;
- description ;
- représentation tactique facultative.

Un exercice peut exister sans représentation tactique. Si elle existe, elle décrit l'organisation de l'atelier.

La représentation tactique est éditée en grand format et présentée en aperçu réduit dans le plan de séance. Le changement d'échelle ou de mode d'affichage ne doit jamais modifier les données tactiques.

### 4.4 Bibliothèque d'exercices

La bibliothèque contient initialement des exercices prédéfinis. L'utilisateur doit pouvoir les personnaliser et créer ses propres exercices.

Il faudra distinguer :
- exercice prédéfini ;
- exercice personnalisé dans la bibliothèque ;
- exercice inséré dans une séance.

La sémantique de copie/référence et le comportement des modifications ultérieures du modèle de bibliothèque ne sont pas encore décidés. Ne pas inventer cette règle.

## 5. Navigation et ergonomie

La navigation doit suivre la logique intellectuelle de l'entraîneur, pas l'organisation interne du code.

Ordre conceptuel :
1. choisir le club ;
2. choisir l'équipe ;
3. travailler dans le contexte de cette équipe ;
4. accéder aux activités et aux données de cette équipe.

Principes confirmés :
- Le club et l'équipe courants doivent être identifiables dans l'interface.
- Le changement d'équipe doit être accessible depuis l'accueil.
- Les rubriques Séances d'entraînement et Matchs sont deux entrées métier distinctes.
- Le retour doit ramener à l'écran réellement précédent, et non à un accueil codé en dur.
- Le retour doit préserver autant que possible le contexte : identifiants, position de défilement, état des minuteries et modifications en cours.
- Le bouton Retour de l'interface et le retour Android doivent suivre la même logique.
- Bibliothèque d'exercices, assistance/contact et paramètres sont transversaux. Leur emplacement exact dans la navigation détaillée reste à finaliser.
- La bibliothèque doit être accessible par un bouton ; elle ne doit pas être perdue dans une rubrique générique « Plus ».
- L'accueil doit proposer un accès au changement d'équipe.

Le nombre et le libellé définitifs des boutons de navigation primaire restent à confirmer. Ne pas déduire leur validation de simples propositions antérieures.

## 6. Référence visuelle UXPilot

Le PDF fourni `uxpilot-export-10-29-26 (2).pdf` présente des concepts d'interface Desktop, tablette Android et mobile Android. C'est une référence visuelle et fonctionnelle, pas une spécification exhaustive ni une preuve d'implémentation.

Éléments relevés :
- Desktop : Dashboard, Squad, Calendar, Attendance, Exercises, Sessions, Tactics, Settings ; aperçu de la prochaine séance, séances à venir, indicateurs d'effectif/présence, meilleurs joueurs et outils tels que Tactical Board et Drill Library.
- Tablette Android : séance du jour, Tactical Board, Upcoming Sessions, Library, Attendance Health, Top Performers et Manage Roster.
- Mobile Android : accueil avec compte à rebours de la prochaine activité, Check-in, New Session, Tactics, résumé des présences, prochaines séances et bouton Exercise Library / Browse Now ; barre basse explicitement libellée HOME, SQUAD, DATE, TRACK, MORE.
- Les maquettes présentent aussi des données fictives (noms, statistiques, dates, version affichée). Elles ne doivent pas être prises pour des données réelles du produit.
- Les compositions Desktop et mobile ne sont pas identiques. La parité fonctionnelle entre plateformes n'impose pas une présentation visuelle identique.

## 7. Périmètre fonctionnel global

Domaines fonctionnels envisagés :
- clubs et équipes ;
- joueurs et effectifs ;
- séances d'entraînement ;
- bibliothèque d'exercices ;
- terrain/tableau tactique ;
- matchs et préparation tactique ;
- calendrier ;
- présences et suivi ;
- assistance et signalement ;
- paramètres ;
- sauvegarde/restauration.

Les domaines avancés (analyse vidéo, statistiques avancées, charge d'entraînement, progression) sont des perspectives de roadmap, pas des exigences déjà spécifiées en détail.

## 8. Hors périmètre initial

- comptes utilisateurs distants ;
- stockage cloud ;
- synchronisation automatique Desktop/Android ;
- collaboration temps réel ;
- gestion multi-utilisateurs simultanée ;
- iOS.

Ne pas introduire ces services ou dépendances sans décision explicite.

## 9. Règles de vérité documentaire

Chaque information de reprise doit être classée :
- **CONFIRMÉE** : explicitement décidée par l'utilisateur ou établie par un état technique rapporté avec preuve.
- **PROPOSITION** : recommandation à soumettre à l'utilisateur.
- **À VÉRIFIER** : affirmation technique non contrôlée directement.
- **NON DÉCIDÉE** : choix qui nécessite une décision utilisateur.
- **VALIDÉE** : conformité démontrée par une vérification reproductible. Un commit seul ne vaut pas validation.

Ne jamais transformer une proposition ou une affirmation d'un agent en décision validée.
