#pragma once

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QSet>
#include <QTime>

namespace Recurrence {

// Garde-fou : une génération ne peut pas dépasser ce nombre de séances.
constexpr int kMaxOccurrences = 200;

// Dates/heures de toutes les occurrences entre `from` et `to` (inclus) tombant un des jours donnés
// (valeurs de Qt::DayOfWeek : 1 = lundi … 7 = dimanche), à l'heure `time`.
// Résultat trié chronologiquement. Liste vide si les paramètres sont invalides.
// Si le résultat dépasse kMaxOccurrences, il est tronqué à kMaxOccurrences + 1 éléments
// (suffisant pour que l'appelant détecte le dépassement sans parcourir des années entières).
QList<QDateTime> occurrences(const QDate &from, const QDate &to, const QSet<int> &weekdays, const QTime &time);

} // namespace Recurrence
