#include "core/BoardRepository.h"
#include "core/Database.h"
#include "core/ExerciseRepository.h"
#include "core/PlanRepository.h"
#include "core/SessionRepository.h"
#include "core/SessionSheet.h"
#include "core/TacticalBoard.h"
#include "core/TeamRepository.h"

#include <QSqlQuery>
#include <QtTest>

using namespace Tactics;

class BoardTest : public QObject
{
    Q_OBJECT

private slots:
    // --- JSON ---------------------------------------------------------------------------------

    void jsonRoundTrip()
    {
        Board b;
        b.pitch = Pitch::Half;
        b.items = {mk(Kind::Player, QPointF(0.25, 0.5), QStringLiteral("9"), 0, 1.0, QStringLiteral("#fbc02d")),
                   mk(Kind::Opponent, QPointF(0.75, 0.125)),
                   mk(Kind::Ball, QPointF(0.5, 0.5)),
                   mk(Kind::Cone, QPointF(0, 1), QString(), 45, 0.5, QStringLiteral("#212121")),
                   mk(Kind::Goal, QPointF(1, 0.5), QString(), 90, 2.0),
                   mk(Kind::Text, QPointF(0.5, 0.9), QStringLiteral("Jeu à 2 touches\nMax 8 s"), 0, 1.5)};
        b.arrows = {{ArrowStyle::Run, QPointF(0.1, 0.2), QPointF(0.3, 0.4)},
                    {ArrowStyle::Pass, QPointF(0.5, 0.5), QPointF(0.75, 0.25)},
                    {ArrowStyle::Dribble, QPointF(0.5, 0.5), QPointF(0.5, 0.75)},
                    {ArrowStyle::Cross, QPointF(0.9, 0.1), QPointF(0.8, 0.5), 0.25},
                    {ArrowStyle::Cross, QPointF(0.9, 0.9), QPointF(0.8, 0.5), -0.4}};

        Board back;
        QString error;
        QVERIFY2(fromJson(toJson(b), &back, &error), qPrintable(error));
        QVERIFY(back == b);
    }

    void rotationIsNormalizedAndOnlyForGoalsAndCones()
    {
        Board b;
        QVERIFY(fromJson(QStringLiteral(
            "{\"version\":1,\"items\":["
            "{\"kind\":\"goal\",\"x\":0.5,\"y\":0.5,\"rot\":450},"
            "{\"kind\":\"cone\",\"x\":0.5,\"y\":0.5,\"rot\":-45},"
            "{\"kind\":\"player\",\"x\":0.5,\"y\":0.5,\"rot\":90},"
            "{\"kind\":\"goal\",\"x\":0.5,\"y\":0.5}]}"), &b));
        QCOMPARE(b.items.at(0).rotation, 90.0);
        QCOMPARE(b.items.at(1).rotation, 315.0);
        QCOMPARE(b.items.at(2).rotation, 0.0);     // un joueur ne pivote pas
        QCOMPARE(b.items.at(3).rotation, 0.0);     // absent : 0
        QVERIFY(!toJson(Board()).contains(QLatin1String("rot")));
    }

    void scaleAndColorAreValidatedOnRead()
    {
        Board b;
        QVERIFY(fromJson(QStringLiteral(
            "{\"version\":1,\"items\":["
            "{\"kind\":\"goal\",\"x\":0.5,\"y\":0.5,\"scale\":50},"
            "{\"kind\":\"goal\",\"x\":0.5,\"y\":0.5,\"scale\":0.01},"
            "{\"kind\":\"player\",\"x\":0.5,\"y\":0.5,\"scale\":3,\"color\":\"#AABBCC\"},"
            "{\"kind\":\"cone\",\"x\":0.5,\"y\":0.5,\"color\":\"rouge\"},"
            "{\"kind\":\"ball\",\"x\":0.5,\"y\":0.5,\"color\":\"#ff0000\"}]}"), &b));
        QCOMPARE(b.items.at(0).scale, kMaxScale);
        QCOMPARE(b.items.at(1).scale, kMinScale);
        QCOMPARE(b.items.at(2).scale, 1.0);                         // un joueur ne change pas de taille
        QCOMPARE(b.items.at(2).color, QStringLiteral("#aabbcc"));   // normalisée en minuscules
        QVERIFY(b.items.at(3).color.isEmpty());                     // couleur invalide ignorée
        QVERIFY(b.items.at(4).color.isEmpty());                     // un ballon garde sa couleur
        QVERIFY(!colorPalette().isEmpty());
        for (const auto &entry : colorPalette())
            QVERIFY(entry.second.size() == 7 && entry.second.startsWith(QLatin1Char('#')));
    }

    void crossCurveIsValidated()
    {
        Board b;
        QVERIFY(fromJson(QStringLiteral(
            "{\"version\":1,\"arrows\":["
            "{\"style\":\"cross\",\"x1\":0.1,\"y1\":0.1,\"x2\":0.5,\"y2\":0.5,\"curve\":9},"
            "{\"style\":\"cross\",\"x1\":0.1,\"y1\":0.1,\"x2\":0.5,\"y2\":0.5,\"curve\":-9},"
            "{\"style\":\"cross\",\"x1\":0.1,\"y1\":0.1,\"x2\":0.5,\"y2\":0.5},"
            "{\"style\":\"pass\",\"x1\":0.1,\"y1\":0.1,\"x2\":0.5,\"y2\":0.5,\"curve\":0.3}]}"), &b));
        QCOMPARE(b.arrows.at(0).curve, kMaxCurve);
        QCOMPARE(b.arrows.at(1).curve, -kMaxCurve);
        QCOMPARE(b.arrows.at(2).curve, kDefaultCurve);   // absente : courbure par défaut
        QCOMPARE(b.arrows.at(3).curve, 0.0);             // une passe reste droite
    }

    void emptyAndDefaultJsonGiveEmptyBoard()
    {
        for (const QString &json : {QStringLiteral("{}"), QString(), QStringLiteral("  ")}) {
            Board b;
            b.items.append(Item());   // doit être remis à zéro
            QVERIFY(fromJson(json, &b));
            QVERIFY(b.items.isEmpty());
            QVERIFY(b.arrows.isEmpty());
            QVERIFY(b.pitch == Pitch::Full);
        }
    }

    void invalidJsonIsRejected()
    {
        Board b;
        QString error;
        QVERIFY(!fromJson(QStringLiteral("pas du json"), &b, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!fromJson(QStringLiteral("[1,2]"), &b));
        QVERIFY(!fromJson(QStringLiteral("{\"version\":99}"), &b));
        QVERIFY(!fromJson(QStringLiteral("{\"version\":1,\"pitch\":\"carre\"}"), &b));
        QVERIFY(!fromJson(QStringLiteral("{\"version\":1,\"items\":[{\"kind\":\"dragon\",\"x\":0.5,\"y\":0.5}]}"), &b));
        QVERIFY(!fromJson(QStringLiteral("{\"version\":1,\"items\":[{\"kind\":\"player\",\"x\":\"a\",\"y\":0.5}]}"), &b));
        QVERIFY(!fromJson(QStringLiteral("{\"version\":1,\"arrows\":[{\"style\":\"pass\",\"x1\":0,\"y1\":0,\"x2\":1}]}"), &b));
    }

    void coordinatesAreClampedOnRead()
    {
        Board b;
        QVERIFY(fromJson(QStringLiteral("{\"version\":1,\"items\":[{\"kind\":\"player\",\"x\":-3,\"y\":9}]}"), &b));
        QCOMPARE(b.items.size(), 1);
        QCOMPARE(b.items.first().pos, QPointF(0, 1));
    }

    // --- Formations ---------------------------------------------------------------------------

    void formationsAreComplete()
    {
        QCOMPARE(formationNames().size(), 4);
        for (const QString &name : formationNames()) {
            const QList<Item> players = formation(name);
            QCOMPARE(players.size(), 11);
            QSet<int> numbers;
            int outfield = 0;
            for (const Item &p : players) {
                QVERIFY(p.kind == Kind::Player);
                QVERIFY(p.pos.x() >= 0 && p.pos.x() <= 1 && p.pos.y() >= 0 && p.pos.y() <= 1);
                numbers.insert(p.label.toInt());
            }
            QCOMPARE(numbers.size(), 11);                 // numéros tous différents
            QVERIFY(numbers.contains(1) && numbers.contains(11));
            QCOMPARE(players.first().label, QStringLiteral("1"));
            QVERIFY(players.first().pos.x() < 0.1);       // gardien près de son but, à gauche

            // Le nom de la formation décrit les lignes : défenseurs-milieux-attaquants.
            QStringList parts = name.split(QLatin1Char('-'));
            int expectedOutfield = 0;
            for (const QString &p : parts)
                expectedOutfield += p.toInt();
            for (const Item &p : players) {
                if (p.label != QLatin1String("1"))
                    ++outfield;
            }
            QCOMPARE(outfield, expectedOutfield);
        }
        QVERIFY(formation(QStringLiteral("9-9-9")).isEmpty());
    }

    void formationLinesHaveTheRightSizes()
    {
        // 4-4-2 : 4 défenseurs (x ≈ 0.2), 4 milieux (x ≈ 0.45), 2 attaquants (x ≈ 0.7).
        int back = 0, mid = 0, fwd = 0;
        for (const Item &p : formation(QStringLiteral("4-4-2"))) {
            if (p.pos.x() < 0.1) continue;
            if (p.pos.x() < 0.3) ++back;
            else if (p.pos.x() < 0.6) ++mid;
            else ++fwd;
        }
        QCOMPARE(back, 4);
        QCOMPARE(mid, 4);
        QCOMPARE(fwd, 2);
    }

    void mirroredOpponentsFaceTheTeam()
    {
        const QList<Item> team = formation(QStringLiteral("4-3-3"));
        const QList<Item> opp = mirroredAsOpponent(team);
        QCOMPARE(opp.size(), team.size());
        for (int i = 0; i < team.size(); ++i) {
            QVERIFY(opp.at(i).kind == Kind::Opponent);
            QCOMPARE(opp.at(i).pos.x(), 1.0 - team.at(i).pos.x());
            QCOMPARE(opp.at(i).pos.y(), 1.0 - team.at(i).pos.y());
            QCOMPARE(opp.at(i).label, team.at(i).label);
        }
    }

    void nextFreeNumber()
    {
        QList<Item> items;
        QCOMPARE(nextFreePlayerNumber(items), 1);
        items.append(mk(Kind::Player, QPointF(), QStringLiteral("1")));
        items.append(mk(Kind::Player, QPointF(), QStringLiteral("3")));
        items.append(mk(Kind::Opponent, QPointF(), QStringLiteral("2")));   // un adversaire n'occupe pas le numéro
        items.append(mk(Kind::Player, QPointF(), QStringLiteral("AB")));    // étiquette non numérique ignorée
        QCOMPARE(nextFreePlayerNumber(items), 2);

        items.clear();
        for (int n = 1; n <= 99; ++n)
            items.append(mk(Kind::Player, QPointF(), QString::number(n)));
        QCOMPARE(nextFreePlayerNumber(items), 0);
    }

    // --- Base ---------------------------------------------------------------------------------

    void init()
    {
        m_db = new Database;
        QString error;
        QVERIFY2(m_db->open(QStringLiteral(":memory:"), &error), qPrintable(error));
        m_repo = new BoardRepository(m_db->handle());
        m_ex = new ExerciseRepository(m_db->handle());
        m_plan = new PlanRepository(m_db->handle());
        m_sessions = new SessionRepository(m_db->handle());

        TeamRepository teams(m_db->handle());
        m_team.name = QStringLiteral("U15");
        QVERIFY(teams.insert(m_team));
        m_session = makeSession(QDate(2026, 10, 27));
    }

    void cleanup()
    {
        delete m_sessions;
        delete m_plan;
        delete m_ex;
        delete m_repo;
        delete m_db;
    }

    void repositoryCrud()
    {
        BoardRecord b;
        b.title = QStringLiteral("  Sortie de balle  ");
        b.formation = QStringLiteral("4-3-3");
        Board content;
        content.items = formation(QStringLiteral("4-3-3"));
        b.sceneJson = toJson(content);
        QVERIFY(m_repo->insert(b));
        QVERIFY(b.id > 0);

        QList<BoardRecord> all = m_repo->list();
        QCOMPARE(all.size(), 1);
        QCOMPARE(all.first().title, QStringLiteral("Sortie de balle"));   // titre nettoyé
        QCOMPARE(all.first().formation, QStringLiteral("4-3-3"));
        QCOMPARE(all.first().exerciseId, -1);
        QCOMPARE(all.first().sessionId, -1);
        QVERIFY(!all.first().updatedAt.isEmpty());
        Board back;
        QVERIFY(fromJson(all.first().sceneJson, &back));
        QCOMPARE(back.items.size(), 11);

        b.title = QStringLiteral("A renommé");
        b.exerciseId = firstExerciseId();
        b.sessionId = m_session.id;
        QVERIFY(m_repo->update(b));
        all = m_repo->list();
        QCOMPARE(all.first().title, QStringLiteral("A renommé"));
        QCOMPARE(all.first().exerciseId, b.exerciseId);
        QCOMPARE(all.first().sessionId, m_session.id);

        QVERIFY(m_repo->remove(b.id));
        QCOMPARE(m_repo->list().size(), 0);
    }

    void defaultSceneIsEmptyBoard()
    {
        BoardRecord b;
        b.title = QStringLiteral("Vide");
        b.sceneJson.clear();
        QVERIFY(m_repo->insert(b));
        const BoardRecord r = m_repo->list().first();
        Board board;
        QVERIFY(fromJson(r.sceneJson, &board));
        QVERIFY(board.items.isEmpty());
    }

    void listIsSortedByTitle()
    {
        for (const QString &t : {QStringLiteral("b"), QStringLiteral("A"), QStringLiteral("c")}) {
            BoardRecord b;
            b.title = t;
            QVERIFY(m_repo->insert(b));
        }
        const QList<BoardRecord> all = m_repo->list();
        QCOMPARE(all.at(0).title, QStringLiteral("A"));
        QCOMPARE(all.at(1).title, QStringLiteral("b"));
        QCOMPARE(all.at(2).title, QStringLiteral("c"));
    }

    void deletingExerciseOrSessionKeepsBoard()
    {
        Exercise e;
        e.title = QStringLiteral("Temp");
        QVERIFY(m_ex->insert(e));
        BoardRecord b;
        b.title = QStringLiteral("Lié");
        b.exerciseId = e.id;
        b.sessionId = m_session.id;
        QVERIFY(m_repo->insert(b));

        QVERIFY(m_ex->remove(e.id));
        QVERIFY(m_sessions->remove(m_session.id));
        const QList<BoardRecord> all = m_repo->list();
        QCOMPARE(all.size(), 1);                      // le schéma survit
        QCOMPARE(all.first().exerciseId, -1);         // ON DELETE SET NULL
        QCOMPARE(all.first().sessionId, -1);
    }

    void forSessionFindsDirectAndExerciseLinks()
    {
        const int exA = firstExerciseId();
        const int exB = m_ex->list(false).at(1).id;
        const int exC = m_ex->list(false).at(2).id;
        QVERIFY(m_plan->add(m_session.id, exA, 15) > 0);
        QVERIFY(m_plan->add(m_session.id, exB, 15) > 0);
        const Session other = makeSession(QDate(2026, 11, 3));

        int direct = addBoard(QStringLiteral("direct"), -1, m_session.id);
        int viaA = addBoard(QStringLiteral("via A"), exA, -1);
        int both = addBoard(QStringLiteral("les deux"), exB, m_session.id);   // pas de doublon attendu
        addBoard(QStringLiteral("exercice hors plan"), exC, -1);
        addBoard(QStringLiteral("autre séance"), -1, other.id);
        addBoard(QStringLiteral("libre"), -1, -1);

        const QList<BoardRecord> found = m_repo->forSession(m_session.id);
        QList<int> ids;
        for (const BoardRecord &r : found)
            ids << r.id;
        QCOMPARE(ids, (QList<int>{direct, viaA, both}));
        QVERIFY(m_repo->forSession(9999).isEmpty());
    }

    // --- Fiche PDF ----------------------------------------------------------------------------

    void sheetListsBoardsAfterThePlan()
    {
        const int exA = firstExerciseId();
        QVERIFY(m_plan->add(m_session.id, exA, 15) > 0);
        const QList<PlanEntry> plan = m_plan->entries(m_session.id);
        QCOMPARE(plan.size(), 1);

        SessionSheet::SheetBoard free;
        free.title = QStringLiteral("Schéma libre");
        free.imageName = QStringLiteral("board_2");
        SessionSheet::SheetBoard linked;
        linked.title = QStringLiteral("Schéma <A>");
        linked.imageName = QStringLiteral("board_1");
        linked.exerciseId = exA;
        SessionSheet::SheetBoard outsidePlan;   // exercice absent du plan : ne doit pas disparaître
        outsidePlan.title = QStringLiteral("Hors plan");
        outsidePlan.imageName = QStringLiteral("board_3");
        outsidePlan.exerciseId = 99999;

        const QString without = SessionSheet::buildHtml(m_team, m_session, plan);
        QVERIFY(!without.contains(QLatin1String("<img")));
        QVERIFY(!without.contains(QStringLiteral("Schémas")));

        const QString html = SessionSheet::buildHtml(m_team, m_session, plan, {free, linked});
        QCOMPARE(html.count(QLatin1String("<img")), 2);
        QVERIFY(html.contains(QStringLiteral("Schéma &lt;A&gt;")));          // titre échappé
        QVERIFY(html.contains(QStringLiteral("exercice : %1").arg(plan.first().exercise.title.toHtmlEscaped())));
        // Plusieurs schémas : ceux des exercices d'abord, même s'ils sont fournis en dernier.
        const int section = html.indexOf(QStringLiteral("Schémas</h2>"));
        QVERIFY(section > html.indexOf(QStringLiteral("Plan de séance")));
        QVERIFY(html.indexOf(QLatin1String("board_1")) > section);
        QVERIFY(html.indexOf(QLatin1String("board_1")) < html.indexOf(QLatin1String("board_2")));
        // Les images ne sont plus dans le tableau du plan (elles écrasaient ses colonnes).
        QVERIFY(html.indexOf(QLatin1String("<img")) > html.lastIndexOf(QLatin1String("</table>")));

        // Un schéma lié à un exercice absent du plan n'est pas affiché (la page l'enverra sans lien d'exercice).
        QCOMPARE(SessionSheet::buildHtml(m_team, m_session, plan, {outsidePlan}).count(QLatin1String("<img")), 0);
    }

private:
    static Item mk(Kind kind, const QPointF &pos, const QString &label = QString(), double rotation = 0,
                   double scale = 1.0, const QString &color = QString())
    {
        Item it;
        it.kind = kind;
        it.pos = pos;
        it.label = label;
        it.rotation = rotation;
        it.scale = scale;
        it.color = color;
        return it;
    }

    Session makeSession(const QDate &date)
    {
        Session s;
        s.teamId = m_team.id;
        s.startAt = QDateTime(date, QTime(18, 30));
        s.durationMin = 90;
        if (!m_sessions->insert(s))
            QTest::qFail("insertion de séance", __FILE__, __LINE__);
        return s;
    }

    int firstExerciseId() const { return m_ex->list(false).first().id; }

    int addBoard(const QString &title, int exerciseId, int sessionId)
    {
        BoardRecord b;
        b.title = title;
        b.exerciseId = exerciseId;
        b.sessionId = sessionId;
        if (!m_repo->insert(b))
            QTest::qFail("insertion de schéma", __FILE__, __LINE__);
        return b.id;
    }

    Database *m_db = nullptr;
    BoardRepository *m_repo = nullptr;
    ExerciseRepository *m_ex = nullptr;
    PlanRepository *m_plan = nullptr;
    SessionRepository *m_sessions = nullptr;
    Team m_team;
    Session m_session;
};

QTEST_GUILESS_MAIN(BoardTest)
#include "tst_board.moc"
