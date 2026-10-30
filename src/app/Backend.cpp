#include "Backend.h"

#include "core/TacticalBoard.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Backend::Backend(const QSqlDatabase &db, QObject *parent)
    : QObject(parent), m_db(db), m_repo(db)
{
}

void Backend::setError(const QString &message)
{
    if (m_lastError == message)
        return;
    m_lastError = message;
    emit lastErrorChanged();
}

bool Backend::find(int id, BoardRecord *out)
{
    QString error;
    const QList<BoardRecord> all = m_repo.list(&error);
    if (!error.isEmpty()) {
        setError(error);
        return false;
    }
    for (const BoardRecord &r : all) {
        if (r.id == id) {
            *out = r;
            return true;
        }
    }
    setError(tr("Schéma introuvable."));
    return false;
}

QVariantList Backend::boards()
{
    QVariantList result;
    QString error;
    const QList<BoardRecord> all = m_repo.list(&error);
    setError(error);
    for (const BoardRecord &r : all) {
        Tactics::Board parsed;
        QVariantMap m;
        m.insert(QStringLiteral("id"), r.id);
        m.insert(QStringLiteral("title"), r.title);
        m.insert(QStringLiteral("updatedAt"), r.updatedAt);
        m.insert(QStringLiteral("exerciseId"), r.exerciseId);
        m.insert(QStringLiteral("sessionId"), r.sessionId);
        const bool readable = Tactics::fromJson(r.sceneJson, &parsed);
        m.insert(QStringLiteral("readable"), readable);
        m.insert(QStringLiteral("itemCount"), readable ? int(parsed.items.size()) : 0);
        // Contenu normalisé pour les miniatures de la liste.
        m.insert(QStringLiteral("json"), readable ? Tactics::toJson(parsed) : QString());
        // Contenu normalisé pour les miniatures de la liste.
        m.insert(QStringLiteral("json"), readable ? Tactics::toJson(parsed) : QString());
        result.append(m);
    }
    return result;
}

QVariantMap Backend::board(int id)
{
    QVariantMap m;
    BoardRecord r;
    if (!find(id, &r))
        return m;
    Tactics::Board parsed;
    QString parseError;
    const bool readable = Tactics::fromJson(r.sceneJson, &parsed, &parseError);
    m.insert(QStringLiteral("id"), r.id);
    m.insert(QStringLiteral("title"), r.title);
    m.insert(QStringLiteral("readable"), readable);
    m.insert(QStringLiteral("error"), parseError);
    // JSON normalisé quand il est lisible ; sinon rien : l'interface n'a rien à éditer.
    m.insert(QStringLiteral("json"), readable ? Tactics::toJson(parsed) : QString());
    return m;
}

int Backend::createBoard(const QString &title)
{
    BoardRecord r;
    r.title = title.trimmed().isEmpty() ? tr("Nouveau schéma") : title.trimmed();
    QString error;
    if (!m_repo.insert(r, &error)) {
        setError(error);
        return -1;
    }
    setError(QString());
    return r.id;
}

int Backend::duplicateBoard(int id)
{
    BoardRecord r;
    if (!find(id, &r))
        return -1;
    r.id = -1;
    r.title = tr("%1 (copie)").arg(r.title);
    QString error;
    if (!m_repo.insert(r, &error)) {
        setError(error);
        return -1;
    }
    setError(QString());
    return r.id;
}

bool Backend::renameBoard(int id, const QString &title)
{
    BoardRecord r;
    if (!find(id, &r))
        return false;
    const QString trimmed = title.trimmed();
    if (trimmed.isEmpty()) {
        setError(tr("Le titre ne peut pas être vide."));
        return false;
    }
    r.title = trimmed;
    QString error;
    if (!m_repo.update(r, &error)) {
        setError(error);
        return false;
    }
    setError(QString());
    return true;
}

bool Backend::deleteBoard(int id)
{
    QString error;
    if (!m_repo.remove(id, &error)) {
        setError(error);
        return false;
    }
    setError(QString());
    return true;
}

bool Backend::saveBoard(int id, const QString &json)
{
    BoardRecord r;
    if (!find(id, &r))
        return false;

    // Un schéma stocké illisible n'est jamais écrasé.
    Tactics::Board stored;
    if (!Tactics::fromJson(r.sceneJson, &stored)) {
        setError(tr("Le contenu enregistré de ce schéma est illisible : il n'est pas modifié."));
        return false;
    }
    Tactics::Board incoming;
    QString parseError;
    if (!Tactics::fromJson(json, &incoming, &parseError)) {
        setError(parseError);
        return false;
    }
    r.sceneJson = Tactics::toJson(incoming);
    QString error;
    if (!m_repo.update(r, &error)) {
        setError(error);
        return false;
    }
    setError(QString());
    return true;
}

QStringList Backend::formationNames() const
{
    return Tactics::formationNames();
}

QString Backend::formationJson(const QString &name, bool withOpponents) const
{
    const QList<Tactics::Item> players = Tactics::formation(name);
    if (players.isEmpty())
        return QString();
    Tactics::Board b;
    b.items = players;
    if (withOpponents)
        b.items.append(Tactics::mirroredAsOpponent(players));
    return Tactics::toJson(b);
}

QVariantList Backend::palette() const
{
    QVariantList result;
    for (const auto &c : Tactics::colorPalette()) {
        QVariantMap m;
        m.insert(QStringLiteral("name"), c.first);
        m.insert(QStringLiteral("color"), c.second);
        result.append(m);
    }
    return result;
}
