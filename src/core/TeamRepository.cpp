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

QList<Team> TeamRepository::all(QString *error, int clubId) const
{
    QList<Team> teams;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, name, category, season, club_id FROM team %1 ORDER BY name COLLATE NOCASE")
                  .arg(clubId > 0 ? QStringLiteral("WHERE club_id = ?") : QString()));
    if (clubId > 0)
        q.addBindValue(clubId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return teams;
    }
    while (q.next()) {
        Team t;
        t.id = q.value(0).toInt();
        t.name = q.value(1).toString();
        t.category = q.value(2).toString();
        t.season = q.value(3).toString();
        t.clubId = q.value(4).isNull() ? -1 : q.value(4).toInt();
        teams.append(t);
    }
    return teams;
}

bool TeamRepository::insert(Team &team, QString *error)
{
    QSqlQuery q(m_db);
    if (team.clubId <= 0) {
        if (!q.exec(QStringLiteral("SELECT MIN(id) FROM club")) || !q.next() || q.value(0).isNull()) {
            setError(error, QStringLiteral("Aucun club : créez d'abord un club."));
            return false;
        }
        team.clubId = q.value(0).toInt();
    } else {
        q.prepare(QStringLiteral("SELECT COUNT(*) FROM club WHERE id = ?"));
        q.addBindValue(team.clubId);
        if (!q.exec() || !q.next() || q.value(0).toInt() == 0) {
            setError(error, QStringLiteral("Club introuvable."));
            return false;
        }
    }
    q = QSqlQuery(m_db);
    q.prepare(QStringLiteral("INSERT INTO team(name, category, season, club_id) VALUES(?, ?, ?, ?)"));
    q.addBindValue(team.name.trimmed());
    q.addBindValue(nullIfEmpty(team.category));
    q.addBindValue(nullIfEmpty(team.season));
    q.addBindValue(team.clubId);
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
    if (q.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Équipe introuvable."));
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
