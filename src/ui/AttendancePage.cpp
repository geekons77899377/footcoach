#include "AttendancePage.h"

#include "core/AttendanceStats.h"

#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

// Même clé que les pages Effectif et Calendrier.
const QString kLastTeamKey = QStringLiteral("squad/lastTeamId");

enum Period { PeriodAll, Period30Days, Period3Months, Period12Months };

QTableWidgetItem *centered(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    return item;
}

struct StatusStyle
{
    const char *letter;
    QColor color;
};

StatusStyle styleFor(const QString &status)
{
    if (status == QLatin1String("present"))
        return {"P", QColor(30, 130, 60)};
    if (status == QLatin1String("absent"))
        return {"A", QColor(190, 40, 40)};
    if (status == QLatin1String("excused"))
        return {"E", QColor(200, 120, 0)};
    if (status == QLatin1String("injured"))
        return {"B", QColor(130, 60, 160)};
    return {"", QColor(Qt::gray)};
}

QColor rateColor(double rate)
{
    if (rate >= 0.8)
        return QColor(30, 130, 60);
    if (rate >= 0.6)
        return QColor(200, 120, 0);
    return QColor(190, 40, 40);
}

} // namespace

AttendancePage::AttendancePage(const QSqlDatabase &db, QWidget *parent)
    : QWidget(parent)
    , m_teams(db)
    , m_attendance(db)
{
    m_teamCombo = new QComboBox;
    m_teamCombo->setMinimumWidth(220);

    m_period = new QComboBox;
    m_period->addItem(tr("Tout l'historique"), PeriodAll);
    m_period->addItem(tr("30 derniers jours"), Period30Days);
    m_period->addItem(tr("3 derniers mois"), Period3Months);
    m_period->addItem(tr("12 derniers mois"), Period12Months);

    m_summary = new QTableWidget(0, 8);
    m_summary->setHorizontalHeaderLabels({tr("N°"), tr("Joueur"), tr("Séances"), tr("Présent"), tr("Absent"),
                                          tr("Excusé"), tr("Blessé"), tr("Taux")});
    m_summary->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_summary->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_summary->setAlternatingRowColors(true);
    m_summary->verticalHeader()->hide();
    m_summary->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    m_grid = new QTableWidget;
    m_grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_grid->setSelectionMode(QAbstractItemView::NoSelection);

    auto *tabs = new QTabWidget;
    tabs->addTab(m_summary, tr("Synthèse"));
    tabs->addTab(m_grid, tr("Grille joueurs × séances"));

    m_info = new QLabel;
    m_info->setWordWrap(true);

    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel(tr("Équipe :")));
    top->addWidget(m_teamCombo);
    top->addWidget(new QLabel(tr("Période :")));
    top->addWidget(m_period);
    top->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(tabs, 1);
    layout->addWidget(m_info);

    connect(m_teamCombo, &QComboBox::currentIndexChanged, this, [this] {
        const int id = currentTeamId();
        if (id >= 0)
            QSettings().setValue(kLastTeamKey, id);
        refresh();
    });
    connect(m_period, &QComboBox::currentIndexChanged, this, &AttendancePage::refresh);

    loadTeams();
}

void AttendancePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Équipes, joueurs ou appels ont pu changer depuis les autres pages.
    loadTeams();
}

int AttendancePage::currentTeamId() const
{
    return m_teamCombo->currentIndex() >= 0 ? m_teamCombo->currentData().toInt() : -1;
}

void AttendancePage::loadTeams()
{
    int preferred = currentTeamId();
    if (preferred < 0)
        preferred = QSettings().value(kLastTeamKey, -1).toInt();

    QString error;
    const QList<Team> teams = m_teams.all(&error);
    if (!error.isEmpty())
        QMessageBox::critical(this, tr("Erreur"), error);

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

    refresh();
}

void AttendancePage::periodRange(QDate *from, QDate *to) const
{
    const QDate today = QDate::currentDate();
    *to = today;
    switch (m_period->currentData().toInt()) {
    case Period30Days:   *from = today.addDays(-30); break;
    case Period3Months:  *from = today.addMonths(-3); break;
    case Period12Months: *from = today.addYears(-1); break;
    default:             *from = QDate(2000, 1, 1); break;
    }
}

void AttendancePage::refresh()
{
    m_summary->setRowCount(0);
    m_grid->clear();
    m_grid->setRowCount(0);
    m_grid->setColumnCount(0);

    const int teamId = currentTeamId();
    if (teamId < 0) {
        m_info->setText(tr("Créez d'abord une équipe dans la page Effectif."));
        return;
    }

    QDate from, to;
    periodRange(&from, &to);
    QString error;
    const AttendanceStats::Data data = m_attendance.loadData(teamId, from, to, &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, tr("Erreur"), error);
        return;
    }
    const QList<AttendanceStats::PlayerStats> stats = AttendanceStats::compute(data);

    // Joueurs affichés : les actifs, plus les inactifs qui ont au moins une séance comptée.
    QList<int> shown;
    for (int i = 0; i < data.players.size(); ++i) {
        if (data.players.at(i).active || stats.at(i).counted() > 0)
            shown << i;
    }

    // --- Synthèse : classée par taux décroissant (joueurs sans taux à la fin) ---
    std::stable_sort(shown.begin(), shown.end(), [&](int a, int b) {
        return stats.at(a).rate() > stats.at(b).rate();
    });

    m_summary->setRowCount(int(shown.size()));
    for (int row = 0; row < shown.size(); ++row) {
        const auto &p = data.players.at(shown.at(row));
        const auto &st = stats.at(shown.at(row));
        QString name = p.fullName();
        if (!p.active)
            name += tr(" (inactif)");
        m_summary->setItem(row, 0, centered(p.jerseyNumber > 0 ? QString::number(p.jerseyNumber) : QStringLiteral("—")));
        m_summary->setItem(row, 1, new QTableWidgetItem(name));
        m_summary->setItem(row, 2, centered(QString::number(st.counted())));
        m_summary->setItem(row, 3, centered(QString::number(st.present)));
        m_summary->setItem(row, 4, centered(QString::number(st.absent)));
        m_summary->setItem(row, 5, centered(QString::number(st.excused)));
        m_summary->setItem(row, 6, centered(QString::number(st.injured)));

        auto *rate = centered(st.rate() < 0 ? QStringLiteral("—")
                                            : QStringLiteral("%1 %").arg(qRound(st.rate() * 100)));
        if (st.rate() >= 0) {
            QFont f = rate->font();
            f.setBold(true);
            rate->setFont(f);
            rate->setForeground(QBrush(rateColor(st.rate())));
        }
        m_summary->setItem(row, 7, rate);
    }

    // --- Grille : une colonne par séance réalisée ---
    // Les joueurs sont dans l'ordre alphabétique (ordre de la base), pas celui du classement.
    QList<int> gridPlayers;
    for (int i = 0; i < data.players.size(); ++i) {
        if (shown.contains(i))
            gridPlayers << i;
    }
    m_grid->setColumnCount(int(data.sessions.size()));
    m_grid->setRowCount(int(gridPlayers.size()));
    QStringList colHeaders;
    for (const auto &s : data.sessions)
        colHeaders << s.date.toString(QStringLiteral("dd/MM"));
    m_grid->setHorizontalHeaderLabels(colHeaders);

    QStringList rowHeaders;
    for (int r = 0; r < gridPlayers.size(); ++r) {
        const auto &p = data.players.at(gridPlayers.at(r));
        rowHeaders << p.fullName();
        for (int c = 0; c < data.sessions.size(); ++c) {
            const QString status = AttendanceStats::cellStatus(data, p, data.sessions.at(c));
            const StatusStyle style = styleFor(status);
            auto *item = centered(QString::fromLatin1(style.letter));
            item->setForeground(QBrush(style.color));
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
            m_grid->setItem(r, c, item);
        }
    }
    m_grid->setVerticalHeaderLabels(rowHeaders);
    for (int c = 0; c < m_grid->columnCount(); ++c)
        m_grid->setColumnWidth(c, 52);

    if (data.sessions.isEmpty()) {
        m_info->setText(tr("Aucune séance réalisée sur la période. Les séances comptent une fois leur statut passé à « Réalisée » "
                           "(automatique quand vous enregistrez l'appel)."));
    } else {
        m_info->setText(tr("%n séance(s) réalisée(s). Taux = présents ÷ (présents + absents) : les excusés et les blessés "
                           "ne comptent pas. Un joueur sans appel enregistré compte comme absent, à partir de son arrivée "
                           "dans l'effectif. Grille : P présent, A absent, E excusé, B blessé.",
                           nullptr, int(data.sessions.size())));
    }
}
