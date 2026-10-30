#pragma once

#include "core/Models.h"

#include <QList>
#include <QSet>
#include <QSqlDatabase>

class SessionRepository
{
public:
    explicit SessionRepository(const QSqlDatabase &db) : m_db(db) {}

    // Une séance par son id ; id < 0 dans le résultat si elle n'existe pas.
    Session get(int id, QString *error = nullptr) const;

    // Séances dont le jour de début est compris entre `from` et `to` (inclus), triées par date.
    QList<Session> listBetween(int teamId, const QDate &from, const QDate &to, QString *error = nullptr) const;
    // Jours (entre `from` et `to` inclus) ayant au moins une séance, tous statuts confondus.
    QSet<QDate> datesWithSessions(int teamId, const QDate &from, const QDate &to) const;

    bool insert(Session &session, QString *error = nullptr);   // renseigne session.id
    // Insertion groupée en une seule transaction : tout ou rien.
    bool insertMany(QList<Session> &sessions, QString *error = nullptr);
    bool update(const Session &session, QString *error = nullptr);
    bool setStatus(int id, const QString &status, QString *error = nullptr);
    // Supprime aussi les présences et le plan d'exercices de la séance (cascade).
    bool remove(int id, QString *error = nullptr);

    // Séances non annulées de l'équipe qui chevauchent [start, start + durationMin[.
    // excludeId : séance en cours d'édition, ignorée.
    QList<Session> conflicts(int teamId, const QDateTime &start, int durationMin, int excludeId = -1) const;

private:
    bool insertOne(Session &session, QString *error);

    QSqlDatabase m_db;
};
