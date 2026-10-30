#pragma once

#include "core/BoardRepository.h"
#include "core/PlayerRepository.h"
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

signals:
    void lastErrorChanged();

private:
    bool find(int id, BoardRecord *out);
    void setError(const QString &message);

    QSqlDatabase m_db;
    BoardRepository m_repo;
    TeamRepository m_teams;
    PlayerRepository m_players;
    QString m_lastError;
};
