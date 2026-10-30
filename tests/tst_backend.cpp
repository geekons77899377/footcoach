#include "app/Backend.h"
#include "core/Database.h"
#include "core/TacticalBoard.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class BackendTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    Database m_db;

private slots:
    void init()
    {
        m_db.close();
        QString error;
        QVERIFY2(m_db.open(m_dir.filePath(QStringLiteral("t.db")), &error), qPrintable(error));
    }

    void cleanup()
    {
        m_db.close();
        QFile::remove(m_dir.filePath(QStringLiteral("t.db")));
    }

    void createListRenameDelete()
    {
        Backend b(m_db.handle());
        QVERIFY(b.boards().isEmpty());

        const int id = b.createBoard(QStringLiteral("  Pressing haut "));
        QVERIFY(id > 0);
        QCOMPARE(b.createBoard(QString()) > 0, true);   // titre par défaut

        const QVariantList list = b.boards();
        QCOMPARE(list.size(), 2);
        QVariantMap mine;
        for (const QVariant &v : list)
            if (v.toMap().value(QStringLiteral("id")).toInt() == id)
                mine = v.toMap();
        QCOMPARE(mine.value(QStringLiteral("title")).toString(), QStringLiteral("Pressing haut"));
        QVERIFY(mine.value(QStringLiteral("readable")).toBool());
        QCOMPARE(mine.value(QStringLiteral("itemCount")).toInt(), 0);

        QVERIFY(b.renameBoard(id, QStringLiteral("Sortie de balle")));
        QCOMPARE(b.board(id).value(QStringLiteral("title")).toString(), QStringLiteral("Sortie de balle"));
        QVERIFY(!b.renameBoard(id, QStringLiteral("   ")));
        QVERIFY(!b.lastError().isEmpty());
        QCOMPARE(b.board(id).value(QStringLiteral("title")).toString(), QStringLiteral("Sortie de balle"));

        QVERIFY(b.deleteBoard(id));
        QCOMPARE(b.boards().size(), 1);
        QVERIFY(b.board(id).isEmpty());
    }

    void saveValidatesAndNormalizes()
    {
        Backend b(m_db.handle());
        const int id = b.createBoard(QStringLiteral("A"));

        // Coordonnées hors du terrain : ramenées dans [0, 1] ; rotation d'un joueur ignorée.
        const QString json = QStringLiteral(
            "{\"version\":1,\"pitch\":\"half\",\"items\":["
            "{\"kind\":\"player\",\"x\":1.4,\"y\":-2,\"label\":\"9\",\"rot\":90}],\"arrows\":[]}");
        QVERIFY2(b.saveBoard(id, json), qPrintable(b.lastError()));

        Tactics::Board back;
        QVERIFY(Tactics::fromJson(b.board(id).value(QStringLiteral("json")).toString(), &back));
        QCOMPARE(back.pitch, Tactics::Pitch::Half);
        QCOMPARE(back.items.size(), 1);
        QCOMPARE(back.items.first().pos, QPointF(1, 0));
        QCOMPARE(back.items.first().rotation, 0.0);

        // Un JSON invalide est refusé et ne change rien.
        QVERIFY(!b.saveBoard(id, QStringLiteral("{pas du json")));
        QVERIFY(!b.saveBoard(id, QStringLiteral("{\"version\":99}")));
        QVERIFY(Tactics::fromJson(b.board(id).value(QStringLiteral("json")).toString(), &back));
        QCOMPARE(back.items.size(), 1);
        QVERIFY(!b.saveBoard(9999, json));   // id inconnu
    }

    void unreadableBoardIsNeverOverwritten()
    {
        Backend b(m_db.handle());
        const int id = b.createBoard(QStringLiteral("Cassé"));
        {
            QSqlQuery q(m_db.handle());
            q.prepare(QStringLiteral("UPDATE tactical_board SET scene_json = ? WHERE id = ?"));
            q.addBindValue(QStringLiteral("{\"version\":42,\"items\":[]}"));
            q.addBindValue(id);
            QVERIFY(q.exec());
        }
        const QVariantMap m = b.board(id);
        QVERIFY(!m.value(QStringLiteral("readable")).toBool());
        QVERIFY(!m.value(QStringLiteral("error")).toString().isEmpty());
        QVERIFY(m.value(QStringLiteral("json")).toString().isEmpty());
        QVERIFY(!b.boards().first().toMap().value(QStringLiteral("readable")).toBool());

        QVERIFY(!b.saveBoard(id, QStringLiteral("{}")));
        QSqlQuery q(m_db.handle());
        QVERIFY(q.exec(QStringLiteral("SELECT scene_json FROM tactical_board WHERE id = %1").arg(id)));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), QStringLiteral("{\"version\":42,\"items\":[]}"));
        // On peut quand même le renommer et le supprimer.
        QVERIFY(b.renameBoard(id, QStringLiteral("Cassé 2")));
        QVERIFY(b.deleteBoard(id));
    }

    void duplicateCopiesContent()
    {
        Backend b(m_db.handle());
        const int id = b.createBoard(QStringLiteral("Original"));
        QVERIFY(b.saveBoard(id, b.formationJson(QStringLiteral("4-3-3"), false)));
        const int copy = b.duplicateBoard(id);
        QVERIFY(copy > 0 && copy != id);
        QCOMPARE(b.board(copy).value(QStringLiteral("title")).toString(), QStringLiteral("Original (copie)"));
        QCOMPARE(b.board(copy).value(QStringLiteral("json")).toString(), b.board(id).value(QStringLiteral("json")).toString());
        QCOMPARE(b.duplicateBoard(9999), -1);
    }

    void formationsAndPalette()
    {
        Backend b(m_db.handle());
        QVERIFY(b.formationNames().contains(QStringLiteral("4-4-2")));

        Tactics::Board board;
        QVERIFY(Tactics::fromJson(b.formationJson(QStringLiteral("4-4-2"), false), &board));
        QCOMPARE(board.items.size(), 11);
        QVERIFY(Tactics::fromJson(b.formationJson(QStringLiteral("4-4-2"), true), &board));
        QCOMPARE(board.items.size(), 22);
        QCOMPARE(int(board.items.last().kind), int(Tactics::Kind::Opponent));
        QVERIFY(b.formationJson(QStringLiteral("9-9-9"), false).isEmpty());

        const QVariantList palette = b.palette();
        QVERIFY(palette.size() >= 5);
        QVERIFY(palette.first().toMap().value(QStringLiteral("color")).toString().startsWith(QLatin1Char('#')));
    }

    void errorSignalIsEmittedOnChangeOnly()
    {
        Backend b(m_db.handle());
        QSignalSpy spy(&b, &Backend::lastErrorChanged);
        QVERIFY(!b.saveBoard(12345, QStringLiteral("{}")));
        QCOMPARE(spy.count(), 1);
        QVERIFY(!b.saveBoard(12345, QStringLiteral("{}")));
        QCOMPARE(spy.count(), 1);        // même message : pas de nouveau signal
        QVERIFY(b.createBoard(QStringLiteral("x")) > 0);
        QCOMPARE(spy.count(), 2);        // effacé
        QVERIFY(b.lastError().isEmpty());
    }

    void teamsAndPlayers()
    {
        Backend b(m_db.handle());
        QVERIFY(b.teams().isEmpty());
        QVERIFY(!b.saveTeam({{"name", "  "}}).value("ok").toBool());
        const QVariantMap t = b.saveTeam({{"name", "U15"}, {"category", "U15"}, {"season", "2026-2027"}});
        QVERIFY(t.value("ok").toBool());
        const int team = t.value("id").toInt();
        QVERIFY(team > 0);
        QCOMPARE(b.teams().size(), 1);

        auto player = [&](const QString &first, const QString &last, int jersey) {
            return QVariantMap{{"teamId", team}, {"firstName", first}, {"lastName", last},
                               {"jerseyNumber", jersey}, {"position", "MID"}, {"birthDate", "2012-05-04"}};
        };
        const QVariantMap a = b.savePlayer(player("Léo", "Martin", 10));
        QVERIFY2(a.value("ok").toBool(), qPrintable(a.value("error").toString()));
        const int idA = a.value("id").toInt();

        // Numéro déjà pris, nom vide, date invalide, poste inconnu : refus.
        QVERIFY(!b.savePlayer(player("Max", "Durand", 10)).value("ok").toBool());
        QVERIFY(!b.savePlayer(player("", "Durand", 7)).value("ok").toBool());
        QVariantMap bad = player("Max", "Durand", 7);
        bad["birthDate"] = "31/02/2012";
        QVERIFY(!b.savePlayer(bad).value("ok").toBool());
        bad = player("Max", "Durand", 7);
        bad["position"] = "XXX";
        QVERIFY(!b.savePlayer(bad).value("ok").toBool());
        QCOMPARE(b.players(team, true).size(), 1);

        // Modifier le joueur sans changer son numéro reste possible.
        QVariantMap edit = player("Léo", "Martin", 10);
        edit["id"] = idA;
        edit["phone"] = "0600000000";
        QVERIFY(b.savePlayer(edit).value("ok").toBool());
        const QVariantMap read = b.players(team, true).first().toMap();
        QCOMPARE(read.value("phone").toString(), QStringLiteral("0600000000"));
        QCOMPARE(read.value("birthDate").toString(), QStringLiteral("2012-05-04"));
        QVERIFY(read.value("age").toInt() >= 13);

        // Désactivé : son numéro se libère ; la réactivation est refusée si le numéro a été repris.
        QVERIFY(b.setPlayerActive(idA, false).value("ok").toBool());
        QCOMPARE(b.players(team, false).size(), 0);
        QVERIFY(b.savePlayer(player("Max", "Durand", 10)).value("ok").toBool());
        QVERIFY(!b.setPlayerActive(idA, true).value("ok").toBool());

        QVERIFY(b.deletePlayer(idA).value("ok").toBool());
        QCOMPARE(b.players(team, true).size(), 1);
        QCOMPARE(b.teams().first().toMap().value("playerCount").toInt(), 1);
        QVERIFY(b.deleteTeam(team).value("ok").toBool());
        QVERIFY(b.teams().isEmpty());
    }

    void sessionsAttendanceAndStats()
    {
        Backend b(m_db.handle());
        const int team = b.saveTeam({{"name", "U15"}}).value("id").toInt();
        auto player = [&](const QString &last, int jersey) {
            return b.savePlayer({{"teamId", team}, {"firstName", "X"}, {"lastName", last}, {"jerseyNumber", jersey}})
                .value("id").toInt();
        };
        const int p1 = player("Alpha", 1), p2 = player("Bravo", 2);

        QVariantMap s{{"teamId", team}, {"date", "2026-11-03"}, {"time", "18:30"}, {"durationMin", 90}, {"theme", "Finition"}};
        const QVariantMap a = b.saveSession(s, false);
        QVERIFY2(a.value("ok").toBool(), qPrintable(a.value("error").toString()));
        const int sid = a.value("id").toInt();

        // Chevauchement : confirmation demandée, puis possible avec force.
        QVariantMap clash = s;
        clash["time"] = "19:00";
        const QVariantMap c = b.saveSession(clash, false);
        QVERIFY(!c.value("ok").toBool());
        QVERIFY(c.value("needsConfirm").toBool());
        QCOMPARE(b.sessions(team, "2026-11-01", "2026-11-30").size(), 1);
        QVERIFY(b.saveSession(clash, true).value("ok").toBool());
        QCOMPARE(b.sessions(team, "2026-11-01", "2026-11-30").size(), 2);
        QCOMPARE(b.sessionDates(team, "2026-11-01", "2026-11-30"), QStringList{"2026-11-03"});

        // Validations.
        QVariantMap bad = s;
        bad["time"] = "25:00";
        QVERIFY(!b.saveSession(bad, true).value("ok").toBool());
        bad = s;
        bad["durationMin"] = 5;
        QVERIFY(!b.saveSession(bad, true).value("ok").toBool());

        // Appel : par défaut absents ; enregistrement + passage à « réalisée ».
        const QVariantMap sheet = b.attendanceSheet(sid);
        QVERIFY(sheet.value("ok").toBool());
        QCOMPARE(sheet.value("players").toList().size(), 2);
        QCOMPARE(sheet.value("players").toList().first().toMap().value("status").toString(), QStringLiteral("absent"));
        const QVariantList entries{QVariantMap{{"playerId", p1}, {"status", "present"}},
                                   QVariantMap{{"playerId", p2}, {"status", "excused"}, {"note", "malade"}}};
        QVERIFY(b.saveAttendance(sid, entries, true).value("ok").toBool());
        QVERIFY(!b.saveAttendance(sid, {QVariantMap{{"playerId", p1}, {"status", "???"}}}, false).value("ok").toBool());
        const QVariantMap first = b.sessions(team, "2026-11-03", "2026-11-03").first().toMap();
        QCOMPARE(first.value("status").toString(), QStringLiteral("done"));
        QCOMPARE(first.value("present").toInt(), 1);
        QCOMPARE(first.value("total").toInt(), 2);

        // Statistiques : p1 présent (100 %), p2 excusé (hors taux).
        const QVariantMap st = b.attendanceStats(team, "2026-01-01", "2026-12-31");
        QCOMPARE(st.value("sessions").toList().size(), 1);
        const QVariantList rows = st.value("players").toList();
        QCOMPARE(rows.first().toMap().value("id").toInt(), p1);
        QCOMPARE(rows.first().toMap().value("rate").toDouble(), 1.0);
        QCOMPARE(rows.at(1).toMap().value("excused").toInt(), 1);
        QCOMPARE(rows.at(1).toMap().value("rate").toDouble(), -1.0);

        // Séance annulée : pas d'appel, et son statut ne bloque pas.
        QVERIFY(b.setSessionStatus(sid, "cancelled", false).value("ok").toBool());
        QVERIFY(!b.attendanceSheet(sid).value("ok").toBool());

        // Récurrence : lundi et jeudi pendant deux semaines.
        QVariantMap rule{{"teamId", team}, {"from", "2026-12-01"}, {"to", "2026-12-14"}, {"weekdays", QVariantList{1, 4}},
                         {"time", "18:00"}, {"durationMin", 90}};
        const QVariantMap pre = b.previewRecurrence(rule);
        QVERIFY(pre.value("ok").toBool());
        QCOMPARE(pre.value("count").toInt(), 4);        // jeu 3, lun 7, jeu 10, lun 14
        QCOMPARE(b.createRecurring(rule, false).value("count").toInt(), 4);
        QVERIFY(b.createRecurring(rule, false).value("needsConfirm").toBool());   // même créneaux : conflits
        QCOMPARE(b.sessions(team, "2026-12-01", "2026-12-31").size(), 4);
        rule["weekdays"] = QVariantList();
        QVERIFY(!b.previewRecurrence(rule).value("ok").toBool());

        QVERIFY(b.deleteSession(sid).value("ok").toBool());
        QCOMPARE(b.sessions(team, "2026-11-01", "2026-11-30").size(), 1);
    }
};

QTEST_GUILESS_MAIN(BackendTest)
#include "tst_backend.moc"
