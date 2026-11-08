// Compatibilité du format d'échange « .footcoach » (voir docs/versionnage.md).
//
// Les fichiers de tests/fixtures/ sont des références figées : ils ne sont JAMAIS régénérés pour « faire passer »
// un test. Ils ne se recréent (action volontaire) qu'avec FOOTCOACH_WRITE_FIXTURES=<dossier> et le test
// writeFixtures, après revue (voir tests/fixtures/README.md).
#include "app/Backend.h"
#include "core/Database.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

#ifndef FOOTCOACH_FIXTURES_DIR
#error "FOOTCOACH_FIXTURES_DIR doit être défini par CMake"
#endif

namespace {

const QStringList kTables = {QStringLiteral("club"),           QStringLiteral("team"),
                             QStringLiteral("player"),         QStringLiteral("training_session"),
                             QStringLiteral("attendance"),     QStringLiteral("football_match"),
                             QStringLiteral("match_player"),   QStringLiteral("exercise"),
                             QStringLiteral("session_exercise"), QStringLiteral("tactical_board"),
                             QStringLiteral("injury")};

// Tables présentes dans un fichier du schéma donné (le schéma 6 a ajouté club, football_match et match_player).
bool tableExistsInSchema(const QString &t, int schema)
{
    return schema >= 6 || !(t == QLatin1String("club") || t == QLatin1String("football_match")
                            || t == QLatin1String("match_player"));
}

QString fixturePath(int schema)
{
    return QStringLiteral(FOOTCOACH_FIXTURES_DIR "/schema%1.footcoach").arg(schema);
}

QJsonObject readJson(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}

bool writeJson(const QString &path, const QJsonObject &o)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return f.write(QJsonDocument(o).toJson(QJsonDocument::Compact)) > 0;
}

// Découpe un script de migration comme le fait le moteur (lignes de commentaire ignorées, un statement par « ; »).
QStringList statementsOf(const QString &resource)
{
    QFile f(resource);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QString script;
    for (const QString &line : QString::fromUtf8(f.readAll()).split(QLatin1Char('\n')))
        if (!line.trimmed().startsWith(QLatin1String("--")))
            script += line + QLatin1Char('\n');
    QStringList out;
    for (const QString &s : script.split(QLatin1Char(';')))
        if (!s.trimmed().isEmpty())
            out << s;
    return out;
}

// Données d'exemple fictives. Les colonnes valables dans tous les schémas 1 à 4 (la colonne
// exercise.archived n'existe qu'à partir du schéma 3 et prend sa valeur par défaut).
const QStringList kSampleData = {
    "INSERT INTO team(id,name,category,season) VALUES(1,'Les Étoiles U15','U15','2026-2027')",
    "INSERT INTO team(id,name,category,season) VALUES(2,'Seniors B',NULL,NULL)",
    "INSERT INTO player(id,team_id,first_name,last_name,birth_date,jersey_number,position,preferred_foot,phone,email,notes,active) "
    "VALUES(1,1,'Zoé','O''Brien','2011-03-04',9,'FWD','R','06 00 00 00 01','zoe@example.invalid','Capitaine',1)",
    "INSERT INTO player(id,team_id,first_name,last_name,birth_date,jersey_number,position,preferred_foot,phone,email,notes,active) "
    "VALUES(2,1,'Yanis','Lefèvre',NULL,NULL,NULL,NULL,NULL,NULL,NULL,0)",
    "INSERT INTO player(id,team_id,first_name,last_name,birth_date,jersey_number,position,preferred_foot,phone,email,notes,active) "
    "VALUES(3,2,'Aïcha','Benali','1998-11-30',1,'GK','L',NULL,NULL,NULL,1)",
    "INSERT INTO training_session(id,team_id,start_at,duration_min,location,theme,notes,status) "
    "VALUES(1,1,'2026-11-03T18:30',90,'Stade municipal','Finition','Prévoir les chasubles','planned')",
    "INSERT INTO training_session(id,team_id,start_at,duration_min,location,theme,notes,status) "
    "VALUES(2,1,'2026-10-27T18:30',75,NULL,NULL,NULL,'cancelled')",
    "INSERT INTO attendance(session_id,player_id,status,note) VALUES(1,1,'present','à l''heure')",
    "INSERT INTO attendance(session_id,player_id,status,note) VALUES(1,2,'injured',NULL)",
    "INSERT INTO exercise(title,category,objective,duration_min,players_min,players_max,equipment,description) "
    "VALUES('Exercice de référence','game','Objectif « test »',25,8,16,'Ballons, chasubles','Description sur\nplusieurs lignes')",
    "INSERT INTO session_exercise(session_id,exercise_id,position,duration_min,notes) "
    "VALUES(1,(SELECT id FROM exercise WHERE title='Exercice de référence'),1,20,'Insister sur le premier contrôle')",
    "INSERT INTO tactical_board(id,title,formation,scene_json,thumbnail,exercise_id,session_id) "
    "VALUES(1,'Pressing haut','4-3-3','{\"items\":[{\"t\":\"player\",\"x\":0.5}]}',x'89504E470D0A1A0A',"
    "(SELECT id FROM exercise WHERE title='Exercice de référence'),1)",
    "INSERT INTO injury(player_id,start_date,end_date,description) VALUES(2,'2026-10-20',NULL,'Entorse')",
};

// Ajouts propres au schéma 5 : un terrain rattaché au bloc de la séance 1 (le schéma de référence ci-dessus reste lié à l'exercice).
const QStringList kSampleDataSchema5 = {
    "INSERT INTO tactical_board(id,title,formation,scene_json,exercise_id,session_id,session_exercise_id) "
    "VALUES(2,'Atelier 1 : jeu à thème',NULL,'{\"items\":[{\"t\":\"player\",\"x\":0.25}]}',NULL,NULL,"
    "(SELECT id FROM session_exercise WHERE session_id=1 AND position=1))",
};

// Ajouts propres au schéma 6 : les équipes appartiennent au club créé par la migration, un match avec deux convoqués
// et son schéma tactique.
const QStringList kSampleDataSchema6 = {
    "UPDATE team SET club_id = 1",
    "INSERT INTO football_match(id,team_id,start_at,opponent,location,home_away,status,score_for,score_against,notes) "
    "VALUES(1,1,'2026-11-08T15:00','FC Rivière','Stade d''Été','away','planned',NULL,NULL,'Match de championnat')",
    "INSERT INTO match_player(match_id,player_id,role) VALUES(1,1,'starter')",
    "INSERT INTO match_player(match_id,player_id,role) VALUES(1,2,'substitute')",
    "INSERT INTO tactical_board(id,title,formation,scene_json,match_id) "
    "VALUES(3,'Match contre FC Rivière','4-4-2','{\"items\":[{\"t\":\"player\",\"x\":0.75}]}',1)",
};

// Ajouts propres au schéma 7 : licence et photo (petit BLOB JPEG fictif) du joueur 1 ; licence du joueur 3 dans une autre équipe.
const QStringList kSampleDataSchema7 = {
    "UPDATE player SET licence = 'AB12345', photo = x'FFD8FFE000104A46494600FFD9' WHERE id = 1",
    "UPDATE player SET licence = 'AB12345' WHERE id = 3",
};

// Prépare un fichier de base au schéma « schema » (migrations 001 à 00N, user_version, données d'exemple), sans passer par Database.
bool buildDatabaseAtSchema(const QString &dbFile, const QString &conn, int schema, QString *error)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
    db.setDatabaseName(dbFile);
    if (!db.open()) {
        *error = db.lastError().text();
        return false;
    }
    QSqlQuery q(db);
    bool ok = q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    const QDir sql(QStringLiteral(":/sql"));
    const QStringList files = sql.entryList({QStringLiteral("*.sql")}, QDir::Files, QDir::Name);
    for (int n = 0; ok && n < schema; ++n)
        for (const QString &st : statementsOf(sql.filePath(files.at(n))))
            if (!(ok = q.exec(st))) {
                *error = q.lastError().text() + " / " + st;
                break;
            }
    ok = ok && q.exec(QStringLiteral("PRAGMA user_version = %1").arg(schema));
    for (const QString &st : kSampleData)
        if (ok && !(ok = q.exec(st)))
            *error = q.lastError().text() + " / " + st;
    if (ok && schema >= 5)
        for (const QString &st : kSampleDataSchema5)
            if (ok && !(ok = q.exec(st)))
                *error = q.lastError().text() + " / " + st;
    if (ok && schema >= 6)
        for (const QString &st : kSampleDataSchema6)
            if (ok && !(ok = q.exec(st)))
                *error = q.lastError().text() + " / " + st;
    if (ok && schema >= 7)
        for (const QString &st : kSampleDataSchema7)
            if (ok && !(ok = q.exec(st)))
                *error = q.lastError().text() + " / " + st;
    return ok;
}

} // namespace

class DataFormatTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    Database m_db;
    int m_counter = 0;

    // Snapshot canonique des tables de la base de l'application (export complet relu).
    QJsonObject snapshot(Backend &b)
    {
        const QString p = m_dir.filePath(QStringLiteral("snap%1.footcoach").arg(++m_counter));
        if (!b.exportDataFile(p).value("ok").toBool())
            return {};
        return readJson(p).value("tables").toObject();
    }

    // Le club par défaut d'un ancien fichier est créé à l'import : son horodatage n'est pas comparable d'un import à l'autre.
    static QJsonObject stable(QJsonObject tables)
    {
        QJsonObject club = tables.value("club").toObject();
        QJsonArray rows;
        for (const QJsonValue &r : club.value("rows").toArray()) {
            QJsonArray row = r.toArray();
            row[row.size() - 1] = QString();
            rows.append(row);
        }
        club["rows"] = rows;
        tables["club"] = club;
        return tables;
    }

    QString tmpFile(const QJsonObject &o)
    {
        const QString p = m_dir.filePath(QStringLiteral("t%1.footcoach").arg(++m_counter));
        writeJson(p, o);
        return p;
    }

    static QString url(const QString &path) { return QUrl::fromLocalFile(path).toString(); }

    // Chaque ligne du fichier doit se retrouver dans la base, colonne par colonne (colonnes du fichier seulement).
    void verifyContains(const QJsonObject &fileTables, const QJsonObject &dbTables)
    {
        for (const QString &t : kTables) {
            if (!fileTables.contains(t))                 // fichier d'un schéma antérieur à la table : rien à comparer
                continue;
            const QJsonArray fcols = fileTables.value(t).toObject().value("columns").toArray();
            const QJsonArray frows = fileTables.value(t).toObject().value("rows").toArray();
            const QJsonArray dcols = dbTables.value(t).toObject().value("columns").toArray();
            const QJsonArray drows = dbTables.value(t).toObject().value("rows").toArray();
            QCOMPARE(drows.size(), frows.size());
            for (int r = 0; r < frows.size(); ++r)
                for (int c = 0; c < fcols.size(); ++c) {
                    int dc = -1;
                    for (int k = 0; k < dcols.size(); ++k)
                        if (dcols.at(k) == fcols.at(c))
                            dc = k;
                    QVERIFY2(dc >= 0, qPrintable(t + "." + fcols.at(c).toString()));
                    QVERIFY2(drows.at(r).toArray().at(dc) == frows.at(r).toArray().at(c),
                             qPrintable(QStringLiteral("%1 ligne %2 colonne %3").arg(t).arg(r).arg(fcols.at(c).toString())));
                }
        }
    }

private slots:
    void init()
    {
        m_db.close();
        QFile::remove(m_dir.filePath(QStringLiteral("t.db")));
        QString error;
        QVERIFY2(m_db.open(m_dir.filePath(QStringLiteral("t.db")), &error), qPrintable(error));
    }

    void cleanup() { m_db.close(); }

    // ----- Création volontaire des fichiers de référence (ignorée en fonctionnement normal) -----
    void writeFixtures()
    {
        const QString out = qEnvironmentVariable("FOOTCOACH_WRITE_FIXTURES");
        if (out.isEmpty())
            QSKIP("Définir FOOTCOACH_WRITE_FIXTURES=<dossier> pour recréer les fichiers de référence");
        QVERIFY(QDir().mkpath(out));
        // Les fichiers 1 à 5 sont figés (créés par les versions d'époque) : seul le schéma courant se recrée, car l'export
        // du code actuel ne sait plus lire une base d'un schéma sans club.
        for (int schema = 7; schema <= 7; ++schema) {
            const QString conn = QStringLiteral("fixture-%1").arg(schema);
            const QString dbFile = m_dir.filePath(QStringLiteral("fx%1.db").arg(schema));
            {
                QString error;
                QVERIFY2(buildDatabaseAtSchema(dbFile, conn, schema, &error), qPrintable(error));
                QSqlDatabase db = QSqlDatabase::database(conn);
                QCoreApplication::setApplicationVersion(schema == 4 ? QStringLiteral("0.8.0")
                                                        : schema == 5 ? QStringLiteral("0.10.0")
                                                        : schema == 6 ? QStringLiteral("0.11.0")
                                                        : schema == 7 ? QStringLiteral("0.12.0")
                                                                      : QStringLiteral("reconstitue-schema-%1").arg(schema));
                Backend b(db);
                QVERIFY(b.exportDataFile(QDir(out).filePath(QStringLiteral("schema%1.footcoach").arg(schema))).value("ok").toBool());
            }
            QSqlDatabase::removeDatabase(conn);
        }
    }

    // ----- Structure figée du format version 1 -----
    void referenceFilesHaveTheV1Structure()
    {
        const QStringList keys = {"app", "exportedAt", "format", "schema", "tables", "version"};
        for (int schema = 1; schema <= 7; ++schema) {
            const QJsonObject o = readJson(fixturePath(schema));
            QVERIFY2(!o.isEmpty(), qPrintable(fixturePath(schema)));
            QCOMPARE(o.keys(), keys);
            QCOMPARE(o.value("format").toString(), QStringLiteral("footcoach-data"));
            QCOMPARE(o.value("version").toInt(), 1);
            QCOMPARE(o.value("schema").toInt(), schema);
            QCOMPARE(o.value("tables").toObject().keys().size(), schema >= 6 ? kTables.size() : kTables.size() - 3);
            for (const QString &t : kTables) {
                if (!tableExistsInSchema(t, schema))
                    continue;
                const QJsonObject tb = o.value("tables").toObject().value(t).toObject();
                QVERIFY(tb.value("columns").isArray());
                QVERIFY(tb.value("rows").isArray());
            }
        }
        QCOMPARE(readJson(fixturePath(4)).value("app").toString(), QStringLiteral("0.8.0"));
        QCOMPARE(readJson(fixturePath(5)).value("app").toString(), QStringLiteral("0.10.0"));
    }

    // Le fichier du schéma 6 contient bien les nouvelles données : club, match, convoqués, schéma tactique de match.
    void schema6ReferenceFileHasClubsAndMatches()
    {
        const QJsonObject t = readJson(fixturePath(7)).value("tables").toObject();
        auto rows = [&](const char *name) { return t.value(QLatin1String(name)).toObject().value("rows").toArray().size(); };
        QCOMPARE(rows("club"), 1);
        QCOMPARE(rows("team"), 2);
        QCOMPARE(rows("football_match"), 1);
        QCOMPARE(rows("match_player"), 2);
        QCOMPARE(rows("tactical_board"), 3);
    }

    // Le fichier du schéma 7 porte licence et photo ; ils survivent à l'import, à l'API et à un nouvel export.
    void schema7PhotoAndLicenceRoundTrip()
    {
        Backend b(m_db.handle());
        QVERIFY(b.importDataFile(url(fixturePath(7))).value("ok").toBool());
        QCOMPARE(b.players(1, true).at(1).toMap().value("licence").toString(), QStringLiteral("AB12345"));
        QVERIFY(b.players(1, true).at(1).toMap().value("hasPhoto").toBool());
        QVERIFY(!b.players(1, true).at(0).toMap().value("hasPhoto").toBool());
        QVERIFY(b.playerPhoto(1).startsWith(QStringLiteral("data:image/jpeg;base64,/9j/")));   // FFD8FF...
        QVERIFY(b.playerPhoto(2).isEmpty());
        // Même licence dans une autre équipe : permise ; le export/import ne perd rien.
        QCOMPARE(b.players(2, true).first().toMap().value("licence").toString(), QStringLiteral("AB12345"));
        const QJsonObject first = stable(snapshot(b));
        const QString out = m_dir.filePath(QStringLiteral("again.footcoach"));
        QVERIFY(b.exportDataFile(out).value("ok").toBool());
        QVERIFY(b.importDataFile(url(out)).value("ok").toBool());
        QCOMPARE(stable(snapshot(b)), first);
        // Un fichier qui donne deux fois la même licence à une équipe est refusé en entier.
        QJsonObject root = readJson(fixturePath(7));
        QJsonObject tables = root.value("tables").toObject();
        QJsonObject pl = tables.value("player").toObject();
        QJsonArray cols = pl.value("columns").toArray();
        int li = -1;
        for (int i = 0; i < cols.size(); ++i)
            if (cols.at(i).toString() == QLatin1String("licence"))
                li = i;
        QVERIFY(li > 0);
        QJsonArray rows = pl.value("rows").toArray();
        QJsonArray second = rows.at(1).toArray();           // joueur 2, même équipe que le joueur 1
        second[li] = QStringLiteral("ab12345");             // casse ignorée
        rows[1] = second;
        pl["rows"] = rows;
        tables["player"] = pl;
        root["tables"] = tables;
        const QVariantMap r = b.importDataFile(url(tmpFile(root)));
        QVERIFY(!r.value("ok").toBool());
        QCOMPARE(stable(snapshot(b)), first);
    }

    // Un export fait aujourd'hui doit garder exactement la structure du fichier de référence du schéma courant (6),
    // et ne retirer aucune colonne du fichier de référence de la 0.8.0 (schéma 4) : le format v1 évolue par ajout de colonnes.
    void currentExportKeepsTheV1Structure()
    {
        Backend b(m_db.handle());
        const QString p = m_dir.filePath(QStringLiteral("now.footcoach"));
        QVERIFY(b.exportDataFile(p).value("ok").toBool());
        const QJsonObject now = readJson(p), ref = readJson(fixturePath(7)), old = readJson(fixturePath(4));
        QCOMPARE(now.keys(), ref.keys());
        QCOMPARE(now.value("format"), ref.value("format"));
        QCOMPARE(now.value("version"), ref.value("version"));
        QCOMPARE(now.value("schema"), ref.value("schema"));
        QCOMPARE(now.value("schema").toInt(), 7);
        for (const QString &t : kTables) {
            const QJsonArray cols = now.value("tables").toObject().value(t).toObject().value("columns").toArray();
            QCOMPARE(cols, ref.value("tables").toObject().value(t).toObject().value("columns").toArray());
            // aucune colonne du schéma 4 n'a disparu, et les nouvelles viennent après
            const QJsonArray oldCols = old.value("tables").toObject().value(t).toObject().value("columns").toArray();
            for (int i = 0; i < oldCols.size(); ++i)
                QCOMPARE(cols.at(i), oldCols.at(i));
        }
        QCOMPARE(now.value("tables").toObject().value("tactical_board").toObject().value("columns").toArray().size(),
                 old.value("tables").toObject().value("tactical_board").toObject().value("columns").toArray().size() + 2);
    }

    // ----- Rétrocompatibilité : chaque fichier de référence s'importe sur le schéma courant -----
    void importReferenceFiles_data()
    {
        QTest::addColumn<int>("schema");
        for (int s = 1; s <= 7; ++s)
            QTest::newRow(qPrintable(QStringLiteral("schema %1").arg(s))) << s;
    }

    void importReferenceFiles()
    {
        QFETCH(int, schema);
        Backend b(m_db.handle());
        const QString file = fixturePath(schema);
        const QJsonObject ref = readJson(file);

        const QVariantMap info = b.inspectDataFile(file);
        QVERIFY2(info.value("ok").toBool(), qPrintable(info.value("error").toString()));
        QCOMPARE(info.value("counts").toMap().value("teams").toInt(), 2);
        QCOMPARE(info.value("counts").toMap().value("players").toInt(), 3);
        QCOMPARE(info.value("counts").toMap().value("sessions").toInt(), 2);
        QCOMPARE(info.value("counts").toMap().value("boards").toInt(), schema >= 6 ? 3 : (schema >= 5 ? 2 : 1));
        QCOMPARE(info.value("counts").toMap().value("clubs").toInt(), schema >= 6 ? 1 : 0);
        QCOMPARE(info.value("counts").toMap().value("matches").toInt(), schema >= 6 ? 1 : 0);

        const QVariantMap r = b.importDataFile(url(file));
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));

        const QJsonObject after = snapshot(b);
        verifyContains(ref.value("tables").toObject(), after);

        // Lecture par l'API de l'application : accents, apostrophes, NULL, BLOB, plan, appel.
        QCOMPARE(b.teams().size(), 2);
        const int team = b.teams().at(0).toMap().value("id").toInt();
        QCOMPARE(team, 1);
        // Club : un fichier d'avant le schéma 6 n'en a pas, ses équipes rejoignent un club par défaut (aucune sans club).
        QCOMPARE(b.clubs().size(), 1);
        for (const QVariant &t : b.teams())
            QCOMPARE(t.toMap().value("clubId").toInt(), b.clubs().at(0).toMap().value("id").toInt());
        if (schema >= 6) {
            const QVariantMap m = b.match(1);
            QVERIFY(m.value("ok").toBool());
            QCOMPARE(m.value("match").toMap().value("opponent").toString(), QStringLiteral("FC Rivière"));
            QCOMPARE(m.value("players").toList().size(), 2);
            QVERIFY(m.value("boardId").toInt() > 0);
        } else {
            QCOMPARE(b.matches(1, QStringLiteral("2000-01-01"), QStringLiteral("2100-01-01")).size(), 0);
        }
        QCOMPARE(b.players(1, true).size(), 2);
        QCOMPARE(b.players(1, false).size(), 1);                 // un joueur inactif
        QStringList lastNames;
        for (const QVariant &p : b.players(1, true))
            lastNames << p.toMap().value("lastName").toString();
        lastNames.sort();
        QCOMPARE(lastNames, (QStringList{QStringLiteral("Lefèvre"), QStringLiteral("O'Brien")}));
        QCOMPARE(b.plan(1).value("entries").toList().size(), 1);
        QVERIFY(b.attendanceSheet(1).value("ok").toBool());
        QCOMPARE(b.boards().size(), 1);                          // la bibliothèque n'affiche pas les terrains de bloc

        // Colonne apparue (schéma 5 : session_exercise_id) : absente des fichiers 1 à 4 → NULL, présente dans le 5
        QSqlQuery qb(m_db.handle());
        QVERIFY(qb.exec(QStringLiteral("SELECT COUNT(*) FROM tactical_board WHERE session_exercise_id IS NOT NULL")) && qb.next());
        QCOMPARE(qb.value(0).toInt(), schema >= 5 ? 1 : 0);
        QCOMPARE(b.plan(1).value("entries").toList().first().toMap().value("boardId").toInt() > 0, schema >= 5);

        // Colonne apparue après le schéma du fichier : valeur par défaut, exercice visible et non archivé.
        const QVariantList all = b.exercises(true);
        QVERIFY(all.size() >= 1);
        for (const QVariant &e : all)
            QVERIFY(!e.toMap().value("archived").toBool());

        // Un fichier de schéma antérieur au 004 n'apporte pas les exercices de départ : ils ne sont pas recréés.
        const int exercisesInFile = ref.value("tables").toObject().value("exercise").toObject().value("rows").toArray().size();
        QCOMPARE(all.size(), exercisesInFile);

        // Réexporter puis réimporter ne change rien (idempotence).
        const QJsonObject first = stable(snapshot(b));
        QVERIFY(b.importDataFile(url(file)).value("ok").toBool());
        QCOMPARE(stable(snapshot(b)), first);

        // Étape 2 : le terrain d'un bloc fonctionne sur les données issues de chaque schéma de fichier (1 à 5).
        const QVariantMap block = b.plan(1).value("entries").toList().first().toMap();
        const int item = block.value("itemId").toInt();
        QCOMPARE(block.value("boardId").toInt() > 0, schema >= 5);               // seul le fichier 5 en contient un
        const QVariantMap made = b.createBlockBoard(item);
        QVERIFY2(made.value("ok").toBool(), qPrintable(made.value("error").toString()));
        QCOMPARE(made.value("created").toBool(), schema < 5);                    // idempotent : le terrain du fichier 5 est retrouvé
        const QVariantMap planned = b.plan(1).value("entries").toList().first().toMap();
        QCOMPARE(planned.value("boardId").toInt(), made.value("boardId").toInt());
        // Le contenu du terrain du fichier 5 est un JSON d'exemple qui n'est pas une scène valide : il est signalé
        // illisible et jamais écrasé. Un terrain créé ici est vide, lisible et modifiable.
        QCOMPARE(planned.value("boardReadable").toBool(), schema < 5);
        QCOMPARE(b.saveBoard(made.value("boardId").toInt(), QStringLiteral("{\"version\":1,\"pitch\":\"full\",\"items\":[],\"arrows\":[]}")),
                 schema < 5);
        QCOMPARE(b.boards().size(), 1);                                          // la bibliothèque ne montre pas les terrains de bloc
        // 2 terrains au total : le schéma de référence du fichier et le terrain du bloc (créé ici, ou celui du fichier 5)
        // (+ le schéma tactique du match dans le fichier 6)
        QCOMPARE(snapshot(b).value("tactical_board").toObject().value("rows").toArray().size(), schema >= 6 ? 3 : 2);
    }

    // ----- Migration 005 : base réelle au schéma 4 ouverte par le code courant -----
    void migrationFromSchema4KeepsExistingData()
    {
        const QString file = m_dir.filePath(QStringLiteral("old4.db"));
        {
            QString error;
            QVERIFY2(buildDatabaseAtSchema(file, QStringLiteral("old4"), 4, &error), qPrintable(error));
        }
        QSqlDatabase::removeDatabase(QStringLiteral("old4"));

        Database db;
        QString error;
        QVERIFY2(db.open(file, &error), qPrintable(error));
        QCOMPARE(db.schemaVersion(), 7);
        QSqlQuery q(db.handle());
        auto count = [&](const QString &sql) { return q.exec(sql) && q.next() ? q.value(0).toInt() : -1; };
        QCOMPARE(count("SELECT COUNT(*) FROM team"), 2);
        QCOMPARE(count("SELECT COUNT(*) FROM player"), 3);
        QCOMPARE(count("SELECT COUNT(*) FROM training_session"), 2);
        QCOMPARE(count("SELECT COUNT(*) FROM attendance"), 2);
        QCOMPARE(count("SELECT COUNT(*) FROM session_exercise"), 1);
        QCOMPARE(count("SELECT COUNT(*) FROM injury"), 1);
        QVERIFY(count("SELECT COUNT(*) FROM exercise") > 1);                     // exercices de départ de la 004 conservés
        QCOMPARE(count("SELECT COUNT(*) FROM tactical_board"), 1);
        QCOMPARE(count("SELECT COUNT(*) FROM tactical_board WHERE session_exercise_id IS NULL"), 1);
        QVERIFY(q.exec(QStringLiteral("SELECT title, formation, scene_json, length(thumbnail), exercise_id, session_id FROM tactical_board")) && q.next());
        QCOMPARE(q.value(0).toString(), QStringLiteral("Pressing haut"));
        QCOMPARE(q.value(1).toString(), QStringLiteral("4-3-3"));
        QCOMPARE(q.value(3).toInt(), 8);
        QVERIFY(!q.value(4).isNull());
        QCOMPARE(q.value(5).toInt(), 1);
        // Schéma 6 : un club par défaut reçoit toutes les équipes existantes ; aucune donnée de match.
        QCOMPARE(count("SELECT COUNT(*) FROM club"), 1);
        QCOMPARE(count("SELECT COUNT(*) FROM team WHERE club_id = (SELECT id FROM club)"), 2);
        QCOMPARE(count("SELECT COUNT(*) FROM team WHERE club_id IS NULL"), 0);
        QCOMPARE(count("SELECT COUNT(*) FROM football_match"), 0);
        QCOMPARE(count("SELECT COUNT(*) FROM tactical_board WHERE match_id IS NOT NULL"), 0);
        QCOMPARE(count("PRAGMA foreign_keys"), 1);                               // actives sur la connexion de l'application
        QCOMPARE(count("SELECT COUNT(*) FROM pragma_foreign_key_check"), 0);
        QCOMPARE(count("SELECT COUNT(*) FROM sqlite_master WHERE type='index' AND name='idx_board_session_exercise'"), 1);

        // Rouvrir ne rejoue rien.
        db.close();
        QVERIFY2(db.open(file, &error), qPrintable(error));
        QCOMPARE(db.schemaVersion(), 7);
        QSqlQuery q2(db.handle());
        QVERIFY(q2.exec(QStringLiteral("SELECT COUNT(*) FROM tactical_board")) && q2.next());
        QCOMPARE(q2.value(0).toInt(), 1);
    }

    // Cascade réelle après import du fichier de référence du schéma 5 : bloc → terrain, séance → blocs → terrains.
    void importedBlockBoardFollowsItsBlockAndSession()
    {
        Backend b(m_db.handle());
        QVERIFY(b.importDataFile(url(fixturePath(5))).value("ok").toBool());
        QSqlQuery q(m_db.handle());
        auto count = [&](const QString &sql) { return q.exec(sql) && q.next() ? q.value(0).toInt() : -1; };
        QCOMPARE(count("SELECT COUNT(*) FROM tactical_board"), 2);
        QCOMPARE(count("SELECT COUNT(*) FROM tactical_board WHERE session_exercise_id IS NOT NULL"), 1);
        QVERIFY(q.exec(QStringLiteral("DELETE FROM training_session WHERE id = 1")));
        QCOMPARE(count("SELECT COUNT(*) FROM session_exercise"), 0);
        QCOMPARE(count("SELECT COUNT(*) FROM tactical_board WHERE session_exercise_id IS NOT NULL"), 0);
        QCOMPARE(count("SELECT COUNT(*) FROM tactical_board WHERE exercise_id IS NOT NULL OR title = 'Pressing haut'"), 1);   // la référence reste
    }

    // Un fichier du schéma 5 est refusé par une application qui n'en connaît que 4 : la règle est « schema > schéma courant ».
    void fileNewerThanCurrentSchemaIsRefusedWithAClearMessage()
    {
        Backend b(m_db.handle());
        QJsonObject root = readJson(fixturePath(7));
        root["schema"] = 8;
        const QVariantMap r = b.importDataFile(url(tmpFile(root)));
        QVERIFY(!r.value("ok").toBool());
        QVERIFY2(r.value("error").toString().contains(QStringLiteral("plus récente")), qPrintable(r.value("error").toString()));
        root = readJson(fixturePath(7));
        root["version"] = 2;
        const QVariantMap r2 = b.importDataFile(url(tmpFile(root)));
        QVERIFY(!r2.value("ok").toBool());
        QVERIFY(r2.value("error").toString().contains(QStringLiteral("Version de fichier")));
    }

    void referenceFileContainsTheSeedExercisesOnlyFromSchema4()
    {
        auto n = [&](int s) { return readJson(fixturePath(s)).value("tables").toObject().value("exercise").toObject().value("rows").toArray().size(); };
        QCOMPARE(n(1), 1);
        QCOMPARE(n(2), 1);
        QCOMPARE(n(3), 1);
        QVERIFY(n(4) > 1);
    }

    // ----- Fichiers refusés : message explicite, données inchangées -----
    void refusedFiles_data()
    {
        QTest::addColumn<QByteArray>("mutation");
        QTest::addColumn<QString>("messagePart");
        QTest::newRow("schema plus récent") << QByteArray("schema=8") << QString("plus récente");
        QTest::newRow("schema très grand") << QByteArray("schema=999") << QString("plus récente");
        QTest::newRow("schema absent") << QByteArray("schema=remove") << QString("");
        QTest::newRow("schema nul") << QByteArray("schema=0") << QString("");
        QTest::newRow("format version 2") << QByteArray("version=2") << QString("Version de fichier");
        QTest::newRow("format version 0") << QByteArray("version=0") << QString("Version de fichier");
        QTest::newRow("format version absente") << QByteArray("version=remove") << QString("Version de fichier");
        QTest::newRow("autre format") << QByteArray("format=autre") << QString("pas un export");
        QTest::newRow("table absente (export partiel)") << QByteArray("drop=exercise") << QString("incomplet");
        QTest::newRow("tables vides absentes") << QByteArray("drop=*") << QString("incomplet");
        QTest::newRow("colonne inconnue") << QByteArray("addcol=player") << QString("colonne inconnue");
        QTest::newRow("ligne trop courte") << QByteArray("shortrow=player") << QString("mal formée");
        QTest::newRow("valeur illisible") << QByteArray("badvalue=team") << QString("Import refusé");
        QTest::newRow("clé primaire en double") << QByteArray("dup=team") << QString("Import refusé");
        QTest::newRow("contrainte CHECK violée") << QByteArray("badstatus=training_session") << QString("Import refusé");
        QTest::newRow("clé étrangère violée") << QByteArray("badfk=player") << QString("Import refusé");
    }

    void refusedFiles()
    {
        QFETCH(QByteArray, mutation);
        QFETCH(QString, messagePart);
        Backend b(m_db.handle());
        QVERIFY(b.importDataFile(url(fixturePath(4))).value("ok").toBool());       // état de départ connu
        const QJsonObject before = snapshot(b);

        QJsonObject root = readJson(fixturePath(4));
        QJsonObject tables = root.value("tables").toObject();
        const QList<QByteArray> kv = mutation.split('=');
        const QByteArray op = kv.at(0), arg = kv.at(1);
        auto mutateTable = [&](const QString &t, auto fn) {
            QJsonObject tb = tables.value(t).toObject();
            fn(tb);
            tables[t] = tb;
        };
        if (op == "schema") {
            if (arg == "remove") root.remove("schema"); else root["schema"] = arg.toInt();
        } else if (op == "version") {
            if (arg == "remove") root.remove("version"); else root["version"] = arg.toInt();
        } else if (op == "format") {
            root["format"] = QString::fromUtf8(arg);
        } else if (op == "drop") {
            if (arg == "*") tables = QJsonObject(); else tables.remove(QString::fromUtf8(arg));
        } else if (op == "addcol") {
            mutateTable(QString::fromUtf8(arg), [](QJsonObject &tb) {
                QJsonArray c = tb.value("columns").toArray();
                c.append("colonne_fantome");
                tb["columns"] = c;
            });
        } else if (op == "shortrow") {
            mutateTable(QString::fromUtf8(arg), [](QJsonObject &tb) {
                QJsonArray rows = tb.value("rows").toArray();
                QJsonArray r = rows.at(0).toArray();
                r.removeLast();
                rows[0] = r;
                tb["rows"] = rows;
            });
        } else if (op == "badvalue") {
            mutateTable(QString::fromUtf8(arg), [](QJsonObject &tb) {
                QJsonArray rows = tb.value("rows").toArray();
                QJsonArray r = rows.at(0).toArray();
                r[1] = QJsonArray{1, 2};                      // tableau imbriqué : pas une valeur SQLite
                rows[0] = r;
                tb["rows"] = rows;
            });
        } else if (op == "dup") {
            mutateTable(QString::fromUtf8(arg), [](QJsonObject &tb) {
                QJsonArray rows = tb.value("rows").toArray();
                rows.append(rows.at(0));                       // même identifiant deux fois
                tb["rows"] = rows;
            });
        } else if (op == "badstatus") {
            mutateTable(QString::fromUtf8(arg), [](QJsonObject &tb) {
                const QJsonArray cols = tb.value("columns").toArray();
                int idx = -1;
                for (int i = 0; i < cols.size(); ++i)
                    if (cols.at(i).toString() == "status") idx = i;
                QJsonArray rows = tb.value("rows").toArray();
                QJsonArray r = rows.at(0).toArray();
                r[idx] = QStringLiteral("n'importe quoi");
                rows[0] = r;
                tb["rows"] = rows;
            });
        } else if (op == "badfk") {
            mutateTable(QString::fromUtf8(arg), [](QJsonObject &tb) {
                QJsonArray rows = tb.value("rows").toArray();
                QJsonArray r = rows.at(0).toArray();
                r[1] = 424242;                                   // team_id inexistant
                rows[0] = r;
                tb["rows"] = rows;
            });
        }
        root["tables"] = tables;

        const QString file = tmpFile(root);
        const QVariantMap insp = b.inspectDataFile(file);
        const QVariantMap r = b.importDataFile(url(file));
        QVERIFY2(!r.value("ok").toBool(), "le fichier aurait dû être refusé");
        QVERIFY(!r.value("error").toString().isEmpty());
        if (!messagePart.isEmpty())
            QVERIFY2(r.value("error").toString().contains(messagePart), qPrintable(r.value("error").toString()));
        Q_UNUSED(insp);
        QCOMPARE(snapshot(b), before);                           // aucune donnée modifiée
    }

    // --- Règles de liens des terrains à l'import (SDD 5.3, décision D18 : refus total, aucune réparation) ---

    static int columnIndex(const QJsonObject &table, const QString &name)
    {
        const QJsonArray cols = table.value("columns").toArray();
        for (int i = 0; i < cols.size(); ++i)
            if (cols.at(i).toString() == name)
                return i;
        return -1;
    }

    // Copie d'un terrain du fichier (ligne « from ») ajoutée avec l'identifiant 99 ; set : colonne -> valeur à écraser.
    static QJsonObject withExtraBoard(QJsonObject root, int from, const QMap<QString, QJsonValue> &set)
    {
        QJsonObject tables = root.value("tables").toObject();
        QJsonObject tb = tables.value("tactical_board").toObject();
        QJsonArray rows = tb.value("rows").toArray();
        QJsonArray r = rows.at(from).toArray();
        r[columnIndex(tb, "id")] = 99;
        for (auto it = set.begin(); it != set.end(); ++it)
            r[columnIndex(tb, it.key())] = it.value();
        rows.append(r);
        tb["rows"] = rows;
        tables["tactical_board"] = tb;
        root["tables"] = tables;
        return root;
    }

    QDir backupDir() const { return QDir(QFileInfo(m_db.path()).absolutePath() + "/sauvegardes"); }
    QStringList backupNames() const
    {
        return backupDir().entryList({QStringLiteral("avant-import-*.footcoach")}, QDir::Files, QDir::Name);
    }

    void invalidBoardLinksRefuseTheWholeImport_data()
    {
        QTest::addColumn<int>("copyOfRow");                   // ligne du fichier dont on copie le terrain
        QTest::addColumn<QStringList>("overrideKeyValues");   // colonne, valeur, colonne, valeur…
        // Ligne 1 : terrain de bloc (session_exercise_id = 1) ; ligne 0 : schéma de référence de l'exercice 18.
        QTest::newRow("terrain de bloc aussi lié à un exercice") << 1 << QStringList{"exercise_id", "18"};
        QTest::newRow("terrain de bloc aussi lié à une séance") << 1 << QStringList{"session_id", "1"};
        QTest::newRow("deux terrains pour un même bloc") << 1 << QStringList{"title", "Doublon"};
        QTest::newRow("deux schémas de référence pour un exercice") << 0 << QStringList{"title", "Doublon"};
    }

    void invalidBoardLinksRefuseTheWholeImport()
    {
        QFETCH(int, copyOfRow);
        QFETCH(QStringList, overrideKeyValues);
        Backend b(m_db.handle());
        QVERIFY(b.importDataFile(url(fixturePath(5))).value("ok").toBool());       // état de départ connu
        const QJsonObject before = snapshot(b);
        const QStringList backupsBefore = backupNames();

        QMap<QString, QJsonValue> set;
        for (int i = 0; i + 1 < overrideKeyValues.size(); i += 2) {
            const QString key = overrideKeyValues.at(i), val = overrideKeyValues.at(i + 1);
            set.insert(key, key == "title" ? QJsonValue(val) : QJsonValue(val.toInt()));
        }
        const QString file = tmpFile(withExtraBoard(readJson(fixturePath(5)), copyOfRow, set));
        const QVariantMap r = b.importDataFile(url(file));

        QVERIFY2(!r.value("ok").toBool(), "un import avec des liens invalides aurait dû être refusé");
        QVERIFY(!b.lastError().isEmpty());                                      // l'erreur est consultable (avant tout export)
        const QString message = r.value("error").toString();
        QVERIFY2(message.contains(QStringLiteral("Import refusé")), qPrintable(message));
        QVERIFY2(message.contains(QStringLiteral("liens de terrains incohérents")), qPrintable(message));
        QVERIFY2(message.contains(QStringLiteral("Doublon")) || message.contains(QStringLiteral("Atelier"))
                     || message.contains(QStringLiteral("Pressing")),
                 qPrintable(message));                                          // le terrain fautif est nommé
        QCOMPARE(snapshot(b), before);                                          // aucune donnée modifiée
        QCOMPARE(backupNames(), backupsBefore);                                 // aucune sauvegarde créée ni évincée
    }

    // Avant le schéma 5, plusieurs schémas de référence pour un même exercice étaient possibles : une ancienne
    // sauvegarde valable ne doit pas être refusée (la règle ne vaut que pour les fichiers du schéma 5 ou plus récent).
    void olderFileWithSeveralReferenceBoardsIsStillAccepted()
    {
        Backend b(m_db.handle());
        QJsonObject root = withExtraBoard(readJson(fixturePath(4)), 0, {{"title", QJsonValue(QStringLiteral("Second schéma"))}});
        const QVariantMap r = b.importDataFile(url(tmpFile(root)));
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(snapshot(b).value("tactical_board").toObject().value("rows").toArray().size(), 2);
    }

    void validBoardLinksAreAccepted()
    {
        Backend b(m_db.handle());
        const QVariantMap r = b.importDataFile(url(fixturePath(5)));
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(snapshot(b).value("tactical_board").toObject().value("rows").toArray().size(), 2);
    }

    // Un import refusé ne doit pas faire tourner les trois sauvegardes ; un import réussi garde les trois plus récentes.
    void backupsRotateOnlyAfterASuccessfulImport()
    {
        Backend b(m_db.handle());
        QVERIFY(b.importDataFile(url(fixturePath(5))).value("ok").toBool());
        QVERIFY(backupDir().exists());
        const QStringList fake = {"avant-import-20200101-000001.footcoach", "avant-import-20200101-000002.footcoach",
                                  "avant-import-20200101-000003.footcoach"};
        for (const QString &n : fake) {
            QFile f(backupDir().filePath(n));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("{}");
        }
        const QStringList existing = backupNames();

        // Import refusé (liens invalides) : toutes les sauvegardes sont conservées, aucune n'est ajoutée.
        const QString bad = tmpFile(withExtraBoard(readJson(fixturePath(5)), 1, {{"exercise_id", QJsonValue(18)}}));
        QVERIFY(!b.importDataFile(url(bad)).value("ok").toBool());
        QCOMPARE(backupNames(), existing);

        // Import refusé pour une autre raison (colonne inconnue) : idem.
        QJsonObject root = readJson(fixturePath(5));
        QJsonObject tables = root.value("tables").toObject();
        QJsonObject tb = tables.value("team").toObject();
        QJsonArray cols = tb.value("columns").toArray();
        cols.append("colonne_fantome");
        tb["columns"] = cols;
        tables["team"] = tb;
        root["tables"] = tables;
        QVERIFY(!b.importDataFile(url(tmpFile(root))).value("ok").toBool());
        QCOMPARE(backupNames(), existing);

        // Import réussi : une nouvelle sauvegarde, et seules les trois plus récentes restent.
        const QVariantMap ok = b.importDataFile(url(fixturePath(5)));
        QVERIFY(ok.value("ok").toBool());
        const QStringList after = backupNames();
        QCOMPARE(after.size(), 3);
        QVERIFY(after.contains(ok.value("backupName").toString()));
        QVERIFY(!after.contains(fake.at(0)));
    }

    // Deux imports dans la même seconde : la sauvegarde existante n'est jamais écrasée ni supprimée, et la rotation
    // supprime bien la plus ancienne (« …-120000 » avant « …-120000-2 »).
    void backupNamesNeverCollide()
    {
        Backend b(m_db.handle());
        QVERIFY(b.importDataFile(url(fixturePath(5))).value("ok").toBool());   // la base n'est plus vide
        QDir d = backupDir();
        for (const QString &n : backupNames())
            QVERIFY(QFile::remove(d.filePath(n)));
        QVERIFY(d.exists());

        // Les trois premiers noms de la seconde courante sont déjà pris (si la seconde change pendant le test, le nom
        // du nouvel import diffère et le test reste valable : il vérifie alors seulement la rotation).
        QStringList taken;
        const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
        for (const QString &suffix : {QString(), QString("-2"), QString("-3")}) {
            const QString n = "avant-import-" + stamp + suffix + ".footcoach";
            QFile f(d.filePath(n));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("sauvegarde-existante-" + n.toUtf8());
            taken << n;
        }
        auto untouched = [&]() {
            for (const QString &n : taken) {
                QFile f(d.filePath(n));
                if (!f.exists())
                    return false;
                f.open(QIODevice::ReadOnly);
                if (f.readAll() != "sauvegarde-existante-" + n.toUtf8())
                    return false;
            }
            return true;
        };

        // Import refusé : aucune sauvegarde existante n'est supprimée (la sienne n'est pas celle d'un autre).
        const QString bad = tmpFile(withExtraBoard(readJson(fixturePath(5)), 1, {{"exercise_id", QJsonValue(18)}}));
        QVERIFY(!b.importDataFile(url(bad)).value("ok").toBool());
        QVERIFY2(untouched(), "un import refusé a modifié une sauvegarde existante");
        QCOMPARE(backupNames().size(), taken.size());

        // Import réussi dans la même seconde : nouveau nom, aucune sauvegarde existante tronquée.
        const QVariantMap ok = b.importDataFile(url(fixturePath(5)));
        QVERIFY(ok.value("ok").toBool());
        const QString created = ok.value("backupName").toString();
        QVERIFY2(!taken.contains(created), qPrintable("nom déjà pris : " + created));
        QVERIFY(QFile::exists(d.filePath(created)));
        QVERIFY(QFileInfo(d.filePath(created)).size() > 100);          // le vrai export, pas un marqueur
        // Rotation : trois restent, la nouvelle en fait partie, les plus anciennes ont disparu en premier.
        const QStringList after = backupNames();
        QCOMPARE(after.size(), 3);
        QVERIFY(after.contains(created));
        for (const QString &n : after)
            if (n != created && taken.contains(n)) {
                QFile f(d.filePath(n));
                f.open(QIODevice::ReadOnly);
                QCOMPARE(f.readAll(), "sauvegarde-existante-" + n.toUtf8());   // intacte
            }
        QVERIFY(!after.contains(taken.first()));
        for (const QString &n : backupNames())
            QFile::remove(d.filePath(n));                           // ne pas peser sur les tests suivants
    }

    void notAnExportAtAll()
    {
        Backend b(m_db.handle());
        const QJsonObject before = snapshot(b);
        auto tryBytes = [&](const QByteArray &bytes) {
            const QString p = m_dir.filePath(QStringLiteral("raw%1").arg(++m_counter));
            QFile f(p);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(bytes);
            f.close();
            QVERIFY2(!b.importDataFile(p).value("ok").toBool(), bytes.constData());
            QVERIFY2(!b.inspectDataFile(p).value("ok").toBool(), bytes.constData());
        };
        tryBytes("");
        tryBytes("pas du json");
        tryBytes("[1,2,3]");
        tryBytes("{}");
        tryBytes("{\"format\":\"footcoach-data\"}");
        QVERIFY(!b.importDataFile(m_dir.filePath(QStringLiteral("absent.footcoach"))).value("ok").toBool());
        QCOMPARE(snapshot(b), before);
    }

    // ----- Comportements actuels qui expliquent les règles de la version 2 (voir docs/versionnage.md) -----
    // Le lecteur actuel ignore les clés inconnues de premier niveau : un export partiel écrit en version 1
    // serait donc lu comme un export complet par la 0.8.0. Les exports partiels doivent être en version 2.
    void unknownTopLevelKeysAreIgnoredByV1Readers()
    {
        Backend b(m_db.handle());
        QJsonObject root = readJson(fixturePath(4));
        root["kind"] = QStringLiteral("selection");
        root["scope"] = QStringLiteral("exercises");
        QVERIFY(b.importDataFile(url(tmpFile(root))).value("ok").toBool());
    }

    // Un fichier version 1 dont toutes les tables sont vides EST un export complet d'une base vide : il remplace tout.
    void emptyFullExportReplacesEverything()
    {
        Backend b(m_db.handle());
        QVERIFY(b.importDataFile(url(fixturePath(4))).value("ok").toBool());
        QJsonObject root = readJson(fixturePath(4));
        QJsonObject tables = root.value("tables").toObject();
        for (const QString &t : kTables) {
            QJsonObject tb = tables.value(t).toObject();
            tb["rows"] = QJsonArray();
            tables[t] = tb;
        }
        root["tables"] = tables;
        QVERIFY(b.importDataFile(url(tmpFile(root))).value("ok").toBool());
        QCOMPARE(b.teams().size(), 0);
        QCOMPARE(b.exercises(true).size(), 0);
    }

    // Une sauvegarde automatique est créée avant un remplacement, et elle est elle-même un fichier valide.
    void automaticBackupIsAValidFile()
    {
        Backend b(m_db.handle());
        b.saveTeam({{"name", "Avant"}});
        const QVariantMap r = b.importDataFile(url(fixturePath(4)));
        QVERIFY(r.value("ok").toBool());
        const QString name = r.value("backupName").toString();
        QVERIFY(!name.isEmpty());
        const QString path = QFileInfo(m_db.path()).absolutePath() + "/sauvegardes/" + name;
        const QJsonObject backup = readJson(path);
        QCOMPARE(backup.value("format").toString(), QStringLiteral("footcoach-data"));
        QCOMPARE(backup.value("version").toInt(), 1);
        QVERIFY(b.inspectDataFile(path).value("ok").toBool());
        // Revenir en arrière fonctionne.
        QVERIFY(b.importDataFile(url(path)).value("ok").toBool());
        QCOMPARE(b.teams().size(), 1);
        QCOMPARE(b.teams().at(0).toMap().value("name").toString(), QStringLiteral("Avant"));
    }
};

QTEST_MAIN(DataFormatTest)
#include "tst_dataformat.moc"
