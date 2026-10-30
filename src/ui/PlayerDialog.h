#pragma once

#include "core/Models.h"

#include <QDialog>

class PlayerRepository;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;

// Formulaire d'ajout / modification d'un joueur. Le joueur édité est renvoyé par player().
class PlayerDialog : public QDialog
{
    Q_OBJECT

public:
    PlayerDialog(const Player &initial, const PlayerRepository &repo, QWidget *parent = nullptr);

    Player player() const;

    void accept() override;

private:
    Player m_player;
    const PlayerRepository &m_repo;

    QLineEdit *m_firstName = nullptr;
    QLineEdit *m_lastName = nullptr;
    QSpinBox *m_jersey = nullptr;
    QComboBox *m_position = nullptr;
    QComboBox *m_foot = nullptr;
    QDateEdit *m_birth = nullptr;
    QLineEdit *m_phone = nullptr;
    QLineEdit *m_email = nullptr;
    QPlainTextEdit *m_notes = nullptr;
    QCheckBox *m_active = nullptr;
};
