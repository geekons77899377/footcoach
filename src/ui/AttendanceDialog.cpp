#include "AttendanceDialog.h"

#include "ui/Labels.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

enum Column { ColNumber, ColName, ColStatus, ColNote, ColumnCount };

} // namespace

AttendanceDialog::AttendanceDialog(const Session &session, const QList<Player> &players,
                                   const QMap<int, AttendanceRepository::Entry> &existing, QWidget *parent)
    : QDialog(parent)
    , m_players(players)
{
    setWindowTitle(tr("Appel — %1 à %2")
                       .arg(session.startAt.date().toString(QStringLiteral("dd/MM/yyyy")),
                            session.startAt.time().toString(QStringLiteral("HH:mm"))));
    setModal(true);
    resize(640, 560);

    m_table = new QTableWidget(int(players.size()), ColumnCount);
    m_table->setHorizontalHeaderLabels({tr("N°"), tr("Joueur"), tr("Statut"), tr("Note")});
    m_table->verticalHeader()->hide();
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->horizontalHeader()->setSectionResizeMode(ColName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColNote, QHeaderView::Stretch);

    for (int row = 0; row < players.size(); ++row) {
        const Player &p = players.at(row);

        auto *number = new QTableWidgetItem(p.jerseyNumber > 0 ? QString::number(p.jerseyNumber) : QStringLiteral("—"));
        number->setTextAlignment(Qt::AlignCenter);
        number->setFlags(number->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, ColNumber, number);

        QString name = QStringLiteral("%1 %2").arg(p.lastName, p.firstName);
        if (!p.active)
            name += tr(" (inactif)");
        auto *nameItem = new QTableWidgetItem(name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, ColName, nameItem);

        auto *combo = new QComboBox;
        combo->addItem(tr("Présent"), QStringLiteral("present"));
        combo->addItem(tr("Absent"), QStringLiteral("absent"));
        combo->addItem(tr("Excusé"), QStringLiteral("excused"));
        combo->addItem(tr("Blessé"), QStringLiteral("injured"));

        const auto it = existing.constFind(p.id);
        const QString status = (it != existing.constEnd()) ? it->status : QStringLiteral("absent");
        combo->setCurrentIndex(qMax(0, combo->findData(status)));
        m_table->setCellWidget(row, ColStatus, combo);
        connect(combo, &QComboBox::currentIndexChanged, this, &AttendanceDialog::updateSummary);

        m_table->setItem(row, ColNote, new QTableWidgetItem(it != existing.constEnd() ? it->note : QString()));
    }
    m_table->resizeColumnToContents(ColNumber);
    m_table->setColumnWidth(ColStatus, 120);

    auto *allPresent = new QPushButton(tr("Tous présents"));
    auto *allAbsent = new QPushButton(tr("Tous absents"));
    m_summary = new QLabel;

    auto *quick = new QHBoxLayout;
    quick->addWidget(allPresent);
    quick->addWidget(allAbsent);
    quick->addStretch();
    quick->addWidget(m_summary);

    m_markDone = new QCheckBox(tr("Marquer la séance comme réalisée"));
    m_markDone->setChecked(session.status == QLatin1String("planned"));
    m_markDone->setVisible(session.status == QLatin1String("planned"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(allPresent, &QPushButton::clicked, this, [this] { setAll(QStringLiteral("present")); });
    connect(allAbsent, &QPushButton::clicked, this, [this] { setAll(QStringLiteral("absent")); });

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(quick);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_markDone);
    layout->addWidget(buttons);

    updateSummary();
}

QComboBox *AttendanceDialog::statusCombo(int row) const
{
    return qobject_cast<QComboBox *>(m_table->cellWidget(row, ColStatus));
}

void AttendanceDialog::setAll(const QString &status)
{
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (QComboBox *combo = statusCombo(row))
            combo->setCurrentIndex(combo->findData(status));
    }
}

void AttendanceDialog::updateSummary()
{
    int present = 0;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        const QComboBox *combo = statusCombo(row);
        if (combo && combo->currentData().toString() == QLatin1String("present"))
            ++present;
    }
    m_summary->setText(tr("%1 présent(s) sur %2").arg(present).arg(m_table->rowCount()));
}

QList<AttendanceRepository::Entry> AttendanceDialog::entries() const
{
    QList<AttendanceRepository::Entry> result;
    for (int row = 0; row < m_players.size(); ++row) {
        AttendanceRepository::Entry e;
        e.playerId = m_players.at(row).id;
        if (const QComboBox *combo = statusCombo(row))
            e.status = combo->currentData().toString();
        if (const QTableWidgetItem *note = m_table->item(row, ColNote))
            e.note = note->text();
        result.append(e);
    }
    return result;
}

bool AttendanceDialog::markAsDone() const
{
    // isHidden() et non isVisible() : le résultat ne doit pas dépendre de l'affichage effectif de la fenêtre.
    return !m_markDone->isHidden() && m_markDone->isChecked();
}
