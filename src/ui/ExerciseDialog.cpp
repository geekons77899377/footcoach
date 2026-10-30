#include "ExerciseDialog.h"

#include "ui/Labels.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

QSpinBox *optionalSpin(int max, int value, const QString &suffix = QString())
{
    auto *spin = new QSpinBox;
    spin->setRange(0, max);
    spin->setSpecialValueText(QStringLiteral("—"));   // 0 = non précisé
    spin->setSuffix(suffix);
    spin->setValue(value);
    return spin;
}

} // namespace

ExerciseDialog::ExerciseDialog(const Exercise &initial, QWidget *parent)
    : QDialog(parent)
    , m_exercise(initial)
{
    setWindowTitle(initial.id < 0 ? tr("Nouvel exercice") : tr("Modifier l'exercice"));
    setModal(true);
    resize(520, 480);

    m_title = new QLineEdit(initial.title);

    m_category = new QComboBox;
    for (const auto &e : Labels::categories())
        m_category->addItem(e.second, e.first);
    m_category->setCurrentIndex(qMax(0, m_category->findData(initial.category)));

    m_objective = new QLineEdit(initial.objective);
    m_objective->setPlaceholderText(tr("ex. Améliorer la qualité de passe sous pression"));

    m_duration = optionalSpin(300, initial.durationMin, tr(" min"));
    m_playersMin = optionalSpin(30, initial.playersMin);
    m_playersMax = optionalSpin(30, initial.playersMax);

    auto *players = new QHBoxLayout;
    players->addWidget(new QLabel(tr("de")));
    players->addWidget(m_playersMin);
    players->addWidget(new QLabel(tr("à")));
    players->addWidget(m_playersMax);
    players->addStretch();

    m_equipment = new QLineEdit(initial.equipment);
    m_equipment->setPlaceholderText(tr("ex. Ballons, plots, chasubles (séparés par des virgules)"));

    m_description = new QPlainTextEdit(initial.description);
    m_description->setPlaceholderText(tr("Organisation, consignes, variantes…"));

    auto *form = new QFormLayout;
    form->addRow(tr("Titre *"), m_title);
    form->addRow(tr("Catégorie *"), m_category);
    form->addRow(tr("Objectif"), m_objective);
    form->addRow(tr("Durée conseillée"), m_duration);
    form->addRow(tr("Joueurs"), players);
    form->addRow(tr("Matériel"), m_equipment);
    form->addRow(tr("Description"), m_description);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &ExerciseDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ExerciseDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form, 1);
    layout->addWidget(buttons);

    m_title->setFocus();
}

Exercise ExerciseDialog::exercise() const
{
    Exercise e = m_exercise;
    e.title = m_title->text().trimmed();
    e.category = m_category->currentData().toString();
    e.objective = m_objective->text().trimmed();
    e.durationMin = m_duration->value();
    e.playersMin = m_playersMin->value();
    e.playersMax = m_playersMax->value();
    e.equipment = m_equipment->text().trimmed();
    e.description = m_description->toPlainText().trimmed();
    return e;
}

void ExerciseDialog::accept()
{
    if (m_title->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Le titre est obligatoire."));
        m_title->setFocus();
        return;
    }
    if (m_playersMin->value() > 0 && m_playersMax->value() > 0 && m_playersMax->value() < m_playersMin->value()) {
        QMessageBox::warning(this, windowTitle(), tr("Le nombre maximum de joueurs est inférieur au minimum."));
        return;
    }
    QDialog::accept();
}
