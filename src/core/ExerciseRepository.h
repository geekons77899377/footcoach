#pragma once

#include "core/Models.h"

#include <QList>
#include <QSqlDatabase>

class ExerciseRepository
{
public:
    explicit ExerciseRepository(const QSqlDatabase &db) : m_db(db) {}

    // Triés par titre. Les exercices archivés ne sont inclus que sur demande.
    QList<Exercise> list(bool includeArchived, QString *error = nullptr) const;
    bool insert(Exercise &exercise, QString *error = nullptr);   // renseigne exercise.id
    bool update(const Exercise &exercise, QString *error = nullptr);
    bool setArchived(int id, bool archived, QString *error = nullptr);
    // Suppression définitive. Refusée (false + message) si l'exercice est utilisé dans au moins un plan.
    bool remove(int id, QString *error = nullptr);
    // Nombre de plans de séance qui utilisent cet exercice.
    int usageCount(int id) const;

private:
    QSqlDatabase m_db;
};
