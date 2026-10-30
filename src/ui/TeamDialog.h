#pragma once

#include "core/Models.h"

#include <QDialog>

class TeamRepository;
class QLineEdit;
class QListWidget;
class QPushButton;

// Saisie d'une équipe (nom obligatoire, catégorie et saison facultatives).
class TeamEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TeamEditDialog(const Team &initial, QWidget *parent = nullptr);

    Team team() const;
    void accept() override;

private:
    Team m_team;
    QLineEdit *m_name = nullptr;
    QLineEdit *m_category = nullptr;
    QLineEdit *m_season = nullptr;
};

// Liste des équipes : création, modification, suppression.
class TeamManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TeamManagerDialog(TeamRepository &repo, QWidget *parent = nullptr);

    bool changed() const { return m_changed; }

private:
    void reload();
    int currentTeamId() const;
    void addTeam();
    void editTeam();
    void deleteTeam();

    TeamRepository &m_repo;
    QListWidget *m_list = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_delete = nullptr;
    QList<Team> m_teams;
    bool m_changed = false;
};
