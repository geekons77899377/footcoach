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

const char *kColumns = "id, title, formation, scene_json, exercise_id, session_id, session_exercise_id, match_id, updated_at";

BoardRecord readRecord(const QSqlQuery &q)
{
    BoardRecord b;
    b.id = q.value(0).toInt();
    b.title = q.value(1).toString();
    b.formation = q.value(2).toString();
    b.sceneJson = q.value(3).toString();
    b.exerciseId = q.value(4).isNull() ? -1 : q.value(4).toInt();
    b.sessionId = q.value(5).isNull() ? -1 : q.value(5).toInt();
    b.sessionExerciseId = q.value(6).isNull() ? -1 : q.value(6).toInt();
    b.matchId = q.value(7).isNull() ? -1 : q.value(7).toInt();
    b.updatedAt = q.value(8).toString();
    return b;
}

} // namespace

QList<BoardRecord> BoardRepository::list(QString *error, bool includeBlockBoards) const
{
    QList<BoardRecord> result;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT %1 FROM tactical_board %2 ORDER BY title COLLATE NOCASE, id")
                    .arg(QLatin1String(kColumns),
                         includeBlockBoards ? QString() : QStringLiteral("WHERE session_exercise_id IS NULL AND match_id IS NULL")))) {
        setError(error, q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(readRecord(q));
    return result;
}

bool BoardRepository::get(int id, BoardRecord *out, QString *error) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT %1 FROM tactical_board WHERE id = ?").arg(QLatin1String(kColumns)));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (!q.next()) {
        setError(error, QStringLiteral("Schéma introuvable."));
        return false;
    }
    *out = readRecord(q);
    return true;
}

bool BoardRepository::forMatch(int matchId, BoardRecord *out, QString *error) const
{
    setError(error, QString());
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT %1 FROM tactical_board WHERE match_id = ? ORDER BY id LIMIT 1")
                  .arg(QLatin1String(kColumns)));
    q.addBindValue(matchId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (!q.next())
        return false;
    *out = readRecord(q);
    return true;
}

bool BoardRepository::forBlock(int sessionExerciseId, BoardRecord *out, QString *error) const
{
    setError(error, QString());
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT %1 FROM tactical_board WHERE session_exercise_id = ? ORDER BY id LIMIT 1")
                  .arg(QLatin1String(kColumns)));
    q.addBindValue(sessionExerciseId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (!q.next())
        return false;
    *out = readRecord(q);
    return true;
}

QList<BoardRecord> BoardRepository::forBlocksOfSession(int sessionId, QString *error) const
{
    QList<BoardRecord> result;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT %1 FROM tactical_board WHERE session_exercise_id IN "
                             "(SELECT id FROM session_exercise WHERE session_id = ?) ORDER BY id")
                  .arg(QLatin1String(kColumns)));
    q.addBindValue(sessionId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(readRecord(q));
    return result;
}

bool BoardRepository::referenceForExercise(int exerciseId, BoardRecord *out, QString *error) const
{
    setError(error, QString());
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT %1 FROM tactical_board WHERE exercise_id = ? AND session_exercise_id IS NULL "
                             "ORDER BY updated_at DESC, id DESC LIMIT 1")
                  .arg(QLatin1String(kColumns)));
    q.addBindValue(exerciseId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (!q.next())
        return false;
    *out = readRecord(q);
    return true;
}

QList<BoardRecord> BoardRepository::forSession(int sessionId, QString *error) const
{
    QList<BoardRecord> result;
    QSqlQuery q(m_db);
    // Une seule ligne par schéma (un OR sur une seule table, pas de jointure) : un schéma de référence n'est pas
    // compté deux fois quand l'exercice est plusieurs fois dans le plan. Un terrain de bloc n'est jamais pris pour un
    // schéma de référence, même si d'anciennes données lui donnaient aussi un exercice.
    q.prepare(QStringLiteral(
                  "SELECT %1 FROM tactical_board WHERE session_id = ? "
                  "OR (session_exercise_id IS NULL AND exercise_id IN "
                  "(SELECT exercise_id FROM session_exercise WHERE session_id = ?)) "
                  "OR session_exercise_id IN (SELECT id FROM session_exercise WHERE session_id = ?) "
                  "ORDER BY id")
                  .arg(QLatin1String(kColumns)));
    q.addBindValue(sessionId);
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

bool BoardRepository::checkLinks(const BoardRecord &board, int previousExerciseId, int previousBlockId,
                                 int previousMatchId, QString *error) const
{
    if (board.matchId > 0 && (board.exerciseId > 0 || board.sessionId > 0 || board.sessionExerciseId > 0)) {
        setError(error, QStringLiteral("Le schéma d'un match ne peut pas être lié à un exercice, une séance "
                                       "ou un bloc."));
        return false;
    }
    if (board.matchId > 0 && board.matchId != previousMatchId) {
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral("SELECT COUNT(*) FROM tactical_board WHERE match_id = ? AND id != ?"));
        q.addBindValue(board.matchId);
        q.addBindValue(board.id);
        if (!q.exec() || !q.next()) {
            setError(error, q.lastError().text());
            return false;
        }
        if (q.value(0).toInt() > 0) {
            setError(error, QStringLiteral("Ce match a déjà un schéma tactique."));
            return false;
        }
    }
    if (board.sessionExerciseId > 0 && (board.exerciseId > 0 || board.sessionId > 0)) {
        setError(error, QStringLiteral("Le terrain d'un bloc ne peut pas être aussi un schéma de référence "
                                       "ni être lié à une séance."));
        return false;
    }
    // Les contrôles d'unicité ne portent que sur un lien nouveau : un schéma ancien déjà lié reste enregistrable.
    if (board.sessionExerciseId > 0 && board.sessionExerciseId != previousBlockId) {
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral("SELECT COUNT(*) FROM tactical_board WHERE session_exercise_id = ? AND id != ?"));
        q.addBindValue(board.sessionExerciseId);
        q.addBindValue(board.id);
        if (!q.exec() || !q.next()) {
            setError(error, q.lastError().text());
            return false;
        }
        if (q.value(0).toInt() > 0) {
            setError(error, QStringLiteral("Ce bloc a déjà un terrain."));
            return false;
        }
    }
    if (board.exerciseId > 0 && board.exerciseId != previousExerciseId) {
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral(
            "SELECT COUNT(*) FROM tactical_board WHERE exercise_id = ? AND session_exercise_id IS NULL AND id != ?"));
        q.addBindValue(board.exerciseId);
        q.addBindValue(board.id);
        if (!q.exec() || !q.next()) {
            setError(error, q.lastError().text());
            return false;
        }
        if (q.value(0).toInt() > 0) {
            setError(error, QStringLiteral("Cet exercice a déjà un schéma de référence."));
            return false;
        }
    }
    return true;
}

bool BoardRepository::validateAllLinks(bool referenceUniqueness, QStringList *problems, QString *error) const
{
    QString listError;
    const QList<BoardRecord> all = list(&listError, true);
    if (!listError.isEmpty()) {
        setError(error, listError);
        return false;
    }
    bool valid = true;
    for (const BoardRecord &b : all) {
        QString why;
        // Sans unicité de référence, l'exercice lié est traité comme « déjà lié » : seul le lien d'un bloc est contrôlé.
        if (!checkLinks(b, referenceUniqueness ? -1 : b.exerciseId, -1, -1, &why)) {
            valid = false;
            if (problems)
                problems->append(QStringLiteral("terrain n° %1 « %2 » : %3").arg(b.id).arg(b.title, why));
        }
    }
    return valid;
}

bool BoardRepository::insert(BoardRecord &board, QString *error)
{
    if (!checkLinks(board, -1, -1, -1, error))
        return false;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO tactical_board(title, formation, scene_json, exercise_id, session_id, session_exercise_id, "
        "match_id) VALUES(?, ?, ?, ?, ?, ?, ?)"));
    q.addBindValue(board.title.trimmed());
    q.addBindValue(nullIfEmpty(board.formation));
    q.addBindValue(board.sceneJson.isEmpty() ? QStringLiteral("{}") : board.sceneJson);
    q.addBindValue(nullIfNotPositive(board.exerciseId));
    q.addBindValue(nullIfNotPositive(board.sessionId));
    q.addBindValue(nullIfNotPositive(board.sessionExerciseId));
    q.addBindValue(nullIfNotPositive(board.matchId));
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    board.id = q.lastInsertId().toInt();
    return true;
}

bool BoardRepository::update(const BoardRecord &board, QString *error)
{
    BoardRecord previous;
    QString lookupError;
    if (!get(board.id, &previous, &lookupError)) {
        setError(error, lookupError);
        return false;
    }
    if (!checkLinks(board, previous.exerciseId, previous.sessionExerciseId, previous.matchId, error))
        return false;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE tactical_board SET title = ?, formation = ?, scene_json = ?, exercise_id = ?, session_id = ?, "
        "session_exercise_id = ?, match_id = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?"));
    q.addBindValue(board.title.trimmed());
    q.addBindValue(nullIfEmpty(board.formation));
    q.addBindValue(board.sceneJson.isEmpty() ? QStringLiteral("{}") : board.sceneJson);
    q.addBindValue(nullIfNotPositive(board.exerciseId));
    q.addBindValue(nullIfNotPositive(board.sessionId));
    q.addBindValue(nullIfNotPositive(board.sessionExerciseId));
    q.addBindValue(nullIfNotPositive(board.matchId));
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
    if (q.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Schéma introuvable."));
        return false;
    }
    return true;
}
