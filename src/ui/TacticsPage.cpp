#include "TacticsPage.h"

#include "core/TacticalBoard.h"
#include "ui/BoardPropertiesDialog.h"
#include "ui/BoardScene.h"

#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QShowEvent>
#include <QMenu>
#include <QPixmap>
#include <QIcon>
#include <QToolBar>
#include <QToolButton>
#include <QUndoStack>
#include <QUrl>
#include <QVBoxLayout>

namespace {

// Vue qui garde tout le terrain visible, quelle que soit la taille de la fenêtre.
class BoardView : public QGraphicsView
{
public:
    explicit BoardView(QGraphicsScene *scene, QWidget *parent = nullptr)
        : QGraphicsView(scene, parent)
    {
        setRenderHint(QPainter::Antialiasing, true);
        setDragMode(QGraphicsView::RubberBandDrag);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFrameShape(QFrame::NoFrame);
        setMinimumSize(420, 300);
        connect(scene, &QGraphicsScene::sceneRectChanged, this, [this]() { fit(); });
    }

    void fit() { fitInView(sceneRect(), Qt::KeepAspectRatio); }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QGraphicsView::resizeEvent(event);
        fit();
    }
    void showEvent(QShowEvent *event) override
    {
        QGraphicsView::showEvent(event);
        fit();
    }
};

QString safeFileName(const QString &title)
{
    QString name = title;
    name.replace(QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}_-]+")), QStringLiteral("_"));
    name = name.left(40);
    while (name.startsWith(QLatin1Char('_')))
        name.remove(0, 1);
    while (name.endsWith(QLatin1Char('_')))
        name.chop(1);
    return name.isEmpty() ? QStringLiteral("schema") : name;
}

} // namespace

TacticsPage::TacticsPage(const QSqlDatabase &db, QWidget *parent)
    : QWidget(parent)
    , m_db(db)
    , m_repo(db)
    , m_exercises(db)
{
    // --- Colonne de gauche : liste des schémas --------------------------------------------
    m_list = new QListWidget;
    m_list->setMinimumWidth(200);
    m_list->setMaximumWidth(280);
    m_newButton = new QPushButton(tr("Nouveau…"));
    m_propsButton = new QPushButton(tr("Propriétés…"));
    m_duplicateButton = new QPushButton(tr("Dupliquer"));
    m_deleteButton = new QPushButton(tr("Supprimer"));

    auto *left = new QVBoxLayout;
    left->addWidget(new QLabel(tr("<b>Schémas</b>")));
    left->addWidget(m_list, 1);
    left->addWidget(m_newButton);
    left->addWidget(m_propsButton);
    left->addWidget(m_duplicateButton);
    left->addWidget(m_deleteButton);

    // --- Zone de droite : barre d'outils, options, terrain -------------------------------
    m_scene = new BoardScene(this);
    m_view = new BoardView(m_scene);

    auto *toolbar = new QToolBar;
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    auto *toolGroup = new QActionGroup(this);
    toolGroup->setExclusive(true);
    struct ToolDef { QString text; QString tip; BoardScene::Tool tool; };
    const QList<ToolDef> tools = {
        {tr("Sélection"), tr("Sélectionner et déplacer (double-clic sur un joueur ou une note : modifier le texte)"), BoardScene::Tool::Select},
        {tr("Joueur"), tr("Poser un joueur de l'équipe (clic sur le terrain)"), BoardScene::Tool::Player},
        {tr("Adversaire"), tr("Poser un adversaire"), BoardScene::Tool::Opponent},
        {tr("Ballon"), tr("Poser un ballon"), BoardScene::Tool::Ball},
        {tr("Plot"), tr("Poser un plot"), BoardScene::Tool::Cone},
        {tr("But"), tr("Poser un but mobile"), BoardScene::Tool::Goal},
        {tr("Texte"), tr("Ajouter une note texte (clic sur le terrain, double-clic pour la modifier)"), BoardScene::Tool::Text},
        {tr("Course"), tr("Flèche de course (pointillé) : glisser sur le terrain"), BoardScene::Tool::ArrowRun},
        {tr("Passe"), tr("Flèche de passe (trait plein) : glisser sur le terrain"), BoardScene::Tool::ArrowPass},
        {tr("Centre"), tr("Flèche de centre (courbe) : glisser sur le terrain ; « Inverser la courbure » pour changer de côté"), BoardScene::Tool::ArrowCross},
        {tr("Conduite"), tr("Flèche de conduite de balle (ondulée) : glisser sur le terrain"), BoardScene::Tool::ArrowDribble},
    };
    for (const ToolDef &def : tools) {
        QAction *a = toolbar->addAction(def.text);
        a->setCheckable(true);
        a->setToolTip(def.tip);
        a->setData(int(def.tool));
        toolGroup->addAction(a);
        m_toolActions.append(a);
        connect(a, &QAction::triggered, this, [this, def]() { m_scene->setTool(def.tool); });
        if (def.tool == BoardScene::Tool::Select) {
            a->setChecked(true);
            toolbar->addSeparator();
        }
    }
    connect(m_scene, &BoardScene::toolChanged, this, [this](BoardScene::Tool tool) {
        for (QAction *a : m_toolActions)
            a->setChecked(a->data().toInt() == int(tool));
    });

    auto *actionsBar = new QToolBar;
    actionsBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    QAction *undo = m_scene->undoStack()->createUndoAction(this, tr("Annuler"));
    undo->setShortcut(QKeySequence::Undo);
    undo->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    QAction *redo = m_scene->undoStack()->createRedoAction(this, tr("Rétablir"));
    redo->setShortcut(QKeySequence::Redo);
    redo->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    actionsBar->addAction(undo);
    actionsBar->addAction(redo);
    addAction(undo);   // les raccourcis fonctionnent aussi quand le focus est sur la vue
    addAction(redo);

    actionsBar->addSeparator();
    m_deleteAction = actionsBar->addAction(tr("Supprimer la sélection"));
    m_deleteAction->setToolTip(tr("Supprime les éléments sélectionnés (touche Suppr)"));
    QAction *rotateLeft = actionsBar->addAction(tr("Pivoter à gauche"));
    rotateLeft->setToolTip(tr("Fait pivoter les buts et plots sélectionnés de 45° (touche Maj+R)"));
    QAction *rotateRight = actionsBar->addAction(tr("Pivoter à droite"));
    rotateRight->setToolTip(tr("Fait pivoter les buts et plots sélectionnés de 45° (touche R)"));
    connect(rotateLeft, &QAction::triggered, this, [this]() { m_scene->rotateSelection(-45); });
    connect(rotateRight, &QAction::triggered, this, [this]() { m_scene->rotateSelection(45); });
    QAction *flip = actionsBar->addAction(tr("Inverser la courbure"));
    flip->setToolTip(tr("Change le côté de la courbure des centres sélectionnés (touche C)"));
    connect(flip, &QAction::triggered, this, [this]() { m_scene->flipSelectedCurves(); });
    QAction *bigger = actionsBar->addAction(tr("Agrandir"));
    bigger->setToolTip(tr("Agrandit les buts, plots et notes sélectionnés (touche +)"));
    QAction *smaller = actionsBar->addAction(tr("Réduire"));
    smaller->setToolTip(tr("Réduit les buts, plots et notes sélectionnés (touche -)"));
    connect(bigger, &QAction::triggered, this, [this]() { m_scene->scaleSelection(1.25); });
    connect(smaller, &QAction::triggered, this, [this]() { m_scene->scaleSelection(1.0 / 1.25); });

    // Couleur des joueurs, adversaires, plots et notes sélectionnés.
    auto *colorButton = new QToolButton;
    colorButton->setText(tr("Couleur"));
    colorButton->setToolTip(tr("Change la couleur des joueurs, adversaires, plots et notes sélectionnés"));
    colorButton->setPopupMode(QToolButton::InstantPopup);
    auto *colorMenu = new QMenu(colorButton);
    auto addColor = [&](const QString &name, const QString &hex) {
        QPixmap swatch(16, 16);
        swatch.fill(hex.isEmpty() ? QColor(Qt::transparent) : QColor(hex));
        QAction *a = colorMenu->addAction(QIcon(swatch), name);
        connect(a, &QAction::triggered, this, [this, hex]() { m_scene->setSelectionColor(hex); });
    };
    for (const auto &entry : Tactics::colorPalette())
        addColor(tr(entry.first.toUtf8().constData()), entry.second);
    colorMenu->addSeparator();
    addColor(tr("Couleur par défaut"), QString());
    colorButton->setMenu(colorMenu);
    actionsBar->addWidget(colorButton);

    m_clearAction = actionsBar->addAction(tr("Tout effacer"));
    m_exportAction = actionsBar->addAction(tr("Exporter en PNG…"));
    connect(m_deleteAction, &QAction::triggered, m_scene, &BoardScene::deleteSelection);
    connect(m_clearAction, &QAction::triggered, this, [this]() {
        const auto answer = QMessageBox::question(this, tr("Tout effacer"),
                                                  tr("Effacer tous les éléments de ce schéma ? (annulable avec « Annuler »)"));
        if (answer == QMessageBox::Yes)
            m_scene->clearAll();
    });
    connect(m_exportAction, &QAction::triggered, this, &TacticsPage::exportPng);

    m_pitchCombo = new QComboBox;
    m_pitchCombo->addItem(tr("Terrain entier"), int(Tactics::Pitch::Full));
    m_pitchCombo->addItem(tr("Demi-terrain"), int(Tactics::Pitch::Half));
    m_formationCombo = new QComboBox;
    m_formationCombo->addItems(Tactics::formationNames());
    m_opponents = new QCheckBox(tr("avec adversaires en miroir"));
    m_applyFormation = new QPushButton(tr("Placer la formation"));
    m_applyFormation->setToolTip(tr("Remplace les joueurs de l'équipe par la formation choisie"));

    auto *options = new QHBoxLayout;
    options->addWidget(new QLabel(tr("Terrain :")));
    options->addWidget(m_pitchCombo);
    options->addSpacing(16);
    options->addWidget(new QLabel(tr("Formation :")));
    options->addWidget(m_formationCombo);
    options->addWidget(m_opponents);
    options->addWidget(m_applyFormation);
    options->addStretch(1);

    m_linkLabel = new QLabel;
    m_linkLabel->setWordWrap(true);
    auto *hint = new QLabel(tr("Enregistrement automatique. Clic droit ou Échap : retour à la sélection."));
    hint->setStyleSheet(QStringLiteral("color: gray;"));

    m_editor = new QWidget;
    auto *editorLayout = new QVBoxLayout(m_editor);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->addWidget(toolbar);
    editorLayout->addWidget(actionsBar);
    editorLayout->addLayout(options);
    editorLayout->addWidget(m_view, 1);
    editorLayout->addWidget(m_linkLabel);
    editorLayout->addWidget(hint);

    auto *layout = new QHBoxLayout(this);
    layout->addLayout(left);
    layout->addWidget(m_editor, 1);

    // --- Connexions ---------------------------------------------------------------------
    connect(m_list, &QListWidget::currentRowChanged, this, [this]() { loadCurrent(); });
    connect(m_newButton, &QPushButton::clicked, this, &TacticsPage::newBoard);
    connect(m_propsButton, &QPushButton::clicked, this, &TacticsPage::editProperties);
    connect(m_duplicateButton, &QPushButton::clicked, this, &TacticsPage::duplicateBoard);
    connect(m_deleteButton, &QPushButton::clicked, this, &TacticsPage::deleteBoard);
    connect(m_applyFormation, &QPushButton::clicked, this, &TacticsPage::applyFormation);
    connect(m_scene, &BoardScene::boardEdited, this, [this]() {
        // Annuler / rétablir peuvent changer le type de terrain : on resynchronise la liste déroulante.
        m_pitchCombo->setCurrentIndex(m_pitchCombo->findData(int(m_scene->board().pitch)));
        updateButtons();
        saveCurrent();
    });
    connect(m_pitchCombo, &QComboBox::activated, this, [this]() {
        if (!m_loading)
            m_scene->setPitch(static_cast<Tactics::Pitch>(m_pitchCombo->currentData().toInt()));
    });

    reloadList();
}

void TacticsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Exercices et séances ont pu être modifiés ailleurs : les libellés de lien sont recalculés.
    updateLinkLabel();
}

int TacticsPage::currentRow() const
{
    const int row = m_list->currentRow();
    return (row >= 0 && row < m_boards.size()) ? row : -1;
}

BoardRecord *TacticsPage::currentRecord()
{
    const int row = currentRow();
    return row >= 0 ? &m_boards[row] : nullptr;
}

void TacticsPage::reloadList(int selectId)
{
    QString error;
    const QList<BoardRecord> boards = m_repo.list(&error);
    if (!error.isEmpty()) {
        showError(error);
        return;
    }

    int keepId = selectId;
    if (keepId < 0) {
        if (const BoardRecord *r = currentRecord())
            keepId = r->id;
    }

    m_boards = boards;
    m_list->blockSignals(true);   // un seul chargement, fait explicitement plus bas
    m_list->clear();
    int selectRow = -1;
    for (int i = 0; i < m_boards.size(); ++i) {
        m_list->addItem(m_boards.at(i).title);
        if (m_boards.at(i).id == keepId)
            selectRow = i;
    }
    if (selectRow < 0 && !m_boards.isEmpty())
        selectRow = 0;
    m_list->setCurrentRow(selectRow);
    m_list->blockSignals(false);
    loadCurrent();
}

void TacticsPage::loadCurrent()
{
    if (m_loading)
        return;
    m_loading = true;

    m_scene->undoStack()->clear();
    m_scene->setTool(BoardScene::Tool::Select);
    for (QAction *a : m_toolActions)
        a->setChecked(a->data().toInt() == int(BoardScene::Tool::Select));

    Tactics::Board board;
    m_editable = false;
    if (const BoardRecord *r = currentRecord()) {
        QString error;
        if (Tactics::fromJson(r->sceneJson, &board, &error)) {
            m_editable = true;
        } else {
            board = Tactics::Board();
            QMessageBox::warning(this, tr("Schéma illisible"),
                                 tr("Le contenu de « %1 » ne peut pas être lu :\n%2\n\n"
                                    "Le schéma reste intact dans la base ; il n'est pas modifiable ici.")
                                     .arg(r->title, error));
        }
        m_pitchCombo->setCurrentIndex(m_pitchCombo->findData(int(board.pitch)));
        if (!r->formation.isEmpty() && m_formationCombo->findText(r->formation) >= 0)
            m_formationCombo->setCurrentText(r->formation);
    }
    m_scene->setBoard(board);

    m_loading = false;
    updateButtons();
    updateLinkLabel();
}

void TacticsPage::saveCurrent()
{
    if (m_loading || !m_editable)
        return;
    BoardRecord *r = currentRecord();
    if (!r)
        return;
    r->sceneJson = Tactics::toJson(m_scene->board());
    QString error;
    if (!m_repo.update(*r, &error))
        showError(tr("L'enregistrement automatique a échoué :\n%1").arg(error));
}

void TacticsPage::updateButtons()
{
    const bool hasBoard = currentRow() >= 0;
    m_propsButton->setEnabled(hasBoard);
    m_duplicateButton->setEnabled(hasBoard);
    m_deleteButton->setEnabled(hasBoard);
    m_editor->setEnabled(hasBoard && m_editable);
    const bool full = m_pitchCombo->currentData().toInt() == int(Tactics::Pitch::Full);
    m_formationCombo->setEnabled(full);
    m_opponents->setEnabled(full);
    m_applyFormation->setEnabled(full);
}

void TacticsPage::updateLinkLabel()
{
    const BoardRecord *r = currentRecord();
    if (!r) {
        m_linkLabel->setText(tr("Aucun schéma : créez-en un avec « Nouveau… »."));
        return;
    }
    QString exercise = tr("aucun");
    if (r->exerciseId > 0) {
        exercise = tr("(introuvable)");
        for (const Exercise &e : m_exercises.list(true)) {
            if (e.id == r->exerciseId)
                exercise = e.title;
        }
    }
    QString session = tr("aucune");
    if (r->sessionId > 0) {
        session = tr("(introuvable)");
        for (const SessionChoice &c : sessionChoices(m_db)) {
            if (c.id == r->sessionId)
                session = c.label;
        }
    }
    m_linkLabel->setText(tr("<b>%1</b> — exercice lié : %2 — séance liée : %3")
                             .arg(r->title.toHtmlEscaped(), exercise.toHtmlEscaped(), session.toHtmlEscaped()));
}

void TacticsPage::newBoard()
{
    BoardRecord record;
    record.title = tr("Nouveau schéma");
    BoardPropertiesDialog dialog(m_db, record, true, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    record = dialog.result();
    QString error;
    if (!m_repo.insert(record, &error)) {
        showError(error);
        return;
    }
    reloadList(record.id);
}

void TacticsPage::editProperties()
{
    BoardRecord *r = currentRecord();
    if (!r)
        return;
    BoardPropertiesDialog dialog(m_db, *r, false, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    BoardRecord updated = dialog.result();
    if (m_editable)
        updated.sceneJson = Tactics::toJson(m_scene->board());   // contenu à jour (jamais écrasé si illisible)
    QString error;
    if (!m_repo.update(updated, &error)) {
        showError(error);
        return;
    }
    reloadList(updated.id);   // l'historique d'annulation repart de zéro, comme pour tout rechargement
}

void TacticsPage::duplicateBoard()
{
    const BoardRecord *r = currentRecord();
    if (!r)
        return;
    BoardRecord copy = *r;
    if (m_editable)
        copy.sceneJson = Tactics::toJson(m_scene->board());
    copy.id = -1;
    copy.title = tr("%1 (copie)").arg(r->title);
    QString error;
    if (!m_repo.insert(copy, &error)) {
        showError(error);
        return;
    }
    reloadList(copy.id);
}

void TacticsPage::deleteBoard()
{
    const BoardRecord *r = currentRecord();
    if (!r)
        return;
    const auto answer = QMessageBox::question(this, tr("Supprimer le schéma"),
                                              tr("Supprimer définitivement « %1 » ?").arg(r->title));
    if (answer != QMessageBox::Yes)
        return;
    QString error;
    if (!m_repo.remove(r->id, &error)) {
        showError(error);
        return;
    }
    reloadList();
}

void TacticsPage::exportPng()
{
    const BoardRecord *r = currentRecord();
    if (!r)
        return;
    const QString path = QFileDialog::getSaveFileName(this, tr("Exporter le schéma"),
                                                      safeFileName(r->title) + QStringLiteral(".png"),
                                                      tr("Image PNG (*.png)"));
    if (path.isEmpty())
        return;
    const QImage image = m_scene->renderImage(2000);
    if (!image.save(path, "PNG")) {
        showError(tr("Impossible d'écrire l'image : %1").arg(path));
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        QMessageBox::information(this, tr("Exporter le schéma"), tr("Image enregistrée :\n%1").arg(path));
}

void TacticsPage::applyFormation()
{
    BoardRecord *r = currentRecord();
    if (!r)
        return;
    r->formation = m_formationCombo->currentText();
    m_scene->applyFormation(r->formation, m_opponents->isChecked());
}

void TacticsPage::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Erreur"), message);
}
