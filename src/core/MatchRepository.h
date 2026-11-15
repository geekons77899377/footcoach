#pragma once

#include "core/Models.h"

#include <QDate>
#include <QList>
#include <QSet>
#include <QSqlDatabase>

// Matchs (table football_match) et joueurs convoqués (match_player). Un match est distinct d'une séance : aucune
// de ses données n'est partagée avec training_session.
// Schéma 14 : un match occupe ses joueurs de son coup d'envoi à la fin de sa durée. Un joueur ne peut pas être convoqué
// à deux matchs dont les créneaux se chevauchent ; il peut l'être à un match qui commence quand l'autre est fini.
// Les matchs annulés ne comptent pas.
constexpr int kMatchMinDuration = 1;
constexpr int kMatchMaxDuration = 600;

class MatchRepository
{
public:
    explicit MatchRepository(const QSqlDatabase &db) : m_db(db) {}

    // Durée par défaut d'un match selon la catégorie de l'équipe : 30 min pour les U10 / U11 (« U10-U11 », « U10U11 »…),
    // 90 min sinon (décision du 15/11/2026).
    static int defaultDuration(const QString &teamCategory);
    // SQL de la migration 014, rejoué à l'import d'un fichier plus ancien : durée par défaut selon la catégorie.
    static QString defaultDurationUpdateSql();

    // Match vide (id -1) s'il n'existe pas ou en cas d'erreur (message dans *error).
    Match get(int id, QString *error = nullptr) const;
    // Matchs de l'équipe dont le jour est entre from et to inclus, par date.
    QList<Match> listBetween(int teamId, const QDate &from, const QDate &to, QString *error = nullptr) const;
    QSet<QDate> datesWithMatches(int teamId, const QDate &from, const QDate &to) const;

    // Refus (rien n'est écrit, message dans *error) : adversaire vide, date invalide, rôle ou statut inconnu, score
    // négatif ou un seul des deux scores renseigné, durée hors de 1 à 600 min, équipe introuvable, ou nouveau créneau
    // qui chevauche un autre match d'un de ses convoqués.
    bool insert(Match &match, QString *error = nullptr);       // renseigne match.id
    bool update(const Match &match, QString *error = nullptr); // ne change pas l'équipe
    // Supprime le match, ses convocations et son terrain (cascade). false si le match n'existe pas.
    bool remove(int id, QString *error = nullptr);

    // Joueurs convoqués, titulaires d'abord.
    QList<MatchPlayer> players(int matchId, QString *error = nullptr) const;
    // Remplace la liste des convoqués, en une transaction. Refus (rien n'est modifié) si un joueur n'est pas de
    // l'équipe du match, apparaît deux fois, si le rôle est inconnu, ou si un joueur est déjà convoqué à un autre match
    // dont le créneau chevauche celui-ci.
    bool setPlayers(int matchId, const QList<MatchPlayer> &players, QString *error = nullptr);

private:
    // Message décrivant le premier conflit de créneau des joueurs avec un autre match, vide s'il n'y en a pas.
    QString slotConflict(int matchId, const QDateTime &startAt, int durationMin, const QList<int> &playerIds,
                         QString *dbError) const;

    QSqlDatabase m_db;
};
