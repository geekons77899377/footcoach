#pragma once

#include "core/TacticalBoard.h"

#include <QGraphicsScene>
#include <QImage>
#include <QUndoStack>
#include <functional>

class QGraphicsSceneMouseEvent;
class QKeyEvent;
class TokenItem;
class ArrowItem;

// Scène du tableau tactique : terrain dessiné en fond, jetons (joueurs, adversaires, ballon, plots, buts)
// déplaçables et flèches. Le contenu est entièrement décrit par un Tactics::Board.
//
// Annuler / rétablir : chaque action pousse une commande qui mémorise l'état avant et après
// (le schéma est petit, c'est plus simple et plus sûr que des commandes par élément).
class BoardScene : public QGraphicsScene
{
    Q_OBJECT

public:
    enum class Tool {
        Select,
        Player, Opponent, Ball, Cone, Goal, Text,
        ArrowRun, ArrowPass, ArrowDribble, ArrowCross
    };

    static constexpr qreal kPitchLength = 1050;   // unités de scène (10 par mètre)
    static constexpr qreal kPitchWidth = 680;
    static constexpr qreal kMargin = 30;

    explicit BoardScene(QObject *parent = nullptr);

    QUndoStack *undoStack() { return &m_undo; }

    // Remplace tout le contenu (sans toucher à la pile d'annulation).
    void setBoard(const Tactics::Board &board);
    Tactics::Board board() const;

    void setTool(Tool tool);
    Tool tool() const { return m_tool; }

    // Actions annulables.
    void setPitch(Tactics::Pitch pitch);
    void applyFormation(const QString &name, bool withOpponents);
    void clearAll();
    void deleteSelection();
    // Fait pivoter les buts et plots sélectionnés de `degrees` (sens horaire si positif) ; la sélection est conservée.
    void rotateSelection(double degrees);
    // Multiplie la taille des buts, plots et notes sélectionnés (limitée à Tactics::kMinScale..kMaxScale).
    void scaleSelection(double factor);
    // Change la couleur des joueurs, adversaires, plots et notes sélectionnés ; "" = couleur par défaut.
    void setSelectionColor(const QString &color);
    // Inverse le sens de la courbure des centres sélectionnés (bombée à gauche <-> à droite).
    void flipSelectedCurves();
    void editLabel(TokenItem *token);
    // Nombre d'éléments sélectionnés qui acceptent une couleur / un pivotement et une taille.
    int selectedColorable() const;
    int selectedTransformable() const;

    // Image du schéma (fond compris) de largeur donnée ; hauteur proportionnelle.
    QImage renderImage(int width);

    // Rectangle du terrain dans la scène, selon le type de terrain.
    QRectF pitchRect() const;

signals:
    // Émis après chaque modification de l'utilisateur (ajout, déplacement, suppression, annulation…).
    void boardEdited();
    // Émis quand l'outil change sans passer par setTool() de l'interface (Échap, clic droit).
    void toolChanged(BoardScene::Tool tool);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    friend class BoardCommand;

    QPointF toNormalized(const QPointF &scenePos) const;
    QPointF toScene(const QPointF &normalized) const;
    void pushChange(const Tactics::Board &before, const Tactics::Board &after, const QString &text);
    void reload(const Tactics::Board &board);   // utilisé par les commandes : recrée les éléments puis émet boardEdited
    // Applique `after` à la scène et l'enregistre dans la pile d'annulation.
    void applyChange(const Tactics::Board &after, const QString &text);
    // Modifie les éléments sélectionnés avec `change` (qui renvoie true s'il a touché l'élément),
    // enregistre l'action et conserve la sélection.
    void modifySelection(const QString &text, const std::function<bool(Tactics::Item &)> &change);
    void changeTool(Tool tool);                 // setTool + signal toolChanged

    QUndoStack m_undo;
    Tool m_tool = Tool::Select;
    Tactics::Pitch m_pitch = Tactics::Pitch::Full;
    QList<TokenItem *> m_tokens;
    QList<ArrowItem *> m_arrows;

    // Gestes en cours.
    bool m_hasPress = false;
    Tactics::Board m_beforeGesture;
    QPointF m_pressScenePos;
    ArrowItem *m_preview = nullptr;
};
