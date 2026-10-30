#pragma once

#include "core/BoardRepository.h"
#include "core/ExerciseRepository.h"

#include <QDialog>
#include <QList>
#include <QSqlDatabase>

class QComboBox;
class QLineEdit;

// Séance proposée pour lier un schéma : id et libellé « 27/10/2026 18:30 — U15 — thème ».
struct SessionChoice
{
    int id = -1;
    QString label;
};

// Séances de toutes les équipes : d'abord les séances à venir (de la plus proche à la plus lointaine),
// puis les passées (de la plus récente à la plus ancienne).
QList<SessionChoice> sessionChoices(const QSqlDatabase &db);

// Titre du schéma et liens vers un exercice et/ou une séance.
class BoardPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    BoardPropertiesDialog(const QSqlDatabase &db, const BoardRecord &board, bool isNew, QWidget *parent = nullptr);

    // Le schéma de départ avec le titre et les liens saisis (le contenu n'est pas modifié).
    BoardRecord result() const;

    void accept() override;

private:
    BoardRecord m_board;
    QLineEdit *m_title = nullptr;
    QComboBox *m_exercise = nullptr;
    QComboBox *m_session = nullptr;
};
