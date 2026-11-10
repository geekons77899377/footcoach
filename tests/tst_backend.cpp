#include "app/Backend.h"
#include "core/Database.h"
#include "core/TacticalBoard.h"

#include <QBuffer>
#include <QImage>
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
        // Les exemples U10-U11 (migration 008) ont leur propre test : ici on part d'une base sans schéma tactique.
        QSqlQuery(m_db.handle()).exec(QStringLiteral("DELETE FROM tactical_board WHERE exercise_id IN (SELECT id FROM exercise WHERE category_id = (SELECT id FROM exercise_category WHERE name = 'U10-U11'))"));

    }

    void cleanup()
    {
        m_db.close();
        QFile::remove(m_dir.filePath(QStringLiteral("t.db")));
    }

    void exerciseBoardIsCreatedOnDemandAndReadThroughTheBackend()
    {
        Backend b(m_db.handle());
        const QVariantMap saved = b.saveExercise({{"title", "Rondo test"}, {"category", "technical"}});
        QVERIFY(saved.value("ok").toBool());
        const int ex = saved.value("id").toInt();
        QCOMPARE(b.exerciseBoard(ex).value("boardId").toInt(), 0);               // aucun terrain au départ

        const QVariantMap created = b.createExerciseBoard(ex);
        QVERIFY(created.value("ok").toBool());
        QVERIFY(created.value("created").toBool());
        const int boardId = created.value("boardId").toInt();
        QVERIFY(boardId > 0);

        const QVariantMap again = b.createExerciseBoard(ex);                      // pas de deuxième schéma de référence
        QVERIFY(again.value("ok").toBool());
        QVERIFY(!again.value("created").toBool());
        QCOMPARE(again.value("boardId").toInt(), boardId);

        const QVariantMap read = b.exerciseBoard(ex);
        QCOMPARE(read.value("boardId").toInt(), boardId);
        QCOMPARE(read.value("title").toString(), QStringLiteral("Rondo test"));
        QVERIFY(read.value("readable").toBool());

        QVERIFY(!b.createExerciseBoard(424242).value("ok").toBool());            // exercice inconnu
        QCOMPARE(b.exerciseBoard(424242).value("boardId").toInt(), 0);
    }

    void seededU10U11ExerciseHasItsBoardThroughTheBackend()
    {
        Database fresh;
        QString error;
        QVERIFY2(fresh.open(QStringLiteral(":memory:"), &error), qPrintable(error));
        Backend b(fresh.handle());
        int found = 0;
        for (const QVariant &v : b.exercises(false)) {
            const QVariantMap e = v.toMap();
            if (e.value("categoryName").toString() != QStringLiteral("U10-U11"))
                continue;
            ++found;
            const QVariantMap board = b.exerciseBoard(e.value("id").toInt());
            QVERIFY2(board.value("boardId").toInt() > 0, qPrintable(e.value("title").toString()));
            QVERIFY(board.value("readable").toBool());
            QVERIFY(board.value("json").toString().contains(QStringLiteral("\"items\"")));
        }
        QCOMPARE(found, 27);
    }

    // Migration 009 : « U10-U11 · » quitte les titres et devient une catégorie.
    void migration009MovesU10U11FromTitlesToACategory()
    {
        Database fresh;
        QString error;
        QVERIFY2(fresh.open(QStringLiteral(":memory:"), &error), qPrintable(error));
        Backend b(fresh.handle());
        const QVariantList cats = b.categories();
        QCOMPARE(cats.size(), 1);
        QCOMPARE(cats.at(0).toMap().value("name").toString(), QStringLiteral("U10-U11"));
        QCOMPARE(cats.at(0).toMap().value("count").toInt(), 27);
        int inCategory = 0;
        for (const QVariant &v : b.exercises(true)) {
            const QVariantMap e = v.toMap();
            QVERIFY2(!e.value("title").toString().contains(QStringLiteral("U10-U11")), qPrintable(e.value("title").toString()));
            if (e.value("categoryId").toInt() > 0)
                ++inCategory;
        }
        QCOMPARE(inCategory, 27);
        QSqlQuery q(fresh.handle());
        QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM tactical_board WHERE title LIKE '%U10-U11%'")) && q.next());
        QCOMPARE(q.value(0).toInt(), 0);
    }

    void categoriesCanBeCreatedRenamedAssignedAndDeleted()
    {
        Backend b(m_db.handle());
        const int seeded = b.categories().size();                 // U10-U11 (migration 009)

        QVERIFY(!b.addCategory(QStringLiteral("   ")).value("ok").toBool());
        QVERIFY(!b.addCategory(QString(41, QLatin1Char('x'))).value("ok").toBool());
        QVERIFY(!b.addCategory(QStringLiteral("u10-u11")).value("ok").toBool());          // unique sans tenir compte de la casse
        const QVariantMap added = b.addCategory(QStringLiteral("  U12   U13 "));
        QVERIFY(added.value("ok").toBool());
        const int cat = added.value("id").toInt();
        QCOMPARE(b.categories().size(), seeded + 1);
        QVERIFY(!b.addCategory(QStringLiteral("U12 U13")).value("ok").toBool());           // espaces normalisés

        const QVariantMap saved = b.saveExercise({{"title", "Conduite"}, {"category", "technical"}, {"categoryId", cat}});
        QVERIFY2(saved.value("ok").toBool(), qPrintable(saved.value("error").toString()));
        QVariantMap mine;
        for (const QVariant &v : b.exercises(false))
            if (v.toMap().value("id").toInt() == saved.value("id").toInt())
                mine = v.toMap();
        QCOMPARE(mine.value("categoryId").toInt(), cat);
        QCOMPARE(mine.value("categoryName").toString(), QStringLiteral("U12 U13"));
        QCOMPARE(mine.value("category").toString(), QStringLiteral("technical"));          // l'activité est indépendante

        QVERIFY(b.renameCategory(cat, QStringLiteral("U12-U13")).value("ok").toBool());
        QVERIFY(!b.renameCategory(cat, QStringLiteral("U10-U11")).value("ok").toBool());   // déjà prise
        QVERIFY(!b.renameCategory(987654, QStringLiteral("Autre")).value("ok").toBool());

        // Supprimer la catégorie garde l'exercice, sans catégorie.
        QVERIFY(b.deleteCategory(cat).value("ok").toBool());
        QVERIFY(!b.deleteCategory(cat).value("ok").toBool());
        for (const QVariant &v : b.exercises(false))
            if (v.toMap().value("id").toInt() == saved.value("id").toInt()) {
                QCOMPARE(v.toMap().value("categoryId").toInt(), 0);
                QCOMPARE(v.toMap().value("categoryName").toString(), QString());
            }
        // Un exercice sans categoryId reste sans catégorie.
        const QVariantMap plain = b.saveExercise({{"title", "Sans catégorie"}, {"category", "warmup"}});
        QVERIFY(plain.value("ok").toBool());
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

    // Étape 2 : terrain propre à un bloc, créé à la demande et relu par plan() (contrat utilisé par PlanEditor.qml).
    void blockBoardThroughTheBackend()
    {
        const QString dbFile = m_dir.filePath(QStringLiteral("t.db"));     // celle ouverte par init()
        int team = 0, sess = 0, i1 = 0, i2 = 0, boardId = 0;
        const QString json = QStringLiteral(
            "{\"version\":1,\"pitch\":\"half\",\"items\":["
            "{\"kind\":\"player\",\"x\":0.25,\"y\":0.5,\"label\":\"9\"}],\"arrows\":[]}");
        {
            Database &db = m_db;
            Backend b(db.handle());
            team = b.saveTeam({{"name", "U15"}}).value("id").toInt();
            sess = b.saveSession({{"teamId", team}, {"date", "2026-11-03"}, {"time", "18:30"},
                                  {"durationMin", 60}}, false).value("id").toInt();
            const int ex1 = b.saveExercise({{"title", "Rondo 5 contre 2"}, {"category", "technical"}}).value("id").toInt();
            const int ex2 = b.saveExercise({{"title", "Jeu réduit"}, {"category", "game"}}).value("id").toInt();
            i1 = b.addToPlan(sess, ex1, 20).value("id").toInt();
            i2 = b.addToPlan(sess, ex2, 20).value("id").toInt();

            // Bloc sans terrain : rien n'est créé tout seul (règle B).
            QVariantList rows = b.plan(sess).value("entries").toList();
            QCOMPARE(rows.at(0).toMap().value("boardId").toInt(), -1);
            QVERIFY(!rows.at(0).toMap().value("boardReadable").toBool());
            QVERIFY(rows.at(0).toMap().value("boardJson").toString().isEmpty());
            QVERIFY(b.boards().isEmpty());

            // Création à la demande : terrain vide au nom de l'exercice, idempotente.
            const QVariantMap made = b.createBlockBoard(i1);
            QVERIFY2(made.value("ok").toBool(), qPrintable(made.value("error").toString()));
            QVERIFY(made.value("created").toBool());
            boardId = made.value("boardId").toInt();
            QVERIFY(boardId > 0);
            const QVariantMap again = b.createBlockBoard(i1);
            QVERIFY(again.value("ok").toBool());
            QVERIFY(!again.value("created").toBool());
            QCOMPARE(again.value("boardId").toInt(), boardId);
            QCOMPARE(b.board(boardId).value("title").toString(), QStringLiteral("Rondo 5 contre 2"));
            QVERIFY(b.board(boardId).value("readable").toBool());
            QVERIFY(b.boards().isEmpty());                          // le terrain d'un bloc n'est pas dans la bibliothèque

            rows = b.plan(sess).value("entries").toList();
            QCOMPARE(rows.at(0).toMap().value("boardId").toInt(), boardId);
            QVERIFY(rows.at(0).toMap().value("boardReadable").toBool());
            QCOMPARE(rows.at(1).toMap().value("boardId").toInt(), -1);   // l'autre bloc n'a rien

            // Modification puis relecture par plan().
            QVERIFY2(b.saveBoard(boardId, json), qPrintable(b.lastError()));
            Tactics::Board back;
            QVERIFY(Tactics::fromJson(b.plan(sess).value("entries").toList().at(0).toMap().value("boardJson").toString(), &back));
            QCOMPARE(back.pitch, Tactics::Pitch::Half);
            QCOMPARE(back.items.size(), 1);

            // Blocs inconnus ou séance annulée : refusé, sans rien créer.
            QVERIFY(!b.createBlockBoard(424242).value("ok").toBool());
            QVERIFY(!b.lastError().isEmpty());
        }
        {
            // Réouverture de la base : le terrain est retrouvé tel quel.
            Database &db = m_db;
            db.close();
            QString error;
            QVERIFY2(db.open(dbFile, &error), qPrintable(error));
            Backend b(db.handle());
            const QVariantMap row = b.plan(sess).value("entries").toList().at(0).toMap();
            QCOMPARE(row.value("boardId").toInt(), boardId);
            Tactics::Board back;
            QVERIFY(Tactics::fromJson(row.value("boardJson").toString(), &back));
            QCOMPARE(back.items.size(), 1);
            QCOMPARE(back.items.first().label, QStringLiteral("9"));

            // Contenu illisible : signalé, jamais réécrit.
            QSqlQuery q(db.handle());
            QVERIFY(q.exec(QStringLiteral("UPDATE tactical_board SET scene_json = '{pas du json' WHERE id = %1").arg(boardId)));
            const QVariantMap broken = b.plan(sess).value("entries").toList().at(0).toMap();
            QCOMPARE(broken.value("boardId").toInt(), boardId);
            QVERIFY(!broken.value("boardReadable").toBool());
            QVERIFY(broken.value("boardJson").toString().isEmpty());
            QVERIFY(!b.saveBoard(boardId, json));
            QVERIFY(q.exec(QStringLiteral("SELECT scene_json FROM tactical_board WHERE id = %1").arg(boardId)) && q.next());
            QCOMPARE(q.value(0).toString(), QStringLiteral("{pas du json"));

            // Retrait du terrain : le bloc reste, sans terrain ; supprimer le bloc supprime son terrain.
            QVERIFY(b.deleteBoard(boardId));
            QCOMPARE(b.plan(sess).value("entries").toList().size(), 2);
            QCOMPARE(b.plan(sess).value("entries").toList().at(0).toMap().value("boardId").toInt(), -1);
            const int second = b.createBlockBoard(i2).value("boardId").toInt();
            QVERIFY(second > 0);
            QVERIFY(b.removePlanItem(i2).value("ok").toBool());
            QVERIFY(b.board(second).isEmpty());

            // Séance annulée : pas de création.
            QVERIFY(b.setSessionStatus(sess, "cancelled", false).value("ok").toBool());
            QVERIFY(!b.createBlockBoard(i1).value("ok").toBool());
            QVERIFY(b.plan(sess).value("entries").toList().at(0).toMap().value("boardId").toInt() == -1);
        }
    }

    // Licence : facultative, lettres et chiffres, 20 au plus, unique dans l'équipe sans tenir compte de la casse.
    void playerLicenceValidationAndUniqueness()
    {
        Backend b(m_db.handle());
        const int team = b.saveTeam({{"name", "U15"}}).value("id").toInt();
        const int other = b.saveTeam({{"name", "U17"}}).value("id").toInt();
        auto save = [&](int t, const QString &first, const QString &licence, int id = -1) {
            return b.savePlayer({{"id", id}, {"teamId", t}, {"firstName", first}, {"licence", licence}});
        };
        const QVariantMap a = save(team, "Ana", " ab 123 ");
        QVERIFY2(a.value("ok").toBool(), qPrintable(a.value("error").toString()));
        QCOMPARE(b.players(team, true).first().toMap().value("licence").toString(), QStringLiteral("AB123"));   // normalisée
        QVERIFY(save(team, "Bob", "").value("ok").toBool());                                   // facultative
        QVERIFY(save(team, "Cléo", "").value("ok").toBool());                                  // plusieurs joueurs sans licence
        const QVariantMap dup = save(team, "Dan", "ab123");
        QVERIFY(!dup.value("ok").toBool());
        QVERIFY2(dup.value("error").toString().contains(QStringLiteral("déjà utilisé")), qPrintable(dup.value("error").toString()));
        QVERIFY(save(other, "Dan", "AB123").value("ok").toBool());                             // autre équipe : permis
        QVERIFY(!save(team, "Eve", "AB-123").value("ok").toBool());                            // caractère interdit
        QVERIFY(!save(team, "Eve", "123456789012345678901").value("ok").toBool());             // 21 caractères
        QVERIFY(save(team, "Eve", "123456789012345678AB").value("ok").toBool());               // 20 caractères
        // Modifier un joueur sans changer sa licence reste possible ; lui donner celle d'un autre est refusé, même inactif.
        const int ana = a.value("id").toInt();
        QVERIFY(save(team, "Ana", "AB123", ana).value("ok").toBool());
        const int bob = b.players(team, true).at(1).toMap().value("id").toInt();
        QVERIFY(b.setPlayerActive(ana, false).value("ok").toBool());
        QVERIFY(!save(team, "Bob", "ab123", bob).value("ok").toBool());
        // Retirer la licence libère le numéro.
        QVERIFY(save(team, "Ana", "", ana).value("ok").toBool());
        QVERIFY(save(team, "Bob", "ab123", bob).value("ok").toBool());
    }

    // Photo : réduite à 480 px en JPEG, stockée dans la base, hors des listes ; retirée ou conservée selon les clés.
    void playerPhotoStoredReducedAndOptional()
    {
        Backend b(m_db.handle());
        const int team = b.saveTeam({{"name", "U15"}}).value("id").toInt();
        QImage big(1600, 900, QImage::Format_ARGB32);
        big.fill(QColor(30, 160, 90, 128));                         // translucide : doit devenir un JPEG opaque
        const QString src = m_dir.filePath(QStringLiteral("grande.png"));
        QVERIFY(big.save(src));
        const QVariantMap prep = b.preparePhoto(QUrl::fromLocalFile(src).toString());
        QVERIFY2(prep.value("ok").toBool(), qPrintable(prep.value("error").toString()));
        const QString data = prep.value("data").toString();
        QVERIFY(data.startsWith(QStringLiteral("data:image/jpeg;base64,")));
        QImage reduced;
        QVERIFY(reduced.loadFromData(QByteArray::fromBase64(data.mid(23).toLatin1()), "JPEG"));
        QCOMPARE(qMax(reduced.width(), reduced.height()), 480);
        QCOMPARE(reduced.width() * 9, reduced.height() * 16);        // proportions conservées (480 × 270)
        // Les chemins sans « file:// » (content://, chemin brut) passent aussi par QFile.
        QVERIFY(b.preparePhoto(src).value("ok").toBool());

        const QVariantMap saved = b.savePlayer({{"teamId", team}, {"firstName", "Ana"}, {"photo", data}});
        QVERIFY2(saved.value("ok").toBool(), qPrintable(saved.value("error").toString()));
        const int id = saved.value("id").toInt();
        QVERIFY(b.players(team, true).first().toMap().value("hasPhoto").toBool());
        QCOMPARE(b.playerPhoto(id), data);
        // Enregistrer sans les clés photo ne touche pas à l'image.
        QVERIFY(b.savePlayer({{"id", id}, {"teamId", team}, {"firstName", "Ana"}, {"notes", "x"}}).value("ok").toBool());
        QCOMPARE(b.playerPhoto(id), data);
        // Un joueur sans photo : chaîne vide, hasPhoto faux.
        const int bob = b.savePlayer({{"teamId", team}, {"firstName", "Bob"}}).value("id").toInt();
        QVERIFY(b.playerPhoto(bob).isEmpty());
        QVERIFY(!b.players(team, true).at(1).toMap().value("hasPhoto").toBool());
        // Retrait.
        QVERIFY(b.savePlayer({{"id", id}, {"teamId", team}, {"firstName", "Ana"}, {"removePhoto", true}}).value("ok").toBool());
        QVERIFY(b.playerPhoto(id).isEmpty());
        QVERIFY(!b.players(team, true).first().toMap().value("hasPhoto").toBool());
    }

    // Import en lot : « <licence>.jpg » → joueur de l'équipe ; remplace l'existant ; signale inconnus, doublons, fichiers illisibles.
    void importPhotosByLicence()
    {
        Backend b(m_db.handle());
        const int team = b.saveTeam({{"name", "U15"}}).value("id").toInt();
        const int other = b.saveTeam({{"name", "U17"}}).value("id").toInt();
        auto mk = [&](int t, const QString &first, const QString &lic) {
            return b.savePlayer({{"teamId", t}, {"firstName", first}, {"licence", lic}}).value("id").toInt();
        };
        const int ana = mk(team, "Ana", "9603907336");
        const int bob = mk(team, "Bob", "9605678531");
        const int dan = mk(other, "Dan", "9999999999");
        mk(team, "Eve", "");                                         // sans licence : jamais ciblée
        auto png = [&](const QString &name, const QColor &c) {
            QImage img(60, 80, QImage::Format_RGB32);
            img.fill(c);
            const QString path = m_dir.filePath(name);
            img.save(path);
            return QUrl::fromLocalFile(path).toString();
        };
        const QString bobOld = png("old.png", Qt::red);
        QVERIFY(b.savePlayer({{"id", bob}, {"teamId", team}, {"firstName", "Bob"}, {"licence", "9605678531"},
                              {"photo", b.preparePhoto(bobOld).value("data")}}).value("ok").toBool());
        const QString bobBefore = b.playerPhoto(bob);
        QDir(m_dir.path()).mkdir(QStringLiteral("lot"));
        QFile bad(m_dir.filePath(QStringLiteral("lot/9603885403.jpg")));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("pas une image");
        bad.close();
        const QVariantList urls = {
            png("lot/9603907336.png", Qt::blue),                    // Ana : ajoutée
            png("lot/9605678531.PNG", Qt::green),                   // Bob : remplacée (extension en majuscules)
            png("lot/9999999999.png", Qt::yellow),                  // licence d'une autre équipe : inconnue ici
            png("lot/0000000000.png", Qt::black),                   // inconnue
            png("lot/9603907336 (1).png", Qt::white),               // nom non conforme : inconnue
            QUrl::fromLocalFile(m_dir.filePath(QStringLiteral("lot/9603885403.jpg"))).toString(),   // illisible mais licence inconnue
        };
        const QVariantMap r = b.importPhotos(team, urls);
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(r.value("added").toInt(), 1);
        QCOMPARE(r.value("replaced").toInt(), 1);
        QCOMPARE(r.value("failed").toInt(), 4);
        QVERIFY(!b.playerPhoto(ana).isEmpty());
        QVERIFY(b.playerPhoto(bob) != bobBefore);
        QVERIFY(b.playerPhoto(dan).isEmpty());                      // l'autre équipe n'est pas touchée
        const QVariantList items = r.value("items").toList();
        QCOMPARE(items.size(), 6);
        QCOMPARE(items.at(0).toMap().value("status").toString(), QStringLiteral("added"));
        QCOMPARE(items.at(1).toMap().value("status").toString(), QStringLiteral("replaced"));
        QCOMPARE(items.at(1).toMap().value("player").toString(), QStringLiteral("Bob"));
        QCOMPARE(items.at(2).toMap().value("status").toString(), QStringLiteral("unknown"));
        // Fichier illisible pour une licence connue : erreur propre, la photo existante reste intacte.
        const int cleo = mk(team, "Cléo", "9603885403");
        const QVariantMap r2 = b.importPhotos(team, {QUrl::fromLocalFile(m_dir.filePath(QStringLiteral("lot/9603885403.jpg"))).toString()});
        QCOMPARE(r2.value("failed").toInt(), 1);
        QCOMPARE(r2.value("items").toList().first().toMap().value("status").toString(), QStringLiteral("error"));
        QVERIFY(b.playerPhoto(cleo).isEmpty());
        // Deux fichiers pour la même licence : le premier gagne, le second est signalé.
        const QVariantMap r3 = b.importPhotos(team, {png("lot/9605678531.png", Qt::cyan), png("lot/9605678531.jpeg", Qt::magenta)});
        QCOMPARE(r3.value("replaced").toInt(), 1);
        QCOMPARE(r3.value("failed").toInt(), 1);
        // Cas limites.
        QVERIFY(!b.importPhotos(team, {}).value("ok").toBool());
        QVERIFY(!b.importPhotos(9999, urls).value("ok").toBool());
    }

    void playerPhotoRefusesBadInput()
    {
        Backend b(m_db.handle());
        const int team = b.saveTeam({{"name", "U15"}}).value("id").toInt();
        const QString txt = m_dir.filePath(QStringLiteral("faux.png"));
        QFile f(txt);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("ceci n'est pas une image");
        f.close();
        QVERIFY(!b.preparePhoto(txt).value("ok").toBool());
        QVERIFY(!b.preparePhoto(QUrl::fromLocalFile(m_dir.filePath(QStringLiteral("absent.png"))).toString()).value("ok").toBool());
        QVERIFY(!b.preparePhoto(QString()).value("ok").toBool());
        // Le backend ne fait pas confiance à l'interface : une « photo » qui n'est pas un JPEG est refusée, rien n'est créé.
        for (const QString &bad : {QStringLiteral("data:image/jpeg;base64,AAAA"), QStringLiteral("data:text/html;base64,PGI+"),
                                   QStringLiteral("n'importe quoi")}) {
            const QVariantMap r = b.savePlayer({{"teamId", team}, {"firstName", "Ana"}, {"photo", bad}});
            QVERIFY2(!r.value("ok").toBool(), qPrintable(bad));
        }
        QCOMPARE(b.players(team, true).size(), 0);
    }

    void playerNameOptionalAndUnique()
    {
        Backend b(m_db.handle());
        const int team = b.saveTeam({{"name", "U15"}}).value("id").toInt();
        const int other = b.saveTeam({{"name", "U17"}}).value("id").toInt();
        auto save = [&](int t, const QString &first, const QString &last, int id = -1) {
            return b.savePlayer({{"id", id}, {"teamId", t}, {"firstName", first}, {"lastName", last}});
        };
        // Le prénom seul suffit ; le prénom reste obligatoire.
        const QVariantMap lucas = save(team, "Lucas", "");
        QVERIFY2(lucas.value("ok").toBool(), qPrintable(lucas.value("error").toString()));
        QVERIFY(!save(team, "", "Martin").value("ok").toBool());
        // Doublon : mêmes prénom et nom sans tenir compte des majuscules, accents ni espaces superflus.
        QVERIFY(!save(team, "  LUCAS ", "").value("ok").toBool());
        QVERIFY(save(team, "Lucas", "Martin").value("ok").toBool());       // un nom le distingue
        QVERIFY(!save(team, "lucas", "MARTIN").value("ok").toBool());
        const int leo = save(team, "Léo", "").value("id").toInt();
        QVERIFY(!save(team, "Leo", "").value("ok").toBool());               // accent ignoré
        QVERIFY(save(other, "Lucas", "").value("ok").toBool());             // autre équipe : permis
        // Modifier un joueur sans changer son nom reste possible ; lui donner le nom d'un autre est refusé.
        QVERIFY(save(team, "Léo", "", leo).value("ok").toBool());
        QVERIFY(!save(team, "Lucas", "", leo).value("ok").toBool());
        // Un joueur inactif compte aussi (réactivation sans doublon).
        QVERIFY(b.setPlayerActive(leo, false).value("ok").toBool());
        QVERIFY(!save(team, "Léo", "").value("ok").toBool());
    }

    void importPlayersList()
    {
        Backend b(m_db.handle());
        const int team = b.saveTeam({{"name", "U15"}}).value("id").toInt();
        const int other = b.saveTeam({{"name", "U17"}}).value("id").toInt();
        QVERIFY(b.savePlayer({{"teamId", team}, {"firstName", "Ali"}, {"lastName", "Alpha"}, {"jerseyNumber", 9}})
                    .value("ok").toBool());

        // Liste de prénoms seuls (lignes vides et fins de ligne Windows tolérées).
        QVariantMap r = b.importPlayers(team, QStringLiteral("Lucas\r\n\r\n  Théo  \nHugo\n"));
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(r.value("created").toInt(), 3);
        QCOMPARE(b.players(team, true).size(), 4);
        const QVariantList all = b.players(team, true);
        bool theo = false;
        for (const QVariant &v : all)
            theo = theo || v.toMap().value("firstName").toString() == QStringLiteral("Théo");
        QVERIFY(theo);

        // Champs facultatifs, en-tête de tableur, séparateurs « ; » et tabulation, poste et date de plusieurs formes.
        r = b.importPlayers(team, QStringLiteral("Prénom;Nom;Numéro;Poste;Naissance\n"
                                                 "Noah;Durand;7;Milieu;04/05/2012\n"
                                                 "Eli\tBlanc\t11\tgk\t2011-02-03\n"
                                                 "Sam;;;Attaquant\n"));
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(r.value("created").toInt(), 3);
        QVariantMap noah;
        for (const QVariant &v : b.players(team, true))
            if (v.toMap().value("firstName").toString() == QLatin1String("Noah"))
                noah = v.toMap();
        QCOMPARE(noah.value("lastName").toString(), QStringLiteral("Durand"));
        QCOMPARE(noah.value("jerseyNumber").toInt(), 7);
        QCOMPARE(noah.value("position").toString(), QStringLiteral("MID"));
        QCOMPARE(noah.value("birthDate").toString(), QStringLiteral("2012-05-04"));

        // Tout ou rien : une ligne refusée, rien n'est créé ; chaque erreur porte son numéro de ligne.
        const int before = b.players(team, true).size();
        r = b.importPlayers(team, QStringLiteral("Zoé\n"            // 1 : valide
                                                 "lucas\n"          // 2 : déjà dans l'effectif (casse ignorée)
                                                 "Max;;9\n"         // 3 : numéro 9 pris par Ali
                                                 "Zoé\n"            // 4 : doublon dans la liste
                                                 ";Nom\n"           // 5 : prénom manquant
                                                 "Tom;;100\n"       // 6 : numéro invalide
                                                 "Ian;;;Libero\n"   // 7 : poste inconnu
                                                 "Eva;;;;31/02/2012\n"   // 8 : date invalide
                                                 "A;B;1;GK;;x\n"));      // 9 : trop de colonnes
        QVERIFY(!r.value("ok").toBool());
        QCOMPARE(r.value("created").toInt(), 0);
        QCOMPARE(b.players(team, true).size(), before);
        QList<int> lines;
        for (const QVariant &e : r.value("errors").toList())
            lines << e.toMap().value("line").toInt();
        QCOMPARE(lines, (QList<int>{2, 3, 4, 5, 6, 7, 8, 9}));
        QVERIFY(r.value("error").toString().contains(QStringLiteral("Rien n'a été importé")));

        // Numéro répété dans la liste ; même prénom permis dans une autre équipe ; équipe inconnue ; liste vide.
        QVERIFY(!b.importPlayers(team, QStringLiteral("P1;;20\nP2;;20")).value("ok").toBool());
        QVERIFY(b.importPlayers(other, QStringLiteral("Lucas;;9")).value("ok").toBool());
        QVERIFY(!b.importPlayers(9999, QStringLiteral("X")).value("ok").toBool());
        QVERIFY(!b.importPlayers(team, QStringLiteral("  \n \n")).value("ok").toBool());
        QVERIFY(!b.importPlayers(team, QStringLiteral("Prénom;Nom")).value("ok").toBool());   // en-tête seul
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
