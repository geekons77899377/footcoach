#include "app/Backend.h"
#include "core/BoardRepository.h"
#include "core/ClubRepository.h"
#include "core/Database.h"
#include "core/MatchRepository.h"
#include "core/PlayerRepository.h"
#include "core/SessionRepository.h"
#include "core/TeamRepository.h"

#include "app/ChallengeSheet.h"

#include <QProcess>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

// Clubs (L4) et matchs (L5) : dépôts, règles de liens des schémas, API du Backend.
class MatchesTest : public QObject
{
    Q_OBJECT

    Database m_db;
    int m_team = -1;
    int m_p1 = -1, m_p2 = -1;

    int count(const QString &sql)
    {
        QSqlQuery q(m_db.handle());
        return q.exec(sql) && q.next() ? q.value(0).toInt() : -1;
    }

    Match makeMatch(const QString &opponent, const QDateTime &when) const
    {
        Match m;
        m.teamId = m_team;
        m.opponent = opponent;
        m.startAt = when;
        return m;
    }

private slots:
    void init()
    {
        QString error;
        QVERIFY2(m_db.open(QStringLiteral(":memory:"), &error), qPrintable(error));
        // Les exemples U10-U11 (migration 008) ont leur propre test : ici on part d'une base sans schéma tactique.
        QSqlQuery(m_db.handle()).exec(QStringLiteral("DELETE FROM tactical_board WHERE exercise_id IN (SELECT id FROM exercise WHERE title LIKE 'U10-U11%')"));

        TeamRepository teams(m_db.handle());
        Team t;
        t.name = QStringLiteral("U15");
        QVERIFY(teams.insert(t));
        m_team = t.id;
        PlayerRepository players(m_db.handle());
        Player a;
        a.teamId = m_team;
        a.firstName = QStringLiteral("Zoé");
        a.lastName = QStringLiteral("Martin");
        QVERIFY(players.insert(a));
        m_p1 = a.id;
        Player b = a;
        b.id = -1;
        b.firstName = QStringLiteral("Yanis");
        QVERIFY(players.insert(b));
        m_p2 = b.id;
    }

    void cleanup() { m_db.close(); }

    // ----- Clubs -----
    void freshDatabaseHasADefaultClubAndTeamsBelongToIt()
    {
        ClubRepository clubs(m_db.handle());
        QCOMPARE(clubs.all().size(), 1);
        TeamRepository teams(m_db.handle());
        QCOMPARE(teams.all().first().clubId, clubs.all().first().id);
    }

    void teamCannotBeCreatedInAnUnknownClub()
    {
        TeamRepository teams(m_db.handle());
        Team t;
        t.name = QStringLiteral("U17");
        t.clubId = 4242;
        QString error;
        QVERIFY(!teams.insert(t, &error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(teams.all().size(), 1);
    }

    void clubCrud()
    {
        ClubRepository clubs(m_db.handle());
        Club c;
        QString error;
        c.name = QStringLiteral("   ");
        QVERIFY(!clubs.insert(c, &error));                        // nom vide refusé
        c.name = QStringLiteral("AS Voisins");
        QVERIFY(clubs.insert(c));
        QVERIFY(c.id > 0);
        c.name = QStringLiteral("AS Voisins 2");
        QVERIFY(clubs.update(c));
        Club read;
        QVERIFY(clubs.get(c.id, &read));
        QCOMPARE(read.name, QStringLiteral("AS Voisins 2"));
        Club ghost;
        ghost.id = 9999;
        ghost.name = QStringLiteral("x");
        QVERIFY(!clubs.update(ghost, &error));                    // 0 ligne modifiée = échec
        QVERIFY(error.contains(QStringLiteral("introuvable")));
    }

    void deletingAClubCascadesToItsTeamsAndTheLastClubIsProtected()
    {
        ClubRepository clubs(m_db.handle());
        Club other;
        other.name = QStringLiteral("Autre club");
        QVERIFY(clubs.insert(other));
        TeamRepository teams(m_db.handle());
        Team t;
        t.name = QStringLiteral("Équipe de l'autre club");
        t.clubId = other.id;
        QVERIFY(teams.insert(t));
        QCOMPARE(teams.all(nullptr, other.id).size(), 1);
        QCOMPARE(teams.all(nullptr, clubs.all().first().id).size(), 1);

        QVERIFY(clubs.remove(other.id));
        QCOMPARE(count("SELECT COUNT(*) FROM team"), 1);          // seule l'équipe du club supprimé a disparu
        QCOMPARE(count("SELECT COUNT(*) FROM team WHERE id = " + QString::number(t.id)), 0);

        QString error;
        QVERIFY(!clubs.remove(clubs.all().first().id, &error));  // dernier club
        QVERIFY(!error.isEmpty());
        QVERIFY(!clubs.remove(9999, &error));                     // inconnu : pas un succès
        QCOMPARE(count("SELECT COUNT(*) FROM club"), 1);
    }

    void deletingAnUnknownTeamFails()
    {
        TeamRepository teams(m_db.handle());
        QString error;
        QVERIFY(!teams.remove(9999, &error));
        QVERIFY(!error.isEmpty());
    }

    // ----- Matchs -----
    void matchCrudAndValidation()
    {
        MatchRepository repo(m_db.handle());
        Match m = makeMatch(QStringLiteral("FC Rivière"), QDateTime(QDate(2026, 11, 8), QTime(15, 0)));
        m.location = QStringLiteral("Stade");
        QVERIFY(repo.insert(m));
        QVERIFY(m.id > 0);
        const Match back = repo.get(m.id);
        QCOMPARE(back.opponent, QStringLiteral("FC Rivière"));
        QCOMPARE(back.startAt, m.startAt);
        QCOMPARE(back.status, QStringLiteral("planned"));
        QCOMPARE(back.scoreFor, -1);

        m.status = QStringLiteral("played");
        m.scoreFor = 2;
        m.scoreAgainst = 1;
        QVERIFY(repo.update(m));
        QCOMPARE(repo.get(m.id).scoreFor, 2);
        QCOMPARE(repo.get(m.id).scoreAgainst, 1);

        QString error;
        Match bad = makeMatch(QString(), m.startAt);
        QVERIFY(!repo.insert(bad, &error));                       // adversaire vide
        bad = makeMatch(QStringLiteral("X"), QDateTime());
        QVERIFY(!repo.insert(bad, &error));                       // date invalide
        bad = makeMatch(QStringLiteral("X"), m.startAt);
        bad.status = QStringLiteral("nimporte");
        QVERIFY(!repo.insert(bad, &error));
        bad = makeMatch(QStringLiteral("X"), m.startAt);
        bad.scoreFor = 3;                                         // un seul score
        QVERIFY(!repo.insert(bad, &error));
        bad = makeMatch(QStringLiteral("X"), m.startAt);
        bad.teamId = 4242;
        QVERIFY(!repo.insert(bad, &error));                       // équipe inconnue
        QCOMPARE(count("SELECT COUNT(*) FROM football_match"), 1);

        Match ghost = m;
        ghost.id = 9999;
        QVERIFY(!repo.update(ghost, &error));
        QVERIFY(!repo.remove(9999, &error));                      // suppression de 0 ligne = échec
        QVERIFY(repo.remove(m.id));
        QVERIFY(repo.get(m.id).id < 0);
    }

    void matchesAreListedByDayAndAreNotSessions()
    {
        MatchRepository repo(m_db.handle());
        Match a = makeMatch(QStringLiteral("A"), QDateTime(QDate(2026, 11, 8), QTime(15, 0)));
        Match b = makeMatch(QStringLiteral("B"), QDateTime(QDate(2026, 11, 9), QTime(23, 30)));
        Match c = makeMatch(QStringLiteral("C"), QDateTime(QDate(2026, 12, 1), QTime(10, 0)));
        QVERIFY(repo.insert(a) && repo.insert(b) && repo.insert(c));
        const QList<Match> list = repo.listBetween(m_team, QDate(2026, 11, 8), QDate(2026, 11, 9));
        QCOMPARE(list.size(), 2);                                 // le dernier jour est inclus, même tard le soir
        QCOMPARE(list.at(0).opponent, QStringLiteral("A"));
        QCOMPARE(repo.datesWithMatches(m_team, QDate(2026, 11, 1), QDate(2026, 11, 30)).size(), 2);
        QCOMPARE(repo.listBetween(m_team + 1, QDate(2026, 1, 1), QDate(2026, 12, 31)).size(), 0);
        // Aucune séance n'a été créée par ces matchs, et inversement.
        SessionRepository sessions(m_db.handle());
        QCOMPARE(sessions.listBetween(m_team, QDate(2026, 1, 1), QDate(2026, 12, 31)).size(), 0);
    }

    void convokedPlayersMustBelongToTheMatchTeam()
    {
        MatchRepository repo(m_db.handle());
        Match m = makeMatch(QStringLiteral("A"), QDateTime(QDate(2026, 11, 8), QTime(15, 0)));
        QVERIFY(repo.insert(m));
        QString error;
        QVERIFY(repo.setPlayers(m.id, {{m_p1, QStringLiteral("starter")}, {m_p2, QStringLiteral("substitute")}}));
        QCOMPARE(repo.players(m.id).size(), 2);
        QCOMPARE(repo.players(m.id).first().role, QStringLiteral("starter"));

        // Joueur d'une autre équipe : refus, la convocation précédente est conservée.
        TeamRepository teams(m_db.handle());
        Team other;
        other.name = QStringLiteral("Seniors");
        QVERIFY(teams.insert(other));
        PlayerRepository players(m_db.handle());
        Player stranger;
        stranger.teamId = other.id;
        stranger.firstName = QStringLiteral("Léo");
        stranger.lastName = QStringLiteral("Autre");
        QVERIFY(players.insert(stranger));
        QVERIFY(!repo.setPlayers(m.id, {{m_p1, QStringLiteral("starter")}, {stranger.id, QStringLiteral("starter")}}, &error));
        QCOMPARE(repo.players(m.id).size(), 2);
        QVERIFY(!repo.setPlayers(m.id, {{m_p1, QStringLiteral("starter")}, {m_p1, QStringLiteral("substitute")}}, &error));
        QVERIFY(!repo.setPlayers(m.id, {{m_p1, QStringLiteral("capitaine")}}, &error));
        QVERIFY(!repo.setPlayers(9999, {}, &error));
        QCOMPARE(repo.players(m.id).size(), 2);

        // Vider la liste est permis.
        QVERIFY(repo.setPlayers(m.id, {}));
        QCOMPARE(repo.players(m.id).size(), 0);
    }

    // Un joueur ne peut être que sur un seul match à la même heure (créneau de kMatchSlotMinutes).
    void aPlayerCannotBeOnTwoMatchesAtTheSameTime()
    {
        MatchRepository repo(m_db.handle());
        Match a = makeMatch(QStringLiteral("A"), QDateTime(QDate(2026, 11, 8), QTime(15, 0)));
        Match same = makeMatch(QStringLiteral("B"), QDateTime(QDate(2026, 11, 8), QTime(15, 0)));
        Match close = makeMatch(QStringLiteral("C"), QDateTime(QDate(2026, 11, 8), QTime(16, 30)));
        Match later = makeMatch(QStringLiteral("D"), QDateTime(QDate(2026, 11, 8), QTime(17, 0)));
        QVERIFY(repo.insert(a) && repo.insert(same) && repo.insert(close) && repo.insert(later));
        QVERIFY(repo.setPlayers(a.id, {{m_p1, QStringLiteral("starter")}}));

        QString error;
        QVERIFY(!repo.setPlayers(same.id, {{m_p1, QStringLiteral("starter")}}, &error));      // même heure
        QVERIFY2(error.contains(QStringLiteral("déjà convoqué")), qPrintable(error));
        QVERIFY(error.contains(QStringLiteral("contre A")));
        QCOMPARE(repo.players(same.id).size(), 0);                                           // rien n'est écrit
        QVERIFY(!repo.setPlayers(close.id, {{m_p1, QStringLiteral("substitute")}}, &error));  // 90 min d'écart : chevauche
        QVERIFY(repo.setPlayers(later.id, {{m_p1, QStringLiteral("starter")}}));              // 120 min d'écart : libre
        QVERIFY(repo.setPlayers(same.id, {{m_p2, QStringLiteral("starter")}}));               // un autre joueur : permis

        // Réenregistrer le même match ne s'oppose pas à lui-même.
        QVERIFY(repo.setPlayers(a.id, {{m_p1, QStringLiteral("starter")}, {m_p2, QStringLiteral("substitute")}}) == false);
        QVERIFY(repo.setPlayers(a.id, {{m_p1, QStringLiteral("substitute")}}));

        // Déplacer un match convoqué sur le créneau d'un autre est refusé, sans rien modifier.
        Match moved = repo.get(later.id);
        moved.startAt = QDateTime(QDate(2026, 11, 8), QTime(15, 30));
        QVERIFY(!repo.update(moved, &error));
        QCOMPARE(repo.get(later.id).startAt, QDateTime(QDate(2026, 11, 8), QTime(17, 0)));

        // Un match annulé libère ses joueurs ; le rétablir est refusé si le créneau est repris.
        Match cancelled = repo.get(a.id);
        cancelled.status = QStringLiteral("cancelled");
        QVERIFY(repo.update(cancelled));
        QVERIFY(repo.setPlayers(same.id, {{m_p1, QStringLiteral("starter")}}));                // 15:00, comme le match annulé
        cancelled.status = QStringLiteral("planned");
        QVERIFY(!repo.update(cancelled, &error));
        QCOMPARE(repo.get(a.id).status, QStringLiteral("cancelled"));
    }

    void deletingAPlayerOrAMatchCleansConvocations()
    {
        MatchRepository repo(m_db.handle());
        Match m = makeMatch(QStringLiteral("A"), QDateTime(QDate(2026, 11, 8), QTime(15, 0)));
        QVERIFY(repo.insert(m));
        QVERIFY(repo.setPlayers(m.id, {{m_p1, QStringLiteral("starter")}, {m_p2, QStringLiteral("starter")}}));
        PlayerRepository players(m_db.handle());
        QVERIFY(players.remove(m_p2));
        QCOMPARE(repo.players(m.id).size(), 1);
        QVERIFY(repo.remove(m.id));
        QCOMPARE(count("SELECT COUNT(*) FROM match_player"), 0);
        QCOMPARE(count("SELECT COUNT(*) FROM player"), 1);        // les joueurs restent
    }

    void deletingTheTeamDeletesItsMatchesAndBoards()
    {
        Backend b(m_db.handle());
        QVariantMap in;
        in["teamId"] = m_team;
        in["date"] = "2026-11-08";
        in["time"] = "15:00";
        in["opponent"] = "FC Rivière";
        const QVariantMap saved = b.saveMatch(in);
        QVERIFY2(saved.value("ok").toBool(), qPrintable(saved.value("error").toString()));
        QVERIFY(b.createMatchBoard(saved.value("id").toInt()).value("ok").toBool());
        QCOMPARE(count("SELECT COUNT(*) FROM tactical_board WHERE match_id IS NOT NULL"), 1);
        QVERIFY(b.deleteTeam(m_team).value("ok").toBool());
        QCOMPARE(count("SELECT COUNT(*) FROM football_match"), 0);
        QCOMPARE(count("SELECT COUNT(*) FROM tactical_board"), 0);
    }

    // ----- Schémas tactiques de match -----
    void matchBoardIsExclusiveAndUniquePerMatch()
    {
        MatchRepository matches(m_db.handle());
        Match m = makeMatch(QStringLiteral("A"), QDateTime(QDate(2026, 11, 8), QTime(15, 0)));
        QVERIFY(matches.insert(m));
        BoardRepository boards(m_db.handle());
        QString error;

        BoardRecord both;
        both.title = QStringLiteral("x");
        both.matchId = m.id;
        both.sessionId = 1;
        QVERIFY(!boards.insert(both, &error));                    // match + séance : refusé
        both.sessionId = -1;
        both.exerciseId = 1;
        QVERIFY(!boards.insert(both, &error));                    // match + exercice : refusé
        both.exerciseId = -1;
        both.sessionExerciseId = 1;
        QVERIFY(!boards.insert(both, &error));                    // match + bloc : refusé
        QCOMPARE(count("SELECT COUNT(*) FROM tactical_board"), 0);

        BoardRecord ok;
        ok.title = QStringLiteral("Match");
        ok.matchId = m.id;
        QVERIFY2(boards.insert(ok, &error), qPrintable(error));
        BoardRecord second;
        second.title = QStringLiteral("Deuxième");
        second.matchId = m.id;
        QVERIFY(!boards.insert(second, &error));                  // un seul schéma par match
        QVERIFY(error.contains(QStringLiteral("déjà")));

        BoardRecord found;
        QVERIFY(boards.forMatch(m.id, &found));
        QCOMPARE(found.id, ok.id);
        QVERIFY(!boards.forMatch(m.id + 1, &found));

        // Il n'apparaît pas dans la bibliothèque de schémas.
        QCOMPARE(boards.list(nullptr, false).size(), 0);
        QCOMPARE(boards.list(nullptr, true).size(), 1);

        // Et ne peut pas être détourné vers une séance par une mise à jour.
        BoardRecord changed = ok;
        changed.sessionId = 1;
        QVERIFY(!boards.update(changed, &error));
    }

    void importValidationRefusesMatchBoardWithOtherLinks()
    {
        MatchRepository matches(m_db.handle());
        Match m = makeMatch(QStringLiteral("A"), QDateTime(QDate(2026, 11, 8), QTime(15, 0)));
        QVERIFY(matches.insert(m));
        QSqlQuery q(m_db.handle());
        // Données incohérentes forcées à la main : deux schémas pour le même match.
        QVERIFY(q.exec(QStringLiteral("INSERT INTO tactical_board(title, match_id) VALUES('a', %1)").arg(m.id)));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO tactical_board(title, match_id) VALUES('b', %1)").arg(m.id)));
        BoardRepository boards(m_db.handle());
        QStringList problems;
        QVERIFY(!boards.validateAllLinks(true, &problems));
        QVERIFY(!problems.isEmpty());
    }

    void deletingAnUnknownBoardFails()
    {
        BoardRepository boards(m_db.handle());
        QString error;
        QVERIFY(!boards.remove(9999, &error));
        Backend b(m_db.handle());
        QVERIFY(!b.deleteBoard(9999));
        QVERIFY(!b.lastError().isEmpty());
    }

    // ----- API du Backend -----
    void backendMatchRoundTrip()
    {
        Backend b(m_db.handle());
        QVERIFY(b.clubs().size() == 1);
        QCOMPARE(b.teams().first().toMap().value("clubId").toInt(), b.clubs().first().toMap().value("id").toInt());

        QVariantMap in;
        in["teamId"] = m_team;
        in["date"] = "2026-11-08";
        in["time"] = "15:00";
        in["opponent"] = "FC Rivière";
        in["homeAway"] = "away";
        in["location"] = "Stade d'Été";
        const QVariantMap saved = b.saveMatch(in);
        QVERIFY2(saved.value("ok").toBool(), qPrintable(saved.value("error").toString()));
        const int id = saved.value("id").toInt();

        QVariantMap bad = in;
        bad["opponent"] = "";
        QVERIFY(!b.saveMatch(bad).value("ok").toBool());
        bad = in;
        bad["date"] = "2026-13-45";
        QVERIFY(!b.saveMatch(bad).value("ok").toBool());

        QCOMPARE(b.matches(m_team, "2026-11-01", "2026-11-30").size(), 1);
        QCOMPARE(b.matchDates(m_team, "2026-11-01", "2026-11-30"), QStringList{"2026-11-08"});
        QCOMPARE(b.sessionDates(m_team, "2026-11-01", "2026-11-30").size(), 0);   // pas une séance

        QVERIFY(b.setMatchPlayers(id, {QVariantMap{{"playerId", m_p1}, {"role", "starter"}},
                                        QVariantMap{{"playerId", m_p2}, {"role", "substitute"}}})
                    .value("ok").toBool());
        const QVariantMap full = b.match(id);
        QVERIFY(full.value("ok").toBool());
        QCOMPARE(full.value("players").toList().size(), 2);
        QCOMPARE(full.value("match").toMap().value("convoked").toInt(), 2);
        QCOMPARE(full.value("boardId").toInt(), -1);

        const QVariantMap made = b.createMatchBoard(id);
        QVERIFY(made.value("ok").toBool());
        QVERIFY(made.value("created").toBool());
        QVERIFY(!b.createMatchBoard(id).value("created").toBool());               // idempotent
        QCOMPARE(b.match(id).value("boardId").toInt(), made.value("boardId").toInt());
        QCOMPARE(b.boards().size(), 0);                                          // pas dans la bibliothèque

        QVERIFY(!b.createMatchBoard(9999).value("ok").toBool());
        QVERIFY(!b.deleteMatch(9999).value("ok").toBool());
        QVERIFY(b.deleteMatch(id).value("ok").toBool());
        QCOMPARE(count("SELECT COUNT(*) FROM tactical_board"), 0);                // le schéma suit le match
        QVERIFY(!b.match(id).value("ok").toBool());
    }

    void backendClubApi()
    {
        Backend b(m_db.handle());
        QVERIFY(!b.saveClub({{"name", "  "}}).value("ok").toBool());
        const QVariantMap c = b.saveClub({{"name", "AS Voisins"}});
        QVERIFY(c.value("ok").toBool());
        QCOMPARE(b.clubs().size(), 2);
        const int club = c.value("id").toInt();
        const QVariantMap t = b.saveTeam({{"name", "U13"}, {"clubId", club}});
        QVERIFY(t.value("ok").toBool());
        QCOMPARE(b.teams(club).size(), 1);
        QCOMPARE(b.teams().size(), 2);
        QCOMPARE(b.clubs().last().toMap().value("teamCount").toInt() + b.clubs().first().toMap().value("teamCount").toInt(), 2);
        // Modifier une équipe ne la déplace pas de club.
        QVERIFY(b.saveTeam({{"id", t.value("id")}, {"name", "U13 bis"}, {"clubId", b.clubs().first().toMap().value("id")}})
                    .value("ok").toBool());
        QCOMPARE(b.teams(club).size(), 1);
        QVERIFY(b.deleteClub(club).value("ok").toBool());
        QCOMPARE(b.teams().size(), 1);
        QVERIFY(!b.deleteClub(b.clubs().first().toMap().value("id").toInt()).value("ok").toBool());   // dernier club
    }

    // ----- Feuille de challenge 2/2 -----
    void challengeLicenceMustBeTenDigits()
    {
        QVERIFY(ChallengeSheet::isValidLicence(QStringLiteral("9603907336")));
        QVERIFY(ChallengeSheet::isValidLicence(QStringLiteral(" 96 0390 7336 ")));      // espaces ignorés
        QVERIFY(ChallengeSheet::isValidLicence(QStringLiteral("2318073452")));
        QVERIFY(!ChallengeSheet::isValidLicence(QString()));
        QVERIFY(!ChallengeSheet::isValidLicence(QStringLiteral("960390733")));          // 9 chiffres
        QVERIFY(!ChallengeSheet::isValidLicence(QStringLiteral("96039073366")));        // 11 chiffres
        QVERIFY(!ChallengeSheet::isValidLicence(QStringLiteral("96039A7336")));         // une lettre
        QVERIFY(!ChallengeSheet::isValidLicence(QStringLiteral("9603-07336")));
    }

    void challengeSheetCheckAndExport()
    {
        Backend b(m_db.handle());
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        auto addPlayer = [&](const QString &first, const QString &last, const QString &licence) {
            return b.savePlayer({{"teamId", m_team}, {"firstName", first}, {"lastName", last}, {"licence", licence}})
                .value("id").toInt();
        };
        const int match = b.saveMatch({{"teamId", m_team}, {"date", "2026-11-14"}, {"time", "10:30"}, {"opponent", "AS Test"}})
                              .value("id").toInt();
        QVERIFY(match > 0);
        const QString out = dir.filePath(QStringLiteral("feuille.pdf"));

        // Aucun convoqué : refus, aucun fichier.
        QVariantMap r = b.exportChallengeSheet(match, out);
        QVERIFY(!r.value("ok").toBool());
        QVERIFY2(r.value("error").toString().contains(QStringLiteral("Aucun joueur")), qPrintable(r.value("error").toString()));
        QVERIFY(!QFileInfo::exists(out));

        // Licence manquante ou invalide : refus ; le compte-rendu nomme chaque joueur et sa raison.
        const int ok1 = addPlayer("Adam", "Alioua", "9605922556");
        const int none = addPlayer("Sans", "Licence", "");
        const int bad = addPlayer("Trop", "Court", "960592255");
        const int letters = addPlayer("Avec", "Lettres", "96059AB556");
        const int ok2 = addPlayer("Ilyan", "Alioua", "9605922562");
        QVERIFY(b.setMatchPlayers(match, {QVariantMap{{"playerId", ok1}, {"role", "starter"}},
                                          QVariantMap{{"playerId", none}, {"role", "starter"}},
                                          QVariantMap{{"playerId", bad}, {"role", "substitute"}},
                                          QVariantMap{{"playerId", letters}, {"role", "substitute"}},
                                          QVariantMap{{"playerId", ok2}, {"role", "starter"}}}).value("ok").toBool());
        r = b.challengeSheetCheck(match);
        QVERIFY(!r.value("ok").toBool());
        QCOMPARE(r.value("count").toInt(), 5);
        const QVariantList problems = r.value("problems").toList();
        QCOMPARE(problems.size(), 3);
        // Ordre alphabétique des noms : Court, Lettres, Licence.
        QVERIFY(problems.at(0).toMap().value("name").toString().contains(QStringLiteral("Court")));
        QVERIFY(problems.at(0).toMap().value("message").toString().contains(QStringLiteral("10 chiffres")));
        QVERIFY(problems.at(1).toMap().value("name").toString().contains(QStringLiteral("Lettres")));
        QVERIFY(problems.at(1).toMap().value("message").toString().contains(QStringLiteral("10 chiffres")));
        QVERIFY(problems.at(2).toMap().value("name").toString().contains(QStringLiteral("Licence")));
        QVERIFY(problems.at(2).toMap().value("message").toString().contains(QStringLiteral("manquant")));
        r = b.exportChallengeSheet(match, out);
        QVERIFY(!r.value("ok").toBool());
        QVERIFY(!QFileInfo::exists(out));

        // Les non convoqués ne comptent pas : on ne garde que les joueurs valides.
        QVERIFY(b.setMatchPlayers(match, {QVariantMap{{"playerId", ok2}, {"role", "starter"}},
                                          QVariantMap{{"playerId", ok1}, {"role", "starter"}}}).value("ok").toBool());
        r = b.challengeSheetCheck(match);
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(r.value("teamName").toString(), QStringLiteral("U15"));
        QCOMPARE(r.value("fileName").toString(), QStringLiteral("feuille-challenge-u15-2026-11-14.pdf"));
        r = b.exportChallengeSheet(match, QUrl::fromLocalFile(out).toString());
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QFile f(out);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray pdf = f.readAll();
        QCOMPARE(pdf.left(5), QByteArray("%PDF-"));
        QCOMPARE(qint64(pdf.size()), r.value("bytes").toLongLong());
        QVERIFY(pdf.size() > 4000);
        QCOMPARE(pdf.count("/Type /Page\n"), 1);          // une seule page
        // Contenu et ordre des lignes (nom puis prénom), vérifiés si pdftotext est installé.
        const QString tool = QStandardPaths::findExecutable(QStringLiteral("pdftotext"));
        if (!tool.isEmpty()) {
            QProcess p;
            p.start(tool, {QStringLiteral("-layout"), out, QStringLiteral("-")});
            QVERIFY(p.waitForFinished(15000));
            const QString text = QString::fromUtf8(p.readAllStandardOutput());
            QVERIFY(text.contains(QStringLiteral("FEUILLE DE CHALLENGE 2/2")));
            QVERIFY(text.contains(QStringLiteral("U15")));
            const int adam = text.indexOf(QStringLiteral("9605922556")), ilyan = text.indexOf(QStringLiteral("9605922562"));
            QVERIFY(adam > 0 && ilyan > adam);              // Alioua Adam avant Alioua Ilyan malgré l'ordre de convocation
            QCOMPARE(text.count(QStringLiteral("Blessure")), 5);   // quatre en-têtes de bloc + la note de bas de page
        }

        // Fichier déjà présent : réécrit en entier, sans reste de l'ancien contenu (ancien fichier plus gros).
        {
            QFile big(out);
            QVERIFY(big.open(QIODevice::WriteOnly | QIODevice::Truncate));
            big.write(QByteArray(pdf.size() + 5000, 'x'));
            big.close();
            r = b.exportChallengeSheet(match, QUrl::fromLocalFile(out).toString());
            QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
            QFile again(out);
            QVERIFY(again.open(QIODevice::ReadOnly));
            QCOMPARE(again.size(), qint64(pdf.size()));
            QCOMPARE(again.read(5), QByteArray("%PDF-"));
        }
        // Fichier temporaire d'ouverture : nom sans chemin, ancienne feuille supprimée, jamais d'écrasement bloquant.
        {
            const QUrl t1(b.challengeSheetTempUrl(QStringLiteral("../../x/feuille-challenge-u15-2026-11-14.pdf")));
            QVERIFY(t1.isLocalFile());
            QCOMPARE(QFileInfo(t1.toLocalFile()).fileName(), QStringLiteral("feuille-challenge-u15-2026-11-14.pdf"));
            QFile first(t1.toLocalFile());
            QVERIFY(first.open(QIODevice::WriteOnly));
            first.write("ancienne feuille");
            first.close();
            const QUrl t2(b.challengeSheetTempUrl(QStringLiteral("feuille-challenge-u15-2026-11-14.pdf")));
            QCOMPARE(t2, t1);                                       // l'ancienne a été supprimée : même nom, fichier neuf
            QVERIFY(!QFileInfo::exists(t2.toLocalFile()));
            QVERIFY(b.exportChallengeSheet(match, t2.toString()).value("ok").toBool());
            QVERIFY(QFileInfo(t2.toLocalFile()).size() > 4000);
            QFile::remove(t2.toLocalFile());
        }

        // Plus de 12 convoqués : refus avec le nombre à retirer.
        QVariantList many;
        for (int i = 0; i < 13; ++i)
            many.append(QVariantMap{{"playerId", addPlayer(QStringLiteral("J%1").arg(i), QStringLiteral("N%1").arg(i),
                                                           QStringLiteral("96000000%1").arg(i + 10)) },
                                    {"role", "starter"}});
        QVERIFY(b.setMatchPlayers(match, many).value("ok").toBool());
        r = b.challengeSheetCheck(match);
        QVERIFY(!r.value("ok").toBool());
        QVERIFY2(r.value("error").toString().contains(QStringLiteral("12 lignes")), qPrintable(r.value("error").toString()));
        QVERIFY2(r.value("error").toString().contains(QStringLiteral("Retirez-en 1")), qPrintable(r.value("error").toString()));

        // Match inconnu, fichier impossible à écrire.
        QVERIFY(!b.challengeSheetCheck(9999).value("ok").toBool());
        QVERIFY(b.setMatchPlayers(match, {QVariantMap{{"playerId", ok1}, {"role", "starter"}}}).value("ok").toBool());
        r = b.exportChallengeSheet(match, dir.filePath(QStringLiteral("absent/feuille.pdf")));
        QVERIFY(!r.value("ok").toBool());
        QVERIFY2(r.value("error").toString().contains(QStringLiteral("écrire")), qPrintable(r.value("error").toString()));
    }
};

QTEST_MAIN(MatchesTest)   // QGuiApplication (hors écran) : le PDF de la feuille de challenge utilise les polices
#include "tst_matches.moc"
