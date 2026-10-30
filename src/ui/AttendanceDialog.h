#pragma once

#include "core/AttendanceRepository.h"
#include "core/Models.h"

#include <QDialog>
#include <QList>
#include <QMap>

class QCheckBox;
class QComboBox;
class QLabel;
class QTableWidget;

// Appel d'une séance : un statut et une note par joueur.
// Les joueurs sans appel enregistré démarrent « absents » (règle : pas d'appel = absent).
class AttendanceDialog : public QDialog
{
    Q_OBJECT

public:
    AttendanceDialog(const Session &session, const QList<Player> &players,
                     const QMap<int, AttendanceRepository::Entry> &existing, QWidget *parent = nullptr);

    QList<AttendanceRepository::Entry> entries() const;
    // Vrai si l'utilisateur veut passer la séance « prévue » en « réalisée ».
    bool markAsDone() const;

private:
    QComboBox *statusCombo(int row) const;
    void setAll(const QString &status);
    void updateSummary();

    QList<Player> m_players;
    QTableWidget *m_table = nullptr;
    QLabel *m_summary = nullptr;
    QCheckBox *m_markDone = nullptr;
};
