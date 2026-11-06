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

    // Clé d'unicité d'un joueur dans son équipe : prénom + nom, sans accents ni majuscules, espaces simplifiés.
    static QString fold(const QString &s);   // sans accents ni majuscules, espaces simplifiés
    static QString nameKey(const QString &firstName, const QString &lastName);
    // Vrai si un autre joueur de l'équipe (actif ou non) a la même clé prénom + nom (excludeId = joueur en cours d'édition).
    bool nameTaken(int teamId, const QString &firstName, const QString &lastName, int excludeId = -1) const;

    // Vrai si un autre joueur ACTIF de l'équipe porte déjà ce numéro (excludeId = joueur en cours d'édition).
    bool jerseyTaken(int teamId, int number, int excludeId = -1) const;

private:
    QSqlDatabase m_db;
};
