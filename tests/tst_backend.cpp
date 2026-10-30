#include "app/Backend.h"
#include "core/Database.h"
#include "core/TacticalBoard.h"

#include <QJsonArray>
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

    void exercisesAndPlan()
    {
        Backend b(m_db.handle());

        // Bibliothèque de départ, catégories, validation.
        const QVariantList base = b.exercises(false);
        QVERIFY(base.size() >= 10);
        QCOMPARE(b.exerciseCategories().size(), 6);
        QVERIFY(!b.saveExercise({{"title", "  "}}).value("ok").toBool());
        QVERIFY(!b.saveExercise({{"title", "X"}, {"category", "inconnu"}}).value("ok").toBool());
        QVERIFY(!b.saveExercise({{"title", "X"}, {"playersMin", 8}, {"playersMax", 4}}).value("ok").toBool());

        const QVariantMap e1 = b.saveExercise({{"title", " Rondo 5 contre 2 "}, {"category", "technical"},
                                               {"durationMin", 20}, {"equipment", "Ballons, plots; chasubles"}});
        QVERIFY2(e1.value("ok").toBool(), qPrintable(e1.value("error").toString()));
        const int ex1 = e1.value("id").toInt();
        const QVariantMap e2 = b.saveExercise({{"title", "Jeu réduit"}, {"category", "game"},
                                               {"equipment", "ballons, Cages"}});
        const int ex2 = e2.value("id").toInt();
        QCOMPARE(b.exercises(false).size(), base.size() + 2);

        // Séance + plan : durée par défaut (conseillée, sinon 15), horaires, total, matériel sans doublon.
        const int team = b.saveTeam({{"name", "U15"}}).value("id").toInt();
        const int sess = b.saveSession({{"teamId", team}, {"date", "2026-11-03"}, {"time", "18:30"},
                                        {"durationMin", 30}}, false).value("id").toInt();
        QVERIFY(sess > 0);
        const int i1 = b.addToPlan(sess, ex1, 0).value("id").toInt();
        const int i2 = b.addToPlan(sess, ex2, 0).value("id").toInt();
        QVERIFY(i1 > 0 && i2 > 0);

        QVariantMap plan = b.plan(sess);
        QVERIFY(plan.value("ok").toBool());
        QVariantList rows = plan.value("entries").toList();
        QCOMPARE(rows.size(), 2);
        QCOMPARE(rows.at(0).toMap().value("durationMin").toInt(), 20);     // conseillée
        QCOMPARE(rows.at(1).toMap().value("durationMin").toInt(), 15);     // à défaut
        QCOMPARE(rows.at(0).toMap().value("start").toString(), QStringLiteral("18:30"));
        QCOMPARE(rows.at(1).toMap().value("start").toString(), QStringLiteral("18:50"));
        QCOMPARE(rows.at(1).toMap().value("end").toString(), QStringLiteral("19:05"));
        QCOMPARE(plan.value("totalMin").toInt(), 35);
        QCOMPARE(plan.value("sessionMin").toInt(), 30);
        const QStringList gear = plan.value("equipment").toStringList();
        QCOMPARE(gear, (QStringList{"Ballons", "plots", "chasubles", "Cages"}));

        // Modification, ordre, retrait.
        QVERIFY(b.updatePlanItem(i2, 10, "Deux touches").value("ok").toBool());
        QVERIFY(!b.updatePlanItem(i2, 999, "").value("ok").toBool());
        QVERIFY(b.movePlanItem(i2, -1).value("ok").toBool());
        rows = b.plan(sess).value("entries").toList();
        QCOMPARE(rows.at(0).toMap().value("itemId").toInt(), i2);
        QCOMPARE(rows.at(0).toMap().value("notes").toString(), QStringLiteral("Deux touches"));
        QCOMPARE(b.plan(sess).value("totalMin").toInt(), 30);

        // Un exercice utilisé ne se supprime pas : on l'archive ; il reste lisible dans le plan.
        QVERIFY(!b.deleteExercise(ex1).value("ok").toBool());
        QVERIFY(b.setExerciseArchived(ex1, true).value("ok").toBool());
        QCOMPARE(b.exercises(false).size(), base.size() + 1);
        QCOMPARE(b.plan(sess).value("entries").toList().size(), 2);
        QVERIFY(b.removePlanItem(i1).value("ok").toBool());
        QVERIFY(b.deleteExercise(ex1).value("ok").toBool());

        // Séance annulée : pas d'ajout au plan ; séance inconnue : erreur.
        QVERIFY(b.setSessionStatus(sess, "cancelled", false).value("ok").toBool());
        QVERIFY(!b.addToPlan(sess, ex2, 10).value("ok").toBool());
        QVERIFY(!b.plan(9999).value("ok").toBool());
    }

    void exportImportRoundTrip()
    {
        // Appareil A : une équipe, des joueurs, une séance avec plan et appel, un schéma.
        Backend a(m_db.handle());
        const int team = a.saveTeam({{"name", "U15"}, {"category", "U15"}}).value("id").toInt();
        const int p1 = a.savePlayer({{"teamId", team}, {"firstName", "Ali"}, {"lastName", "Alpha"},
                                     {"jerseyNumber", 9}, {"birthDate", "2011-03-04"}}).value("id").toInt();
        a.savePlayer({{"teamId", team}, {"firstName", "Bob"}, {"lastName", "Bravo"}, {"jerseyNumber", 4}});
        const int sess = a.saveSession({{"teamId", team}, {"date", "2026-11-03"}, {"time", "18:30"},
                                        {"durationMin", 90}, {"theme", "Finition"}}, false).value("id").toInt();
        const int ex = a.saveExercise({{"title", "Mon exercice"}, {"category", "game"}, {"durationMin", 25},
                                       {"equipment", "Ballons"}}).value("id").toInt();
        QVERIFY(a.addToPlan(sess, ex, 0).value("ok").toBool());
        QVERIFY(a.saveAttendance(sess, {QVariantMap{{"playerId", p1}, {"status", "present"}, {"note", "ok"}}}, true)
                    .value("ok").toBool());
        const int board = a.createBoard("Pressing");
        QVERIFY(board > 0);
        const int nExercises = a.exercises(true).size();

        const QString file = m_dir.filePath(QStringLiteral("export.footcoach"));
        QVERIFY(a.exportDataFile(file).value("ok").toBool());
        QVERIFY(a.suggestedExportName().endsWith(QStringLiteral(".footcoach")));

        // Appareil B : base neuve avec ses propres données (qui seront remplacées).
        Database other(QStringLiteral("footcoach-b"));
        QString error;
        QVERIFY2(other.open(m_dir.filePath(QStringLiteral("b.db")), &error), qPrintable(error));
        auto cleanupOther = qScopeGuard([&] { other.close(); });      // exécuté après la destruction de b
        Backend b(other.handle());
        b.saveTeam({{"name", "Autre équipe"}});
        b.createBoard("À remplacer");
        QSignalSpy replaced(&b, &Backend::dataReplaced);

        const QVariantMap info = b.inspectDataFile(file);
        QVERIFY2(info.value("ok").toBool(), qPrintable(info.value("error").toString()));
        QCOMPARE(info.value("counts").toMap().value("players").toInt(), 2);
        QCOMPARE(info.value("counts").toMap().value("sessions").toInt(), 1);
        QCOMPARE(info.value("current").toMap().value("teams").toInt(), 1);
        QCOMPARE(b.teams().size(), 1);                    // inspecter ne modifie rien

        const QVariantMap r = b.importDataFile(QUrl::fromLocalFile(file).toString());
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(replaced.count(), 1);
        QVERIFY(!r.value("backupName").toString().isEmpty());

        const QVariantList teams = b.teams();
        QCOMPARE(teams.size(), 1);
        QCOMPARE(teams.at(0).toMap().value("name").toString(), QStringLiteral("U15"));
        const QVariantList players = b.players(team, true);
        QCOMPARE(players.size(), 2);
        QCOMPARE(b.boards().size(), 1);
        QCOMPARE(b.boards().at(0).toMap().value("title").toString(), QStringLiteral("Pressing"));
        QCOMPARE(b.exercises(true).size(), nExercises);
        const QVariantMap plan = b.plan(sess);
        QVERIFY(plan.value("ok").toBool());
        QCOMPARE(plan.value("entries").toList().size(), 1);
        const QVariantMap sheet = b.attendanceSheet(sess);
        QVERIFY2(sheet.value("ok").toBool(), qPrintable(sheet.value("error").toString()));

        // Fichier abîmé, mauvais format, version plus récente : refusés, données intactes.
        const QString bad = m_dir.filePath(QStringLiteral("bad.footcoach"));
        auto write = [&](const QByteArray &bytes) { QFile f(bad); QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate)); f.write(bytes); };
        write("pas du json");
        QVERIFY(!b.importDataFile(bad).value("ok").toBool());
        write("{\"format\":\"autre\"}");
        QVERIFY(!b.importDataFile(bad).value("ok").toBool());
        write("{\"format\":\"footcoach-data\",\"version\":1,\"schema\":999,\"tables\":{}}");
        QVERIFY(!b.importDataFile(bad).value("ok").toBool());
        // Contenu incohérent (joueur d'une équipe inexistante) : la transaction est annulée.
        QFile src(file);
        QVERIFY(src.open(QIODevice::ReadOnly));
        QJsonObject root = QJsonDocument::fromJson(src.readAll()).object();
        QJsonObject tables = root.value("tables").toObject();
        QJsonObject pl = tables.value("player").toObject();
        QJsonArray rows = pl.value("rows").toArray();
        QJsonArray row0 = rows.at(0).toArray();
        row0[1] = 424242;                                  // team_id
        rows[0] = row0;
        pl["rows"] = rows;
        tables["player"] = pl;
        root["tables"] = tables;
        write(QJsonDocument(root).toJson(QJsonDocument::Compact));
        const QVariantMap refused = b.importDataFile(bad);
        QVERIFY(!refused.value("ok").toBool());
        QCOMPARE(b.players(team, true).size(), 2);         // rien n'a changé
        QCOMPARE(b.teams().size(), 1);
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
