package org.footcoach.app;

import android.app.Activity;
import android.content.ClipData;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import androidx.core.content.FileProvider;
import java.io.File;

// Partage d'un fichier (export .footcoach, feuille de challenge PDF) par la feuille Android. Appelée depuis C++
// (Backend::shareAndroid).
// Retourne null en cas de succès, sinon le motif de l'échec à afficher.
public final class ShareBridge {
    private ShareBridge() {}

    public static String partager(Activity activite, String chemin, String titre, String mime) {
        try {
            Uri uri = FileProvider.getUriForFile(activite, activite.getPackageName() + ".exports", new File(chemin));
            Intent envoi = new Intent(Intent.ACTION_SEND);
            envoi.setType(mime);
            envoi.putExtra(Intent.EXTRA_STREAM, uri);
            // Le droit de lecture doit suivre l'URI jusqu'à l'application choisie : ClipData + drapeau sur
            // l'intent ET sur le sélecteur.
            envoi.setClipData(ClipData.newRawUri("", uri));
            envoi.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            Intent selecteur = Intent.createChooser(envoi, titre);
            selecteur.setClipData(ClipData.newRawUri("", uri));
            selecteur.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            // Droit explicite pour chaque application qui peut ouvrir l'envoi (Android 11+ : voir <queries>),
            // et pour le paquet du sélecteur, qui ouvre le fournisseur pour les aperçus.
            PackageManager pm = activite.getPackageManager();
            for (Intent intention : new Intent[] {envoi, selecteur}) {
                for (ResolveInfo cible : pm.queryIntentActivities(intention, PackageManager.MATCH_DEFAULT_ONLY)) {
                    activite.grantUriPermission(cible.activityInfo.packageName, uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
                }
            }
            activite.grantUriPermission("com.android.intentresolver", uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
            activite.startActivity(selecteur);
            return null;
        } catch (IllegalArgumentException e) {
            return "Fichier introuvable pour le partage : " + e.getMessage();
        } catch (RuntimeException e) {
            return "Partage impossible : " + e.getMessage();
        }
    }
}
