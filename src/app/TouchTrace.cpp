#include "TouchTrace.h"

#include <QElapsedTimer>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTouchEvent>
#include <QtDebug>

#include <algorithm>

namespace {

QElapsedTimer &traceClock()
{
    static QElapsedTimer t;
    if (!t.isValid())
        t.start();
    return t;
}

QString describe(const QQuickItem *item)
{
    QString type = QString::fromLatin1(item->metaObject()->className());
    // Les types QML s'appellent « BoardView_QMLTYPE_12 » : on garde le nom lisible.
    const int cut = type.indexOf(QLatin1String("_QML"));
    if (cut > 0)
        type.truncate(cut);
    return item->objectName().isEmpty() ? type : QStringLiteral("%1#%2").arg(type, item->objectName());
}

// Parcourt les enfants du plus haut (z, puis ordre) au plus bas ; renvoie le plus profond visible sous le point.
const QQuickItem *topmostAt(const QQuickItem *item, const QPointF &scenePos, QStringList *chain)
{
    const QList<QQuickItem *> kids = item->childItems();
    QList<QQuickItem *> sorted = kids;
    std::stable_sort(sorted.begin(), sorted.end(), [](QQuickItem *a, QQuickItem *b) { return a->z() < b->z(); });
    for (int i = sorted.size() - 1; i >= 0; --i) {
        const QQuickItem *c = sorted.at(i);
        if (!c->isVisible() || c->opacity() <= 0.0 || !c->isEnabled())
            continue;
        const QPointF p = c->mapFromScene(scenePos);
        if (!c->clip() || c->contains(p)) {
            QStringList sub;
            const QQuickItem *hit = topmostAt(c, scenePos, &sub);
            if (hit || c->contains(p)) {
                chain->append(describe(c));
                chain->append(sub);
                return hit ? hit : c;
            }
        }
    }
    return nullptr;
}

// Qui a saisi le point : élément QML ou gestionnaire (TapHandler, DragHandler…) avec son élément.
QString describeGrabber(QObject *o)
{
    if (!o)
        return QStringLiteral("personne");
    if (auto *item = qobject_cast<QQuickItem *>(o))
        return describe(item);
    QString type = QString::fromLatin1(o->metaObject()->className());
    const int cut = type.indexOf(QLatin1String("_QML"));
    if (cut > 0)
        type.truncate(cut);
    if (!o->objectName().isEmpty())
        type += QLatin1Char('#') + o->objectName();
    if (auto *owner = qobject_cast<QQuickItem *>(o->parent()))
        type += QStringLiteral(" de ") + describe(owner);
    return type;
}

} // namespace

bool TouchTrace::enabledByEnvironment()
{
    return qEnvironmentVariable("FOOTCOACH_TOUCH_TRACE", QStringLiteral("1")) != QLatin1String("0");
}

QString TouchTrace::describeItemAt(QQuickWindow *window, const QPointF &scenePos)
{
    if (!window || !window->contentItem())
        return QString();
    QStringList chain;
    topmostAt(window->contentItem(), scenePos, &chain);
    // Les 6 derniers maillons suffisent à reconnaître la page et l'élément.
    if (chain.size() > 6)
        chain = chain.mid(chain.size() - 6);
    return chain.join(QStringLiteral(" > "));
}

bool TouchTrace::eventFilter(QObject *watched, QEvent *event)
{
    auto *window = qobject_cast<QQuickWindow *>(watched);
    if (!window)
        return false;
    const char *what = nullptr;
    switch (event->type()) {
    case QEvent::MouseButtonPress: what = "souris appui"; break;
    case QEvent::MouseButtonRelease: what = "souris relâché"; break;
    case QEvent::TouchBegin: what = "doigt appui"; break;
    case QEvent::TouchEnd: what = "doigt relâché"; break;
    case QEvent::TouchCancel: what = "doigt annulé"; break;
    default: return false;
    }
    auto *pe = static_cast<QPointerEvent *>(event);
    const QPointF pos = pe->pointCount() > 0 ? pe->point(0).scenePosition() : QPointF();
    // Mouvements de souris synthétisés depuis le doigt : déjà tracés comme toucher.
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        if (static_cast<QMouseEvent *>(event)->source() != Qt::MouseEventNotSynthesized)
            return false;
    }
    // Au relâchement, le point garde qui l'a saisi à l'appui : c'est celui qui recevra le « clic ».
    QString grab;
    if (pe->pointCount() > 0 && event->type() != QEvent::MouseButtonPress && event->type() != QEvent::TouchBegin) {
        const QEventPoint &pt = pe->point(0);
        QStringList passive;
        for (const auto &g : pe->passiveGrabbers(pt))
            passive << describeGrabber(g.data());
        grab = QStringLiteral(" | saisi par : %1").arg(describeGrabber(pe->exclusiveGrabber(pt)));
        if (!passive.isEmpty())
            grab += QStringLiteral(" | observé par : ") + passive.join(QStringLiteral(", "));
    }
    qInfo().noquote() << QStringLiteral("[toucher] %1 ms | %2 à (%3, %4) | sous le doigt : %5%6")
                             .arg(traceClock().elapsed())
                             .arg(QString::fromUtf8(what))
                             .arg(pos.x(), 0, 'f', 0)
                             .arg(pos.y(), 0, 'f', 0)
                             .arg(describeItemAt(window, pos), grab);
    return false;
}
