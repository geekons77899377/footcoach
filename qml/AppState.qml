pragma Singleton
import QtQuick

// État partagé entre les pages : l'équipe sélectionnée reste la même dans Effectif, Calendrier et Suivi.
QtObject {
    property int teamId: -1
    property real bottomInset: 0          // hauteur à laisser libre en bas des feuilles (barre de navigation + zone sûre)
}
