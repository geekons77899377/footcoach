#pragma once

#include <QDate>
#include <QHash>
#include <QList>
#include <QPair>
#include <QString>

// Calcul des présences, sans accès base : testable seul.
//
// Règles :
//  * seules les séances « réalisées » comptent ;
//  * un joueur sans statut enregistré pour une séance compte comme ABSENT, mais seulement si
//    le joueur est actif et que la séance a eu lieu à partir du jour où il a été ajouté à l'effectif
//    (sinon un joueur arrivé en cours de saison serait pénalisé pour des séances qu'il n'a pas pu faire) ;
//  * un statut enregistré explicitement compte toujours ;
//  * taux = présents ÷ (présents + absents) : les excusés et les blessés sortent du dénominateur.
namespace AttendanceStats {

struct SessionRef
{
    int id = -1;
    QDate date;
};

struct PlayerRef
{
    int id = -1;
    QString lastName;
    QString firstName;
    int jerseyNumber = 0;
    QDate joined;           // jour d'ajout à l'effectif
    bool active = true;

    QString fullName() const { return QStringLiteral("%1 %2").arg(lastName, firstName).trimmed(); }
};

struct Data
{
    QList<SessionRef> sessions;                      // séances réalisées, par date croissante
    QList<PlayerRef> players;
    QHash<QPair<int, int>, QString> rows;            // (sessionId, playerId) -> statut enregistré
};

struct PlayerStats
{
    int playerId = -1;
    int present = 0;
    int absent = 0;
    int excused = 0;
    int injured = 0;

    int counted() const { return present + absent + excused + injured; }
    int denominator() const { return present + absent; }
    // Taux de présence entre 0 et 1 ; -1 s'il n'y a rien à calculer.
    double rate() const { return denominator() > 0 ? double(present) / denominator() : -1.0; }
};

// "present", "absent", "excused", "injured", ou chaîne vide si la séance ne concerne pas ce joueur.
QString cellStatus(const Data &data, const PlayerRef &player, const SessionRef &session);

// Une entrée par joueur de data.players, dans le même ordre.
QList<PlayerStats> compute(const Data &data);

} // namespace AttendanceStats
