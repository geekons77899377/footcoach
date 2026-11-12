#pragma once

#include "core/Models.h"

#include <QList>
#include <QSqlDatabase>

// Créneaux réguliers d'entraînement (schéma 10). Les séances générées portent training_session.slot_id.
class TrainingSlotRepository
{
public:
    explicit TrainingSlotRepository(const QSqlDatabase &db) : m_db(db) {}

    TrainingSlot get(int id, QString *error = nullptr) const;        // id < 0 si introuvable
    QList<TrainingSlot> list(int teamId, QString *error = nullptr) const;

    // Ces trois méthodes ne gèrent pas de transaction : l'appelant les place dans la sienne.
    bool insert(TrainingSlot &slot, QString *error = nullptr);       // renseigne slot.id
    bool update(const TrainingSlot &slot, QString *error = nullptr);
    bool remove(int id, QString *error = nullptr);                   // séances restantes : slot_id = NULL

    static QString weekdaysToText(const QList<int> &days);           // « 2,4 »
    static QList<int> weekdaysFromText(const QString &text);         // jours valides, triés, sans doublon

private:
    QSqlDatabase m_db;
};
