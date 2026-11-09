#include "Backend.h"

#include "ChallengeSheet.h"

#include <QBuffer>
#include <QDate>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QHash>
#include <QStandardPaths>
#include <QUrl>
#include <algorithm>

namespace {

const QString kDay = QStringLiteral("yyyy-MM-dd");

QVariantMap result(bool ok, int id = -1, const QString &error = QString())
{
    QVariantMap m;
    m.insert(QStringLiteral("ok"), ok);
    m.insert(QStringLiteral("id"), id);
    m.insert(QStringLiteral("error"), error);
    return m;
}

QVariantMap matchMap(const Match &m, int convoked = 0, bool hasBoard = false)
{
    QVariantMap r;
    r.insert(QStringLiteral("id"), m.id);
    r.insert(QStringLiteral("teamId"), m.teamId);
    r.insert(QStringLiteral("date"), m.startAt.date().toString(kDay));
    r.insert(QStringLiteral("time"), m.startAt.time().toString(QStringLiteral("HH:mm")));
    r.insert(QStringLiteral("opponent"), m.opponent);
    r.insert(QStringLiteral("location"), m.location);
    r.insert(QStringLiteral("homeAway"), m.homeAway);
    r.insert(QStringLiteral("status"), m.status);
    r.insert(QStringLiteral("scoreFor"), m.scoreFor);
    r.insert(QStringLiteral("scoreAgainst"), m.scoreAgainst);
    r.insert(QStringLiteral("notes"), m.notes);
    r.insert(QStringLiteral("convoked"), convoked);
    r.insert(QStringLiteral("hasBoard"), hasBoard);
    return r;
}

} // namespace

QVariantList Backend::matches(int teamId, const QString &from, const QString &to)
{
    QVariantList list;
    const QDate d0 = QDate::fromString(from, kDay), d1 = QDate::fromString(to, kDay);
    if (!d0.isValid() || !d1.isValid())
        return list;
    QString error;
    const QList<Match> all = m_matches.listBetween(teamId, d0, d1, &error);
    setError(error);
    for (const Match &m : all) {
        BoardRecord b;
        list.append(matchMap(m, m_matches.players(m.id).size(), m_repo.forMatch(m.id, &b)));
    }
    return list;
}

QStringList Backend::matchDates(int teamId, const QString &from, const QString &to)
{
    QStringList out;
    const QDate d0 = QDate::fromString(from, kDay), d1 = QDate::fromString(to, kDay);
    if (!d0.isValid() || !d1.isValid())
        return out;
    for (const QDate &d : m_matches.datesWithMatches(teamId, d0, d1))
        out.append(d.toString(kDay));
    out.sort();
    return out;
}

QVariantMap Backend::match(int id)
{
    QString error;
    const Match m = m_matches.get(id, &error);
    if (m.id < 0) {
        const QString msg = error.isEmpty() ? tr("Match introuvable.") : error;
        setError(msg);
        QVariantMap r = result(false, id, msg);
        return r;
    }
    QHash<int, Player> byId;
    for (const Player &p : m_players.list(m.teamId, true, &error))
        byId.insert(p.id, p);
    QVariantList players;
    for (const MatchPlayer &mp : m_matches.players(id, &error)) {
        const Player p = byId.value(mp.playerId);
        QVariantMap e;
        e.insert(QStringLiteral("playerId"), mp.playerId);
        e.insert(QStringLiteral("name"), p.fullName());
        e.insert(QStringLiteral("jerseyNumber"), p.jerseyNumber);
        e.insert(QStringLiteral("role"), mp.role);
        players.append(e);
    }
    BoardRecord b;
    const bool hasBoard = m_repo.forMatch(id, &b, &error);
    setError(error);
    QVariantMap r = result(true, id);
    r.insert(QStringLiteral("match"), matchMap(m, players.size(), hasBoard));
    r.insert(QStringLiteral("players"), players);
    r.insert(QStringLiteral("boardId"), hasBoard ? b.id : -1);
    return r;
}

QVariantMap Backend::saveMatch(const QVariantMap &in)
{
    Match m;
    m.id = in.value(QStringLiteral("id"), -1).toInt();
    m.teamId = in.value(QStringLiteral("teamId"), -1).toInt();
    const QDate day = QDate::fromString(in.value(QStringLiteral("date")).toString(), kDay);
    const QTime time = QTime::fromString(in.value(QStringLiteral("time")).toString(), QStringLiteral("HH:mm"));
    if (day.isValid() && time.isValid())
        m.startAt = QDateTime(day, time);
    m.opponent = in.value(QStringLiteral("opponent")).toString().trimmed();
    m.location = in.value(QStringLiteral("location")).toString().trimmed();
    m.homeAway = in.value(QStringLiteral("homeAway"), QStringLiteral("home")).toString();
    m.status = in.value(QStringLiteral("status"), QStringLiteral("planned")).toString();
    m.scoreFor = in.value(QStringLiteral("scoreFor"), -1).toInt();
    m.scoreAgainst = in.value(QStringLiteral("scoreAgainst"), -1).toInt();
    m.notes = in.value(QStringLiteral("notes")).toString().trimmed();
    QString error;
    const bool ok = m.id > 0 ? m_matches.update(m, &error) : m_matches.insert(m, &error);
    setError(error);
    return ok ? result(true, m.id) : result(false, -1, error);
}

QVariantMap Backend::deleteMatch(int id)
{
    QString error;
    const bool ok = m_matches.remove(id, &error);
    setError(error);
    return result(ok, id, error);
}

QVariantMap Backend::setMatchPlayers(int matchId, const QVariantList &entries)
{
    QList<MatchPlayer> list;
    for (const QVariant &v : entries) {
        const QVariantMap e = v.toMap();
        MatchPlayer p;
        p.playerId = e.value(QStringLiteral("playerId"), -1).toInt();
        p.role = e.value(QStringLiteral("role"), QStringLiteral("starter")).toString();
        list.append(p);
    }
    QString error;
    const bool ok = m_matches.setPlayers(matchId, list, &error);
    setError(error);
    return result(ok, matchId, error);
}

QVariantMap Backend::createMatchBoard(int matchId)
{
    QString error;
    const Match m = m_matches.get(matchId, &error);
    if (m.id < 0) {
        const QString msg = error.isEmpty() ? tr("Match introuvable.") : error;
        setError(msg);
        return result(false, matchId, msg);
    }
    BoardRecord existing;
    if (m_repo.forMatch(matchId, &existing, &error)) {
        setError(QString());
        QVariantMap r = result(true, matchId);
        r.insert(QStringLiteral("boardId"), existing.id);
        r.insert(QStringLiteral("created"), false);
        return r;
    }
    if (!error.isEmpty()) {
        setError(error);
        return result(false, matchId, error);
    }
    BoardRecord record;
    record.title = tr("Match contre %1").arg(m.opponent);
    record.matchId = matchId;
    if (!m_repo.insert(record, &error)) {
        setError(error);
        return result(false, matchId, error);
    }
    setError(QString());
    QVariantMap r = result(true, matchId);
    r.insert(QStringLiteral("boardId"), record.id);
    r.insert(QStringLiteral("created"), true);
    return r;
}

// --- Feuille de challenge ---------------------------------------------------------------------------------

namespace {

QString slug(const QString &s)
{
    QString out;
    for (const QChar c : PlayerRepository::fold(s)) {
        if (c.isLetterOrNumber() && c.unicode() < 128)
            out += c;
        else if (!out.isEmpty() && !out.endsWith(QLatin1Char('-')))
            out += QLatin1Char('-');
    }
    while (out.endsWith(QLatin1Char('-')))
        out.chop(1);
    return out.isEmpty() ? QStringLiteral("equipe") : out;
}

// Ordre alphabétique : nom, puis prénom, sans tenir compte des accents ni de la casse.
void sortByName(QList<Player> *players)
{
    std::sort(players->begin(), players->end(), [](const Player &a, const Player &b) {
        const QString la = PlayerRepository::fold(a.lastName), lb = PlayerRepository::fold(b.lastName);
        return la != lb ? la < lb : PlayerRepository::fold(a.firstName) < PlayerRepository::fold(b.firstName);
    });
}

} // namespace

// Rassemble la feuille d'un match : convoqués triés (nom, prénom), contrôle des licences. ok faux si rien n'est imprimable.
QVariantMap Backend::challengeSheetCheck(int matchId)
{
    QVariantMap r;
    r.insert(QStringLiteral("ok"), false);
    r.insert(QStringLiteral("count"), 0);
    r.insert(QStringLiteral("problems"), QVariantList());
    auto refuse = [&](const QString &message) {
        r.insert(QStringLiteral("error"), message);
        setError(message);
        return r;
    };

    QString error;
    const Match m = m_matches.get(matchId, &error);
    if (m.id < 0)
        return refuse(error.isEmpty() ? tr("Match introuvable.") : error);
    QString teamName;
    for (const Team &t : m_teams.all(&error)) {
        if (t.id == m.teamId)
            teamName = t.name;
    }
    r.insert(QStringLiteral("teamName"), teamName);
    r.insert(QStringLiteral("fileName"),
             QStringLiteral("feuille-challenge-%1-%2.pdf").arg(slug(teamName), m.startAt.date().toString(kDay)));

    QHash<int, Player> byId;
    for (const Player &p : m_players.list(m.teamId, true, &error))
        byId.insert(p.id, p);
    QList<Player> players;
    for (const MatchPlayer &mp : m_matches.players(matchId, &error)) {
        if (byId.contains(mp.playerId))
            players.append(byId.value(mp.playerId));
    }
    sortByName(&players);
    r.insert(QStringLiteral("count"), players.size());
    if (players.isEmpty())
        return refuse(tr("Aucun joueur convoqué pour ce match."));
    if (players.size() > ChallengeSheet::kRows)
        return refuse(tr("La feuille a %1 lignes : %2 joueurs sont convoqués. Retirez-en %3 de la convocation.")
                          .arg(ChallengeSheet::kRows).arg(players.size()).arg(players.size() - ChallengeSheet::kRows));

    QVariantList problems;
    for (const Player &p : players) {
        if (ChallengeSheet::isValidLicence(p.licence))
            continue;
        QVariantMap e;
        e.insert(QStringLiteral("name"), p.fullName());
        e.insert(QStringLiteral("message"),
                 p.licence.isEmpty() ? tr("n° de licence manquant")
                                     : tr("n° de licence invalide (%1 chiffres attendus)").arg(ChallengeSheet::kLicenceDigits));
        problems.append(e);
    }
    if (!problems.isEmpty()) {
        r.insert(QStringLiteral("problems"), problems);
        return refuse(problems.size() > 1 ? tr("%1 joueurs n'ont pas de n° de licence valide.").arg(problems.size())
                                          : tr("1 joueur n'a pas de n° de licence valide."));
    }
    r.insert(QStringLiteral("ok"), true);
    r.insert(QStringLiteral("error"), QString());
    setError(QString());
    return r;
}

// Fichier temporaire (cache de l'application) où la feuille est créée avant d'être ouverte dans une application externe.
QString Backend::challengeSheetTempUrl(const QString &fileName)
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if (!dir.mkpath(QStringLiteral("challenge")))
        return QString();
    dir.cd(QStringLiteral("challenge"));
    QString name = QFileInfo(fileName).fileName();           // jamais de chemin fourni par l'appelant
    if (name.isEmpty() || !name.endsWith(QStringLiteral(".pdf")))
        name = QStringLiteral("feuille-challenge.pdf");
    // Les feuilles précédentes ne servent plus : on les supprime. Si l'une est verrouillée (lecteur encore ouvert),
    // on prend un nom libre plutôt que d'écraser un fichier en cours de lecture.
    for (const QFileInfo &old : dir.entryInfoList({QStringLiteral("*.pdf")}, QDir::Files))
        QFile::remove(old.absoluteFilePath());
    const QString base = name.chopped(4);
    QString candidate = name;
    for (int n = 2; QFile::exists(dir.filePath(candidate)) && n < 100; ++n)
        candidate = QStringLiteral("%1-%2.pdf").arg(base).arg(n);
    return QUrl::fromLocalFile(dir.filePath(candidate)).toString();
}

QVariantMap Backend::exportChallengeSheet(int matchId, const QString &url)
{
    QVariantMap r = challengeSheetCheck(matchId);
    if (!r.value(QStringLiteral("ok")).toBool())
        return r;
    auto fail = [&](const QString &message) {
        r.insert(QStringLiteral("ok"), false);
        r.insert(QStringLiteral("error"), message);
        setError(message);
        return r;
    };

    // Mêmes convoqués, dans le même ordre que le contrôle.
    QString error;
    const Match m = m_matches.get(matchId, &error);
    QHash<int, Player> byId;
    for (const Player &p : m_players.list(m.teamId, true, &error))
        byId.insert(p.id, p);
    QList<Player> players;
    for (const MatchPlayer &mp : m_matches.players(matchId, &error)) {
        if (byId.contains(mp.playerId))
            players.append(byId.value(mp.playerId));
    }
    sortByName(&players);
    QList<ChallengeEntry> entries;
    for (const Player &p : players)
        entries.append({p.lastName, p.firstName, PlayerRepository::normalizeLicence(p.licence)});

    QByteArray pdf;
    QBuffer buffer(&pdf);
    buffer.open(QIODevice::WriteOnly);
    QString problem;
    if (!ChallengeSheet::writePdf(&buffer, r.value(QStringLiteral("teamName")).toString(), entries, &problem))
        return fail(tr("Impossible de créer le PDF : %1").arg(problem));
    buffer.close();

    // Sur Android, une adresse content:// vers un fichier déjà existant n'accepte pas toujours le mode « tronquer » :
    // on essaie plusieurs modes d'ouverture, puis on ajuste la taille ; l'erreur indique le dernier motif connu.
    const QUrl u(url);
    const QString target = u.isLocalFile() ? u.toLocalFile() : url;
    QFile f(target);
    const QIODevice::OpenMode modes[] = {QIODevice::WriteOnly | QIODevice::Truncate,
                                         QIODevice::ReadWrite | QIODevice::Truncate,
                                         QIODevice::WriteOnly,
                                         QIODevice::ReadWrite};
    bool opened = false, truncated = false;
    for (const QIODevice::OpenMode mode : modes) {
        if (f.open(mode)) {
            opened = true;
            truncated = mode.testFlag(QIODevice::Truncate);
            break;
        }
    }
    if (!opened && u.isLocalFile() && QFile::exists(target) && QFile::remove(target)) {
        opened = f.open(QIODevice::WriteOnly | QIODevice::Truncate);      // fichier en lecture seule : on le remplace
        truncated = opened;
    }
    if (!opened)
        return fail(tr("Impossible d'écrire le fichier (%1) : %2")
                        .arg(u.scheme().isEmpty() ? QStringLiteral("local") : u.scheme(), f.errorString()));
    const bool written = f.write(pdf) == pdf.size();
    bool sized = true;
    if (written && !truncated)
        sized = f.resize(pdf.size());                                    // ancien contenu plus long : on coupe le reste
    f.close();
    if (!written)
        return fail(tr("Écriture incomplète du fichier."));
    if (!sized)
        r.insert(QStringLiteral("warning"), tr("Le fichier existait déjà : un reste de l'ancien contenu peut subsister."));
    r.insert(QStringLiteral("bytes"), pdf.size());
    setError(QString());
    return r;
}
