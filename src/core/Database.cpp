#include "Database.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>

#include <algorithm>
#include <utility>
#include <vector>

namespace {

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

// "001_init.sql" -> 1 ; -1 si le nom ne respecte pas le format NNN_nom.sql
int versionFromName(const QString &fileName)
{
    static const QRegularExpression re(QStringLiteral("^(\\d+)_.*\\.sql$"));
    const QRegularExpressionMatch m = re.match(fileName);
    return m.hasMatch() ? m.captured(1).toInt() : -1;
}

// Découpe un script en statements.
// Limites volontaires : pas de TRIGGER, et aucun ';' ni '--' dans un littéral de chaîne.
QStringList splitStatements(const QString &script)
{
    static const QRegularExpression comment(QStringLiteral("--[^\\n]*"));
    QString cleaned = script;
    cleaned.remove(comment);

    QStringList statements;
    const QStringList parts = cleaned.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty())
            statements << trimmed;
    }
    return statements;
}

} // namespace

Database::~Database()
{
    close();
}

bool Database::open(const QString &filePath, QString *error)
{
    // ":memory:" = base SQLite en mémoire (tests) : aucun dossier à créer.
    const bool inMemory = (filePath == QLatin1String(":memory:"));
    if (!inMemory && !QDir().mkpath(QFileInfo(filePath).absolutePath())) {
        setError(error, QStringLiteral("Impossible de créer le dossier de la base : %1")
                            .arg(QFileInfo(filePath).absolutePath()));
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
    db.setDatabaseName(filePath);
    if (!db.open()) {
        setError(error, db.lastError().text());
        return false;
    }

    // Doit être exécuté hors transaction, à chaque ouverture de connexion.
    QSqlQuery pragma(db);
    if (!pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        setError(error, pragma.lastError().text());
        return false;
    }

    m_path = filePath;
    return migrate(error);
}

void Database::close()
{
    if (!QSqlDatabase::contains(m_connection))
        return;
    {
        QSqlDatabase db = QSqlDatabase::database(m_connection, false);
        if (db.isOpen())
            db.close();
    } // aucun QSqlDatabase vivant avant removeDatabase()
    QSqlDatabase::removeDatabase(m_connection);
}

QSqlDatabase Database::handle() const
{
    return QSqlDatabase::database(m_connection, false);
}

int Database::schemaVersion() const
{
    QSqlQuery q(handle());
    if (q.exec(QStringLiteral("PRAGMA user_version")) && q.next())
        return q.value(0).toInt();
    return -1;
}

bool Database::applyScript(const QString &resourcePath, QString *error)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(error, QStringLiteral("Ressource introuvable : %1").arg(resourcePath));
        return false;
    }

    const QString script = QString::fromUtf8(file.readAll());
    QSqlQuery q(handle());
    for (const QString &statement : splitStatements(script)) {
        if (!q.exec(statement)) {
            setError(error, QStringLiteral("%1\n--> %2").arg(q.lastError().text(), statement));
            return false;
        }
    }
    return true;
}

bool Database::migrate(QString *error)
{
    const QDir dir(QStringLiteral(":/sql"));
    std::vector<std::pair<int, QString>> migrations;
    for (const QString &name : dir.entryList({QStringLiteral("*.sql")}, QDir::Files)) {
        const int version = versionFromName(name);
        if (version > 0)
            migrations.emplace_back(version, dir.filePath(name));
    }
    std::sort(migrations.begin(), migrations.end());

    const int current = schemaVersion();
    if (current < 0) {
        setError(error, QStringLiteral("Lecture de la version du schéma impossible."));
        return false;
    }

    const int latest = migrations.empty() ? 0 : migrations.back().first;
    if (current > latest) {
        setError(error, QStringLiteral("La base (schéma v%1) est plus récente que l'application (schéma v%2).")
                            .arg(current)
                            .arg(latest));
        return false;
    }

    QSqlDatabase db = handle();
    for (const auto &[version, path] : migrations) {
        if (version <= current)
            continue;

        if (!db.transaction()) {
            setError(error, db.lastError().text());
            return false;
        }

        QString scriptError;
        bool ok = applyScript(path, &scriptError);
        if (ok) {
            QSqlQuery q(db);
            ok = q.exec(QStringLiteral("PRAGMA user_version = %1").arg(version));
            if (!ok)
                scriptError = q.lastError().text();
        }

        if (!ok) {
            db.rollback();
            setError(error, QStringLiteral("Migration %1 échouée :\n%2").arg(version).arg(scriptError));
            return false;
        }

        if (!db.commit()) {
            setError(error, db.lastError().text());
            return false;
        }
    }
    return true;
}
