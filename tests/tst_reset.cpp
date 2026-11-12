// Effacement de toutes les données (retour à une première installation) et suppression de plusieurs joueurs.
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
};

QTEST_MAIN(ResetTest)
#include "tst_reset.moc"
