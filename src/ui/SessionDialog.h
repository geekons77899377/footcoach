#pragma once

#include "core/Models.h"

#include <QDialog>

class SessionRepository;
class QComboBox;
class QDateEdit;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QTimeEdit;

// Formulaire d'une séance (l'équipe est fixée par l'appelant).
class SessionDialog : public QDialog
{
    Q_OBJECT

public:
    SessionDialog(const Session &initial, const SessionRepository &repo, QWidget *parent = nullptr);

    Session session() const;
    void accept() override;

private:
    Session m_session;
    const SessionRepository &m_repo;

    QDateEdit *m_date = nullptr;
    QTimeEdit *m_time = nullptr;
    QSpinBox *m_duration = nullptr;
    QLineEdit *m_location = nullptr;
    QLineEdit *m_theme = nullptr;
    QComboBox *m_status = nullptr;
    QPlainTextEdit *m_notes = nullptr;
};
