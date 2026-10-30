#include "PlanSchedule.h"

namespace PlanSchedule {

QList<Slot> schedule(const QDateTime &sessionStart, const QList<int> &durationsMin)
{
    QList<Slot> timeline;
    timeline.reserve(durationsMin.size());
    QDateTime cursor = sessionStart;
    for (int d : durationsMin) {
        const int minutes = qMax(0, d);
        Slot s;
        s.start = cursor;
        cursor = cursor.addSecs(qint64(minutes) * 60);
        s.end = cursor;
        timeline.append(s);
    }
    return timeline;
}

int totalMinutes(const QList<int> &durationsMin)
{
    int total = 0;
    for (int d : durationsMin)
        total += qMax(0, d);
    return total;
}

} // namespace PlanSchedule
