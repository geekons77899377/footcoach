#pragma once

#include "core/Models.h"

#include <QList>
#include <QSqlDatabase>

// Schéma 12 : éducateurs d'un club (table educator) et éducateur responsable d'une équipe (team.educator_id).
class EducatorRepository
{
public:
    explicit EducatorRepository(const QSqlDatabase &db) : m_db(db) {}

    // Éducateurs du club, triés par nom puis prénom.
    QList<Educator> list(int clubId, QString *error = nullptr) const;
    bool get(int id, Educator *out, QString *error = nullptr) const;
    // Création (educator.id <= 0, renseigne l'id) ou modification (prénom, nom, licence ; le club ne change pas).
    // Refus : prénom ou nom vide, club introuvable, licence déjà prise par un autre éducateur du club.
    bool save(Educator &educator, QString *error = nullptr);
    // Supprime l'éducateur ; ses équipes n'ont plus de responsable (ON DELETE SET NULL).
    bool remove(int id, QString *error = nullptr);
    // Nombre d'équipes dont il est le responsable.
    int teamCount(int educatorId) const;
    // educatorId <= 0 : aucun responsable. Refus si l'éducateur n'est pas du club de l'équipe.
    bool setTeamEducator(int teamId, int educatorId, QString *error = nullptr);
    // Responsable de l'équipe ; false (sans erreur) si elle n'en a pas.
    bool ofTeam(int teamId, Educator *out) const;

private:
    QSqlDatabase m_db;
};
