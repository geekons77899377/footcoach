#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStandardPaths>

#include "app/Backend.h"
#include "core/Database.h"

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

    QQmlApplicationEngine engine;
    QScopedPointer<Backend> backend;
    if (opened) {
        backend.reset(new Backend(db.handle()));
    }
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), backend.data());
    engine.rootContext()->setContextProperty(QStringLiteral("startupError"),
                                             opened ? QString() : QObject::tr("Impossible d'ouvrir la base de données :\n%1").arg(error));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("FootCoachUi"), QStringLiteral("Main"));
    return app.exec();
}
