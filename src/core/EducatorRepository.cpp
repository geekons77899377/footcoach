#include "EducatorRepository.h"

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

Educator fromQuery(const QSqlQuery &q)
{
    Educator e;
    e.id = q.value(0).toInt();
    e.clubId = q.value(1).toInt();
    e.firstName = q.value(2).toString();
    e.lastName = q.value(3).toString();
    e.licence = q.value(4).toString();
    return e;
}

const char *const kColumns = "id, club_id, first_name, last_name, licence";

} // namespace

QList<Educator> EducatorRepository::list(int clubId, QString *error) const
{
    QList<Educator> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT %1 FROM educator WHERE club_id = ? "
                             "ORDER BY last_name COLLATE NOCASE, first_name COLLATE NOCASE, id")
                  .arg(QLatin1String(kColumns)));
    q.addBindValue(clubId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return out;
    }
    while (q.next())
        out.append(fromQuery(q));
    return out;
}

bool EducatorRepository::get(int id, Educator *out, QString *error) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT %1 FROM educator WHERE id = ?").arg(QLatin1String(kColumns)));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (!q.next()) {
        setError(error, QStringLiteral("Éducateur introuvable."));
        return false;
    }
    if (out)
        *out = fromQuery(q);
    return true;
}

bool EducatorRepository::save(Educator &e, QString *error)
{
    e.firstName = e.firstName.simplified();
    e.lastName = e.lastName.simplified();
    e.licence = PlayerRepository::normalizeLicence(e.licence);
    if (e.firstName.isEmpty() || e.lastName.isEmpty()) {
        setError(error, QStringLiteral("Le prénom et le nom de l'éducateur sont obligatoires."));
        return false;
    }
    QSqlQuery q(m_db);
    if (e.id > 0) {
        Educator old;
        if (!get(e.id, &old, error))
            return false;
        e.clubId = old.clubId;              // un éducateur ne change pas de club
    } else {
        q.prepare(QStringLiteral("SELECT COUNT(*) FROM club WHERE id = ?"));
        q.addBindValue(e.clubId);
        if (!q.exec() || !q.next() || q.value(0).toInt() == 0) {
            setError(error, QStringLiteral("Club introuvable."));
            return false;
        }
    }
    if (!e.licence.isEmpty()) {
        q = QSqlQuery(m_db);
        q.prepare(QStringLiteral("SELECT first_name, last_name FROM educator "
                                 "WHERE club_id = ? AND UPPER(licence) = ? AND id <> ? LIMIT 1"));
        q.addBindValue(e.clubId);
        q.addBindValue(e.licence.toUpper());
        q.addBindValue(e.id);
        if (q.exec() && q.next()) {
            setError(error, QStringLiteral("Ce n° de licence est déjà celui de %1 %2.")
                                .arg(q.value(0).toString(), q.value(1).toString()));
            return false;
        }
    }
    q = QSqlQuery(m_db);
    if (e.id > 0) {
        q.prepare(QStringLiteral("UPDATE educator SET first_name = ?, last_name = ?, licence = ? WHERE id = ?"));
    } else {
        q.prepare(QStringLiteral("INSERT INTO educator(first_name, last_name, licence, club_id) VALUES(?, ?, ?, ?)"));
    }
    q.addBindValue(e.firstName);
    q.addBindValue(e.lastName);
    q.addBindValue(e.licence.isEmpty() ? QVariant() : QVariant(e.licence));
    q.addBindValue(e.id > 0 ? e.id : e.clubId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (e.id <= 0)
        e.id = q.lastInsertId().toInt();
    return true;
}

bool EducatorRepository::remove(int id, QString *error)
{
    QSqlQuery q(m_db);
    // Filet si les clés étrangères étaient désactivées : l'équipe perd son responsable.
    q.prepare(QStringLiteral("UPDATE team SET educator_id = NULL WHERE educator_id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    q = QSqlQuery(m_db);
    q.prepare(QStringLiteral("DELETE FROM educator WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (q.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Éducateur introuvable."));
        return false;
    }
    return true;
}

int EducatorRepository::teamCount(int educatorId) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM team WHERE educator_id = ?"));
    q.addBindValue(educatorId);
    return (q.exec() && q.next()) ? q.value(0).toInt() : 0;
}

bool EducatorRepository::setTeamEducator(int teamId, int educatorId, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT club_id FROM team WHERE id = ?"));
    q.addBindValue(teamId);
    if (!q.exec() || !q.next()) {
        setError(error, QStringLiteral("Équipe introuvable."));
        return false;
    }
    const int clubId = q.value(0).toInt();
    if (educatorId > 0) {
        Educator e;
        if (!get(educatorId, &e, error))
            return false;
        if (e.clubId != clubId) {
            setError(error, QStringLiteral("Cet éducateur n'est pas du club de l'équipe."));
            return false;
        }
    }
    q = QSqlQuery(m_db);
    q.prepare(QStringLiteral("UPDATE team SET educator_id = ? WHERE id = ?"));
    q.addBindValue(educatorId > 0 ? QVariant(educatorId) : QVariant());
    q.addBindValue(teamId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool EducatorRepository::ofTeam(int teamId, Educator *out) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT e.id, e.club_id, e.first_name, e.last_name, e.licence "
                             "FROM team t JOIN educator e ON e.id = t.educator_id WHERE t.id = ?"));
    q.addBindValue(teamId);
    if (!q.exec() || !q.next())
        return false;
    if (out)
        *out = fromQuery(q);
    return true;
}
