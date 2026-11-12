// Séances, appel et statistiques de présence de Backend (voir Backend.h).
#include "Backend.h"

#include "core/AttendanceStats.h"
#include "core/Recurrence.h"

#include <QHash>
#include <QSqlError>
#include <QSqlQuery>

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
    m.insert(QStringLiteral("slotId"), s.slotId);
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
    // Une séance modifiée garde son créneau (le formulaire ne le connaît pas).
    if (s.id > 0) {
        const Session old = m_sessions.get(s.id);
        if (old.id < 0) {
            setError(tr("Séance introuvable."));
            return result(false, -1, m_lastError);
        }
        s.slotId = old.slotId;
    }

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

// Lit et contrôle une règle hebdomadaire {teamId, from, to, weekdays, time, durationMin, location, theme}.
bool parseRule(const QVariantMap &rule, TrainingSlot *slot, QString *problem)
{
    TrainingSlot t;
    t.teamId = rule.value(QStringLiteral("teamId"), -1).toInt();
    t.firstDay = QDate::fromString(rule.value(QStringLiteral("from")).toString(), kDay);
    t.lastDay = QDate::fromString(rule.value(QStringLiteral("to")).toString(), kDay);
    t.startTime = QTime::fromString(rule.value(QStringLiteral("time")).toString(), QStringLiteral("HH:mm"));
    QList<int> days;
    for (const QVariant &v : rule.value(QStringLiteral("weekdays")).toList())
        days.append(v.toInt());
    t.weekdays = TrainingSlotRepository::weekdaysFromText(TrainingSlotRepository::weekdaysToText(days));
    t.durationMin = rule.value(QStringLiteral("durationMin"), 90).toInt();
    t.location = rule.value(QStringLiteral("location")).toString().trimmed();
    t.theme = rule.value(QStringLiteral("theme")).toString().trimmed();
    if (t.teamId <= 0)
        *problem = QObject::tr("Aucune équipe sélectionnée.");
    else if (t.weekdays.isEmpty())
        *problem = QObject::tr("Cochez au moins un jour.");
    else if (!t.firstDay.isValid() || !t.lastDay.isValid() || !t.startTime.isValid())
        *problem = QObject::tr("Les dates et l'heure doivent être valides.");
    else if (t.lastDay < t.firstDay)
        *problem = QObject::tr("La date de fin précède la date de début.");
    else if (t.durationMin < 15 || t.durationMin > 300)
        *problem = QObject::tr("La durée doit être comprise entre 15 et 300 minutes.");
    *slot = t;
    return problem->isEmpty();
}

// Débuts des séances du créneau entre from et to (inclus). Erreur si trop nombreuses.
QList<QDateTime> starts(const TrainingSlot &t, const QDate &from, const QDate &to, QString *problem)
{
    if (to < from)
        return {};
    const QSet<int> days(t.weekdays.begin(), t.weekdays.end());
    const QList<QDateTime> list = Recurrence::occurrences(from, to, days, t.startTime);
    if (list.size() > Recurrence::kMaxOccurrences) {
        *problem = QObject::tr("Trop de séances (maximum %1) : raccourcissez la période.").arg(Recurrence::kMaxOccurrences);
        return {};
    }
    return list;
}

Session sessionFrom(const TrainingSlot &t, const QDateTime &start)
{
    Session s;
    s.teamId = t.teamId;
    s.startAt = start;
    s.durationMin = t.durationMin;
    s.location = t.location;
    s.theme = t.theme;
    s.slotId = t.id;
    return s;
}

QList<Session> expand(const QVariantMap &rule, TrainingSlot *slot, QString *problem)
{
    QList<Session> out;
    if (!parseRule(rule, slot, problem))
        return out;
    const QList<QDateTime> list = starts(*slot, slot->firstDay, slot->lastDay, problem);
    if (!problem->isEmpty())
        return out;
    if (list.isEmpty()) {
        *problem = QObject::tr("Aucune séance sur cette période pour les jours choisis.");
        return out;
    }
    for (const QDateTime &start : list)
        out.append(sessionFrom(*slot, start));
    return out;
}

QVariantMap slotMap(const TrainingSlot &t, const QList<Session> &sessions, const QHash<int, SessionRepository::Usage> &usage)
{
    QVariantMap m;
    m.insert(QStringLiteral("id"), t.id);
    m.insert(QStringLiteral("teamId"), t.teamId);
    QVariantList days;
    for (int d : t.weekdays)
        days.append(d);
    m.insert(QStringLiteral("weekdays"), days);
    m.insert(QStringLiteral("time"), t.startTime.toString(QStringLiteral("HH:mm")));
    m.insert(QStringLiteral("endTime"), t.startTime.addSecs(qint64(t.durationMin) * 60).toString(QStringLiteral("HH:mm")));
    m.insert(QStringLiteral("durationMin"), t.durationMin);
    m.insert(QStringLiteral("location"), t.location);
    m.insert(QStringLiteral("theme"), t.theme);
    m.insert(QStringLiteral("from"), t.firstDay.toString(kDay));
    m.insert(QStringLiteral("to"), t.lastDay.toString(kDay));
    const QDate today = QDate::currentDate();
    int upcoming = 0, withAttendance = 0;
    for (const Session &s : sessions) {
        if (s.startAt.date() >= today)
            ++upcoming;
        if (usage.value(s.id).attendance)
            ++withAttendance;
    }
    m.insert(QStringLiteral("total"), int(sessions.size()));
    m.insert(QStringLiteral("upcoming"), upcoming);
    m.insert(QStringLiteral("withAttendance"), withAttendance);
    return m;
}

} // namespace

QVariantMap Backend::previewRecurrence(const QVariantMap &rule)
{
    QString problem;
    TrainingSlot slot;
    const QList<Session> list = expand(rule, &slot, &problem);
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
    TrainingSlot slot;
    QList<Session> list = expand(rule, &slot, &problem);
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
    // Créneau et séances : tout ou rien.
    QString error;
    bool ok = m_db.transaction();
    if (!ok)
        error = m_db.lastError().text();
    if (ok)
        ok = m_slots.insert(slot, &error);
    for (Session &s : list) {
        if (!ok)
            break;
        s.slotId = slot.id;
        ok = m_sessions.insert(s, &error);
    }
    if (ok && !m_db.commit()) {
        ok = false;
        error = m_db.lastError().text();
    }
    if (!ok)
        m_db.rollback();
    setError(error);
    QVariantMap r = ok ? result(true, list.first().id) : result(false, -1, error);
    r.insert(QStringLiteral("count"), ok ? int(list.size()) : 0);
    r.insert(QStringLiteral("clashes"), clashes);
    r.insert(QStringLiteral("slotId"), ok ? slot.id : -1);
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

// --- Créneaux réguliers (schéma 10) ------------------------------------------------------------------

QVariantList Backend::trainingSlots(int teamId)
{
    QVariantList out;
    QString error;
    for (const TrainingSlot &t : m_slots.list(teamId, &error))
        out.append(slotMap(t, m_sessions.listBySlot(t.id, &error), m_sessions.usageOfSlot(t.id, &error)));
    setError(error);
    return out;
}

QVariantMap Backend::trainingSlot(int id)
{
    QString error;
    const TrainingSlot t = m_slots.get(id, &error);
    if (t.id < 0) {
        setError(error.isEmpty() ? tr("Créneau introuvable.") : error);
        return result(false, id, m_lastError);
    }
    QVariantMap m = slotMap(t, m_sessions.listBySlot(id, &error), m_sessions.usageOfSlot(id, &error));
    setError(error);
    m.insert(QStringLiteral("ok"), error.isEmpty());
    m.insert(QStringLiteral("error"), error);
    return m;
}

QVariantMap Backend::planSlotUpdate(int slotId, const QVariantMap &rule, bool apply, bool force)
{
    auto fail = [&](const QString &message) {
        if (apply)
            setError(message);
        QVariantMap r = result(false, slotId, message);
        for (const char *k : {"updated", "added", "removed", "removedWithPlan", "kept", "clashes"})
            r.insert(QLatin1String(k), 0);
        return r;
    };
    QString error;
    const TrainingSlot old = m_slots.get(slotId, &error);
    if (old.id < 0)
        return fail(error.isEmpty() ? tr("Créneau introuvable.") : error);
    QVariantMap fixedRule = rule;
    fixedRule.insert(QStringLiteral("teamId"), old.teamId);       // un créneau ne change pas d'équipe
    TrainingSlot next;
    QString problem;
    if (!parseRule(fixedRule, &next, &problem))
        return fail(problem);
    next.id = old.id;

    // Seules les séances à venir changent ; les séances passées restent telles quelles.
    const QDate today = QDate::currentDate();
    const QList<QDateTime> wanted = starts(next, std::max(today, next.firstDay), next.lastDay, &problem);
    if (!problem.isEmpty())
        return fail(problem);
    QHash<QDate, QDateTime> wantedByDay;
    for (const QDateTime &d : wanted)
        wantedByDay.insert(d.date(), d);

    const QList<Session> existing = m_sessions.listBySlot(slotId, &error);
    const QHash<int, SessionRepository::Usage> usage = m_sessions.usageOfSlot(slotId, &error);
    if (!error.isEmpty())
        return fail(error);

    QList<Session> toUpdate, toAdd;
    QList<int> toRemove;
    int removedWithPlan = 0, kept = 0;
    QSet<QDate> covered;            // jours déjà occupés par une séance du créneau (gardée ou mise à jour)
    for (const Session &e : existing) {
        const QDate day = e.startAt.date();
        if (day < today)
            continue;
        if (usage.value(e.id).attendance) {             // appel fait : on n'y touche jamais
            ++kept;
            covered.insert(day);
            continue;
        }
        if (!wantedByDay.contains(day) || covered.contains(day)) {
            toRemove.append(e.id);
            if (usage.value(e.id).plan)
                ++removedWithPlan;
            continue;
        }
        covered.insert(day);
        // Un champ modifié à la main sur cette séance (différent de l'ancien créneau) est respecté.
        Session u = e;
        if (e.startAt.time() == old.startTime)
            u.startAt = QDateTime(day, next.startTime);
        if (e.durationMin == old.durationMin)
            u.durationMin = next.durationMin;
        if (e.location == old.location)
            u.location = next.location;
        if (e.theme == old.theme)
            u.theme = next.theme;
        if (u.startAt != e.startAt || u.durationMin != e.durationMin || u.location != e.location || u.theme != e.theme)
            toUpdate.append(u);
    }
    for (const QDateTime &d : wanted)
        if (!covered.contains(d.date()))
            toAdd.append(sessionFrom(next, d));

    // Chevauchements avec les séances qui n'appartiennent pas à ce créneau.
    int clashes = 0;
    auto clashesWithOthers = [&](const Session &s) {
        if (s.status == QLatin1String("cancelled"))
            return false;
        for (const Session &c : m_sessions.conflicts(s.teamId, s.startAt, s.durationMin, s.id))
            if (c.slotId != slotId)
                return true;
        return false;
    };
    for (const Session &s : std::as_const(toUpdate))
        clashes += clashesWithOthers(s) ? 1 : 0;
    for (const Session &s : std::as_const(toAdd))
        clashes += clashesWithOthers(s) ? 1 : 0;

    QVariantMap r = result(true, slotId);
    r.insert(QStringLiteral("updated"), int(toUpdate.size()));
    r.insert(QStringLiteral("added"), int(toAdd.size()));
    r.insert(QStringLiteral("removed"), int(toRemove.size()));
    r.insert(QStringLiteral("removedWithPlan"), removedWithPlan);
    r.insert(QStringLiteral("kept"), kept);
    r.insert(QStringLiteral("clashes"), clashes);
    if (!apply)
        return r;
    if (clashes > 0 && !force) {
        r.insert(QStringLiteral("ok"), false);
        r.insert(QStringLiteral("needsConfirm"), true);
        r.insert(QStringLiteral("error"), tr("%n séance(s) chevaucheront une autre séance de l'équipe.", nullptr, clashes));
        return r;
    }

    bool ok = m_db.transaction();
    if (!ok)
        error = m_db.lastError().text();
    if (ok)
        ok = m_slots.update(next, &error);
    for (int id : std::as_const(toRemove)) {
        if (!ok)
            break;
        ok = m_sessions.remove(id, &error);
    }
    for (const Session &s : std::as_const(toUpdate)) {
        if (!ok)
            break;
        ok = m_sessions.update(s, &error);
    }
    for (Session &s : toAdd) {
        if (!ok)
            break;
        ok = m_sessions.insert(s, &error);
    }
    if (ok && !m_db.commit()) {
        ok = false;
        error = m_db.lastError().text();
    }
    if (!ok) {
        m_db.rollback();
        return fail(error);
    }
    setError(QString());
    return r;
}

QVariantMap Backend::previewSlotUpdate(int slotId, const QVariantMap &rule)
{
    return planSlotUpdate(slotId, rule, false, false);
}

QVariantMap Backend::updateSlot(int slotId, const QVariantMap &rule, bool force)
{
    return planSlotUpdate(slotId, rule, true, force);
}

namespace {

struct SlotDeletion
{
    QList<int> upcoming, past;      // séances supprimables (sans appel)
    int kept = 0;                   // séances avec appel, conservées
    int withPlanUpcoming = 0, withPlanPast = 0;
};

SlotDeletion planDeletion(const QList<Session> &sessions, const QHash<int, SessionRepository::Usage> &usage)
{
    SlotDeletion d;
    const QDate today = QDate::currentDate();
    for (const Session &s : sessions) {
        const SessionRepository::Usage u = usage.value(s.id);
        if (u.attendance) {
            ++d.kept;
        } else if (s.startAt.date() >= today) {
            d.upcoming.append(s.id);
            d.withPlanUpcoming += u.plan ? 1 : 0;
        } else {
            d.past.append(s.id);
            d.withPlanPast += u.plan ? 1 : 0;
        }
    }
    return d;
}

} // namespace

QVariantMap Backend::previewSlotDelete(int slotId)
{
    QString error;
    const TrainingSlot t = m_slots.get(slotId, &error);
    if (t.id < 0)
        return result(false, slotId, error.isEmpty() ? tr("Créneau introuvable.") : error);
    const SlotDeletion d = planDeletion(m_sessions.listBySlot(slotId, &error), m_sessions.usageOfSlot(slotId, &error));
    if (!error.isEmpty())
        return result(false, slotId, error);
    QVariantMap r = result(true, slotId);
    r.insert(QStringLiteral("upcoming"), int(d.upcoming.size()));
    r.insert(QStringLiteral("past"), int(d.past.size()));
    r.insert(QStringLiteral("kept"), d.kept);
    r.insert(QStringLiteral("withPlan"), d.withPlanUpcoming);
    r.insert(QStringLiteral("withPlanPast"), d.withPlanPast);
    return r;
}

QVariantMap Backend::deleteSlot(int slotId, bool includePast)
{
    QString error;
    const TrainingSlot t = m_slots.get(slotId, &error);
    if (t.id < 0) {
        setError(error.isEmpty() ? tr("Créneau introuvable.") : error);
        return result(false, slotId, m_lastError);
    }
    const SlotDeletion d = planDeletion(m_sessions.listBySlot(slotId, &error), m_sessions.usageOfSlot(slotId, &error));
    if (!error.isEmpty()) {
        setError(error);
        return result(false, slotId, error);
    }
    QList<int> ids = d.upcoming;
    if (includePast)
        ids += d.past;
    bool ok = m_db.transaction();
    if (!ok)
        error = m_db.lastError().text();
    for (int id : std::as_const(ids)) {
        if (!ok)
            break;
        ok = m_sessions.remove(id, &error);
    }
    // Séances restantes : séances seules (la clé étrangère le fait aussi, on ne s'en remet pas qu'à elle).
    if (ok) {
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral("UPDATE training_session SET slot_id = NULL WHERE slot_id = ?"));
        q.addBindValue(slotId);
        ok = q.exec();
        if (!ok)
            error = q.lastError().text();
    }
    if (ok)
        ok = m_slots.remove(slotId, &error);
    if (ok && !m_db.commit()) {
        ok = false;
        error = m_db.lastError().text();
    }
    if (!ok) {
        m_db.rollback();
        setError(error);
        return result(false, slotId, error);
    }
    setError(QString());
    QVariantMap r = result(true, slotId);
    r.insert(QStringLiteral("removed"), int(ids.size()));
    r.insert(QStringLiteral("kept"), int(d.kept + (includePast ? 0 : d.past.size())));
    return r;
}
