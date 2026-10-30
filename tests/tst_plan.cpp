#include "core/Database.h"
#include "core/ExerciseRepository.h"
#include "core/PlanRepository.h"
#include "core/PlanSchedule.h"
#include "core/SessionRepository.h"
#include "core/SessionSheet.h"
#include "core/TeamRepository.h"

#include <QSqlQuery>
#include <QtTest>

class PlanTest : public QObject
{
    Q_OBJECT

private slots:
    // --- Horaires -----------------------------------------------------------------------------

    void scheduleIsSequential()
    {
        const QDateTime start(QDate(2026, 10, 27), QTime(18, 30));
        const auto timeline = PlanSchedule::schedule(start, {15, 20, 0, 10});
        QCOMPARE(timeline.size(), 4);
        QCOMPARE(timeline.at(0).start, start);
        QCOMPARE(timeline.at(0).end.time(), QTime(18, 45));
        QCOMPARE(timeline.at(1).start.time(), QTime(18, 45));
        QCOMPARE(timeline.at(1).end.time(), QTime(19, 5));
        QCOMPARE(timeline.at(2).start, timeline.at(2).end);                 // durée nulle : créneau vide
        QCOMPARE(timeline.at(3).end.time(), QTime(19, 15));
        QCOMPARE(PlanSchedule::totalMinutes({15, 20, 0, 10}), 45);
    }

    void scheduleIgnoresNegativeDurationsAndCrossesMidnight()
    {
        const QDateTime start(QDate(2026, 10, 27), QTime(23, 30));
        const auto timeline = PlanSchedule::schedule(start, {-5, 60});
        QCOMPARE(timeline.at(0).end, start);
        QCOMPARE(timeline.at(1).end, QDateTime(QDate(2026, 10, 28), QTime(0, 30)));
        QCOMPARE(PlanSchedule::totalMinutes({-5, 60}), 60);
        QVERIFY(PlanSchedule::schedule(start, {}).isEmpty());
    }

    // --- Base : migrations et exercices de départ ----------------------------------------------

    void init()
    {
        m_db = new Database;
        QString error;
        QVERIFY2(m_db->open(QStringLiteral(":memory:"), &error), qPrintable(error));
        m_ex = new ExerciseRepository(m_db->handle());
        m_plan = new PlanRepository(m_db->handle());
        m_sessions = new SessionRepository(m_db->handle());

        TeamRepository teams(m_db->handle());
        Team t;
        t.name = QStringLiteral("U15");
        QVERIFY(teams.insert(t));
        m_team = t;
        Session s;
        s.teamId = t.id;
        s.startAt = QDateTime(QDate(2026, 10, 27), QTime(18, 30));
        s.durationMin = 90;
        QVERIFY(m_sessions->insert(s));
        m_session = s;
    }

    void cleanup()
    {
        delete m_sessions;
        delete m_plan;
        delete m_ex;
        delete m_db;
    }

    void seedExercisesPresent()
    {
        QCOMPARE(m_db->schemaVersion(), 4);
        const QList<Exercise> all = m_ex->list(false);
        QVERIFY2(all.size() >= 15, "une quinzaine d'exercices de départ attendus");
        QSet<QString> categories;
        for (const Exercise &e : all) {
            QVERIFY(!e.title.isEmpty());
            QVERIFY(!e.description.isEmpty());
            QVERIFY(e.durationMin > 0);
            QVERIFY(!e.archived);
            categories.insert(e.category);
        }
        QCOMPARE(categories.size(), 6);   // les six catégories sont représentées
    }

    // --- ExerciseRepository --------------------------------------------------------------------

    void exerciseCrud()
    {
        Exercise e;
        e.title = QStringLiteral("  Jeu de têtes  ");
        e.category = QStringLiteral("technical");
        e.durationMin = 10;
        e.playersMin = 4;
        QVERIFY(m_ex->insert(e));
        QVERIFY(e.id > 0);

        e.playersMax = 12;
        e.equipment = QStringLiteral("Ballons");
        QVERIFY(m_ex->update(e));

        Exercise found;
        for (const Exercise &x : m_ex->list(false)) {
            if (x.id == e.id)
                found = x;
        }
        QCOMPARE(found.title, QStringLiteral("Jeu de têtes"));   // espaces retirés
        QCOMPARE(found.playersMin, 4);
        QCOMPARE(found.playersMax, 12);
        QCOMPARE(found.equipment, QStringLiteral("Ballons"));

        QVERIFY(m_ex->remove(e.id));   // jamais utilisé : suppression autorisée
    }

    void invalidCategoryRejected()
    {
        Exercise e;
        e.title = QStringLiteral("X");
        e.category = QStringLiteral("bogus");
        QString error;
        QVERIFY(!m_ex->insert(e, &error));
        QVERIFY(!error.isEmpty());
    }

    void archivedHiddenUnlessRequested()
    {
        const int before = int(m_ex->list(false).size());
        const Exercise first = m_ex->list(false).first();
        QVERIFY(m_ex->setArchived(first.id, true));
        QCOMPARE(int(m_ex->list(false).size()), before - 1);
        QCOMPARE(int(m_ex->list(true).size()), before);
        QVERIFY(m_ex->setArchived(first.id, false));
        QCOMPARE(int(m_ex->list(false).size()), before);
    }

    void usedExerciseCannotBeDeletedButCanBeArchived()
    {
        const Exercise ex = m_ex->list(false).first();
        QVERIFY(m_plan->add(m_session.id, ex.id, 15) > 0);
        QCOMPARE(m_ex->usageCount(ex.id), 1);

        QString error;
        QVERIFY(!m_ex->remove(ex.id, &error));
        QVERIFY(error.contains(QStringLiteral("archivez")));

        QVERIFY(m_ex->setArchived(ex.id, true));
        // L'exercice archivé reste lisible dans le plan existant.
        const auto entries = m_plan->entries(m_session.id);
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.first().exercise.title, ex.title);
        QVERIFY(entries.first().exercise.archived);
    }

    void usageCountCountsSessionsNotLines()
    {
        const Exercise ex = m_ex->list(false).first();
        QVERIFY(m_plan->add(m_session.id, ex.id, 10) > 0);
        QVERIFY(m_plan->add(m_session.id, ex.id, 10) > 0);   // même exercice deux fois dans le même plan
        QCOMPARE(m_ex->usageCount(ex.id), 1);
    }

    // --- PlanRepository ------------------------------------------------------------------------

    void addAppendsInOrder()
    {
        const QList<Exercise> all = m_ex->list(false);
        const int a = m_plan->add(m_session.id, all.at(0).id, 10, QStringLiteral("  première  "));
        const int b = m_plan->add(m_session.id, all.at(1).id, 20);
        const int c = m_plan->add(m_session.id, all.at(2).id, 0);   // durée absente -> NULL -> 0
        QVERIFY(a > 0 && b > 0 && c > 0);

        const auto e = m_plan->entries(m_session.id);
        QCOMPARE(e.size(), 3);
        QCOMPARE(e.at(0).position, 1);
        QCOMPARE(e.at(1).position, 2);
        QCOMPARE(e.at(2).position, 3);
        QCOMPARE(e.at(0).exercise.id, all.at(0).id);
        QCOMPARE(e.at(0).notes, QStringLiteral("première"));
        QCOMPARE(e.at(1).durationMin, 20);
        QCOMPARE(e.at(2).durationMin, 0);
    }

    void plansAreIndependentPerSession()
    {
        Session other;
        other.teamId = m_team.id;
        other.startAt = QDateTime(QDate(2026, 10, 29), QTime(18, 30));
        QVERIFY(m_sessions->insert(other));
        const Exercise ex = m_ex->list(false).first();
        QVERIFY(m_plan->add(m_session.id, ex.id, 10) > 0);
        QVERIFY(m_plan->add(other.id, ex.id, 10) > 0);
        QCOMPARE(m_plan->entries(m_session.id).first().position, 1);
        QCOMPARE(m_plan->entries(other.id).first().position, 1);   // chaque plan repart à 1
    }

    void moveUpAndDown()
    {
        const QList<Exercise> all = m_ex->list(false);
        const int a = m_plan->add(m_session.id, all.at(0).id, 10);
        const int b = m_plan->add(m_session.id, all.at(1).id, 10);
        const int c = m_plan->add(m_session.id, all.at(2).id, 10);

        QVERIFY(m_plan->move(c, -1));   // a c b
        QCOMPARE(ids(), QList<int>({a, c, b}));
        QVERIFY(m_plan->move(a, +1));   // c a b
        QCOMPARE(ids(), QList<int>({c, a, b}));
        QVERIFY(m_plan->move(c, -1));   // déjà premier : sans effet
        QCOMPARE(ids(), QList<int>({c, a, b}));
        QVERIFY(m_plan->move(b, +1));   // déjà dernier : sans effet
        QCOMPARE(ids(), QList<int>({c, a, b}));

        const auto e = m_plan->entries(m_session.id);
        QCOMPARE(e.at(0).position, 1);
        QCOMPARE(e.at(2).position, 3);

        QString error;
        QVERIFY(!m_plan->move(a, 2, &error));   // seuls -1 et +1 sont valides
    }

    void removeCompactsPositions()
    {
        const QList<Exercise> all = m_ex->list(false);
        const int a = m_plan->add(m_session.id, all.at(0).id, 10);
        const int b = m_plan->add(m_session.id, all.at(1).id, 10);
        const int c = m_plan->add(m_session.id, all.at(2).id, 10);
        const int d = m_plan->add(m_session.id, all.at(3).id, 10);

        QVERIFY(m_plan->remove(b));
        QCOMPARE(ids(), QList<int>({a, c, d}));
        const auto e = m_plan->entries(m_session.id);
        QCOMPARE(e.at(0).position, 1);
        QCOMPARE(e.at(1).position, 2);
        QCOMPARE(e.at(2).position, 3);

        // On peut réajouter après suppression sans conflit de position.
        QVERIFY(m_plan->add(m_session.id, all.at(4).id, 10) > 0);
        QCOMPARE(m_plan->entries(m_session.id).last().position, 4);

        QString error;
        QVERIFY(!m_plan->remove(99999, &error));   // ligne inexistante
        QVERIFY(!error.isEmpty());
    }

    void updateItem()
    {
        const Exercise ex = m_ex->list(false).first();
        const int id = m_plan->add(m_session.id, ex.id, 10);
        QVERIFY(m_plan->updateItem(id, 25, QStringLiteral("Insister sur la première passe")));
        const auto e = m_plan->entries(m_session.id).first();
        QCOMPARE(e.durationMin, 25);
        QCOMPARE(e.notes, QStringLiteral("Insister sur la première passe"));
    }

    void deletingSessionRemovesItsPlan()
    {
        const Exercise ex = m_ex->list(false).first();
        QVERIFY(m_plan->add(m_session.id, ex.id, 10) > 0);
        QVERIFY(m_sessions->remove(m_session.id));
        QCOMPARE(m_plan->entries(m_session.id).size(), 0);
        QCOMPARE(m_ex->usageCount(ex.id), 0);   // l'exercice redevient supprimable
    }

    // --- Fiche de séance -----------------------------------------------------------------------

    void sheetContainsScheduleAndEscapesHtml()
    {
        Exercise e1;
        e1.title = QStringLiteral("Rondo <4> & \"co\"");
        e1.category = QStringLiteral("technical");
        e1.equipment = QStringLiteral("Ballons, Plots");
        QVERIFY(m_ex->insert(e1));
        Exercise e2;
        e2.title = QStringLiteral("Match");
        e2.category = QStringLiteral("game");
        e2.equipment = QStringLiteral("plots, Chasubles, Aucun");   // « plots » en doublon (casse), « Aucun » ignoré
        QVERIFY(m_ex->insert(e2));
        QVERIFY(m_plan->add(m_session.id, e1.id, 15, QStringLiteral("Note <b>")) > 0);
        QVERIFY(m_plan->add(m_session.id, e2.id, 80) > 0);   // 95 min au total, séance de 90

        Session s = m_session;
        s.location = QStringLiteral("Stade A");
        s.theme = QStringLiteral("Possession");
        const QString html = SessionSheet::buildHtml(m_team, s, m_plan->entries(s.id));

        QVERIFY(html.contains(QStringLiteral("Rondo &lt;4&gt; &amp; &quot;co&quot;")) || html.contains(QStringLiteral("Rondo &lt;4&gt; &amp; \"co\"")));
        QVERIFY(!html.contains(QStringLiteral("<4>")));
        QVERIFY(html.contains(QStringLiteral("Note &lt;b&gt;")));
        QVERIFY(html.contains(QStringLiteral("18:30")));
        QVERIFY(html.contains(QStringLiteral("18:45")));                       // fin du 1er exercice
        QVERIFY(html.contains(QStringLiteral("20:05")));                       // fin du 2e (18:45 + 80 min)
        QVERIFY(html.contains(QStringLiteral("Stade A")));
        QVERIFY(html.contains(QStringLiteral("Total des exercices : 95 min")));
        QVERIFY(html.contains(QStringLiteral("dépasse la durée de la séance de 5 min")));
        QVERIFY(html.contains(QStringLiteral("Matériel à prévoir :")));
        QCOMPARE(html.count(QStringLiteral("Plots"), Qt::CaseInsensitive) >= 1, true);
        // Dans la liste « Matériel à prévoir » : « Plots » une seule fois (dédoublonnage) et jamais « Aucun ».
        const int idx = html.indexOf(QStringLiteral("Matériel à prévoir"));
        QVERIFY(idx > 0);
        const QString toBring = html.mid(idx);
        QCOMPARE(toBring.count(QStringLiteral("lots"), Qt::CaseInsensitive), 1);
        QVERIFY(!toBring.contains(QStringLiteral("Aucun")));
        QVERIFY(toBring.contains(QStringLiteral("Chasubles")));
    }

    void sheetWithoutOvertimeOrPlan()
    {
        const QString empty = SessionSheet::buildHtml(m_team, m_session, {});
        QVERIFY(empty.contains(QStringLiteral("Aucun exercice dans le plan")));
        QVERIFY(!empty.contains(QStringLiteral("dépasse")));

        const Exercise ex = m_ex->list(false).first();
        QVERIFY(m_plan->add(m_session.id, ex.id, 30) > 0);
        const QString ok = SessionSheet::buildHtml(m_team, m_session, m_plan->entries(m_session.id));
        QVERIFY(!ok.contains(QStringLiteral("dépasse")));
        QVERIFY(ok.contains(QStringLiteral("Temps libre : 60 min")));
    }

    void suggestedFileNameIsSafe()
    {
        Team t;
        t.name = QStringLiteral("FC Éxemple / U15 : \"A\"");
        Session s = m_session;
        const QString name = SessionSheet::suggestedFileName(t, s);
        QVERIFY(name.startsWith(QStringLiteral("Seance_2026-10-27_")));
        QVERIFY(name.endsWith(QStringLiteral(".pdf")));
        for (QChar ch : QStringLiteral("/\\:*?\"<>|"))
            QVERIFY2(!name.contains(ch), "caractère interdit dans un nom de fichier Windows");
        QCOMPARE(SessionSheet::suggestedFileName(Team{}, s), QStringLiteral("Seance_2026-10-27.pdf"));
    }

private:
    QList<int> ids() const
    {
        QList<int> result;
        for (const PlanEntry &e : m_plan->entries(m_session.id))
            result << e.itemId;
        return result;
    }

    Database *m_db = nullptr;
    ExerciseRepository *m_ex = nullptr;
    PlanRepository *m_plan = nullptr;
    SessionRepository *m_sessions = nullptr;
    Team m_team;
    Session m_session;
};

QTEST_GUILESS_MAIN(PlanTest)
#include "tst_plan.moc"
