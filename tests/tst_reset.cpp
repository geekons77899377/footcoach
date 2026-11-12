// Effacement de toutes les données (retour à une première installation), suppression et déplacement de plusieurs
// joueurs.
#include "app/Backend.h"
#include "core/Database.h"

#include <QDir>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

namespace {

int count(const QSqlDatabase &db, const QString &sql)
{
    QSqlQuery q(db);
    return q.exec(sql) && q.next() ? q.value(0).toInt() : -1;
}

QStringList column(const QSqlDatabase &db, const QString &sql)
{
    QStringList out;
    QSqlQuery q(db);
    if (q.exec(sql))
        while (q.next())
            out << q.value(0).toString();
    return out;
}

const QStringList kTables = {"club", "team", "player", "player_contact", "training_slot", "training_session",
                             "attendance", "exercise_category", "exercise", "session_exercise", "tactical_board",
                             "match", "match_player"};

} // namespace

class ResetTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    // Remplit la base avec une équipe, des joueurs, un créneau, un appel, un match et un exercice perso.
    static void fill(Backend &b)
    {
        const int team = b.saveTeam({{"name", "U11"}}).value("id").toInt();
        QVERIFY(team > 0);
        const int p1 = b.savePlayer({{"teamId", team}, {"firstName", "Ana"}, {"lastName", "A"}}).value("id").toInt();
        QVERIFY(b.savePlayer({{"teamId", team}, {"firstName", "Bob"}, {"lastName", "B"}}).value("ok").toBool());
        const QString today = QDate::currentDate().toString("yyyy-MM-dd");
        const QVariantMap r = b.createRecurring({{"teamId", team}, {"from", today},
                                                 {"to", QDate::currentDate().addDays(30).toString("yyyy-MM-dd")},
                                                 {"weekdays", QVariantList{1, 3}}, {"time", "18:00"}, {"durationMin", 90}}, true);
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QVERIFY(b.saveAttendance(r.value("id").toInt(), {QVariantMap{{"playerId", p1}, {"status", "present"}}}, false)
                    .value("ok").toBool());
        const int match = b.saveMatch({{"teamId", team}, {"date", today}, {"time", "10:00"}, {"opponent", "FC X"}}).value("id").toInt();
        QVERIFY(match > 0);
        QVERIFY(b.setMatchPlayers(match, {QVariantMap{{"playerId", p1}, {"role", "starter"}}}).value("ok").toBool());
        QVERIFY(b.saveExercise({{"title", "Mon exercice"}, {"category", "technical"}, {"durationMin", 10}}).value("ok").toBool());
    }

private slots:
    void resetGivesBackAFreshInstall()
    {
        Database fresh(QStringLiteral("fresh"));
        QVERIFY(fresh.open(m_dir.filePath("fresh.db")));
        Database db(QStringLiteral("used"));
        QVERIFY(db.open(m_dir.filePath("used.db")));
        Backend b(db.handle());
        fill(b);
        QVERIFY(count(db.handle(), "SELECT COUNT(*) FROM player") == 2);

        QSignalSpy replaced(&b, &Backend::dataReplaced);
        const QVariantMap r = b.resetAllData();
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(replaced.count(), 1);

        for (const QString &t : kTables)
            QCOMPARE(count(db.handle(), "SELECT COUNT(*) FROM " + t), count(fresh.handle(), "SELECT COUNT(*) FROM " + t));
        QCOMPARE(count(db.handle(), "SELECT COUNT(*) FROM club"), 1);           // « Mon club », comme à l'installation
        QVERIFY(count(db.handle(), "SELECT COUNT(*) FROM exercise") >= 40);     // bibliothèque et exemples U10-U11
        const QString ids = "SELECT id || ':' || title || ':' || IFNULL(category_id, '') FROM exercise ORDER BY id";
        QCOMPARE(column(db.handle(), ids), column(fresh.handle(), ids));       // mêmes identifiants qu'une base neuve
        const QString boards = "SELECT exercise_id || ':' || title FROM tactical_board ORDER BY id";
        QCOMPARE(column(db.handle(), boards), column(fresh.handle(), boards));  // terrains reliés aux bons exercices
        QCOMPARE(db.schemaVersion(), fresh.schemaVersion());

        // Copie de sécurité écrite avant, importable.
        const QString name = r.value("backupName").toString();
        QVERIFY(name.startsWith("avant-effacement-"));
        const QString path = m_dir.filePath("sauvegardes/" + name);
        QVERIFY(QFile::exists(path));
        const QVariantMap info = b.inspectDataFile(QUrl::fromLocalFile(path).toString());
        QVERIFY2(info.value("ok").toBool(), qPrintable(info.value("error").toString()));
        QCOMPARE(info.value("counts").toMap().value("players").toInt(), 2);
        QVERIFY(b.importDataFile(QUrl::fromLocalFile(path).toString()).value("ok").toBool());
        QCOMPARE(count(db.handle(), "SELECT COUNT(*) FROM player"), 2);
        QCOMPARE(count(db.handle(), "SELECT COUNT(*) FROM training_slot"), 1);

        // On peut retravailler normalement après l'effacement.
        QVERIFY(b.resetAllData().value("ok").toBool());
        QVERIFY(b.saveTeam({{"name", "U13"}}).value("ok").toBool());
        QCOMPARE(b.dataCounts().value("teams").toInt(), 1);
    }

    void onlyTheThreeLatestBackupsAreKept()
    {
        Database db(QStringLiteral("rot"));
        QVERIFY(db.open(m_dir.filePath("rot/r.db")));
        Backend b(db.handle());
        QStringList names;
        for (int i = 0; i < 5; ++i) {
            const QVariantMap r = b.resetAllData();
            QVERIFY(r.value("ok").toBool());
            names << r.value("backupName").toString();
        }
        QCOMPARE(QSet<QString>(names.begin(), names.end()).size(), 5);         // jamais d'écrasement
        const QStringList left = QDir(m_dir.filePath("rot/sauvegardes")).entryList({"avant-effacement-*.footcoach"}, QDir::Files, QDir::Name);
        QCOMPARE(left, names.mid(2));
    }

    void deletePlayersRemovesTheSelectionAtOnce()
    {
        Database db(QStringLiteral("del"));
        QVERIFY(db.open(m_dir.filePath("del.db")));
        Backend b(db.handle());
        const int team = b.saveTeam({{"name", "U11"}}).value("id").toInt();
        QList<int> ids;
        for (const char *n : {"Ana", "Bob", "Cléo", "Dan"})
            ids << b.savePlayer({{"teamId", team}, {"firstName", n}, {"lastName", "X"}}).value("id").toInt();
        const QVariantMap s = b.saveSession({{"teamId", team}, {"date", QDate::currentDate().toString("yyyy-MM-dd")},
                                             {"time", "18:00"}, {"durationMin", 60}}, true);
        QVERIFY(b.saveAttendance(s.value("id").toInt(), {QVariantMap{{"playerId", ids[0]}, {"status", "present"}},
                                                         QVariantMap{{"playerId", ids[3]}, {"status", "absent"}}}, false)
                    .value("ok").toBool());

        const QVariantMap r = b.deletePlayers({ids[0], ids[1], ids[1]});   // doublon ignoré
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(r.value("removed").toInt(), 2);
        QCOMPARE(column(db.handle(), "SELECT first_name FROM player ORDER BY id"), (QStringList{"Cléo", "Dan"}));
        QCOMPARE(count(db.handle(), "SELECT COUNT(*) FROM attendance"), 1);     // présences supprimées avec le joueur

        const QVariantMap empty = b.deletePlayers({});
        QVERIFY(!empty.value("ok").toBool());
        QCOMPARE(empty.value("removed").toInt(), 0);
        QCOMPARE(count(db.handle(), "SELECT COUNT(*) FROM player"), 2);
    }

    void movePlayersToAnotherTeam()
    {
        Database db(QStringLiteral("move"));
        QVERIFY(db.open(m_dir.filePath("move.db")));
        Backend b(db.handle());
        const int u11 = b.saveTeam({{"name", "U11"}}).value("id").toInt();
        const int u13 = b.saveTeam({{"name", "U13"}}).value("id").toInt();
        auto add = [&](int team, const char *first, int number, const char *licence) {
            const QVariantMap r = b.savePlayer({{"teamId", team}, {"firstName", first}, {"lastName", "X"},
                                                {"jerseyNumber", number}, {"licence", licence}});
            if (!r.value("ok").toBool())
                qWarning() << r.value("error");
            return r.value("id").toInt();
        };
        const int ana = add(u11, "Ana", 9, "");          // n° 9 libre en U13 : gardé
        const int bob = add(u11, "Bob", 10, "");         // n° 10 pris en U13 : retiré
        const int leo = add(u11, "Léo", 0, "");          // même nom qu'un joueur de U13 : pas déplacé
        const int max = add(u11, "Max", 0, "AB123");     // même licence qu'un joueur de U13 : pas déplacé
        add(u13, "Zoé", 10, "");
        add(u13, "Léo", 0, "");
        add(u13, "Tom", 0, "ab123");
        QVERIFY(ana > 0 && bob > 0 && leo > 0 && max > 0);
        const int session = b.saveSession({{"teamId", u11}, {"date", QDate::currentDate().toString("yyyy-MM-dd")},
                                           {"time", "18:00"}, {"durationMin", 60}}, true).value("id").toInt();
        QVERIFY(b.saveAttendance(session, {QVariantMap{{"playerId", ana}, {"status", "present"}}}, false).value("ok").toBool());

        const QVariantMap r = b.movePlayers({ana, bob, leo, max}, u13);
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(r.value("moved").toInt(), 2);
        QCOMPARE(r.value("numbersCleared").toInt(), 1);
        QCOMPARE(r.value("skipped").toStringList(), (QStringList{"Léo X", "Max X"}));
        QCOMPARE(column(db.handle(), QString("SELECT first_name || ':' || IFNULL(jersey_number, '-') FROM player "
                                             "WHERE team_id = %1 ORDER BY first_name").arg(u13)),
                 (QStringList{"Ana:9", "Bob:-", "Léo:-", "Tom:-", "Zoé:10"}));
        QCOMPARE(column(db.handle(), QString("SELECT first_name FROM player WHERE team_id = %1 ORDER BY first_name").arg(u11)),
                 (QStringList{"Léo", "Max"}));
        QCOMPARE(count(db.handle(), QString("SELECT COUNT(*) FROM attendance WHERE player_id = %1").arg(ana)), 1);   // historique gardé

        QVERIFY(b.movePlayers({ana}, u13).value("moved").toInt() == 0);            // déjà dans l'équipe
        QVERIFY(!b.movePlayers({ana}, 9999).value("ok").toBool());
        QVERIFY(!b.movePlayers({}, u11).value("ok").toBool());
    }
};

QTEST_MAIN(ResetTest)
#include "tst_reset.moc"
