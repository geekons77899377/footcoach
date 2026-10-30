#include "BoardPropertiesDialog.h"

#include "core/SessionRepository.h"
#include "core/TeamRepository.h"

#include <QComboBox>
#include <algorithm>
#include <QDate>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QVBoxLayout>

QList<SessionChoice> sessionChoices(const QSqlDatabase &db)
{
    const TeamRepository teams(db);
    const SessionRepository sessions(db);
    const QDate today = QDate::currentDate();

    QList<Session> upcoming;
    QList<Session> past;
    QMap<int, QString> teamNames;
    for (const Team &team : teams.all()) {
        teamNames.insert(team.id, team.name);
        for (const Session &s : sessions.listBetween(team.id, QDate(2000, 1, 1), QDate(2100, 12, 31))) {
            if (s.startAt.date() >= today)
                upcoming.append(s);
            else
                past.append(s);
        }
    }
    std::sort(upcoming.begin(), upcoming.end(), [](const Session &a, const Session &b) { return a.startAt < b.startAt; });
    std::sort(past.begin(), past.end(), [](const Session &a, const Session &b) { return a.startAt > b.startAt; });

    QList<SessionChoice> result;
    for (const QList<Session> *list : {&upcoming, &past}) {
        for (const Session &s : *list) {
            QString label = QStringLiteral("%1 — %2")
                                .arg(s.startAt.toString(QStringLiteral("dd/MM/yyyy HH:mm")), teamNames.value(s.teamId));
            if (!s.theme.isEmpty())
                label += QStringLiteral(" — %1").arg(s.theme);
            result.append({s.id, label});
        }
    }
    return result;
}

BoardPropertiesDialog::BoardPropertiesDialog(const QSqlDatabase &db, const BoardRecord &board, bool isNew, QWidget *parent)
    : QDialog(parent)
    , m_board(board)
{
    setWindowTitle(isNew ? tr("Nouveau schéma") : tr("Propriétés du schéma"));
    setMinimumWidth(460);

    m_title = new QLineEdit(board.title);
    m_title->setMaxLength(120);

    m_exercise = new QComboBox;
    m_exercise->addItem(tr("(aucun)"), -1);
    for (const Exercise &e : ExerciseRepository(db).list(true)) {
        // Un exercice archivé n'est proposé que s'il est déjà lié à ce schéma.
        if (e.archived && e.id != board.exerciseId)
            continue;
        m_exercise->addItem(e.archived ? tr("%1 (archivé)").arg(e.title) : e.title, e.id);
    }
    m_exercise->setCurrentIndex(qMax(0, m_exercise->findData(board.exerciseId)));

    m_session = new QComboBox;
    m_session->addItem(tr("(aucune)"), -1);
    for (const SessionChoice &c : sessionChoices(db))
        m_session->addItem(c.label, c.id);
    m_session->setCurrentIndex(qMax(0, m_session->findData(board.sessionId)));

    auto *form = new QFormLayout;
    form->addRow(tr("Titre"), m_title);
    form->addRow(tr("Exercice lié"), m_exercise);
    form->addRow(tr("Séance liée"), m_session);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &BoardPropertiesDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void BoardPropertiesDialog::accept()
{
    if (m_title->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Le titre est obligatoire."));
        m_title->setFocus();
        return;
    }
    QDialog::accept();
}

BoardRecord BoardPropertiesDialog::result() const
{
    BoardRecord b = m_board;
    b.title = m_title->text().trimmed();
    b.exerciseId = m_exercise->currentData().toInt();
    b.sessionId = m_session->currentData().toInt();
    return b;
}
