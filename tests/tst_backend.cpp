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
};

QTEST_GUILESS_MAIN(BackendTest)
#include "tst_backend.moc"
