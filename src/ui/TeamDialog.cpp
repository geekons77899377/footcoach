#include "TeamDialog.h"

#include "core/TeamRepository.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

TeamEditDialog::TeamEditDialog(const Team &initial, QWidget *parent)
    : QDialog(parent)
    , m_team(initial)
{
    setWindowTitle(initial.id < 0 ? tr("Nouvelle équipe") : tr("Modifier l'équipe"));

    m_name = new QLineEdit(initial.name);
    m_category = new QLineEdit(initial.category);
    m_category->setPlaceholderText(tr("ex. U15, Seniors"));
    m_season = new QLineEdit(initial.season);
    m_season->setPlaceholderText(tr("ex. 2026-2027"));

    auto *form = new QFormLayout;
    form->addRow(tr("Nom *"), m_name);
    form->addRow(tr("Catégorie"), m_category);
    form->addRow(tr("Saison"), m_season);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &TeamEditDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &TeamEditDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

Team TeamEditDialog::team() const
{
    Team t = m_team;
    t.name = m_name->text().trimmed();
    t.category = m_category->text().trimmed();
    t.season = m_season->text().trimmed();
    return t;
}

void TeamEditDialog::accept()
{
    if (m_name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Le nom de l'équipe est obligatoire."));
        return;
    }
    QDialog::accept();
}

TeamManagerDialog::TeamManagerDialog(TeamRepository &repo, QWidget *parent)
    : QDialog(parent)
    , m_repo(repo)
{
    setWindowTitle(tr("Gérer les équipes"));
    resize(420, 320);

    m_list = new QListWidget;

    auto *add = new QPushButton(tr("Ajouter…"));
    m_edit = new QPushButton(tr("Modifier…"));
    m_delete = new QPushButton(tr("Supprimer…"));
    auto *close = new QPushButton(tr("Fermer"));

    auto *side = new QVBoxLayout;
    side->addWidget(add);
    side->addWidget(m_edit);
    side->addWidget(m_delete);
    side->addStretch();
    side->addWidget(close);

    auto *layout = new QHBoxLayout(this);
    layout->addWidget(m_list, 1);
    layout->addLayout(side);

    connect(add, &QPushButton::clicked, this, &TeamManagerDialog::addTeam);
    connect(m_edit, &QPushButton::clicked, this, &TeamManagerDialog::editTeam);
    connect(m_delete, &QPushButton::clicked, this, &TeamManagerDialog::deleteTeam);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &TeamManagerDialog::editTeam);
    connect(m_list, &QListWidget::currentRowChanged, this, [this] {
        const bool has = currentTeamId() >= 0;
        m_edit->setEnabled(has);
        m_delete->setEnabled(has);
    });
    connect(close, &QPushButton::clicked, this, &QDialog::accept);

    reload();
}

void TeamManagerDialog::reload()
{
    const int keepId = currentTeamId();
    QString error;
    m_teams = m_repo.all(&error);
    if (!error.isEmpty())
        QMessageBox::critical(this, windowTitle(), error);

    m_list->clear();
    int select = m_teams.isEmpty() ? -1 : 0;
    for (int i = 0; i < m_teams.size(); ++i) {
        const Team &t = m_teams.at(i);
        QStringList extra;
        if (!t.category.isEmpty())
            extra << t.category;
        if (!t.season.isEmpty())
            extra << t.season;
        m_list->addItem(extra.isEmpty() ? t.name : QStringLiteral("%1  (%2)").arg(t.name, extra.join(QStringLiteral(", "))));
        if (t.id == keepId)
            select = i;
    }
    m_list->setCurrentRow(select);
    const bool has = currentTeamId() >= 0;
    m_edit->setEnabled(has);
    m_delete->setEnabled(has);
}

int TeamManagerDialog::currentTeamId() const
{
    const int row = m_list ? m_list->currentRow() : -1;
    return (row >= 0 && row < m_teams.size()) ? m_teams.at(row).id : -1;
}

void TeamManagerDialog::addTeam()
{
    TeamEditDialog dlg(Team{}, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    Team t = dlg.team();
    QString error;
    if (!m_repo.insert(t, &error)) {
        QMessageBox::critical(this, windowTitle(), error);
        return;
    }
    m_changed = true;
    reload();
}

void TeamManagerDialog::editTeam()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_teams.size())
        return;
    TeamEditDialog dlg(m_teams.at(row), this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    QString error;
    if (!m_repo.update(dlg.team(), &error)) {
        QMessageBox::critical(this, windowTitle(), error);
        return;
    }
    m_changed = true;
    reload();
}

void TeamManagerDialog::deleteTeam()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_teams.size())
        return;
    const Team &t = m_teams.at(row);
    const int players = m_repo.playerCount(t.id);

    const auto answer = QMessageBox::warning(
        this, windowTitle(),
        tr("Supprimer définitivement l'équipe « %1 » ?\n\n"
           "Ses %2 joueur(s), ses séances et leurs présences seront aussi supprimés. "
           "Cette action est irréversible.")
            .arg(t.name)
            .arg(players),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes)
        return;

    QString error;
    if (!m_repo.remove(t.id, &error)) {
        QMessageBox::critical(this, windowTitle(), error);
        return;
    }
    m_changed = true;
    reload();
}
