#include "CalendarPage.h"

#include "ui/AttendanceDialog.h"
#include "ui/Labels.h"
#include "ui/RecurrenceDialog.h"
#include "ui/SessionDialog.h"

#include <QBrush>
#include <QCalendarWidget>
#include <QColor>
#include <QComboBox>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QTextCharFormat>
#include <QVBoxLayout>

namespace {

// Même clé que la page Effectif : l'équipe choisie est partagée entre les deux pages.
const QString kLastTeamKey = QStringLiteral("squad/lastTeamId");

enum Scope { ScopeDay, ScopeWeek, ScopeMonth };

QString timeRange(const Session &s)
{
    return QStringLiteral("%1 – %2").arg(s.startAt.time().toString(QStringLiteral("HH:mm")),
                                         s.endAt().time().toString(QStringLiteral("HH:mm")));
}

} // namespace

CalendarPage::CalendarPage(const QSqlDatabase &db, QWidget *parent)
    : QWidget(parent)
    , m_teams(db)
    , m_sessions(db)
    , m_players(db)
    , m_attendance(db)
{
    m_teamCombo = new QComboBox;
    m_teamCombo->setMinimumWidth(220);

    m_calendar = new QCalendarWidget;
    m_calendar->setLocale(QLocale(QLocale::French));
    m_calendar->setFirstDayOfWeek(Qt::Monday);
    m_calendar->setGridVisible(true);
    m_calendar->setVerticalHeaderFormat(QCalendarWidget::ISOWeekNumbers);
    m_calendar->setMinimumWidth(380);

    m_scope = new QComboBox;
    m_scope->addItem(tr("Jour sélectionné"), ScopeDay);
    m_scope->addItem(tr("Semaine"), ScopeWeek);
    m_scope->addItem(tr("Mois"), ScopeMonth);
    m_scope->setCurrentIndex(ScopeWeek);

    m_table = new QTableWidget(0, 6);
    m_table->setHorizontalHeaderLabels({tr("Date"), tr("Horaire"), tr("Thème"), tr("Lieu"), tr("Statut"), tr("Présents")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    m_add = new QPushButton(tr("Nouvelle séance…"));
    m_recurring = new QPushButton(tr("Séances récurrentes…"));
    m_edit = new QPushButton(tr("Modifier…"));
    m_attendanceButton = new QPushButton(tr("Faire l'appel…"));
    m_statusButton = new QPushButton(tr("Statut"));
    m_delete = new QPushButton(tr("Supprimer…"));
    m_info = new QLabel;

    auto *statusMenu = new QMenu(m_statusButton);
    for (const auto &e : Labels::statuses()) {
        const QString code = e.first;
        statusMenu->addAction(e.second, this, [this, code] { changeStatus(code); });
    }
    m_statusButton->setMenu(statusMenu);

    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel(tr("Équipe :")));
    top->addWidget(m_teamCombo);
    top->addStretch();

    auto *right = new QVBoxLayout;
    auto *scopeRow = new QHBoxLayout;
    scopeRow->addWidget(new QLabel(tr("Afficher :")));
    scopeRow->addWidget(m_scope);
    scopeRow->addStretch();
    right->addLayout(scopeRow);
    right->addWidget(m_table, 1);

    auto *middle = new QHBoxLayout;
    auto *left = new QVBoxLayout;
    left->addWidget(m_calendar);
    left->addStretch();
    middle->addLayout(left);
    middle->addLayout(right, 1);

    auto *bottom = new QHBoxLayout;
    bottom->addWidget(m_add);
    bottom->addWidget(m_recurring);
    bottom->addWidget(m_edit);
    bottom->addWidget(m_attendanceButton);
    bottom->addWidget(m_statusButton);
    bottom->addWidget(m_delete);
    bottom->addStretch();
    bottom->addWidget(m_info);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addLayout(middle, 1);
    layout->addLayout(bottom);

    connect(m_teamCombo, &QComboBox::currentIndexChanged, this, [this] {
        const int id = currentTeamId();
        if (id >= 0)
            QSettings().setValue(kLastTeamKey, id);
        reloadAll();
    });
    connect(m_calendar, &QCalendarWidget::selectionChanged, this, &CalendarPage::reloadList);
    connect(m_calendar, &QCalendarWidget::currentPageChanged, this, [this] {
        refreshHighlights();
        reloadList();
    });
    connect(m_scope, &QComboBox::currentIndexChanged, this, &CalendarPage::reloadList);
    connect(m_table, &QTableWidget::currentCellChanged, this, &CalendarPage::updateButtons);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &CalendarPage::editSelected);
    connect(m_add, &QPushButton::clicked, this, &CalendarPage::addSession);
    connect(m_recurring, &QPushButton::clicked, this, &CalendarPage::addRecurring);
    connect(m_edit, &QPushButton::clicked, this, &CalendarPage::editSelected);
    connect(m_attendanceButton, &QPushButton::clicked, this, &CalendarPage::takeAttendance);
    connect(m_delete, &QPushButton::clicked, this, &CalendarPage::deleteSelected);

    loadTeams();
}

void CalendarPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Les équipes ont pu être créées ou renommées depuis la page Effectif.
    loadTeams();
}

int CalendarPage::currentTeamId() const
{
    return m_teamCombo->currentIndex() >= 0 ? m_teamCombo->currentData().toInt() : -1;
}

const Session *CalendarPage::selectedSession() const
{
    const int row = m_table->currentRow();
    return (row >= 0 && row < m_listed.size()) ? &m_listed.at(row) : nullptr;
}

void CalendarPage::loadTeams()
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
        QString label = t.name;
        if (!t.category.isEmpty())
            label += QStringLiteral(" (%1)").arg(t.category);
        m_teamCombo->addItem(label, t.id);
        if (t.id == preferred)
            select = i;
    }
    if (!teams.isEmpty())
        m_teamCombo->setCurrentIndex(select);
    m_teamCombo->blockSignals(false);

    reloadAll();
}

void CalendarPage::reloadAll()
{
    refreshHighlights();
    reloadList();
}

void CalendarPage::refreshHighlights()
{
    // Efface tous les formats précédents (QDate() invalide = tout réinitialiser).
    m_calendar->setDateTextFormat(QDate(), QTextCharFormat());

    const int teamId = currentTeamId();
    if (teamId < 0)
        return;

    // Le calendrier affiche aussi quelques jours des mois voisins.
    const QDate first(m_calendar->yearShown(), m_calendar->monthShown(), 1);
    const QSet<QDate> dates = m_sessions.datesWithSessions(teamId, first.addDays(-7), first.addMonths(1).addDays(7));

    QTextCharFormat fmt;
    QFont bold;
    bold.setBold(true);
    fmt.setFont(bold);
    fmt.setBackground(QBrush(QColor(190, 225, 255)));
    fmt.setForeground(QBrush(QColor(10, 40, 90)));
    for (const QDate &d : dates)
        m_calendar->setDateTextFormat(d, fmt);
}

void CalendarPage::visibleRange(QDate *from, QDate *to) const
{
    const QDate day = m_calendar->selectedDate();
    switch (m_scope->currentData().toInt()) {
    case ScopeDay:
        *from = *to = day;
        break;
    case ScopeWeek:
        *from = day.addDays(1 - day.dayOfWeek());   // lundi
        *to = from->addDays(6);
        break;
    default:
        *from = QDate(day.year(), day.month(), 1);
        *to = from->addMonths(1).addDays(-1);
        break;
    }
}

void CalendarPage::reloadList()
{
    const int keepId = selectedSession() ? selectedSession()->id : -1;

    m_listed.clear();
    m_table->setRowCount(0);

    const int teamId = currentTeamId();
    if (teamId >= 0) {
        QDate from, to;
        visibleRange(&from, &to);
        QString error;
        m_listed = m_sessions.listBetween(teamId, from, to, &error);
        if (!error.isEmpty())
            showError(error);
    }

    QList<int> ids;
    for (const Session &s : std::as_const(m_listed))
        ids << s.id;
    const QMap<int, QPair<int, int>> summary = m_attendance.summary(ids);

    const QLocale fr(QLocale::French);
    m_table->setRowCount(int(m_listed.size()));
    for (int row = 0; row < m_listed.size(); ++row) {
        const Session &s = m_listed.at(row);
        const QStringList cells = {
            fr.toString(s.startAt.date(), QStringLiteral("ddd dd/MM/yyyy")),
            timeRange(s),
            s.theme,
            s.location,
            Labels::label(Labels::statuses(), s.status),
            summary.contains(s.id) ? QStringLiteral("%1 / %2").arg(summary.value(s.id).first).arg(summary.value(s.id).second)
                                   : QStringLiteral("—"),
        };
        for (int col = 0; col < cells.size(); ++col) {
            auto *item = new QTableWidgetItem(cells.at(col));
            if (s.status == QLatin1String("cancelled")) {
                QFont f = item->font();
                f.setStrikeOut(true);
                item->setFont(f);
                item->setForeground(QBrush(QColor(Qt::gray)));
            }
            m_table->setItem(row, col, item);
        }
    }
    m_table->resizeColumnToContents(0);
    m_table->resizeColumnToContents(1);

    if (keepId >= 0) {
        for (int row = 0; row < m_listed.size(); ++row) {
            if (m_listed.at(row).id == keepId) {
                m_table->selectRow(row);
                break;
            }
        }
    }
    // Rien de sélectionné (premier affichage, changement de période…) : on sélectionne la première séance,
    // pour que les boutons Modifier / Faire l'appel / Statut / Supprimer soient utilisables tout de suite.
    if (m_table->currentRow() < 0 && !m_listed.isEmpty())
        m_table->selectRow(0);
    updateButtons();
}

void CalendarPage::updateButtons()
{
    const bool hasTeam = currentTeamId() >= 0;
    const bool hasSel = selectedSession() != nullptr;
    m_add->setEnabled(hasTeam);
    m_recurring->setEnabled(hasTeam);
    m_edit->setEnabled(hasSel);
    m_attendanceButton->setEnabled(hasSel);
    m_statusButton->setEnabled(hasSel);
    m_delete->setEnabled(hasSel);

    if (!hasTeam)
        m_info->setText(tr("Créez d'abord une équipe dans la page Effectif."));
    else
        m_info->setText(tr("%n séance(s) affichée(s)", nullptr, int(m_listed.size())));
}

void CalendarPage::addSession()
{
    Session s;
    s.teamId = currentTeamId();
    if (s.teamId < 0)
        return;
    s.startAt = QDateTime(m_calendar->selectedDate(), QTime(18, 30));

    SessionDialog dlg(s, m_sessions, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    Session created = dlg.session();
    QString error;
    if (!m_sessions.insert(created, &error)) {
        showError(error);
        return;
    }
    m_calendar->setSelectedDate(created.startAt.date());
    reloadAll();
}

void CalendarPage::addRecurring()
{
    const int teamId = currentTeamId();
    if (teamId < 0)
        return;

    RecurrenceDialog dlg(teamId, m_calendar->selectedDate(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QList<Session> toCreate = dlg.sessions();
    int clashes = 0;
    for (const Session &s : std::as_const(toCreate)) {
        if (!m_sessions.conflicts(teamId, s.startAt, s.durationMin).isEmpty())
            ++clashes;
    }
    if (clashes > 0) {
        const auto answer = QMessageBox::warning(
            this, tr("Séances récurrentes"),
            tr("%n séance(s) sur %1 chevauchent une séance existante de l'équipe.\n\n"
               "Créer quand même toutes les séances ?",
               nullptr, clashes)
                .arg(toCreate.size()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }

    QString error;
    if (!m_sessions.insertMany(toCreate, &error)) {
        showError(error);
        return;
    }
    if (!toCreate.isEmpty())
        m_calendar->setSelectedDate(toCreate.first().startAt.date());
    reloadAll();
    QMessageBox::information(this, tr("Séances récurrentes"),
                             tr("%n séance(s) créée(s).", nullptr, int(toCreate.size())));
}

void CalendarPage::editSelected()
{
    const Session *current = selectedSession();
    if (!current)
        return;

    SessionDialog dlg(*current, m_sessions, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const Session edited = dlg.session();
    QString error;
    if (!m_sessions.update(edited, &error)) {
        showError(error);
        return;
    }
    m_calendar->setSelectedDate(edited.startAt.date());
    reloadAll();
}

void CalendarPage::takeAttendance()
{
    const Session *s = selectedSession();
    if (!s)
        return;
    const Session session = *s;   // copie : la liste est rechargée plus bas

    if (session.status == QLatin1String("cancelled")) {
        QMessageBox::information(this, tr("Faire l'appel"),
                                 tr("Cette séance est annulée : on ne peut pas faire l'appel. "
                                    "Changez d'abord son statut si elle a bien eu lieu."));
        return;
    }

    QString error;
    const QMap<int, AttendanceRepository::Entry> existing = m_attendance.load(session.id, &error);
    if (!error.isEmpty()) {
        showError(error);
        return;
    }

    // Joueurs actifs, plus les inactifs déjà présents dans l'appel (pour ne pas effacer leur historique).
    QList<Player> players;
    for (const Player &p : m_players.list(session.teamId, true, &error)) {
        if (p.active || existing.contains(p.id))
            players.append(p);
    }
    if (!error.isEmpty()) {
        showError(error);
        return;
    }
    if (players.isEmpty()) {
        QMessageBox::information(this, tr("Faire l'appel"),
                                 tr("Cette équipe n'a aucun joueur actif : ajoutez des joueurs dans la page Effectif."));
        return;
    }

    AttendanceDialog dlg(session, players, existing, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    if (!m_attendance.save(session.id, dlg.entries(), &error)) {
        showError(error);
        return;
    }
    if (dlg.markAsDone() && !m_sessions.setStatus(session.id, QStringLiteral("done"), &error)) {
        showError(error);
        return;
    }
    reloadAll();
}

void CalendarPage::changeStatus(const QString &status)
{
    const Session *s = selectedSession();
    if (!s || s->status == status)
        return;

    // Sortir une séance de l'état « annulée » peut la faire chevaucher une autre séance.
    if (status != QLatin1String("cancelled")) {
        const QList<Session> clash = m_sessions.conflicts(s->teamId, s->startAt, s->durationMin, s->id);
        if (!clash.isEmpty()) {
            const auto answer = QMessageBox::warning(
                this, tr("Changer le statut"),
                tr("Cette séance chevauche une autre séance de l'équipe (%1 à %2).\n\nChanger le statut quand même ?")
                    .arg(clash.first().startAt.date().toString(QStringLiteral("dd/MM/yyyy")),
                         clash.first().startAt.time().toString(QStringLiteral("HH:mm"))),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes)
                return;
        }
    }

    QString error;
    if (!m_sessions.setStatus(s->id, status, &error)) {
        showError(error);
        return;
    }
    reloadAll();
}

void CalendarPage::deleteSelected()
{
    const Session *s = selectedSession();
    if (!s)
        return;

    const auto answer = QMessageBox::warning(
        this, tr("Supprimer la séance"),
        tr("Supprimer définitivement la séance du %1 à %2 ?\n\n"
           "Ses présences et son plan d'exercices seront aussi effacés. "
           "Pour la conserver dans l'historique, utilisez plutôt le statut « Annulée ».")
            .arg(s->startAt.date().toString(QStringLiteral("dd/MM/yyyy")),
                 s->startAt.time().toString(QStringLiteral("HH:mm"))),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes)
        return;

    QString error;
    if (!m_sessions.remove(s->id, &error)) {
        showError(error);
        return;
    }
    reloadAll();
}

void CalendarPage::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Erreur"), message);
}
