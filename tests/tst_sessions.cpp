#include "core/Database.h"
#include "core/Recurrence.h"
#include "core/SessionRepository.h"
#include "core/TeamRepository.h"

#include <QSqlQuery>
#include <QtTest>

class SessionsTest : public QObject
{
    Q_OBJECT

private slots:
    // --- Recurrence -------------------------------------------------------------------------

    void recurrenceTuesdaysAndThursdays()
    {
        // Semaine du lundi 2026-10-26 au dimanche 2026-11-08 : mardis 27/10 et 3/11, jeudis 29/10 et 5/11.
        const QList<QDateTime> r = Recurrence::occurrences(QDate(2026, 10, 26), QDate(2026, 11, 8),
                                                           {Qt::Tuesday, Qt::Thursday}, QTime(18, 30));
        QCOMPARE(r.size(), 4);
        QCOMPARE(r.at(0), QDateTime(QDate(2026, 10, 27), QTime(18, 30)));
        QCOMPARE(r.at(1), QDateTime(QDate(2026, 10, 29), QTime(18, 30)));
        QCOMPARE(r.at(2), QDateTime(QDate(2026, 11, 3), QTime(18, 30)));
        QCOMPARE(r.at(3), QDateTime(QDate(2026, 11, 5), QTime(18, 30)));
    }

    void recurrenceBoundsAreInclusive()
    {
        // 2026-10-27 est un mardi : début et fin le même jour -> une occurrence.
        QCOMPARE(Recurrence::occurrences(QDate(2026, 10, 27), QDate(2026, 10, 27), {Qt::Tuesday}, QTime(10, 0)).size(), 1);
    }

    void recurrenceInvalidInputs()
    {
        const QSet<int> tue{Qt::Tuesday};
        QVERIFY(Recurrence::occurrences(QDate(2026, 11, 1), QDate(2026, 10, 1), tue, QTime(10, 0)).isEmpty());   // fin < début
        QVERIFY(Recurrence::occurrences(QDate(2026, 10, 1), QDate(2026, 11, 1), {}, QTime(10, 0)).isEmpty());   // aucun jour
        QVERIFY(Recurrence::occurrences(QDate(), QDate(2026, 11, 1), tue, QTime(10, 0)).isEmpty());              // date invalide
        QVERIFY(Recurrence::occurrences(QDate(2026, 10, 1), QDate(2026, 11, 1), tue, QTime()).isEmpty());        // heure invalide
    }

    void recurrenceIsCapped()
    {
        // Tous les jours pendant 10 ans : le résultat est tronqué à kMaxOccurrences + 1.
        const QSet<int> all{1, 2, 3, 4, 5, 6, 7};
        const auto r = Recurrence::occurrences(QDate(2026, 1, 1), QDate(2036, 1, 1), all, QTime(10, 0));
        QCOMPARE(r.size(), Recurrence::kMaxOccurrences + 1);
    }

    // --- SessionRepository ------------------------------------------------------------------

    void init()
    {
        m_db = new Database;
        QString error;
        QVERIFY2(m_db->open(QStringLiteral(":memory:"), &error), qPrintable(error));
        m_repo = new SessionRepository(m_db->handle());
        TeamRepository teams(m_db->handle());
        Team t;
        t.name = QStringLiteral("U15");
        QVERIFY(teams.insert(t));
        m_teamId = t.id;
        Team t2;
        t2.name = QStringLiteral("Seniors");
        QVERIFY(teams.insert(t2));
        m_otherTeamId = t2.id;
    }

    void cleanup()
    {
        delete m_repo;
        delete m_db;
    }

    void insertAndRead()
    {
        Session s = make(QDate(2026, 10, 27), QTime(18, 30), 90);
        s.theme = QStringLiteral("Possession");
        s.location = QStringLiteral("Stade A");
        QVERIFY(m_repo->insert(s));
        QVERIFY(s.id > 0);

        const QList<Session> r = m_repo->listBetween(m_teamId, QDate(2026, 10, 27), QDate(2026, 10, 27));
        QCOMPARE(r.size(), 1);
        QCOMPARE(r.first().startAt, QDateTime(QDate(2026, 10, 27), QTime(18, 30)));
        QCOMPARE(r.first().durationMin, 90);
        QCOMPARE(r.first().theme, QStringLiteral("Possession"));
        QCOMPARE(r.first().status, QStringLiteral("planned"));
        QCOMPARE(r.first().endAt(), QDateTime(QDate(2026, 10, 27), QTime(20, 0)));
    }

    void listBetweenBoundsAndTeamIsolation()
    {
        Session a = make(QDate(2026, 10, 26), QTime(23, 59), 30);   // dernier moment de la veille
        Session b = make(QDate(2026, 10, 27), QTime(0, 0), 30);     // premier moment du jour
        Session c = make(QDate(2026, 10, 27), QTime(23, 59), 30);   // dernier moment du jour
        Session d = make(QDate(2026, 10, 28), QTime(0, 0), 30);     // lendemain
        Session other = make(QDate(2026, 10, 27), QTime(10, 0), 30);
        other.teamId = m_otherTeamId;
        for (Session *s : {&a, &b, &c, &d, &other})
            QVERIFY(m_repo->insert(*s));

        const QList<Session> r = m_repo->listBetween(m_teamId, QDate(2026, 10, 27), QDate(2026, 10, 27));
        QCOMPARE(r.size(), 2);
        QCOMPARE(r.at(0).id, b.id);
        QCOMPARE(r.at(1).id, c.id);
    }

    void datesWithSessions()
    {
        Session a = make(QDate(2026, 10, 27), QTime(18, 0), 60);
        Session b = make(QDate(2026, 10, 27), QTime(20, 0), 60);   // même jour
        Session c = make(QDate(2026, 10, 30), QTime(18, 0), 60);
        for (Session *s : {&a, &b, &c})
            QVERIFY(m_repo->insert(*s));
        const QSet<QDate> dates = m_repo->datesWithSessions(m_teamId, QDate(2026, 10, 1), QDate(2026, 10, 31));
        QCOMPARE(dates.size(), 2);
        QVERIFY(dates.contains(QDate(2026, 10, 27)));
        QVERIFY(dates.contains(QDate(2026, 10, 30)));
    }

    void conflictDetection()
    {
        Session base = make(QDate(2026, 10, 27), QTime(18, 0), 90);   // 18:00 - 19:30
        QVERIFY(m_repo->insert(base));

        auto clash = [&](const QTime &t, int dur, int exclude = -1) {
            return !m_repo->conflicts(m_teamId, QDateTime(QDate(2026, 10, 27), t), dur, exclude).isEmpty();
        };
        QVERIFY(clash(QTime(18, 30), 60));        // chevauchement partiel
        QVERIFY(clash(QTime(17, 0), 120));        // englobe le début
        QVERIFY(clash(QTime(18, 15), 15));        // contenue
        QVERIFY(!clash(QTime(19, 30), 60));       // commence exactement à la fin : pas de conflit
        QVERIFY(!clash(QTime(16, 30), 90));       // finit exactement au début : pas de conflit
        QVERIFY(!clash(QTime(18, 30), 60, base.id));   // la séance éditée s'ignore elle-même

        // Autre équipe : jamais de conflit.
        QVERIFY(m_repo->conflicts(m_otherTeamId, QDateTime(QDate(2026, 10, 27), QTime(18, 30)), 60).isEmpty());
    }

    void conflictAcrossMidnight()
    {
        Session late = make(QDate(2026, 10, 27), QTime(23, 0), 120);   // 23:00 -> 01:00 le lendemain
        QVERIFY(m_repo->insert(late));
        QVERIFY(!m_repo->conflicts(m_teamId, QDateTime(QDate(2026, 10, 28), QTime(0, 30)), 30).isEmpty());
        QVERIFY(m_repo->conflicts(m_teamId, QDateTime(QDate(2026, 10, 28), QTime(1, 0)), 30).isEmpty());
    }

    void cancelledSessionsDoNotConflict()
    {
        Session s = make(QDate(2026, 10, 27), QTime(18, 0), 90);
        QVERIFY(m_repo->insert(s));
        QVERIFY(m_repo->setStatus(s.id, QStringLiteral("cancelled")));
        QVERIFY(m_repo->conflicts(m_teamId, QDateTime(QDate(2026, 10, 27), QTime(18, 30)), 60).isEmpty());
        QVERIFY(m_repo->setStatus(s.id, QStringLiteral("planned")));
        QVERIFY(!m_repo->conflicts(m_teamId, QDateTime(QDate(2026, 10, 27), QTime(18, 30)), 60).isEmpty());
    }

    void invalidStatusRejected()
    {
        Session s = make(QDate(2026, 10, 27), QTime(18, 0), 90);
        s.status = QStringLiteral("bogus");
        QString error;
        QVERIFY(!m_repo->insert(s, &error));
        QVERIFY(!error.isEmpty());
    }

    void updateSession()
    {
        Session s = make(QDate(2026, 10, 27), QTime(18, 0), 90);
        QVERIFY(m_repo->insert(s));
        s.startAt = QDateTime(QDate(2026, 11, 2), QTime(9, 15));
        s.durationMin = 60;
        s.theme = QStringLiteral("Finition");
        QVERIFY(m_repo->update(s));
        QVERIFY(m_repo->listBetween(m_teamId, QDate(2026, 10, 27), QDate(2026, 10, 27)).isEmpty());
        const QList<Session> r = m_repo->listBetween(m_teamId, QDate(2026, 11, 2), QDate(2026, 11, 2));
        QCOMPARE(r.size(), 1);
        QCOMPARE(r.first().startAt.time(), QTime(9, 15));
        QCOMPARE(r.first().theme, QStringLiteral("Finition"));
    }

    void insertManyIsAtomic()
    {
        QList<Session> batch;
        batch << make(QDate(2026, 10, 27), QTime(18, 0), 90)
              << make(QDate(2026, 10, 29), QTime(18, 0), 90);
        batch << make(QDate(2026, 10, 31), QTime(18, 0), 90);
        batch.last().durationMin = 0;   // viole CHECK (duration_min > 0)

        QString error;
        QVERIFY(!m_repo->insertMany(batch, &error));
        QVERIFY(!error.isEmpty());
        // Rien ne doit rester des deux premières séances.
        QCOMPARE(m_repo->listBetween(m_teamId, QDate(2026, 10, 1), QDate(2026, 10, 31)).size(), 0);

        batch.last().durationMin = 90;
        QVERIFY(m_repo->insertMany(batch));
        QCOMPARE(m_repo->listBetween(m_teamId, QDate(2026, 10, 1), QDate(2026, 10, 31)).size(), 3);
        QVERIFY(batch.first().id > 0);
    }

    void removeCascadesAttendanceAndExercisePlan()
    {
        Session s = make(QDate(2026, 10, 27), QTime(18, 0), 90);
        QVERIFY(m_repo->insert(s));
        QSqlQuery q(m_db->handle());
        QVERIFY(q.exec(QStringLiteral("INSERT INTO player(team_id, first_name, last_name) VALUES(%1, 'A', 'B')").arg(m_teamId)));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO attendance(session_id, player_id) VALUES(%1, 1)").arg(s.id)));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO exercise(title) VALUES('Rondo')")));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO session_exercise(session_id, exercise_id, position) VALUES(%1, 1, 1)").arg(s.id)));

        QVERIFY(m_repo->remove(s.id));
        for (const char *table : {"attendance", "session_exercise"}) {
            QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(QLatin1String(table))));
            QVERIFY(q.next());
            QCOMPARE(q.value(0).toInt(), 0);
        }
    }

private:
    Session make(const QDate &d, const QTime &t, int duration) const
    {
        Session s;
        s.teamId = m_teamId;
        s.startAt = QDateTime(d, t);
        s.durationMin = duration;
        return s;
    }

    Database *m_db = nullptr;
    SessionRepository *m_repo = nullptr;
    int m_teamId = -1;
    int m_otherTeamId = -1;
};

QTEST_GUILESS_MAIN(SessionsTest)
#include "tst_sessions.moc"
