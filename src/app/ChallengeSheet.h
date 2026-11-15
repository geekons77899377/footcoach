#pragma once

#include <QIODevice>
#include <QList>
#include <QString>

// Une ligne de la « Feuille de challenge 2/2 » : un joueur de l'équipe.
struct ChallengeEntry
{
    QString lastName;
    QString firstName;
    QString licence;
};

// En-tête du bloc « Équipe 1 » : club, éducateur responsable. Un champ vide laisse la case vierge (à remplir à la main).
struct ChallengeHeader
{
    QString teamName;               // titre du PDF seulement (n'est pas imprimé sur la feuille)
    QString clubName;               // case « ÉQUIPE 1 : »
    QString responsibleName;        // « Nom Responsable » : « NOM Prénom » de l'éducateur responsable de l'équipe
    QString responsibleLicence;     // « N° de licence » du responsable (déjà contrôlé : 10 chiffres, ou vide)
};

// Feuille de challenge 2/2 (quatre blocs « Équipe 1 à 4 », comme la feuille papier du district) : seul le bloc
// « Équipe 1 » est rempli (nom du club, une ligne par joueur : n°, nom, prénom, n° de licence, responsable et sa
// licence), les autres restent vierges pour les autres clubs. Colonne « Blessure » laissée vide, à compléter à la main.
//
// Sortie conçue pour l'impression en noir et blanc : une page A4 paysage, traits noirs uniquement, aucun aplat, aucune
// couleur ni nuance de gris, aucun logo ni motif décoratif. Qt Gui (QPdfWriter) : ne pas utiliser depuis src/core.
class ChallengeSheet
{
public:
    static constexpr int kRows = 12;            // lignes par équipe sur la feuille
    static constexpr int kLicenceDigits = 10;   // un n° de licence est fait de 10 chiffres

    // Vrai si « licence » (espaces ignorés) est faite d'exactement kLicenceDigits chiffres.
    static bool isValidLicence(const QString &licence);

    // Écrit le PDF (une page). entries : kRows au plus, dans l'ordre d'affichage. false + message si impossible.
    static bool writePdf(QIODevice *out, const ChallengeHeader &header, const QList<ChallengeEntry> &entries,
                         QString *error);
};
