#pragma once

#include "core/Models.h"

#include <QDialog>
#include <QList>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QTableWidget;

// Choix d'un ou plusieurs exercices de la bibliothèque (hors archivés) à ajouter à un plan.
class ExercisePicker : public QDialog
{
    Q_OBJECT

public:
    explicit ExercisePicker(const QList<Exercise> &exercises, QWidget *parent = nullptr);

    QList<Exercise> selected() const;

private:
    void applyFilters();
    void showDetails();

    QList<Exercise> m_all;
    QList<Exercise> m_shown;
    QLineEdit *m_search = nullptr;
    QComboBox *m_category = nullptr;
    QTableWidget *m_table = nullptr;
    QPlainTextEdit *m_details = nullptr;
};
