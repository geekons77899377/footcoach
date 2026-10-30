#pragma once

#include "core/Models.h"

#include <QList>
#include <QSqlDatabase>

class TeamRepository
{
public:
    explicit TeamRepository(const QSqlDatabase &db) : m_db(db) {}

    QList<Team> all(QString *error = nullptr) const;
    bool insert(Team &team, QString *error = nullptr);   // renseigne team.id
    bool update(const Team &team, QString *error = nullptr);
    // Supprime l'équipe ET (par cascade) ses joueurs, séances et présences.
    bool remove(int id, QString *error = nullptr);
    int playerCount(int teamId) const;

private:
    QSqlDatabase m_db;
};
