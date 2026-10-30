#pragma once

#include "core/AttendanceStats.h"

#include <QList>
#include <QMap>
#include <QSqlDatabase>

class AttendanceRepository
{
public:
    struct Entry
    {
        int playerId = -1;
        QString status = QStringLiteral("absent");   // "present", "absent", "excused", "injured"
        QString note;
    };

    explicit AttendanceRepository(const QSqlDatabase &db) : m_db(db) {}

    // Appel enregistré d'une séance, indexé par id de joueur.
    QMap<int, Entry> load(int sessionId, QString *error = nullptr) const;
    // Remplace tout l'appel de la séance (une seule transaction : tout ou rien).
    bool save(int sessionId, const QList<Entry> &entries, QString *error = nullptr);

    // Pour chaque séance : (nombre de présents, nombre de joueurs dans l'appel). Absent du résultat = pas d'appel.
    QMap<int, QPair<int, int>> summary(const QList<int> &sessionIds) const;

    // Données brutes des séances réalisées de l'équipe entre `from` et `to` (inclus).
    AttendanceStats::Data loadData(int teamId, const QDate &from, const QDate &to, QString *error = nullptr) const;

private:
    QSqlDatabase m_db;
};
