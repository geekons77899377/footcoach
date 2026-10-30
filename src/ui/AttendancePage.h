#pragma once

#include "core/AttendanceRepository.h"
#include "core/TeamRepository.h"

#include <QSqlDatabase>
#include <QWidget>

class QComboBox;
class QLabel;
class QShowEvent;
class QTableWidget;

// Page « Présences » : synthèse (taux par joueur) et grille joueurs × séances réalisées.
class AttendancePage : public QWidget
{
    Q_OBJECT

public:
    explicit AttendancePage(const QSqlDatabase &db, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void loadTeams();
    void refresh();
    int currentTeamId() const;
    void periodRange(QDate *from, QDate *to) const;

    TeamRepository m_teams;
    AttendanceRepository m_attendance;

    QComboBox *m_teamCombo = nullptr;
    QComboBox *m_period = nullptr;
    QTableWidget *m_summary = nullptr;
    QTableWidget *m_grid = nullptr;
    QLabel *m_info = nullptr;
};
