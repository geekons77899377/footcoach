#include "ExercisePicker.h"

#include "ui/Labels.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

ExercisePicker::ExercisePicker(const QList<Exercise> &exercises, QWidget *parent)
    : QDialog(parent)
    , m_all(exercises)
{
    setWindowTitle(tr("Ajouter des exercices au plan"));
    setModal(true);
    resize(700, 560);

    m_search = new QLineEdit;
    m_search->setPlaceholderText(tr("Rechercher dans le titre, l'objectif, le matériel…"));
    m_search->setClearButtonEnabled(true);

    m_category = new QComboBox;
    m_category->addItem(tr("Toutes les catégories"), QString());
    for (const auto &e : Labels::categories())
        m_category->addItem(e.second, e.first);

    m_table = new QTableWidget(0, 3);
    m_table->setHorizontalHeaderLabels({tr("Exercice"), tr("Catégorie"), tr("Durée")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    m_details = new QPlainTextEdit;
    m_details->setReadOnly(true);
    m_details->setMaximumHeight(130);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Ajouter au plan"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *top = new QHBoxLayout;
    top->addWidget(m_search, 1);
    top->addWidget(m_category);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_details);
    layout->addWidget(buttons);

    connect(m_search, &QLineEdit::textChanged, this, &ExercisePicker::applyFilters);
    connect(m_category, &QComboBox::currentIndexChanged, this, &ExercisePicker::applyFilters);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ExercisePicker::showDetails);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &QDialog::accept);

    applyFilters();
}

void ExercisePicker::applyFilters()
{
    const QString text = m_search->text().trimmed();
    const QString category = m_category->currentData().toString();

    m_shown.clear();
    for (const Exercise &e : std::as_const(m_all)) {
        if (!category.isEmpty() && e.category != category)
            continue;
        if (!text.isEmpty() && !e.title.contains(text, Qt::CaseInsensitive)
            && !e.objective.contains(text, Qt::CaseInsensitive) && !e.equipment.contains(text, Qt::CaseInsensitive)
            && !e.description.contains(text, Qt::CaseInsensitive))
            continue;
        m_shown.append(e);
    }

    m_table->setRowCount(int(m_shown.size()));
    for (int row = 0; row < m_shown.size(); ++row) {
        const Exercise &e = m_shown.at(row);
        m_table->setItem(row, 0, new QTableWidgetItem(e.title));
        m_table->setItem(row, 1, new QTableWidgetItem(Labels::label(Labels::categories(), e.category)));
        auto *dur = new QTableWidgetItem(e.durationMin > 0 ? tr("%1 min").arg(e.durationMin) : QStringLiteral("—"));
        dur->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 2, dur);
    }
    showDetails();
}

void ExercisePicker::showDetails()
{
    const QList<QTableWidgetSelectionRange> ranges = m_table->selectedRanges();
    if (ranges.isEmpty() || ranges.first().topRow() >= m_shown.size()) {
        m_details->clear();
        return;
    }
    const Exercise &e = m_shown.at(ranges.first().topRow());
    QStringList lines;
    if (!e.objective.isEmpty())
        lines << tr("Objectif : %1").arg(e.objective);
    if (!e.description.isEmpty())
        lines << e.description;
    if (!e.equipment.isEmpty())
        lines << tr("Matériel : %1").arg(e.equipment);
    m_details->setPlainText(lines.join(QStringLiteral("\n\n")));
}

QList<Exercise> ExercisePicker::selected() const
{
    QList<Exercise> result;
    QList<int> rows;
    for (const QModelIndex &idx : m_table->selectionModel()->selectedRows())
        rows << idx.row();
    std::sort(rows.begin(), rows.end());   // dans l'ordre de la liste, pas dans l'ordre de clic
    for (int row : std::as_const(rows)) {
        if (row >= 0 && row < m_shown.size())
            result.append(m_shown.at(row));
    }
    return result;
}
