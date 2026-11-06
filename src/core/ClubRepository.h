#pragma once

#include "core/Models.h"

#include <QList>
#include <QSqlDatabase>

// Clubs (table club). Une équipe appartient toujours à un club (voir TeamRepository::insert).
class ClubRepository
{
public:
    explicit ClubRepository(const QSqlDatabase &db) : m_db(db) {}

    QList<Club> all(QString *error = nullptr) const;
    bool get(int id, Club *out, QString *error = nullptr) const;
    bool insert(Club &club, QString *error = nullptr);       // renseigne club.id ; refus si le nom est vide
    bool update(const Club &club, QString *error = nullptr);
    // Supprime le club ET (par cascade) ses équipes, joueurs, séances, matchs et présences. Refusé s'il n'existe
    // pas, ou s'il est le dernier club (l'application a toujours au moins un club).
    bool remove(int id, QString *error = nullptr);
    int teamCount(int clubId) const;

private:
    QSqlDatabase m_db;
};
