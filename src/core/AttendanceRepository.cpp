#include "AttendanceRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace {

const QString kDateFormat = QStringLiteral("yyyy-MM-dd");

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

} // namespace

QMap<int, AttendanceRepository::Entry> AttendanceRepository::load(int sessionId, QString *error) const
{
    QMap<int, Entry> entries;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT player_id, status, note FROM attendance WHERE session_id = ?"));
    q.addBindValue(sessionId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return entries;
    }
    while (q.next()) {
        Entry e;
        e.playerId = q.value(0).toInt();
        e.status = q.value(1).toString();
        e.note = q.value(2).toString();
        entries.insert(e.playerId, e);
    }
    return entries;
}

bool AttendanceRepository::save(int sessionId, const QList<Entry> &entries, QString *error)
{
    if (!m_db.transaction()) {
        setError(error, m_db.lastError().text());
        return false;
    }

    QSqlQuery del(m_db);
    del.prepare(QStringLiteral("DELETE FROM attendance WHERE session_id = ?"));
    del.addBindValue(sessionId);
    if (!del.exec()) {
        setError(error, del.lastError().text());
        m_db.rollback();
        return false;
    }

    for (const Entry &e : entries) {
        QSqlQuery ins(m_db);
        ins.prepare(QStringLiteral("INSERT INTO attendance(session_id, player_id, status, note) VALUES(?, ?, ?, ?)"));
        ins.addBindValue(sessionId);
        ins.addBindValue(e.playerId);
        ins.addBindValue(e.status);
        ins.addBindValue(e.note.trimmed().isEmpty() ? QVariant() : QVariant(e.note.trimmed()));
        if (!ins.exec()) {
            setError(error, ins.lastError().text());
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        setError(error, m_db.lastError().text());
        m_db.rollback();
        return false;
    }
    return true;
}

QMap<int, QPair<int, int>> AttendanceRepository::summary(const QList<int> &sessionIds) const
{
    QMap<int, QPair<int, int>> result;
    if (sessionIds.isEmpty())
        return result;

    // Les ids sont des entiers : la liste IN peut être construite sans risque d'injection.
    QStringList ids;
    for (int id : sessionIds)
        ids << QString::number(id);

    QSqlQuery q(m_db);
    const QString sql = QStringLiteral(
        "SELECT session_id, SUM(CASE WHEN status = 'present' THEN 1 ELSE 0 END), COUNT(*) "
        "FROM attendance WHERE session_id IN (%1) GROUP BY session_id").arg(ids.join(QLatin1Char(',')));
    if (q.exec(sql)) {
        while (q.next())
            result.insert(q.value(0).toInt(), {q.value(1).toInt(), q.value(2).toInt()});
    }
    return result;
}

AttendanceStats::Data AttendanceRepository::loadData(int teamId, const QDate &from, const QDate &to, QString *error) const
{
    AttendanceStats::Data data;
    const QString fromStr = from.toString(kDateFormat);
    const QString toStr = to.addDays(1).toString(kDateFormat);

    QSqlQuery qs(m_db);
    qs.prepare(QStringLiteral(
        "SELECT id, substr(start_at, 1, 10) FROM training_session "
        "WHERE team_id = ? AND status = 'done' AND start_at >= ? AND start_at < ? ORDER BY start_at"));
    qs.addBindValue(teamId);
    qs.addBindValue(fromStr);
    qs.addBindValue(toStr);
    if (!qs.exec()) {
        setError(error, qs.lastError().text());
        return data;
    }
    while (qs.next()) {
        AttendanceStats::SessionRef s;
        s.id = qs.value(0).toInt();
        s.date = QDate::fromString(qs.value(1).toString(), kDateFormat);
        data.sessions.append(s);
    }

    QSqlQuery qp(m_db);
    qp.prepare(QStringLiteral(
        "SELECT id, last_name, first_name, jersey_number, active, substr(created_at, 1, 10) FROM player "
        "WHERE team_id = ? ORDER BY last_name COLLATE NOCASE, first_name COLLATE NOCASE"));
    qp.addBindValue(teamId);
    if (!qp.exec()) {
        setError(error, qp.lastError().text());
        return data;
    }
    while (qp.next()) {
        AttendanceStats::PlayerRef p;
        p.id = qp.value(0).toInt();
        p.lastName = qp.value(1).toString();
        p.firstName = qp.value(2).toString();
        p.jerseyNumber = qp.value(3).isNull() ? 0 : qp.value(3).toInt();
        p.active = qp.value(4).toInt() != 0;
        p.joined = QDate::fromString(qp.value(5).toString(), kDateFormat);
        data.players.append(p);
    }

    QSqlQuery qa(m_db);
    qa.prepare(QStringLiteral(
        "SELECT a.session_id, a.player_id, a.status FROM attendance a "
        "JOIN training_session s ON s.id = a.session_id "
        "WHERE s.team_id = ? AND s.status = 'done' AND s.start_at >= ? AND s.start_at < ?"));
    qa.addBindValue(teamId);
    qa.addBindValue(fromStr);
    qa.addBindValue(toStr);
    if (!qa.exec()) {
        setError(error, qa.lastError().text());
        return data;
    }
    while (qa.next())
        data.rows.insert({qa.value(0).toInt(), qa.value(1).toInt()}, qa.value(2).toString());

    return data;
}
