#pragma once

#include <QObject>

class QQuickItem;
class QQuickWindow;

// Instrumentation des touchers (diagnostic, 14/11/2026) : chaque appui et relâchement, souris ou doigt, est écrit dans
// le journal avec sa position et l'élément QML le plus haut sous le doigt (chaîne de parents avec objectName et type).
// Avec les traces QML « [toucher] » (bouton, terrain, couche de blocage), on voit qui reçoit quoi dans
// « Sortie de l'application » de Qt Creator ou `adb logcat | grep toucher`.
// Activé par défaut tant que le diagnostic dure ; FOOTCOACH_TOUCH_TRACE=0 le coupe.
class TouchTrace : public QObject
{
    Q_OBJECT
public:
    explicit TouchTrace(QObject *parent = nullptr) : QObject(parent) {}
    static bool enabledByEnvironment();
    // Élément visible le plus haut sous (x, y) en coordonnées de la fenêtre, décrit de la racine vers le haut.
    static QString describeItemAt(QQuickWindow *window, const QPointF &scenePos);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};
