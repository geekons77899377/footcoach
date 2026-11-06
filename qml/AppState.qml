pragma Singleton
import QtQuick

// État partagé entre les pages : l'équipe sélectionnée reste la même dans Effectif, Calendrier et Suivi.
QtObject {
    property int teamId: -1
    property real bottomInset: 0          // hauteur à laisser libre en bas des feuilles (barre de navigation + zone sûre)

    // Retour cohérent : quand une page est ouverte depuis une autre (un exercice depuis l'accueil, le plan d'une séance
    // depuis le calendrier, Données depuis Paramètres...), « retour » ramène à la page d'origine, pas à la liste de la
    // page ouverte. returnFor : section ouverte ; returnTo : section d'origine.
    property string returnTo: ""
    property string returnFor: ""
    signal returnRequested(string target)

    // À appeler quand l'utilisateur revient en arrière dans la section : vrai si un retour vers l'origine est demandé.
    function leaveItem(section) {
        if (returnFor !== section || returnTo === "")
            return false;
        var target = returnTo;
        returnTo = "";
        returnFor = "";
        returnRequested(target);
        return true;
    }
    function clearReturn() {
        returnTo = "";
        returnFor = "";
    }
}
