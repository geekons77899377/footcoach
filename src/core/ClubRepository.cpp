#include "ClubRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

} // namespace

QList<Club> ClubRepository::all(QString *error) const
{
    QList<Club> clubs;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT id, name, department FROM club ORDER BY name COLLATE NOCASE, id"))) {
        setError(error, q.lastError().text());
        return clubs;
    }
    while (q.next()) {
        Club c;
        c.id = q.value(0).toInt();
        c.name = q.value(1).toString();
        c.department = q.value(2).toString();
        clubs.append(c);
    }
    return clubs;
}

bool ClubRepository::get(int id, Club *out, QString *error) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, name, department FROM club WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (!q.next()) {
        setError(error, QStringLiteral("Club introuvable."));
        return false;
    }
    out->id = q.value(0).toInt();
    out->name = q.value(1).toString();
    out->department = q.value(2).toString();
    return true;
}

bool ClubRepository::insert(Club &club, QString *error)
{
    if (club.name.trimmed().isEmpty()) {
        setError(error, QStringLiteral("Le nom du club est obligatoire."));
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO club(name) VALUES(?)"));
    q.addBindValue(club.name.trimmed());
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    club.id = q.lastInsertId().toInt();
    return true;
}

bool ClubRepository::update(const Club &club, QString *error)
{
    if (club.name.trimmed().isEmpty()) {
        setError(error, QStringLiteral("Le nom du club est obligatoire."));
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE club SET name = ? WHERE id = ?"));
    q.addBindValue(club.name.trimmed());
    q.addBindValue(club.id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (q.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Club introuvable."));
        return false;
    }
    return true;
}

bool ClubRepository::remove(int id, QString *error)
{
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM club")) || !q.next()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (q.value(0).toInt() <= 1) {
        setError(error, QStringLiteral("Impossible de supprimer le dernier club."));
        return false;
    }
    q = QSqlQuery(m_db);
    q.prepare(QStringLiteral("DELETE FROM club WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (q.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Club introuvable."));
        return false;
    }
    return true;
}

bool ClubRepository::setDepartment(int id, const QString &department, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE club SET department = ? WHERE id = ?"));
    const QString d = department.trimmed().toUpper();
    q.addBindValue(d.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : QVariant(d));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (q.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Club introuvable."));
        return false;
    }
    return true;
}

int ClubRepository::clubOfTeam(int teamId) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT club_id FROM team WHERE id = ?"));
    q.addBindValue(teamId);
    return (q.exec() && q.next() && !q.value(0).isNull()) ? q.value(0).toInt() : -1;
}

int ClubRepository::teamCount(int clubId) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM team WHERE club_id = ?"));
    q.addBindValue(clubId);
    return (q.exec() && q.next()) ? q.value(0).toInt() : 0;
}
