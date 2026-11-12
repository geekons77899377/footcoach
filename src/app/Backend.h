#pragma once

#include "core/BoardRepository.h"
#include "core/AttendanceRepository.h"
#include "core/ClubRepository.h"
#include "core/MatchRepository.h"
#include "core/ExerciseRepository.h"
#include "core/PlanRepository.h"
#include "core/PlayerRepository.h"
#include "core/SessionRepository.h"
#include "core/TrainingSlotRepository.h"
#include "core/TeamRepository.h"

#include <QJsonObject>
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

    // Liste des schémas, triés par titre : {id, title, updatedAt, exerciseId, sessionId, sessionExerciseId, readable, itemCount, json}.
    // Les terrains des blocs de séance n'y figurent pas (voir plan() : boardId, boardJson).
    Q_INVOKABLE QVariantList boards();
    // {id, title, json, readable, error}. readable == false : le JSON stocké est illisible (le schéma est alors
    // à ne pas modifier, pour ne pas écraser ce qu'il contient). Map vide si l'id n'existe pas.
    Q_INVOKABLE QVariantMap board(int id);

    // Schéma de référence d'un exercice : {boardId, title, json, readable}, boardId = 0 si l'exercice n'en a pas.
    Q_INVOKABLE QVariantMap exerciseBoard(int exerciseId);
    // Crée le schéma de référence (vide) d'un exercice qui n'en a pas, ou renvoie l'existant : {ok, boardId, created, error}.
    Q_INVOKABLE QVariantMap createExerciseBoard(int exerciseId);

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
    // Clubs : [{id, name, teamCount}], triés par nom. Un club regroupe des équipes ; toute équipe appartient à un club.
    Q_INVOKABLE QVariantList clubs();
    // Crée (id absent ou <= 0) ou renomme un club : {id, name}. Résultat {ok, id, error}.
    Q_INVOKABLE QVariantMap saveClub(const QVariantMap &club);
    // Supprime le club et, en cascade, ses équipes, joueurs, séances et matchs. Refusé pour le dernier club.
    Q_INVOKABLE QVariantMap deleteClub(int id);
    // Équipes : [{id, clubId, name, category, season, playerCount}], triées comme dans la base ; clubId > 0 : celles de
    // ce club seulement.
    Q_INVOKABLE QVariantList teams(int clubId = -1);
    // Crée (id absent ou <= 0) ou modifie une équipe : {id, clubId (création : premier club si absent), name, category,
    // season}. Le club d'une équipe existante ne change pas. Résultat {ok, id, error}.
    Q_INVOKABLE QVariantMap saveTeam(const QVariantMap &team);
    Q_INVOKABLE QVariantMap deleteTeam(int id);         // supprime aussi joueurs, séances et présences (cascade)
    // Joueurs d'une équipe : [{id, teamId, firstName, lastName, fullName, birthDate (yyyy-MM-dd ou vide),
    // age (-1 si inconnu), jerseyNumber (0 = aucun), position, preferredFoot, phone, email, notes, active}].
    Q_INVOKABLE QVariantList players(int teamId, bool includeInactive);
    // Crée (id absent ou <= 0) ou modifie un joueur. Refus : prénom vide (le nom est facultatif), prénom + nom déjà dans l'équipe, date invalide, numéro hors 0..99
    // ou déjà porté par un autre joueur actif de l'équipe, poste ou pied inconnu. Résultat {ok, id, error}.
    Q_INVOKABLE QVariantMap savePlayer(const QVariantMap &player);
    // Lit l'image choisie (file:// ou content://), la réduit (480 px, JPEG) ; {ok, error, data = data URL JPEG}.
    Q_INVOKABLE QVariantMap preparePhoto(const QString &url);
    // Import en lot : fichiers « <licence>.jpg » rattachés aux joueurs de l'équipe ; {ok, error, added, replaced, failed, items}.
    Q_INVOKABLE QVariantMap importPhotos(int teamId, const QVariantList &urls);
    Q_INVOKABLE QString playerPhoto(int id);        // data URL JPEG, "" si pas de photo
    // Import d'une liste collée : un joueur par ligne, champs séparés par « ; » ou une tabulation, dans l'ordre
    // Prénom ; Nom ; Numéro ; Poste ; Naissance (JJ/MM/AAAA ou AAAA-MM-JJ). Seul le prénom est obligatoire ; une première ligne
    // « Prénom ; Nom … » est ignorée. Prénom + nom unique dans l'équipe (accents et majuscules ignorés), numéro unique parmi
    // les joueurs actifs. TOUT OU RIEN : à la moindre ligne refusée rien n'est créé. Résultat {ok, error, created,
    // errors: [{line, message}]}.
    Q_INVOKABLE QVariantMap importPlayers(int teamId, const QString &text);
    Q_INVOKABLE QVariantMap setPlayerActive(int id, bool active);   // la réactivation vérifie le numéro
    Q_INVOKABLE QVariantMap deletePlayer(int id);                   // définitif (présences et blessures aussi)

    // --- Séances (calendrier) ---------------------------------------------------------------------
    // Séances de l'équipe dont le jour est entre from et to (AAAA-MM-JJ, inclus), par date :
    // [{id, teamId, date, time, endTime, durationMin, location, theme, notes, status, present, total, slotId}]
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
    // Depuis le schéma 10, createRecurring enregistre aussi un créneau régulier (résultat : slotId) et y relie
    // les séances créées.
    Q_INVOKABLE QVariantMap createRecurring(const QVariantMap &rule, bool force);

    // --- Créneaux réguliers (schéma 10) ---------------------------------------------------------------
    // « À venir » = séance dont le jour est aujourd'hui ou plus tard. Une séance avec un appel enregistré n'est
    // jamais modifiée ni supprimée par ces fonctions.
    // Liste : [{id, teamId, weekdays [1..7], time, endTime, durationMin, location, theme, from, to,
    //           total, upcoming, withAttendance}].
    Q_INVOKABLE QVariantList trainingSlots(int teamId);
    Q_INVOKABLE QVariantMap trainingSlot(int id);         // même contenu, plus ok/error
    // Modification d'un créneau : la nouvelle règle (même forme que pour createRecurring) s'applique aux séances
    // à venir. Jour gardé : heure, durée, lieu et thème mis à jour (sauf champ modifié à la main sur cette séance) ;
    // jour retiré : séance supprimée ; jour nouveau : séance créée. Aperçu et application :
    // {ok, error, updated, added, removed, removedWithPlan, kept, clashes, needsConfirm}.
    Q_INVOKABLE QVariantMap previewSlotUpdate(int slotId, const QVariantMap &rule);
    Q_INVOKABLE QVariantMap updateSlot(int slotId, const QVariantMap &rule, bool force);
    // Déprogrammation : {ok, error, upcoming, past, kept, withPlan} (séances supprimables à venir, passées, gardées
    // car elles ont un appel, supprimables ayant un plan). deleteSlot supprime les séances à venir sans appel (et
    // les passées sans appel si includePast), puis le créneau ; les séances gardées deviennent des séances seules.
    Q_INVOKABLE QVariantMap previewSlotDelete(int slotId);
    Q_INVOKABLE QVariantMap deleteSlot(int slotId, bool includePast);

    // --- Matchs ----------------------------------------------------------------------------------------
    // Matchs de l'équipe dont le jour est entre from et to (AAAA-MM-JJ, inclus), par date : [{id, teamId, date, time,
    // opponent, location, homeAway (home|away), status (planned|played|cancelled), scoreFor, scoreAgainst (-1 si
    // inconnu), notes, convoked (nombre de joueurs convoqués), hasBoard}].
    Q_INVOKABLE QVariantList matches(int teamId, const QString &from, const QString &to);
    Q_INVOKABLE QStringList matchDates(int teamId, const QString &from, const QString &to);
    // Un match avec ses convoqués : {ok, error, match: {...}, players: [{playerId, name, jerseyNumber, role}], boardId}.
    Q_INVOKABLE QVariantMap match(int id);
    // Crée (id absent ou <= 0) ou modifie un match : {id, teamId, date, time (HH:mm), opponent, location, homeAway,
    // status, scoreFor, scoreAgainst, notes}. Résultat {ok, id, error}.
    Q_INVOKABLE QVariantMap saveMatch(const QVariantMap &match);
    Q_INVOKABLE QVariantMap deleteMatch(int id);         // supprime aussi convocations et schéma tactique
    // Remplace les convoqués : entries [{playerId, role (starter|substitute)}]. Refus (rien n'est modifié) si un
    // joueur n'est pas de l'équipe du match.
    Q_INVOKABLE QVariantMap setMatchPlayers(int matchId, const QVariantList &entries);
    // Feuille de challenge 2/2 d'un match (PDF noir et blanc, voir ChallengeSheet) : joueurs = convoqués du match, ordre
    // alphabétique. Contrôle : {ok, error, count, problems [{name, message}], teamName, fileName}. Refus (ok faux) si aucun
    // convoqué, plus de 12, ou si l'un n'a pas un n° de licence de 10 chiffres (problems les liste). Export : mêmes clés,
    // écrit le PDF dans le fichier choisi (file://, content:// ou chemin) et ajoute bytes.
    Q_INVOKABLE QVariantMap challengeSheetCheck(int matchId);
    Q_INVOKABLE QString challengeSheetTempUrl(const QString &fileName);
    Q_INVOKABLE QVariantMap exportChallengeSheet(int matchId, const QString &url);

    // Schéma tactique du match, créé à la demande (terrain vide, titre « Match contre … »). Idempotent :
    // {ok, error, id, boardId, created}. L'ouverture, l'enregistrement et le retrait se font avec board(), saveBoard()
    // et deleteBoard().
    Q_INVOKABLE QVariantMap createMatchBoard(int matchId);

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
    // Catégories d'exercices définies par l'utilisateur (ex. U10-U11) : [{id, name, count}] ; « Activité » (échauffement,
    // technique...) reste exerciseCategories(). saveExercise accepte categoryId (0 : aucune).
    Q_INVOKABLE QVariantList categories();
    Q_INVOKABLE QVariantMap addCategory(const QString &name);              // {ok, id, error}
    Q_INVOKABLE QVariantMap renameCategory(int id, const QString &name);
    Q_INVOKABLE QVariantMap deleteCategory(int id);                        // les exercices restent, sans catégorie
    Q_INVOKABLE QVariantMap saveExercise(const QVariantMap &exercise);   // id < 0 : création
    Q_INVOKABLE QVariantMap setExerciseArchived(int id, bool archived);
    Q_INVOKABLE QVariantMap deleteExercise(int id);      // refusé si l'exercice est dans un plan

    // --- Plan de séance -------------------------------------------------------------------------------
    // {ok, error, session: {...}, entries: [{itemId, position, start, end (HH:mm), durationMin, notes, exerciseId,
    // title, category, categoryLabel, objective, equipment, description, archived,
    // boardId (terrain du bloc, -1 s'il n'en a pas), boardReadable, boardJson (JSON normalisé, vide si illisible)}],
    // totalMin, sessionMin,
    // equipment: [matériel à prévoir, sans doublon]}.
    Q_INVOKABLE QVariantMap plan(int sessionId);
    // durationMin <= 0 : la durée conseillée de l'exercice, ou 15 min à défaut.
    Q_INVOKABLE QVariantMap addToPlan(int sessionId, int exerciseId, int durationMin);
    Q_INVOKABLE QVariantMap updatePlanItem(int itemId, int durationMin, const QString &notes);
    Q_INVOKABLE QVariantMap removePlanItem(int itemId);
    Q_INVOKABLE QVariantMap movePlanItem(int itemId, int delta);       // -1 : monte, +1 : descend
    // Terrain propre au bloc itemId. Créé seulement à la demande de l'entraîneur (règle B) : terrain vide, au nom de
    // l'exercice. Idempotent : si le bloc a déjà un terrain, c'est celui-là qui est renvoyé (created = false).
    // {ok, error, id: id du bloc, boardId, created}. Refusé si le bloc n'existe pas ou si la séance est annulée.
    // L'ouverture, l'enregistrement et le retrait se font avec board(), saveBoard() et deleteBoard().
    Q_INVOKABLE QVariantMap createBlockBoard(int itemId);

    // --- Écran ------------------------------------------------------------------------------------------
    // Garde l'écran allumé tant que la séance du jour est affichée (Android : drapeau FLAG_KEEP_SCREEN_ON de la
    // fenêtre, sans permission ; il s'efface seul quand l'application quitte l'avant-plan). Sans effet ailleurs.
    Q_INVOKABLE void setKeepScreenOn(bool on);

    // --- Transfert de données entre appareils (téléphone ↔ tablette) ---------------------------------------
    // Fichier « .footcoach » : tout le contenu de la base en JSON versionné. url : chemin local, file:/// ou
    // content:// (sélecteur de fichiers Android).
    Q_INVOKABLE QString suggestedExportName() const;
    Q_INVOKABLE QVariantMap exportDataFile(const QString &url);
    // Lit un fichier sans rien modifier : {ok, error, exportedAt, appVersion, counts: {teams, players, sessions,
    // exercises, boards}, current: {…mêmes clés pour la base actuelle}}.
    Q_INVOKABLE QVariantMap inspectDataFile(const QString &url);
    // REMPLACE toutes les données de l'appareil par celles du fichier, en une seule transaction (tout ou rien).
    // Une sauvegarde de l'état actuel est écrite avant : {ok, error, backupName}.
    Q_INVOKABLE QVariantMap importDataFile(const QString &url);

signals:
    void lastErrorChanged();
    void dataReplaced();               // les données ont été remplacées par un import : relire les listes

private:
    QByteArray buildExport(int *schema = nullptr);
    bool createBlockBoardFor(int itemId, int exerciseId, const QString &title, QString *error);
    bool parseDataFile(const QString &url, QJsonObject *root, QString *error) const;
    QVariantMap countsOfDatabase();
    QVariantMap planSlotUpdate(int slotId, const QVariantMap &rule, bool apply, bool force);
    bool find(int id, BoardRecord *out);
    void setError(const QString &message);

    QSqlDatabase m_db;
    BoardRepository m_repo;
    TeamRepository m_teams;
    ClubRepository m_clubs;
    MatchRepository m_matches;
    PlayerRepository m_players;
    SessionRepository m_sessions;
    TrainingSlotRepository m_slots;
    AttendanceRepository m_attendance;
    ExerciseRepository m_exercises;
    PlanRepository m_plan;
    QString m_lastError;
};
