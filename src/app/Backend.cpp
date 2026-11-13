#include "Backend.h"

#include <algorithm>

#include "core/TacticalBoard.h"

#include <QBuffer>
#include <QDate>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QUrl>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>
#endif
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Backend::Backend(const QSqlDatabase &db, QObject *parent)
    : QObject(parent), m_db(db), m_repo(db), m_teams(db), m_clubs(db), m_matches(db), m_players(db), m_sessions(db), m_slots(db), m_attendance(db), m_exercises(db), m_plan(db)
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
    if (m_repo.get(id, out, &error))
        return true;
    setError(error.isEmpty() ? tr("Schéma introuvable.") : error);
    return false;
}

QVariantList Backend::boards()
{
    QVariantList result;
    QString error;
    // Bibliothèque de schémas : les terrains des blocs de séance (copies propres à une séance) n'y figurent pas.
    const QList<BoardRecord> all = m_repo.list(&error, false);
    setError(error);
    for (const BoardRecord &r : all) {
        Tactics::Board parsed;
        QVariantMap m;
        m.insert(QStringLiteral("id"), r.id);
        m.insert(QStringLiteral("title"), r.title);
        m.insert(QStringLiteral("updatedAt"), r.updatedAt);
        m.insert(QStringLiteral("exerciseId"), r.exerciseId);
        m.insert(QStringLiteral("sessionId"), r.sessionId);
        m.insert(QStringLiteral("sessionExerciseId"), r.sessionExerciseId);
        m.insert(QStringLiteral("matchId"), r.matchId);
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

QVariantMap Backend::exerciseBoard(int exerciseId)
{
    QVariantMap m;
    m.insert(QStringLiteral("boardId"), 0);
    BoardRecord r;
    QString error;
    if (!m_repo.referenceForExercise(exerciseId, &r, &error)) {
        setError(error);
        return m;
    }
    Tactics::Board parsed;
    const bool readable = Tactics::fromJson(r.sceneJson, &parsed);
    m.insert(QStringLiteral("boardId"), r.id);
    m.insert(QStringLiteral("title"), r.title);
    m.insert(QStringLiteral("readable"), readable);
    m.insert(QStringLiteral("json"), readable ? Tactics::toJson(parsed) : QString());
    setError(QString());
    return m;
}

QVariantMap Backend::createExerciseBoard(int exerciseId)
{
    QVariantMap m;
    auto fail = [&](const QString &msg) {
        setError(msg);
        m.insert(QStringLiteral("ok"), false);
        m.insert(QStringLiteral("error"), msg);
        return m;
    };
    QString error;
    QString title;
    for (const Exercise &e : m_exercises.list(true, &error))
        if (e.id == exerciseId)
            title = e.title;
    if (title.isEmpty())
        return fail(error.isEmpty() ? tr("Cet exercice n'existe plus.") : error);

    BoardRecord existing;
    if (m_repo.referenceForExercise(exerciseId, &existing, &error)) {
        m.insert(QStringLiteral("ok"), true);
        m.insert(QStringLiteral("boardId"), existing.id);
        m.insert(QStringLiteral("created"), false);
        return m;
    }
    if (!error.isEmpty())
        return fail(error);
    BoardRecord r;
    r.title = title;
    r.exerciseId = exerciseId;
    if (!m_repo.insert(r, &error))
        return fail(error);
    setError(QString());
    m.insert(QStringLiteral("ok"), true);
    m.insert(QStringLiteral("boardId"), r.id);
    m.insert(QStringLiteral("created"), true);
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
    // La copie est un schéma libre : elle ne devient ni un deuxième schéma de référence d'un exercice ni un
    // deuxième terrain du même bloc.
    r.exerciseId = -1;
    r.sessionExerciseId = -1;
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

QStringList Backend::formationNamesFor(int perSide) const
{
    return Tactics::formationNames(perSide);
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
    m.insert(QStringLiteral("licence"), p.licence);
    m.insert(QStringLiteral("hasPhoto"), p.hasPhoto);
    return m;
}

constexpr int kPhotoMaxSide = 480;               // côté le plus long de la photo stockée
constexpr qint64 kPhotoMaxSource = 40LL * 1024 * 1024;   // image choisie : 40 Mo au plus
constexpr int kPhotoMaxStored = 400 * 1024;      // octets de la photo réduite
const QString kPhotoPrefix = QStringLiteral("data:image/jpeg;base64,");

QByteArray decodePhotoData(const QString &dataUrl)
{
    if (!dataUrl.startsWith(kPhotoPrefix))
        return {};
    return QByteArray::fromBase64(dataUrl.mid(kPhotoPrefix.size()).toLatin1());
}

// Lit l'image (file://, content:// ou chemin brut), la réduit en JPEG ; faux + message si impossible.
bool reducePhotoFile(const QString &url, QByteArray *jpegOut, QString *error)
{
    auto fail = [&](const char *msg) {
        *error = QCoreApplication::translate("Backend", msg);
        return false;
    };
    const QUrl u(url);
    const QString path = u.isLocalFile() ? u.toLocalFile() : url;   // content:// (Android) s'ouvre tel quel avec QFile
    QFile f(path);
    if (path.isEmpty() || !f.open(QIODevice::ReadOnly))
        return fail("Impossible d'ouvrir cette image.");
    if (f.size() > kPhotoMaxSource)
        return fail("Cette image est trop volumineuse.");
    QImageReader reader(&f);
    reader.setAutoTransform(true);          // respecte l'orientation EXIF des photos de téléphone
    QImage img = reader.read();
    if (img.isNull())
        return fail("Ce fichier n'est pas une image lisible.");
    if (img.width() > kPhotoMaxSide || img.height() > kPhotoMaxSide)
        img = img.scaled(kPhotoMaxSide, kPhotoMaxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    // JPEG sans transparence : fond blanc.
    QImage flat(img.size(), QImage::Format_RGB32);
    flat.fill(Qt::white);
    {
        QPainter painter(&flat);
        painter.drawImage(0, 0, img);
    }
    QByteArray jpeg;
    for (int quality : {85, 70, 55}) {
        jpeg.clear();
        QBuffer buf(&jpeg);
        buf.open(QIODevice::WriteOnly);
        if (!flat.save(&buf, "JPEG", quality))
            return fail("Impossible de réduire cette image.");
        if (jpeg.size() <= kPhotoMaxStored)
            break;
    }
    if (jpeg.size() > kPhotoMaxStored)
        return fail("Cette image est trop volumineuse.");
    *jpegOut = jpeg;
    return true;
}

} // namespace

QVariantMap Backend::preparePhoto(const QString &url)
{
    QVariantMap out;
    QByteArray jpeg;
    QString error;
    const bool ok = reducePhotoFile(url, &jpeg, &error);
    out.insert(QStringLiteral("ok"), ok);
    if (ok)
        out.insert(QStringLiteral("data"), kPhotoPrefix + QString::fromLatin1(jpeg.toBase64()));
    else
        out.insert(QStringLiteral("error"), error);
    return out;
}

// Import en lot : « <licence>.jpg » → joueur de l'équipe qui porte cette licence (casse ignorée). Chaque fichier est
// indépendant ; une photo existante est remplacée. Compte-rendu par fichier : status = added | replaced | unknown | error.
QVariantMap Backend::importPhotos(int teamId, const QVariantList &urls)
{
    constexpr int kMaxFiles = 400;
    QVariantMap out;
    QVariantList items;
    int added = 0, replaced = 0, failed = 0;
    auto finish = [&](bool ok, const QString &error) {
        out.insert(QStringLiteral("ok"), ok);
        out.insert(QStringLiteral("error"), error);
        out.insert(QStringLiteral("added"), added);
        out.insert(QStringLiteral("replaced"), replaced);
        out.insert(QStringLiteral("failed"), failed);
        out.insert(QStringLiteral("items"), items);
        if (!ok)
            setError(error);
        return out;
    };

    QString error;
    bool teamFound = false;
    for (const Team &t : m_teams.all(&error))
        teamFound = teamFound || t.id == teamId;
    if (!teamFound)
        return finish(false, tr("Aucune équipe sélectionnée."));
    if (urls.isEmpty())
        return finish(false, tr("Aucune photo choisie."));
    if (urls.size() > kMaxFiles)
        return finish(false, tr("Trop de fichiers (%1 au plus).").arg(kMaxFiles));

    QHash<QString, Player> byLicence;       // licence normalisée -> joueur
    for (const Player &p : m_players.list(teamId, true, &error)) {
        if (!p.licence.isEmpty())
            byLicence.insert(p.licence, p);
    }
    QSet<QString> seen;                     // une licence n'est traitée qu'une fois par import
    for (const QVariant &v : urls) {
        const QString url = v.toString();
        const QUrl u(url);
        const QString path = u.isLocalFile() ? u.toLocalFile() : url;
        const QString file = QFileInfo(path).fileName();
        const QString base = PlayerRepository::normalizeLicence(QFileInfo(path).completeBaseName());
        QVariantMap item{{QStringLiteral("file"), file}};
        auto add = [&](const QString &status, const QString &message, const QString &player = QString()) {
            item.insert(QStringLiteral("status"), status);
            item.insert(QStringLiteral("message"), message);
            item.insert(QStringLiteral("player"), player);
            items.append(item);
        };
        if (!byLicence.contains(base)) {
            ++failed;
            add(QStringLiteral("unknown"), tr("Aucun joueur de l'équipe n'a ce numéro de licence."));
            continue;
        }
        if (seen.contains(base)) {
            ++failed;
            add(QStringLiteral("error"), tr("Plusieurs fichiers pour la même licence : seul le premier est utilisé."));
            continue;
        }
        seen.insert(base);
        const Player &p = byLicence.value(base);
        QByteArray jpeg;
        QString problem;
        if (!reducePhotoFile(url, &jpeg, &problem) || !m_players.setPhoto(p.id, jpeg, &problem)) {
            ++failed;
            add(QStringLiteral("error"), problem, p.fullName());
            continue;
        }
        if (p.hasPhoto) {
            ++replaced;
            add(QStringLiteral("replaced"), tr("Photo remplacée."), p.fullName());
        } else {
            ++added;
            add(QStringLiteral("added"), tr("Photo ajoutée."), p.fullName());
        }
    }
    return finish(true, QString());
}

QString Backend::playerPhoto(int id)
{
    const QByteArray jpeg = m_players.photo(id);
    return jpeg.isEmpty() ? QString() : kPhotoPrefix + QString::fromLatin1(jpeg.toBase64());
}

QVariantList Backend::clubs()
{
    QVariantList list;
    QString error;
    const QList<Club> all = m_clubs.all(&error);
    setError(error);
    for (const Club &c : all) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), c.id);
        m.insert(QStringLiteral("name"), c.name);
        m.insert(QStringLiteral("teamCount"), m_clubs.teamCount(c.id));
        list.append(m);
    }
    return list;
}

QVariantMap Backend::saveClub(const QVariantMap &club)
{
    Club c;
    c.id = club.value(QStringLiteral("id"), -1).toInt();
    c.name = club.value(QStringLiteral("name")).toString().trimmed();
    if (c.name.isEmpty()) {
        setError(tr("Le nom du club est obligatoire."));
        return result(false, -1, m_lastError);
    }
    QString error;
    const bool ok = c.id > 0 ? m_clubs.update(c, &error) : m_clubs.insert(c, &error);
    setError(error);
    return ok ? result(true, c.id) : result(false, -1, error);
}

QVariantMap Backend::deleteClub(int id)
{
    QString error;
    const bool ok = m_clubs.remove(id, &error);
    setError(error);
    return result(ok, id, error);
}

QVariantList Backend::teams(int clubId)
{
    QVariantList list;
    QString error;
    const QList<Team> all = m_teams.all(&error, clubId);
    setError(error);
    QHash<int, QString> clubNames;
    for (const Club &c : m_clubs.all())
        clubNames.insert(c.id, c.name);
    for (const Team &t : all) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), t.id);
        m.insert(QStringLiteral("clubId"), t.clubId);
        m.insert(QStringLiteral("clubName"), clubNames.value(t.clubId));
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
    t.clubId = team.value(QStringLiteral("clubId"), -1).toInt();
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

QVariantList Backend::allPlayers(bool includeInactive)
{
    QString error;
    QList<QPair<QString, QVariantMap>> rows;
    for (const Team &t : m_teams.all(&error)) {
        for (const Player &p : m_players.list(t.id, includeInactive, &error)) {
            QVariantMap m = toMap(p);
            m.insert(QStringLiteral("teamName"), t.name);
            rows.append({p.lastName.toLower() + QLatin1Char('\x01') + p.firstName.toLower(), m});
        }
    }
    setError(error);
    std::stable_sort(rows.begin(), rows.end(), [](const auto &a, const auto &b) { return a.first.localeAwareCompare(b.first) < 0; });
    QVariantList list;
    for (const auto &r : std::as_const(rows))
        list.append(r.second);
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
    p.licence = PlayerRepository::normalizeLicence(in.value(QStringLiteral("licence")).toString());

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
    else if (p.firstName.isEmpty())
        problem = tr("Le prénom est obligatoire.");
    else if (m_players.nameTaken(p.teamId, p.firstName, p.lastName, p.id))
        problem = tr("%1 est déjà dans l'effectif de cette équipe.").arg(p.fullName());
    else if (p.licence.size() > 20 || p.licence.contains(QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}]"))))
        problem = tr("Le numéro de licence ne peut contenir que des lettres et des chiffres (20 au plus).");
    else if (m_players.licenceTaken(p.teamId, p.licence, p.id))
        problem = tr("Le numéro de licence %1 est déjà utilisé dans cette équipe.").arg(p.licence);
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
    // Photo : « photo » = image déjà réduite par preparePhoto(), « removePhoto » = la retirer ; sinon inchangée.
    QByteArray newPhoto;
    const bool removePhoto = in.value(QStringLiteral("removePhoto")).toBool();
    const QString photoData = in.value(QStringLiteral("photo")).toString();
    if (problem.isEmpty() && !photoData.isEmpty()) {
        newPhoto = decodePhotoData(photoData);
        QImage check;
        if (newPhoto.isEmpty() || newPhoto.size() > kPhotoMaxStored || !check.loadFromData(newPhoto, "JPEG"))
            problem = tr("La photo n'est pas valide.");
    }
    if (!problem.isEmpty()) {
        setError(problem);
        return result(false, -1, problem);
    }

    QString error;
    bool ok = p.id > 0 ? m_players.update(p, &error) : m_players.insert(p, &error);
    if (ok && (removePhoto || !newPhoto.isEmpty()))
        ok = m_players.setPhoto(p.id, removePhoto && newPhoto.isEmpty() ? QByteArray() : newPhoto, &error);
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

QVariantMap Backend::deletePlayers(const QVariantList &ids)
{
    QString error;
    QList<int> list;
    for (const QVariant &v : ids) {
        const int id = v.toInt();
        if (id > 0 && !list.contains(id))
            list.append(id);
    }
    if (list.isEmpty()) {
        setError(tr("Aucun joueur sélectionné."));
        QVariantMap r = result(false, -1, m_lastError);
        r.insert(QStringLiteral("removed"), 0);
        return r;
    }
    bool ok = m_db.transaction();
    if (!ok)
        error = m_db.lastError().text();
    for (int id : std::as_const(list)) {
        if (!ok)
            break;
        ok = m_players.remove(id, &error);
    }
    if (ok && !m_db.commit()) {
        ok = false;
        error = m_db.lastError().text();
    }
    if (!ok)
        m_db.rollback();
    setError(error);
    QVariantMap r = result(ok, -1, error);
    r.insert(QStringLiteral("removed"), ok ? int(list.size()) : 0);
    return r;
}

QVariantMap Backend::movePlayers(const QVariantList &ids, int teamId)
{
    QVariantMap out;
    out.insert(QStringLiteral("moved"), 0);
    out.insert(QStringLiteral("numbersCleared"), 0);
    out.insert(QStringLiteral("skipped"), QStringList());
    auto fail = [&](const QString &message) {
        setError(message);
        out.insert(QStringLiteral("ok"), false);
        out.insert(QStringLiteral("error"), message);
        return out;
    };
    QString error;
    bool teamFound = false;
    for (const Team &t : m_teams.all(&error))
        teamFound = teamFound || t.id == teamId;
    if (!teamFound)
        return fail(error.isEmpty() ? tr("Équipe d'arrivée introuvable.") : error);
    QList<int> list;
    for (const QVariant &v : ids)
        if (v.toInt() > 0 && !list.contains(v.toInt()))
            list.append(v.toInt());
    if (list.isEmpty())
        return fail(tr("Aucun joueur sélectionné."));

    int moved = 0, cleared = 0;
    QStringList skipped;
    bool ok = m_db.transaction();
    if (!ok)
        error = m_db.lastError().text();
    for (int id : std::as_const(list)) {
        if (!ok)
            break;
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral("SELECT team_id, first_name, last_name, IFNULL(licence, ''), IFNULL(jersey_number, 0), active "
                                 "FROM player WHERE id = ?"));
        q.addBindValue(id);
        if (!q.exec()) {
            ok = false;
            error = q.lastError().text();
            break;
        }
        if (!q.next() || q.value(0).toInt() == teamId)
            continue;                                   // introuvable ou déjà dans l'équipe : rien à faire
        const QString first = q.value(1).toString(), last = q.value(2).toString(), licence = q.value(3).toString();
        const int number = q.value(4).toInt();
        const bool active = q.value(5).toInt() != 0;
        if (m_players.nameTaken(teamId, first, last, id) || (!licence.isEmpty() && m_players.licenceTaken(teamId, licence, id))) {
            skipped << QStringLiteral("%1 %2").arg(first, last).trimmed();
            continue;
        }
        const bool clear = active && number > 0 && m_players.jerseyTaken(teamId, number, id);
        QSqlQuery u(m_db);
        u.prepare(clear ? QStringLiteral("UPDATE player SET team_id = ?, jersey_number = NULL WHERE id = ?")
                        : QStringLiteral("UPDATE player SET team_id = ? WHERE id = ?"));
        u.addBindValue(teamId);
        u.addBindValue(id);
        if (!u.exec()) {
            ok = false;
            error = u.lastError().text();
            break;
        }
        ++moved;
        cleared += clear ? 1 : 0;
    }
    if (ok && !m_db.commit()) {
        ok = false;
        error = m_db.lastError().text();
    }
    if (!ok) {
        m_db.rollback();
        return fail(error);
    }
    setError(QString());
    out.insert(QStringLiteral("ok"), true);
    out.insert(QStringLiteral("error"), QString());
    out.insert(QStringLiteral("moved"), moved);
    out.insert(QStringLiteral("numbersCleared"), cleared);
    out.insert(QStringLiteral("skipped"), skipped);
    return out;
}

QVariantMap Backend::importPlayers(int teamId, const QString &text)
{
    constexpr int kMaxPlayers = 300;
    QVariantMap out;
    out.insert(QStringLiteral("created"), 0);
    out.insert(QStringLiteral("errors"), QVariantList());
    auto fail = [&](const QString &message) {
        setError(message);
        out.insert(QStringLiteral("ok"), false);
        out.insert(QStringLiteral("error"), message);
        return out;
    };

    QString error;
    bool teamFound = false;
    for (const Team &t : m_teams.all(&error))
        teamFound = teamFound || t.id == teamId;
    if (!teamFound)
        return fail(tr("Aucune équipe sélectionnée."));

    QSet<QString> keys;                     // clés prénom + nom : joueurs existants (actifs ou non) puis ceux de la liste
    QHash<int, QString> jerseys;            // numéro -> joueur actif qui le porte
    for (const Player &p : m_players.list(teamId, true, &error)) {
        keys.insert(PlayerRepository::nameKey(p.firstName, p.lastName));
        if (p.active && p.jerseyNumber > 0)
            jerseys.insert(p.jerseyNumber, p.fullName());
    }

    static const QHash<QString, QString> positions = {
        {QStringLiteral("gk"), QStringLiteral("GK")}, {QStringLiteral("g"), QStringLiteral("GK")},
        {QStringLiteral("gardien"), QStringLiteral("GK")},
        {QStringLiteral("def"), QStringLiteral("DEF")}, {QStringLiteral("d"), QStringLiteral("DEF")},
        {QStringLiteral("defenseur"), QStringLiteral("DEF")},
        {QStringLiteral("mid"), QStringLiteral("MID")}, {QStringLiteral("m"), QStringLiteral("MID")},
        {QStringLiteral("milieu"), QStringLiteral("MID")},
        {QStringLiteral("fwd"), QStringLiteral("FWD")}, {QStringLiteral("a"), QStringLiteral("FWD")},
        {QStringLiteral("attaquant"), QStringLiteral("FWD")}};

    QList<Player> toCreate;
    QVariantList errors;
    QHash<QString, int> keyLine;            // clé -> ligne de la liste, pour signaler les doublons internes
    auto addError = [&](int line, const QString &message) {
        errors.append(QVariantMap{{QStringLiteral("line"), line}, {QStringLiteral("message"), message}});
    };

    const QStringList lines = text.split(QLatin1Char('\n'));
    bool first = true;
    for (int i = 0; i < lines.size(); ++i) {
        const QString raw = lines.at(i).trimmed();
        if (raw.isEmpty())
            continue;
        QStringList cells = raw.split(QRegularExpression(QStringLiteral("[;\t]")));
        for (QString &c : cells)
            c = c.trimmed();
        const int lineNo = i + 1;
        if (first) {
            first = false;
            if (PlayerRepository::fold(cells.at(0)) == QLatin1String("prenom"))
                continue;                   // ligne d'en-tête d'un tableur
        }
        if (toCreate.size() >= kMaxPlayers) {
            addError(lineNo, tr("Trop de joueurs : %1 au maximum par import.").arg(kMaxPlayers));
            break;
        }
        if (cells.size() > 5) {
            addError(lineNo, tr("Trop de colonnes (Prénom ; Nom ; Numéro ; Poste ; Naissance)."));
            continue;
        }
        Player p;
        p.teamId = teamId;
        p.firstName = cells.value(0);
        p.lastName = cells.value(1);
        if (p.firstName.isEmpty()) {
            addError(lineNo, tr("Le prénom est obligatoire."));
            continue;
        }
        const QString num = cells.value(2);
        if (!num.isEmpty()) {
            bool okNum = false;
            const int n = num.toInt(&okNum);
            if (!okNum || n < 1 || n > 99) {
                addError(lineNo, tr("Numéro « %1 » invalide (1 à 99).").arg(num));
                continue;
            }
            p.jerseyNumber = n;
        }
        const QString pos = cells.value(3);
        if (!pos.isEmpty()) {
            p.position = positions.value(PlayerRepository::fold(pos));
            if (p.position.isEmpty()) {
                addError(lineNo, tr("Poste « %1 » inconnu (gardien, défenseur, milieu, attaquant).").arg(pos));
                continue;
            }
        }
        const QString birth = cells.value(4);
        if (!birth.isEmpty()) {
            QDate d = QDate::fromString(birth, QStringLiteral("d/M/yyyy"));
            if (!d.isValid())
                d = QDate::fromString(birth, Qt::ISODate);
            if (!d.isValid() || d > QDate::currentDate()) {
                addError(lineNo, tr("Date de naissance « %1 » invalide (JJ/MM/AAAA).").arg(birth));
                continue;
            }
            p.birthDate = d;
        }
        const QString key = PlayerRepository::nameKey(p.firstName, p.lastName);
        if (keyLine.contains(key)) {
            addError(lineNo, tr("%1 figure déjà à la ligne %2 de la liste.").arg(p.fullName()).arg(keyLine.value(key)));
            continue;
        }
        if (keys.contains(key)) {
            addError(lineNo, tr("%1 est déjà dans l'effectif de cette équipe.").arg(p.fullName()));
            continue;
        }
        if (p.jerseyNumber > 0 && jerseys.contains(p.jerseyNumber)) {
            addError(lineNo, tr("Le numéro %1 est déjà porté par %2.").arg(p.jerseyNumber).arg(jerseys.value(p.jerseyNumber)));
            continue;
        }
        keyLine.insert(key, lineNo);
        if (p.jerseyNumber > 0)
            jerseys.insert(p.jerseyNumber, p.fullName());
        toCreate.append(p);
    }

    if (!errors.isEmpty()) {
        out.insert(QStringLiteral("errors"), errors);
        return fail(tr("%n ligne(s) à corriger. Rien n'a été importé.", nullptr, errors.size()));
    }
    if (toCreate.isEmpty())
        return fail(tr("Aucun joueur à importer."));

    if (!m_db.transaction())
        return fail(m_db.lastError().text());
    for (Player &p : toCreate) {
        if (!m_players.insert(p, &error)) {
            m_db.rollback();
            return fail(error);
        }
    }
    if (!m_db.commit()) {
        const QString e = m_db.lastError().text();
        m_db.rollback();
        return fail(e);
    }
    setError(QString());
    out.insert(QStringLiteral("ok"), true);
    out.insert(QStringLiteral("created"), int(toCreate.size()));
    out.insert(QStringLiteral("error"), QString());
    return out;
}

void Backend::setKeepScreenOn(bool on)
{
#ifdef Q_OS_ANDROID
    // android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON = 0x80. À appeler depuis le fil de l'interface
    // Android ; l'activité est celle de Qt.
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([on]() {
        QJniObject activity = QNativeInterface::QAndroidApplication::context();
        if (!activity.isValid())
            return;
        QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (!window.isValid())
            return;
        const jint flag = 0x80;
        if (on)
            window.callMethod<void>("addFlags", "(I)V", flag);
        else
            window.callMethod<void>("clearFlags", "(I)V", flag);
    });
#else
    Q_UNUSED(on);
#endif
}
