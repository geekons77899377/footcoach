#include "Backend.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QUrl>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

// Lieux de match (schéma 11) : liste des stades d'un club, chargée depuis Data ES (recensement des équipements
// sportifs du ministère des Sports) à la demande de l'entraîneur, dans les Paramètres. Aucun appel réseau sans cette
// action ; seul le code du département est envoyé.

namespace {

constexpr int kVenueTimeoutMs = 30000;
constexpr qint64 kVenueMaxBytes = 8LL * 1024 * 1024;

QVariantMap venueResult(bool ok, const QString &error = QString(), int id = -1)
{
    QVariantMap m;
    m.insert(QStringLiteral("ok"), ok);
    m.insert(QStringLiteral("error"), error);
    m.insert(QStringLiteral("id"), id);
    return m;
}

QVariantMap venueMap(const Venue &v)
{
    QVariantMap m;
    m.insert(QStringLiteral("id"), v.id);
    m.insert(QStringLiteral("clubId"), v.clubId);
    m.insert(QStringLiteral("name"), v.name);
    m.insert(QStringLiteral("field"), v.field);
    m.insert(QStringLiteral("address"), v.address);
    m.insert(QStringLiteral("postcode"), v.postcode);
    m.insert(QStringLiteral("city"), v.city);
    m.insert(QStringLiteral("label"), v.label());
    m.insert(QStringLiteral("source"), v.source);
    m.insert(QStringLiteral("esStatus"), v.esStatus);
    m.insert(QStringLiteral("unavailable"), v.unavailable);
    m.insert(QStringLiteral("isHome"), v.isHome);
    m.insert(QStringLiteral("selectable"), v.selectable());
    m.insert(QStringLiteral("lat"), qIsNaN(v.lat) ? QVariant() : QVariant(v.lat));
    m.insert(QStringLiteral("lon"), qIsNaN(v.lon) ? QVariant() : QVariant(v.lon));
    return m;
}

} // namespace

QVariantMap Backend::venueSummary(int clubId)
{
    Club club;
    QString error;
    if (!m_clubs.get(clubId, &club, &error)) {
        setError(error);
        return venueResult(false, error);
    }
    int total = 0, selectable = 0, unavailable = 0, removed = 0, closed = 0, manual = 0;
    const QList<Venue> all = m_venues.list(clubId, true, &error);
    for (const Venue &v : all) {
        ++total;
        if (v.selectable())
            ++selectable;
        if (v.unavailable)
            ++unavailable;
        if (v.esStatus == QLatin1String("removed"))
            ++removed;
        if (v.esStatus == QLatin1String("closed"))
            ++closed;
        if (v.source == QLatin1String("manual"))
            ++manual;
    }
    const Venue home = m_venues.home(clubId, &error);
    setError(error);
    QVariantMap r = venueResult(error.isEmpty(), error, clubId);
    r.insert(QStringLiteral("clubId"), clubId);
    r.insert(QStringLiteral("clubName"), club.name);
    r.insert(QStringLiteral("department"), club.department);
    r.insert(QStringLiteral("lastRefresh"), m_venues.lastRefresh(clubId));
    r.insert(QStringLiteral("total"), total);
    r.insert(QStringLiteral("selectable"), selectable);
    r.insert(QStringLiteral("unavailable"), unavailable);
    r.insert(QStringLiteral("removed"), removed);
    r.insert(QStringLiteral("closed"), closed);
    r.insert(QStringLiteral("manual"), manual);
    r.insert(QStringLiteral("homeVenueId"), home.id);
    r.insert(QStringLiteral("homeLabel"), home.id > 0 ? home.label() : QString());
    return r;
}

QVariantMap Backend::setClubDepartment(int clubId, const QString &department)
{
    const QString d = department.trimmed().toUpper();
    if (!d.isEmpty() && !VenueRepository::isValidDepartment(d)) {
        const QString e = tr("Numéro de département inconnu : tapez par exemple 60, 2A ou 974.");
        setError(e);
        return venueResult(false, e);
    }
    QString error;
    const bool ok = m_clubs.setDepartment(clubId, d, &error);
    setError(error);
    return venueResult(ok, error, clubId);
}

QVariantList Backend::venues(int clubId, bool includeUnselectable)
{
    QVariantList out;
    QString error;
    for (const Venue &v : m_venues.list(clubId, includeUnselectable, &error))
        out.append(venueMap(v));
    setError(error);
    return out;
}

QVariantMap Backend::saveVenue(const QVariantMap &in)
{
    Venue v;
    v.id = in.value(QStringLiteral("id"), -1).toInt();
    v.clubId = in.value(QStringLiteral("clubId"), -1).toInt();
    v.name = in.value(QStringLiteral("name")).toString();
    v.field = in.value(QStringLiteral("field")).toString();
    v.address = in.value(QStringLiteral("address")).toString();
    v.postcode = in.value(QStringLiteral("postcode")).toString();
    v.city = in.value(QStringLiteral("city")).toString();
    QString error;
    const bool ok = m_venues.saveManual(v, &error);
    setError(error);
    QVariantMap r = venueResult(ok, error, ok ? v.id : -1);
    Venue saved;
    if (ok && m_venues.get(v.id, &saved))
        r.insert(QStringLiteral("venue"), venueMap(saved));
    return r;
}

QVariantMap Backend::deleteVenue(int id)
{
    QString error;
    const bool ok = m_venues.removeManual(id, &error);
    setError(error);
    return venueResult(ok, error, id);
}

QVariantMap Backend::setVenueUnavailable(int id, bool unavailable)
{
    QString error;
    const bool ok = m_venues.setUnavailable(id, unavailable, &error);
    setError(error);
    return venueResult(ok, error, id);
}

QVariantMap Backend::setHomeVenue(int clubId, int venueId)
{
    QString error;
    const bool ok = m_venues.setHome(clubId, venueId, &error);
    setError(error);
    return venueResult(ok, error, venueId);
}

QVariantMap Backend::applyVenueData(int clubId, const QByteArray &json)
{
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isArray()) {
        const QString e = tr("Réponse de Data ES illisible : la liste n'a pas été modifiée.");
        setError(e);
        QVariantMap r = venueResult(false, e);
        r.insert(QStringLiteral("clubId"), clubId);
        return r;
    }
    VenueRepository::RefreshStats st;
    QString error;
    const bool ok = m_venues.applyDataEs(clubId, doc.array(), &st, &error);
    setError(error);
    QVariantMap r = venueResult(ok, error);
    r.insert(QStringLiteral("clubId"), clubId);
    r.insert(QStringLiteral("total"), st.total);
    r.insert(QStringLiteral("added"), st.added);
    r.insert(QStringLiteral("updated"), st.updated);
    r.insert(QStringLiteral("closed"), st.closed);
    r.insert(QStringLiteral("removed"), st.removed);
    r.insert(QStringLiteral("reopened"), st.reopened);
    return r;
}

bool Backend::refreshVenues(int clubId)
{
    if (m_venueReply) {
        setError(tr("Une mise à jour des stades est déjà en cours."));
        return false;
    }
    Club club;
    QString error;
    if (!m_clubs.get(clubId, &club, &error)) {
        setError(error);
        return false;
    }
    if (!VenueRepository::isValidDepartment(club.department)) {
        setError(tr("Indiquez d'abord le département du club."));
        return false;
    }
    if (!m_net)
        m_net = new QNetworkAccessManager(this);
    QNetworkRequest req(VenueRepository::dataEsUrl(club.department));
    req.setTransferTimeout(kVenueTimeoutMs);
    req.setRawHeader("Accept", "application/json");
    // Une seule requête : HTTP/1.1 suffit. En HTTP/2, la connexion était coupée derrière le proxy de la machine de
    // test (13/11/2026) ; HTTP/1.1 y passe. Non vérifié sur téléphone.
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    m_venueReply = m_net->get(req);
    emit venuesBusyChanged();
    QNetworkReply *reply = m_venueReply;
    connect(reply, &QNetworkReply::downloadProgress, reply, [reply](qint64 received, qint64) {
        if (received > kVenueMaxBytes)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, clubId]() {
        QVariantMap r;
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            r = venueResult(false, tr("Mise à jour annulée : la liste n'a pas été modifiée."));
            r.insert(QStringLiteral("clubId"), clubId);
            r.insert(QStringLiteral("cancelled"), true);
        } else if (reply->error() != QNetworkReply::NoError) {
            r = venueResult(false, tr("Impossible de joindre Data ES (vérifiez la connexion Internet) : "
                                      "la liste n'a pas été modifiée."));
            r.insert(QStringLiteral("clubId"), clubId);
            r.insert(QStringLiteral("detail"), reply->errorString());
        } else {
            r = applyVenueData(clubId, reply->readAll());
        }
        if (!r.value(QStringLiteral("ok")).toBool())
            setError(r.value(QStringLiteral("error")).toString());
        reply->deleteLater();
        if (m_venueReply == reply)
            m_venueReply = nullptr;
        emit venuesBusyChanged();
        emit venuesRefreshed(r);
    });
    return true;
}

void Backend::cancelVenueRefresh()
{
    if (m_venueReply)
        m_venueReply->abort();
}

QVariantMap Backend::suggestedVenue(int teamId, const QString &homeAway, const QString &opponent)
{
    const int clubId = m_clubs.clubOfTeam(teamId);
    if (clubId <= 0)
        return {};
    Venue v;
    if (homeAway == QLatin1String("home")) {
        v = m_venues.home(clubId);
    } else {
        const int id = m_venues.opponentVenue(clubId, opponent);
        if (id <= 0 || !m_venues.get(id, &v))
            return {};
    }
    if (v.id <= 0 || !v.selectable())
        return {};
    return venueMap(v);
}

QString Backend::geoUri(double lat, double lon, const QString &label, const QString &address)
{
    const auto enc = [](const QString &t) { return QString::fromLatin1(QUrl::toPercentEncoding(t.simplified())); };
    if (!qIsNaN(lat) && !qIsNaN(lon) && !(lat == 0.0 && lon == 0.0)) {
        // Coordonnées seules, sans nom : avec « q=lat,lon(nom) », Waze a cherché le nom comme du texte et proposé un
        // autre terrain à 47 km (« Terrain de Football & Tir à l'Arc » → un terrain de tir à l'arc ; essai sur
        // appareil, 13/11/2026).
        Q_UNUSED(label)
        const QString ll = QString::number(lat, 'f', 6) + QLatin1Char(',') + QString::number(lon, 'f', 6);
        return QStringLiteral("geo:") + ll + QStringLiteral("?q=") + ll;
    }
    if (address.simplified().isEmpty())
        return QString();
    return QStringLiteral("geo:0,0?q=") + enc(address);
}

QVariantMap Backend::openItinerary(int venueId, const QString &address)
{
    Venue v;
    QString uri;
    if (venueId > 0 && m_venues.get(venueId, &v))
        uri = geoUri(v.lat, v.lon, v.city.isEmpty() ? v.name : v.name + QStringLiteral(" - ") + v.city,
                     address.isEmpty() ? v.label() : address);
    else
        uri = geoUri(qQNaN(), qQNaN(), QString(), address);
    if (uri.isEmpty()) {
        const QString e = tr("Indiquez d'abord le stade ou l'adresse du match.");
        setError(e);
        return venueResult(false, e);
    }
    QVariantMap r;
#ifdef Q_OS_ANDROID
    QJniObject activite = QJniObject::callStaticObjectMethod("org/qtproject/qt/android/QtNative", "activity",
                                                             "()Landroid/app/Activity;");
    QJniObject erreur = QJniObject::callStaticObjectMethod(
        "org/footcoach/app/ShareBridge", "itineraire",
        "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", activite.object(),
        QJniObject::fromString(uri).object<jstring>(), QJniObject::fromString(tr("Itinéraire avec…")).object<jstring>());
    const QString e = erreur.isValid() ? erreur.toString() : QString();
    setError(e);
    r = venueResult(e.isEmpty(), e);
#else
    r = venueResult(false, tr("L'itinéraire s'ouvre sur le téléphone (Waze, Google Maps…)."));
#endif
    r.insert(QStringLiteral("uri"), uri);
    return r;
}
