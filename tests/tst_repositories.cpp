#include "core/Database.h"
#include "core/PlayerRepository.h"
#include "core/TeamRepository.h"

#include <QSqlQuery>
#include <QtTest>

class RepositoriesTest : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_db = new Database;
        QString error;
        QVERIFY2(m_db->open(QStringLiteral(":memory:"), &error), qPrintable(error));
        m_teams = new TeamRepository(m_db->handle());
        m_players = new PlayerRepository(m_db->handle());

        Team t;
        t.name = QStringLiteral("U15");
        QVERIFY(m_teams->insert(t));
        m_teamId = t.id;
    }

    void cleanup()
    {
        delete m_players;
        delete m_teams;
        delete m_db;
    }

    void migrationsApplied()
    {
        QCOMPARE(m_db->schemaVersion(), 4);
    }

    void insertAndList()
    {
        Player p = makePlayer(QStringLiteral("Zidane"), QStringLiteral("Zinédine"), 10);
        p.birthDate = QDate(2010, 5, 17);
        p.position = QStringLiteral("MID");
        p.phone = QStringLiteral("  ");   // vide après trim -> NULL, relu comme chaîne vide
        QVERIFY(m_players->insert(p));
        QVERIFY(p.id > 0);

        const QList<Player> list = m_players->list(m_teamId, false);
        QCOMPARE(list.size(), 1);
        QCOMPARE(list.first().lastName, QStringLiteral("Zidane"));
        QCOMPARE(list.first().birthDate, QDate(2010, 5, 17));
        QCOMPARE(list.first().jerseyNumber, 10);
        QCOMPARE(list.first().position, QStringLiteral("MID"));
        QVERIFY(list.first().phone.isEmpty());
        QVERIFY(list.first().active);
    }

    void listIsSortedByLastName()
    {
        Player b = makePlayer(QStringLiteral("bernard"), QStringLiteral("Luc"), 0);
        Player a = makePlayer(QStringLiteral("Adam"), QStringLiteral("Zoé"), 0);
        QVERIFY(m_players->insert(b));
        QVERIFY(m_players->insert(a));
        const QList<Player> list = m_players->list(m_teamId, false);
        QCOMPARE(list.size(), 2);
        QCOMPARE(list.at(0).lastName, QStringLiteral("Adam"));
        QCOMPARE(list.at(1).lastName, QStringLiteral("bernard"));
    }

    void noJerseyAllowedForSeveralPlayers()
    {
        Player a = makePlayer(QStringLiteral("A"), QStringLiteral("a"), 0);
        Player b = makePlayer(QStringLiteral("B"), QStringLiteral("b"), 0);
        QVERIFY(m_players->insert(a));
        QVERIFY(m_players->insert(b));
    }

    void jerseyUniqueAmongActivePlayers()
    {
        Player a = makePlayer(QStringLiteral("A"), QStringLiteral("a"), 7);
        QVERIFY(m_players->insert(a));
        QVERIFY(m_players->jerseyTaken(m_teamId, 7));
        QVERIFY(!m_players->jerseyTaken(m_teamId, 7, a.id));   // le joueur lui-même est exclu
        QVERIFY(!m_players->jerseyTaken(m_teamId, 8));

        Player dup = makePlayer(QStringLiteral("B"), QStringLiteral("b"), 7);
        QString error;
        QVERIFY(!m_players->insert(dup, &error));
        QVERIFY(!error.isEmpty());
    }

    void deactivatedPlayerFreesJersey()
    {
        Player a = makePlayer(QStringLiteral("A"), QStringLiteral("a"), 7);
        QVERIFY(m_players->insert(a));
        QVERIFY(m_players->setActive(a.id, false));
        QVERIFY(!m_players->jerseyTaken(m_teamId, 7));

        Player b = makePlayer(QStringLiteral("B"), QStringLiteral("b"), 7);
        QVERIFY(m_players->insert(b));

        // Réactiver A alors que B a repris le 7 est refusé par l'index unique.
        QString error;
        QVERIFY(!m_players->setActive(a.id, true, &error));
        QVERIFY(!error.isEmpty());
    }

    void inactivePlayersHiddenUnlessRequested()
    {
        Player a = makePlayer(QStringLiteral("A"), QStringLiteral("a"), 1);
        QVERIFY(m_players->insert(a));
        QVERIFY(m_players->setActive(a.id, false));
        QCOMPARE(m_players->list(m_teamId, false).size(), 0);
        QCOMPARE(m_players->list(m_teamId, true).size(), 1);
        QVERIFY(!m_players->list(m_teamId, true).first().active);
    }

    void updatePlayer()
    {
        Player a = makePlayer(QStringLiteral("A"), QStringLiteral("a"), 4);
        QVERIFY(m_players->insert(a));
        a.lastName = QStringLiteral("Durand");
        a.jerseyNumber = 0;
        a.email = QStringLiteral("x@y.fr");
        QVERIFY(m_players->update(a));
        const Player r = m_players->list(m_teamId, true).first();
        QCOMPARE(r.lastName, QStringLiteral("Durand"));
        QCOMPARE(r.jerseyNumber, 0);
        QCOMPARE(r.email, QStringLiteral("x@y.fr"));
    }

    void invalidPositionRejectedByDatabase()
    {
        Player a = makePlayer(QStringLiteral("A"), QStringLiteral("a"), 4);
        a.position = QStringLiteral("XXX");
        QString error;
        QVERIFY(!m_players->insert(a, &error));
    }

    void removePlayerCascadesAttendance()
    {
        Player a = makePlayer(QStringLiteral("A"), QStringLiteral("a"), 4);
        QVERIFY(m_players->insert(a));
        QSqlQuery q(m_db->handle());
        QVERIFY(q.exec(QStringLiteral("INSERT INTO training_session(team_id, start_at) VALUES(%1, '2026-10-24T10:00')")
                           .arg(m_teamId)));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO attendance(session_id, player_id) VALUES(1, %1)").arg(a.id)));

        QVERIFY(m_players->remove(a.id));
        QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM attendance")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 0);
    }

    void teamUpdateAndRemoveCascadesPlayers()
    {
        Team t;
        t.name = QStringLiteral("Seniors");
        t.category = QStringLiteral("Seniors");
        QVERIFY(m_teams->insert(t));
        QCOMPARE(m_teams->all().size(), 2);

        t.name = QStringLiteral("Seniors A");
        QVERIFY(m_teams->update(t));

        Player p = makePlayer(QStringLiteral("A"), QStringLiteral("a"), 1);
        p.teamId = t.id;
        QVERIFY(m_players->insert(p));
        QCOMPARE(m_teams->playerCount(t.id), 1);

        QVERIFY(m_teams->remove(t.id));
        QCOMPARE(m_teams->all().size(), 1);
        QCOMPARE(m_players->list(t.id, true).size(), 0);
    }

private:
    Player makePlayer(const QString &last, const QString &first, int jersey) const
    {
        Player p;
        p.teamId = m_teamId;
        p.lastName = last;
        p.firstName = first;
        p.jerseyNumber = jersey;
        return p;
    }

    Database *m_db = nullptr;
    TeamRepository *m_teams = nullptr;
    PlayerRepository *m_players = nullptr;
    int m_teamId = -1;
};

QTEST_GUILESS_MAIN(RepositoriesTest)
#include "tst_repositories.moc"
