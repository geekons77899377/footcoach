#pragma once

#include "core/Models.h"

#include <QDate>
#include <QList>
#include <QSet>
#include <QSqlDatabase>

// Matchs (table football_match) et joueurs convoqués (match_player). Un match est distinct d'une séance : aucune
// de ses données n'est partagée avec training_session.
class MatchRepository
{
public:
    explicit MatchRepository(const QSqlDatabase &db) : m_db(db) {}

    // Match vide (id -1) s'il n'existe pas ou en cas d'erreur (message dans *error).
    Match get(int id, QString *error = nullptr) const;
    // Matchs de l'équipe dont le jour est entre from et to inclus, par date.
    QList<Match> listBetween(int teamId, const QDate &from, const QDate &to, QString *error = nullptr) const;
    QSet<QDate> datesWithMatches(int teamId, const QDate &from, const QDate &to) const;

    // Refus (rien n'est écrit, message dans *error) : adversaire vide, date invalide, rôle ou statut inconnu, score
    // négatif ou un seul des deux scores renseigné, équipe introuvable.
    bool insert(Match &match, QString *error = nullptr);       // renseigne match.id
    bool update(const Match &match, QString *error = nullptr); // ne change pas l'équipe
    // Supprime le match, ses convocations et son terrain (cascade). false si le match n'existe pas.
    bool remove(int id, QString *error = nullptr);

    // Joueurs convoqués, titulaires d'abord.
    QList<MatchPlayer> players(int matchId, QString *error = nullptr) const;
    // Remplace la liste des convoqués, en une transaction. Refus (rien n'est modifié) si un joueur n'est pas de
    // l'équipe du match, apparaît deux fois, ou si le rôle est inconnu.
    bool setPlayers(int matchId, const QList<MatchPlayer> &players, QString *error = nullptr);

private:
    QSqlDatabase m_db;
};
