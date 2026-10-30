# Publication de FootCoach sur Google Play

Compte : **personnel** (création après le 13/11/2023) → test fermé obligatoire avant la production.
Sources : [niveau d'API cible](https://developer.android.com/google/play/requirements/target-sdk),
[test fermé](https://support.google.com/googleplay/android-developer/answer/14151465),
[Qt et pages de 16 Ko](https://www.qt.io/blog/android-15-and-16-support).

## 1. Exigences techniques (état du projet)
| Point | Valeur | Remarque |
|---|---|---|
| Identifiant | `org.footcoach.app` | Ne plus changer après la première publication. |
| `minSdk` / `targetSdk` | 23 / **36** | API 36 exigée pour les nouvelles applications depuis le 31/08/2026. |
| `versionCode` | 12 | À augmenter à chaque envoi sur Google Play. |
| Version | `project(FootCoach VERSION …)` | Source unique : `versionName` en découle. |
| ABI | arm64-v8a **et** armeabi-v7a | Voir §3. |
| Qt | 6.9.3 minimum | Première version 6.9 compatible avec les pages de 16 Ko. |

À vérifier sur l'AAB ou l'APK produit (Release) :
```
aapt2 dump badging app.apk | findstr "package sdkVersion targetSdkVersion uses-permission"
```
Attendu : `targetSdkVersion:'36'` et **aucune** `uses-permission`.

## 2. Clé d'upload (une seule fois, hors dépôt)
```
keytool -genkeypair -v -keystore D:\Secrets\footcoach-upload.jks -alias footcoach -keyalg RSA -keysize 2048 -validity 10000
```
- Choisissez deux mots de passe robustes et notez-les dans un gestionnaire de mots de passe.
- **Sauvegardez le fichier `.jks` ailleurs (clé USB, cloud chiffré).** Le dépôt l'ignore (`*.jks`).
- Activez « Play App Signing » dans Play Console (proposé par défaut) : Google garde la clé de signature
  définitive, la vôtre n'est que la clé d'*upload* et peut être réinitialisée en cas de perte.

## 3. Compilation Release dans Qt Creator
1. Kit **Android Qt 6.9.3 Clang arm64-v8a** ; configuration **Release**.
2. *Projets → Construire → Paramètres de construction Android* : cocher les ABI **arm64-v8a** et **armeabi-v7a**
   (installer le kit armv7 de Qt si besoin via la Maintenance Tool).
3. Étape *Build Android APK* : cocher **Sign package**, choisir le `.jks` et l'alias ; cocher
   **Build Android App Bundle (*.aab)** pour la production.
4. Résultats : `…/android-build-FootCoach/build/outputs/bundle/release/*.aab` (pour Google Play) et
   `…/apk/release/*.apk` (test direct sur appareil : `adb install -r`).
5. Test sur un vrai appareil avant tout envoi : installation de l'APK Release, parcours Tactique, Effectif,
   Calendrier, Appel, Suivi, rotation de l'écran, bouton retour.

## 4. Play Console — création de la fiche (manuel)
1. *Créer une application* : nom, langue par défaut français, application (pas un jeu), gratuite.
2. *Contenu de l'application* : politique de confidentialité (URL publique — voir `docs/confidentialite.md`),
   accès à l'application (aucun identifiant requis), publicité (aucune), classification du contenu (questionnaire),
   public cible (voir remarque ci-dessous), sécurité des données, catégorie « Sports ».
3. *Fiche Play Store* : textes (`store/fiche.md`), icône 512 × 512 (`store/icon-512.png`), image de présentation
   1024 × 500 (à créer), au moins 2 captures d'écran téléphone ; ajoutez des captures tablette 7" et 10".

Remarque public cible : l'application gère des données de joueurs mineurs, mais elle s'adresse à des **entraîneurs
adultes**. Indiquez un public cible adulte ; ne choisissez pas « moins de 13 ans » (cela déclencherait la
politique « Familles »).

## 5. Test interne puis test fermé
1. *Tests → Test interne → Créer une version* : importer l'AAB, notes de version, enregistrer et publier.
   Ajouter jusqu'à 100 testeurs par adresse Gmail, partager le lien d'inscription.
2. *Tests → Test fermé* : créer une piste, importer le même AAB (ou plus récent), ajouter une liste de testeurs
   (adresses Google).
3. **Au moins 12 testeurs inscrits en continu pendant 14 jours** avant la demande de production. Un testeur qui
   quitte avant 14 jours ne compte pas ; s'il revient, les 14 jours doivent être consécutifs. Prévoyez 15 à 20
   testeurs pour avoir de la marge.
4. Pendant les 14 jours : publiez au moins une mise à jour si un défaut est trouvé, relevez les retours.
   L'engagement des testeurs est examiné.
5. *Tableau de bord → Demander l'accès à la production* : répondre au questionnaire (déroulement du test,
   retours reçus, corrections apportées). Délai de revue indiqué : 7 jours ou moins.
6. Après accord : *Production → Créer une version* avec l'AAB, déploiement progressif conseillé.
