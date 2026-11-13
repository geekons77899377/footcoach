package org.footcoach.app;

import androidx.core.content.FileProvider;

// Fournisseur dédié aux exports (autorité ${applicationId}.exports). Sous-classe distincte de FileProvider :
// deux <provider> partageant la même classe se confondent dans le gestionnaire de paquets, et le partage échoue
// avec « Permission Denial » sur le sélecteur.
public class ExportFileProvider extends FileProvider {
}
