#pragma once

#include <QDateTime>
#include <QList>

namespace PlanSchedule {

struct Slot
{
    QDateTime start;
    QDateTime end;
};

// Horaires successifs des exercices d'un plan : le premier commence à `sessionStart`,
// chacun suit le précédent sans pause. Une durée <= 0 donne un créneau vide.
QList<Slot> schedule(const QDateTime &sessionStart, const QList<int> &durationsMin);

int totalMinutes(const QList<int> &durationsMin);

} // namespace PlanSchedule
