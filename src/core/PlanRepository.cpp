#include "PlanRepository.h"

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

QList<PlanEntry> PlanRepository::entries(int sessionId, QString *error) const
{
    QList<PlanEntry> result;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT se.id, se.position, se.duration_min, se.notes, "
        "e.id, e.title, e.category, e.objective, e.duration_min, e.players_min, e.players_max, "
        "e.equipment, e.description, e.archived "
        "FROM session_exercise se JOIN exercise e ON e.id = se.exercise_id "
        "WHERE se.session_id = ? ORDER BY se.position"));
    q.addBindValue(sessionId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return result;
    }
    while (q.next()) {
        PlanEntry p;
        p.itemId = q.value(0).toInt();
        p.sessionId = sessionId;
        p.position = q.value(1).toInt();
        p.durationMin = q.value(2).isNull() ? 0 : q.value(2).toInt();
        p.notes = q.value(3).toString();
        p.exercise.id = q.value(4).toInt();
        p.exercise.title = q.value(5).toString();
        p.exercise.category = q.value(6).toString();
        p.exercise.objective = q.value(7).toString();
        p.exercise.durationMin = q.value(8).isNull() ? 0 : q.value(8).toInt();
        p.exercise.playersMin = q.value(9).isNull() ? 0 : q.value(9).toInt();
        p.exercise.playersMax = q.value(10).isNull() ? 0 : q.value(10).toInt();
        p.exercise.equipment = q.value(11).toString();
        p.exercise.description = q.value(12).toString();
        p.exercise.archived = q.value(13).toInt() != 0;
        result.append(p);
    }
    return result;
}

int PlanRepository::add(int sessionId, int exerciseId, int durationMin, const QString &notes, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO session_exercise(session_id, exercise_id, position, duration_min, notes) "
        "VALUES(?, ?, (SELECT COALESCE(MAX(position), 0) + 1 FROM session_exercise WHERE session_id = ?), ?, ?)"));
    q.addBindValue(sessionId);
    q.addBindValue(exerciseId);
    q.addBindValue(sessionId);
    q.addBindValue(durationMin > 0 ? QVariant(durationMin) : QVariant());
    q.addBindValue(notes.trimmed().isEmpty() ? QVariant() : QVariant(notes.trimmed()));
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool PlanRepository::updateItem(int itemId, int durationMin, const QString &notes, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE session_exercise SET duration_min = ?, notes = ? WHERE id = ?"));
    q.addBindValue(durationMin > 0 ? QVariant(durationMin) : QVariant());
    q.addBindValue(notes.trimmed().isEmpty() ? QVariant() : QVariant(notes.trimmed()));
    q.addBindValue(itemId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool PlanRepository::renumber(int sessionId, QString *error)
{
    // Après une suppression, les positions suivantes ne font que diminuer : en les traitant dans
    // l'ordre croissant, aucune ne rencontre une position encore occupée (contrainte UNIQUE).
    QSqlQuery sel(m_db);
    sel.prepare(QStringLiteral("SELECT id FROM session_exercise WHERE session_id = ? ORDER BY position"));
    sel.addBindValue(sessionId);
    if (!sel.exec()) {
        setError(error, sel.lastError().text());
        return false;
    }
    QList<int> ids;
    while (sel.next())
        ids.append(sel.value(0).toInt());

    for (int i = 0; i < ids.size(); ++i) {
        QSqlQuery upd(m_db);
        upd.prepare(QStringLiteral("UPDATE session_exercise SET position = ? WHERE id = ?"));
        upd.addBindValue(i + 1);
        upd.addBindValue(ids.at(i));
        if (!upd.exec()) {
            setError(error, upd.lastError().text());
            return false;
        }
    }
    return true;
}

bool PlanRepository::remove(int itemId, QString *error)
{
    QSqlQuery find(m_db);
    find.prepare(QStringLiteral("SELECT session_id FROM session_exercise WHERE id = ?"));
    find.addBindValue(itemId);
    if (!find.exec() || !find.next()) {
        setError(error, find.lastError().text().isEmpty() ? QStringLiteral("Ligne de plan introuvable.")
                                                          : find.lastError().text());
        return false;
    }
    const int sessionId = find.value(0).toInt();

    if (!m_db.transaction()) {
        setError(error, m_db.lastError().text());
        return false;
    }
    QSqlQuery del(m_db);
    del.prepare(QStringLiteral("DELETE FROM session_exercise WHERE id = ?"));
    del.addBindValue(itemId);
    if (!del.exec() || !renumber(sessionId, error)) {
        if (error && error->isEmpty())
            *error = del.lastError().text();
        m_db.rollback();
        return false;
    }
    if (!m_db.commit()) {
        setError(error, m_db.lastError().text());
        m_db.rollback();
        return false;
    }
    return true;
}

bool PlanRepository::move(int itemId, int delta, QString *error)
{
    if (delta != -1 && delta != 1) {
        setError(error, QStringLiteral("Déplacement invalide."));
        return false;
    }

    QSqlQuery cur(m_db);
    cur.prepare(QStringLiteral("SELECT session_id, position FROM session_exercise WHERE id = ?"));
    cur.addBindValue(itemId);
    if (!cur.exec() || !cur.next()) {
        setError(error, QStringLiteral("Ligne de plan introuvable."));
        return false;
    }
    const int sessionId = cur.value(0).toInt();
    const int position = cur.value(1).toInt();

    QSqlQuery nb(m_db);
    nb.prepare(QStringLiteral("SELECT id FROM session_exercise WHERE session_id = ? AND position = ?"));
    nb.addBindValue(sessionId);
    nb.addBindValue(position + delta);
    if (!nb.exec()) {
        setError(error, nb.lastError().text());
        return false;
    }
    if (!nb.next())
        return true;   // déjà en première ou dernière position
    const int otherId = nb.value(0).toInt();

    // Échange en trois temps (position -1 provisoire) pour respecter UNIQUE(session_id, position).
    if (!m_db.transaction()) {
        setError(error, m_db.lastError().text());
        return false;
    }
    const struct { int id; int pos; } steps[] = {{itemId, -1}, {otherId, position}, {itemId, position + delta}};
    for (const auto &st : steps) {
        QSqlQuery upd(m_db);
        upd.prepare(QStringLiteral("UPDATE session_exercise SET position = ? WHERE id = ?"));
        upd.addBindValue(st.pos);
        upd.addBindValue(st.id);
        if (!upd.exec()) {
            setError(error, upd.lastError().text());
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
