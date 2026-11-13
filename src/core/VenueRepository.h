#pragma once

#include "core/Models.h"

#include <QJsonArray>
#include <QList>
#include <QSqlDatabase>
#include <QUrl>

// Schéma 11 : lieux de match (table venue) d'un club, stade retenu pour chaque adversaire (opponent_venue).
// Les terrains « data_es » viennent du recensement des équipements sportifs (Data ES, ministère des Sports) ; ils ne sont
// jamais supprimés par une mise à jour, seulement marqués « removed » ou « closed ».
class VenueRepository
{
public:
    explicit VenueRepository(const QSqlDatabase &db) : m_db(db) {}

    // Bilan d'une mise à jour depuis Data ES.
    struct RefreshStats
    {
        int total = 0;      // terrains reçus
        int added = 0;      // nouveaux
        int updated = 0;    // déjà connus, encore disponibles dans Data ES
        int closed = 0;     // reçus mais hors service (inst_hs_bool = « Oui »)
        int removed = 0;    // connus mais absents de la réponse : passent à « removed »
        int reopened = 0;   // étaient « removed » ou « closed », de nouveau en service
    };

    // Lieux du club, par commune puis nom. includeUnselectable = false : seulement ceux qu'on peut choisir pour un
    // nouveau match (ni indisponibles, ni retirés, ni hors service).
    QList<Venue> list(int clubId, bool includeUnselectable, QString *error = nullptr) const;
    bool get(int id, Venue *out, QString *error = nullptr) const;
    // Stade du club (is_home), id -1 s'il n'y en a pas.
    Venue home(int clubId, QString *error = nullptr) const;

    // Lieu saisi à la main : création (id < 0) ou modification. Refus si le nom est vide, si le club n'existe pas, ou
    // pour modifier un terrain Data ES (seuls « indisponible » et « stade du club » se changent sur ceux-là).
    bool saveManual(Venue &venue, QString *error = nullptr);
    // Supprime un lieu saisi à la main (les matchs gardent le texte du lieu). Refus pour un terrain Data ES.
    bool removeManual(int id, QString *error = nullptr);
    bool setUnavailable(int id, bool unavailable, QString *error = nullptr);
    // Stade du club : venueId <= 0 retire le stade du club. Refus si le lieu n'est pas de ce club.
    bool setHome(int clubId, int venueId, QString *error = nullptr);

    // Applique la réponse de Data ES (tableau d'enregistrements de /exports/json) aux terrains du club, en une
    // transaction. Refus (rien n'est modifié) si la réponse est vide ou mal formée : une réponse vide marquerait
    // sinon tous les terrains comme retirés.
    bool applyDataEs(int clubId, const QJsonArray &records, RefreshStats *stats, QString *error = nullptr);
    // Date de la dernière mise à jour Data ES du club (« yyyy-MM-dd HH:mm »), vide si jamais faite.
    QString lastRefresh(int clubId) const;

    // Stade retenu pour un adversaire du club, -1 s'il n'y en a pas.
    int opponentVenue(int clubId, const QString &opponent) const;
    bool rememberOpponentVenue(int clubId, const QString &opponent, int venueId, QString *error = nullptr);

    // Nom d'adversaire normalisé : minuscules, sans accents ni ponctuation, espaces réduits (« U.S. Nanteuil » et
    // « us nanteuil » donnent la même clé).
    static QString opponentKey(const QString &opponent);
    // Code de département accepté par Data ES : 01 à 95 (sauf 20), 2A, 2B, 971 à 976.
    static bool isValidDepartment(const QString &department);
    // Adresse de l'appel Data ES : terrains de football du département, champs utiles seulement.
    static QUrl dataEsUrl(const QString &department);

private:
    QSqlDatabase m_db;
};
