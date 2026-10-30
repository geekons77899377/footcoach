#include "PlayerDialog.h"

#include "core/PlayerRepository.h"
#include "ui/Labels.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

// Date sentinelle affichée comme « — » : signifie « date inconnue ».
const QDate kUnknownDate(1900, 1, 1);

void fillCombo(QComboBox *combo, const QList<QPair<QString, QString>> &entries, const QString &current)
{
    combo->addItem(QStringLiteral("—"), QString());
    for (const auto &e : entries)
        combo->addItem(e.second, e.first);
    const int index = combo->findData(current);
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

} // namespace

PlayerDialog::PlayerDialog(const Player &initial, const PlayerRepository &repo, QWidget *parent)
    : QDialog(parent)
    , m_player(initial)
    , m_repo(repo)
{
    setWindowTitle(initial.id < 0 ? tr("Nouveau joueur") : tr("Modifier le joueur"));
    setModal(true);

    m_firstName = new QLineEdit(initial.firstName);
    m_lastName = new QLineEdit(initial.lastName);

    m_jersey = new QSpinBox;
    m_jersey->setRange(0, 99);
    m_jersey->setSpecialValueText(QStringLiteral("—"));
    m_jersey->setValue(initial.jerseyNumber);

    m_position = new QComboBox;
    fillCombo(m_position, Labels::positions(), initial.position);

    m_foot = new QComboBox;
    fillCombo(m_foot, Labels::feet(), initial.preferredFoot);

    m_birth = new QDateEdit;
    m_birth->setCalendarPopup(true);
    m_birth->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_birth->setMinimumDate(kUnknownDate);
    m_birth->setMaximumDate(QDate::currentDate());
    m_birth->setSpecialValueText(QStringLiteral("—"));
    m_birth->setDate(initial.birthDate.isValid() ? initial.birthDate : kUnknownDate);

    m_phone = new QLineEdit(initial.phone);
    m_email = new QLineEdit(initial.email);

    m_notes = new QPlainTextEdit(initial.notes);
    m_notes->setFixedHeight(80);

    m_active = new QCheckBox(tr("Joueur actif"));
    m_active->setChecked(initial.active);

    auto *form = new QFormLayout;
    form->addRow(tr("Prénom *"), m_firstName);
    form->addRow(tr("Nom *"), m_lastName);
    form->addRow(tr("Numéro de maillot"), m_jersey);
    form->addRow(tr("Poste"), m_position);
    form->addRow(tr("Pied fort"), m_foot);
    form->addRow(tr("Date de naissance"), m_birth);
    form->addRow(tr("Téléphone"), m_phone);
    form->addRow(tr("E-mail"), m_email);
    form->addRow(tr("Notes"), m_notes);
    form->addRow(QString(), m_active);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &PlayerDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &PlayerDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    m_lastName->setFocus();
}

Player PlayerDialog::player() const
{
    Player p = m_player;
    p.firstName = m_firstName->text().trimmed();
    p.lastName = m_lastName->text().trimmed();
    p.jerseyNumber = m_jersey->value();
    p.position = m_position->currentData().toString();
    p.preferredFoot = m_foot->currentData().toString();
    p.birthDate = (m_birth->date() == kUnknownDate) ? QDate() : m_birth->date();
    p.phone = m_phone->text().trimmed();
    p.email = m_email->text().trimmed();
    p.notes = m_notes->toPlainText().trimmed();
    p.active = m_active->isChecked();
    return p;
}

void PlayerDialog::accept()
{
    const Player p = player();

    if (p.firstName.isEmpty() || p.lastName.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Le nom et le prénom sont obligatoires."));
        return;
    }

    if (p.active && p.jerseyNumber > 0 && m_repo.jerseyTaken(p.teamId, p.jerseyNumber, p.id)) {
        QMessageBox::warning(this, windowTitle(),
                             tr("Le numéro %1 est déjà porté par un autre joueur actif de l'équipe.")
                                 .arg(p.jerseyNumber));
        m_jersey->setFocus();
        return;
    }

    QDialog::accept();
}
