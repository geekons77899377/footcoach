#include "AttendanceStats.h"

namespace AttendanceStats {

QString cellStatus(const Data &data, const PlayerRef &player, const SessionRef &session)
{
    const auto it = data.rows.constFind({session.id, player.id});
    if (it != data.rows.constEnd())
        return it.value();

    if (player.active && session.date >= player.joined)
        return QStringLiteral("absent");   // pas d'appel enregistré pour lui = absent
    return QString();
}

QList<PlayerStats> compute(const Data &data)
{
    QList<PlayerStats> result;
    result.reserve(data.players.size());
    for (const PlayerRef &p : data.players) {
        PlayerStats st;
        st.playerId = p.id;
        for (const SessionRef &s : data.sessions) {
            const QString status = cellStatus(data, p, s);
            if (status == QLatin1String("present"))
                ++st.present;
            else if (status == QLatin1String("absent"))
                ++st.absent;
            else if (status == QLatin1String("excused"))
                ++st.excused;
            else if (status == QLatin1String("injured"))
                ++st.injured;
        }
        result.append(st);
    }
    return result;
}

} // namespace AttendanceStats
