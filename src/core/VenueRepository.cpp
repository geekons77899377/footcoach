#include "VenueRepository.h"

#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrlQuery>
#include <QVariant>

namespace {

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

QVariant nullIfEmpty(const QString &s)
{
    return s.trimmed().isEmpty() ? QVariant(QMetaType::fromType<QString>()) : QVariant(s.trimmed());
}

QVariant nullIfNaN(double v)
{
    return qIsNaN(v) ? QVariant(QMetaType::fromType<double>()) : QVariant(v);
}

const QString kSelect = QStringLiteral(
    "SELECT id, club_id, name, field, address, postcode, city, lat, lon, source, source_id, es_status, unavailable, "
    "is_home FROM venue");

Venue readVenue(const QSqlQuery &q)
{
    Venue v;
    v.id = q.value(0).toInt();
    v.clubId = q.value(1).toInt();
    v.name = q.value(2).toString();
    v.field = q.value(3).toString();
    v.address = q.value(4).toString();
    v.postcode = q.value(5).toString();
    v.city = q.value(6).toString();
    v.lat = q.value(7).isNull() ? qQNaN() : q.value(7).toDouble();
    v.lon = q.value(8).isNull() ? qQNaN() : q.value(8).toDouble();
    v.source = q.value(9).toString();
    v.sourceId = q.value(10).toString();
    v.esStatus = q.value(11).toString();
    v.unavailable = q.value(12).toInt() != 0;
    v.isHome = q.value(13).toInt() != 0;
    return v;
}

// Data ES écrit souvent les noms en majuscules (« COMPLEXE SPORTIF ») : on les remet en casse de titre, sans toucher
// aux noms déjà en casse mixte.
QString tidy(const QString &raw)
{
    const QString s = raw.simplified();
    if (s.isEmpty() || s != s.toUpper())
        return s;
    static const QSet<QString> small = {QStringLiteral("de"), QStringLiteral("du"), QStringLiteral("des"),
                                        QStringLiteral("la"), QStringLiteral("le"), QStringLiteral("les"),
                                        QStringLiteral("et"), QStringLiteral("en"), QStringLiteral("sur"),
                                        QStringLiteral("aux"), QStringLiteral("au")};
    QString out;
    bool first = true;
    for (const QString &word : s.toLower().split(QLatin1Char(' '))) {
        if (!first)
            out += QLatin1Char(' ');
        if (!first && small.contains(word)) {
            out += word;
        } else {
            // Majuscule après un début de mot, un trait d'union ou une apostrophe (« Saint-Jean », « l'Erable »).
            QString w = word;
            for (int i = 0; i < w.size(); ++i)
                if (i == 0 || w.at(i - 1) == QLatin1Char('-') || w.at(i - 1) == QLatin1Char('\''))
                    w[i] = w.at(i).toUpper();
            out += w;
        }
        first = false;
    }
    return out;
}

} // namespace

QString Venue::label() const
{
    QString head = name.trimmed();
    const QString f = field.trimmed();
    // Le nom du terrain n'apporte rien quand il est générique (« Terrain de football ») ou répète le nom.
    if (!f.isEmpty() && f.compare(QLatin1String("Terrain de football"), Qt::CaseInsensitive) != 0
        && f.compare(head, Qt::CaseInsensitive) != 0)
        head += QStringLiteral(" (%1)").arg(f);
    QStringList parts;
    if (!head.isEmpty())
        parts << head;
    if (!address.trimmed().isEmpty())
        parts << address.trimmed();
    const QString town = (postcode.trimmed() + QLatin1Char(' ') + city.trimmed()).trimmed();
    if (!town.isEmpty())
        parts << town;
    return parts.join(QStringLiteral(", "));
}

QList<Venue> VenueRepository::list(int clubId, bool includeUnselectable, QString *error) const
{
    QList<Venue> out;
    QSqlQuery q(m_db);
    q.prepare(kSelect + QStringLiteral(" WHERE club_id = ?")
              + (includeUnselectable ? QString() : QStringLiteral(" AND unavailable = 0 AND es_status = 'ok'"))
              + QStringLiteral(" ORDER BY is_home DESC, city COLLATE NOCASE, name COLLATE NOCASE, field COLLATE NOCASE, id"));
    q.addBindValue(clubId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return out;
    }
    while (q.next())
        out.append(readVenue(q));
    return out;
}

bool VenueRepository::get(int id, Venue *out, QString *error) const
{
    QSqlQuery q(m_db);
    q.prepare(kSelect + QStringLiteral(" WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (!q.next()) {
        setError(error, QStringLiteral("Lieu introuvable."));
        return false;
    }
    *out = readVenue(q);
    return true;
}

Venue VenueRepository::home(int clubId, QString *error) const
{
    QSqlQuery q(m_db);
    q.prepare(kSelect + QStringLiteral(" WHERE club_id = ? AND is_home = 1"));
    q.addBindValue(clubId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return Venue();
    }
    return q.next() ? readVenue(q) : Venue();
}

bool VenueRepository::saveManual(Venue &v, QString *error)
{
    if (v.name.trimmed().isEmpty()) {
        setError(error, QStringLiteral("Le nom du lieu est obligatoire."));
        return false;
    }
    QSqlQuery q(m_db);
    if (v.id > 0) {
        Venue old;
        if (!get(v.id, &old, error))
            return false;
        if (old.source != QLatin1String("manual")) {
            setError(error, QStringLiteral("Un terrain de la liste officielle ne se modifie pas : marquez-le indisponible "
                                           "ou ajoutez un lieu à la main."));
            return false;
        }
        q.prepare(QStringLiteral("UPDATE venue SET name = ?, field = ?, address = ?, postcode = ?, city = ?, lat = ?, "
                                 "lon = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?"));
    } else {
        q.prepare(QStringLiteral("INSERT INTO venue(name, field, address, postcode, city, lat, lon, club_id, source) "
                                 "VALUES(?, ?, ?, ?, ?, ?, ?, ?, 'manual')"));
    }
    q.addBindValue(v.name.trimmed());
    q.addBindValue(nullIfEmpty(v.field));
    q.addBindValue(nullIfEmpty(v.address));
    q.addBindValue(nullIfEmpty(v.postcode));
    q.addBindValue(nullIfEmpty(v.city));
    q.addBindValue(nullIfNaN(v.lat));
    q.addBindValue(nullIfNaN(v.lon));
    q.addBindValue(v.id > 0 ? v.id : v.clubId);
    if (!q.exec()) {
        setError(error, q.lastError().text().contains(QLatin1String("FOREIGN KEY"), Qt::CaseInsensitive)
                            ? QStringLiteral("Club introuvable.")
                            : q.lastError().text());
        return false;
    }
    if (v.id <= 0) {
        v.id = q.lastInsertId().toInt();
        v.source = QStringLiteral("manual");
    }
    return true;
}

bool VenueRepository::removeManual(int id, QString *error)
{
    Venue v;
    if (!get(id, &v, error))
        return false;
    if (v.source != QLatin1String("manual")) {
        setError(error, QStringLiteral("Un terrain de la liste officielle ne se supprime pas : marquez-le indisponible."));
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM venue WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

bool VenueRepository::setUnavailable(int id, bool unavailable, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE venue SET unavailable = ? WHERE id = ?"));
    q.addBindValue(unavailable ? 1 : 0);
    q.addBindValue(id);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    if (q.numRowsAffected() == 0) {
        setError(error, QStringLiteral("Lieu introuvable."));
        return false;
    }
    return true;
}

bool VenueRepository::setHome(int clubId, int venueId, QString *error)
{
    if (venueId > 0) {
        Venue v;
        if (!get(venueId, &v, error))
            return false;
        if (v.clubId != clubId) {
            setError(error, QStringLiteral("Ce lieu n'appartient pas au club."));
            return false;
        }
    }
    if (!m_db.transaction()) {
        setError(error, m_db.lastError().text());
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE venue SET is_home = 0 WHERE club_id = ? AND is_home = 1"));
    q.addBindValue(clubId);
    bool ok = q.exec();
    if (ok && venueId > 0) {
        QSqlQuery s(m_db);
        s.prepare(QStringLiteral("UPDATE venue SET is_home = 1 WHERE id = ?"));
        s.addBindValue(venueId);
        ok = s.exec();
        if (!ok)
            q = s;
    }
    if (!ok || !m_db.commit()) {
        const QString e = ok ? m_db.lastError().text() : q.lastError().text();
        m_db.rollback();
        setError(error, e);
        return false;
    }
    return true;
}

bool VenueRepository::applyDataEs(int clubId, const QJsonArray &records, RefreshStats *stats, QString *error)
{
    RefreshStats st;
    struct Item { QString id, name, field, address, postcode, city; double lat, lon; bool closed; };
    QList<Item> items;
    QSet<QString> seen;
    for (const QJsonValue &rv : records) {
        if (!rv.isObject()) {
            setError(error, QStringLiteral("Réponse de Data ES illisible."));
            return false;
        }
        const QJsonObject o = rv.toObject();
        Item it;
        it.id = o.value(QStringLiteral("equip_numero")).toString().trimmed();
        if (it.id.isEmpty() || seen.contains(it.id))
            continue;
        seen.insert(it.id);
        it.name = tidy(o.value(QStringLiteral("inst_nom")).toString());
        it.field = tidy(o.value(QStringLiteral("equip_nom")).toString());
        if (it.name.isEmpty())
            it.name = it.field.isEmpty() ? QStringLiteral("Terrain de football") : it.field;
        it.address = tidy(o.value(QStringLiteral("inst_adresse")).toString());
        it.postcode = o.value(QStringLiteral("inst_cp")).toString().trimmed();
        it.city = o.value(QStringLiteral("new_name")).toString().simplified();
        const QJsonObject c = o.value(QStringLiteral("equip_coordonnees")).toObject();
        it.lat = c.contains(QStringLiteral("lat")) ? c.value(QStringLiteral("lat")).toDouble() : qQNaN();
        it.lon = c.contains(QStringLiteral("lon")) ? c.value(QStringLiteral("lon")).toDouble() : qQNaN();
        it.closed = o.value(QStringLiteral("inst_hs_bool")).toString().compare(QLatin1String("Oui"), Qt::CaseInsensitive) == 0;
        items.append(it);
    }
    if (items.isEmpty()) {
        setError(error, QStringLiteral("Data ES n'a renvoyé aucun terrain de football pour ce département : "
                                       "la liste n'a pas été modifiée."));
        return false;
    }

    // Terrains Data ES déjà connus du club : identifiant -> (id, état).
    QHash<QString, QPair<int, QString>> known;
    {
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral("SELECT source_id, id, es_status FROM venue WHERE club_id = ? AND source = 'data_es'"));
        q.addBindValue(clubId);
        if (!q.exec()) {
            setError(error, q.lastError().text());
            return false;
        }
        while (q.next())
            known.insert(q.value(0).toString(), qMakePair(q.value(1).toInt(), q.value(2).toString()));
    }
    {
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral("SELECT COUNT(*) FROM club WHERE id = ?"));
        q.addBindValue(clubId);
        if (!q.exec() || !q.next() || q.value(0).toInt() == 0) {
            setError(error, QStringLiteral("Club introuvable."));
            return false;
        }
    }

    if (!m_db.transaction()) {
        setError(error, m_db.lastError().text());
        return false;
    }
    bool ok = true;
    QString failure;
    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    for (const Item &it : items) {
        const QString status = it.closed ? QStringLiteral("closed") : QStringLiteral("ok");
        QSqlQuery q(m_db);
        if (known.contains(it.id)) {
            const QString before = known.value(it.id).second;
            q.prepare(QStringLiteral("UPDATE venue SET name = ?, field = ?, address = ?, postcode = ?, city = ?, lat = ?, "
                                     "lon = ?, es_status = ?, updated_at = ? WHERE id = ?"));
            if (it.closed)
                ++st.closed;
            else if (before != QLatin1String("ok"))
                ++st.reopened;
            else
                ++st.updated;
        } else {
            q.prepare(QStringLiteral("INSERT INTO venue(name, field, address, postcode, city, lat, lon, es_status, "
                                     "updated_at, club_id, source, source_id) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                                     "'data_es', ?)"));
            if (it.closed)
                ++st.closed;
            else
                ++st.added;
        }
        q.addBindValue(it.name);
        q.addBindValue(nullIfEmpty(it.field));
        q.addBindValue(nullIfEmpty(it.address));
        q.addBindValue(nullIfEmpty(it.postcode));
        q.addBindValue(nullIfEmpty(it.city));
        q.addBindValue(nullIfNaN(it.lat));
        q.addBindValue(nullIfNaN(it.lon));
        q.addBindValue(status);
        q.addBindValue(now);
        if (known.contains(it.id)) {
            q.addBindValue(known.value(it.id).first);
        } else {
            q.addBindValue(clubId);
            q.addBindValue(it.id);
        }
        if (!q.exec()) {
            ok = false;
            failure = q.lastError().text();
            break;
        }
    }
    // Terrains connus absents de la réponse : retirés (jamais supprimés, les matchs peuvent y renvoyer).
    for (auto k = known.cbegin(); ok && k != known.cend(); ++k) {
        if (seen.contains(k.key()) || k.value().second == QLatin1String("removed"))
            continue;
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral("UPDATE venue SET es_status = 'removed', updated_at = ? WHERE id = ?"));
        q.addBindValue(now);
        q.addBindValue(k.value().first);
        if (!q.exec()) {
            ok = false;
            failure = q.lastError().text();
        }
        ++st.removed;
    }
    if (ok)
        ok = m_db.commit();
    if (!ok) {
        const QString e = failure.isEmpty() ? m_db.lastError().text() : failure;
        m_db.rollback();
        setError(error, e);
        return false;
    }
    st.total = items.size();
    if (stats)
        *stats = st;
    return true;
}

QString VenueRepository::lastRefresh(int clubId) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT MAX(updated_at) FROM venue WHERE club_id = ? AND source = 'data_es'"));
    q.addBindValue(clubId);
    if (!q.exec() || !q.next() || q.value(0).isNull())
        return QString();
    return q.value(0).toString().left(16);
}

int VenueRepository::opponentVenue(int clubId, const QString &opponent) const
{
    const QString key = opponentKey(opponent);
    if (key.isEmpty())
        return -1;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT venue_id FROM opponent_venue WHERE club_id = ? AND opponent_key = ?"));
    q.addBindValue(clubId);
    q.addBindValue(key);
    return (q.exec() && q.next()) ? q.value(0).toInt() : -1;
}

bool VenueRepository::rememberOpponentVenue(int clubId, const QString &opponent, int venueId, QString *error)
{
    const QString key = opponentKey(opponent);
    if (key.isEmpty()) {
        setError(error, QStringLiteral("L'adversaire est obligatoire."));
        return false;
    }
    Venue v;
    if (!get(venueId, &v, error))
        return false;
    if (v.clubId != clubId) {
        setError(error, QStringLiteral("Ce lieu n'appartient pas au club."));
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO opponent_venue(club_id, opponent_key, opponent, venue_id) VALUES(?, ?, ?, ?) "
                             "ON CONFLICT(club_id, opponent_key) DO UPDATE SET opponent = excluded.opponent, "
                             "venue_id = excluded.venue_id"));
    q.addBindValue(clubId);
    q.addBindValue(key);
    q.addBindValue(opponent.simplified());
    q.addBindValue(venueId);
    if (!q.exec()) {
        setError(error, q.lastError().text());
        return false;
    }
    return true;
}

QString VenueRepository::opponentKey(const QString &opponent)
{
    QString out;
    for (const QChar ch : opponent.normalized(QString::NormalizationForm_D)) {
        if (ch.category() == QChar::Mark_NonSpacing)
            continue;
        out += ch.isLetterOrNumber() ? ch.toLower() : QLatin1Char(' ');
    }
    // « U.S. » devient « u s » : on recolle les lettres isolées qui se suivent (« us »).
    const QStringList words = out.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QStringList merged;
    bool lastIsInitials = false;
    for (const QString &w : words) {
        const bool initial = w.size() == 1 && w.at(0).isLetter();
        if (initial && lastIsInitials)
            merged.last() += w;
        else
            merged << w;
        lastIsInitials = initial || (lastIsInitials && initial);
    }
    return merged.join(QLatin1Char(' '));
}

bool VenueRepository::isValidDepartment(const QString &department)
{
    static const QRegularExpression re(QStringLiteral("^(0[1-9]|1[0-9]|[3-8][0-9]|9[0-5]|2[1-9]|2A|2B|97[1-6])$"));
    return re.match(department.trimmed().toUpper()).hasMatch();
}

QUrl VenueRepository::dataEsUrl(const QString &department)
{
    QUrl url(QStringLiteral("https://equipements.sports.gouv.fr/api/explore/v2.1/catalog/datasets/data-es/exports/json"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("select"),
                       QStringLiteral("equip_numero,inst_nom,equip_nom,inst_adresse,inst_cp,new_name,equip_coordonnees,inst_hs_bool"));
    query.addQueryItem(QStringLiteral("where"),
                       QStringLiteral("dep_code='%1' and equip_type_name='Terrain de football'")
                           .arg(department.trimmed().toUpper()));
    url.setQuery(query);
    return url;
}
