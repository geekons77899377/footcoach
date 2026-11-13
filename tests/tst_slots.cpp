// Créneaux réguliers (schéma 10) : création, modification et déprogrammation en bloc ; migration 010 ;
// présence des convoqués conservée quand on modifie la convocation.
#include "app/Backend.h"
#include "core/Database.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

namespace {

const QString kDay = QStringLiteral("yyyy-MM-dd");

QString day(int offset) { return QDate::currentDate().addDays(offset).toString(kDay); }

// Jour de la semaine (1..7) de la date du jour + offset.
int weekdayOf(int offset) { return QDate::currentDate().addDays(offset).dayOfWeek(); }

int count(const QSqlDatabase &db, const QString &sql)
{
    QSqlQuery q(db);
    return q.exec(sql) && q.next() ? q.value(0).toInt() : -1;
}

} // namespace

class SlotsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    Database m_db;
    int m_team = -1;

    // Créneau d'un jour par semaine (le jour de la semaine d'aujourd'hui), de -14 jours à +20 jours :
    // séances à J-14, J-7, J, J+7, J+14 (5 séances, dont 3 à venir).
    QVariantMap weekly(int durationMin = 90) const
    {
        return {{"teamId", m_team}, {"from", day(-14)}, {"to", day(20)}, {"weekdays", QVariantList{weekdayOf(0)}},
                {"time", "18:30"}, {"durationMin", durationMin}, {"location", "Stade"}, {"theme", "Technique"}};
    }

    QList<QVariantMap> slotSessions(Backend &b, int slotId)
    {
        QList<QVariantMap> out;
        for (const QVariant &v : b.sessions(m_team, day(-60), day(60)))
            if (v.toMap().value("slotId").toInt() == slotId)
                out.append(v.toMap());
        return out;
    }

private slots:
    void init()
    {
        m_db.close();
        QFile::remove(m_dir.filePath(QStringLiteral("t.db")));
        QString error;
        QVERIFY2(m_db.open(m_dir.filePath(QStringLiteral("t.db")), &error), qPrintable(error));
        Backend b(m_db.handle());
        m_team = b.saveTeam({{"name", "U11"}}).value("id").toInt();
        QVERIFY(m_team > 0);
    }

    void cleanup() { m_db.close(); }

    void createRecurringRecordsTheSlotAndLinksSessions()
    {
        Backend b(m_db.handle());
        const QVariantMap r = b.createRecurring(weekly(), false);
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(r.value("count").toInt(), 5);
        const int slot = r.value("slotId").toInt();
        QVERIFY(slot > 0);
        QCOMPARE(slotSessions(b, slot).size(), 5);

        const QVariantList list = b.trainingSlots(m_team);
        QCOMPARE(list.size(), 1);
        const QVariantMap s = list.first().toMap();
        QCOMPARE(s.value("time").toString(), QStringLiteral("18:30"));
        QCOMPARE(s.value("endTime").toString(), QStringLiteral("20:00"));
        QCOMPARE(s.value("weekdays").toList(), (QVariantList{weekdayOf(0)}));
        QCOMPARE(s.value("total").toInt(), 5);
        QCOMPARE(s.value("upcoming").toInt(), 3);
        QCOMPARE(s.value("withAttendance").toInt(), 0);
        QVERIFY(b.trainingSlot(slot).value("ok").toBool());
        QVERIFY(!b.trainingSlot(9999).value("ok").toBool());
    }

    // Une séance modifiée seule garde son créneau (le formulaire de séance ne transmet pas slotId).
    void editingOneSessionKeepsItsSlot()
    {
        Backend b(m_db.handle());
        const int slot = b.createRecurring(weekly(), false).value("slotId").toInt();
        QVariantMap s = slotSessions(b, slot).last();
        s.remove("slotId");
        s.insert("theme", "Jeu réduit");
        QVERIFY(b.saveSession(s, true).value("ok").toBool());
        QCOMPARE(slotSessions(b, slot).size(), 5);
    }

    void updateChangesOnlyUpcomingSessionsAndRespectsManualEdits()
    {
        Backend b(m_db.handle());
        const int slot = b.createRecurring(weekly(), false).value("slotId").toInt();
        // Séance à J+14 modifiée à la main : lieu « Gymnase » (le thème reste celui du créneau).
        QVariantMap manual = slotSessions(b, slot).last();
        manual.insert("location", "Gymnase");
        QVERIFY(b.saveSession(manual, true).value("ok").toBool());

        QVariantMap rule = weekly(120);
        rule.insert("time", "19:00");
        rule.insert("location", "Terrain B");
        const QVariantMap p = b.previewSlotUpdate(slot, rule);
        QVERIFY2(p.value("ok").toBool(), qPrintable(p.value("error").toString()));
        QCOMPARE(p.value("updated").toInt(), 3);
        QCOMPARE(p.value("added").toInt(), 0);
        QCOMPARE(p.value("removed").toInt(), 0);
        QCOMPARE(slotSessions(b, slot).first().value("time").toString(), QStringLiteral("18:30"));   // aperçu : rien écrit

        QVERIFY(b.updateSlot(slot, rule, false).value("ok").toBool());
        const QList<QVariantMap> after = slotSessions(b, slot);
        QCOMPARE(after.size(), 5);
        for (const QVariantMap &s : after) {
            const bool upcoming = s.value("date").toString() >= day(0);
            QCOMPARE(s.value("time").toString(), upcoming ? QStringLiteral("19:00") : QStringLiteral("18:30"));
            QCOMPARE(s.value("durationMin").toInt(), upcoming ? 120 : 90);
        }
        QCOMPARE(after.at(2).value("location").toString(), QStringLiteral("Terrain B"));
        QCOMPARE(after.last().value("location").toString(), QStringLiteral("Gymnase"));     // modification à la main gardée
        QCOMPARE(b.trainingSlot(slot).value("time").toString(), QStringLiteral("19:00"));
    }

    void updateMovesDaysAndNeverTouchesSessionsWithAttendance()
    {
        Backend b(m_db.handle());
        const int slot = b.createRecurring(weekly(), false).value("slotId").toInt();
        const QVariantMap player = b.savePlayer({{"teamId", m_team}, {"firstName", "Léo"}, {"lastName", "Martin"}});
        QVERIFY(player.value("ok").toBool());
        // Appel fait sur la séance de J+7 (en avance, cas rare mais possible).
        const QVariantMap plus7 = slotSessions(b, slot).at(3);
        QVERIFY(b.saveAttendance(plus7.value("id").toInt(),
                                 {QVariantMap{{"playerId", player.value("id")}, {"status", "present"}}}, false)
                    .value("ok").toBool());

        // Le créneau passe au lendemain : J, J+14 supprimées ; J+1, J+8, J+15 créées ; J+7 (appel) gardée.
        QVariantMap rule = weekly();
        rule.insert("weekdays", QVariantList{weekdayOf(1)});
        const QVariantMap p = b.previewSlotUpdate(slot, rule);
        QCOMPARE(p.value("removed").toInt(), 2);
        QCOMPARE(p.value("added").toInt(), 3);
        QCOMPARE(p.value("kept").toInt(), 1);
        QVERIFY(b.updateSlot(slot, rule, false).value("ok").toBool());

        QStringList dates;
        for (const QVariantMap &s : slotSessions(b, slot))
            dates << s.value("date").toString();
        QCOMPARE(dates, (QStringList{day(-14), day(-7), day(1), day(7), day(8), day(15)}));
    }

    void updateWarnsAboutClashesWithOtherSessions()
    {
        Backend b(m_db.handle());
        const int slot = b.createRecurring(weekly(), false).value("slotId").toInt();
        QVERIFY(b.saveSession({{"teamId", m_team}, {"date", day(7)}, {"time", "20:00"}, {"durationMin", 60}}, true)
                    .value("ok").toBool());
        QVariantMap rule = weekly();
        rule.insert("time", "19:30");
        const QVariantMap r = b.updateSlot(slot, rule, false);
        QVERIFY(!r.value("ok").toBool());
        QVERIFY(r.value("needsConfirm").toBool());
        QCOMPARE(r.value("clashes").toInt(), 1);
        QCOMPARE(slotSessions(b, slot).at(3).value("time").toString(), QStringLiteral("18:30"));   // rien écrit
        QVERIFY(b.updateSlot(slot, rule, true).value("ok").toBool());
        QCOMPARE(slotSessions(b, slot).at(3).value("time").toString(), QStringLiteral("19:30"));
    }

    void invalidUpdateIsRefused()
    {
        Backend b(m_db.handle());
        const int slot = b.createRecurring(weekly(), false).value("slotId").toInt();
        QVariantMap rule = weekly();
        rule.insert("weekdays", QVariantList{});
        QVERIFY(!b.updateSlot(slot, rule, false).value("ok").toBool());
        QVERIFY(!b.updateSlot(9999, weekly(), false).value("ok").toBool());
        QCOMPARE(slotSessions(b, slot).size(), 5);
    }

    void deleteRemovesUpcomingWithoutAttendanceByDefault()
    {
        Backend b(m_db.handle());
        const int slot = b.createRecurring(weekly(), false).value("slotId").toInt();
        const QVariantMap player = b.savePlayer({{"teamId", m_team}, {"firstName", "Léo"}});
        const QList<QVariantMap> all = slotSessions(b, slot);
        // Appel sur J-7 (passée) et J+7 (à venir) ; plan d'exercices sur J+14.
        for (int i : {1, 3})
            QVERIFY(b.saveAttendance(all.at(i).value("id").toInt(),
                                     {QVariantMap{{"playerId", player.value("id")}, {"status", "present"}}}, false)
                        .value("ok").toBool());
        QSqlQuery q(m_db.handle());
        QVERIFY(q.exec(QStringLiteral("INSERT INTO session_exercise(session_id, exercise_id, position, duration_min) "
                                      "VALUES(%1, (SELECT MIN(id) FROM exercise), 0, 10)").arg(all.at(4).value("id").toInt())));

        const QVariantMap p = b.previewSlotDelete(slot);
        QVERIFY(p.value("ok").toBool());
        QCOMPARE(p.value("upcoming").toInt(), 2);     // J et J+14
        QCOMPARE(p.value("past").toInt(), 1);         // J-14
        QCOMPARE(p.value("kept").toInt(), 2);         // J-7 et J+7 (appel)
        QCOMPARE(p.value("withPlan").toInt(), 1);

        const QVariantMap r = b.deleteSlot(slot, false);
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(r.value("removed").toInt(), 2);
        QVERIFY(b.trainingSlots(m_team).isEmpty());
        QStringList dates;
        for (const QVariant &v : b.sessions(m_team, day(-60), day(60))) {
            dates << v.toMap().value("date").toString();
            QCOMPARE(v.toMap().value("slotId").toInt(), -1);              // devenues des séances seules
        }
        QCOMPARE(dates, (QStringList{day(-14), day(-7), day(7)}));
        QCOMPARE(count(m_db.handle(), QStringLiteral("SELECT COUNT(*) FROM session_exercise")), 0);
    }

    void deleteCanIncludePastSessionsWithoutAttendance()
    {
        Backend b(m_db.handle());
        const int slot = b.createRecurring(weekly(), false).value("slotId").toInt();
        const QVariantMap r = b.deleteSlot(slot, true);
        QVERIFY(r.value("ok").toBool());
        QCOMPARE(r.value("removed").toInt(), 5);
        QVERIFY(b.sessions(m_team, day(-60), day(60)).isEmpty());
        QVERIFY(!b.deleteSlot(slot, true).value("ok").toBool());           // déjà supprimé
    }

    // La modification de la convocation garde la présence des joueurs toujours convoqués (schéma 10).
    void changingConvocationKeepsPresence()
    {
        Backend b(m_db.handle());
        const int p1 = b.savePlayer({{"teamId", m_team}, {"firstName", "Ana"}}).value("id").toInt();
        const int p2 = b.savePlayer({{"teamId", m_team}, {"firstName", "Bob"}}).value("id").toInt();
        const int p3 = b.savePlayer({{"teamId", m_team}, {"firstName", "Cléo"}}).value("id").toInt();
        const QVariantMap m = b.saveMatch({{"teamId", m_team}, {"date", day(3)}, {"time", "10:00"}, {"opponent", "FC Test"}});
        QVERIFY2(m.value("ok").toBool(), qPrintable(m.value("error").toString()));
        const int match = m.value("id").toInt();
        QVERIFY(b.setMatchPlayers(match, {QVariantMap{{"playerId", p1}, {"role", "starter"}},
                                          QVariantMap{{"playerId", p2}, {"role", "starter"}}}).value("ok").toBool());
        QSqlQuery q(m_db.handle());
        QVERIFY(q.exec(QStringLiteral("UPDATE match_player SET presence = 'absent' WHERE player_id = %1").arg(p1)));
        QCOMPARE(count(m_db.handle(), QStringLiteral("SELECT COUNT(*) FROM match_player WHERE presence = 'unknown'")), 1);

        QVERIFY(b.setMatchPlayers(match, {QVariantMap{{"playerId", p1}, {"role", "substitute"}},
                                          QVariantMap{{"playerId", p3}, {"role", "starter"}}}).value("ok").toBool());
        QCOMPARE(count(m_db.handle(), QStringLiteral("SELECT COUNT(*) FROM match_player")), 2);
        QCOMPARE(count(m_db.handle(), QStringLiteral("SELECT COUNT(*) FROM match_player WHERE player_id = %1 "
                                                     "AND presence = 'absent' AND role = 'substitute'").arg(p1)), 1);
        QCOMPARE(count(m_db.handle(), QStringLiteral("SELECT COUNT(*) FROM match_player WHERE player_id = %1 "
                                                     "AND presence = 'unknown'").arg(p3)), 1);
        QVERIFY(b.setMatchPlayers(match, {}).value("ok").toBool());
        QCOMPARE(count(m_db.handle(), QStringLiteral("SELECT COUNT(*) FROM match_player")), 0);
    }

    // Migration 010 sur une base du schéma 9 : téléphone et e-mail existants deviennent le contact 1.
    void migrationCopiesExistingPhoneAndEmailToContact1()
    {
        QSqlQuery q(m_db.handle());
        // On revient au schéma 9 pour rejouer la migration 010 (et la 011, qui la suit).
        QVERIFY(q.exec(QStringLiteral("DROP TABLE opponent_venue")));
        QVERIFY(q.exec(QStringLiteral("DROP INDEX idx_match_venue")));
        QVERIFY(q.exec(QStringLiteral("ALTER TABLE football_match DROP COLUMN venue_id")));
        QVERIFY(q.exec(QStringLiteral("DROP TABLE venue")));
        QVERIFY(q.exec(QStringLiteral("ALTER TABLE club DROP COLUMN department")));
        QVERIFY(q.exec(QStringLiteral("DROP TABLE player_contact")));
        QVERIFY(q.exec(QStringLiteral("DROP INDEX idx_session_slot")));
        QVERIFY(q.exec(QStringLiteral("ALTER TABLE training_session DROP COLUMN slot_id")));
        QVERIFY(q.exec(QStringLiteral("DROP TABLE training_slot")));
        QVERIFY(q.exec(QStringLiteral("ALTER TABLE match_player DROP COLUMN presence")));
        QVERIFY(q.exec(QStringLiteral("PRAGMA user_version = 9")));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO player(team_id, first_name, last_name, phone, email) VALUES "
                                      "(%1, 'A', 'A', ' 0601020304 ', NULL), (%1, 'B', 'B', '', 'b@x.fr'), "
                                      "(%1, 'C', 'C', '  ', '')").arg(m_team)));
        m_db.close();
        QString error;
        QVERIFY2(m_db.open(m_dir.filePath(QStringLiteral("t.db")), &error), qPrintable(error));
        QCOMPARE(m_db.schemaVersion(), 11);
        const QSqlDatabase db = m_db.handle();
        QCOMPARE(count(db, QStringLiteral("SELECT COUNT(*) FROM player_contact")), 2);
        QCOMPARE(count(db, QStringLiteral("SELECT COUNT(*) FROM player_contact c JOIN player p ON p.id = c.player_id "
                                          "WHERE p.first_name = 'A' AND c.rank = 1 AND c.phone = '0601020304' AND c.email IS NULL")), 1);
        QCOMPARE(count(db, QStringLiteral("SELECT COUNT(*) FROM player_contact c JOIN player p ON p.id = c.player_id "
                                          "WHERE p.first_name = 'B' AND c.phone IS NULL AND c.email = 'b@x.fr'")), 1);
        QCOMPARE(count(db, QStringLiteral("SELECT COUNT(*) FROM training_slot")), 0);
    }

    // Export puis import : créneaux, liens des séances et présences voyagent avec le fichier.
    void slotsSurviveExportAndImport()
    {
        Backend b(m_db.handle());
        const int slot = b.createRecurring(weekly(), false).value("slotId").toInt();
        const QString file = m_dir.filePath(QStringLiteral("s.footcoach"));
        QVERIFY(b.exportDataFile(file).value("ok").toBool());
        QVERIFY(b.deleteSlot(slot, true).value("ok").toBool());
        QVERIFY(b.trainingSlots(m_team).isEmpty());
        const QVariantMap r = b.importDataFile(file);
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(b.trainingSlots(m_team).size(), 1);
        QCOMPARE(slotSessions(b, slot).size(), 5);
    }
};

QTEST_MAIN(SlotsTest)
#include "tst_slots.moc"
