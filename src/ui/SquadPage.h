#pragma once

#include "core/PlayerRepository.h"
#include "core/TeamRepository.h"

#include <QSqlDatabase>
#include <QWidget>

class PlayerFilterProxyModel;
class PlayerTableModel;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableView;

// Page « Effectif » : choix de l'équipe, liste filtrable des joueurs, ajout / modification /
// désactivation / suppression définitive.
class SquadPage : public QWidget
{
    Q_OBJECT

public:
    explicit SquadPage(const QSqlDatabase &db, QWidget *parent = nullptr);

private:
    void loadTeams(int preferredTeamId);
    void reloadPlayers();
    void updateButtons();
    int currentTeamId() const;
    const Player *selectedPlayer() const;

    void manageTeams();
    void addPlayer();
    void editSelected();
    void toggleActiveSelected();
    void deleteSelected();
    void showError(const QString &message);

    TeamRepository m_teams;
    PlayerRepository m_players;

    QComboBox *m_teamCombo = nullptr;
    QLineEdit *m_search = nullptr;
    QComboBox *m_positionFilter = nullptr;
    QCheckBox *m_showInactive = nullptr;
    QTableView *m_table = nullptr;
    QLabel *m_count = nullptr;
    QPushButton *m_add = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_toggle = nullptr;
    QPushButton *m_delete = nullptr;

    PlayerTableModel *m_model = nullptr;
    PlayerFilterProxyModel *m_proxy = nullptr;
};
