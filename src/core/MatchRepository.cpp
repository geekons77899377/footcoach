#include "MatchRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
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

QVariant nullIfNegative(int v)
{
    return v >= 0 ? QVariant(v) : QVariant();
}

const QString kSelect = QStringLiteral(
    "SELECT id, team_id, start_at, opponent, location, home_away, status, score_for, score_against, notes, venue_id "
    "FROM football_match");

Match readMatch(const QSqlQuery &q)
{
    Match m;
    m.id = q.value(0).toInt();
    m.teamId = q.value(1).toInt();
    m.startAt = QDateTime::fromString(q.value(2).toString(), kDateTimeFormat);
    m.opponent = q.value(3).toString();
    m.location = q.value(4).toString();
    m.homeAway = q.value(5).toString();
    m.status = q.value(6).toString();
    m.scoreFor = q.value(7).isNull() ? -1 : q.value(7).toInt();
    m.scoreAgainst = q.value(8).isNull() ? -1 : q.value(8).toInt();
    m.notes = q.value(9).toString();
    m.venueId = q.value(10).isNull() ? -1 : q.value(10).toInt();
    return m;
}

bool validate(const Match &m, QString *error)
{
    if (m.opponent.trimmed().isEmpty()) {
        setError(error, QStringLiteral("L'adversaire est obligatoire."));
        return false;
    }
    if (!m.startAt.isValid()) {
        setError(error, QStringLiteral("La date et l'heure du match sont invalides."));
        return false;
    }
    if (m.homeAway != QLatin1String("home") && m.homeAway != QLatin1String("away")) {
        setError(error, QStringLiteral("Lieu du match inconnu (domicile ou extérieur)."));
        return false;
    }
    if (m.status != QLatin1String("planned") && m.status != QLatin1String("played")
        && m.status != QLatin1String("cancelled")) {
        setError(error, QStringLiteral("Statut du match inconnu."));
        return false;
    }
    if ((m.scoreFor >= 0) != (m.scoreAgainst >= 0)) {
        setError(error, QStringLiteral("Le score est incomplet : renseignez les deux scores ou aucun."));
        return false;
    }
    return true;
}

} // namespace

QString MatchRepository::slotConflict(int matchId, const QDateTime &startAt, const QList<int> &playerIds,
                                      QString *dbError) const
{
    if (playerIds.isEmpty() || !startAt.isValid())
        return QString();
    for (int pid : playerIds) {
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral(
            "SELECT p.first_name, p.last_name, m.opponent, m.start_at FROM match_player mp "
            "JOIN football_match m ON m.id = mp.match_id JOIN player p ON p.id = mp.player_id "
            "WHERE mp.player_id = ? AND m.id != ? AND m.status != 'cancelled'"));
        q.addBindValue(pid);
        q.addBindValue(matchId);
        if (!q.exec()) {
            setError(dbError, q.lastError().text());
            return QStringLiteral("erreur");
        }
        while (q.next()) {
            const QDateTime other = QDateTime::fromString(q.value(3).toString(), kDateTimeFormat);
            if (other.isValid() && qAbs(other.secsTo(startAt)) < kMatchSlotMinutes * 60)
                return QStringLiteral("%1 %2 est déjà convoqué au match contre %3 (%4).")
                    .arg(q.value(0).toString(), q.value(1).toString(), q.value(2).toString(),
                         other.toString(QStringLiteral("dd/MM/yyyy HH:mm")));
        }
    }
    return QString();
}

Match MatchRepository::get(int id, QString *error) const
{
    QSqlQuery q(m_db);
    q.prepare(kSelect + QStringLiteral(" WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return Match();
    }
    return q.next() ? readMatch(q) : Match();
}

QList<Match> MatchRepository::listBetween(int teamId, const QDate &from, const QDate &to, QString *error) const
{
    QList<Match> matches;
    QSqlQuery q(m_db);
    q.prepare(kSelect + QStringLiteral(" WHERE team_id = ? AND start_at >= ? AND start_at < ? ORDER BY start_at, id"));
    q.addBindValue(teamId);
    q.addBindValue(from.toString(kDateFormat));
    q.addBindValue(to.addDays(1).toString(kDateFormat));
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return matches;
    }
    while (q.next())
        matches.append(readMatch(q));
    return matches;
}

QSet<QDate> MatchRepository::datesWithMatches(int teamId, const QDate &from, const QDate &to) const
{
    QSet<QDate> dates;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT DISTINCT substr(start_at, 1, 10) FROM football_match "
                             "WHERE team_id = ? AND start_at >= ? AND start_at < ?"));
    q.addBindValue(teamId);
    q.addBindValue(from.toString(kDateFormat));
    q.addBindValue(to.addDays(1).toString(kDateFormat));
    if (q.exec())
        while (q.next())
            dates.insert(QDate::fromString(q.value(0).toString(), kDateFormat));
    return dates;
}

bool MatchRepository::insert(Match &match, QString *error)
{
    if (!validate(match, error))
        return false;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO football_match(team_id, start_at, opponent, location, home_away, status, score_for, "
        "score_against, notes, venue_id) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    q.addBindValue(match.teamId);
    q.addBindValue(match.startAt.toString(kDateTimeFormat));
    q.addBindValue(match.opponent.trimmed());
    q.addBindValue(nullIfEmpty(match.location));
    q.addBindValue(match.homeAway);
    q.addBindValue(match.status);
    q.addBindValue(nullIfNegative(match.scoreFor));
    q.addBindValue(nullIfNegative(match.scoreAgainst));
    q.addBindValue(nullIfEmpty(match.notes));
    q.addBindValue(match.venueId > 0 ? QVariant(match.venueId) : QVariant(QMetaType::fromType<int>()));
    if (!q.exec()) {
        // La clé étrangère refuse une équipe inexistante.
        setError(error, q.lastError().text().contains(QLatin1String("FOREIGN KEY"), Qt::CaseInsensitive)
                            ? QStringLiteral("Équipe introuvable.")
                            : q.lastError().text());
        return false;
    }
    match.id = q.lastInsertId().toInt();
    return true;
}

bool MatchRepository::update(const Match &match, QString *error)
{
    if (!validate(match, error))
        return false;
    if (match.status != QLatin1String("cancelled")) {
        // Déplacer le match ne doit pas mettre un joueur déjà convoqué en conflit avec un autre match.
        QList<int> ids;
        for (const MatchPlayer &p : players(match.id))
            ids.append(p.playerId);
        QString dbError;
        const QString conflict = slotConflict(match.id, match.startAt, ids, &dbError);
        if (!conflict.isEmpty()) {
            setError(error, dbError.isEmpty() ? conflict : dbError);
            return false;
        }
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE football_match SET start_at = ?, opponent = ?, location = ?, home_away = ?, status = ?, "
        "score_for = ?, score_against = ?, notes = ?, venue_id = ? WHERE id = ?"));
    q.addBindValue(match.startAt.toString(kDateTimeFormat));
    q.addBindValue(match.opponent.trimmed());
    q.addBindValue(nullIfEmpty(match.location));
    q.addBindValue(match.homeAway);
    q.addBindValue(match.status);
    q.addBindValue(nullIfNegative(match.scoreFor));
    q.addBindValue(nullIfNegative(match.scoreAgainst));
    q.addBindValue(nullIfEmpty(match.notes));
    q.addBindValue(match.venueId > 0 ? QVariant(match.venueId) : QVariant(QMetaType::fromType<int>()));
    q.addBindValue(match.id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (q.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Match introuvable."));
        return false;
    }
    return true;
}

bool MatchRepository::remove(int id, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM football_match WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (q.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Match introuvable."));
        return false;
    }
    return true;
}

QList<MatchPlayer> MatchRepository::players(int matchId, QString *error) const
{
    QList<MatchPlayer> list;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT player_id, role, presence FROM match_player WHERE match_id = ? "
                             "ORDER BY CASE role WHEN 'starter' THEN 0 ELSE 1 END, player_id"));
    q.addBindValue(matchId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return list;
    }
    while (q.next()) {
        MatchPlayer p;
        p.playerId = q.value(0).toInt();
        p.role = q.value(1).toString();
        p.presence = q.value(2).toString();
        list.append(p);
    }
    return list;
}

bool MatchRepository::setPlayers(int matchId, const QList<MatchPlayer> &players, QString *error)
{
    const Match match = get(matchId, error);
    if (match.id < 0) {
        if (error && error->isEmpty())
            setError(error, QStringLiteral("Match introuvable."));
        return false;
    }
    QSet<int> seen;
    for (const MatchPlayer &p : players) {
        if (p.role != QLatin1String("starter") && p.role != QLatin1String("substitute")) {
            setError(error, QStringLiteral("Rôle de joueur inconnu."));
            return false;
        }
        if (seen.contains(p.playerId)) {
            setError(error, QStringLiteral("Un joueur ne peut être convoqué qu'une fois."));
            return false;
        }
        seen.insert(p.playerId);
        QSqlQuery c(m_db);
        c.prepare(QStringLiteral("SELECT COUNT(*) FROM player WHERE id = ? AND team_id = ?"));
        c.addBindValue(p.playerId);
        c.addBindValue(match.teamId);
        if (!c.exec() || !c.next() || c.value(0).toInt() == 0) {
            setError(error, QStringLiteral("Ce joueur n'appartient pas à l'équipe du match."));
            return false;
        }
    }
    if (match.status != QLatin1String("cancelled")) {
        QString dbError;
        const QString conflict = slotConflict(matchId, match.startAt, QList<int>(seen.begin(), seen.end()), &dbError);
        if (!conflict.isEmpty()) {
            setError(error, dbError.isEmpty() ? conflict : dbError);
            return false;
        }
    }
    if (!m_db.transaction()) {
        setError(error, m_db.lastError().text());
        return false;
    }
    bool ok = true;
    QString failure;
    // Les convoqués gardés conservent leur présence (schéma 10) : on retire seulement les joueurs qui ne sont plus
    // convoqués, puis on ajoute ou met à jour le rôle des autres.
    QStringList keep;
    for (const MatchPlayer &p : players)
        keep << QString::number(p.playerId);
    QSqlQuery del(m_db);
    del.prepare(QStringLiteral("DELETE FROM match_player WHERE match_id = ?")
                + (keep.isEmpty() ? QString() : QStringLiteral(" AND player_id NOT IN (%1)").arg(keep.join(QLatin1Char(',')))));
    del.addBindValue(matchId);
    if (!del.exec()) {
        ok = false;
        failure = del.lastError().text();
    }
    for (const MatchPlayer &p : players) {
        if (!ok)
            break;
        QSqlQuery ins(m_db);
        ins.prepare(QStringLiteral("INSERT INTO match_player(match_id, player_id, role) VALUES(?, ?, ?) "
                                   "ON CONFLICT(match_id, player_id) DO UPDATE SET role = excluded.role"));
        ins.addBindValue(matchId);
        ins.addBindValue(p.playerId);
        ins.addBindValue(p.role);
        if (!ins.exec()) {
            ok = false;
            failure = ins.lastError().text();
        }
    }
    if (ok)
        ok = m_db.commit();
    else
        failure = failure.isEmpty() ? m_db.lastError().text() : failure;
    if (!ok) {
        m_db.rollback();
        setError(error, failure.isEmpty() ? m_db.lastError().text() : failure);
        return false;
    }
    return true;
}
