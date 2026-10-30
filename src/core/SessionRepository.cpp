#include "SessionRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

const QString kDateTimeFormat = QStringLiteral("yyyy-MM-dd'T'HH:mm");
const QString kDateFormat = QStringLiteral("yyyy-MM-dd");

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

QVariant nullIfEmpty(const QString &s)
{
    return s.trimmed().isEmpty() ? QVariant() : QVariant(s.trimmed());
}

const QString kSelect = QStringLiteral(
    "SELECT id, team_id, start_at, duration_min, location, theme, notes, status FROM training_session");

Session readSession(const QSqlQuery &q)
{
    Session s;
    s.id = q.value(0).toInt();
    s.teamId = q.value(1).toInt();
    s.startAt = QDateTime::fromString(q.value(2).toString(), kDateTimeFormat);
    s.durationMin = q.value(3).toInt();
    s.location = q.value(4).toString();
    s.theme = q.value(5).toString();
    s.notes = q.value(6).toString();
    s.status = q.value(7).toString();
    return s;
}

} // namespace

Session SessionRepository::get(int id, QString *error) const
{
    QSqlQuery q(m_db);
    q.prepare(kSelect + QStringLiteral(" WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return Session();
    }
    return q.next() ? readSession(q) : Session();
}

QList<Session> SessionRepository::listBetween(int teamId, const QDate &from, const QDate &to, QString *error) const
{
    QList<Session> sessions;
    QSqlQuery q(m_db);
    // Comparaison de chaînes ISO : "2026-10-24T10:00" < "2026-10-25" (borne exclusive = lendemain).
    q.prepare(kSelect + QStringLiteral(" WHERE team_id = ? AND start_at >= ? AND start_at < ? ORDER BY start_at"));
    q.addBindValue(teamId);
    q.addBindValue(from.toString(kDateFormat));
    q.addBindValue(to.addDays(1).toString(kDateFormat));
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return sessions;
    }
    while (q.next())
        sessions.append(readSession(q));
    return sessions;
}

QSet<QDate> SessionRepository::datesWithSessions(int teamId, const QDate &from, const QDate &to) const
{
    QSet<QDate> dates;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT DISTINCT substr(start_at, 1, 10) FROM training_session "
        "WHERE team_id = ? AND start_at >= ? AND start_at < ?"));
    q.addBindValue(teamId);
    q.addBindValue(from.toString(kDateFormat));
    q.addBindValue(to.addDays(1).toString(kDateFormat));
    if (q.exec()) {
        while (q.next())
            dates.insert(QDate::fromString(q.value(0).toString(), kDateFormat));
    }
    return dates;
}

bool SessionRepository::insertOne(Session &s, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO training_session(team_id, start_at, duration_min, location, theme, notes, status) "
        "VALUES(?, ?, ?, ?, ?, ?, ?)"));
    q.addBindValue(s.teamId);
    q.addBindValue(s.startAt.toString(kDateTimeFormat));
    q.addBindValue(s.durationMin);
    q.addBindValue(nullIfEmpty(s.location));
    q.addBindValue(nullIfEmpty(s.theme));
    q.addBindValue(nullIfEmpty(s.notes));
    q.addBindValue(s.status);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    s.id = q.lastInsertId().toInt();
    return true;
}

bool SessionRepository::insert(Session &session, QString *error)
{
    return insertOne(session, error);
}

bool SessionRepository::insertMany(QList<Session> &sessions, QString *error)
{
    if (!m_db.transaction()) {
        setError(error, m_db.lastError().text());
        return false;
    }
    for (Session &s : sessions) {
        if (!insertOne(s, error)) {
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

bool SessionRepository::update(const Session &s, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE training_session SET team_id = ?, start_at = ?, duration_min = ?, location = ?, "
        "theme = ?, notes = ?, status = ? WHERE id = ?"));
    q.addBindValue(s.teamId);
    q.addBindValue(s.startAt.toString(kDateTimeFormat));
    q.addBindValue(s.durationMin);
    q.addBindValue(nullIfEmpty(s.location));
    q.addBindValue(nullIfEmpty(s.theme));
    q.addBindValue(nullIfEmpty(s.notes));
    q.addBindValue(s.status);
    q.addBindValue(s.id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool SessionRepository::setStatus(int id, const QString &status, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE training_session SET status = ? WHERE id = ?"));
    q.addBindValue(status);
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool SessionRepository::remove(int id, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM training_session WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

QList<Session> SessionRepository::conflicts(int teamId, const QDateTime &start, int durationMin, int excludeId) const
{
    QList<Session> result;
    if (!start.isValid() || durationMin <= 0)
        return result;

    const QDateTime end = start.addSecs(qint64(durationMin) * 60);

    // On ne charge que la veille, le jour et le lendemain (une séance dure moins de 24 h),
    // puis on compare les intervalles en C++.
    QSqlQuery q(m_db);
    q.prepare(kSelect + QStringLiteral(
        " WHERE team_id = ? AND status <> 'cancelled' AND id <> ? AND start_at >= ? AND start_at < ?"
        " ORDER BY start_at"));
    q.addBindValue(teamId);
    q.addBindValue(excludeId);
    q.addBindValue(start.date().addDays(-1).toString(kDateFormat));
    q.addBindValue(start.date().addDays(2).toString(kDateFormat));
    if (!q.exec())
        return result;

    while (q.next()) {
        const Session other = readSession(q);
        if (other.startAt < end && other.endAt() > start)
            result.append(other);
    }
    return result;
}
