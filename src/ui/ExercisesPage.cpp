#include "ExercisesPage.h"

#include "ui/ExerciseDialog.h"
#include "ui/Labels.h"

#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

QString playersText(const Exercise &e)
{
    if (e.playersMin > 0 && e.playersMax > 0)
        return e.playersMin == e.playersMax ? QString::number(e.playersMin)
                                            : QStringLiteral("%1–%2").arg(e.playersMin).arg(e.playersMax);
    if (e.playersMin > 0)
        return QStringLiteral("%1+").arg(e.playersMin);
    if (e.playersMax > 0)
        return QStringLiteral("≤ %1").arg(e.playersMax);
    return QStringLiteral("—");
}

} // namespace

ExercisesPage::ExercisesPage(const QSqlDatabase &db, QWidget *parent)
    : QWidget(parent)
    , m_repo(db)
{
    m_search = new QLineEdit;
    m_search->setPlaceholderText(tr("Rechercher dans le titre, l'objectif, le matériel, la description…"));
    m_search->setClearButtonEnabled(true);

    m_category = new QComboBox;
    m_category->addItem(tr("Toutes les catégories"), QString());
    for (const auto &e : Labels::categories())
        m_category->addItem(e.second, e.first);

    m_showArchived = new QCheckBox(tr("Afficher les exercices archivés"));

    m_table = new QTableWidget(0, 5);
    m_table->setHorizontalHeaderLabels({tr("Exercice"), tr("Catégorie"), tr("Durée"), tr("Joueurs"), tr("Matériel")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    m_details = new QPlainTextEdit;
    m_details->setReadOnly(true);
    m_details->setMaximumHeight(150);

    m_add = new QPushButton(tr("Ajouter…"));
    m_edit = new QPushButton(tr("Modifier…"));
    m_duplicate = new QPushButton(tr("Dupliquer"));
    m_archive = new QPushButton(tr("Archiver"));
    m_delete = new QPushButton(tr("Supprimer…"));
    m_count = new QLabel;

    auto *filters = new QHBoxLayout;
    filters->addWidget(m_search, 1);
    filters->addWidget(m_category);
    filters->addWidget(m_showArchived);

    auto *bottom = new QHBoxLayout;
    bottom->addWidget(m_add);
    bottom->addWidget(m_edit);
    bottom->addWidget(m_duplicate);
    bottom->addWidget(m_archive);
    bottom->addWidget(m_delete);
    bottom->addStretch();
    bottom->addWidget(m_count);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(filters);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_details);
    layout->addLayout(bottom);

    connect(m_search, &QLineEdit::textChanged, this, &ExercisesPage::applyFilters);
    connect(m_category, &QComboBox::currentIndexChanged, this, &ExercisesPage::applyFilters);
    connect(m_showArchived, &QCheckBox::toggled, this, &ExercisesPage::reload);
    connect(m_table, &QTableWidget::currentCellChanged, this, [this] {
        showDetails();
        updateButtons();
    });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &ExercisesPage::editSelected);
    connect(m_add, &QPushButton::clicked, this, &ExercisesPage::addExercise);
    connect(m_edit, &QPushButton::clicked, this, &ExercisesPage::editSelected);
    connect(m_duplicate, &QPushButton::clicked, this, &ExercisesPage::duplicateSelected);
    connect(m_archive, &QPushButton::clicked, this, &ExercisesPage::toggleArchiveSelected);
    connect(m_delete, &QPushButton::clicked, this, &ExercisesPage::deleteSelected);

    reload();
}

const Exercise *ExercisesPage::selectedExercise() const
{
    const int row = m_table->currentRow();
    return (row >= 0 && row < m_shown.size()) ? &m_shown.at(row) : nullptr;
}

void ExercisesPage::reload()
{
    QString error;
    m_all = m_repo.list(m_showArchived->isChecked(), &error);
    if (!error.isEmpty())
        showError(error);
    applyFilters();
}

void ExercisesPage::applyFilters()
{
    const int keepId = selectedExercise() ? selectedExercise()->id : -1;
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
        QString title = e.title;
        if (e.archived)
            title += tr(" (archivé)");
        auto *dur = new QTableWidgetItem(e.durationMin > 0 ? tr("%1 min").arg(e.durationMin) : QStringLiteral("—"));
        dur->setTextAlignment(Qt::AlignCenter);
        auto *players = new QTableWidgetItem(playersText(e));
        players->setTextAlignment(Qt::AlignCenter);
        const QList<QTableWidgetItem *> items = {
            new QTableWidgetItem(title),
            new QTableWidgetItem(Labels::label(Labels::categories(), e.category)),
            dur,
            players,
            new QTableWidgetItem(e.equipment),
        };
        for (int col = 0; col < items.size(); ++col) {
            if (e.archived)
                items.at(col)->setForeground(QBrush(QColor(Qt::gray)));
            m_table->setItem(row, col, items.at(col));
        }
    }
    m_table->resizeColumnToContents(1);
    m_table->resizeColumnToContents(2);
    m_table->resizeColumnToContents(3);

    int select = m_shown.isEmpty() ? -1 : 0;
    for (int row = 0; row < m_shown.size(); ++row) {
        if (m_shown.at(row).id == keepId)
            select = row;
    }
    if (select >= 0)
        m_table->selectRow(select);

    m_count->setText(tr("%n exercice(s)", nullptr, int(m_shown.size())));
    showDetails();
    updateButtons();
}

void ExercisesPage::showDetails()
{
    const Exercise *e = selectedExercise();
    if (!e) {
        m_details->clear();
        return;
    }
    QStringList lines;
    if (!e->objective.isEmpty())
        lines << tr("Objectif : %1").arg(e->objective);
    if (!e->description.isEmpty())
        lines << e->description;
    if (e->durationMin > 0 || e->playersMin > 0 || e->playersMax > 0)
        lines << tr("Durée conseillée : %1   |   Joueurs : %2")
                     .arg(e->durationMin > 0 ? tr("%1 min").arg(e->durationMin) : QStringLiteral("—"), playersText(*e));
    if (!e->equipment.isEmpty())
        lines << tr("Matériel : %1").arg(e->equipment);
    m_details->setPlainText(lines.join(QStringLiteral("\n\n")));
}

void ExercisesPage::updateButtons()
{
    const Exercise *e = selectedExercise();
    m_edit->setEnabled(e != nullptr);
    m_duplicate->setEnabled(e != nullptr);
    m_archive->setEnabled(e != nullptr);
    m_archive->setText(e && e->archived ? tr("Restaurer") : tr("Archiver"));
    m_delete->setEnabled(e != nullptr);
}

void ExercisesPage::addExercise()
{
    ExerciseDialog dlg(Exercise{}, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    Exercise created = dlg.exercise();
    QString error;
    if (!m_repo.insert(created, &error)) {
        showError(error);
        return;
    }
    reload();
}

void ExercisesPage::editSelected()
{
    const Exercise *current = selectedExercise();
    if (!current)
        return;
    ExerciseDialog dlg(*current, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    QString error;
    if (!m_repo.update(dlg.exercise(), &error)) {
        showError(error);
        return;
    }
    reload();
}

void ExercisesPage::duplicateSelected()
{
    const Exercise *current = selectedExercise();
    if (!current)
        return;
    Exercise copy = *current;
    copy.id = -1;
    copy.archived = false;
    copy.title = tr("%1 (copie)").arg(current->title);
    QString error;
    if (!m_repo.insert(copy, &error)) {
        showError(error);
        return;
    }
    reload();
}

void ExercisesPage::toggleArchiveSelected()
{
    const Exercise *e = selectedExercise();
    if (!e)
        return;
    QString error;
    if (!m_repo.setArchived(e->id, !e->archived, &error)) {
        showError(error);
        return;
    }
    reload();
}

void ExercisesPage::deleteSelected()
{
    const Exercise *e = selectedExercise();
    if (!e)
        return;
    const Exercise exercise = *e;   // copie : la liste est rechargée plus bas

    const int used = m_repo.usageCount(exercise.id);
    if (used > 0) {
        // Une clé étrangère interdit la suppression : on propose l'archivage à la place.
        if (exercise.archived) {
            QMessageBox::information(this, tr("Supprimer l'exercice"),
                                     tr("« %1 » est utilisé dans %n plan(s) de séance : il ne peut pas être supprimé. "
                                        "Il est déjà archivé, donc masqué de la bibliothèque.",
                                        nullptr, used)
                                         .arg(exercise.title));
            return;
        }
        const auto answer = QMessageBox::question(
            this, tr("Supprimer l'exercice"),
            tr("« %1 » est utilisé dans %n plan(s) de séance : il ne peut pas être supprimé.\n\n"
               "Voulez-vous l'archiver ? Il disparaîtra de la bibliothèque mais restera visible dans les plans existants.",
               nullptr, used)
                .arg(exercise.title),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);
        if (answer != QMessageBox::Yes)
            return;
        QString error;
        if (!m_repo.setArchived(exercise.id, true, &error))
            showError(error);
        reload();
        return;
    }

    const auto answer = QMessageBox::warning(this, tr("Supprimer l'exercice"),
                                             tr("Supprimer définitivement « %1 » ? Cette action est irréversible.")
                                                 .arg(exercise.title),
                                             QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes)
        return;
    QString error;
    if (!m_repo.remove(exercise.id, &error)) {
        showError(error);
        return;
    }
    reload();
}

void ExercisesPage::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Erreur"), message);
}
