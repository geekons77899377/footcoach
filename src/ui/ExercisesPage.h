#pragma once

#include "core/ExerciseRepository.h"

#include <QSqlDatabase>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;

// Page « Exercices » : bibliothèque d'exercices (recherche, filtre par catégorie, archivage).
class ExercisesPage : public QWidget
{
    Q_OBJECT

public:
    explicit ExercisesPage(const QSqlDatabase &db, QWidget *parent = nullptr);

private:
    void reload();
    void applyFilters();
    void updateButtons();
    void showDetails();
    const Exercise *selectedExercise() const;

    void addExercise();
    void editSelected();
    void duplicateSelected();
    void toggleArchiveSelected();
    void deleteSelected();
    void showError(const QString &message);

    ExerciseRepository m_repo;
    QList<Exercise> m_all;
    QList<Exercise> m_shown;

    QLineEdit *m_search = nullptr;
    QComboBox *m_category = nullptr;
    QCheckBox *m_showArchived = nullptr;
    QTableWidget *m_table = nullptr;
    QPlainTextEdit *m_details = nullptr;
    QLabel *m_count = nullptr;
    QPushButton *m_add = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_duplicate = nullptr;
    QPushButton *m_archive = nullptr;
    QPushButton *m_delete = nullptr;
};
