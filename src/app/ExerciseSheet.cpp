#include "ExerciseSheet.h"

#include "core/PlayerRepository.h"

#include <QFontMetricsF>
#include <QIODevice>
#include <QLinearGradient>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPainterPath>
#include <QPdfWriter>
#include <QRadialGradient>
#include <QRegularExpression>
#include <QtMath>

#include <algorithm>
#include <cmath>

// Repère logique 1536 x 1086 (rapport A4 paysage). Tout est dessiné en vectoriel : le PDF reste net à l'impression.

namespace {

using namespace Tactics;

const qreal W = ExerciseSheet::kWidth, H = ExerciseSheet::kHeight;
const QColor kNavy("#0E3A6B"), kNavyDark("#0A2C52"), kBg("#EEF4FB"), kSky("#CFE7F8"), kGreen("#43A535"),
    kGrass1("#4FA232"), kGrass2("#5CB13A"), kYellow("#FFD43B"), kGold("#F2B705"), kText("#14243A"),
    kBlue("#1F6FD1"), kOrange("#F57C00"), kCheck("#2EAD4B"), kSkin("#F1C29A"), kHair("#3B2A20"), kMuted("#6B7C90");

// Dimensions du terrain (mètres), mêmes valeurs que BoardGeometry.js.
const qreal kFieldL = 105, kFieldW = 68, kHalfL = 52.5;

QFont font(qreal px, int weight = QFont::DemiBold)
{
    QFont f(QStringLiteral("sans-serif"));
    f.setPixelSize(std::max(1, qRound(px)));
    f.setWeight(QFont::Weight(weight));
    return f;
}

void roundRect(QPainter &p, const QRectF &r, qreal rad, const QColor &fill, const QColor &stroke = Qt::transparent,
               qreal sw = 0)
{
    p.setPen(stroke.alpha() ? QPen(stroke, sw) : QPen(Qt::NoPen));
    p.setBrush(fill);
    p.drawRoundedRect(r, rad, rad);
}

// Texte sur une ligne, réduit jusqu'à minPx pour tenir dans la largeur, puis élidé.
void fitLine(QPainter &p, const QRectF &r, const QString &s, Qt::Alignment align, qreal maxPx, qreal minPx,
             int weight, const QColor &color)
{
    qreal px = maxPx;
    QFont f = font(px, weight);
    while (px > minPx && QFontMetricsF(f).horizontalAdvance(s) > r.width()) {
        px -= 1;
        f = font(px, weight);
    }
    p.setFont(f);
    p.setPen(color);
    p.drawText(r, align | Qt::AlignVCenter, QFontMetricsF(f).elidedText(s, Qt::ElideRight, r.width()));
}

// Texte sur plusieurs lignes, réduit jusqu'à minPx pour tenir dans le rectangle.
void fitParagraph(QPainter &p, const QRectF &r, const QString &s, qreal maxPx, qreal minPx, int weight,
                  const QColor &color)
{
    qreal px = maxPx;
    QFont f = font(px, weight);
    const int flags = Qt::AlignCenter | Qt::TextWordWrap;
    while (px > minPx && QFontMetricsF(f).boundingRect(r, flags, s).height() > r.height()) {
        px -= 1;
        f = font(px, weight);
    }
    p.save();
    p.setClipRect(r);
    p.setFont(f);
    p.setPen(color);
    p.drawText(r, flags, s);
    p.restore();
}

// ---------- éléments du terrain ----------

void cone(QPainter &p, QPointF base, qreal h, const QColor &c)
{
    const qreal w = h * 0.78;
    QPainterPath body;
    body.moveTo(base.x() - w / 2, base.y());
    body.lineTo(base.x() - w * 0.12, base.y() - h);
    body.quadTo(base.x(), base.y() - h * 1.06, base.x() + w * 0.12, base.y() - h);
    body.lineTo(base.x() + w / 2, base.y());
    body.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 55));
    p.drawEllipse(QPointF(base.x() + w * 0.18, base.y() + 2), w * 0.62, h * 0.16);
    p.setBrush(c.darker(130));
    p.drawEllipse(base, w * 0.62, h * 0.14);
    QLinearGradient g(base.x() - w / 2, 0, base.x() + w / 2, 0);
    g.setColorAt(0, c.lighter(125));
    g.setColorAt(0.55, c);
    g.setColorAt(1, c.darker(135));
    p.setBrush(g);
    p.drawPath(body);
    p.setBrush(QColor(255, 255, 255, 70));
    QPainterPath shine;
    shine.moveTo(base.x() - w * 0.28, base.y() - h * 0.1);
    shine.lineTo(base.x() - w * 0.07, base.y() - h * 0.92);
    shine.lineTo(base.x() - w * 0.01, base.y() - h * 0.92);
    shine.lineTo(base.x() - w * 0.14, base.y() - h * 0.1);
    p.drawPath(shine);
}

void ball(QPainter &p, QPointF c, qreal r)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 60));
    p.drawEllipse(c + QPointF(r * 0.3, r * 0.85), r * 0.9, r * 0.3);
    p.setBrush(Qt::white);
    p.drawEllipse(c, r, r);
    const auto pentagon = [](QPointF center, qreal rad, qreal rotDeg) {
        QPolygonF poly;
        for (int i = 0; i < 5; ++i) {
            const qreal a = qDegreesToRadians(rotDeg + i * 72.0);
            poly << center + QPointF(qCos(a), qSin(a)) * rad;
        }
        return poly;
    };
    p.save();
    QPainterPath clip;
    clip.addEllipse(c, r, r);
    p.setClipPath(clip);
    const QColor ink("#1B1B1B");
    p.setBrush(ink);
    p.drawPolygon(pentagon(c, r * 0.34, -90));
    for (int i = 0; i < 5; ++i) {
        const qreal a = qDegreesToRadians(-90.0 + i * 72.0);
        const QPointF dir(qCos(a), qSin(a));
        p.setPen(QPen(ink, r * 0.07));
        p.drawLine(c + dir * r * 0.34, c + dir * r * 0.72);
        p.setPen(Qt::NoPen);
        p.drawPolygon(pentagon(c + dir * r * 0.98, r * 0.3, -90 + i * 72.0 + 36));
    }
    p.restore();
    p.setPen(QPen(QColor("#222222"), r * 0.12));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, r, r);
}

// Joueur stylisé, pieds en « foot », environ 110 unités de haut à l'échelle 1.
void figure(QPainter &p, QPointF foot, qreal s, const QColor &shirt, const QString &label)
{
    p.save();
    p.translate(foot);
    p.scale(s, s);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 55));
    p.drawEllipse(QPointF(6, 2), 26, 6);
    p.setPen(QPen(kSkin.darker(105), 7, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(-6, -34), QPointF(-14, -6));
    p.drawLine(QPointF(6, -34), QPointF(12, -8));
    p.setPen(QPen(Qt::white, 7, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(-12, -16), QPointF(-14, -6));
    p.drawLine(QPointF(10, -18), QPointF(12, -8));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#1A1A1A"));
    p.drawRoundedRect(QRectF(-21, -7, 13, 6), 3, 3);
    p.drawRoundedRect(QRectF(8, -9, 13, 6), 3, 3);
    p.setBrush(kNavyDark);
    p.drawRoundedRect(QRectF(-13, -46, 26, 16), 4, 4);
    QLinearGradient jg(-16, 0, 16, 0);
    jg.setColorAt(0, shirt.lighter(120));
    jg.setColorAt(1, shirt.darker(115));
    p.setBrush(jg);
    p.drawRoundedRect(QRectF(-15, -78, 30, 36), 8, 8);
    p.setPen(QPen(kSkin, 6, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(-14, -70), QPointF(-24, -50));
    p.drawLine(QPointF(14, -70), QPointF(24, -56));
    p.setPen(QPen(shirt, 8, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(-13, -72), QPointF(-18, -64));
    p.drawLine(QPointF(13, -72), QPointF(18, -64));
    p.setPen(Qt::NoPen);
    p.setBrush(kSkin);
    p.drawEllipse(QPointF(0, -90), 11, 12);
    p.setBrush(kHair);
    QPainterPath hair;
    hair.moveTo(-11, -91);
    hair.cubicTo(-12, -106, 12, -108, 11, -92);
    hair.cubicTo(5, -98, -4, -98, -11, -91);
    p.drawPath(hair);
    if (!label.isEmpty()) {
        p.setFont(font(label.size() > 2 ? 11 : 15, QFont::Bold));
        p.setPen(shirt.lightness() > 170 ? kText : QColor(Qt::white));
        p.drawText(QRectF(-15, -78, 30, 34), Qt::AlignCenter, label);
    }
    p.restore();
}

void tag(QPainter &p, QPointF center, const QString &s, qreal px, qreal rot = 0, const QColor &textColor = Qt::white)
{
    p.save();
    p.translate(center);
    p.rotate(rot);
    const QFont f = font(px, QFont::Bold);
    const QFontMetricsF fm(f);
    const qreal tw = fm.horizontalAdvance(s);
    const QRectF r(-tw / 2 - 12, -fm.height() / 2 - 3, tw + 24, fm.height() + 6);
    roundRect(p, r, 8, kNavy);
    p.setPen(textColor);
    p.setFont(f);
    p.drawText(r, Qt::AlignCenter, s);
    p.restore();
}

// Tracé d'une flèche en coordonnées écran : reprise de arrowGeometry (BoardGeometry.js).
void arrow(QPainter &p, const Arrow &a, QPointF p1, QPointF p2, qreal width, qreal head)
{
    const qreal dx = p2.x() - p1.x(), dy = p2.y() - p1.y();
    const qreal length = std::hypot(dx, dy);
    if (length < 1)
        return;
    const qreal headLength = std::min(head, length);
    const qreal nx = dy / length, ny = -dx / length;
    QPolygonF line;
    line << p1;
    QPointF ref = p1;
    if (a.style == ArrowStyle::Cross) {
        const QPointF c((p1.x() + p2.x()) / 2 + nx * (2 * a.curve * length), (p1.y() + p2.y()) / 2 + ny * (2 * a.curve * length));
        for (int i = 1; i <= 48; ++i) {
            const qreal t = i / 48.0, u = 1 - t;
            const QPointF pt = u * u * p1 + 2 * u * t * c + t * t * p2;
            if (std::hypot(pt.x() - p2.x(), pt.y() - p2.y()) < headLength * 0.6)
                break;
            line << pt;
        }
        ref = c;
    } else {
        const qreal body = length - headLength * 0.6;
        const QPointF e = p1 + QPointF(dx, dy) / length * body;
        if (a.style == ArrowStyle::Dribble) {
            const qreal amp = width * 1.4, period = width * 7;
            const int steps = std::max(8, int(body / 2));
            for (int j = 1; j <= steps; ++j) {
                const qreal t = qreal(j) / steps;
                const QPointF b = p1 + (e - p1) * t;
                const qreal off = amp * std::sin(t * body / period * 2 * M_PI) * std::min(1.0, t * body / (period / 2));
                line << b + QPointF(nx, ny) * off;
            }
        } else {
            line << e;
        }
    }
    QPen pen(Qt::white, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    if (a.style == ArrowStyle::Run) {
        pen.setStyle(Qt::CustomDashLine);
        pen.setDashPattern({2.2, 2.0});
    }
    p.setPen(QPen(QColor(0, 0, 0, 50), width + 2, pen.style(), Qt::RoundCap, Qt::RoundJoin));
    if (a.style == ArrowStyle::Run) {
        QPen sh = p.pen();
        sh.setDashPattern({2.2 * width / (width + 2), 2.0 * width / (width + 2)});
        p.setPen(sh);
    }
    p.setBrush(Qt::NoBrush);
    p.drawPolyline(line.translated(1.5, 1.5));
    p.setPen(pen);
    p.drawPolyline(line);
    const qreal ang = std::atan2(p2.y() - ref.y(), p2.x() - ref.x());
    QPolygonF tri;
    tri << p2 << p2 - QPointF(std::cos(ang - M_PI / 7), std::sin(ang - M_PI / 7)) * headLength
        << p2 - QPointF(std::cos(ang + M_PI / 7), std::sin(ang + M_PI / 7)) * headLength;
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawPolygon(tri);
}

QPolygonF arcPts(qreal cx, qreal cy, qreal r, qreal a0, qreal a1, int n)
{
    QPolygonF out;
    for (int i = 0; i <= n; ++i) {
        const qreal a = qDegreesToRadians(a0 + (a1 - a0) * i / n);
        out << QPointF(cx + r * std::cos(a), cy + r * std::sin(a));
    }
    return out;
}

QPolygonF rectPts(qreal x, qreal y, qreal w, qreal h)
{
    return QPolygonF({QPointF(x, y), QPointF(x + w, y), QPointF(x + w, y + h), QPointF(x, y + h), QPointF(x, y)});
}

QColor itemColor(const Item &it)
{
    if (!it.color.isEmpty() && QColor::isValidColorName(it.color))
        return QColor(it.color);
    switch (it.kind) {
    case Kind::Player: return QColor("#1565c0");
    case Kind::Opponent: return QColor("#c62828");
    case Kind::Cone: return QColor("#ff9800");
    default: return QColor(Qt::white);
    }
}

// Schéma de l'exercice dans le cadre « panel » : cadré sur la zone utilisée (éléments et flèches), avec les traits
// du terrain réel. Orientation choisie comme dans l'appli : celle qui donne le plus grand dessin.
void drawBoard(QPainter &p, const QRectF &panel, const ExerciseSheetData &data)
{
    QPainterPath clip;
    clip.addRoundedRect(panel, 16, 16);
    p.save();
    p.setClipPath(clip);
    // Bandes de tonte, plus sombres vers les bords (couleurs opaques : les dégradés transparents sortent mal en PDF).
    const int stripes = 12;
    for (int i = 0; i < stripes; ++i) {
        const qreal edge = std::abs((i + 0.5) / stripes - 0.5) * 2;      // 0 au centre, 1 au bord
        const QColor base = i % 2 ? kGrass1 : kGrass2;
        p.fillRect(QRectF(panel.left() + i * panel.width() / stripes, panel.top(), panel.width() / stripes + 1,
                          panel.height()),
                   base.darker(100 + int(edge * 14)));
    }

    const Board &b = data.board;
    if (!data.hasBoard || (b.items.isEmpty() && b.arrows.isEmpty())) {
        p.restore();
        fitParagraph(p, panel.adjusted(60, 60, -60, -60),
                     QStringLiteral("Pas encore de schéma : dessinez-le sur le terrain de l'exercice dans FootCoach."),
                     30, 18, QFont::Bold, QColor(255, 255, 255, 230));
        return;
    }

    const bool half = b.pitch == Pitch::Half;
    const qreal depth = half ? kHalfL : kFieldL;
    // Mètres : X le long du terrain (sens du jeu), Y en travers (de la gauche vers la droite du sens de jeu).
    const auto metres = [&](QPointF n) { return QPointF(n.x() * depth, n.y() * kFieldW); };

    QRectF used;
    const auto extend = [&](QPointF m) { used = used.isNull() ? QRectF(m, QSizeF(0.01, 0.01)) : used.united(QRectF(m, QSizeF(0.01, 0.01))); };
    for (const Item &it : b.items)
        extend(metres(it.pos));
    for (const Arrow &a : b.arrows) {
        extend(metres(a.from));
        extend(metres(a.to));
    }
    const qreal margin = 4;
    used.adjust(-margin, -margin, margin, margin);

    // Orientation : horizontale (écran x = X) ou verticale (écran x = Y, but visé en haut).
    const auto scaleFor = [&](bool vertical, QSizeF size) {
        const qreal w = vertical ? size.height() : size.width();
        const qreal h = vertical ? size.width() : size.height();
        return std::min(panel.width() / w, panel.height() / h);
    };
    const QSizeF minSize(std::max(used.width(), 18.0), std::max(used.height(), 18.0));
    // Orientation habituelle du type de terrain (demi-terrain : but en haut), sauf si l'autre dessine nettement plus grand.
    bool vertical = half;
    if (scaleFor(!half, minSize) > scaleFor(half, minSize) * 1.15)
        vertical = !half;
    const qreal s = std::min(scaleFor(vertical, minSize), 60.0);    // pixels par mètre
    // Zone affichée en mètres : centrée sur la zone utilisée, au format du cadre, sans dépasser le terrain (+ 3 m).
    const qreal viewX = (vertical ? panel.height() : panel.width()) / s;
    const qreal viewY = (vertical ? panel.width() : panel.height()) / s;
    const auto place = [](qreal centre, qreal view, qreal lo, qreal hi) {
        if (view >= hi - lo)
            return (lo + hi) / 2 - view / 2;
        return std::clamp(centre - view / 2, lo, hi - view);
    };
    const qreal x0 = place(used.center().x(), viewX, -3, depth + 3);
    const qreal y0 = place(used.center().y(), viewY, -3, kFieldW + 3);
    const auto screen = [&](QPointF m) {
        if (vertical)
            return QPointF(panel.left() + (m.y() - y0) * s, panel.top() + (x0 + viewX - m.x()) * s);
        return QPointF(panel.left() + (m.x() - x0) * s, panel.top() + (m.y() - y0) * s);
    };

    // Traits du terrain entier (BoardGeometry.pitchLines) ; en demi-terrain, seule la moitié offensive.
    const qreal L = kFieldL, Wd = kFieldW, c = Wd / 2;
    const qreal penD = 16.5, penH = 20.16, goalD = 5.5, goalH = 9.16, spot = 11, r = 9.15;
    const qreal arcA = qRadiansToDegrees(std::acos((penD - spot) / r));
    QList<QPolygonF> lines = {rectPts(0, 0, L, Wd), QPolygonF({QPointF(L / 2, 0), QPointF(L / 2, Wd)}),
                              arcPts(L / 2, c, r, 0, 360, 72), rectPts(0, c - penH, penD, 2 * penH),
                              rectPts(L - penD, c - penH, penD, 2 * penH), rectPts(0, c - goalH, goalD, 2 * goalH),
                              rectPts(L - goalD, c - goalH, goalD, 2 * goalH), arcPts(spot, c, r, -arcA, arcA, 24),
                              arcPts(L - spot, c, r, 180 - arcA, 180 + arcA, 24), arcPts(0, 0, 1, 0, 90, 8),
                              arcPts(L, 0, 1, 90, 180, 8), arcPts(L, Wd, 1, 180, 270, 8), arcPts(0, Wd, 1, 270, 360, 8)};
    const QList<QPolygonF> goals = {rectPts(-2.4, c - 3.66, 2.4, 7.32), rectPts(L, c - 3.66, 2.4, 7.32)};
    const qreal shift = half ? L / 2 : 0;
    const auto toScreen = [&](const QPolygonF &poly) {
        QPolygonF out;
        for (const QPointF &pt : poly)
            out << screen(QPointF(pt.x() - shift, pt.y()));
        return out;
    };
    if (half)
        p.setClipRect(QRectF(screen(QPointF(0, 0)), screen(QPointF(depth + 3, kFieldW))).normalized().intersected(panel),
                      Qt::IntersectClip);
    p.setPen(QPen(QColor(255, 255, 255, 215), std::clamp(s * 0.12, 2.0, 4.0)));
    p.setBrush(Qt::NoBrush);
    for (const QPolygonF &poly : lines)
        p.drawPolyline(toScreen(poly));
    p.setBrush(QColor(255, 255, 255, 70));
    for (const QPolygonF &poly : goals)
        p.drawPolygon(toScreen(poly));
    p.setBrush(Qt::white);
    for (const QPointF &dot : {QPointF(L / 2, c), QPointF(spot, c), QPointF(L - spot, c)})
        p.drawEllipse(screen(QPointF(dot.x() - shift, dot.y())), 3, 3);
    p.setClipPath(clip);

    // Tailles indépendantes du zoom dans des bornes lisibles.
    const qreal figScale = std::clamp(s * 0.055, 0.5, 1.4);
    const qreal coneH = std::clamp(s * 1.5, 14.0, 40.0);
    const qreal ballR = std::clamp(s * 0.45, 6.0, 12.0);
    const qreal arrowW = std::clamp(s * 0.18, 3.0, 5.0);
    const qreal defaultOffset = (vertical == half) ? 0 : (vertical ? -90 : 90);

    // Ordre de dessin : buts, flèches, plots, ballons, joueurs du haut vers le bas de l'écran, notes.
    for (const Item &it : b.items) {
        if (it.kind != Kind::Goal)
            continue;
        const QPointF c0 = screen(metres(it.pos));
        p.save();
        p.translate(c0);
        p.rotate(it.rotation + defaultOffset);
        const qreal gw = 1.6 * s, gh = 6 * s * it.scale;
        const QRectF g(-gw / 2, -gh / 2, gw, std::max(gh, 20.0));
        p.setPen(QPen(Qt::white, 3));
        p.setBrush(QColor(255, 255, 255, 60));
        p.drawRect(g);
        p.setPen(QPen(QColor(255, 255, 255, 120), 1));
        for (qreal y = g.top() + 8; y < g.bottom(); y += 8)
            p.drawLine(QPointF(g.left(), y), QPointF(g.right(), y));
        p.restore();
    }
    for (const Arrow &a : b.arrows)
        arrow(p, a, screen(metres(a.from)), screen(metres(a.to)), arrowW, arrowW * 4.2);
    QList<const Item *> order;
    for (const Item &it : b.items) {
        if (it.kind == Kind::Cone || it.kind == Kind::Ball || it.kind == Kind::Player || it.kind == Kind::Opponent)
            order.append(&it);
    }
    std::stable_sort(order.begin(), order.end(), [&](const Item *a, const Item *b2) {
        return screen(metres(a->pos)).y() < screen(metres(b2->pos)).y();
    });
    for (const Item *it : order) {
        const QPointF pt = screen(metres(it->pos));
        switch (it->kind) {
        case Kind::Cone: cone(p, pt + QPointF(0, coneH * 0.35), coneH * it->scale, itemColor(*it)); break;
        case Kind::Ball: ball(p, pt, ballR); break;
        default: figure(p, pt + QPointF(0, 40 * figScale), figScale, itemColor(*it), it->label); break;
        }
    }
    for (const Item &it : b.items) {
        if (it.kind == Kind::Text && !it.label.trimmed().isEmpty())
            tag(p, screen(metres(it.pos)), it.label.trimmed(), std::clamp(24 * it.scale, 14.0, 34.0), it.rotation,
                it.color.isEmpty() ? QColor(Qt::white) : itemColor(it));
    }
    p.restore();
}

// ---------- icônes ----------

void iconClipboard(QPainter &p, QRectF r)
{
    roundRect(p, r, 6, Qt::white, kNavy, 3);
    roundRect(p, QRectF(r.center().x() - r.width() * 0.22, r.top() - 6, r.width() * 0.44, 12), 4, QColor("#9AA8B8"),
              kNavy, 2);
    for (int i = 0; i < 3; ++i) {
        const qreal y = r.top() + r.height() * (0.3 + i * 0.24);
        p.setPen(QPen(kCheck, 3.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolyline(QPolygonF({QPointF(r.left() + 8, y), QPointF(r.left() + 12, y + 4), QPointF(r.left() + 19, y - 4)}));
        p.setPen(QPen(QColor("#5C6B7D"), 3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(r.left() + 24, y), QPointF(r.right() - 7, y));
    }
}

void iconTrophy(QPainter &p, QPointF c, qreal s)
{
    QLinearGradient g(c.x() - s, 0, c.x() + s, 0);
    g.setColorAt(0, QColor("#FFE38A"));
    g.setColorAt(0.5, kGold);
    g.setColorAt(1, QColor("#C98B00"));
    p.setPen(QPen(QColor("#A67300"), 2));
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(c.x() - s * 1.05, c.y() - s * 0.75, s * 0.8, s * 0.8), 90 * 16, 180 * 16);
    p.drawArc(QRectF(c.x() + s * 0.25, c.y() - s * 0.75, s * 0.8, s * 0.8), -90 * 16, 180 * 16);
    QPainterPath cup;
    cup.moveTo(c.x() - s * 0.65, c.y() - s * 0.8);
    cup.lineTo(c.x() + s * 0.65, c.y() - s * 0.8);
    cup.cubicTo(c.x() + s * 0.65, c.y() + s * 0.1, c.x() + s * 0.2, c.y() + s * 0.3, c.x() + s * 0.1, c.y() + s * 0.35);
    cup.lineTo(c.x() - s * 0.1, c.y() + s * 0.35);
    cup.cubicTo(c.x() - s * 0.2, c.y() + s * 0.3, c.x() - s * 0.65, c.y() + s * 0.1, c.x() - s * 0.65, c.y() - s * 0.8);
    p.setBrush(g);
    p.drawPath(cup);
    p.drawRect(QRectF(c.x() - s * 0.1, c.y() + s * 0.35, s * 0.2, s * 0.3));
    p.drawRoundedRect(QRectF(c.x() - s * 0.45, c.y() + s * 0.62, s * 0.9, s * 0.25), 3, 3);
}

void iconStar(QPainter &p, QPointF c, qreal r)
{
    QPolygonF star;
    for (int i = 0; i < 10; ++i) {
        const qreal a = qDegreesToRadians(-90.0 + i * 36.0);
        star << c + QPointF(qCos(a), qSin(a)) * ((i % 2) ? r * 0.45 : r);
    }
    QRadialGradient g(c - QPointF(r * 0.2, r * 0.3), r * 1.2);
    g.setColorAt(0, QColor("#FFE680"));
    g.setColorAt(1, kGold);
    p.setPen(QPen(QColor("#C98B00"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(g);
    p.drawPolygon(star);
}

void iconMegaphone(QPainter &p, QPointF c, qreal s)
{
    QPainterPath horn;
    horn.moveTo(c.x() - s * 0.6, c.y() - s * 0.22);
    horn.lineTo(c.x() + s * 0.45, c.y() - s * 0.65);
    horn.lineTo(c.x() + s * 0.45, c.y() + s * 0.65);
    horn.lineTo(c.x() - s * 0.6, c.y() + s * 0.22);
    horn.closeSubpath();
    p.setPen(QPen(QColor("#B85600"), 2));
    p.setBrush(kOrange);
    p.drawPath(horn);
    p.setBrush(QColor("#5E6B78"));
    p.drawRoundedRect(QRectF(c.x() - s * 0.85, c.y() - s * 0.25, s * 0.3, s * 0.5), 3, 3);
    p.drawRoundedRect(QRectF(c.x() - s * 0.5, c.y() + s * 0.18, s * 0.18, s * 0.45), 3, 3);
    p.setPen(QPen(kOrange, 3, Qt::SolidLine, Qt::RoundCap));
    for (int i = -1; i <= 1; ++i)
        p.drawLine(QPointF(c.x() + s * 0.62, c.y() + i * s * 0.45), QPointF(c.x() + s * 0.85, c.y() + i * s * 0.62));
}

void iconStopwatch(QPainter &p, QPointF c, qreal r)
{
    p.setPen(QPen(Qt::white, 3));
    p.setBrush(QColor("#E8EEF5"));
    p.drawEllipse(c, r, r);
    p.setBrush(Qt::white);
    p.drawRoundedRect(QRectF(c.x() - r * 0.22, c.y() - r * 1.35, r * 0.44, r * 0.3), 2, 2);
    p.setPen(QPen(kNavy, 3, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(c, c + QPointF(0, -r * 0.65));
    p.drawLine(c, c + QPointF(r * 0.45, r * 0.25));
    for (int i = 0; i < 12; ++i) {
        const qreal a = qDegreesToRadians(i * 30.0);
        p.drawLine(c + QPointF(qCos(a), qSin(a)) * r * 0.82, c + QPointF(qCos(a), qSin(a)) * r * 0.92);
    }
}

// Éducateur stylisé (buste), dessin original.
void coach(QPainter &p, QPointF base, qreal s)
{
    p.save();
    p.translate(base);
    p.scale(s, s);
    p.setPen(Qt::NoPen);
    QPainterPath body;
    body.moveTo(-60, 0);
    body.cubicTo(-60, -55, -40, -78, 0, -80);
    body.cubicTo(40, -78, 60, -55, 60, 0);
    body.closeSubpath();
    QLinearGradient g(-60, 0, 60, 0);
    g.setColorAt(0, QColor("#1D4F8C"));
    g.setColorAt(1, kNavyDark);
    p.setBrush(g);
    p.drawPath(body);
    p.setPen(QPen(QColor("#1D4F8C"), 22, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(-40, -60), QPointF(-78, -100));
    p.setPen(Qt::NoPen);
    p.setBrush(kSkin);
    p.drawEllipse(QPointF(-84, -108), 12, 11);
    p.setPen(QPen(kSkin, 6, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(-90, -112), QPointF(-104, -122));
    p.setPen(Qt::NoPen);
    p.setBrush(kSkin);
    p.drawRect(QRectF(-12, -96, 24, 20));
    p.setPen(QPen(QColor("#DDE3EA"), 2.5));
    p.drawLine(QPointF(-10, -78), QPointF(-4, -40));
    p.drawLine(QPointF(10, -78), QPointF(4, -40));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#C9D1DB"));
    p.drawRoundedRect(QRectF(-8, -42, 16, 10), 3, 3);
    p.setBrush(kSkin);
    p.drawEllipse(QPointF(0, -122), 27, 31);
    p.drawEllipse(QPointF(-27, -120), 6, 9);
    p.drawEllipse(QPointF(27, -120), 6, 9);
    p.setBrush(kHair);
    QPainterPath beard;
    beard.moveTo(-26, -122);
    beard.cubicTo(-26, -88, 26, -88, 26, -122);
    beard.cubicTo(18, -102, -18, -102, -26, -122);
    p.drawPath(beard);
    QPainterPath hair;
    hair.moveTo(-27, -128);
    hair.cubicTo(-30, -165, 30, -168, 27, -128);
    hair.cubicTo(18, -146, -12, -150, -27, -128);
    p.drawPath(hair);
    p.setBrush(QColor("#2B1D14"));
    p.drawEllipse(QPointF(-10, -126), 3.2, 3.6);
    p.drawEllipse(QPointF(10, -126), 3.2, 3.6);
    p.setPen(QPen(QColor("#2B1D14"), 3, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(-16, -136), QPointF(-5, -137));
    p.drawLine(QPointF(5, -137), QPointF(16, -136));
    p.setPen(QPen(Qt::white, 3, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(QRectF(-10, -117, 20, 10), 200 * 16, 140 * 16);
    p.restore();
}

enum class Bullet { Dot, Number, Check, Plain };

void bullet(QPainter &p, QPointF c, Bullet kind, int n, qreal px)
{
    const qreal r = std::max(9.0, px * 0.58);
    p.setPen(Qt::NoPen);
    switch (kind) {
    case Bullet::Number:
        p.setBrush(kBlue);
        p.drawEllipse(c, r, r);
        p.setPen(Qt::white);
        p.setFont(font(r * 1.2, QFont::Bold));
        p.drawText(QRectF(c.x() - r, c.y() - r, 2 * r, 2 * r), Qt::AlignCenter, QString::number(n));
        break;
    case Bullet::Check:
        p.setBrush(kCheck);
        p.drawEllipse(c, r, r);
        p.setPen(QPen(Qt::white, r * 0.25, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolyline(QPolygonF({c + QPointF(-r * 0.43, 0), c + QPointF(-r * 0.1, r * 0.36), c + QPointF(r * 0.5, -r * 0.36)}));
        break;
    case Bullet::Dot:
        p.setBrush(QColor("#F5A623"));
        p.drawEllipse(c, r * 0.5, r * 0.5);
        break;
    case Bullet::Plain:
        p.setBrush(kNavy);
        p.drawEllipse(c, r * 0.42, r * 0.42);
        break;
    }
}

// Liste à puces dans « area », à la plus grande taille (de maxPx à minPx) qui tient ; au-delà, coupée.
void bulletList(QPainter &p, const QRectF &area, const QStringList &items, Bullet kind, qreal maxPx, qreal minPx)
{
    if (items.isEmpty())
        return;
    const int flags = Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop;
    const auto heightAt = [&](qreal px) {
        const QFont f = font(px, QFont::Medium);
        const QFontMetricsF fm(f);
        const qreal tw = area.width() - px * 1.6;
        qreal h = 0;
        for (const QString &s : items)
            h += fm.boundingRect(QRectF(0, 0, tw, 10000), flags, s).height() + px * 0.22;
        return h;
    };
    qreal px = maxPx;
    while (px > minPx && heightAt(px) > area.height())
        px -= 0.5;
    const QFont f = font(px, QFont::Medium);
    const QFontMetricsF fm(f);
    const qreal tx = area.left() + px * 1.6;
    p.save();
    p.setClipRect(area.adjusted(-2, -2, 2, 2));
    qreal y = area.top();
    for (int i = 0; i < items.size(); ++i) {
        const QRectF box(tx, y, area.right() - tx, 10000);
        const QRectF br = fm.boundingRect(box, flags, items.at(i));
        p.setPen(kText);
        p.setFont(f);
        p.drawText(box, flags, items.at(i));
        bullet(p, QPointF(area.left() + px * 0.62, y + fm.height() / 2), kind, i + 1, px);
        y += br.height() + px * 0.22;
    }
    p.restore();
}

// Cadre à titre : bandeau marine, fond blanc. Renvoie la zone du contenu.
QRectF card(QPainter &p, const QRectF &r, const QString &title, qreal headerW = -1)
{
    roundRect(p, r.adjusted(0, 3, 0, 3), 16, QColor(14, 58, 107, 35));
    roundRect(p, r, 16, Qt::white, kNavy, 2.5);
    const qreal hw = headerW > 0 ? headerW : r.width();
    QPainterPath head;
    head.addRoundedRect(QRectF(r.left(), r.top(), hw, 56), 16, 16);
    QPainterPath flat;
    flat.addRect(QRectF(r.left(), r.top() + 30, hw, 26));
    head = head.united(flat);
    p.setPen(Qt::NoPen);
    p.setBrush(kNavy);
    p.drawPath(head);
    fitLine(p, QRectF(r.left() + 88, r.top(), hw - 96, 56), title, Qt::AlignLeft, 31, 20, QFont::Bold, Qt::white);
    return r.adjusted(20, 72, -16, -14);
}

void headerBall(QPainter &p, QPointF c, qreal r) { ball(p, c, r); }

} // namespace

QStringList ExerciseSheet::splitLines(const QString &text)
{
    static const QRegularExpression bulletRe(QStringLiteral("^\\s*([-*•·]|\\d+[.)])\\s+"));
    QStringList out;
    for (QString line : text.split(QLatin1Char('\n'))) {
        line.remove(bulletRe);
        line = line.simplified();
        if (!line.isEmpty())
            out.append(line);
    }
    return out;
}

QString ExerciseSheet::fileName(const QString &title)
{
    QString out;
    for (const QChar c : PlayerRepository::fold(title)) {
        if (c.isLetterOrNumber() && c.unicode() < 128)
            out += c;
        else if (!out.isEmpty() && !out.endsWith(QLatin1Char('-')))
            out += QLatin1Char('-');
    }
    while (out.endsWith(QLatin1Char('-')))
        out.chop(1);
    return QStringLiteral("fiche-%1.pdf").arg(out.isEmpty() ? QStringLiteral("exercice") : out.left(60));
}

void ExerciseSheet::paint(QPainter &p, const ExerciseSheetData &d)
{
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.fillRect(QRectF(0, 0, W, H), kBg);

    // ----- bandeau titre, pastilles -----
    const bool hasCat = !d.ageCategory.trimmed().isEmpty();
    const bool hasDur = d.durationMin > 0;
    qreal right = W - 14;
    QRectF durPill, catPill;
    if (hasDur) {
        durPill = QRectF(right - 216, 16, 216, 74);
        right = durPill.left() - 20;
    }
    if (hasCat) {
        catPill = QRectF(right - 228, 16, 228, 74);
        right = catPill.left() - 24;
    }
    const QRectF band(14, 12, right - 14, 82);
    roundRect(p, band, 41, kNavy);
    headerBall(p, QPointF(66, 56), 46);
    fitLine(p, QRectF(126, 12, band.right() - 146, 82), d.title, Qt::AlignHCenter, 64, 30, QFont::ExtraBold, Qt::white);
    if (hasCat) {
        roundRect(p, catPill, 22, kGreen);
        fitLine(p, catPill.adjusted(12, 0, -12, 0), d.ageCategory.trimmed(), Qt::AlignHCenter, 44, 20, QFont::Bold, Qt::white);
    }
    if (hasDur) {
        roundRect(p, durPill, 22, kNavy);
        iconStopwatch(p, QPointF(durPill.left() + 44, 56), 24);
        fitLine(p, QRectF(durPill.left() + 76, 16, durPill.width() - 84, 74), QStringLiteral("%1 min").arg(d.durationMin),
                Qt::AlignHCenter, 44, 22, QFont::Bold, Qt::white);
    }
    if (!d.objective.trimmed().isEmpty()) {
        const QRectF obj(150, 102, 1230, 48);
        roundRect(p, obj, 24, kSky);
        fitLine(p, obj.adjusted(24, 0, -24, 0), QStringLiteral("Objectifs : %1").arg(d.objective.simplified()),
                Qt::AlignHCenter, 29, 16, QFont::Bold, kNavyDark);
    }

    // ----- schéma -----
    drawBoard(p, QRectF(12, 164, 950, 586), d);

    // ----- Organisation -----
    QRectF a = card(p, QRectF(982, 168, 540, 252), QStringLiteral("Organisation"));
    cone(p, QPointF(1022, 222), 50, kOrange);
    bulletList(p, a, d.organisation, Bullet::Plain, 27, 14);

    // ----- Consignes -----
    a = card(p, QRectF(982, 436, 540, 314), QStringLiteral("Consignes"));
    iconClipboard(p, QRectF(1000, 446, 42, 52));
    bulletList(p, a, d.instructions, Bullet::Number, 24, 13);

    // ----- Évolutions -----
    a = card(p, QRectF(12, 770, 486, 266), QStringLiteral("Évolutions"));
    iconTrophy(p, QPointF(54, 796), 30);
    bulletList(p, a, d.variations, Bullet::Dot, 23, 13);

    // ----- Critères de réussite -----
    a = card(p, QRectF(514, 770, 462, 266), QStringLiteral("Critères de réussite"));
    iconStar(p, QPointF(552, 798), 24);
    bulletList(p, a, d.criteria, Bullet::Check, 23, 13);

    // ----- Consigne éducateur -----
    const QRectF cr(990, 770, 532, 266);
    card(p, cr, QStringLiteral("Consigne éducateur"), 440);
    iconMegaphone(p, QPointF(1032, 798), 24);
    QString cue = d.coachCue.simplified();
    if (!cue.isEmpty()) {
        if (!cue.startsWith(QStringLiteral("«")) && !cue.startsWith(QLatin1Char('"')))
            cue = QStringLiteral("« %1 »").arg(cue);
        QPainterPath bubble;
        bubble.addRoundedRect(QRectF(1002, 840, 350, 180), 22, 22);
        QPainterPath tail;
        tail.addPolygon(QPolygonF({QPointF(1342, 900), QPointF(1384, 930), QPointF(1342, 950)}));
        bubble = bubble.united(tail);
        p.setPen(Qt::NoPen);
        p.setBrush(kYellow);
        p.drawPath(bubble);
        fitParagraph(p, QRectF(1016, 850, 322, 160), cue, 27, 14, QFont::Bold, kNavyDark);
    }
    p.save();
    QPainterPath cclip;
    cclip.addRoundedRect(cr, 16, 16);
    p.setClipPath(cclip);
    coach(p, QPointF(1462, 1040), 1.25);
    p.restore();

    p.setPen(kMuted);
    p.setFont(font(15, QFont::Medium));
    p.drawText(QRectF(14, 1046, 1508, 30), Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("Fiche FootCoach"));
}

bool ExerciseSheet::writePdf(QIODevice *out, const ExerciseSheetData &data, QString *error)
{
    if (!out || !out->isWritable()) {
        if (error)
            *error = QStringLiteral("Destination non accessible en écriture.");
        return false;
    }
    QPdfWriter pdf(out);
    pdf.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Landscape, QMarginsF(0, 0, 0, 0)));
    pdf.setResolution(300);
    pdf.setTitle(QStringLiteral("Fiche exercice - %1").arg(data.title));
    pdf.setCreator(QStringLiteral("FootCoach"));
    QPainter p;
    if (!p.begin(&pdf)) {
        if (error)
            *error = QStringLiteral("Impossible de démarrer le dessin du PDF.");
        return false;
    }
    const QRect page = p.viewport();
    p.scale(page.width() / W, page.height() / H);
    paint(p, data);
    p.end();
    return true;
}
