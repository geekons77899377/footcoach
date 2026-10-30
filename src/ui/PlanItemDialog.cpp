#include "PlanItemDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QVBoxLayout>

PlanItemDialog::PlanItemDialog(const PlanEntry &entry, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Exercice du plan"));
    setModal(true);

    auto *title = new QLabel(QStringLiteral("<b>%1</b>").arg(entry.exercise.title.toHtmlEscaped()));

    m_duration = new QSpinBox;
    m_duration->setRange(1, 300);
    m_duration->setSuffix(tr(" min"));
    m_duration->setValue(entry.durationMin > 0 ? entry.durationMin
                                               : (entry.exercise.durationMin > 0 ? entry.exercise.durationMin : 10));

    m_notes = new QPlainTextEdit(entry.notes);
    m_notes->setPlaceholderText(tr("Consignes particulières pour cette séance…"));
    m_notes->setFixedHeight(100);

    auto *form = new QFormLayout;
    form->addRow(tr("Durée dans ce plan"), m_duration);
    form->addRow(tr("Note"), m_notes);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

int PlanItemDialog::durationMin() const
{
    return m_duration->value();
}

QString PlanItemDialog::notes() const
{
    return m_notes->toPlainText().trimmed();
}
