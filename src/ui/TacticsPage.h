#pragma once

#include "core/BoardRepository.h"
#include "core/ExerciseRepository.h"

#include <QList>
#include <QSqlDatabase>
#include <QWidget>

class BoardScene;
class QAction;
class QCheckBox;
class QComboBox;
class QGraphicsView;
class QLabel;
class QListWidget;
class QPushButton;
class QShowEvent;

// Page « Tactique » : liste des schémas à gauche, tableau (terrain, jetons, flèches) à droite.
// Les modifications sont enregistrées automatiquement après chaque action.
class TacticsPage : public QWidget
{
    Q_OBJECT

public:
    explicit TacticsPage(const QSqlDatabase &db, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void reloadList(int selectId = -1);
    void loadCurrent();
    void saveCurrent();
    void updateButtons();
    void updateLinkLabel();
    int currentRow() const;
    BoardRecord *currentRecord();

    void newBoard();
    void editProperties();
    void duplicateBoard();
    void deleteBoard();
    void exportPng();
    void applyFormation();
    void showError(const QString &message);

    QSqlDatabase m_db;
    BoardRepository m_repo;
    ExerciseRepository m_exercises;

    QList<BoardRecord> m_boards;
    bool m_loading = false;      // vrai pendant le chargement d'un schéma : pas d'enregistrement
    bool m_editable = false;     // faux si le JSON du schéma sélectionné est illisible

    QListWidget *m_list = nullptr;
    QPushButton *m_newButton = nullptr;
    QPushButton *m_propsButton = nullptr;
    QPushButton *m_duplicateButton = nullptr;
    QPushButton *m_deleteButton = nullptr;

    BoardScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;
    QWidget *m_editor = nullptr;
    QLabel *m_linkLabel = nullptr;
    QComboBox *m_pitchCombo = nullptr;
    QComboBox *m_formationCombo = nullptr;
    QCheckBox *m_opponents = nullptr;
    QPushButton *m_applyFormation = nullptr;
    QAction *m_deleteAction = nullptr;
    QAction *m_clearAction = nullptr;
    QAction *m_exportAction = nullptr;
    QList<QAction *> m_toolActions;
};
