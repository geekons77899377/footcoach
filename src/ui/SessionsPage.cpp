#include "SessionsPage.h"

#include "core/PlanSchedule.h"
#include "core/TacticalBoard.h"
#include "core/SessionSheet.h"
#include "ui/BoardScene.h"
#include "ui/ExercisePicker.h"
#include "ui/Labels.h"
#include "ui/PdfExport.h"
#include "ui/PlanItemDialog.h"

#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QImage>
#include <QLocale>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include <QSplitter>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {

// Même clé que les autres pages : l'équipe choisie est partagée.
const QString kLastTeamKey = QStringLiteral("squad/lastTeamId");

enum Scope { ScopeUpcoming, ScopePast, ScopeAll };

QTableWidgetItem *cell(const QString &text, bool centered = false)
{
    auto *item = new QTableWidgetItem(text);
    if (centered)
        item->setTextAlignment(Qt::AlignCenter);
    return item;
}

} // namespace

SessionsPage::SessionsPage(const QSqlDatabase &db, QWidget *parent)
    : QWidget(parent)
    , m_teams(db)
    , m_sessions(db)
    , m_exercises(db)
    , m_plan(db)
    , m_boards(db)
{
    m_teamCombo = new QComboBox;
    m_teamCombo->setMinimumWidth(220);

    m_scope = new QComboBox;
    m_scope->addItem(tr("Séances à venir"), ScopeUpcoming);
    m_scope->addItem(tr("Séances passées"), ScopePast);
    m_scope->addItem(tr("Toutes les séances"), ScopeAll);

    m_sessionTable = new QTableWidget(0, 4);
    m_sessionTable->setHorizontalHeaderLabels({tr("Date"), tr("Thème"), tr("Statut"), tr("Plan")});
    m_sessionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sessionTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sessionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_sessionTable->setAlternatingRowColors(true);
    m_sessionTable->verticalHeader()->hide();
    m_sessionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    m_planTitle = new QLabel;
    m_planTitle->setTextFormat(Qt::RichText);

    m_planTable = new QTableWidget(0, 6);
    m_planTable->setHorizontalHeaderLabels({tr("N°"), tr("Horaire"), tr("Exercice"), tr("Catégorie"), tr("Durée"), tr("Note")});
    m_planTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_planTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_planTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_planTable->setAlternatingRowColors(true);
    m_planTable->verticalHeader()->hide();
    m_planTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_planTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

    m_total = new QLabel;
    m_total->setTextFormat(Qt::RichText);

    m_addButton = new QPushButton(tr("Ajouter des exercices…"));
    m_editButton = new QPushButton(tr("Durée / note…"));
    m_upButton = new QPushButton(tr("Monter"));
    m_downButton = new QPushButton(tr("Descendre"));
    m_removeButton = new QPushButton(tr("Retirer"));
    m_pdfButton = new QPushButton(tr("Exporter en PDF…"));

    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel(tr("Équipe :")));
    top->addWidget(m_teamCombo);
    top->addWidget(m_scope);
    top->addStretch();

    auto *planButtons = new QHBoxLayout;
    planButtons->addWidget(m_addButton);
    planButtons->addWidget(m_editButton);
    planButtons->addWidget(m_upButton);
    planButtons->addWidget(m_downButton);
    planButtons->addWidget(m_removeButton);
    planButtons->addStretch();
    planButtons->addWidget(m_pdfButton);

    auto *rightPane = new QWidget;
    auto *right = new QVBoxLayout(rightPane);
    right->setContentsMargins(0, 0, 0, 0);
    right->addWidget(m_planTitle);
    right->addWidget(m_planTable, 1);
    right->addWidget(m_total);
    right->addLayout(planButtons);

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(m_sessionTable);
    splitter->addWidget(rightPane);
    splitter->setChildrenCollapsible(false);
    m_sessionTable->setMinimumWidth(420);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({470, 680});

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(splitter, 1);

    connect(m_teamCombo, &QComboBox::currentIndexChanged, this, [this] {
        const int id = currentTeamId();
        if (id >= 0)
            QSettings().setValue(kLastTeamKey, id);
        reloadSessions();
    });
    connect(m_scope, &QComboBox::currentIndexChanged, this, &SessionsPage::reloadSessions);
    connect(m_sessionTable, &QTableWidget::currentCellChanged, this, &SessionsPage::reloadPlan);
    connect(m_planTable, &QTableWidget::currentCellChanged, this, &SessionsPage::updateButtons);
    connect(m_planTable, &QTableWidget::cellDoubleClicked, this, &SessionsPage::editItem);
    connect(m_addButton, &QPushButton::clicked, this, &SessionsPage::addExercises);
    connect(m_editButton, &QPushButton::clicked, this, &SessionsPage::editItem);
    connect(m_upButton, &QPushButton::clicked, this, [this] { moveItem(-1); });
    connect(m_downButton, &QPushButton::clicked, this, [this] { moveItem(+1); });
    connect(m_removeButton, &QPushButton::clicked, this, &SessionsPage::removeItem);
    connect(m_pdfButton, &QPushButton::clicked, this, &SessionsPage::exportPdf);

    loadTeams();
}

void SessionsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Équipes et séances ont pu changer depuis les autres pages.
    loadTeams();
}

int SessionsPage::currentTeamId() const
{
    return m_teamCombo->currentIndex() >= 0 ? m_teamCombo->currentData().toInt() : -1;
}

Team SessionsPage::currentTeam() const
{
    Team result;
    const int id = currentTeamId();
    for (const Team &t : m_teams.all()) {
        if (t.id == id)
            result = t;
    }
    return result;
}

const Session *SessionsPage::selectedSession() const
{
    const int row = m_sessionTable->currentRow();
    return (row >= 0 && row < m_listedSessions.size()) ? &m_listedSessions.at(row) : nullptr;
}

const PlanEntry *SessionsPage::selectedItem() const
{
    const int row = m_planTable->currentRow();
    return (row >= 0 && row < m_entries.size()) ? &m_entries.at(row) : nullptr;
}

void SessionsPage::loadTeams()
{
    int preferred = currentTeamId();
    if (preferred < 0)
        preferred = QSettings().value(kLastTeamKey, -1).toInt();

    QString error;
    const QList<Team> teams = m_teams.all(&error);
    if (!error.isEmpty())
        showError(error);

    m_teamCombo->blockSignals(true);
    m_teamCombo->clear();
    int select = 0;
    for (int i = 0; i < teams.size(); ++i) {
        const Team &t = teams.at(i);
        m_teamCombo->addItem(t.category.isEmpty() ? t.name : QStringLiteral("%1 (%2)").arg(t.name, t.category), t.id);
        if (t.id == preferred)
            select = i;
    }
    if (!teams.isEmpty())
        m_teamCombo->setCurrentIndex(select);
    m_teamCombo->blockSignals(false);

    reloadSessions();
}

void SessionsPage::reloadSessions()
{
    const int keepId = selectedSession() ? selectedSession()->id : -1;

    m_listedSessions.clear();
    m_sessionTable->blockSignals(true);   // le plan n'est rechargé qu'une fois, à la fin
    m_sessionTable->setRowCount(0);

    const int teamId = currentTeamId();
    if (teamId >= 0) {
        const QDate today = QDate::currentDate();
        QDate from(2000, 1, 1), to(2100, 12, 31);
        switch (m_scope->currentData().toInt()) {
        case ScopeUpcoming: from = today; break;
        case ScopePast:     to = today.addDays(-1); break;
        default: break;
        }
        QString error;
        m_listedSessions = m_sessions.listBetween(teamId, from, to, &error);
        if (!error.isEmpty())
            showError(error);
        if (m_scope->currentData().toInt() == ScopePast)
            std::reverse(m_listedSessions.begin(), m_listedSessions.end());   // les plus récentes d'abord
    }

    const QLocale fr(QLocale::French);
    m_sessionTable->setRowCount(int(m_listedSessions.size()));
    for (int row = 0; row < m_listedSessions.size(); ++row) {
        const Session &s = m_listedSessions.at(row);
        const QList<PlanEntry> plan = m_plan.entries(s.id);
        QList<int> durations;
        for (const PlanEntry &p : plan)
            durations << p.durationMin;
        const QString planText = plan.isEmpty() ? QStringLiteral("—")
                                                : tr("%n exercice(s), %1 min", nullptr, int(plan.size()))
                                                      .arg(PlanSchedule::totalMinutes(durations));
        m_sessionTable->setItem(row, 0, cell(QStringLiteral("%1 %2").arg(
            fr.toString(s.startAt.date(), QStringLiteral("ddd dd/MM")), s.startAt.time().toString(QStringLiteral("HH:mm")))));
        m_sessionTable->setItem(row, 1, cell(s.theme));
        m_sessionTable->setItem(row, 2, cell(Labels::label(Labels::statuses(), s.status)));
        m_sessionTable->setItem(row, 3, cell(planText));
        if (s.status == QLatin1String("cancelled")) {
            for (int col = 0; col < 4; ++col)
                m_sessionTable->item(row, col)->setForeground(QBrush(QColor(Qt::gray)));
        }
    }
    m_sessionTable->resizeColumnToContents(0);
    m_sessionTable->resizeColumnToContents(2);
    m_sessionTable->resizeColumnToContents(3);

    int select = m_listedSessions.isEmpty() ? -1 : 0;   // première séance sélectionnée par défaut
    for (int row = 0; row < m_listedSessions.size(); ++row) {
        if (m_listedSessions.at(row).id == keepId)
            select = row;
    }
    if (select >= 0)
        m_sessionTable->selectRow(select);
    m_sessionTable->blockSignals(false);

    reloadPlan();
}

void SessionsPage::reloadPlan()
{
    const int keepItem = selectedItem() ? selectedItem()->itemId : -1;

    m_entries.clear();
    m_planTable->setRowCount(0);

    const Session *s = selectedSession();
    if (!s) {
        m_planTitle->setText(currentTeamId() < 0 ? tr("Créez d'abord une équipe dans la page Effectif.")
                                                 : tr("Aucune séance dans cette liste. Créez-en dans la page Calendrier."));
        m_total->clear();
        updateButtons();
        return;
    }

    QString error;
    m_entries = m_plan.entries(s->id, &error);
    if (!error.isEmpty())
        showError(error);

    QList<int> durations;
    for (const PlanEntry &p : std::as_const(m_entries))
        durations << p.durationMin;
    const QList<PlanSchedule::Slot> timeline = PlanSchedule::schedule(s->startAt, durations);

    const QLocale fr(QLocale::French);
    m_planTitle->setText(tr("<b>Plan de la séance du %1</b> à %2 (%3 min)")
                             .arg(fr.toString(s->startAt.date(), QStringLiteral("dddd d MMMM")),
                                  s->startAt.time().toString(QStringLiteral("HH:mm")))
                             .arg(s->durationMin));

    m_planTable->setRowCount(int(m_entries.size()));
    for (int row = 0; row < m_entries.size(); ++row) {
        const PlanEntry &p = m_entries.at(row);
        QString title = p.exercise.title;
        if (p.exercise.archived)
            title += tr(" (archivé)");
        m_planTable->setItem(row, 0, cell(QString::number(p.position), true));
        m_planTable->setItem(row, 1, cell(QStringLiteral("%1 – %2").arg(timeline.at(row).start.time().toString(QStringLiteral("HH:mm")),
                                                                         timeline.at(row).end.time().toString(QStringLiteral("HH:mm"))), true));
        m_planTable->setItem(row, 2, cell(title));
        m_planTable->setItem(row, 3, cell(Labels::label(Labels::categories(), p.exercise.category)));
        m_planTable->setItem(row, 4, cell(tr("%1 min").arg(p.durationMin), true));
        m_planTable->setItem(row, 5, cell(p.notes));
    }
    m_planTable->resizeColumnToContents(0);
    m_planTable->resizeColumnToContents(1);
    m_planTable->resizeColumnToContents(3);
    m_planTable->resizeColumnToContents(4);

    for (int row = 0; row < m_entries.size(); ++row) {
        if (m_entries.at(row).itemId == keepItem)
            m_planTable->selectRow(row);
    }

    // Dépassement : simple avertissement (pas de blocage), en rouge.
    const int total = PlanSchedule::totalMinutes(durations);
    if (total > s->durationMin) {
        m_total->setText(tr("<font color=\"#c00000\"><b>Total : %1 min — dépasse la durée de la séance (%2 min) de %3 min</b></font>")
                             .arg(total).arg(s->durationMin).arg(total - s->durationMin));
    } else if (m_entries.isEmpty()) {
        m_total->setText(tr("Plan vide. Séance de %1 min.").arg(s->durationMin));
    } else {
        m_total->setText(tr("Total : <b>%1 min</b> sur %2 min (temps libre : %3 min)")
                             .arg(total).arg(s->durationMin).arg(s->durationMin - total));
    }
    updateButtons();
}

void SessionsPage::updateButtons()
{
    const bool hasSession = selectedSession() != nullptr;
    const PlanEntry *item = selectedItem();
    m_addButton->setEnabled(hasSession);
    m_pdfButton->setEnabled(hasSession);
    m_editButton->setEnabled(item != nullptr);
    m_removeButton->setEnabled(item != nullptr);
    m_upButton->setEnabled(item != nullptr && item->position > 1);
    m_downButton->setEnabled(item != nullptr && item->position < int(m_entries.size()));
}

void SessionsPage::addExercises()
{
    const Session *s = selectedSession();
    if (!s)
        return;
    const int sessionId = s->id;

    QString error;
    const QList<Exercise> library = m_exercises.list(false, &error);
    if (!error.isEmpty()) {
        showError(error);
        return;
    }
    if (library.isEmpty()) {
        QMessageBox::information(this, tr("Ajouter des exercices"),
                                 tr("La bibliothèque est vide. Ajoutez d'abord des exercices dans la page Exercices."));
        return;
    }

    ExercisePicker picker(library, this);
    if (picker.exec() != QDialog::Accepted)
        return;

    for (const Exercise &e : picker.selected()) {
        // Durée initiale : celle conseillée par l'exercice, 10 min à défaut (modifiable ensuite).
        if (m_plan.add(sessionId, e.id, e.durationMin > 0 ? e.durationMin : 10, QString(), &error) < 0) {
            showError(error);
            break;
        }
    }
    reloadSessions();   // met aussi à jour la colonne « Plan » de la liste des séances
}

void SessionsPage::editItem()
{
    const PlanEntry *item = selectedItem();
    if (!item)
        return;
    const PlanEntry entry = *item;   // copie : la liste est rechargée plus bas

    PlanItemDialog dlg(entry, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    QString error;
    if (!m_plan.updateItem(entry.itemId, dlg.durationMin(), dlg.notes(), &error)) {
        showError(error);
        return;
    }
    reloadSessions();
}

void SessionsPage::moveItem(int delta)
{
    const PlanEntry *item = selectedItem();
    if (!item)
        return;
    QString error;
    if (!m_plan.move(item->itemId, delta, &error)) {
        showError(error);
        return;
    }
    reloadPlan();   // la sélection suit la ligne déplacée (retrouvée par son id)
}

void SessionsPage::removeItem()
{
    const PlanEntry *item = selectedItem();
    if (!item)
        return;
    QString error;
    if (!m_plan.remove(item->itemId, &error)) {
        showError(error);
        return;
    }
    reloadSessions();
}

void SessionsPage::exportPdf()
{
    const Session *s = selectedSession();
    if (!s)
        return;
    const Session session = *s;
    const Team team = currentTeam();

    QString error;
    const QList<PlanEntry> plan = m_plan.entries(session.id, &error);
    if (!error.isEmpty()) {
        showError(error);
        return;
    }

    const QString path = QFileDialog::getSaveFileName(this, tr("Exporter la fiche de séance"),
                                                      SessionSheet::suggestedFileName(team, session),
                                                      tr("Document PDF (*.pdf)"));
    if (path.isEmpty())
        return;

    // Schémas tactiques liés à la séance ou à l'un de ses exercices, rendus en images pour le PDF.
    QList<SessionSheet::SheetBoard> sheetBoards;
    QMap<QString, QImage> images;
    int unreadable = 0;
    const QList<BoardRecord> records = m_boards.forSession(session.id, &error);
    if (!error.isEmpty()) {
        showError(error);
        return;
    }
    QSet<int> planExercises;
    for (const PlanEntry &p : plan)
        planExercises.insert(p.exercise.id);
    for (const BoardRecord &record : records) {
        Tactics::Board board;
        if (!Tactics::fromJson(record.sceneJson, &board)) {
            ++unreadable;
            continue;
        }
        BoardScene scene;
        scene.setBoard(board);
        SessionSheet::SheetBoard entry;
        entry.title = record.title;
        entry.imageName = QStringLiteral("board_%1").arg(record.id);
        // Sous l'exercice s'il fait partie du plan ; sinon dans la section des schémas de la séance.
        entry.exerciseId = planExercises.contains(record.exerciseId) ? record.exerciseId : -1;
        sheetBoards.append(entry);
        images.insert(entry.imageName, scene.renderImage(1000));
    }

    if (!PdfExport::writeHtmlToPdf(path, SessionSheet::buildHtml(team, session, plan, sheetBoards), &error, images)) {
        showError(error);
        return;
    }
    if (unreadable > 0)
        QMessageBox::warning(this, tr("Exporter la fiche de séance"),
                             tr("%n schéma(s) illisible(s) n'ont pas été inclus dans la fiche.", nullptr, unreadable));
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        QMessageBox::information(this, tr("Exporter la fiche de séance"), tr("PDF enregistré :\n%1").arg(path));
}

void SessionsPage::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Erreur"), message);
}
