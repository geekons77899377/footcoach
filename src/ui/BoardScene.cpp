#include "BoardScene.h"

#include <QFont>
#include <QFontMetricsF>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <functional>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#include <QUndoCommand>
#include <QtMath>
#include <cmath>

using Tactics::ArrowStyle;
using Tactics::Kind;

// ---------------------------------------------------------------------------------------------
// Jeton : joueur, adversaire, ballon, plot ou but.
// ---------------------------------------------------------------------------------------------

class TokenItem : public QGraphicsItem
{
public:
    TokenItem(Kind kind, const QString &label, const QString &color)
        : m_kind(kind), m_label(label), m_color(color)
    {
        setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
        switch (kind) {
        case Kind::Goal: setZValue(2); break;
        case Kind::Cone: setZValue(3); break;
        case Kind::Player:
        case Kind::Opponent: setZValue(4); break;
        case Kind::Ball: setZValue(5); break;
        case Kind::Text: setZValue(6); break;
        }
    }

    Kind kind() const { return m_kind; }
    QString label() const { return m_label; }
    void setLabel(const QString &label)
    {
        prepareGeometryChange();
        m_label = label;
        update();
    }
    bool hasLabel() const { return m_kind == Kind::Player || m_kind == Kind::Opponent || m_kind == Kind::Text; }
    QString color() const { return m_color; }

    // Couleur effective (celle choisie, sinon celle du type d'élément).
    QColor fill() const
    {
        if (QColor c(m_color); !m_color.isEmpty() && c.isValid())
            return c;
        switch (m_kind) {
        case Kind::Player: return QColor(21, 101, 192);
        case Kind::Opponent: return QColor(198, 40, 40);
        case Kind::Cone: return QColor(255, 152, 0);
        default: return QColor(255, 255, 255);
        }
    }

    QRectF boundingRect() const override
    {
        switch (m_kind) {
        case Kind::Player:
        case Kind::Opponent: return QRectF(-19, -19, 38, 38);
        case Kind::Ball: return QRectF(-13, -13, 26, 26);
        case Kind::Cone: return QRectF(-15, -16, 30, 30);
        case Kind::Goal: return QRectF(-12, -40, 24, 80);
        case Kind::Text: return textRect().adjusted(-3, -3, 3, 3);
        }
        return QRectF();
    }

    QPainterPath shape() const override
    {
        QPainterPath path;
        switch (m_kind) {
        case Kind::Player:
        case Kind::Opponent: path.addEllipse(QPointF(0, 0), 16, 16); break;
        case Kind::Ball: path.addEllipse(QPointF(0, 0), 10, 10); break;
        case Kind::Cone: path.addRect(QRectF(-12, -13, 24, 24)); break;
        case Kind::Goal: path.addRect(QRectF(-8, -36, 16, 72)); break;
        case Kind::Text: path.addRect(textRect()); break;
        }
        return path;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->setRenderHint(QPainter::Antialiasing, true);
        if (isSelected()) {
            painter->setPen(QPen(QColor(255, 235, 59), 3, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(boundingRect().adjusted(1, 1, -1, -1));
        }
        switch (m_kind) {
        case Kind::Player:
        case Kind::Opponent: {
            painter->setPen(QPen(Qt::white, 2.5));
            painter->setBrush(fill());
            painter->drawEllipse(QPointF(0, 0), 15, 15);
            if (!m_label.isEmpty()) {
                QFont font;
                font.setBold(true);
                font.setPixelSize(m_label.size() <= 2 ? 16 : 11);
                painter->setFont(font);
                painter->setPen(isLight(fill()) ? QColor(33, 33, 33) : QColor(Qt::white));   // reste lisible sur fond clair
                painter->drawText(QRectF(-15, -15, 30, 30), Qt::AlignCenter, m_label);
            }
            break;
        }
        case Kind::Ball:
            painter->setPen(QPen(Qt::black, 2));
            painter->setBrush(Qt::white);
            painter->drawEllipse(QPointF(0, 0), 9, 9);
            break;
        case Kind::Cone: {
            QPolygonF triangle;
            triangle << QPointF(0, -12) << QPointF(11, 10) << QPointF(-11, 10);
            painter->setPen(QPen(fill().darker(200), 2));
            painter->setBrush(fill());
            painter->drawPolygon(triangle);
            break;
        }
        case Kind::Goal:
            painter->setPen(QPen(Qt::white, 2.5));
            painter->setBrush(QColor(255, 255, 255, 70));
            painter->drawRect(QRectF(-6, -35, 12, 70));
            break;
        case Kind::Text: {
            const QRectF box = textRect();
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(0, 0, 0, 150));
            painter->drawRoundedRect(box, 5, 5);
            painter->setFont(textFont());
            painter->setPen(m_color.isEmpty() ? QColor(Qt::white) : fill());
            painter->drawText(box, Qt::AlignCenter, m_label);
            break;
        }
        }
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        if (change == ItemPositionChange && scene()) {
            // Reste dans le terrain.
            const QRectF pitch = static_cast<BoardScene *>(scene())->pitchRect();
            QPointF p = value.toPointF();
            p.setX(qBound(pitch.left(), p.x(), pitch.right()));
            p.setY(qBound(pitch.top(), p.y(), pitch.bottom()));
            return p;
        }
        return QGraphicsItem::itemChange(change, value);
    }

private:
    static bool isLight(const QColor &c) { return c.lightness() > 170; }

    static QFont textFont()
    {
        QFont font;
        font.setPixelSize(18);
        return font;
    }

    // Cadre de la note, centré sur l'origine de l'élément (texte sur plusieurs lignes possible).
    QRectF textRect() const
    {
        QRectF r = QFontMetricsF(textFont()).boundingRect(QRectF(), Qt::AlignLeft, m_label);
        r.moveCenter(QPointF(0, 0));
        return r.adjusted(-8, -5, 8, 5);
    }

    Kind m_kind;
    QString m_label;
    QString m_color;
};

// ---------------------------------------------------------------------------------------------
// Flèche : course (pointillé), passe (trait plein) ou conduite de balle (ondulé).
// Les extrémités sont en coordonnées de scène ; l'élément lui-même n'est pas déplaçable.
// ---------------------------------------------------------------------------------------------

class ArrowItem : public QGraphicsItem
{
public:
    ArrowItem(ArrowStyle style, const QPointF &from, const QPointF &to, double curve = 0)
        : m_style(style), m_from(from), m_to(to), m_curve(curve)
    {
        setFlag(ItemIsSelectable, true);
        setZValue(1);
    }

    ArrowStyle style() const { return m_style; }
    double curve() const { return m_curve; }
    QPointF from() const { return m_from; }
    QPointF to() const { return m_to; }
    void setEnd(const QPointF &to)
    {
        prepareGeometryChange();
        m_to = to;
    }

    QRectF boundingRect() const override { return fullPath().boundingRect().adjusted(-14, -14, 14, 14); }

    QPainterPath shape() const override
    {
        QPainterPathStroker stroker;
        stroker.setWidth(16);
        return stroker.createStroke(linePath());
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QColor color = isSelected() ? QColor(255, 235, 59) : QColor(255, 255, 255);
        QPen pen(color, 3.5);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        if (m_style == ArrowStyle::Run)
            pen.setDashPattern({2.2, 2.0});   // en multiples de la largeur du trait
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(linePath());

        QPen headPen(color, 1);
        painter->setPen(headPen);
        painter->setBrush(color);
        painter->drawPolygon(headPolygon());
    }

private:
    // Le tracé s'arrête avant la pointe pour ne pas déborder de la tête de flèche.
    QPainterPath linePath() const
    {
        QPainterPath path;
        const QLineF line(m_from, m_to);
        const qreal length = line.length();
        if (length < 1) {
            path.moveTo(m_from);
            return path;
        }
        const qreal headLength = qMin<qreal>(18, length);
        if (m_style == ArrowStyle::Cross) {
            // Courbe de Bézier quadratique ; le tracé s'arrête sous la tête de flèche.
            const QPointF c = control();
            path.moveTo(m_from);
            const int steps = 48;
            for (int i = 1; i <= steps; ++i) {
                const qreal t = qreal(i) / steps;
                const QPointF p = bezier(t, c);
                if (QLineF(p, m_to).length() < headLength * 0.6)
                    break;
                path.lineTo(p);
            }
            return path;
        }
        const qreal bodyLength = length - headLength * 0.6;
        QLineF body = line;
        body.setLength(bodyLength);

        path.moveTo(m_from);
        if (m_style == ArrowStyle::Dribble) {
            // Sinusoïde : amplitude 6, période ≈ 28.
            const QLineF normal = body.normalVector().unitVector();
            const qreal steps = qMax<qreal>(8, bodyLength / 2);
            for (int i = 1; i <= int(steps); ++i) {
                const qreal t = i / steps;
                const QPointF base = body.pointAt(t);
                const qreal offset = 6 * qSin(t * bodyLength / 28 * 2 * M_PI) * qMin<qreal>(1, t * bodyLength / 14);
                path.lineTo(base + QPointF(normal.dx(), normal.dy()) * offset);
            }
        } else {
            path.lineTo(body.p2());
        }
        return path;
    }

    QPolygonF headPolygon() const
    {
        const QLineF line(m_from, m_to);
        if (line.length() < 1)
            return QPolygonF();
        // Pour un centre, la tête suit la tangente de la courbe à son extrémité.
        const QPointF reference = m_style == ArrowStyle::Cross ? control() : m_from;
        const qreal angle = qAtan2(m_to.y() - reference.y(), m_to.x() - reference.x());
        const qreal size = qMin<qreal>(18, line.length());
        QPolygonF head;
        head << m_to
             << m_to - QPointF(qCos(angle - M_PI / 7) * size, qSin(angle - M_PI / 7) * size)
             << m_to - QPointF(qCos(angle + M_PI / 7) * size, qSin(angle + M_PI / 7) * size);
        return head;
    }

    // Point de contrôle de la courbe : au milieu de la corde, décalé vers la gauche du sens de la flèche.
    QPointF control() const
    {
        const QLineF line(m_from, m_to);
        const QPointF mid = (m_from + m_to) / 2;
        if (line.length() < 1)
            return mid;
        const QPointF normal(line.dy() / line.length(), -line.dx() / line.length());   // gauche (y vers le bas)
        // Une Bézier quadratique atteint la moitié du décalage du point de contrôle : on double pour que
        // la courbure soit bien la flèche (hauteur de l'arc) en fraction de la longueur.
        return mid + normal * (2 * m_curve * line.length());
    }

    QPointF bezier(qreal t, const QPointF &c) const
    {
        const qreal u = 1 - t;
        return u * u * m_from + 2 * u * t * c + t * t * m_to;
    }

    QPainterPath fullPath() const
    {
        QPainterPath path = linePath();
        QPainterPath head;
        head.addPolygon(headPolygon());
        return path.united(head);
    }

    ArrowStyle m_style;
    QPointF m_from;
    QPointF m_to;
    double m_curve;
};

// ---------------------------------------------------------------------------------------------
// Commande d'annulation : mémorise l'état avant et après.
// ---------------------------------------------------------------------------------------------

class BoardCommand : public QUndoCommand
{
public:
    BoardCommand(BoardScene *scene, const Tactics::Board &before, const Tactics::Board &after, const QString &text)
        : QUndoCommand(text), m_scene(scene), m_before(before), m_after(after)
    {
    }

    void undo() override { m_scene->reload(m_before); }
    void redo() override
    {
        if (m_first) {   // l'état « après » est déjà affiché quand la commande est empilée
            m_first = false;
            return;
        }
        m_scene->reload(m_after);
    }

private:
    BoardScene *m_scene;
    Tactics::Board m_before;
    Tactics::Board m_after;
    bool m_first = true;
};

// ---------------------------------------------------------------------------------------------
// BoardScene
// ---------------------------------------------------------------------------------------------

BoardScene::BoardScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setSceneRect(pitchRect().adjusted(-kMargin, -kMargin, kMargin, kMargin));
}

QRectF BoardScene::pitchRect() const
{
    // Demi-terrain : on regarde vers le but visé, placé en haut (rectangle de 68 m de large sur 52,5 m de haut).
    if (m_pitch == Tactics::Pitch::Half)
        return QRectF(kMargin, kMargin, kPitchWidth, kPitchLength / 2);
    return QRectF(kMargin, kMargin, kPitchLength, kPitchWidth);
}

QPointF BoardScene::toNormalized(const QPointF &scenePos) const
{
    const QRectF r = pitchRect();
    const qreal u = qBound(0.0, (scenePos.x() - r.left()) / r.width(), 1.0);    // 0..1, de gauche à droite de l'écran
    const qreal v = qBound(0.0, (scenePos.y() - r.top()) / r.height(), 1.0);    // 0..1, de haut en bas de l'écran
    if (m_pitch == Tactics::Pitch::Half)
        return QPointF(1.0 - v, u);   // x : vers le but (en haut) ; y : de la gauche vers la droite du sens de jeu
    return QPointF(u, v);
}

QPointF BoardScene::toScene(const QPointF &n) const
{
    const QRectF r = pitchRect();
    if (m_pitch == Tactics::Pitch::Half)
        return QPointF(r.left() + n.y() * r.width(), r.top() + (1.0 - n.x()) * r.height());
    return QPointF(r.left() + n.x() * r.width(), r.top() + n.y() * r.height());
}

void BoardScene::setBoard(const Tactics::Board &board)
{
    // Désélectionne d'abord : la suppression d'éléments sélectionnés n'a alors aucun effet de bord.
    clearSelection();
    qDeleteAll(m_tokens);
    qDeleteAll(m_arrows);
    m_tokens.clear();
    m_arrows.clear();
    m_hasPress = false;
    delete m_preview;      // flèche en cours de tracé, le cas échéant
    m_preview = nullptr;

    m_pitch = board.pitch;
    setSceneRect(pitchRect().adjusted(-kMargin, -kMargin, kMargin, kMargin));

    for (const Tactics::Item &it : board.items) {
        auto *token = new TokenItem(it.kind, it.label, it.color);
        addItem(token);
        token->setPos(toScene(it.pos));
        token->setRotation(it.rotation);
        token->setScale(it.scale);
        m_tokens.append(token);
    }
    for (const Tactics::Arrow &a : board.arrows) {
        auto *arrow = new ArrowItem(a.style, toScene(a.from), toScene(a.to), a.curve);
        addItem(arrow);
        m_arrows.append(arrow);
    }
    update();
}

Tactics::Board BoardScene::board() const
{
    Tactics::Board b;
    b.pitch = m_pitch;
    for (const TokenItem *t : m_tokens) {
        Tactics::Item it;
        it.kind = t->kind();
        it.pos = toNormalized(t->pos());
        it.label = t->label();
        it.rotation = Tactics::isRotatable(it.kind) ? t->rotation() : 0;
        it.scale = Tactics::isScalable(it.kind) ? t->scale() : 1.0;
        it.color = Tactics::isColorable(it.kind) ? t->color() : QString();
        b.items.append(it);
    }
    for (const ArrowItem *a : m_arrows) {
        Tactics::Arrow arrow;
        arrow.style = a->style();
        arrow.from = toNormalized(a->from());
        arrow.to = toNormalized(a->to());
        arrow.curve = a->curve();
        b.arrows.append(arrow);
    }
    return b;
}

void BoardScene::reload(const Tactics::Board &board)
{
    setBoard(board);
    emit boardEdited();
}

void BoardScene::pushChange(const Tactics::Board &before, const Tactics::Board &after, const QString &text)
{
    m_undo.push(new BoardCommand(this, before, after, text));
    emit boardEdited();
}

void BoardScene::applyChange(const Tactics::Board &after, const QString &text)
{
    const Tactics::Board before = board();
    if (before == after)
        return;
    setBoard(after);
    pushChange(before, after, text);
}

void BoardScene::setTool(Tool tool)
{
    m_tool = tool;
    // Un geste d'arc commencé n'est pas abandonné ici : il se termine au relâchement de la souris.
}

void BoardScene::changeTool(Tool tool)
{
    setTool(tool);
    emit toolChanged(tool);
}

void BoardScene::setPitch(Tactics::Pitch pitch)
{
    Tactics::Board after = board();
    if (after.pitch == pitch)
        return;
    after.pitch = pitch;
    applyChange(after, tr("Changer de terrain"));
}

void BoardScene::applyFormation(const QString &name, bool withOpponents)
{
    if (m_pitch != Tactics::Pitch::Full)
        return;
    const QList<Tactics::Item> players = Tactics::formation(name);
    if (players.isEmpty())
        return;

    Tactics::Board after = board();
    QList<Tactics::Item> kept;
    for (const Tactics::Item &it : after.items) {
        if (it.kind == Kind::Player || (withOpponents && it.kind == Kind::Opponent))
            continue;
        kept.append(it);
    }
    kept.append(players);
    if (withOpponents)
        kept.append(Tactics::mirroredAsOpponent(players));
    after.items = kept;
    applyChange(after, tr("Formation %1").arg(name));
}

void BoardScene::clearAll()
{
    Tactics::Board after;
    after.pitch = m_pitch;
    applyChange(after, tr("Tout effacer"));
}

void BoardScene::deleteSelection()
{
    Tactics::Board after;
    after.pitch = m_pitch;
    bool removed = false;
    const Tactics::Board current = board();
    for (int i = 0; i < m_tokens.size(); ++i) {
        if (m_tokens.at(i)->isSelected())
            removed = true;
        else
            after.items.append(current.items.at(i));
    }
    for (int i = 0; i < m_arrows.size(); ++i) {
        if (m_arrows.at(i)->isSelected())
            removed = true;
        else
            after.arrows.append(current.arrows.at(i));
    }
    if (removed)
        applyChange(after, tr("Supprimer"));
}

void BoardScene::modifySelection(const QString &text, const std::function<bool(Tactics::Item &)> &change)
{
    Tactics::Board after = board();
    QList<int> selected;   // indices (l'ordre des jetons est conservé par le rechargement)
    bool changed = false;
    for (int i = 0; i < m_tokens.size(); ++i) {
        if (!m_tokens.at(i)->isSelected())
            continue;
        selected.append(i);
        if (change(after.items[i]))
            changed = true;
    }
    if (!changed)
        return;
    const Tactics::Board before = board();
    if (after == before)
        return;   // rien n'a vraiment changé (taille au maximum, même couleur…)
    applyChange(after, text);
    for (int i : selected)
        m_tokens.at(i)->setSelected(true);
}

void BoardScene::rotateSelection(double degrees)
{
    modifySelection(tr("Pivoter"), [degrees](Tactics::Item &item) {
        if (!Tactics::isRotatable(item.kind))
            return false;
        item.rotation = std::fmod(item.rotation + degrees + 360.0 * 4, 360.0);
        return true;
    });
}

void BoardScene::scaleSelection(double factor)
{
    modifySelection(tr("Changer la taille"), [factor](Tactics::Item &item) {
        if (!Tactics::isScalable(item.kind))
            return false;
        const double scaled = qRound(item.scale * factor * 100) / 100.0;   // arrondi : pas de dérive après plusieurs pas
        item.scale = qBound(Tactics::kMinScale, scaled, Tactics::kMaxScale);
        return true;
    });
}

void BoardScene::setSelectionColor(const QString &color)
{
    modifySelection(tr("Changer la couleur"), [color](Tactics::Item &item) {
        if (!Tactics::isColorable(item.kind))
            return false;
        item.color = color;
        return true;
    });
}

void BoardScene::flipSelectedCurves()
{
    Tactics::Board after = board();
    QList<int> selected;
    bool changed = false;
    for (int i = 0; i < m_arrows.size(); ++i) {
        if (!m_arrows.at(i)->isSelected())
            continue;
        selected.append(i);
        if (after.arrows.at(i).style == ArrowStyle::Cross) {
            after.arrows[i].curve = -after.arrows.at(i).curve;
            changed = true;
        }
    }
    if (!changed)
        return;
    applyChange(after, tr("Inverser la courbure"));
    for (int i : selected)
        m_arrows.at(i)->setSelected(true);
}

int BoardScene::selectedColorable() const
{
    int n = 0;
    for (const TokenItem *t : m_tokens)
        n += (t->isSelected() && Tactics::isColorable(t->kind())) ? 1 : 0;
    return n;
}

int BoardScene::selectedTransformable() const
{
    int n = 0;
    for (const TokenItem *t : m_tokens)
        n += (t->isSelected() && Tactics::isScalable(t->kind())) ? 1 : 0;
    return n;
}

void BoardScene::editLabel(TokenItem *token)
{
    if (!token || !token->hasLabel())
        return;
    QWidget *parent = views().isEmpty() ? nullptr : views().first();
    bool ok = false;
    if (token->kind() == Kind::Text) {
        const QString note = QInputDialog::getMultiLineText(parent, tr("Note"), tr("Texte de la note :"),
                                                            token->label(), &ok).trimmed().left(300);
        const int idx = m_tokens.indexOf(token);
        if (!ok || note.isEmpty() || note == token->label() || idx < 0)
            return;
        Tactics::Board changed = board();
        changed.items[idx].label = note;
        applyChange(changed, tr("Modifier la note"));
        return;
    }
    QString text = QInputDialog::getText(parent, tr("Étiquette"), tr("Numéro ou texte court (3 caractères maximum) :"),
                                         QLineEdit::Normal, token->label(), &ok);
    if (!ok)
        return;
    text = text.trimmed().left(3);
    if (text == token->label())
        return;

    Tactics::Board after = board();
    const int index = m_tokens.indexOf(token);
    if (index < 0)
        return;
    after.items[index].label = text;
    applyChange(after, tr("Modifier l'étiquette"));
}

QImage BoardScene::renderImage(int width)
{
    const QRectF source = sceneRect();
    const int height = qRound(width * source.height() / source.width());
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    // Les contours de sélection ne doivent pas apparaître sur l'image.
    const QList<QGraphicsItem *> selected = selectedItems();
    clearSelection();
    {
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        render(&painter, QRectF(0, 0, width, height), source);
    }
    for (QGraphicsItem *item : selected)
        item->setSelected(true);
    return image;
}

// --- Terrain ---------------------------------------------------------------------------------

void BoardScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->fillRect(rect, QColor(36, 90, 43));

    const QRectF pr = pitchRect();
    const bool half = m_pitch == Tactics::Pitch::Half;
    const QRectF all = sceneRect();

    // Le terrain est dessiné en coordonnées « virtuelles » (terrain entier, horizontal, en dixièmes de mètre).
    // Demi-terrain : on tourne le repère pour que le but visé soit en haut, et on ne garde que la moitié visible.
    QTransform fieldToScene;
    if (half)
        fieldToScene = QTransform(0, -1, 1, 0, pr.left(), pr.top() + kPitchLength);   // (vx, vy) -> (vy, 1050 - vx)
    else
        fieldToScene = QTransform::fromTranslate(pr.left(), pr.top());

    // Bandes de pelouse (10 bandes sur la longueur du terrain entier), limitées au terrain affiché.
    painter->save();
    painter->setClipRect(pr);
    painter->setTransform(fieldToScene, true);
    const qreal stripe = kPitchLength / 10;
    for (int i = 0; i < 10; ++i)
        painter->fillRect(QRectF(i * stripe, 0, stripe, kPitchWidth),
                          i % 2 == 0 ? QColor(46, 125, 50) : QColor(52, 140, 57));
    painter->restore();

    painter->save();
    // Lignes : rien au-delà de la ligne médiane en demi-terrain (le reste du repère est hors de la zone visible).
    painter->setClipRect(half ? QRectF(all.left(), all.top(), all.width(), pr.bottom() - all.top()) : all);
    painter->setTransform(fieldToScene, true);

    const QPen line(QColor(255, 255, 255, 235), 2.5);
    painter->setPen(line);
    painter->setBrush(Qt::NoBrush);

    const qreal L = kPitchLength;
    const qreal W = kPitchWidth;
    painter->drawRect(QRectF(0, 0, L, W));
    painter->drawLine(QPointF(L / 2, 0), QPointF(L / 2, W));
    painter->drawEllipse(QPointF(L / 2, W / 2), 91.5, 91.5);

    // Surfaces de réparation et de but, des deux côtés.
    const qreal penaltyDepth = 165, penaltyHalf = 201.6, goalDepth = 55, goalHalf = 91.6;
    painter->drawRect(QRectF(0, W / 2 - penaltyHalf, penaltyDepth, 2 * penaltyHalf));
    painter->drawRect(QRectF(L - penaltyDepth, W / 2 - penaltyHalf, penaltyDepth, 2 * penaltyHalf));
    painter->drawRect(QRectF(0, W / 2 - goalHalf, goalDepth, 2 * goalHalf));
    painter->drawRect(QRectF(L - goalDepth, W / 2 - goalHalf, goalDepth, 2 * goalHalf));

    // Arcs de cercle des surfaces (rayon 9,15 m autour du point de penalty, à 11 m de la ligne).
    // cos(a) = (165 - 110) / 91.5 -> a ≈ 53°.
    const qreal arcAngle = qRadiansToDegrees(qAcos((penaltyDepth - 110) / 91.5));
    painter->drawArc(QRectF(110 - 91.5, W / 2 - 91.5, 183, 183), qRound(-arcAngle * 16), qRound(2 * arcAngle * 16));
    painter->drawArc(QRectF(L - 110 - 91.5, W / 2 - 91.5, 183, 183), qRound((180 - arcAngle) * 16),
                     qRound(2 * arcAngle * 16));

    // Points : centre et penalties.
    painter->setBrush(line.color());
    painter->drawEllipse(QPointF(L / 2, W / 2), 3, 3);
    painter->drawEllipse(QPointF(110, W / 2), 3, 3);
    painter->drawEllipse(QPointF(L - 110, W / 2), 3, 3);
    painter->setBrush(Qt::NoBrush);

    // Arcs de corner (rayon 1 m).
    painter->drawArc(QRectF(-10, -10, 20, 20), 270 * 16, 90 * 16);
    painter->drawArc(QRectF(L - 10, -10, 20, 20), 180 * 16, 90 * 16);
    painter->drawArc(QRectF(L - 10, W - 10, 20, 20), 90 * 16, 90 * 16);
    painter->drawArc(QRectF(-10, W - 10, 20, 20), 0 * 16, 90 * 16);

    // Buts (7,32 m de large, 2,4 m de profondeur), dessinés derrière les lignes de but.
    painter->setBrush(QColor(255, 255, 255, 60));
    painter->drawRect(QRectF(-24, W / 2 - 36.6, 24, 73.2));
    painter->drawRect(QRectF(L, W / 2 - 36.6, 24, 73.2));

    painter->restore();
}

// --- Souris et clavier -----------------------------------------------------------------------

void BoardScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::RightButton && m_tool != Tool::Select) {
        changeTool(Tool::Select);
        event->accept();
        return;
    }
    if (event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    if (m_tool == Tool::Select) {
        m_beforeGesture = board();
        m_hasPress = true;
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    const QRectF pr = pitchRect();
    QPointF p = event->scenePos();
    p.setX(qBound(pr.left(), p.x(), pr.right()));
    p.setY(qBound(pr.top(), p.y(), pr.bottom()));

    if (m_tool == Tool::ArrowRun || m_tool == Tool::ArrowPass || m_tool == Tool::ArrowDribble
        || m_tool == Tool::ArrowCross) {
        const ArrowStyle style = m_tool == Tool::ArrowRun ? ArrowStyle::Run
                                 : m_tool == Tool::ArrowPass ? ArrowStyle::Pass
                                 : m_tool == Tool::ArrowCross ? ArrowStyle::Cross : ArrowStyle::Dribble;
        m_pressScenePos = p;
        m_preview = new ArrowItem(style, p, p, style == ArrowStyle::Cross ? Tactics::kDefaultCurve : 0.0);
        addItem(m_preview);
        event->accept();
        return;
    }

    // Outils de pose : un jeton à l'endroit du clic.
    Tactics::Board after = board();
    Tactics::Item item;
    item.pos = toNormalized(p);
    QString text;
    if (m_tool == Tool::Goal && m_pitch == Tactics::Pitch::Half)
        item.rotation = 90;   // en demi-terrain, la ligne de but est horizontale
    switch (m_tool) {
    case Tool::Player:
        item.kind = Kind::Player;
        if (const int n = Tactics::nextFreePlayerNumber(after.items))
            item.label = QString::number(n);
        text = tr("Ajouter un joueur");
        break;
    case Tool::Opponent:
        item.kind = Kind::Opponent;
        text = tr("Ajouter un adversaire");
        break;
    case Tool::Ball:
        item.kind = Kind::Ball;
        text = tr("Ajouter un ballon");
        break;
    case Tool::Cone:
        item.kind = Kind::Cone;
        text = tr("Ajouter un plot");
        break;
    case Tool::Goal:
        item.kind = Kind::Goal;
        text = tr("Ajouter un but");
        break;
    case Tool::Text: {
        QWidget *parent = views().isEmpty() ? nullptr : views().first();
        bool ok = false;
        const QString note = QInputDialog::getMultiLineText(parent, tr("Note"), tr("Texte de la note :"),
                                                            QString(), &ok).trimmed().left(300);
        if (!ok || note.isEmpty()) {
            event->accept();
            return;
        }
        item.kind = Kind::Text;
        item.label = note;
        text = tr("Ajouter une note");
        break;
    }
    default:
        return;
    }
    after.items.append(item);
    applyChange(after, text);
    event->accept();
}

void BoardScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_preview) {
        const QRectF pr = pitchRect();
        QPointF p = event->scenePos();
        p.setX(qBound(pr.left(), p.x(), pr.right()));
        p.setY(qBound(pr.top(), p.y(), pr.bottom()));
        m_preview->setEnd(p);
        event->accept();
        return;
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void BoardScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_preview) {
        ArrowItem *preview = m_preview;
        m_preview = nullptr;
        const ArrowStyle style = preview->style();
        const double curve = preview->curve();
        const QPointF from = preview->from();
        const QPointF to = preview->to();
        removeItem(preview);
        delete preview;

        if (QLineF(from, to).length() >= 20) {
            Tactics::Board after = board();
            Tactics::Arrow arrow;
            arrow.style = style;
            arrow.from = toNormalized(from);
            arrow.to = toNormalized(to);
            arrow.curve = curve;
            after.arrows.append(arrow);
            applyChange(after, tr("Ajouter une flèche"));
        }
        event->accept();
        return;
    }

    QGraphicsScene::mouseReleaseEvent(event);

    if (m_hasPress) {
        m_hasPress = false;
        const Tactics::Board now = board();
        if (!(now == m_beforeGesture))
            pushChange(m_beforeGesture, now, tr("Déplacer"));
    }
}

void BoardScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_tool == Tool::Select && event->button() == Qt::LeftButton) {
        if (auto *token = dynamic_cast<TokenItem *>(itemAt(event->scenePos(), QTransform()))) {
            editLabel(token);
            event->accept();
            return;
        }
    }
    QGraphicsScene::mouseDoubleClickEvent(event);
}

void BoardScene::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelection();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        scaleSelection(1.25);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Minus) {
        scaleSelection(1.0 / 1.25);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_C) {
        flipSelectedCurves();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_R) {
        rotateSelection(event->modifiers() & Qt::ShiftModifier ? -45 : 45);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape && m_tool != Tool::Select) {
        changeTool(Tool::Select);
        event->accept();
        return;
    }
    QGraphicsScene::keyPressEvent(event);
}
