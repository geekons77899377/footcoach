#pragma once

#include "core/Models.h"

#include <QList>
#include <QSqlDatabase>

// Plan d'une séance : liste ordonnée d'exercices (table session_exercise).
class PlanRepository
{
public:
    explicit PlanRepository(const QSqlDatabase &db) : m_db(db) {}

    // Lignes du plan, par position croissante, avec les champs de l'exercice (y compris archivé).
    QList<PlanEntry> entries(int sessionId, QString *error = nullptr) const;
    // Ajoute l'exercice en fin de plan. Renvoie l'id de la ligne, ou -1 en cas d'erreur.
    int add(int sessionId, int exerciseId, int durationMin, const QString &notes = QString(), QString *error = nullptr);
    bool updateItem(int itemId, int durationMin, const QString &notes, QString *error = nullptr);
    // Retire la ligne et recompacte les positions (1, 2, 3…).
    bool remove(int itemId, QString *error = nullptr);
    // delta = -1 : monte d'un cran, +1 : descend. Sans effet (true) aux extrémités.
    bool move(int itemId, int delta, QString *error = nullptr);

private:
    bool renumber(int sessionId, QString *error);

    QSqlDatabase m_db;
};
