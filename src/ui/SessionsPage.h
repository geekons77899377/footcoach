#pragma once

#include "core/BoardRepository.h"
#include "core/ExerciseRepository.h"
#include "core/PlanRepository.h"
#include "core/SessionRepository.h"
#include "core/TeamRepository.h"

#include <QSqlDatabase>
#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QShowEvent;
class QTableWidget;

// Page « Séances » : choix d'une séance, puis construction de son plan d'exercices
// (ajout depuis la bibliothèque, réordonnancement, durées, notes) et export en PDF.
class SessionsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SessionsPage(const QSqlDatabase &db, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void loadTeams();
    void reloadSessions();
    void reloadPlan();
    void updateButtons();
    int currentTeamId() const;
    const Session *selectedSession() const;
    const PlanEntry *selectedItem() const;
    Team currentTeam() const;

    void addExercises();
    void editItem();
    void moveItem(int delta);
    void removeItem();
    void exportPdf();
    void showError(const QString &message);

    TeamRepository m_teams;
    SessionRepository m_sessions;
    ExerciseRepository m_exercises;
    PlanRepository m_plan;
    BoardRepository m_boards;

    QList<Session> m_listedSessions;
    QList<PlanEntry> m_entries;

    QComboBox *m_teamCombo = nullptr;
    QComboBox *m_scope = nullptr;
    QTableWidget *m_sessionTable = nullptr;
    QTableWidget *m_planTable = nullptr;
    QLabel *m_planTitle = nullptr;
    QLabel *m_total = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_upButton = nullptr;
    QPushButton *m_downButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_pdfButton = nullptr;
};
