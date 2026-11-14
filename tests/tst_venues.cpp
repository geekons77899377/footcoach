#include "app/Backend.h"
#include "core/ClubRepository.h"
#include "core/Database.h"
#include "core/TeamRepository.h"
#include "core/VenueRepository.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxyFactory>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUrlQuery>
#include <QtTest>

#ifndef FOOTCOACH_FIXTURES_DIR
#error "FOOTCOACH_FIXTURES_DIR doit être défini par CMake"
#endif

// Lieux de match (schéma 11) : liste des stades d'un club chargée depuis Data ES, stade du club, stade retenu pour
// chaque adversaire, lien avec les matchs, export et import. Le réseau n'est pas utilisé : la réponse de Data ES vient
// d'un extrait réel (tests/fixtures/data-es-oise-extrait.json).
class VenuesTest : public QObject
{
    Q_OBJECT

    Database m_db;
    QTemporaryDir m_dir;
    int m_club = -1;
    int m_team = -1;

    int count(const QString &sql)
    {
        QSqlQuery q(m_db.handle());
        return q.exec(sql) && q.next() ? q.value(0).toInt() : -1;
    }

    static QByteArray fixture()
    {
        QFile f(QStringLiteral(FOOTCOACH_FIXTURES_DIR "/data-es-oise-extrait.json"));
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    }

    // L'extrait, modifié : sans le terrain « id », ou avec ce terrain hors service.
    static QByteArray fixtureWithout(const QString &id)
    {
        QJsonArray out;
        for (const QJsonValue &v : QJsonDocument::fromJson(fixture()).array())
            if (v.toObject().value("equip_numero").toString() != id)
                out.append(v);
        return QJsonDocument(out).toJson();
    }
    static QByteArray fixtureClosed(const QString &id)
    {
        QJsonArray out;
        for (const QJsonValue &v : QJsonDocument::fromJson(fixture()).array()) {
            QJsonObject o = v.toObject();
            if (o.value("equip_numero").toString() == id)
                o["inst_hs_bool"] = "Oui";
            out.append(o);
        }
        return QJsonDocument(out).toJson();
    }

    int venueIdOf(const QString &sourceId)
    {
        QSqlQuery q(m_db.handle());
        q.prepare(QStringLiteral("SELECT id FROM venue WHERE source_id = ?"));
        q.addBindValue(sourceId);
        return q.exec() && q.next() ? q.value(0).toInt() : -1;
    }

private slots:
    void init()
    {
        QString error;
        QVERIFY2(m_db.open(m_dir.filePath(QStringLiteral("v.db")), &error), qPrintable(error));
        QSqlQuery q(m_db.handle());
        for (const QString &t : {QStringLiteral("opponent_venue"), QStringLiteral("venue"), QStringLiteral("team")})
            QVERIFY(q.exec(QStringLiteral("DELETE FROM %1").arg(t)));
        ClubRepository clubs(m_db.handle());
        const QList<Club> all = clubs.all();
        QVERIFY(!all.isEmpty());
        m_club = all.first().id;
        TeamRepository teams(m_db.handle());
        Team t;
        t.name = QStringLiteral("U11");
        t.clubId = m_club;
        QVERIFY(teams.insert(t));
        m_team = t.id;
    }

    void cleanup() { m_db.close(); QFile::remove(m_dir.filePath(QStringLiteral("v.db"))); }

    void fixtureIsReadable()
    {
        QCOMPARE(QJsonDocument::fromJson(fixture()).array().size(), 6);
    }

    // ----- Département et adresse de l'appel -----
    void departmentIsCheckedAndStored()
    {
        Backend b(m_db.handle());
        for (const QString &bad : {"20", "00", "96", "99", "2C", "977", "6", "abc"})
            QVERIFY2(!b.setClubDepartment(m_club, bad).value("ok").toBool(), qPrintable(bad));
        for (const QString &good : {"01", "60", "2A", "2b", "95", "971", "976"})
            QVERIFY2(b.setClubDepartment(m_club, good).value("ok").toBool(), qPrintable(good));
        QVERIFY(b.setClubDepartment(m_club, " 60 ").value("ok").toBool());
        QCOMPARE(b.venueSummary(m_club).value("department").toString(), QStringLiteral("60"));
        // Renommer le club ne change pas son département.
        QVERIFY(b.saveClub({{"id", m_club}, {"name", "ES Lagny"}}).value("ok").toBool());
        QCOMPARE(b.venueSummary(m_club).value("department").toString(), QStringLiteral("60"));
        QVERIFY(b.setClubDepartment(m_club, "").value("ok").toBool());
        QVERIFY(b.venueSummary(m_club).value("department").toString().isEmpty());
        QVERIFY(!b.refreshVenues(m_club));                       // sans département : pas d'appel
        QVERIFY(b.lastError().contains(QStringLiteral("département")));
    }

    void dataEsUrlAsksOnlyFootballFieldsOfTheDepartment()
    {
        const QUrl u = VenueRepository::dataEsUrl(QStringLiteral("2a"));
        QCOMPARE(u.host(), QStringLiteral("equipements.sports.gouv.fr"));
        QVERIFY(u.path().endsWith(QStringLiteral("/datasets/data-es/exports/json")));
        const QUrlQuery q(u);
        QCOMPARE(q.queryItemValue("where", QUrl::FullyDecoded),
                 QStringLiteral("dep_code='2A' and equip_type_name='Terrain de football'"));
        QVERIFY(q.queryItemValue("select").contains(QStringLiteral("equip_numero")));
        QVERIFY(q.queryItemValue("select").contains(QStringLiteral("inst_hs_bool")));
    }

    // ----- Première mise à jour -----
    void firstRefreshAddsAllFields()
    {
        Backend b(m_db.handle());
        const QVariantMap r = b.applyVenueData(m_club, fixture());
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(r.value("total").toInt(), 6);
        QCOMPARE(r.value("added").toInt(), 6);
        QCOMPARE(r.value("removed").toInt(), 0);
        const QVariantList list = b.venues(m_club, false);
        QCOMPARE(list.size(), 6);
        // Noms en majuscules remis en casse de titre, commune toujours dans le texte du lieu.
        const int lagny = venueIdOf("E001I603410001");
        QVERIFY(lagny > 0);
        QVariantMap v;
        for (const QVariant &x : list)
            if (x.toMap().value("id").toInt() == lagny)
                v = x.toMap();
        QCOMPARE(v.value("name").toString(), QStringLiteral("Complexe Sportif"));
        QCOMPARE(v.value("label").toString(), QStringLiteral("Complexe Sportif, Rue de Mareuil, 60330 Lagny-le-Sec"));
        QCOMPARE(v.value("source").toString(), QStringLiteral("data_es"));
        QVERIFY(v.value("selectable").toBool());
        QVERIFY(qAbs(v.value("lat").toDouble() - 49.08513) < 1e-6);
        // Un nom de terrain utile est gardé dans le texte, pas le générique « Terrain de football ».
        Venue honneur;
        QVERIFY(VenueRepository(m_db.handle()).get(venueIdOf("E001I604460005"), &honneur));
        QCOMPARE(honneur.label(), QStringLiteral("Stade Chapelle des Marais (Terrain d'honneur de football), Route de Crépy, "
                                                 "60440 Nanteuil-le-Haudouin"));
        QVERIFY(!b.venueSummary(m_club).value("lastRefresh").toString().isEmpty());
    }

    // ----- Mise à jour : terrains retirés, hors service, revenus ; rien n'est supprimé -----
    void refreshMarksMissingAndClosedFieldsWithoutDeletingThem()
    {
        Backend b(m_db.handle());
        QVERIFY(b.applyVenueData(m_club, fixture()).value("ok").toBool());
        const int silly = venueIdOf("E001I606190003");
        const int plessis = venueIdOf("E004I605000003");
        // Un match à Silly-le-Long garde son lieu quand le terrain disparaît de Data ES.
        const QVariantMap m = b.saveMatch({{"teamId", m_team}, {"date", "2026-11-21"}, {"time", "14:00"},
                                           {"opponent", "FC Silly"}, {"homeAway", "away"}, {"venueId", silly}});
        QVERIFY2(m.value("ok").toBool(), qPrintable(m.value("error").toString()));

        QVariantMap r = b.applyVenueData(m_club, fixtureWithout("E001I606190003"));
        QVERIFY(r.value("ok").toBool());
        QCOMPARE(r.value("removed").toInt(), 1);
        QCOMPARE(r.value("updated").toInt(), 5);
        QCOMPARE(count("SELECT COUNT(*) FROM venue"), 6);                       // rien n'est supprimé
        QCOMPARE(b.venues(m_club, false).size(), 5);                            // plus proposé
        QCOMPARE(b.venues(m_club, true).size(), 6);
        QCOMPARE(b.match(m.value("id").toInt()).value("match").toMap().value("venueId").toInt(), silly);
        QVERIFY(b.match(m.value("id").toInt()).value("match").toMap().value("location").toString().contains("Silly-le-Long"));
        QCOMPARE(b.venueSummary(m_club).value("removed").toInt(), 1);
        // L'adversaire n'est plus proposé sur un terrain retiré.
        QVERIFY(b.suggestedVenue(m_team, "away", "FC Silly").isEmpty());

        r = b.applyVenueData(m_club, fixtureClosed("E004I605000003"));
        QVERIFY(r.value("ok").toBool());
        QCOMPARE(r.value("closed").toInt(), 1);
        QCOMPARE(r.value("reopened").toInt(), 1);                               // Silly-le-Long revient
        QVariantMap pl;
        for (const QVariant &x : b.venues(m_club, true))
            if (x.toMap().value("id").toInt() == plessis)
                pl = x.toMap();
        QCOMPARE(pl.value("esStatus").toString(), QStringLiteral("closed"));
        QVERIFY(!pl.value("selectable").toBool());
        QCOMPARE(b.suggestedVenue(m_team, "away", "FC Silly").value("id").toInt(), silly);
    }

    void emptyOrUnreadableAnswerChangesNothing()
    {
        Backend b(m_db.handle());
        QVERIFY(b.applyVenueData(m_club, fixture()).value("ok").toBool());
        const int before = count("SELECT COUNT(*) FROM venue WHERE es_status = 'ok'");
        QVariantMap r = b.applyVenueData(m_club, "[]");
        QVERIFY(!r.value("ok").toBool());
        QVERIFY(r.value("error").toString().contains(QStringLiteral("n'a pas été modifiée")));
        r = b.applyVenueData(m_club, "<html>erreur</html>");
        QVERIFY(!r.value("ok").toBool());
        r = b.applyVenueData(m_club, R"({"total_count": 681, "results": []})");    // format de /records, pas d'/exports
        QVERIFY(!r.value("ok").toBool());
        QCOMPARE(count("SELECT COUNT(*) FROM venue WHERE es_status = 'ok'"), before);
    }

    // ----- Choix de l'entraîneur -----
    void coachCanMarkAFieldUnavailableAndSetTheHomeField()
    {
        Backend b(m_db.handle());
        QVERIFY(b.applyVenueData(m_club, fixture()).value("ok").toBool());
        const int lagny = venueIdOf("E001I603410001");
        const int silly = venueIdOf("E001I606190003");
        QVERIFY(b.setVenueUnavailable(silly, true).value("ok").toBool());
        QCOMPARE(b.venues(m_club, false).size(), 5);
        // Une mise à jour ne revient pas sur le choix de l'entraîneur.
        QVERIFY(b.applyVenueData(m_club, fixture()).value("ok").toBool());
        QCOMPARE(b.venues(m_club, false).size(), 5);
        QVERIFY(b.setVenueUnavailable(silly, false).value("ok").toBool());
        QCOMPARE(b.venues(m_club, false).size(), 6);

        QVERIFY(b.setHomeVenue(m_club, lagny).value("ok").toBool());
        QCOMPARE(b.venueSummary(m_club).value("homeVenueId").toInt(), lagny);
        QCOMPARE(b.suggestedVenue(m_team, "home", "").value("id").toInt(), lagny);
        QCOMPARE(b.venues(m_club, false).first().toMap().value("id").toInt(), lagny);   // stade du club en tête
        QVERIFY(b.setHomeVenue(m_club, silly).value("ok").toBool());                    // un seul stade du club
        QCOMPARE(count("SELECT COUNT(*) FROM venue WHERE is_home = 1"), 1);
        QVERIFY(b.setHomeVenue(m_club, -1).value("ok").toBool());
        QVERIFY(b.suggestedVenue(m_team, "home", "").isEmpty());
        // Stade du club indisponible : plus proposé à domicile.
        QVERIFY(b.setHomeVenue(m_club, lagny).value("ok").toBool());
        QVERIFY(b.setVenueUnavailable(lagny, true).value("ok").toBool());
        QVERIFY(b.suggestedVenue(m_team, "home", "").isEmpty());
    }

    void manualVenuesCanBeEditedButDataEsOnesCannot()
    {
        Backend b(m_db.handle());
        QVERIFY(b.applyVenueData(m_club, fixture()).value("ok").toBool());
        QVERIFY(!b.saveVenue({{"clubId", m_club}, {"name", "  "}}).value("ok").toBool());
        const QVariantMap r = b.saveVenue({{"clubId", m_club}, {"name", "Gymnase Lucien"}, {"address", "2 rue des Écoles"},
                                           {"postcode", "60330"}, {"city", "Lagny-le-Sec"}});
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        const int manual = r.value("id").toInt();
        QCOMPARE(r.value("venue").toMap().value("label").toString(),
                 QStringLiteral("Gymnase Lucien, 2 rue des Écoles, 60330 Lagny-le-Sec"));
        QVERIFY(b.saveVenue({{"id", manual}, {"clubId", m_club}, {"name", "Gymnase Lucien Dupont"}}).value("ok").toBool());
        // Une mise à jour Data ES ne touche pas les lieux saisis à la main.
        QVERIFY(b.applyVenueData(m_club, fixtureWithout("E001I603410001")).value("ok").toBool());
        Venue v;
        QVERIFY(VenueRepository(m_db.handle()).get(manual, &v));
        QCOMPARE(v.name, QStringLiteral("Gymnase Lucien Dupont"));
        QCOMPARE(v.esStatus, QStringLiteral("ok"));
        const int lagny = venueIdOf("E001I603410001");
        QVERIFY(!b.saveVenue({{"id", lagny}, {"clubId", m_club}, {"name", "Autre"}}).value("ok").toBool());
        QVERIFY(!b.deleteVenue(lagny).value("ok").toBool());
        // Supprimer un lieu à la main : le match garde le texte, sans lien.
        const QVariantMap m = b.saveMatch({{"teamId", m_team}, {"date", "2026-11-22"}, {"time", "10:00"},
                                           {"opponent", "AS Test"}, {"venueId", manual}});
        QVERIFY(m.value("ok").toBool());
        QVERIFY(b.deleteVenue(manual).value("ok").toBool());
        const QVariantMap mm = b.match(m.value("id").toInt()).value("match").toMap();
        QCOMPARE(mm.value("venueId").toInt(), -1);
        QVERIFY(mm.value("location").toString().startsWith(QStringLiteral("Gymnase Lucien Dupont")));
    }

    // ----- Matchs -----
    void awayMatchRemembersTheOpponentField()
    {
        Backend b(m_db.handle());
        QVERIFY(b.applyVenueData(m_club, fixture()).value("ok").toBool());
        const int nanteuil = venueIdOf("E001I604460005");
        QVERIFY(b.suggestedVenue(m_team, "away", "US Nanteuil").isEmpty());
        const QVariantMap m = b.saveMatch({{"teamId", m_team}, {"date", "2026-11-28"}, {"time", "14:00"},
                                           {"opponent", "US Nanteuil"}, {"homeAway", "away"}, {"venueId", nanteuil}});
        QVERIFY2(m.value("ok").toBool(), qPrintable(m.value("error").toString()));
        // Lieu vide envoyé : l'adresse du stade est recopiée.
        QVERIFY(b.match(m.value("id").toInt()).value("match").toMap().value("location").toString()
                    .startsWith(QStringLiteral("Stade Chapelle des Marais")));
        // Même adversaire écrit autrement : même stade proposé.
        for (const QString &name : {"US Nanteuil", "U.S. NANTEUIL", "  us   nanteuil "})
            QCOMPARE(b.suggestedVenue(m_team, "away", name).value("id").toInt(), nanteuil);
        QVERIFY(b.suggestedVenue(m_team, "away", "US Nanteuil B").isEmpty());
        // À domicile, rien n'est retenu pour l'adversaire.
        const int lagny = venueIdOf("E001I603410001");
        QVERIFY(b.saveMatch({{"teamId", m_team}, {"date", "2026-12-05"}, {"time", "14:00"}, {"opponent", "FC Ève"},
                             {"homeAway", "home"}, {"venueId", lagny}}).value("ok").toBool());
        QVERIFY(b.suggestedVenue(m_team, "away", "FC Ève").isEmpty());
        // Nouveau lieu pour cet adversaire : il remplace l'ancien.
        const int annexe = venueIdOf("E002I604460005");
        QVERIFY(b.saveMatch({{"id", m.value("id")}, {"teamId", m_team}, {"date", "2026-11-28"}, {"time", "14:00"},
                             {"opponent", "US Nanteuil"}, {"homeAway", "away"}, {"venueId", annexe},
                             {"location", ""}}).value("ok").toBool());
        QCOMPARE(b.suggestedVenue(m_team, "away", "us nanteuil").value("id").toInt(), annexe);
        QCOMPARE(count("SELECT COUNT(*) FROM opponent_venue"), 1);
    }

    void venueOfAnotherClubIsRefused()
    {
        Backend b(m_db.handle());
        const int other = b.saveClub({{"name", "Autre club"}}).value("id").toInt();
        QVERIFY(other > 0);
        const int v = b.saveVenue({{"clubId", other}, {"name", "Stade de l'autre club"}}).value("id").toInt();
        QVERIFY(v > 0);
        const QVariantMap m = b.saveMatch({{"teamId", m_team}, {"date", "2026-11-28"}, {"time", "14:00"},
                                           {"opponent", "X"}, {"venueId", v}});
        QVERIFY(!m.value("ok").toBool());
        QVERIFY(m.value("error").toString().contains(QStringLiteral("club")));
        QCOMPARE(count("SELECT COUNT(*) FROM football_match"), 0);
        QVERIFY(!b.setHomeVenue(m_club, v).value("ok").toBool());
        // Supprimer le club supprime ses lieux.
        QVERIFY(b.deleteClub(other).value("ok").toBool());
        QCOMPARE(count(QStringLiteral("SELECT COUNT(*) FROM venue WHERE id = %1").arg(v)), 0);
    }

    void matchWithoutVenueStillWorks()
    {
        Backend b(m_db.handle());
        const QVariantMap m = b.saveMatch({{"teamId", m_team}, {"date", "2026-11-28"}, {"time", "14:00"},
                                           {"opponent", "X"}, {"location", "Stade inconnu, Paris"}});
        QVERIFY(m.value("ok").toBool());
        const QVariantMap mm = b.match(m.value("id").toInt()).value("match").toMap();
        QCOMPARE(mm.value("venueId").toInt(), -1);
        QCOMPARE(mm.value("location").toString(), QStringLiteral("Stade inconnu, Paris"));
    }

    // ----- Export et import -----
    void venuesTravelWithTheDataFile()
    {
        Backend b(m_db.handle());
        QVERIFY(b.setClubDepartment(m_club, "60").value("ok").toBool());
        QVERIFY(b.applyVenueData(m_club, fixture()).value("ok").toBool());
        const int lagny = venueIdOf("E001I603410001");
        const int nanteuil = venueIdOf("E001I604460005");
        QVERIFY(b.setHomeVenue(m_club, lagny).value("ok").toBool());
        QVERIFY(b.setVenueUnavailable(venueIdOf("E001I606190003"), true).value("ok").toBool());
        const int match = b.saveMatch({{"teamId", m_team}, {"date", "2026-11-28"}, {"time", "14:00"},
                                       {"opponent", "US Nanteuil"}, {"homeAway", "away"}, {"venueId", nanteuil}})
                              .value("id").toInt();
        QVERIFY(match > 0);
        const QString file = m_dir.filePath(QStringLiteral("lieux.footcoach"));
        QVERIFY(b.exportDataFile(file).value("ok").toBool());

        QVERIFY(b.resetAllData().value("ok").toBool());
        QCOMPARE(count("SELECT COUNT(*) FROM venue"), 0);
        const QVariantMap r = b.importDataFile(QUrl::fromLocalFile(file).toString());
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(count("SELECT COUNT(*) FROM venue"), 6);
        QCOMPARE(count("SELECT COUNT(*) FROM venue WHERE unavailable = 1"), 1);
        QCOMPARE(b.venueSummary(m_club).value("department").toString(), QStringLiteral("60"));
        QCOMPARE(b.venueSummary(m_club).value("homeVenueId").toInt(), lagny);
        QCOMPARE(b.match(match).value("match").toMap().value("venueId").toInt(), nanteuil);
        QCOMPARE(b.suggestedVenue(m_team, "away", "US Nanteuil").value("id").toInt(), nanteuil);
    }

    // ----- Vrai appel à Data ES (action volontaire : FOOTCOACH_NETWORK_TESTS=1) -----
    void realDataEsDownload()
    {
        if (qEnvironmentVariable("FOOTCOACH_NETWORK_TESTS") != QLatin1String("1"))
            QSKIP("Définir FOOTCOACH_NETWORK_TESTS=1 pour interroger le vrai service Data ES");
        QNetworkProxyFactory::setUseSystemConfiguration(true);   // proxy éventuel de la machine de test
        Backend b(m_db.handle());
        QVERIFY(b.setClubDepartment(m_club, "60").value("ok").toBool());
        QSignalSpy spy(&b, &Backend::venuesRefreshed);
        QVERIFY(b.refreshVenues(m_club));
        QVERIFY(b.venuesBusy());
        QVERIFY(!b.refreshVenues(m_club));                       // un seul téléchargement à la fois
        QVERIFY(spy.wait(60000));
        const QVariantMap r = spy.first().first().toMap();
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString() + " / " + r.value("detail").toString()));
        QVERIFY(!b.venuesBusy());
        qInfo() << "Data ES, Oise :" << r;
        QVERIFY(r.value("total").toInt() > 500);
        QVERIFY(venueIdOf("E001I603410001") > 0);                // Lagny-le-Sec
    }

    // ----- Itinéraire : adresse « geo: » (documentation Android, intents Google Maps) -----
    void geoUriUsesCoordinatesOrTheAddress()
    {
        // Coordonnées seules : le nom n'est pas envoyé (Waze le cherchait comme du texte et se trompait de lieu).
        QCOMPARE(Backend::geoUri(49.11352, 3.02385, QStringLiteral("Terrain de Football & Tir à l'Arc"), QString()),
                 QStringLiteral("geo:49.113520,3.023850?q=49.113520,3.023850"));
        QCOMPARE(Backend::geoUri(qQNaN(), qQNaN(), QString(), QStringLiteral("2 rue des Écoles, 60330 Lagny-le-Sec")),
                 QStringLiteral("geo:0,0?q=2%20rue%20des%20%C3%89coles%2C%2060330%20Lagny-le-Sec"));
        QCOMPARE(Backend::geoUri(0.0, 0.0, QStringLiteral("X"), QStringLiteral("Stade, Paris")),
                 QStringLiteral("geo:0,0?q=Stade%2C%20Paris"));                 // 0,0 : coordonnées absentes
        QVERIFY(Backend::geoUri(qQNaN(), qQNaN(), QString(), QStringLiteral("  ")).isEmpty());
        Backend b(m_db.handle());
        QVERIFY(b.applyVenueData(m_club, fixture()).value("ok").toBool());
        const QVariantMap r = b.openItinerary(venueIdOf("E001I603410001"), QStringLiteral("texte"));
        QCOMPARE(r.value("uri").toString(), QStringLiteral("geo:49.085130,2.749820?q=49.085130,2.749820"));
        QVERIFY(!b.openItinerary(-1, QString()).value("ok").toBool());          // ni stade ni adresse
    }

    // ----- Fonctions pures -----
    void opponentKeyIgnoresCaseAccentsAndPunctuation()
    {
        QCOMPARE(VenueRepository::opponentKey(QStringLiteral("U.S. Nanteuil")), QStringLiteral("us nanteuil"));
        QCOMPARE(VenueRepository::opponentKey(QStringLiteral("  ÉS   Lagny-le-Sec ")), QStringLiteral("es lagny le sec"));
        QCOMPARE(VenueRepository::opponentKey(QStringLiteral("FC Ève 2")), QStringLiteral("fc eve 2"));
        QVERIFY(VenueRepository::opponentKey(QStringLiteral(" - ")).isEmpty());
    }
};

QTEST_MAIN(VenuesTest)
#include "tst_venues.moc"
