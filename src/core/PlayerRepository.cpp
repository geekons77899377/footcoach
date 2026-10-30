#include "PlayerRepository.h"

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

void bindPlayer(QSqlQuery &q, const Player &p)
{
    q.addBindValue(p.teamId);
    q.addBindValue(p.firstName.trimmed());
    q.addBindValue(p.lastName.trimmed());
    q.addBindValue(p.birthDate.isValid() ? QVariant(p.birthDate.toString(Qt::ISODate)) : QVariant());
    q.addBindValue(p.jerseyNumber > 0 ? QVariant(p.jerseyNumber) : QVariant());
    q.addBindValue(nullIfEmpty(p.position));
    q.addBindValue(nullIfEmpty(p.preferredFoot));
    q.addBindValue(nullIfEmpty(p.phone));
    q.addBindValue(nullIfEmpty(p.email));
    q.addBindValue(nullIfEmpty(p.notes));
    q.addBindValue(p.active ? 1 : 0);
}

} // namespace

QList<Player> PlayerRepository::list(int teamId, bool includeInactive, QString *error) const
{
    QList<Player> players;
    QSqlQuery q(m_db);
    QString sql = QStringLiteral(
        "SELECT id, team_id, first_name, last_name, birth_date, jersey_number, position, "
        "preferred_foot, phone, email, notes, active FROM player WHERE team_id = ?");
    if (!includeInactive)
        sql += QStringLiteral(" AND active = 1");
    sql += QStringLiteral(" ORDER BY last_name COLLATE NOCASE, first_name COLLATE NOCASE");

    q.prepare(sql);
    q.addBindValue(teamId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return players;
    }
    while (q.next()) {
        Player p;
        p.id = q.value(0).toInt();
        p.teamId = q.value(1).toInt();
        p.firstName = q.value(2).toString();
        p.lastName = q.value(3).toString();
        p.birthDate = QDate::fromString(q.value(4).toString(), Qt::ISODate);
        p.jerseyNumber = q.value(5).isNull() ? 0 : q.value(5).toInt();
        p.position = q.value(6).toString();
        p.preferredFoot = q.value(7).toString();
        p.phone = q.value(8).toString();
        p.email = q.value(9).toString();
        p.notes = q.value(10).toString();
        p.active = q.value(11).toInt() != 0;
        players.append(p);
    }
    return players;
}

bool PlayerRepository::insert(Player &player, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO player(team_id, first_name, last_name, birth_date, jersey_number, position, "
        "preferred_foot, phone, email, notes, active) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    bindPlayer(q, player);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    player.id = q.lastInsertId().toInt();
    return true;
}

bool PlayerRepository::update(const Player &player, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE player SET team_id = ?, first_name = ?, last_name = ?, birth_date = ?, "
        "jersey_number = ?, position = ?, preferred_foot = ?, phone = ?, email = ?, notes = ?, "
        "active = ? WHERE id = ?"));
    bindPlayer(q, player);
    q.addBindValue(player.id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool PlayerRepository::setActive(int id, bool active, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE player SET active = ? WHERE id = ?"));
    q.addBindValue(active ? 1 : 0);
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool PlayerRepository::remove(int id, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM player WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool PlayerRepository::jerseyTaken(int teamId, int number, int excludeId) const
{
    if (number <= 0)
        return false;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT 1 FROM player WHERE team_id = ? AND jersey_number = ? AND active = 1 AND id <> ? LIMIT 1"));
    q.addBindValue(teamId);
    q.addBindValue(number);
    q.addBindValue(excludeId);
    return q.exec() && q.next();
}
