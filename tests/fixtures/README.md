# Fichiers de référence du format `.footcoach`

Ne pas modifier à la main. Ne pas régénérer pour faire passer un test.

| Fichier | Origine |
|---|---|
| `schema4.footcoach` | Export complet réel fait par le code de FootCoach 0.8.0 (commit `aa65676`), champ `app` = `0.8.0`. |
| `schema1.footcoach`, `schema2.footcoach`, `schema3.footcoach` | **Reconstitués** : même code d'export, base créée avec les migrations `001` à `00N`. Aucune version publiée n'a produit de fichier avec ces schémas. Champ `app` = `reconstitue-schema-N`. |

Toutes les données sont fictives.

## Les recréer (action volontaire, après revue)

```
FOOTCOACH_WRITE_FIXTURES=<dossier> ./tst_dataformat writeFixtures
```

Voir `docs/versionnage.md`.
