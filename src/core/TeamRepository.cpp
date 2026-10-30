#include "TeamRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

QVariant nullIfEmpty(const QString &s)
{
    return s.trimmed().isEmpty() ? QVariant() : QVariant(s.trimmed());
}

} // namespace

QList<Team> TeamRepository::all(QString *error) const
{
    QList<Team> teams;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT id, name, category, season FROM team ORDER BY name COLLATE NOCASE"))) {
        setError(error, q.lastError().text());
        return teams;
    }
    while (q.next()) {
        Team t;
        t.id = q.value(0).toInt();
        t.name = q.value(1).toString();
        t.category = q.value(2).toString();
        t.season = q.value(3).toString();
        teams.append(t);
    }
    return teams;
}

bool TeamRepository::insert(Team &team, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO team(name, category, season) VALUES(?, ?, ?)"));
    q.addBindValue(team.name.trimmed());
    q.addBindValue(nullIfEmpty(team.category));
    q.addBindValue(nullIfEmpty(team.season));
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    team.id = q.lastInsertId().toInt();
    return true;
}

bool TeamRepository::update(const Team &team, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE team SET name = ?, category = ?, season = ? WHERE id = ?"));
    q.addBindValue(team.name.trimmed());
    q.addBindValue(nullIfEmpty(team.category));
    q.addBindValue(nullIfEmpty(team.season));
    q.addBindValue(team.id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool TeamRepository::remove(int id, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM team WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

int TeamRepository::playerCount(int teamId) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM player WHERE team_id = ?"));
    q.addBindValue(teamId);
    return (q.exec() && q.next()) ? q.value(0).toInt() : 0;
}
