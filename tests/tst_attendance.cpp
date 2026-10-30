#include "core/AttendanceRepository.h"
#include "core/AttendanceStats.h"
#include "core/Database.h"
#include "core/PlayerRepository.h"
#include "core/SessionRepository.h"
#include "core/TeamRepository.h"

#include <QSqlQuery>
#include <QtTest>

using namespace AttendanceStats;

class AttendanceTest : public QObject
{
    Q_OBJECT

private slots:
    // --- Calcul pur --------------------------------------------------------------------------

    void rateExcludesExcusedAndInjured()
    {
        // 4 séances : présent, absent, excusé, blessé -> taux = 1 / (1 + 1) = 50 %.
        Data d = baseData();
        d.rows.insert({1, 10}, QStringLiteral("present"));
        d.rows.insert({2, 10}, QStringLiteral("absent"));
        d.rows.insert({3, 10}, QStringLiteral("excused"));
        d.rows.insert({4, 10}, QStringLiteral("injured"));
        const PlayerStats st = compute(d).first();
        QCOMPARE(st.present, 1);
        QCOMPARE(st.absent, 1);
        QCOMPARE(st.excused, 1);
        QCOMPARE(st.injured, 1);
        QCOMPARE(st.counted(), 4);
        QCOMPARE(st.denominator(), 2);
        QCOMPARE(st.rate(), 0.5);
    }

    void missingRowCountsAsAbsentForActivePlayer()
    {
        Data d = baseData();
        d.rows.insert({1, 10}, QStringLiteral("present"));   // les séances 2, 3, 4 n'ont pas d'appel
        const PlayerStats st = compute(d).first();
        QCOMPARE(st.present, 1);
        QCOMPARE(st.absent, 3);
        QCOMPARE(st.rate(), 0.25);
    }

    void newPlayerNotPenalisedForEarlierSessions()
    {
        Data d = baseData();   // séances les 1er, 8, 15 et 22 septembre
        d.players[0].joined = QDate(2026, 9, 15);
        const PlayerStats st = compute(d).first();
        QCOMPARE(st.absent, 2);     // seulement les séances du 15 et du 22
        QCOMPARE(st.counted(), 2);
    }

    void explicitRowBeforeJoinDateStillCounts()
    {
        Data d = baseData();
        d.players[0].joined = QDate(2026, 9, 15);
        d.rows.insert({1, 10}, QStringLiteral("present"));   // appel saisi pour une séance antérieure à l'arrivée
        const PlayerStats st = compute(d).first();
        QCOMPARE(st.present, 1);
        QCOMPARE(st.absent, 2);
    }

    void inactivePlayerOnlyExplicitRows()
    {
        Data d = baseData();
        d.players[0].active = false;
        d.rows.insert({1, 10}, QStringLiteral("present"));
        d.rows.insert({2, 10}, QStringLiteral("present"));
        const PlayerStats st = compute(d).first();
        QCOMPARE(st.counted(), 2);   // pas d'absence implicite pour un joueur inactif
        QCOMPARE(st.rate(), 1.0);
    }

    void noSessionMeansNoRate()
    {
        Data d = baseData();
        d.sessions.clear();
        QCOMPARE(compute(d).first().rate(), -1.0);
    }

    void onlyExcusedMeansNoRate()
    {
        Data d = baseData();
        for (int s = 1; s <= 4; ++s)
            d.rows.insert({s, 10}, QStringLiteral("excused"));
        const PlayerStats st = compute(d).first();
        QCOMPARE(st.counted(), 4);
        QCOMPARE(st.rate(), -1.0);   // dénominateur nul : pas de division par zéro
    }

    void cellStatusValues()
    {
        Data d = baseData();
        d.rows.insert({1, 10}, QStringLiteral("injured"));
        const PlayerRef &p = d.players.first();
        QCOMPARE(cellStatus(d, p, d.sessions.at(0)), QStringLiteral("injured"));
        QCOMPARE(cellStatus(d, p, d.sessions.at(1)), QStringLiteral("absent"));
        PlayerRef late = p;
        late.joined = QDate(2026, 12, 1);
        QCOMPARE(cellStatus(d, late, d.sessions.at(1)), QString());
    }

    // --- Repository --------------------------------------------------------------------------

    void init()
    {
        m_db = new Database;
        QString error;
        QVERIFY2(m_db->open(QStringLiteral(":memory:"), &error), qPrintable(error));
        m_teams = new TeamRepository(m_db->handle());
        m_players = new PlayerRepository(m_db->handle());
        m_sessions = new SessionRepository(m_db->handle());
        m_att = new AttendanceRepository(m_db->handle());

        Team t;
        t.name = QStringLiteral("U15");
        QVERIFY(m_teams->insert(t));
        m_teamId = t.id;
        for (const char *name : {"Alpha", "Bravo", "Charlie"}) {
            Player p;
            p.teamId = m_teamId;
            p.lastName = QString::fromLatin1(name);
            p.firstName = QStringLiteral("X");
            QVERIFY(m_players->insert(p));
            m_playerIds << p.id;
        }
    }

    void cleanup()
    {
        m_playerIds.clear();
        delete m_att;
        delete m_sessions;
        delete m_players;
        delete m_teams;
        delete m_db;
    }

    void saveReplacesWholeCall()
    {
        const int sid = addSession(QDate(2026, 10, 1), "done");
        QVERIFY(m_att->save(sid, {entry(0, "present"), entry(1, "absent"), entry(2, "excused", "malade")}));
        QCOMPARE(m_att->load(sid).size(), 3);
        QCOMPARE(m_att->load(sid).value(m_playerIds.at(2)).note, QStringLiteral("malade"));

        // Un second enregistrement remplace tout, sans doublon ni reste.
        QVERIFY(m_att->save(sid, {entry(0, "injured")}));
        const auto after = m_att->load(sid);
        QCOMPARE(after.size(), 1);
        QCOMPARE(after.value(m_playerIds.at(0)).status, QStringLiteral("injured"));
    }

    void saveIsAtomic()
    {
        const int sid = addSession(QDate(2026, 10, 1), "done");
        QVERIFY(m_att->save(sid, {entry(0, "present"), entry(1, "present")}));

        // Statut invalide en 2e position : l'ancien appel doit rester intact.
        QString error;
        QVERIFY(!m_att->save(sid, {entry(0, "absent"), entry(1, "bogus")}, &error));
        QVERIFY(!error.isEmpty());
        const auto kept = m_att->load(sid);
        QCOMPARE(kept.size(), 2);
        QCOMPARE(kept.value(m_playerIds.at(0)).status, QStringLiteral("present"));
    }

    void summaryCounts()
    {
        const int s1 = addSession(QDate(2026, 10, 1), "done");
        const int s2 = addSession(QDate(2026, 10, 8), "done");
        QVERIFY(m_att->save(s1, {entry(0, "present"), entry(1, "present"), entry(2, "absent")}));
        const auto sum = m_att->summary({s1, s2});
        QCOMPARE(sum.value(s1), qMakePair(2, 3));
        QVERIFY(!sum.contains(s2));   // pas d'appel
        QVERIFY(m_att->summary({}).isEmpty());
    }

    void loadDataOnlyDoneSessionsInRange()
    {
        const int done1 = addSession(QDate(2026, 10, 1), "done");
        addSession(QDate(2026, 10, 8), "planned");
        addSession(QDate(2026, 10, 15), "cancelled");
        const int outside = addSession(QDate(2026, 12, 1), "done");
        QVERIFY(m_att->save(done1, {entry(0, "present")}));
        QVERIFY(m_att->save(outside, {entry(0, "present")}));

        const Data d = m_att->loadData(m_teamId, QDate(2026, 10, 1), QDate(2026, 10, 31));
        QCOMPARE(d.sessions.size(), 1);
        QCOMPARE(d.sessions.first().id, done1);
        QCOMPARE(d.sessions.first().date, QDate(2026, 10, 1));
        QCOMPARE(d.players.size(), 3);
        QCOMPARE(d.rows.size(), 1);
        QVERIFY(d.players.first().joined.isValid());
    }

    void endToEndRate()
    {
        const int s1 = addSession(QDate::currentDate().addDays(-14), "done");
        const int s2 = addSession(QDate::currentDate().addDays(-7), "done");
        QVERIFY(m_att->save(s1, {entry(0, "present"), entry(1, "excused"), entry(2, "present")}));
        QVERIFY(m_att->save(s2, {entry(0, "present"), entry(1, "present"), entry(2, "absent")}));

        const Data d = m_att->loadData(m_teamId, QDate(2000, 1, 1), QDate::currentDate());
        const QList<PlayerStats> st = compute(d);
        QCOMPARE(st.size(), 3);
        QCOMPARE(st.at(0).rate(), 1.0);   // Alpha : 2 présents
        QCOMPARE(st.at(1).rate(), 1.0);   // Bravo : 1 excusé (ignoré) + 1 présent
        QCOMPARE(st.at(2).rate(), 0.5);   // Charlie : 1 présent, 1 absent
    }

private:
    int addSession(const QDate &date, const char *status)
    {
        Session s;
        s.teamId = m_teamId;
        s.startAt = QDateTime(date, QTime(18, 30));
        s.status = QString::fromLatin1(status);
        [&] { QVERIFY(m_sessions->insert(s)); }();
        return s.id;
    }

    AttendanceRepository::Entry entry(int playerIndex, const char *status, const char *note = "") const
    {
        AttendanceRepository::Entry e;
        e.playerId = m_playerIds.at(playerIndex);
        e.status = QString::fromLatin1(status);
        e.note = QString::fromLatin1(note);
        return e;
    }

    static Data baseData()
    {
        Data d;
        d.sessions = {{1, QDate(2026, 9, 1)}, {2, QDate(2026, 9, 8)}, {3, QDate(2026, 9, 15)}, {4, QDate(2026, 9, 22)}};
        PlayerRef p;
        p.id = 10;
        p.lastName = QStringLiteral("Dupont");
        p.joined = QDate(2026, 1, 1);
        d.players = {p};
        return d;
    }

    Database *m_db = nullptr;
    TeamRepository *m_teams = nullptr;
    PlayerRepository *m_players = nullptr;
    SessionRepository *m_sessions = nullptr;
    AttendanceRepository *m_att = nullptr;
    int m_teamId = -1;
    QList<int> m_playerIds;
};

QTEST_GUILESS_MAIN(AttendanceTest)
#include "tst_attendance.moc"
