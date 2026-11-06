#include "Backend.h"

#include <QDate>
#include <QHash>

namespace {

const QString kDay = QStringLiteral("yyyy-MM-dd");

QVariantMap result(bool ok, int id = -1, const QString &error = QString())
{
    QVariantMap m;
    m.insert(QStringLiteral("ok"), ok);
    m.insert(QStringLiteral("id"), id);
    m.insert(QStringLiteral("error"), error);
    return m;
}

QVariantMap matchMap(const Match &m, int convoked = 0, bool hasBoard = false)
{
    QVariantMap r;
    r.insert(QStringLiteral("id"), m.id);
    r.insert(QStringLiteral("teamId"), m.teamId);
    r.insert(QStringLiteral("date"), m.startAt.date().toString(kDay));
    r.insert(QStringLiteral("time"), m.startAt.time().toString(QStringLiteral("HH:mm")));
    r.insert(QStringLiteral("opponent"), m.opponent);
    r.insert(QStringLiteral("location"), m.location);
    r.insert(QStringLiteral("homeAway"), m.homeAway);
    r.insert(QStringLiteral("status"), m.status);
    r.insert(QStringLiteral("scoreFor"), m.scoreFor);
    r.insert(QStringLiteral("scoreAgainst"), m.scoreAgainst);
    r.insert(QStringLiteral("notes"), m.notes);
    r.insert(QStringLiteral("convoked"), convoked);
    r.insert(QStringLiteral("hasBoard"), hasBoard);
    return r;
}

} // namespace

QVariantList Backend::matches(int teamId, const QString &from, const QString &to)
{
    QVariantList list;
    const QDate d0 = QDate::fromString(from, kDay), d1 = QDate::fromString(to, kDay);
    if (!d0.isValid() || !d1.isValid())
        return list;
    QString error;
    const QList<Match> all = m_matches.listBetween(teamId, d0, d1, &error);
    setError(error);
    for (const Match &m : all) {
        BoardRecord b;
        list.append(matchMap(m, m_matches.players(m.id).size(), m_repo.forMatch(m.id, &b)));
    }
    return list;
}

QStringList Backend::matchDates(int teamId, const QString &from, const QString &to)
{
    QStringList out;
    const QDate d0 = QDate::fromString(from, kDay), d1 = QDate::fromString(to, kDay);
    if (!d0.isValid() || !d1.isValid())
        return out;
    for (const QDate &d : m_matches.datesWithMatches(teamId, d0, d1))
        out.append(d.toString(kDay));
    out.sort();
    return out;
}

QVariantMap Backend::match(int id)
{
    QString error;
    const Match m = m_matches.get(id, &error);
    if (m.id < 0) {
        const QString msg = error.isEmpty() ? tr("Match introuvable.") : error;
        setError(msg);
        QVariantMap r = result(false, id, msg);
        return r;
    }
    QHash<int, Player> byId;
    for (const Player &p : m_players.list(m.teamId, true, &error))
        byId.insert(p.id, p);
    QVariantList players;
    for (const MatchPlayer &mp : m_matches.players(id, &error)) {
        const Player p = byId.value(mp.playerId);
        QVariantMap e;
        e.insert(QStringLiteral("playerId"), mp.playerId);
        e.insert(QStringLiteral("name"), p.fullName());
        e.insert(QStringLiteral("jerseyNumber"), p.jerseyNumber);
        e.insert(QStringLiteral("role"), mp.role);
        players.append(e);
    }
    BoardRecord b;
    const bool hasBoard = m_repo.forMatch(id, &b, &error);
    setError(error);
    QVariantMap r = result(true, id);
    r.insert(QStringLiteral("match"), matchMap(m, players.size(), hasBoard));
    r.insert(QStringLiteral("players"), players);
    r.insert(QStringLiteral("boardId"), hasBoard ? b.id : -1);
    return r;
}

QVariantMap Backend::saveMatch(const QVariantMap &in)
{
    Match m;
    m.id = in.value(QStringLiteral("id"), -1).toInt();
    m.teamId = in.value(QStringLiteral("teamId"), -1).toInt();
    const QDate day = QDate::fromString(in.value(QStringLiteral("date")).toString(), kDay);
    const QTime time = QTime::fromString(in.value(QStringLiteral("time")).toString(), QStringLiteral("HH:mm"));
    if (day.isValid() && time.isValid())
        m.startAt = QDateTime(day, time);
    m.opponent = in.value(QStringLiteral("opponent")).toString().trimmed();
    m.location = in.value(QStringLiteral("location")).toString().trimmed();
    m.homeAway = in.value(QStringLiteral("homeAway"), QStringLiteral("home")).toString();
    m.status = in.value(QStringLiteral("status"), QStringLiteral("planned")).toString();
    m.scoreFor = in.value(QStringLiteral("scoreFor"), -1).toInt();
    m.scoreAgainst = in.value(QStringLiteral("scoreAgainst"), -1).toInt();
    m.notes = in.value(QStringLiteral("notes")).toString().trimmed();
    QString error;
    const bool ok = m.id > 0 ? m_matches.update(m, &error) : m_matches.insert(m, &error);
    setError(error);
    return ok ? result(true, m.id) : result(false, -1, error);
}

QVariantMap Backend::deleteMatch(int id)
{
    QString error;
    const bool ok = m_matches.remove(id, &error);
    setError(error);
    return result(ok, id, error);
}

QVariantMap Backend::setMatchPlayers(int matchId, const QVariantList &entries)
{
    QList<MatchPlayer> list;
    for (const QVariant &v : entries) {
        const QVariantMap e = v.toMap();
        MatchPlayer p;
        p.playerId = e.value(QStringLiteral("playerId"), -1).toInt();
        p.role = e.value(QStringLiteral("role"), QStringLiteral("starter")).toString();
        list.append(p);
    }
    QString error;
    const bool ok = m_matches.setPlayers(matchId, list, &error);
    setError(error);
    return result(ok, matchId, error);
}

QVariantMap Backend::createMatchBoard(int matchId)
{
    QString error;
    const Match m = m_matches.get(matchId, &error);
    if (m.id < 0) {
        const QString msg = error.isEmpty() ? tr("Match introuvable.") : error;
        setError(msg);
        return result(false, matchId, msg);
    }
    BoardRecord existing;
    if (m_repo.forMatch(matchId, &existing, &error)) {
        setError(QString());
        QVariantMap r = result(true, matchId);
        r.insert(QStringLiteral("boardId"), existing.id);
        r.insert(QStringLiteral("created"), false);
        return r;
    }
    if (!error.isEmpty()) {
        setError(error);
        return result(false, matchId, error);
    }
    BoardRecord record;
    record.title = tr("Match contre %1").arg(m.opponent);
    record.matchId = matchId;
    if (!m_repo.insert(record, &error)) {
        setError(error);
        return result(false, matchId, error);
    }
    setError(QString());
    QVariantMap r = result(true, matchId);
    r.insert(QStringLiteral("boardId"), record.id);
    r.insert(QStringLiteral("created"), true);
    return r;
}
