#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTranslator>

#include "core/Database.h"
#include "ui/MainWindow.h"
#include "ui/Platform.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Doivent être définis avant d'utiliser QStandardPaths::AppDataLocation.
    QCoreApplication::setOrganizationName(QStringLiteral("FootCoach"));
    QCoreApplication::setApplicationName(QStringLiteral("FootCoach"));
    QCoreApplication::setApplicationVersion(QStringLiteral(FOOTCOACH_VERSION));

    // Boutons standard des dialogues (Enregistrer, Annuler…) dans la langue du système, si Qt fournit
    // les traductions (dossier « translations » du kit Qt ; windeployqt les copie au déploiement).
    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(), QStringLiteral("qtbase"), QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&qtTranslator);
    }

    if (Platform::compactLayout()) {
        app.setStyleSheet(Platform::compactStyleSheet());
    }

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString dbPath = QDir(dataDir).filePath(QStringLiteral("footcoach.db"));

    // Déclarée avant la fenêtre : détruite après elle.
    Database db;
    QString error;
    if (!db.open(dbPath, &error)) {
        QMessageBox::critical(nullptr,
                              QObject::tr("FootCoach"),
                              QObject::tr("Impossible d'ouvrir la base de données :\n%1").arg(error));
        return 1;
    }

    MainWindow window(db);
    window.show();
    return app.exec();
}
