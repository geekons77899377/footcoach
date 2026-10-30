#pragma once

#include "core/Models.h"

#include <QDialog>

class QCheckBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QSpinBox;
class QTimeEdit;

// Génération de séances sur une période : jours de la semaine + heure + dates de début/fin.
// Chaque séance générée est ensuite indépendante (aucune règle de série n'est conservée).
class RecurrenceDialog : public QDialog
{
    Q_OBJECT

public:
    RecurrenceDialog(int teamId, const QDate &firstDay, QWidget *parent = nullptr);

    // Séances à créer (id = -1), prêtes à être insérées.
    QList<Session> sessions() const;

    void accept() override;

private:
    void updatePreview();
    QSet<int> selectedWeekdays() const;

    int m_teamId;
    QCheckBox *m_days[7] = {};          // index 0 = lundi
    QTimeEdit *m_time = nullptr;
    QSpinBox *m_duration = nullptr;
    QDateEdit *m_from = nullptr;
    QDateEdit *m_to = nullptr;
    QLineEdit *m_location = nullptr;
    QLineEdit *m_theme = nullptr;
    QLabel *m_preview = nullptr;
};
