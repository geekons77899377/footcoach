# Versionnage des données de FootCoach

Ce document fixe les règles de versionnage du schéma de la base et du fichier d'échange `.footcoach`.
Il décrit l'état réel du code à la version 0.8.0 (« Actuel »), puis les règles retenues pour la suite
(« Décidé »). Rien de ce qui est décrit comme « décidé » n'est encore implémenté.

## 1. Quatre numéros à ne pas confondre

| Numéro | Où | Qui le change | Exemple |
|---|---|---|---|
| Version de l'application | `project(FootCoach VERSION …)` dans CMake, journal des modifications, tag Git `vX.Y.Z` | chaque publication | 0.8.0 |
| `versionCode` Android | `QT_ANDROID_VERSION_CODE` | chaque publication (+1 au moins) | 14 |
| **Schéma de données** | `PRAGMA user_version` de la base SQLite, un fichier `sql/NNN_nom.sql` par version | chaque changement de structure ou de contenu de référence | 4 |
| **Version du format d'échange** | champ `version` du fichier `.footcoach` | seulement si la **forme du fichier** change | 1 |

Un cinquième numéro existe, interne : `version` dans le JSON `scene_json` d'un schéma tactique
(`TacticalBoard`). Il est stocké comme du texte dans une colonne de la base ; le format d'échange ne le lit pas.
Un changement de ce JSON ne change donc ni le schéma de données ni le format d'échange, mais il doit
rester lisible par les anciens schémas enregistrés (`TacticalBoard` s'en charge).

Le champ `schema` du fichier recopie `user_version` de la base qui a produit l'export.

## 2. Format d'échange version 1 (Actuel, figé)

```json
{
  "format": "footcoach-data",
  "version": 1,
  "schema": 4,
  "app": "0.8.0",
  "exportedAt": "2026-10-30T19:06:43",
  "tables": {
    "team": { "columns": ["id", "name", …], "rows": [[1, "…", …], …] },
    "player": …, "training_session": …, "attendance": …,
    "exercise": …, "session_exercise": …, "tactical_board": …, "injury": …
  }
}
```

- Un export **complet** contient toujours les huit tables, même vides.
- Valeurs : texte, nombre ou `null` ; un BLOB s'écrit `{"$b64": "…"}`.
- Les colonnes sont celles de la base qui a exporté. Les lignes suivent l'ordre de ces colonnes.
- L'import **remplace tout** (sauvegarde automatique avant, une seule transaction, tout ou rien).

**Cette structure ne change pas.** Le test `currentExportKeepsTheV1Structure` compare un export actuel
au fichier de référence de la 0.8.0 (clés de premier niveau, version, schéma, colonnes de chaque table).

### Ce que fait le lecteur actuel (0.8.0), vérifié par les tests

| Cas | Résultat |
|---|---|
| `format` différent de `footcoach-data` | refusé |
| `version` différente de 1 (0, 2, absente) | refusé : « Version de fichier non prise en charge : mettez FootCoach à jour » |
| `schema` supérieur à celui de l'appareil | refusé : « Ce fichier vient d'une version plus récente de FootCoach » |
| `schema` absent ou inférieur à 1 | refusé, **avec le même message « version plus récente »** (trompeur : à corriger plus tard) |
| une des huit tables absente | refusé : « Fichier incomplet » |
| colonne inconnue, ligne trop courte, valeur illisible | refusé, rien n'est modifié |
| clé en double, contrainte `CHECK` ou clé étrangère violée | refusé, transaction annulée, données inchangées |
| `schema` inférieur à celui de l'appareil | **accepté** : les colonnes absentes du fichier prennent la valeur par défaut de la base |
| clé inconnue au premier niveau (`kind`, `scope`…) | **ignorée** |
| huit tables présentes mais vides | accepté : c'est l'export complet d'une base vide, il remplace tout |

Les deux dernières lignes expliquent la règle suivante.

## 3. Règles décidées

1. **Les exports complets restent en `version: 1`**, avec la structure ci-dessus, tant que leur forme ne change pas.
   Un export complet fait par une version future reste lisible par la 0.8.0 tant que son `schema` n'est pas plus
   récent que celui de l'appareil qui le lit (sinon l'appareil demande de mettre l'application à jour).
2. **Un export partiel ne doit jamais être lisible comme un remplacement complet par une ancienne version.**
   La 0.8.0 ignore les clés inconnues et traite toute `version: 1` comme un export complet : un fichier partiel
   écrit en version 1 (par exemple huit tables dont certaines vides) effacerait les données de l'appareil.
   Les exports partiels (bibliothèque d'exercices, une équipe) utilisent donc `version: 2`, avec :
   - `kind` : `selection` ;
   - `scope` : ce que le fichier contient (`exercises`, `team`) ;
   - `anonymized` : `true` ou `false` (voir §6).

   La 0.8.0 les refuse (« mettez FootCoach à jour »). Une version qui comprend `version: 2` ne remplace jamais
   les données avec un fichier de `kind: selection`.
3. **Un changement de `version` n'est justifié que si la forme du fichier change** (nouvelle clé obligatoire,
   nouvelle représentation des valeurs, table renommée). Ajouter une colonne à la base ne change pas la version
   du format : elle change le schéma.
4. **Refuser plus récent.** Un fichier dont le `schema` ou la `version` dépasse ce que l'application connaît est
   refusé avec un message explicite qui dit quoi faire (mettre l'application à jour sur cet appareil), sans toucher aux
   données. Un schéma absent ou invalide aura son propre message.
5. **Accepter plus ancien**, y compris tous les schémas 1 à 4, par une mise à niveau explicite (§5).

## 4. Chaque changement de schéma de données

Un changement de structure ou de contenu de référence = **une nouvelle migration**, jamais une modification d'une
migration déjà publiée.

1. Ajouter `sql/NNN_nom.sql` (NNN = schéma suivant) et le déclarer dans `SQL_MIGRATIONS` (`CMakeLists.txt`).
   Contraintes du moteur : un statement par `;`, pas de `;` ni de `--` dans les textes.
2. Dire dans le journal des modifications : « schéma N ».
3. Ajouter à `tests/fixtures/` un fichier de référence du schéma N (§7) et l'ajouter au test.
4. Décider si les valeurs par défaut suffisent pour importer un fichier du schéma N‑1 (§5) ; si non, ajouter la mise à
   niveau et son test dans la même modification.
5. Mettre à jour le tableau du §8.
6. Au moment de la publication de la version : tag Git annoté `vX.Y.Z` sur le commit de publication.
   Les tags sont créés **après confirmation** de la version réellement distribuée (voir §9).

Ne jamais : supprimer ou renommer une colonne dans une migration sans mise à niveau d'import ; modifier une migration
déjà livrée ; réutiliser un numéro de schéma.

## 5. Importer un fichier plus ancien

Aujourd'hui, un fichier de schéma inférieur est importé tel quel : chaque colonne absente prend la valeur par défaut
de la base. Cela suffit tant que les migrations **ajoutent** des colonnes avec une valeur par défaut raisonnable
(c'est le cas de 002, 003 et 004).

Une mise à niveau explicite est **obligatoire** quand une migration :
- renomme ou supprime une colonne (sinon : « colonne inconnue » et l'ancien fichier devient illisible) ;
- change le sens ou le format d'une valeur ;
- ajoute une colonne dont la valeur doit être calculée (pas une constante) ;
- ajoute une table obligatoire (les anciens fichiers ne la contiennent pas ; aujourd'hui une table absente est refusée).

Mécanisme décidé : une fonction par transition `N → N+1`, appliquée au JSON du fichier **avant** l'écriture, enchaînées
de `schema du fichier` jusqu'au schéma de l'appareil. Chaque transition est testée avec le fichier de référence du
schéma N. Rien n'est enchaîné pour un fichier de schéma supérieur à celui de l'appareil.

## 6. Types d'export et d'import (à implémenter, non commencé)

| Export | Format | Contenu |
|---|---|---|
| Copie complète | version 1 (inchangé) | toutes les tables |
| Bibliothèque d'exercices | version 2, `scope: exercises` | `exercise` (non archivés par défaut) |
| Une équipe | version 2, `scope: team` | l'équipe, ses joueurs, séances, plans, présences, blessures (selon l'option), schémas liés |

| Import | Disponible pour |
|---|---|
| Remplacer tout (sauvegarde automatique) | copie complète (version 1) uniquement |
| Ajouter sans effacer | **bibliothèque d'exercices uniquement** dans la première version ; équipes et joueurs : pas de fusion |

Ajouter sans effacer, pour les exercices : les identifiants du fichier ne sont jamais réutilisés (ils sont renumérotés) ;
un exercice du fichier ayant le même titre et la même catégorie qu'un exercice existant est ignoré (avec un décompte
affiché avant validation). À préciser avant de coder : comparaison du titre sans tenir compte de la casse et des
espaces en bout de chaîne ; que faire d'un exercice de même titre mais de description différente.

### Données personnelles retirées par l'option d'anonymisation

L'option s'appelle **« sans données personnelles »**, pas « anonyme » : elle ne garantit pas l'anonymat (voir plus bas).
Inventaire table par table, d'après le schéma 4 :

| Table | Colonne | Traitement proposé |
|---|---|---|
| `player` | `first_name`, `last_name` | remplacés par « Joueur 1 », « Joueur 2 »… |
| `player` | `birth_date` | supprimée (ou année seule, au choix à décider) |
| `player` | `phone`, `email` | supprimés |
| `player` | `notes` | supprimées (texte libre : peut contenir n'importe quoi) |
| `player` | `jersey_number`, `position`, `preferred_foot` | conservés (nécessaires à la compo) mais **quasi‑identifiants** |
| `injury` | toute la table | supprimée (donnée de santé, rattachée à un joueur) |
| `attendance` | `note` | supprimée (texte libre) |
| `attendance` | `status = 'injured'` | remplacé par `absent` (révèle une blessure) |
| `team` | `name`, `season` | `name` remplacé par « Équipe 1 » ; `category` et `season` conservées ou supprimées (à décider) |
| `training_session` | `location` | supprimé (peut désigner le stade du club) |
| `training_session` | `theme`, `notes` | `notes` supprimées (texte libre) ; `theme` conservé (contenu sportif) |
| `training_session` | `start_at` | conservé (nécessaire) : les dates restent des quasi‑identifiants |
| `session_exercise` | `notes` | supprimées (texte libre) |
| `exercise` | tous les champs | conservés : le contenu des exercices est ce qu'on veut partager, mais `description` et `equipment` sont du texte libre écrit par l'utilisateur |
| `tactical_board` | `title`, `scene_json` | `label` des éléments et éléments « texte » peuvent contenir des prénoms : supprimés ou vidés |
| `tactical_board` | `thumbnail` | supprimée (image rendue, qui peut montrer ces libellés) |
| en-tête | `app`, `exportedAt` | conservés (aucune donnée personnelle) |

Limites qui empêchent de parler d'anonymat garanti :
- les champs de texte libre (exercices, thèmes, libellés de schémas) ne sont pas contrôlables ;
- les quasi‑identifiants restants (équipe, catégorie, dates de séance, numéros de maillot, postes) permettent, pour un
  petit club, de retrouver une personne en les recoupant avec d'autres informations ;
- les joueurs mineurs imposent une prudence particulière.

L'écran d'export devra le dire en une phrase ; le fichier porte `anonymized: true` seulement comme indication de
l'option choisie, pas comme garantie.

## 7. Fichiers de référence et tests

Dossier `tests/fixtures/`, test `tests/tst_dataformat.cpp`. Voir aussi `tests/fixtures/README.md`.

- `schema4.footcoach` : export complet réel fait par le code de la version **0.8.0** (commit `aa65676`).
- `schema1.footcoach` à `schema3.footcoach` : **reconstitués**. Aucune version publiée n'a écrit de fichier
  `.footcoach` avec ces schémas (le format est né avec le schéma 4). Ils sont produits par le même code d'export
  sur une base créée avec les vraies migrations `001` à `00N`, remplie de données fictives. Ils prouvent que le
  lecteur actuel sait importer ces schémas ; ils ne sont pas des témoins historiques.

Le test vérifie, pour chacun : structure de la version 1, import sur la base courante (comptes, valeurs ligne par
ligne, accents, apostrophes, `NULL`, BLOB, plan de séance, présences, blessures, schéma tactique), colonne
`archived` à sa valeur par défaut pour les schémas 1 et 2, idempotence d'un second import. Il vérifie aussi seize
fichiers invalides (tableau du §2) : le message d'erreur est non vide et les données sont **identiques** avant et après.

Un fichier de référence n'est jamais régénéré pour faire passer un test.

## 8. Historique des schémas

| Schéma | Migration | Contenu | Présent dans l'historique Git depuis | Fichier de référence |
|---|---|---|---|---|
| 1 | `001_init.sql` | tables initiales | `3b9b2b0` (0.3.0) | reconstitué |
| 2 | `002_player_jersey_unique.sql` | numéro de maillot unique par équipe (index) | `3b9b2b0` (0.3.0) | reconstitué |
| 3 | `003_exercise_archived.sql` | `exercise.archived` | `3b9b2b0` (0.3.0) | reconstitué |
| 4 | `004_seed_exercises.sql` | exercices de départ | `3b9b2b0` (0.3.0) | réel (0.8.0) |

Les quatre migrations apparaissent dès le premier commit du dépôt : l'historique Git ne permet pas de dire dans quelle
version de l'application chaque schéma est né, ni lesquels ont été installés sur un appareil. Seul le schéma 4 est
attesté par un export réel (0.8.0).

## 9. Tags Git

- Tag annoté `vX.Y.Z` sur le commit de la version, avec dans le message le schéma de données et la version du format.
- Premier tag publié confirmé : `v0.8.0` (commit `aa65676`, schéma 4, format 1).
- Les tags `v0.5.3`, `v0.6.0`, `v0.7.0` ne sont **pas** créés : les commits sont identifiés (`b93c81e`, `0286ea7`,
  `e11165d`) mais les versions réellement distribuées ne sont pas confirmées.
- Un tag est créé après confirmation, jamais automatiquement.
