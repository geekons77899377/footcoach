#pragma once

#include <QDate>
#include <QtNumeric>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QTime>

struct Club
{
    int id = -1;
    QString name;
    QString department;     // schéma 11 : code du département (« 60 », « 2A »), vide si non renseigné
};

// Schéma 11 : lieu de match (stade) d'un club.
struct Venue
{
    int id = -1;
    int clubId = -1;
    QString name;           // nom de l'installation (ex. « Complexe sportif »)
    QString field;          // nom du terrain (ex. « Terrain d'honneur »), peut être vide
    QString address;
    QString postcode;
    QString city;
    double lat = qQNaN();   // NaN : inconnu
    double lon = qQNaN();
    QString source = QStringLiteral("manual");    // "manual", "data_es"
    QString sourceId;       // Data ES : equip_numero
    QString esStatus = QStringLiteral("ok");      // "ok", "removed" (absent de Data ES), "closed" (hors service)
    bool unavailable = false;                     // marqué indisponible par l'entraîneur
    bool isHome = false;                          // stade du club (matchs à domicile)

    bool selectable() const { return !unavailable && esStatus == QLatin1String("ok"); }
    // Texte recopié dans le lieu du match : « Nom (terrain), adresse, code postal commune ».
    QString label() const;
};

struct Team
{
    int id = -1;
    int clubId = -1;    // club de l'équipe (obligatoire en base) ; -1 à l'insertion : le premier club
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
    QString licence;            // numéro de licence, vide = aucun
    bool hasPhoto = false;      // renseigné par list() ; la photo elle-même se lit avec PlayerRepository::photo()

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
    int slotId = -1;            // créneau régulier qui a généré la séance, -1 = séance seule

    QDateTime endAt() const { return startAt.addSecs(qint64(durationMin) * 60); }
};

// Créneau régulier d'entraînement (schéma 10) : la règle qui génère les séances.
struct TrainingSlot
{
    int id = -1;
    int teamId = -1;
    QList<int> weekdays;        // 1 = lundi … 7 = dimanche, triés
    QTime startTime;
    int durationMin = 90;
    QString location;
    QString theme;
    QDate firstDay;
    QDate lastDay;              // inclus
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
    int categoryId = 0;         // catégorie définie par l'utilisateur (ex. U10-U11), 0 = aucune
    QString categoryName;       // renseigné à la lecture
};

// Catégorie d'exercices définie par l'utilisateur.
struct ExerciseCategory
{
    int id = -1;
    QString name;
    int exerciseCount = 0;      // exercices (archivés compris) qui la portent
};

// Une ligne du plan d'une séance : un exercice de la bibliothèque, à une position, avec sa durée du jour.
struct PlanEntry
{
    int itemId = -1;            // id de la ligne (table session_exercise)
    int sessionId = -1;
    int position = 0;           // à partir de 1
    int durationMin = 0;        // durée prévue dans CE plan
    QString notes;
    int boardId = -1;           // terrain tactique propre à ce bloc (table tactical_board), -1 : aucun
    Exercise exercise;          // copie des champs de l'exercice (même archivé)
};

// Un match : distinct d'une séance et d'un exercice (table football_match).
struct Match
{
    int id = -1;
    int teamId = -1;
    QDateTime startAt;          // heure locale, stockée en "yyyy-MM-ddTHH:mm"
    QString opponent;
    QString location;
    QString homeAway = QStringLiteral("home");        // "home", "away"
    QString status = QStringLiteral("planned");       // "planned", "played", "cancelled"
    int scoreFor = -1;          // -1 : score inconnu (les deux scores vont ensemble)
    int scoreAgainst = -1;
    QString notes;
    int venueId = -1;           // schéma 11 : lieu choisi dans la liste du club, -1 : saisie libre
};

// Un joueur convoqué pour un match.
struct MatchPlayer
{
    int playerId = -1;
    QString role = QStringLiteral("starter");         // "starter" (titulaire), "substitute" (remplaçant)
    // Schéma 10 : "unknown" (sans réponse), "present", "absent", "excused", "injured".
    QString presence = QStringLiteral("unknown");
};
