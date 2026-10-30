#pragma once

#include "core/Models.h"

#include <QList>
#include <QSqlDatabase>

class PlayerRepository
{
public:
    explicit PlayerRepository(const QSqlDatabase &db) : m_db(db) {}

    QList<Player> list(int teamId, bool includeInactive, QString *error = nullptr) const;
    bool insert(Player &player, QString *error = nullptr);   // renseigne player.id
    bool update(const Player &player, QString *error = nullptr);
    bool setActive(int id, bool active, QString *error = nullptr);
    // Suppression définitive : efface aussi présences et blessures (cascade).
    bool remove(int id, QString *error = nullptr);

    // Vrai si un autre joueur ACTIF de l'équipe porte déjà ce numéro (excludeId = joueur en cours d'édition).
    bool jerseyTaken(int teamId, int number, int excludeId = -1) const;

private:
    QSqlDatabase m_db;
};
