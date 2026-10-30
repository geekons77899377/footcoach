#include "RecurrenceDialog.h"

#include "core/Recurrence.h"

#include <QCheckBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QSpinBox>
#include <QTimeEdit>
#include <QVBoxLayout>

RecurrenceDialog::RecurrenceDialog(int teamId, const QDate &firstDay, QWidget *parent)
    : QDialog(parent)
    , m_teamId(teamId)
{
    setWindowTitle(tr("Séances récurrentes"));
    setModal(true);

    auto *daysRow = new QHBoxLayout;
    const QLocale fr(QLocale::French);
    for (int i = 0; i < 7; ++i) {
        m_days[i] = new QCheckBox(fr.dayName(i + 1, QLocale::ShortFormat));   // 1 = lundi
        daysRow->addWidget(m_days[i]);
    }
    // Par défaut : le jour de la semaine de la date de départ.
    m_days[firstDay.dayOfWeek() - 1]->setChecked(true);

    m_time = new QTimeEdit(QTime(18, 30));
    m_time->setDisplayFormat(QStringLiteral("HH:mm"));

    m_duration = new QSpinBox;
    m_duration->setRange(15, 300);
    m_duration->setSingleStep(5);
    m_duration->setSuffix(tr(" min"));
    m_duration->setValue(90);

    m_from = new QDateEdit(firstDay);
    m_from->setCalendarPopup(true);
    m_to = new QDateEdit(firstDay.addMonths(3));
    m_to->setCalendarPopup(true);
    for (QDateEdit *e : {m_from, m_to})
        e->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));

    m_location = new QLineEdit;
    m_theme = new QLineEdit;

    m_preview = new QLabel;
    m_preview->setWordWrap(true);

    auto *form = new QFormLayout;
    form->addRow(tr("Jours *"), daysRow);
    form->addRow(tr("Heure *"), m_time);
    form->addRow(tr("Durée *"), m_duration);
    form->addRow(tr("Du *"), m_from);
    form->addRow(tr("Au (inclus) *"), m_to);
    form->addRow(tr("Lieu"), m_location);
    form->addRow(tr("Thème"), m_theme);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &RecurrenceDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &RecurrenceDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_preview);
    layout->addWidget(buttons);

    // Les connexions se font en dernier : updatePreview() lit tous les widgets ci-dessus.
    for (QCheckBox *day : m_days)
        connect(day, &QCheckBox::toggled, this, &RecurrenceDialog::updatePreview);
    connect(m_time, &QTimeEdit::timeChanged, this, &RecurrenceDialog::updatePreview);
    connect(m_from, &QDateEdit::dateChanged, this, &RecurrenceDialog::updatePreview);
    connect(m_to, &QDateEdit::dateChanged, this, &RecurrenceDialog::updatePreview);
    updatePreview();
}

QSet<int> RecurrenceDialog::selectedWeekdays() const
{
    QSet<int> days;
    for (int i = 0; i < 7; ++i) {
        if (m_days[i]->isChecked())
            days.insert(i + 1);   // Qt::DayOfWeek : 1 = lundi … 7 = dimanche
    }
    return days;
}

QList<Session> RecurrenceDialog::sessions() const
{
    QList<Session> result;
    const QList<QDateTime> starts =
        Recurrence::occurrences(m_from->date(), m_to->date(), selectedWeekdays(), m_time->time());
    for (const QDateTime &start : starts) {
        Session s;
        s.teamId = m_teamId;
        s.startAt = start;
        s.durationMin = m_duration->value();
        s.location = m_location->text().trimmed();
        s.theme = m_theme->text().trimmed();
        result.append(s);
    }
    return result;
}

void RecurrenceDialog::updatePreview()
{
    const int count = int(sessions().size());
    if (selectedWeekdays().isEmpty())
        m_preview->setText(tr("Cochez au moins un jour."));
    else if (m_to->date() < m_from->date())
        m_preview->setText(tr("La date de fin précède la date de début."));
    else if (count > Recurrence::kMaxOccurrences)
        m_preview->setText(tr("Trop de séances (maximum %1) : raccourcissez la période.")
                               .arg(Recurrence::kMaxOccurrences));
    else
        m_preview->setText(tr("%n séance(s) seront créées.", nullptr, count));
}

void RecurrenceDialog::accept()
{
    const int count = int(sessions().size());
    if (selectedWeekdays().isEmpty() || m_to->date() < m_from->date() || count == 0
        || count > Recurrence::kMaxOccurrences) {
        QMessageBox::warning(this, windowTitle(), m_preview->text());
        return;
    }
    QDialog::accept();
}
