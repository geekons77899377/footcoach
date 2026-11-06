# OpenSSL pour Android (HTTPS)

Qt pour Android ne fournit pas OpenSSL : sans ces bibliothèques, toute connexion HTTPS échoue
(« No functional TLS backend was found »). Elles servent uniquement à l'envoi des signalements (`IssueReporter`).

- Origine : https://github.com/KDAB/android_openssl, dossier `ssl_3/arm64-v8a` (commit b71f147, janvier 2026), OpenSSL 3.x.
- Licence : OpenSSL 3.x est sous licence Apache 2.0 ; le texte fourni par le dépôt KDAB est dans `LICENSE-android_openssl.txt`.
  À mentionner dans les mentions légales de l'application.
- Alignement des segments : 16 Ko (`p_align = 0x4000`, contrôlé avec `readelf -lW`), conforme à l'exigence Google Play.
- ABI : seul `arm64-v8a` est fourni (appareils actuels, exigé par Google Play). Pour un émulateur x86_64, copier
  `ssl_3/x86_64` de ce dépôt dans `android/openssl/x86_64/`.
- Mise à jour : remplacer les deux fichiers par ceux d'une version plus récente du même dépôt, contrôler l'alignement.
