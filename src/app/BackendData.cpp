// Export et import de toutes les données (fichier « .footcoach ») de Backend (voir Backend.h).
#include "Backend.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUrl>

namespace {

const QString kFormat = QStringLiteral("footcoach-data");
constexpr int kFormatVersion = 1;
constexpr qint64 kMaxBytes = 64LL * 1024 * 1024;
constexpr int kMaxRows = 500000;

// Ordre d'insertion (parents avant enfants) ; la suppression se fait dans l'ordre inverse.
const QStringList kTables = {QStringLiteral("team"),           QStringLiteral("player"),
                             QStringLiteral("training_session"), QStringLiteral("attendance"),
                             QStringLiteral("exercise"),       QStringLiteral("session_exercise"),
                             QStringLiteral("tactical_board"), QStringLiteral("injury")};

QVariantMap done(bool ok, const QString &error = QString())
{
    QVariantMap m;
    m.insert(QStringLiteral("ok"), ok);
    m.insert(QStringLiteral("error"), error);
    return m;
}

QString localPath(const QString &url)
{
    const QUrl u(url);
    if (u.isLocalFile())
        return u.toLocalFile();
    return url;        // chemin simple ou content:// (géré par QFile sous Android)
}

QJsonValue toJson(const QVariant &v)
{
    if (v.isNull() || !v.isValid())
        return QJsonValue::Null;
    switch (v.typeId()) {
    case QMetaType::QByteArray: {
        QJsonObject o;
        o.insert(QStringLiteral("$b64"), QString::fromLatin1(v.toByteArray().toBase64()));
        return o;
    }
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        return QJsonValue(v.toLongLong());
    case QMetaType::Double:
    case QMetaType::Float:
        return QJsonValue(v.toDouble());
    default:
        return QJsonValue(v.toString());
    }
}

QVariant fromJson(const QJsonValue &v, bool *ok)
{
    *ok = true;
    if (v.isNull())
        return QVariant(QMetaType::fromType<QString>());
    if (v.isString())
        return v.toString();
    if (v.isBool())
        return v.toBool() ? 1 : 0;
    if (v.isDouble()) {
        const double d = v.toDouble();
        if (d == double(qint64(d)) && qAbs(d) < 9e15)
            return qint64(d);
        return d;
    }
    if (v.isObject() && v.toObject().contains(QStringLiteral("$b64")))
        return QByteArray::fromBase64(v.toObject().value(QStringLiteral("$b64")).toString().toLatin1());
    *ok = false;
    return QVariant();
}

QStringList columnsOf(const QSqlDatabase &db, const QString &table)
{
    QStringList cols;
    QSqlQuery q(db);
    if (q.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table)))
        while (q.next())
            cols.append(q.value(1).toString());
    return cols;
}

int schemaOf(const QSqlDatabase &db)
{
    QSqlQuery q(db);
    return (q.exec(QStringLiteral("PRAGMA user_version")) && q.next()) ? q.value(0).toInt() : -1;
}

QVariantMap countsOf(const QJsonObject &tables)
{
    auto n = [&](const char *t) { return tables.value(QLatin1String(t)).toObject().value(QStringLiteral("rows")).toArray().size(); };
    QVariantMap c;
    c.insert(QStringLiteral("teams"), n("team"));
    c.insert(QStringLiteral("players"), n("player"));
    c.insert(QStringLiteral("sessions"), n("training_session"));
    c.insert(QStringLiteral("exercises"), n("exercise"));
    c.insert(QStringLiteral("boards"), n("tactical_board"));
    return c;
}

} // namespace

QByteArray Backend::buildExport(int *schemaOut)
{
    QJsonObject tables;
    for (const QString &table : kTables) {
        QSqlQuery q(m_db);
        if (!q.exec(QStringLiteral("SELECT * FROM %1 ORDER BY rowid").arg(table))) {
            setError(q.lastError().text());
            return {};
        }
        const QSqlRecord rec = q.record();
        QJsonArray cols, rows;
        for (int i = 0; i < rec.count(); ++i)
            cols.append(rec.fieldName(i));
        while (q.next()) {
            QJsonArray row;
            for (int i = 0; i < rec.count(); ++i)
                row.append(toJson(q.value(i)));
            rows.append(row);
        }
        QJsonObject t;
        t.insert(QStringLiteral("columns"), cols);
        t.insert(QStringLiteral("rows"), rows);
        tables.insert(table, t);
    }
    const int schema = schemaOf(m_db);
    if (schemaOut)
        *schemaOut = schema;
    QJsonObject root;
    root.insert(QStringLiteral("format"), kFormat);
    root.insert(QStringLiteral("version"), kFormatVersion);
    root.insert(QStringLiteral("schema"), schema);
    root.insert(QStringLiteral("app"), QCoreApplication::applicationVersion());
    root.insert(QStringLiteral("exportedAt"), QDateTime::currentDateTime().toString(Qt::ISODate));
    root.insert(QStringLiteral("tables"), tables);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QString Backend::suggestedExportName() const
{
    return QStringLiteral("FootCoach-%1.footcoach").arg(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
}

QVariantMap Backend::countsOfDatabase()
{
    QJsonObject tables;
    for (const QString &table : kTables) {
        QSqlQuery q(m_db);
        int count = 0;
        if (q.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(table)) && q.next())
            count = q.value(0).toInt();
        QJsonArray rows;
        for (int i = 0; i < count; ++i)
            rows.append(QJsonValue());
        QJsonObject t;
        t.insert(QStringLiteral("rows"), rows);
        tables.insert(table, t);
    }
    return countsOf(tables);
}

QVariantMap Backend::exportDataFile(const QString &url)
{
    const QByteArray data = buildExport();
    if (data.isEmpty())
        return done(false, m_lastError.isEmpty() ? tr("Export impossible.") : m_lastError);
    QFile f(localPath(url));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QString e = tr("Impossible d'écrire le fichier : %1").arg(f.errorString());
        setError(e);
        return done(false, e);
    }
    const bool ok = f.write(data) == data.size();
    f.close();
    if (!ok) {
        const QString e = tr("Écriture incomplète du fichier.");
        setError(e);
        return done(false, e);
    }
    setError(QString());
    QVariantMap m = done(true);
    m.insert(QStringLiteral("bytes"), data.size());
    return m;
}

bool Backend::parseDataFile(const QString &url, QJsonObject *root, QString *error) const
{
    QFile f(localPath(url));
    if (!f.open(QIODevice::ReadOnly)) {
        *error = tr("Impossible de lire le fichier : %1").arg(f.errorString());
        return false;
    }
    if (f.size() > kMaxBytes) {
        *error = tr("Fichier trop volumineux (plus de 64 Mo).");
        return false;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (doc.isNull() || !doc.isObject()) {
        *error = tr("Ce fichier n'est pas un export FootCoach valide.");
        return false;
    }
    *root = doc.object();
    if (root->value(QStringLiteral("format")).toString() != kFormat) {
        *error = tr("Ce fichier n'est pas un export FootCoach.");
        return false;
    }
    if (root->value(QStringLiteral("version")).toInt() != kFormatVersion) {
        *error = tr("Version de fichier non prise en charge : mettez FootCoach à jour sur cet appareil.");
        return false;
    }
    const int fileSchema = root->value(QStringLiteral("schema")).toInt(-1);
    const int mySchema = schemaOf(m_db);
    if (fileSchema < 1 || (mySchema >= 0 && fileSchema > mySchema)) {
        *error = tr("Ce fichier vient d'une version plus récente de FootCoach : mettez l'application à jour ici.");
        return false;
    }
    const QJsonObject tables = root->value(QStringLiteral("tables")).toObject();
    int total = 0;
    for (const QString &t : kTables) {
        const QJsonObject o = tables.value(t).toObject();
        if (!o.value(QStringLiteral("columns")).isArray() || !o.value(QStringLiteral("rows")).isArray()) {
            *error = tr("Fichier incomplet (table « %1 » absente).").arg(t);
            return false;
        }
        total += o.value(QStringLiteral("rows")).toArray().size();
    }
    if (total > kMaxRows) {
        *error = tr("Fichier invraisemblable (trop de lignes).");
        return false;
    }
    return true;
}

QVariantMap Backend::inspectDataFile(const QString &url)
{
    QJsonObject root;
    QString error;
    if (!parseDataFile(url, &root, &error)) {
        setError(error);
        return done(false, error);
    }
    QVariantMap m = done(true);
    m.insert(QStringLiteral("exportedAt"), root.value(QStringLiteral("exportedAt")).toString());
    m.insert(QStringLiteral("appVersion"), root.value(QStringLiteral("app")).toString());
    m.insert(QStringLiteral("counts"), countsOf(root.value(QStringLiteral("tables")).toObject()));
    m.insert(QStringLiteral("current"), countsOfDatabase());
    setError(QString());
    return m;
}

QVariantMap Backend::importDataFile(const QString &url)
{
    QJsonObject root;
    QString error;
    if (!parseDataFile(url, &root, &error)) {
        setError(error);
        return done(false, error);
    }
    const QJsonObject tables = root.value(QStringLiteral("tables")).toObject();

    // Valide la forme de chaque ligne avant toute écriture.
    QHash<QString, QStringList> fileCols;
    for (const QString &t : kTables) {
        const QJsonObject o = tables.value(t).toObject();
        const QStringList mine = columnsOf(m_db, t);
        QStringList cols;
        for (const QJsonValue &c : o.value(QStringLiteral("columns")).toArray()) {
            if (!mine.contains(c.toString())) {
                error = tr("Fichier non valide (colonne inconnue « %1.%2 »).").arg(t, c.toString());
                setError(error);
                return done(false, error);
            }
            cols.append(c.toString());
        }
        for (const QJsonValue &r : o.value(QStringLiteral("rows")).toArray())
            if (!r.isArray() || r.toArray().size() != cols.size()) {
                error = tr("Fichier non valide (ligne mal formée dans « %1 »).").arg(t);
                setError(error);
                return done(false, error);
            }
        fileCols.insert(t, cols);
    }

    // Sauvegarde de l'état actuel, pour pouvoir revenir en arrière.
    const QByteArray before = buildExport();
    QString backupName;
    if (!before.isEmpty()) {
        const QDir dir(QFileInfo(m_db.databaseName()).absolutePath() + QStringLiteral("/sauvegardes"));
        QDir().mkpath(dir.absolutePath());
        backupName = QStringLiteral("avant-import-%1.footcoach").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
        QFile bf(dir.filePath(backupName));
        if (!bf.open(QIODevice::WriteOnly | QIODevice::Truncate) || bf.write(before) != before.size()) {
            error = tr("Import annulé : la sauvegarde de vos données actuelles a échoué.");
            setError(error);
            return done(false, error);
        }
        bf.close();
        // Garde les trois plus récentes.
        const QStringList all = dir.entryList({QStringLiteral("avant-import-*.footcoach")}, QDir::Files, QDir::Name);
        for (int i = 0; i < all.size() - 3; ++i)
            QFile::remove(dir.filePath(all.at(i)));
    }

    // Tout ou rien : un seul passage dans une transaction.
    if (!m_db.transaction()) {
        error = m_db.lastError().text();
        setError(error);
        return done(false, error);
    }
    bool ok = true;
    QString failure;
    for (int i = kTables.size() - 1; ok && i >= 0; --i) {
        QSqlQuery q(m_db);
        if (!q.exec(QStringLiteral("DELETE FROM %1").arg(kTables.at(i)))) {
            ok = false;
            failure = q.lastError().text();
        }
    }
    for (const QString &t : kTables) {
        if (!ok)
            break;
        const QStringList cols = fileCols.value(t);
        if (cols.isEmpty())
            continue;
        const QJsonArray rows = tables.value(t).toObject().value(QStringLiteral("rows")).toArray();
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral("INSERT INTO %1(%2) VALUES(%3)")
                      .arg(t, cols.join(QLatin1Char(',')), QStringList(cols.size(), QStringLiteral("?")).join(QLatin1Char(','))));
        for (const QJsonValue &rv : rows) {
            const QJsonArray row = rv.toArray();
            for (int c = 0; c < cols.size(); ++c) {
                bool valueOk = true;
                const QVariant v = fromJson(row.at(c), &valueOk);
                if (!valueOk) {
                    ok = false;
                    failure = tr("valeur illisible dans « %1.%2 »").arg(t, cols.at(c));
                    break;
                }
                q.addBindValue(v);
            }
            if (!ok || !q.exec()) {
                if (ok)
                    failure = q.lastError().text();
                ok = false;
                break;
            }
        }
    }
    if (ok)
        ok = m_db.commit();
    if (!ok) {
        m_db.rollback();
        error = tr("Import refusé, vos données n'ont pas été modifiées (%1).").arg(failure);
        setError(error);
        return done(false, error);
    }

    setError(QString());
    emit dataReplaced();
    QVariantMap m = done(true);
    m.insert(QStringLiteral("backupName"), backupName);
    return m;
}
