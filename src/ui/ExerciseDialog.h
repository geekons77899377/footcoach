#pragma once

#include "core/Models.h"

#include <QDialog>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;

// Formulaire d'un exercice de la bibliothèque.
class ExerciseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExerciseDialog(const Exercise &initial, QWidget *parent = nullptr);

    Exercise exercise() const;
    void accept() override;

private:
    Exercise m_exercise;
    QLineEdit *m_title = nullptr;
    QComboBox *m_category = nullptr;
    QLineEdit *m_objective = nullptr;
    QSpinBox *m_duration = nullptr;
    QSpinBox *m_playersMin = nullptr;
    QSpinBox *m_playersMax = nullptr;
    QLineEdit *m_equipment = nullptr;
    QPlainTextEdit *m_description = nullptr;
};
