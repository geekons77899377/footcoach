#pragma once

#include "core/Models.h"

#include <QList>
#include <QSqlDatabase>

class TeamRepository
{
public:
    explicit TeamRepository(const QSqlDatabase &db) : m_db(db) {}

    // Toutes les équipes, ou celles d'un club (clubId > 0).
    QList<Team> all(QString *error = nullptr, int clubId = -1) const;
    // renseigne team.id. Une équipe a toujours un club : si team.clubId <= 0, c'est le premier club ; refus s'il n'y en
    // a aucun ou si le club n'existe pas.
    bool insert(Team &team, QString *error = nullptr);
    // Ne change pas le club (une équipe ne passe pas d'un club à un autre).
    bool update(const Team &team, QString *error = nullptr);
    // Supprime l'équipe ET (par cascade) ses joueurs, séances et présences.
    bool remove(int id, QString *error = nullptr);
    int playerCount(int teamId) const;

private:
    QSqlDatabase m_db;
};
