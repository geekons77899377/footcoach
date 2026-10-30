// Exercices et plan de séance de Backend (voir Backend.h).
#include "Backend.h"

#include "core/PlanSchedule.h"

#include <QRegularExpression>
#include <QSet>

namespace {

const QStringList kCategories = {QStringLiteral("warmup"), QStringLiteral("technical"), QStringLiteral("tactical"),
                                 QStringLiteral("physical"), QStringLiteral("game"), QStringLiteral("cooldown")};

QString categoryLabel(const QString &id)
{
    if (id == QLatin1String("warmup")) return QObject::tr("Échauffement");
    if (id == QLatin1String("technical")) return QObject::tr("Technique");
    if (id == QLatin1String("tactical")) return QObject::tr("Tactique");
    if (id == QLatin1String("physical")) return QObject::tr("Physique");
    if (id == QLatin1String("game")) return QObject::tr("Jeu");
    if (id == QLatin1String("cooldown")) return QObject::tr("Retour au calme");
    return id;
}

QVariantMap done(bool ok, int id = -1, const QString &error = QString())
{
    QVariantMap m;
    m.insert(QStringLiteral("ok"), ok);
    m.insert(QStringLiteral("id"), id);
    m.insert(QStringLiteral("error"), error);
    return m;
}

QVariantMap exerciseMap(const Exercise &e, int usage)
{
    QVariantMap m;
    m.insert(QStringLiteral("id"), e.id);
    m.insert(QStringLiteral("title"), e.title);
    m.insert(QStringLiteral("category"), e.category);
    m.insert(QStringLiteral("categoryLabel"), categoryLabel(e.category));
    m.insert(QStringLiteral("objective"), e.objective);
    m.insert(QStringLiteral("durationMin"), e.durationMin);
    m.insert(QStringLiteral("playersMin"), e.playersMin);
    m.insert(QStringLiteral("playersMax"), e.playersMax);
    m.insert(QStringLiteral("equipment"), e.equipment);
    m.insert(QStringLiteral("description"), e.description);
    m.insert(QStringLiteral("archived"), e.archived);
    m.insert(QStringLiteral("usage"), usage);
    return m;
}

// Matériel d'un exercice : éléments séparés par des virgules, points-virgules ou retours à la ligne.
QStringList splitEquipment(const QString &text)
{
    QStringList out;
    const QStringList parts = text.split(QRegularExpression(QStringLiteral("[,;\\n]")), Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        const QString t = p.simplified();
        if (!t.isEmpty())
            out.append(t);
    }
    return out;
}

} // namespace

QVariantList Backend::exerciseCategories() const
{
    QVariantList list;
    for (const QString &id : kCategories) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), id);
        m.insert(QStringLiteral("text"), categoryLabel(id));
        list.append(m);
    }
    return list;
}

QVariantList Backend::exercises(bool includeArchived)
{
    QVariantList list;
    QString error;
    const QList<Exercise> all = m_exercises.list(includeArchived, &error);
    setError(error);
    for (const Exercise &e : all)
        list.append(exerciseMap(e, m_exercises.usageCount(e.id)));
    return list;
}

QVariantMap Backend::saveExercise(const QVariantMap &in)
{
    Exercise e;
    e.id = in.value(QStringLiteral("id"), -1).toInt();
    e.title = in.value(QStringLiteral("title")).toString().simplified();
    e.category = in.value(QStringLiteral("category"), QStringLiteral("technical")).toString();
    e.objective = in.value(QStringLiteral("objective")).toString().trimmed();
    e.durationMin = in.value(QStringLiteral("durationMin"), 0).toInt();
    e.playersMin = in.value(QStringLiteral("playersMin"), 0).toInt();
    e.playersMax = in.value(QStringLiteral("playersMax"), 0).toInt();
    e.equipment = in.value(QStringLiteral("equipment")).toString().trimmed();
    e.description = in.value(QStringLiteral("description")).toString().trimmed();
    e.archived = in.value(QStringLiteral("archived"), false).toBool();

    QString problem;
    if (e.title.isEmpty())
        problem = tr("Le titre est obligatoire.");
    else if (!kCategories.contains(e.category))
        problem = tr("Catégorie inconnue.");
    else if (e.durationMin < 0 || e.durationMin > 300)
        problem = tr("La durée doit être comprise entre 1 et 300 minutes.");
    else if (e.playersMin < 0 || e.playersMax < 0 || e.playersMin > 30 || e.playersMax > 30)
        problem = tr("Le nombre de joueurs doit être compris entre 1 et 30.");
    else if (e.playersMin > 0 && e.playersMax > 0 && e.playersMax < e.playersMin)
        problem = tr("Le nombre maximum de joueurs est inférieur au minimum.");
    if (!problem.isEmpty()) {
        setError(problem);
        return done(false, -1, problem);
    }

    QString error;
    const bool ok = e.id > 0 ? m_exercises.update(e, &error) : m_exercises.insert(e, &error);
    setError(ok ? QString() : error);
    return done(ok, ok ? e.id : -1, error);
}

QVariantMap Backend::setExerciseArchived(int id, bool archived)
{
    QString error;
    const bool ok = m_exercises.setArchived(id, archived, &error);
    setError(ok ? QString() : error);
    return done(ok, id, error);
}

QVariantMap Backend::deleteExercise(int id)
{
    QString error;
    const bool ok = m_exercises.remove(id, &error);
    setError(ok ? QString() : error);
    return done(ok, id, error);
}

QVariantMap Backend::plan(int sessionId)
{
    QString error;
    const Session s = m_sessions.get(sessionId, &error);
    if (s.id < 0) {
        setError(error.isEmpty() ? tr("Séance introuvable.") : error);
        return done(false, -1, m_lastError);
    }
    const QList<PlanEntry> entries = m_plan.entries(sessionId, &error);
    if (!error.isEmpty()) {
        setError(error);
        return done(false, -1, error);
    }

    QList<int> durations;
    for (const PlanEntry &e : entries)
        durations.append(e.durationMin);
    const auto times = PlanSchedule::schedule(s.startAt, durations);

    QVariantList rows;
    QStringList equipment;
    QSet<QString> seen;
    for (int i = 0; i < entries.size(); ++i) {
        const PlanEntry &e = entries.at(i);
        QVariantMap m = exerciseMap(e.exercise, 0);
        m.insert(QStringLiteral("itemId"), e.itemId);
        m.insert(QStringLiteral("position"), e.position);
        m.insert(QStringLiteral("exerciseId"), e.exercise.id);
        m.insert(QStringLiteral("durationMin"), e.durationMin);      // durée de CE plan (écrase celle de l'exercice)
        m.insert(QStringLiteral("suggestedMin"), e.exercise.durationMin);
        m.insert(QStringLiteral("notes"), e.notes);
        m.insert(QStringLiteral("start"), e.durationMin > 0 ? times.at(i).start.time().toString(QStringLiteral("HH:mm")) : QString());
        m.insert(QStringLiteral("end"), e.durationMin > 0 ? times.at(i).end.time().toString(QStringLiteral("HH:mm")) : QString());
        rows.append(m);
        for (const QString &item : splitEquipment(e.exercise.equipment)) {
            const QString key = item.toLower();
            if (!seen.contains(key)) {
                seen.insert(key);
                equipment.append(item);
            }
        }
    }

    QVariantMap out = done(true, sessionId);
    out.insert(QStringLiteral("entries"), rows);
    out.insert(QStringLiteral("totalMin"), PlanSchedule::totalMinutes(durations));
    out.insert(QStringLiteral("sessionMin"), s.durationMin);
    out.insert(QStringLiteral("equipment"), equipment);
    return out;
}

QVariantMap Backend::addToPlan(int sessionId, int exerciseId, int durationMin)
{
    QString error;
    const Session s = m_sessions.get(sessionId, &error);
    if (s.id < 0)
        return done(false, -1, error.isEmpty() ? tr("Séance introuvable.") : error);
    if (s.status == QLatin1String("cancelled"))
        return done(false, -1, tr("Cette séance est annulée."));

    int minutes = durationMin;
    if (minutes <= 0) {
        minutes = 15;
        for (const Exercise &e : m_exercises.list(true, nullptr))
            if (e.id == exerciseId && e.durationMin > 0)
                minutes = e.durationMin;
    }
    if (minutes > 300)
        return done(false, -1, tr("La durée doit être comprise entre 1 et 300 minutes."));

    const int id = m_plan.add(sessionId, exerciseId, minutes, QString(), &error);
    setError(id > 0 ? QString() : error);
    return done(id > 0, id, error);
}

QVariantMap Backend::updatePlanItem(int itemId, int durationMin, const QString &notes)
{
    if (durationMin < 0 || durationMin > 300)
        return done(false, -1, tr("La durée doit être comprise entre 1 et 300 minutes."));
    QString error;
    const bool ok = m_plan.updateItem(itemId, durationMin, notes.left(500), &error);
    setError(ok ? QString() : error);
    return done(ok, itemId, error);
}

QVariantMap Backend::removePlanItem(int itemId)
{
    QString error;
    const bool ok = m_plan.remove(itemId, &error);
    setError(ok ? QString() : error);
    return done(ok, itemId, error);
}

QVariantMap Backend::movePlanItem(int itemId, int delta)
{
    QString error;
    const bool ok = m_plan.move(itemId, delta < 0 ? -1 : 1, &error);
    setError(ok ? QString() : error);
    return done(ok, itemId, error);
}
