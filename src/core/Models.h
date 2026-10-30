#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

struct Team
{
    int id = -1;
    QString name;
    QString category;   // ex. U15, Seniors
    QString season;     // ex. 2026-2027
};

struct Player
{
    int id = -1;
    int teamId = -1;
    QString firstName;
    QString lastName;
    QDate birthDate;            // date invalide = inconnue
    int jerseyNumber = 0;       // 0 = pas de numéro
    QString position;           // "GK", "DEF", "MID", "FWD" ou vide
    QString preferredFoot;      // "R", "L", "B" ou vide
    QString phone;
    QString email;
    QString notes;
    bool active = true;

    QString fullName() const { return QStringLiteral("%1 %2").arg(firstName, lastName).trimmed(); }
};

struct Session
{
    int id = -1;
    int teamId = -1;
    QDateTime startAt;          // heure locale, stockée en "yyyy-MM-ddTHH:mm"
    int durationMin = 90;
    QString location;
    QString theme;
    QString notes;
    QString status = QStringLiteral("planned");   // "planned", "done", "cancelled"

    QDateTime endAt() const { return startAt.addSecs(qint64(durationMin) * 60); }
};

struct Exercise
{
    int id = -1;
    QString title;
    QString category = QStringLiteral("technical");   // warmup, technical, tactical, physical, game, cooldown
    QString objective;
    int durationMin = 0;        // durée conseillée, 0 = non précisée
    int playersMin = 0;         // 0 = non précisé
    int playersMax = 0;
    QString equipment;
    QString description;
    bool archived = false;
};

// Une ligne du plan d'une séance : un exercice de la bibliothèque, à une position, avec sa durée du jour.
struct PlanEntry
{
    int itemId = -1;            // id de la ligne (table session_exercise)
    int sessionId = -1;
    int position = 0;           // à partir de 1
    int durationMin = 0;        // durée prévue dans CE plan
    QString notes;
    Exercise exercise;          // copie des champs de l'exercice (même archivé)
};
