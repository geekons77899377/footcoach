#pragma once

#include "core/Models.h"

#include <QDialog>

class QPlainTextEdit;
class QSpinBox;

// Durée et note d'un exercice dans le plan d'une séance.
class PlanItemDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlanItemDialog(const PlanEntry &entry, QWidget *parent = nullptr);

    int durationMin() const;
    QString notes() const;

private:
    QSpinBox *m_duration = nullptr;
    QPlainTextEdit *m_notes = nullptr;
};
