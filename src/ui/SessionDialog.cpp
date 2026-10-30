#include "SessionDialog.h"

#include "core/SessionRepository.h"
#include "ui/Labels.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTimeEdit>
#include <QVBoxLayout>

SessionDialog::SessionDialog(const Session &initial, const SessionRepository &repo, QWidget *parent)
    : QDialog(parent)
    , m_session(initial)
    , m_repo(repo)
{
    setWindowTitle(initial.id < 0 ? tr("Nouvelle séance") : tr("Modifier la séance"));
    setModal(true);

    const QDateTime start = initial.startAt.isValid()
                                ? initial.startAt
                                : QDateTime(QDate::currentDate(), QTime(18, 30));

    m_date = new QDateEdit(start.date());
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat(QStringLiteral("dddd dd/MM/yyyy"));

    m_time = new QTimeEdit(start.time());
    m_time->setDisplayFormat(QStringLiteral("HH:mm"));

    m_duration = new QSpinBox;
    m_duration->setRange(15, 300);
    m_duration->setSingleStep(5);
    m_duration->setSuffix(tr(" min"));
    m_duration->setValue(initial.durationMin);

    m_location = new QLineEdit(initial.location);
    m_theme = new QLineEdit(initial.theme);
    m_theme->setPlaceholderText(tr("ex. Jeu de possession, Finition"));

    m_status = new QComboBox;
    for (const auto &e : Labels::statuses())
        m_status->addItem(e.second, e.first);
    m_status->setCurrentIndex(qMax(0, m_status->findData(initial.status)));

    m_notes = new QPlainTextEdit(initial.notes);
    m_notes->setFixedHeight(80);

    auto *form = new QFormLayout;
    form->addRow(tr("Date *"), m_date);
    form->addRow(tr("Heure *"), m_time);
    form->addRow(tr("Durée *"), m_duration);
    form->addRow(tr("Lieu"), m_location);
    form->addRow(tr("Thème"), m_theme);
    form->addRow(tr("Statut"), m_status);
    form->addRow(tr("Notes"), m_notes);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &SessionDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &SessionDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    m_theme->setFocus();
}

Session SessionDialog::session() const
{
    Session s = m_session;
    s.startAt = QDateTime(m_date->date(), m_time->time());
    s.durationMin = m_duration->value();
    s.location = m_location->text().trimmed();
    s.theme = m_theme->text().trimmed();
    s.status = m_status->currentData().toString();
    s.notes = m_notes->toPlainText().trimmed();
    return s;
}

void SessionDialog::accept()
{
    const Session s = session();

    // Une séance annulée ne bloque personne : on ne contrôle que les autres statuts.
    if (s.status != QLatin1String("cancelled")) {
        const QList<Session> clash = m_repo.conflicts(s.teamId, s.startAt, s.durationMin, s.id);
        if (!clash.isEmpty()) {
            const Session &c = clash.first();
            const auto answer = QMessageBox::warning(
                this, windowTitle(),
                tr("Cette séance chevauche une séance existante de l'équipe :\n\n"
                   "%1 de %2 à %3%4\n\nL'enregistrer quand même ?")
                    .arg(c.startAt.date().toString(QStringLiteral("dd/MM/yyyy")),
                         c.startAt.time().toString(QStringLiteral("HH:mm")),
                         c.endAt().time().toString(QStringLiteral("HH:mm")),
                         c.theme.isEmpty() ? QString() : QStringLiteral(" — ") + c.theme),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes)
                return;
        }
    }
    QDialog::accept();
}
