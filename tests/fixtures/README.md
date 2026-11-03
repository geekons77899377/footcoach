# Fichiers de référence du format `.footcoach`

Ne pas modifier à la main. Ne pas régénérer pour faire passer un test.

| Fichier | Origine |
|---|---|
| `schema4.footcoach` | Export complet réel fait par le code de FootCoach 0.8.0 (commit `aa65676`), champ `app` = `0.8.0`. |
| `schema5.footcoach` | Export fait par le code courant (0.10.0, branche `v1`) sur une base au schéma 5, avec un terrain rattaché à un bloc. Champ `app` = `0.10.0`. N'est pas un export de version publiée. |
| `schema1.footcoach`, `schema2.footcoach`, `schema3.footcoach` | **Reconstitués** : même code d'export, base créée avec les migrations `001` à `00N`. Aucune version publiée n'a produit de fichier avec ces schémas. Champ `app` = `reconstitue-schema-N`. |

Toutes les données sont fictives.

## Les recréer (action volontaire, après revue)

```
FOOTCOACH_WRITE_FIXTURES=<dossier> ./tst_dataformat writeFixtures
```

La commande écrit les fichiers 1 à 5 dans le dossier indiqué (hors du dépôt). Pour ajouter un fichier de référence, ne copier
que le nouveau ; les fichiers existants ne sont pas remplacés.

Voir `docs/versionnage.md`.
