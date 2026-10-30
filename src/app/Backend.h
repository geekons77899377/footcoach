#pragma once

#include "core/BoardRepository.h"
#include "core/AttendanceRepository.h"
#include "core/ExerciseRepository.h"
#include "core/PlanRepository.h"
#include "core/PlayerRepository.h"
#include "core/SessionRepository.h"
#include "core/TeamRepository.h"

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Passerelle entre l'interface QML et la base de données (schémas tactiques pour l'instant).
//
// Cette classe ne dépend que de QtCore/QtSql : elle est exposée à QML par main.cpp (setContextProperty) et se
// teste avec Qt Test sans interface graphique. Le contenu d'un schéma circule en JSON (Tactics::toJson) : QML
// l'édite en mémoire, ce qui est enregistré est toujours revalidé ici avant d'aller dans la base.
class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit Backend(const QSqlDatabase &db, QObject *parent = nullptr);

    QString lastError() const { return m_lastError; }

    // Liste des schémas, triés par titre : {id, title, updatedAt, exerciseId, sessionId, readable}.
    Q_INVOKABLE QVariantList boards();
    // {id, title, json, readable, error}. readable == false : le JSON stocké est illisible (le schéma est alors
    // à ne pas modifier, pour ne pas écraser ce qu'il contient). Map vide si l'id n'existe pas.
    Q_INVOKABLE QVariantMap board(int id);

    Q_INVOKABLE int createBoard(const QString &title);     // id du nouveau schéma, -1 en cas d'échec
    Q_INVOKABLE int duplicateBoard(int id);                // id de la copie, -1 en cas d'échec
    Q_INVOKABLE bool renameBoard(int id, const QString &title);
    Q_INVOKABLE bool deleteBoard(int id);
    // Enregistre le contenu (JSON de Tactics::Board). Refusé, sans rien écrire, si le JSON n'est pas un schéma valide
    // ou si le schéma stocké est illisible.
    Q_INVOKABLE bool saveBoard(int id, const QString &json);

    // Formations de départ : noms, et JSON {"items": [...]} au format du schéma (joueurs numérotés,
    // adversaires en miroir si demandé). Chaîne vide si le nom est inconnu.
    Q_INVOKABLE QStringList formationNames() const;
    Q_INVOKABLE QString formationJson(const QString &name, bool withOpponents) const;
    // Couleurs proposées : [{name, color}].
    Q_INVOKABLE QVariantList palette() const;

    // --- Équipes et joueurs ---------------------------------------------------------------------
    // Équipes : [{id, name, category, season, playerCount}], triées comme dans la base.
    Q_INVOKABLE QVariantList teams();
    // Crée (id absent ou <= 0) ou modifie une équipe : {id, name, category, season}. Résultat {ok, id, error}.
    Q_INVOKABLE QVariantMap saveTeam(const QVariantMap &team);
    Q_INVOKABLE QVariantMap deleteTeam(int id);         // supprime aussi joueurs, séances et présences (cascade)
    // Joueurs d'une équipe : [{id, teamId, firstName, lastName, fullName, birthDate (yyyy-MM-dd ou vide),
    // age (-1 si inconnu), jerseyNumber (0 = aucun), position, preferredFoot, phone, email, notes, active}].
    Q_INVOKABLE QVariantList players(int teamId, bool includeInactive);
    // Crée (id absent ou <= 0) ou modifie un joueur. Refus : nom ou prénom vide, date invalide, numéro hors 0..99
    // ou déjà porté par un autre joueur actif de l'équipe, poste ou pied inconnu. Résultat {ok, id, error}.
    Q_INVOKABLE QVariantMap savePlayer(const QVariantMap &player);
    Q_INVOKABLE QVariantMap setPlayerActive(int id, bool active);   // la réactivation vérifie le numéro
    Q_INVOKABLE QVariantMap deletePlayer(int id);                   // définitif (présences et blessures aussi)

    // --- Séances (calendrier) ---------------------------------------------------------------------
    // Séances de l'équipe dont le jour est entre from et to (AAAA-MM-JJ, inclus), par date :
    // [{id, teamId, date, time, endTime, durationMin, location, theme, notes, status, present, total}]
    // (present et total valent -1 tant qu'aucun appel n'est enregistré). status : planned, done, cancelled.
    Q_INVOKABLE QVariantList sessions(int teamId, const QString &from, const QString &to);
    // Jours (AAAA-MM-JJ) ayant au moins une séance.
    Q_INVOKABLE QStringList sessionDates(int teamId, const QString &from, const QString &to);
    // Crée (id absent ou <= 0) ou modifie une séance : {id, teamId, date, time (HH:mm), durationMin (15..300),
    // location, theme, notes, status}. Si elle chevauche une autre séance de l'équipe et que force est faux,
    // rien n'est écrit et le résultat est {ok:false, needsConfirm:true, error: description du conflit}.
    Q_INVOKABLE QVariantMap saveSession(const QVariantMap &session, bool force);
    Q_INVOKABLE QVariantMap setSessionStatus(int id, const QString &status, bool force);
    Q_INVOKABLE QVariantMap deleteSession(int id);       // supprime aussi appel et plan d'exercices
    // Séances hebdomadaires : {teamId, from, to, weekdays [1 = lundi … 7], time, durationMin, location, theme}.
    // Aperçu : {ok, count, clashes, error}. Création : même résultat, avec needsConfirm si des séances en
    // chevauchent d'autres et que force est faux.
    Q_INVOKABLE QVariantMap previewRecurrence(const QVariantMap &rule);
    Q_INVOKABLE QVariantMap createRecurring(const QVariantMap &rule, bool force);

    // --- Appel et statistiques de présence ----------------------------------------------------------
    // Feuille d'appel : {ok, error, session: {...}, players: [{playerId, name, jerseyNumber, active, status, note}]}
    // (joueurs actifs + inactifs déjà dans l'appel). Impossible pour une séance annulée.
    Q_INVOKABLE QVariantMap attendanceSheet(int sessionId);
    // entries : [{playerId, status (present, absent, excused, injured), note}] ; markDone passe la séance prévue
    // à « réalisée ».
    Q_INVOKABLE QVariantMap saveAttendance(int sessionId, const QVariantList &entries, bool markDone);
    // Synthèse des séances réalisées entre from et to : {sessions: [{id, date}], players: [{id, name,
    // jerseyNumber, active, present, absent, excused, injured, counted, rate (-1 si inconnu), cells [statut par
    // séance, "" si non concerné]}]}, classés par taux décroissant.
    Q_INVOKABLE QVariantMap attendanceStats(int teamId, const QString &from, const QString &to);

    // --- Exercices ------------------------------------------------------------------------------------
    // Catégories : [{id, text}] (échauffement, technique…).
    Q_INVOKABLE QVariantList exerciseCategories() const;
    // Bibliothèque triée par titre : [{id, title, category, categoryLabel, objective, durationMin, playersMin,
    // playersMax, equipment, description, archived, usage}] ; les archivés seulement sur demande.
    Q_INVOKABLE QVariantList exercises(bool includeArchived);
    Q_INVOKABLE QVariantMap saveExercise(const QVariantMap &exercise);   // id < 0 : création
    Q_INVOKABLE QVariantMap setExerciseArchived(int id, bool archived);
    Q_INVOKABLE QVariantMap deleteExercise(int id);      // refusé si l'exercice est dans un plan

    // --- Plan de séance -------------------------------------------------------------------------------
    // {ok, error, session: {...}, entries: [{itemId, position, start, end (HH:mm), durationMin, notes, exerciseId,
    // title, category, categoryLabel, objective, equipment, description, archived}], totalMin, sessionMin,
    // equipment: [matériel à prévoir, sans doublon]}.
    Q_INVOKABLE QVariantMap plan(int sessionId);
    // durationMin <= 0 : la durée conseillée de l'exercice, ou 15 min à défaut.
    Q_INVOKABLE QVariantMap addToPlan(int sessionId, int exerciseId, int durationMin);
    Q_INVOKABLE QVariantMap updatePlanItem(int itemId, int durationMin, const QString &notes);
    Q_INVOKABLE QVariantMap removePlanItem(int itemId);
    Q_INVOKABLE QVariantMap movePlanItem(int itemId, int delta);       // -1 : monte, +1 : descend

signals:
    void lastErrorChanged();

private:
    bool find(int id, BoardRecord *out);
    void setError(const QString &message);

    QSqlDatabase m_db;
    BoardRepository m_repo;
    TeamRepository m_teams;
    PlayerRepository m_players;
    SessionRepository m_sessions;
    AttendanceRepository m_attendance;
    ExerciseRepository m_exercises;
    PlanRepository m_plan;
    QString m_lastError;
};
