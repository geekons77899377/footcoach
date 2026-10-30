#include "Backend.h"

#include "core/TacticalBoard.h"

#include <QDate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Backend::Backend(const QSqlDatabase &db, QObject *parent)
    : QObject(parent), m_db(db), m_repo(db), m_teams(db), m_players(db), m_sessions(db), m_attendance(db)
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

// --- Équipes et joueurs -------------------------------------------------------------------------------

namespace {

QVariantMap result(bool ok, int id = -1, const QString &error = QString())
{
    QVariantMap m;
    m.insert(QStringLiteral("ok"), ok);
    m.insert(QStringLiteral("id"), id);
    m.insert(QStringLiteral("error"), error);
    return m;
}

int ageOf(const QDate &birth)
{
    if (!birth.isValid())
        return -1;
    const QDate today = QDate::currentDate();
    int age = today.year() - birth.year();
    if (today.month() < birth.month() || (today.month() == birth.month() && today.day() < birth.day()))
        --age;
    return age < 0 ? -1 : age;
}

QVariantMap toMap(const Player &p)
{
    QVariantMap m;
    m.insert(QStringLiteral("id"), p.id);
    m.insert(QStringLiteral("teamId"), p.teamId);
    m.insert(QStringLiteral("firstName"), p.firstName);
    m.insert(QStringLiteral("lastName"), p.lastName);
    m.insert(QStringLiteral("fullName"), p.fullName());
    m.insert(QStringLiteral("birthDate"), p.birthDate.isValid() ? p.birthDate.toString(Qt::ISODate) : QString());
    m.insert(QStringLiteral("age"), ageOf(p.birthDate));
    m.insert(QStringLiteral("jerseyNumber"), p.jerseyNumber);
    m.insert(QStringLiteral("position"), p.position);
    m.insert(QStringLiteral("preferredFoot"), p.preferredFoot);
    m.insert(QStringLiteral("phone"), p.phone);
    m.insert(QStringLiteral("email"), p.email);
    m.insert(QStringLiteral("notes"), p.notes);
    m.insert(QStringLiteral("active"), p.active);
    return m;
}

} // namespace

QVariantList Backend::teams()
{
    QVariantList list;
    QString error;
    const QList<Team> all = m_teams.all(&error);
    setError(error);
    for (const Team &t : all) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), t.id);
        m.insert(QStringLiteral("name"), t.name);
        m.insert(QStringLiteral("category"), t.category);
        m.insert(QStringLiteral("season"), t.season);
        m.insert(QStringLiteral("playerCount"), m_teams.playerCount(t.id));
        list.append(m);
    }
    return list;
}

QVariantMap Backend::saveTeam(const QVariantMap &team)
{
    Team t;
    t.id = team.value(QStringLiteral("id"), -1).toInt();
    t.name = team.value(QStringLiteral("name")).toString().trimmed();
    t.category = team.value(QStringLiteral("category")).toString().trimmed();
    t.season = team.value(QStringLiteral("season")).toString().trimmed();
    if (t.name.isEmpty()) {
        setError(tr("Le nom de l'équipe est obligatoire."));
        return result(false, -1, m_lastError);
    }
    QString error;
    const bool ok = t.id > 0 ? m_teams.update(t, &error) : m_teams.insert(t, &error);
    setError(error);
    return ok ? result(true, t.id) : result(false, -1, error);
}

QVariantMap Backend::deleteTeam(int id)
{
    QString error;
    const bool ok = m_teams.remove(id, &error);
    setError(error);
    return result(ok, id, error);
}

QVariantList Backend::players(int teamId, bool includeInactive)
{
    QVariantList list;
    QString error;
    const QList<Player> all = m_players.list(teamId, includeInactive, &error);
    setError(error);
    for (const Player &p : all)
        list.append(toMap(p));
    return list;
}

QVariantMap Backend::savePlayer(const QVariantMap &in)
{
    Player p;
    p.id = in.value(QStringLiteral("id"), -1).toInt();
    p.teamId = in.value(QStringLiteral("teamId"), -1).toInt();
    p.firstName = in.value(QStringLiteral("firstName")).toString().trimmed();
    p.lastName = in.value(QStringLiteral("lastName")).toString().trimmed();
    p.jerseyNumber = in.value(QStringLiteral("jerseyNumber"), 0).toInt();
    p.position = in.value(QStringLiteral("position")).toString();
    p.preferredFoot = in.value(QStringLiteral("preferredFoot")).toString();
    p.phone = in.value(QStringLiteral("phone")).toString().trimmed();
    p.email = in.value(QStringLiteral("email")).toString().trimmed();
    p.notes = in.value(QStringLiteral("notes")).toString().trimmed();
    p.active = in.value(QStringLiteral("active"), true).toBool();

    const QString birth = in.value(QStringLiteral("birthDate")).toString().trimmed();
    if (!birth.isEmpty()) {
        p.birthDate = QDate::fromString(birth, Qt::ISODate);
        if (!p.birthDate.isValid() || p.birthDate > QDate::currentDate()) {
            setError(tr("La date de naissance n'est pas valide."));
            return result(false, -1, m_lastError);
        }
    }

    QString problem;
    if (p.teamId <= 0)
        problem = tr("Aucune équipe sélectionnée.");
    else if (p.firstName.isEmpty() || p.lastName.isEmpty())
        problem = tr("Le nom et le prénom sont obligatoires.");
    else if (p.jerseyNumber < 0 || p.jerseyNumber > 99)
        problem = tr("Le numéro de maillot doit être compris entre 1 et 99.");
    else if (!p.position.isEmpty() && p.position != QLatin1String("GK") && p.position != QLatin1String("DEF")
             && p.position != QLatin1String("MID") && p.position != QLatin1String("FWD"))
        problem = tr("Poste inconnu.");
    else if (!p.preferredFoot.isEmpty() && p.preferredFoot != QLatin1String("R")
             && p.preferredFoot != QLatin1String("L") && p.preferredFoot != QLatin1String("B"))
        problem = tr("Pied fort inconnu.");
    else if (p.active && p.jerseyNumber > 0 && m_players.jerseyTaken(p.teamId, p.jerseyNumber, p.id))
        problem = tr("Le numéro %1 est déjà porté par un autre joueur actif de l'équipe.").arg(p.jerseyNumber);
    if (!problem.isEmpty()) {
        setError(problem);
        return result(false, -1, problem);
    }

    QString error;
    const bool ok = p.id > 0 ? m_players.update(p, &error) : m_players.insert(p, &error);
    setError(error);
    return ok ? result(true, p.id) : result(false, -1, error);
}

QVariantMap Backend::setPlayerActive(int id, bool active)
{
    QString error;
    if (active) {
        // Réactivation : le numéro ne doit pas avoir été repris entre-temps.
        for (const Team &t : m_teams.all(&error)) {
            for (const Player &p : m_players.list(t.id, true, &error)) {
                if (p.id == id && p.jerseyNumber > 0 && m_players.jerseyTaken(p.teamId, p.jerseyNumber, id)) {
                    const QString msg = tr("Le numéro %1 est maintenant porté par un autre joueur actif. "
                                           "Changez-le avant de réactiver ce joueur.").arg(p.jerseyNumber);
                    setError(msg);
                    return result(false, id, msg);
                }
            }
        }
    }
    const bool ok = m_players.setActive(id, active, &error);
    setError(error);
    return result(ok, id, error);
}

QVariantMap Backend::deletePlayer(int id)
{
    QString error;
    const bool ok = m_players.remove(id, &error);
    setError(error);
    return result(ok, id, error);
}
