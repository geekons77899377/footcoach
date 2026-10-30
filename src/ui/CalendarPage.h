#pragma once

#include "core/AttendanceRepository.h"
#include "core/PlayerRepository.h"
#include "core/SessionRepository.h"
#include "core/TeamRepository.h"

#include <QSqlDatabase>
#include <QWidget>

class QCalendarWidget;
class QComboBox;
class QLabel;
class QPushButton;
class QShowEvent;
class QTableWidget;

// Page « Calendrier » : calendrier mensuel (jours avec séance en évidence) et liste des séances
// du jour, de la semaine ou du mois sélectionné, pour l'équipe choisie.
class CalendarPage : public QWidget
{
    Q_OBJECT

public:
    explicit CalendarPage(const QSqlDatabase &db, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void loadTeams();
    void reloadAll();
    void refreshHighlights();
    void reloadList();
    void updateButtons();
    int currentTeamId() const;
    const Session *selectedSession() const;
    void visibleRange(QDate *from, QDate *to) const;

    void addSession();
    void addRecurring();
    void editSelected();
    void takeAttendance();
    void changeStatus(const QString &status);
    void deleteSelected();
    void showError(const QString &message);

    TeamRepository m_teams;
    SessionRepository m_sessions;
    PlayerRepository m_players;
    AttendanceRepository m_attendance;
    QList<Session> m_listed;

    QComboBox *m_teamCombo = nullptr;
    QComboBox *m_scope = nullptr;
    QCalendarWidget *m_calendar = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_info = nullptr;
    QPushButton *m_add = nullptr;
    QPushButton *m_recurring = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_attendanceButton = nullptr;
    QPushButton *m_statusButton = nullptr;
    QPushButton *m_delete = nullptr;
};
