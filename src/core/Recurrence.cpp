#include "Recurrence.h"

namespace Recurrence {

QList<QDateTime> occurrences(const QDate &from, const QDate &to, const QSet<int> &weekdays, const QTime &time)
{
    QList<QDateTime> result;
    if (!from.isValid() || !to.isValid() || !time.isValid() || to < from || weekdays.isEmpty())
        return result;

    for (QDate d = from; d <= to; d = d.addDays(1)) {
        if (!weekdays.contains(d.dayOfWeek()))
            continue;
        result.append(QDateTime(d, time));
        if (result.size() > kMaxOccurrences)
            break;
    }
    return result;
}

} // namespace Recurrence
