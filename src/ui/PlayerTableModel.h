#pragma once

#include "core/Models.h"

#include <QAbstractTableModel>
#include <QList>
#include <QSortFilterProxyModel>

class PlayerTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column { ColNumber, ColLastName, ColFirstName, ColPosition, ColFoot, ColAge, ColActive, ColumnCount };

    // Rôle utilisé par le tri (nombres comparés comme des nombres, pas comme du texte).
    static constexpr int SortRole = Qt::UserRole + 1;

    explicit PlayerTableModel(QObject *parent = nullptr);

    void setPlayers(const QList<Player> &players);
    const Player *playerAt(int row) const;

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    QList<Player> m_players;
};

// Filtre par texte (nom/prénom/numéro) et par poste.
class PlayerFilterProxyModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setTextFilter(const QString &text);
    void setPositionFilter(const QString &positionCode);   // vide = tous

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_text;
    QString m_position;
};
