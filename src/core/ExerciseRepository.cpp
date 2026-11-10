#include "ExerciseRepository.h"

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

QVariant nullIfNotPositive(int v)
{
    return v > 0 ? QVariant(v) : QVariant();
}

void bindExercise(QSqlQuery &q, const Exercise &e)
{
    q.addBindValue(e.title.trimmed());
    q.addBindValue(e.category);
    q.addBindValue(nullIfEmpty(e.objective));
    q.addBindValue(nullIfNotPositive(e.durationMin));
    q.addBindValue(nullIfNotPositive(e.playersMin));
    q.addBindValue(nullIfNotPositive(e.playersMax));
    q.addBindValue(nullIfEmpty(e.equipment));
    q.addBindValue(nullIfEmpty(e.description));
    q.addBindValue(e.archived ? 1 : 0);
    q.addBindValue(nullIfNotPositive(e.categoryId));
}

} // namespace

QList<Exercise> ExerciseRepository::list(bool includeArchived, QString *error) const
{
    QList<Exercise> result;
    QString sql = QStringLiteral(
        "SELECT e.id, e.title, e.category, e.objective, e.duration_min, e.players_min, e.players_max, e.equipment, "
        "e.description, e.archived, e.category_id, c.name FROM exercise e "
        "LEFT JOIN exercise_category c ON c.id = e.category_id");
    if (!includeArchived)
        sql += QStringLiteral(" WHERE e.archived = 0");
    sql += QStringLiteral(" ORDER BY e.title COLLATE NOCASE, e.id");

    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        setError(error, q.lastError().text());
        return result;
    }
    while (q.next()) {
        Exercise e;
        e.id = q.value(0).toInt();
        e.title = q.value(1).toString();
        e.category = q.value(2).toString();
        e.objective = q.value(3).toString();
        e.durationMin = q.value(4).isNull() ? 0 : q.value(4).toInt();
        e.playersMin = q.value(5).isNull() ? 0 : q.value(5).toInt();
        e.playersMax = q.value(6).isNull() ? 0 : q.value(6).toInt();
        e.equipment = q.value(7).toString();
        e.description = q.value(8).toString();
        e.archived = q.value(9).toInt() != 0;
        e.categoryId = q.value(10).isNull() ? 0 : q.value(10).toInt();
        e.categoryName = q.value(11).toString();
        result.append(e);
    }
    return result;
}

bool ExerciseRepository::insert(Exercise &exercise, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO exercise(title, category, objective, duration_min, players_min, players_max, "
        "equipment, description, archived, category_id) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    bindExercise(q, exercise);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    exercise.id = q.lastInsertId().toInt();
    return true;
}

bool ExerciseRepository::update(const Exercise &exercise, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE exercise SET title = ?, category = ?, objective = ?, duration_min = ?, players_min = ?, "
        "players_max = ?, equipment = ?, description = ?, archived = ?, category_id = ? WHERE id = ?"));
    bindExercise(q, exercise);
    q.addBindValue(exercise.id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool ExerciseRepository::setArchived(int id, bool archived, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE exercise SET archived = ? WHERE id = ?"));
    q.addBindValue(archived ? 1 : 0);
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

QList<ExerciseCategory> ExerciseRepository::categories(QString *error) const
{
    QList<ExerciseCategory> result;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT c.id, c.name, (SELECT COUNT(*) FROM exercise e WHERE e.category_id = c.id) "
                               "FROM exercise_category c ORDER BY c.name COLLATE NOCASE, c.id"))) {
        setError(error, q.lastError().text());
        return result;
    }
    while (q.next()) {
        ExerciseCategory c;
        c.id = q.value(0).toInt();
        c.name = q.value(1).toString();
        c.exerciseCount = q.value(2).toInt();
        result.append(c);
    }
    return result;
}

bool ExerciseRepository::addCategory(const QString &name, int *id, QString *error)
{
    const QString trimmed = name.simplified();
    if (trimmed.isEmpty()) {
        setError(error, QStringLiteral("Le nom de la catégorie est obligatoire."));
        return false;
    }
    if (trimmed.size() > 40) {
        setError(error, QStringLiteral("Le nom de la catégorie est trop long (40 caractères au plus)."));
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO exercise_category(name) VALUES(?)"));
    q.addBindValue(trimmed);
    if (!q.exec()) {
        setError(error, q.lastError().text().contains(QLatin1String("UNIQUE"))
                            ? QStringLiteral("Cette catégorie existe déjà.") : q.lastError().text());
        return false;
    }
    if (id)
        *id = q.lastInsertId().toInt();
    return true;
}

bool ExerciseRepository::renameCategory(int id, const QString &name, QString *error)
{
    const QString trimmed = name.simplified();
    if (trimmed.isEmpty()) {
        setError(error, QStringLiteral("Le nom de la catégorie est obligatoire."));
        return false;
    }
    if (trimmed.size() > 40) {
        setError(error, QStringLiteral("Le nom de la catégorie est trop long (40 caractères au plus)."));
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE exercise_category SET name = ? WHERE id = ?"));
    q.addBindValue(trimmed);
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text().contains(QLatin1String("UNIQUE"))
                            ? QStringLiteral("Cette catégorie existe déjà.") : q.lastError().text());
        return false;
    }
    if (q.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Cette catégorie n'existe plus."));
        return false;
    }
    return true;
}

bool ExerciseRepository::removeCategory(int id, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM exercise_category WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (q.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Cette catégorie n'existe plus."));
        return false;
    }
    return true;
}

int ExerciseRepository::usageCount(int id) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT COUNT(DISTINCT session_id) FROM session_exercise WHERE exercise_id = ?"));
    q.addBindValue(id);
    return (q.exec() && q.next()) ? q.value(0).toInt() : 0;
}

bool ExerciseRepository::remove(int id, QString *error)
{
    const int used = usageCount(id);
    if (used > 0) {
        setError(error, QStringLiteral("Exercice utilisé dans %1 séance(s) : archivez-le plutôt que de le supprimer.").arg(used));
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM exercise WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}
