#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>

#include "app/Backend.h"
#include "app/IssueReporter.h"
#include "app/TouchTrace.h"
#include "core/Database.h"
#include "git_revision.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Doivent être définis avant d'utiliser QStandardPaths::AppDataLocation.
    QCoreApplication::setOrganizationName(QStringLiteral("FootCoach"));
    QCoreApplication::setApplicationName(QStringLiteral("FootCoach"));
    QCoreApplication::setApplicationVersion(QStringLiteral(FOOTCOACH_VERSION));

    QQuickStyle::setStyle(QStringLiteral("Material"));

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString dbPath = QDir(dataDir).filePath(QStringLiteral("footcoach.db"));

    // Déclarée avant le moteur QML : détruite après lui.
    Database db;
    QString error;
    const bool opened = db.open(dbPath, &error);

    // Déclaré AVANT le moteur QML : il est détruit après lui (sinon les liaisons QML lisent un « backend » nul
    // pendant la fermeture).
    QScopedPointer<Backend> backend;
    if (opened) {
        backend.reset(new Backend(db.handle()));
    } else {
        qWarning().noquote() << "Base de données :" << error;
    }
    IssueReporter issueReporter;   // avant le moteur : détruit après lui
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("issueReporter"), &issueReporter);
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), backend.data());
    // Version et révision git compilées : { version, sha1 (40 caractères ou « inconnu »), shortSha, dirty }.
    const QString sha = QStringLiteral(FOOTCOACH_GIT_SHA);
    engine.rootContext()->setContextProperty(
        QStringLiteral("buildInfo"),
        QVariantMap{{QStringLiteral("version"), QStringLiteral(FOOTCOACH_VERSION)},
                    {QStringLiteral("sha1"), sha},
                    {QStringLiteral("shortSha"), sha.left(7)},
                    {QStringLiteral("dirty"), FOOTCOACH_GIT_DIRTY != 0}});
    // Diagnostic des touchers (TouchTrace.h) : journal « [toucher] », coupé par FOOTCOACH_TOUCH_TRACE=0.
    const bool traceTouches = TouchTrace::enabledByEnvironment();
    engine.rootContext()->setContextProperty(QStringLiteral("traceTouches"), traceTouches);
    engine.rootContext()->setContextProperty(QStringLiteral("startupError"),
                                             opened ? QString() : QObject::tr("Impossible d'ouvrir la base de données :\n%1").arg(error));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("FootCoachUi"), QStringLiteral("Main"));
    TouchTrace touchTrace;
    if (traceTouches) {
        for (QObject *root : engine.rootObjects())
            if (auto *w = qobject_cast<QQuickWindow *>(root))
                w->installEventFilter(&touchTrace);
    }
    return app.exec();
}
