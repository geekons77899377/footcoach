#include "PlayerTableModel.h"

#include "ui/Labels.h"

#include <QBrush>
#include <QColor>
#include <QDate>

namespace {

int ageOf(const QDate &birth)
{
    if (!birth.isValid())
        return -1;
    const QDate today = QDate::currentDate();
    int age = today.year() - birth.year();
    if (today.month() < birth.month() || (today.month() == birth.month() && today.day() < birth.day()))
        --age;
    return age;
}

} // namespace

PlayerTableModel::PlayerTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void PlayerTableModel::setPlayers(const QList<Player> &players)
{
    beginResetModel();
    m_players = players;
    endResetModel();
}

const Player *PlayerTableModel::playerAt(int row) const
{
    return (row >= 0 && row < m_players.size()) ? &m_players.at(row) : nullptr;
}

int PlayerTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_players.size());
}

int PlayerTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant PlayerTableModel::data(const QModelIndex &index, int role) const
{
    const Player *p = index.isValid() ? playerAt(index.row()) : nullptr;
    if (!p)
        return {};

    if (role == Qt::ForegroundRole)
        return p->active ? QVariant() : QVariant(QBrush(QColor(Qt::gray)));

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColNumber || index.column() == ColAge)
            return int(Qt::AlignCenter);
        return QVariant();
    }

    const bool display = (role == Qt::DisplayRole);
    if (!display && role != SortRole)
        return {};

    switch (index.column()) {
    case ColNumber:
        if (display)
            return p->jerseyNumber > 0 ? QVariant(p->jerseyNumber) : QVariant(QStringLiteral("—"));
        return p->jerseyNumber > 0 ? p->jerseyNumber : 1000;   // sans numéro : en fin de liste
    case ColLastName:
        return p->lastName;
    case ColFirstName:
        return p->firstName;
    case ColPosition:
        return Labels::label(Labels::positions(), p->position);
    case ColFoot:
        return Labels::label(Labels::feet(), p->preferredFoot);
    case ColAge: {
        const int age = ageOf(p->birthDate);
        if (display)
            return age >= 0 ? QVariant(age) : QVariant(QStringLiteral("—"));
        return age;
    }
    case ColActive:
        return p->active ? tr("Actif") : tr("Inactif");
    default:
        return {};
    }
}

QVariant PlayerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColNumber:    return tr("N°");
    case ColLastName:  return tr("Nom");
    case ColFirstName: return tr("Prénom");
    case ColPosition:  return tr("Poste");
    case ColFoot:      return tr("Pied");
    case ColAge:       return tr("Âge");
    case ColActive:    return tr("Statut");
    default:           return {};
    }
}

void PlayerFilterProxyModel::setTextFilter(const QString &text)
{
    m_text = text.trimmed();
    invalidateFilter();
}

void PlayerFilterProxyModel::setPositionFilter(const QString &positionCode)
{
    m_position = positionCode;
    invalidateFilter();
}

bool PlayerFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)
    const auto *model = qobject_cast<const PlayerTableModel *>(sourceModel());
    const Player *p = model ? model->playerAt(sourceRow) : nullptr;
    if (!p)
        return false;

    if (!m_position.isEmpty() && p->position != m_position)
        return false;

    if (!m_text.isEmpty()) {
        const bool match = p->lastName.contains(m_text, Qt::CaseInsensitive)
                           || p->firstName.contains(m_text, Qt::CaseInsensitive)
                           || (p->jerseyNumber > 0 && QString::number(p->jerseyNumber) == m_text);
        if (!match)
            return false;
    }
    return true;
}
