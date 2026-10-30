#pragma once

#include <QSqlDatabase>
#include <QString>

// Ouvre la base SQLite et applique les migrations embarquées (":/sql/NNN_nom.sql").
// La version du schéma est stockée dans PRAGMA user_version.
class Database
{
public:
    Database() = default;
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    // Crée le dossier et le fichier si nécessaire, active les clés étrangères,
    // puis migre le schéma. En cas d'échec, renvoie false et remplit *error.
    bool open(const QString &filePath, QString *error = nullptr);
    void close();

    QString path() const { return m_path; }
    int schemaVersion() const;          // -1 si la lecture échoue
    QSqlDatabase handle() const;        // connexion à utiliser avec QSqlQuery / QSqlTableModel

private:
    bool migrate(QString *error);
    bool applyScript(const QString &resourcePath, QString *error);

    QString m_path;
    const QString m_connection = QStringLiteral("footcoach");
};
