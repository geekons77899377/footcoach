#include "BoardRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

QVariant nullIfNotPositive(int v)
{
    return v > 0 ? QVariant(v) : QVariant();
}

QVariant nullIfEmpty(const QString &s)
{
    return s.trimmed().isEmpty() ? QVariant() : QVariant(s.trimmed());
}

const char *kColumns = "id, title, formation, scene_json, exercise_id, session_id, updated_at";

BoardRecord readRecord(const QSqlQuery &q)
{
    BoardRecord b;
    b.id = q.value(0).toInt();
    b.title = q.value(1).toString();
    b.formation = q.value(2).toString();
    b.sceneJson = q.value(3).toString();
    b.exerciseId = q.value(4).isNull() ? -1 : q.value(4).toInt();
    b.sessionId = q.value(5).isNull() ? -1 : q.value(5).toInt();
    b.updatedAt = q.value(6).toString();
    return b;
}

} // namespace

QList<BoardRecord> BoardRepository::list(QString *error) const
{
    QList<BoardRecord> result;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT %1 FROM tactical_board ORDER BY title COLLATE NOCASE, id")
                    .arg(QLatin1String(kColumns)))) {
        setError(error, q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(readRecord(q));
    return result;
}

QList<BoardRecord> BoardRepository::forSession(int sessionId, QString *error) const
{
    QList<BoardRecord> result;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
                  "SELECT %1 FROM tactical_board WHERE session_id = ? "
                  "OR exercise_id IN (SELECT exercise_id FROM session_exercise WHERE session_id = ?) "
                  "ORDER BY id")
                  .arg(QLatin1String(kColumns)));
    q.addBindValue(sessionId);
    q.addBindValue(sessionId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(readRecord(q));
    return result;
}

bool BoardRepository::insert(BoardRecord &board, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO tactical_board(title, formation, scene_json, exercise_id, session_id) VALUES(?, ?, ?, ?, ?)"));
    q.addBindValue(board.title.trimmed());
    q.addBindValue(nullIfEmpty(board.formation));
    q.addBindValue(board.sceneJson.isEmpty() ? QStringLiteral("{}") : board.sceneJson);
    q.addBindValue(nullIfNotPositive(board.exerciseId));
    q.addBindValue(nullIfNotPositive(board.sessionId));
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    board.id = q.lastInsertId().toInt();
    return true;
}

bool BoardRepository::update(const BoardRecord &board, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE tactical_board SET title = ?, formation = ?, scene_json = ?, exercise_id = ?, session_id = ?, "
        "updated_at = CURRENT_TIMESTAMP WHERE id = ?"));
    q.addBindValue(board.title.trimmed());
    q.addBindValue(nullIfEmpty(board.formation));
    q.addBindValue(board.sceneJson.isEmpty() ? QStringLiteral("{}") : board.sceneJson);
    q.addBindValue(nullIfNotPositive(board.exerciseId));
    q.addBindValue(nullIfNotPositive(board.sessionId));
    q.addBindValue(board.id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool BoardRepository::remove(int id, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM tactical_board WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}
