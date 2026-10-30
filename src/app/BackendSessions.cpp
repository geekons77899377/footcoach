// Séances, appel et statistiques de présence de Backend (voir Backend.h).
#include "Backend.h"

#include "core/AttendanceStats.h"
#include "core/Recurrence.h"

#include <QDate>
#include <QDateTime>
#include <QSet>
#include <QTime>
#include <algorithm>

namespace {

const QString kDay = QStringLiteral("yyyy-MM-dd");
const QStringList kStatuses = {QStringLiteral("planned"), QStringLiteral("done"), QStringLiteral("cancelled")};
const QStringList kAttendance = {QStringLiteral("present"), QStringLiteral("absent"), QStringLiteral("excused"),
                                 QStringLiteral("injured")};

QVariantMap result(bool ok, int id = -1, const QString &error = QString())
{
    QVariantMap m;
    m.insert(QStringLiteral("ok"), ok);
    m.insert(QStringLiteral("id"), id);
    m.insert(QStringLiteral("error"), error);
    m.insert(QStringLiteral("needsConfirm"), false);
    return m;
}

QVariantMap sessionMap(const Session &s, int present = -1, int total = -1)
{
    QVariantMap m;
    m.insert(QStringLiteral("id"), s.id);
    m.insert(QStringLiteral("teamId"), s.teamId);
    m.insert(QStringLiteral("date"), s.startAt.date().toString(kDay));
    m.insert(QStringLiteral("time"), s.startAt.time().toString(QStringLiteral("HH:mm")));
    m.insert(QStringLiteral("endTime"), s.endAt().time().toString(QStringLiteral("HH:mm")));
    m.insert(QStringLiteral("durationMin"), s.durationMin);
    m.insert(QStringLiteral("location"), s.location);
    m.insert(QStringLiteral("theme"), s.theme);
    m.insert(QStringLiteral("notes"), s.notes);
    m.insert(QStringLiteral("status"), s.status);
    m.insert(QStringLiteral("present"), present);
    m.insert(QStringLiteral("total"), total);
    return m;
}

QString describeClash(const Session &c)
{
    return QObject::tr("Cette séance chevauche une séance existante de l'équipe : le %1 de %2 à %3%4.")
        .arg(c.startAt.date().toString(QStringLiteral("dd/MM/yyyy")), c.startAt.time().toString(QStringLiteral("HH:mm")),
             c.endAt().time().toString(QStringLiteral("HH:mm")),
             c.theme.isEmpty() ? QString() : QStringLiteral(" — ") + c.theme);
}

} // namespace

QVariantList Backend::sessions(int teamId, const QString &from, const QString &to)
{
    QVariantList list;
    const QDate d0 = QDate::fromString(from, kDay), d1 = QDate::fromString(to, kDay);
    if (!d0.isValid() || !d1.isValid())
        return list;
    QString error;
    const QList<Session> all = m_sessions.listBetween(teamId, d0, d1, &error);
    setError(error);
    QList<int> ids;
    for (const Session &s : all)
        ids.append(s.id);
    const auto summary = m_attendance.summary(ids);
    for (const Session &s : all) {
        const auto it = summary.constFind(s.id);
        list.append(it == summary.constEnd() ? sessionMap(s) : sessionMap(s, it->first, it->second));
    }
    return list;
}

QStringList Backend::sessionDates(int teamId, const QString &from, const QString &to)
{
    QStringList out;
    const QDate d0 = QDate::fromString(from, kDay), d1 = QDate::fromString(to, kDay);
    if (!d0.isValid() || !d1.isValid())
        return out;
    for (const QDate &d : m_sessions.datesWithSessions(teamId, d0, d1))
        out.append(d.toString(kDay));
    out.sort();
    return out;
}

QVariantMap Backend::saveSession(const QVariantMap &in, bool force)
{
    Session s;
    s.id = in.value(QStringLiteral("id"), -1).toInt();
    s.teamId = in.value(QStringLiteral("teamId"), -1).toInt();
    const QDate date = QDate::fromString(in.value(QStringLiteral("date")).toString(), kDay);
    const QTime time = QTime::fromString(in.value(QStringLiteral("time")).toString(), QStringLiteral("HH:mm"));
    s.durationMin = in.value(QStringLiteral("durationMin"), 90).toInt();
    s.location = in.value(QStringLiteral("location")).toString().trimmed();
    s.theme = in.value(QStringLiteral("theme")).toString().trimmed();
    s.notes = in.value(QStringLiteral("notes")).toString().trimmed();
    s.status = in.value(QStringLiteral("status"), QStringLiteral("planned")).toString();

    QString problem;
    if (s.teamId <= 0)
        problem = tr("Aucune équipe sélectionnée.");
    else if (!date.isValid())
        problem = tr("La date n'est pas valide.");
    else if (!time.isValid())
        problem = tr("L'heure n'est pas valide.");
    else if (s.durationMin < 15 || s.durationMin > 300)
        problem = tr("La durée doit être comprise entre 15 et 300 minutes.");
    else if (!kStatuses.contains(s.status))
        problem = tr("Statut inconnu.");
    if (!problem.isEmpty()) {
        setError(problem);
        return result(false, -1, problem);
    }
    s.startAt = QDateTime(date, time);

    // Une séance annulée ne bloque personne : on ne contrôle que les autres statuts.
    if (!force && s.status != QLatin1String("cancelled")) {
        const QList<Session> clash = m_sessions.conflicts(s.teamId, s.startAt, s.durationMin, s.id);
        if (!clash.isEmpty()) {
            QVariantMap r = result(false, -1, describeClash(clash.first()));
            r.insert(QStringLiteral("needsConfirm"), true);
            return r;
        }
    }
    QString error;
    const bool ok = s.id > 0 ? m_sessions.update(s, &error) : m_sessions.insert(s, &error);
    setError(error);
    return ok ? result(true, s.id) : result(false, -1, error);
}

QVariantMap Backend::setSessionStatus(int id, const QString &status, bool force)
{
    QString error;
    const Session s = m_sessions.get(id, &error);
    if (s.id < 0 || !kStatuses.contains(status)) {
        setError(error.isEmpty() ? tr("Séance introuvable.") : error);
        return result(false, id, m_lastError);
    }
    if (!force && status != QLatin1String("cancelled") && s.status == QLatin1String("cancelled")) {
        const QList<Session> clash = m_sessions.conflicts(s.teamId, s.startAt, s.durationMin, s.id);
        if (!clash.isEmpty()) {
            QVariantMap r = result(false, id, describeClash(clash.first()));
            r.insert(QStringLiteral("needsConfirm"), true);
            return r;
        }
    }
    const bool ok = m_sessions.setStatus(id, status, &error);
    setError(error);
    return result(ok, id, error);
}

QVariantMap Backend::deleteSession(int id)
{
    QString error;
    const bool ok = m_sessions.remove(id, &error);
    setError(error);
    return result(ok, id, error);
}

namespace {

QList<Session> expand(const QVariantMap &rule, QString *problem)
{
    QList<Session> out;
    const QDate from = QDate::fromString(rule.value(QStringLiteral("from")).toString(), kDay);
    const QDate to = QDate::fromString(rule.value(QStringLiteral("to")).toString(), kDay);
    const QTime time = QTime::fromString(rule.value(QStringLiteral("time")).toString(), QStringLiteral("HH:mm"));
    QSet<int> days;
    for (const QVariant &v : rule.value(QStringLiteral("weekdays")).toList()) {
        const int d = v.toInt();
        if (d >= 1 && d <= 7)
            days.insert(d);
    }
    const int duration = rule.value(QStringLiteral("durationMin"), 90).toInt();
    if (rule.value(QStringLiteral("teamId"), -1).toInt() <= 0)
        *problem = QObject::tr("Aucune équipe sélectionnée.");
    else if (days.isEmpty())
        *problem = QObject::tr("Cochez au moins un jour.");
    else if (!from.isValid() || !to.isValid() || !time.isValid())
        *problem = QObject::tr("Les dates et l'heure doivent être valides.");
    else if (to < from)
        *problem = QObject::tr("La date de fin précède la date de début.");
    else if (duration < 15 || duration > 300)
        *problem = QObject::tr("La durée doit être comprise entre 15 et 300 minutes.");
    if (!problem->isEmpty())
        return out;
    const QList<QDateTime> starts = Recurrence::occurrences(from, to, days, time);
    if (starts.size() > Recurrence::kMaxOccurrences) {
        *problem = QObject::tr("Trop de séances (maximum %1) : raccourcissez la période.").arg(Recurrence::kMaxOccurrences);
        return out;
    }
    if (starts.isEmpty()) {
        *problem = QObject::tr("Aucune séance sur cette période pour les jours choisis.");
        return out;
    }
    for (const QDateTime &start : starts) {
        Session s;
        s.teamId = rule.value(QStringLiteral("teamId")).toInt();
        s.startAt = start;
        s.durationMin = duration;
        s.location = rule.value(QStringLiteral("location")).toString().trimmed();
        s.theme = rule.value(QStringLiteral("theme")).toString().trimmed();
        out.append(s);
    }
    return out;
}

} // namespace

QVariantMap Backend::previewRecurrence(const QVariantMap &rule)
{
    QString problem;
    const QList<Session> list = expand(rule, &problem);
    if (!problem.isEmpty()) {
        QVariantMap r = result(false, -1, problem);
        r.insert(QStringLiteral("count"), 0);
        r.insert(QStringLiteral("clashes"), 0);
        return r;
    }
    int clashes = 0;
    for (const Session &s : list)
        if (!m_sessions.conflicts(s.teamId, s.startAt, s.durationMin).isEmpty())
            ++clashes;
    QVariantMap r = result(true);
    r.insert(QStringLiteral("count"), int(list.size()));
    r.insert(QStringLiteral("clashes"), clashes);
    return r;
}

QVariantMap Backend::createRecurring(const QVariantMap &rule, bool force)
{
    QString problem;
    QList<Session> list = expand(rule, &problem);
    if (!problem.isEmpty()) {
        setError(problem);
        QVariantMap r = result(false, -1, problem);
        r.insert(QStringLiteral("count"), 0);
        return r;
    }
    int clashes = 0;
    for (const Session &s : std::as_const(list))
        if (!m_sessions.conflicts(s.teamId, s.startAt, s.durationMin).isEmpty())
            ++clashes;
    if (clashes > 0 && !force) {
        QVariantMap r = result(false, -1, tr("%1 séance(s) sur %2 chevauchent une séance existante de l'équipe.")
                                              .arg(clashes).arg(list.size()));
        r.insert(QStringLiteral("needsConfirm"), true);
        r.insert(QStringLiteral("count"), int(list.size()));
        r.insert(QStringLiteral("clashes"), clashes);
        return r;
    }
    QString error;
    const bool ok = m_sessions.insertMany(list, &error);
    setError(error);
    QVariantMap r = ok ? result(true, list.first().id) : result(false, -1, error);
    r.insert(QStringLiteral("count"), ok ? int(list.size()) : 0);
    r.insert(QStringLiteral("clashes"), clashes);
    return r;
}

QVariantMap Backend::attendanceSheet(int sessionId)
{
    QString error;
    const Session s = m_sessions.get(sessionId, &error);
    if (s.id < 0) {
        setError(error.isEmpty() ? tr("Séance introuvable.") : error);
        return result(false, sessionId, m_lastError);
    }
    if (s.status == QLatin1String("cancelled")) {
        setError(tr("Cette séance est annulée : on ne peut pas faire l'appel. Changez d'abord son statut si elle a bien eu lieu."));
        return result(false, sessionId, m_lastError);
    }
    const QMap<int, AttendanceRepository::Entry> existing = m_attendance.load(sessionId, &error);
    QVariantList players;
    for (const Player &p : m_players.list(s.teamId, true, &error)) {
        if (!p.active && !existing.contains(p.id))
            continue;
        const auto it = existing.constFind(p.id);
        QVariantMap m;
        m.insert(QStringLiteral("playerId"), p.id);
        m.insert(QStringLiteral("name"), QStringLiteral("%1 %2").arg(p.lastName, p.firstName).trimmed());
        m.insert(QStringLiteral("jerseyNumber"), p.jerseyNumber);
        m.insert(QStringLiteral("active"), p.active);
        m.insert(QStringLiteral("status"), it == existing.constEnd() ? QStringLiteral("absent") : it->status);
        m.insert(QStringLiteral("note"), it == existing.constEnd() ? QString() : it->note);
        players.append(m);
    }
    setError(error);
    if (!error.isEmpty())
        return result(false, sessionId, error);
    if (players.isEmpty()) {
        setError(tr("Cette équipe n'a aucun joueur actif : ajoutez des joueurs dans la page Effectif."));
        return result(false, sessionId, m_lastError);
    }
    QVariantMap r = result(true, sessionId);
    r.insert(QStringLiteral("session"), sessionMap(s));
    r.insert(QStringLiteral("players"), players);
    return r;
}

QVariantMap Backend::saveAttendance(int sessionId, const QVariantList &entries, bool markDone)
{
    QString error;
    const Session s = m_sessions.get(sessionId, &error);
    if (s.id < 0)
        return result(false, sessionId, error.isEmpty() ? tr("Séance introuvable.") : error);
    QList<AttendanceRepository::Entry> list;
    for (const QVariant &v : entries) {
        const QVariantMap m = v.toMap();
        AttendanceRepository::Entry e;
        e.playerId = m.value(QStringLiteral("playerId"), -1).toInt();
        e.status = m.value(QStringLiteral("status")).toString();
        e.note = m.value(QStringLiteral("note")).toString().trimmed();
        if (e.playerId <= 0 || !kAttendance.contains(e.status)) {
            setError(tr("Appel invalide."));
            return result(false, sessionId, m_lastError);
        }
        list.append(e);
    }
    if (!m_attendance.save(sessionId, list, &error)) {
        setError(error);
        return result(false, sessionId, error);
    }
    if (markDone && s.status == QLatin1String("planned") && !m_sessions.setStatus(sessionId, QStringLiteral("done"), &error)) {
        setError(error);
        return result(false, sessionId, error);
    }
    return result(true, sessionId);
}

QVariantMap Backend::attendanceStats(int teamId, const QString &from, const QString &to)
{
    QVariantMap out;
    out.insert(QStringLiteral("sessions"), QVariantList());
    out.insert(QStringLiteral("players"), QVariantList());
    const QDate d0 = QDate::fromString(from, kDay), d1 = QDate::fromString(to, kDay);
    if (!d0.isValid() || !d1.isValid())
        return out;
    QString error;
    const AttendanceStats::Data data = m_attendance.loadData(teamId, d0, d1, &error);
    setError(error);
    const QList<AttendanceStats::PlayerStats> stats = AttendanceStats::compute(data);

    QVariantList sessions;
    for (const auto &s : data.sessions) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), s.id);
        m.insert(QStringLiteral("date"), s.date.toString(kDay));
        sessions.append(m);
    }
    // Joueurs actifs, ou inactifs ayant au moins une séance comptée ; classés par taux décroissant.
    QList<int> shown;
    for (int i = 0; i < data.players.size(); ++i)
        if (data.players.at(i).active || stats.at(i).counted() > 0)
            shown << i;
    std::stable_sort(shown.begin(), shown.end(), [&](int a, int b) { return stats.at(a).rate() > stats.at(b).rate(); });
    QVariantList players;
    for (int i : std::as_const(shown)) {
        const auto &p = data.players.at(i);
        const auto &st = stats.at(i);
        QVariantList cells;
        for (const auto &s : data.sessions)
            cells.append(AttendanceStats::cellStatus(data, p, s));
        QVariantMap m;
        m.insert(QStringLiteral("id"), p.id);
        m.insert(QStringLiteral("name"), p.fullName());
        m.insert(QStringLiteral("jerseyNumber"), p.jerseyNumber);
        m.insert(QStringLiteral("active"), p.active);
        m.insert(QStringLiteral("present"), st.present);
        m.insert(QStringLiteral("absent"), st.absent);
        m.insert(QStringLiteral("excused"), st.excused);
        m.insert(QStringLiteral("injured"), st.injured);
        m.insert(QStringLiteral("counted"), st.counted());
        m.insert(QStringLiteral("rate"), st.rate());
        m.insert(QStringLiteral("cells"), cells);
        players.append(m);
    }
    out.insert(QStringLiteral("sessions"), sessions);
    out.insert(QStringLiteral("players"), players);
    return out;
}
