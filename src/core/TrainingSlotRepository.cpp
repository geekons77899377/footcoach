#include "TrainingSlotRepository.h"

#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>
#include <algorithm>

namespace {

const QString kDay = QStringLiteral("yyyy-MM-dd");
const QString kTime = QStringLiteral("HH:mm");

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

QVariant nullIfEmpty(const QString &s)
{
    return s.trimmed().isEmpty() ? QVariant() : QVariant(s.trimmed());
}

const QString kSelect = QStringLiteral(
    "SELECT id, team_id, weekdays, start_time, duration_min, location, theme, first_day, last_day FROM training_slot");

TrainingSlot readSlot(const QSqlQuery &q)
{
    TrainingSlot s;
    s.id = q.value(0).toInt();
    s.teamId = q.value(1).toInt();
    s.weekdays = TrainingSlotRepository::weekdaysFromText(q.value(2).toString());
    s.startTime = QTime::fromString(q.value(3).toString(), kTime);
    s.durationMin = q.value(4).toInt();
    s.location = q.value(5).toString();
    s.theme = q.value(6).toString();
    s.firstDay = QDate::fromString(q.value(7).toString(), kDay);
    s.lastDay = QDate::fromString(q.value(8).toString(), kDay);
    return s;
}

void bindFields(QSqlQuery &q, const TrainingSlot &s)
{
    q.addBindValue(s.teamId);
    q.addBindValue(TrainingSlotRepository::weekdaysToText(s.weekdays));
    q.addBindValue(s.startTime.toString(kTime));
    q.addBindValue(s.durationMin);
    q.addBindValue(nullIfEmpty(s.location));
    q.addBindValue(nullIfEmpty(s.theme));
    q.addBindValue(s.firstDay.toString(kDay));
    q.addBindValue(s.lastDay.toString(kDay));
}

} // namespace

QString TrainingSlotRepository::weekdaysToText(const QList<int> &days)
{
    QStringList parts;
    for (int d : days)
        if (d >= 1 && d <= 7 && !parts.contains(QString::number(d)))
            parts << QString::number(d);
    std::sort(parts.begin(), parts.end());     // un seul chiffre par jour : l'ordre texte est l'ordre numérique
    return parts.join(QLatin1Char(','));
}

QList<int> TrainingSlotRepository::weekdaysFromText(const QString &text)
{
    QSet<int> seen;
    for (const QString &p : text.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        bool ok = false;
        const int d = p.trimmed().toInt(&ok);
        if (ok && d >= 1 && d <= 7)
            seen.insert(d);
    }
    QList<int> out(seen.begin(), seen.end());
    std::sort(out.begin(), out.end());
    return out;
}

TrainingSlot TrainingSlotRepository::get(int id, QString *error) const
{
    QSqlQuery q(m_db);
    q.prepare(kSelect + QStringLiteral(" WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return TrainingSlot();
    }
    return q.next() ? readSlot(q) : TrainingSlot();
}

QList<TrainingSlot> TrainingSlotRepository::list(int teamId, QString *error) const
{
    QList<TrainingSlot> out;
    QSqlQuery q(m_db);
    q.prepare(kSelect + QStringLiteral(" WHERE team_id = ? ORDER BY weekdays, start_time, id"));
    q.addBindValue(teamId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return out;
    }
    while (q.next())
        out.append(readSlot(q));
    return out;
}

bool TrainingSlotRepository::insert(TrainingSlot &slot, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO training_slot(team_id, weekdays, start_time, duration_min, location, theme, first_day, last_day) "
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?)"));
    bindFields(q, slot);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    slot.id = q.lastInsertId().toInt();
    return true;
}

bool TrainingSlotRepository::update(const TrainingSlot &slot, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE training_slot SET team_id = ?, weekdays = ?, start_time = ?, duration_min = ?, location = ?, theme = ?, "
        "first_day = ?, last_day = ? WHERE id = ?"));
    bindFields(q, slot);
    q.addBindValue(slot.id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool TrainingSlotRepository::remove(int id, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM training_slot WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}
