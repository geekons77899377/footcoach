#include "SquadPage.h"

#include "ui/Labels.h"
#include "ui/PlayerDialog.h"
#include "ui/PlayerTableModel.h"
#include "ui/TeamDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

namespace {
const QString kLastTeamKey = QStringLiteral("squad/lastTeamId");
}

SquadPage::SquadPage(const QSqlDatabase &db, QWidget *parent)
    : QWidget(parent)
    , m_teams(db)
    , m_players(db)
{
    m_teamCombo = new QComboBox;
    m_teamCombo->setMinimumWidth(220);
    auto *manage = new QPushButton(tr("Gérer les équipes…"));

    m_search = new QLineEdit;
    m_search->setPlaceholderText(tr("Rechercher (nom, prénom, numéro)"));
    m_search->setClearButtonEnabled(true);

    m_positionFilter = new QComboBox;
    m_positionFilter->addItem(tr("Tous les postes"), QString());
    for (const auto &e : Labels::positions())
        m_positionFilter->addItem(e.second, e.first);

    m_showInactive = new QCheckBox(tr("Afficher les joueurs inactifs"));

    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel(tr("Équipe :")));
    top->addWidget(m_teamCombo);
    top->addWidget(manage);
    top->addStretch();

    auto *filters = new QHBoxLayout;
    filters->addWidget(m_search, 1);
    filters->addWidget(m_positionFilter);
    filters->addWidget(m_showInactive);

    m_model = new PlayerTableModel(this);
    m_proxy = new PlayerFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortRole(PlayerTableModel::SortRole);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);

    m_table = new QTableView;
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(true);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(PlayerTableModel::ColLastName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(PlayerTableModel::ColFirstName, QHeaderView::Stretch);
    m_table->sortByColumn(PlayerTableModel::ColLastName, Qt::AscendingOrder);

    m_add = new QPushButton(tr("Ajouter…"));
    m_edit = new QPushButton(tr("Modifier…"));
    m_toggle = new QPushButton(tr("Désactiver"));
    m_delete = new QPushButton(tr("Supprimer définitivement…"));
    m_count = new QLabel;

    auto *bottom = new QHBoxLayout;
    bottom->addWidget(m_add);
    bottom->addWidget(m_edit);
    bottom->addWidget(m_toggle);
    bottom->addWidget(m_delete);
    bottom->addStretch();
    bottom->addWidget(m_count);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addLayout(filters);
    layout->addWidget(m_table, 1);
    layout->addLayout(bottom);

    connect(manage, &QPushButton::clicked, this, &SquadPage::manageTeams);
    connect(m_teamCombo, &QComboBox::currentIndexChanged, this, [this] {
        const int id = currentTeamId();
        if (id >= 0)
            QSettings().setValue(kLastTeamKey, id);
        reloadPlayers();
    });
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_proxy->setTextFilter(text);
        updateButtons();
    });
    connect(m_positionFilter, &QComboBox::currentIndexChanged, this, [this] {
        m_proxy->setPositionFilter(m_positionFilter->currentData().toString());
        updateButtons();
    });
    connect(m_showInactive, &QCheckBox::toggled, this, &SquadPage::reloadPlayers);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &SquadPage::updateButtons);
    connect(m_table, &QTableView::doubleClicked, this, &SquadPage::editSelected);
    connect(m_add, &QPushButton::clicked, this, &SquadPage::addPlayer);
    connect(m_edit, &QPushButton::clicked, this, &SquadPage::editSelected);
    connect(m_toggle, &QPushButton::clicked, this, &SquadPage::toggleActiveSelected);
    connect(m_delete, &QPushButton::clicked, this, &SquadPage::deleteSelected);

    loadTeams(QSettings().value(kLastTeamKey, -1).toInt());

    // Premier lancement : aucune équipe, on propose d'en créer une tout de suite.
    if (m_teamCombo->count() == 0)
        QTimer::singleShot(0, this, &SquadPage::manageTeams);
}

int SquadPage::currentTeamId() const
{
    return m_teamCombo->currentIndex() >= 0 ? m_teamCombo->currentData().toInt() : -1;
}

const Player *SquadPage::selectedPlayer() const
{
    const QModelIndex proxyIndex = m_table->currentIndex();
    if (!proxyIndex.isValid())
        return nullptr;
    return m_model->playerAt(m_proxy->mapToSource(proxyIndex).row());
}

void SquadPage::loadTeams(int preferredTeamId)
{
    QString error;
    const QList<Team> teams = m_teams.all(&error);
    if (!error.isEmpty())
        showError(error);

    // Le remplissage de la liste déclencherait currentIndexChanged à chaque ajout.
    m_teamCombo->blockSignals(true);
    m_teamCombo->clear();
    int select = 0;
    for (int i = 0; i < teams.size(); ++i) {
        const Team &t = teams.at(i);
        QString label = t.name;
        if (!t.category.isEmpty())
            label += QStringLiteral(" (%1)").arg(t.category);
        m_teamCombo->addItem(label, t.id);
        if (t.id == preferredTeamId)
            select = i;
    }
    if (!teams.isEmpty())
        m_teamCombo->setCurrentIndex(select);
    m_teamCombo->blockSignals(false);

    reloadPlayers();
}

void SquadPage::reloadPlayers()
{
    const int keepId = selectedPlayer() ? selectedPlayer()->id : -1;

    QString error;
    const int teamId = currentTeamId();
    m_model->setPlayers(teamId >= 0 ? m_players.list(teamId, m_showInactive->isChecked(), &error)
                                    : QList<Player>());
    if (!error.isEmpty())
        showError(error);

    // Rétablit la sélection après le rechargement.
    if (keepId >= 0) {
        for (int row = 0; row < m_proxy->rowCount(); ++row) {
            const Player *p = m_model->playerAt(m_proxy->mapToSource(m_proxy->index(row, 0)).row());
            if (p && p->id == keepId) {
                m_table->selectRow(row);
                break;
            }
        }
    }
    updateButtons();
}

void SquadPage::updateButtons()
{
    const bool hasTeam = currentTeamId() >= 0;
    const Player *p = selectedPlayer();

    m_add->setEnabled(hasTeam);
    m_edit->setEnabled(p != nullptr);
    m_toggle->setEnabled(p != nullptr);
    m_toggle->setText(p && !p->active ? tr("Réactiver") : tr("Désactiver"));
    m_delete->setEnabled(p != nullptr);
    m_count->setText(tr("%1 joueur(s) affiché(s)").arg(m_proxy->rowCount()));
}

void SquadPage::manageTeams()
{
    TeamManagerDialog dlg(m_teams, this);
    dlg.exec();
    if (dlg.changed())
        loadTeams(currentTeamId());
}

void SquadPage::addPlayer()
{
    Player p;
    p.teamId = currentTeamId();
    if (p.teamId < 0)
        return;

    PlayerDialog dlg(p, m_players, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    Player created = dlg.player();
    QString error;
    if (!m_players.insert(created, &error)) {
        showError(error);
        return;
    }
    // Un joueur ajouté inactif n'apparaîtrait pas dans la liste : on l'affiche pour le retrouver.
    if (!created.active)
        m_showInactive->setChecked(true);
    reloadPlayers();
}

void SquadPage::editSelected()
{
    const Player *current = selectedPlayer();
    if (!current)
        return;

    PlayerDialog dlg(*current, m_players, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QString error;
    if (!m_players.update(dlg.player(), &error)) {
        showError(error);
        return;
    }
    reloadPlayers();
}

void SquadPage::toggleActiveSelected()
{
    const Player *p = selectedPlayer();
    if (!p)
        return;

    const bool target = !p->active;
    // Réactiver un joueur dont le numéro a été repris par un autre est refusé par l'index unique :
    // on le détecte avant pour donner un message clair.
    if (target && p->jerseyNumber > 0 && m_players.jerseyTaken(p->teamId, p->jerseyNumber, p->id)) {
        QMessageBox::warning(this, tr("Réactiver le joueur"),
                             tr("Le numéro %1 est maintenant porté par un autre joueur actif. "
                                "Modifiez d'abord le numéro de l'un des deux joueurs.")
                                 .arg(p->jerseyNumber));
        return;
    }

    QString error;
    if (!m_players.setActive(p->id, target, &error)) {
        showError(error);
        return;
    }
    reloadPlayers();
}

void SquadPage::deleteSelected()
{
    const Player *p = selectedPlayer();
    if (!p)
        return;

    const auto answer = QMessageBox::warning(
        this, tr("Supprimer définitivement"),
        tr("Supprimer définitivement %1 ?\n\n"
           "Son historique de présences et ses blessures seront aussi effacés. "
           "Cette action est irréversible. Pour conserver l'historique, utilisez plutôt « Désactiver ».")
            .arg(p->fullName()),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes)
        return;

    QString error;
    if (!m_players.remove(p->id, &error)) {
        showError(error);
        return;
    }
    reloadPlayers();
}

void SquadPage::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Erreur"), message);
}
