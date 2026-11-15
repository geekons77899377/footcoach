// Schéma 13 : rubriques de la fiche exercice, exercice « Le parcours des portes » et fiche PDF A4 paysage.
#include "app/Backend.h"
#include "app/ExerciseSheet.h"
#include "core/Database.h"

#include <QBuffer>
#include <QImage>
#include <QPainter>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

class ExerciseSheetTest : public QObject
{
    Q_OBJECT

    Database m_db;

    static QString pdfText(const QString &file)
    {
        const QString tool = QStandardPaths::findExecutable(QStringLiteral("pdftotext"));
        if (tool.isEmpty())
            return QString();
        QProcess proc;
        proc.start(tool, {QStringLiteral("-layout"), file, QStringLiteral("-")});
        proc.waitForFinished(15000);
        return QString::fromUtf8(proc.readAllStandardOutput());
    }

    QVariantMap exercise(Backend &b, const QString &title)
    {
        for (const QVariant &v : b.exercises(true))
            if (v.toMap().value("title").toString() == title)
                return v.toMap();
        return {};
    }

private slots:
    void init()
    {
        QString error;
        QVERIFY2(m_db.open(QStringLiteral(":memory:"), &error), qPrintable(error));
    }
    void cleanup() { m_db.close(); }

    void splitLinesDropsBulletsAndBlankLines()
    {
        QCOMPARE(ExerciseSheet::splitLines(QStringLiteral("- Un\n\n  * Deux  \n• Trois\n3) Quatre\n2. Cinq\nsix")),
                 (QStringList{"Un", "Deux", "Trois", "Quatre", "Cinq", "six"}));
        QCOMPARE(ExerciseSheet::splitLines(QString()), QStringList());
        QCOMPARE(ExerciseSheet::fileName(QStringLiteral("Le parcours des portes !")), QStringLiteral("fiche-le-parcours-des-portes.pdf"));
        QCOMPARE(ExerciseSheet::fileName(QString()), QStringLiteral("fiche-exercice.pdf"));
    }

    void parcoursDesPortesIsSeededWithItsSheetAndBoard()
    {
        Backend b(m_db.handle());
        const QVariantMap e = exercise(b, QStringLiteral("Le parcours des portes"));
        QVERIFY(!e.isEmpty());
        QCOMPARE(e.value("categoryName").toString(), QStringLiteral("U10-U11"));
        QCOMPARE(e.value("durationMin").toInt(), 17);
        QCOMPARE(e.value("playersMin").toInt(), 8);
        QCOMPARE(e.value("playersMax").toInt(), 12);
        QCOMPARE(ExerciseSheet::splitLines(e.value("instructions").toString()).size(), 5);
        QCOMPARE(ExerciseSheet::splitLines(e.value("variations").toString()).size(), 4);
        QCOMPARE(ExerciseSheet::splitLines(e.value("successCriteria").toString()).size(), 4);
        QVERIFY(e.value("coachCue").toString().startsWith(QStringLiteral("Regarde devant toi")));
        const QVariantMap board = b.exerciseBoard(e.value("id").toInt());
        QVERIFY(board.value("readable").toBool());
        Tactics::Board parsed;
        QVERIFY(Tactics::fromJson(board.value("json").toString(), &parsed));
        int cones = 0, players = 0;
        for (const Tactics::Item &it : parsed.items) {
            cones += it.kind == Tactics::Kind::Cone;
            players += it.kind == Tactics::Kind::Player;
        }
        QCOMPARE(cones, 4 + 2 * 11);            // 4 coins + 11 portes de 2 coupelles
        QCOMPARE(players, 2);
        QCOMPARE(parsed.arrows.size(), 4);
    }

    void sectionsAreSavedAndKeptWhenAbsentFromTheMap()
    {
        Backend b(m_db.handle());
        QVariantMap r = b.saveExercise({{"title", "Rondo"}, {"instructions", "Un\nDeux"}, {"variations", "Var"},
                                        {"successCriteria", "Crit"}, {"coachCue", "  Allez !  "}});
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        const int id = r.value("id").toInt();
        QVariantMap e = exercise(b, "Rondo");
        QCOMPARE(e.value("instructions").toString(), QStringLiteral("Un\nDeux"));
        QCOMPARE(e.value("coachCue").toString(), QStringLiteral("Allez !"));
        // Une fiche sans les rubriques (ancien appel) ne les efface pas.
        QVERIFY(b.saveExercise({{"id", id}, {"title", "Rondo 2"}}).value("ok").toBool());
        e = exercise(b, "Rondo 2");
        QCOMPARE(e.value("variations").toString(), QStringLiteral("Var"));
        QCOMPARE(e.value("successCriteria").toString(), QStringLiteral("Crit"));
        // Une rubrique vidée explicitement est effacée.
        QVERIFY(b.saveExercise({{"id", id}, {"title", "Rondo 2"}, {"coachCue", ""}}).value("ok").toBool());
        QCOMPARE(exercise(b, "Rondo 2").value("coachCue").toString(), QString());
    }

    void exportWritesAColourA4LandscapePdf()
    {
        Backend b(m_db.handle());
        const int id = exercise(b, QStringLiteral("Le parcours des portes")).value("id").toInt();
        QCOMPARE(b.exerciseSheetFileName(id), QStringLiteral("fiche-le-parcours-des-portes.pdf"));
        QTemporaryDir dir;
        const QString out = dir.filePath(QStringLiteral("fiche.pdf"));
        const QVariantMap r = b.exportExerciseSheet(id, QUrl::fromLocalFile(out).toString());
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QVERIFY(r.value("hasBoard").toBool());
        QFile f(out);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QVERIFY(f.read(5) == "%PDF-");
        f.close();
        if (qEnvironmentVariableIsSet("FOOTCOACH_SHEET_OUT"))
            QFile::copy(out, qEnvironmentVariable("FOOTCOACH_SHEET_OUT"));
        const QString text = pdfText(out);
        if (text.isEmpty())
            QSKIP("pdftotext absent : contenu du PDF non vérifié");
        for (const char *s : {"Le parcours des portes", "U10", "17 min", "Objectifs", "Organisation", "Consignes",
                              "Évolutions", "Critères de réussite", "Consigne éducateur", "Terrain", "20 m",
                              "8 à 12 joueurs", "1 ballon par joueur", "Départ", "Varier les directions"})
            QVERIFY2(text.contains(QString::fromUtf8(s)), s);

        // Exercice sans schéma ni rubriques : la fiche sort quand même.
        const int bare = b.saveExercise({{"title", "Jeu libre"}}).value("id").toInt();
        const QVariantMap r2 = b.exportExerciseSheet(bare, QUrl::fromLocalFile(dir.filePath("b.pdf")).toString());
        QVERIFY(r2.value("ok").toBool());
        QVERIFY(!r2.value("hasBoard").toBool());
        QVERIFY(pdfText(dir.filePath("b.pdf")).contains(QStringLiteral("Pas encore de schéma")));
        QVERIFY(!b.exportExerciseSheet(99999, QUrl::fromLocalFile(dir.filePath("c.pdf")).toString()).value("ok").toBool());
    }

    // Textes très longs : réduits puis coupés dans leur cadre, sans planter.
    void longTextsStayInsideTheirBoxes()
    {
        ExerciseSheetData d;
        d.title = QString(200, QLatin1Char('T'));
        d.ageCategory = QStringLiteral("Seniors vétérans du dimanche");
        d.durationMin = 120;
        d.objective = QString(400, QLatin1Char('o'));
        for (int i = 0; i < 30; ++i) {
            d.instructions << QStringLiteral("Consigne longue numéro %1 avec beaucoup de mots pour remplir le cadre").arg(i);
            d.variations << QStringLiteral("Évolution %1").arg(i);
        }
        d.coachCue = QString(600, QLatin1Char('c'));
        QImage img(768, 543, QImage::Format_ARGB32);
        QPainter p(&img);
        p.scale(768 / ExerciseSheet::kWidth, 543 / ExerciseSheet::kHeight);
        ExerciseSheet::paint(p, d);
        p.end();
        QByteArray pdf;
        QBuffer buf(&pdf);
        buf.open(QIODevice::WriteOnly);
        QString error;
        QVERIFY(ExerciseSheet::writePdf(&buf, d, &error));
        QVERIFY(pdf.startsWith("%PDF-"));
    }
};

QTEST_MAIN(ExerciseSheetTest)
#include "tst_exercisesheet.moc"
