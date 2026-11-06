#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

#include "app/IssueReporter.h"

// Faux serveur GitHub local : une requête à la fois, réponse configurable (status 0 = ne répond jamais).
class FakeServer : public QTcpServer
{
public:
    int status = 201;
    QByteArray replyBody = R"({"html_url":"https://github.com/x/y/issues/7"})";
    QByteArray request;
    FakeServer()
    {
        connect(this, &QTcpServer::newConnection, this, [this]() {
            QTcpSocket *s = nextPendingConnection();
            connect(s, &QTcpSocket::readyRead, this, [this, s]() {
                request += s->readAll();
                const int hdrEnd = request.indexOf("\r\n\r\n");
                if (hdrEnd < 0)
                    return;
                const QByteArray head = request.left(hdrEnd).toLower();
                const int cl = head.indexOf("content-length:");
                const int len = cl < 0 ? 0 : head.mid(cl + 15, head.indexOf("\r\n", cl) - cl - 15).trimmed().toInt();
                if (request.size() < hdrEnd + 4 + len || status == 0)
                    return;
                s->write("HTTP/1.1 " + QByteArray::number(status) + " X\r\nContent-Type: application/json\r\nContent-Length: "
                         + QByteArray::number(replyBody.size()) + "\r\nConnection: close\r\n\r\n" + replyBody);
                s->disconnectFromHost();
            });
        });
    }
    QUrl url() const { return QUrl(QStringLiteral("http://127.0.0.1:%1/repos/o/r/issues").arg(serverPort())); }
};

class TstIssueReporter : public QObject
{
    Q_OBJECT
private slots:
    void withoutTokenUnavailable()
    {
        IssueReporter r(QUrl(QStringLiteral("http://127.0.0.1:1/")), QString());
        QVERIFY(!r.available());
        QSignalSpy spy(&r, &IssueReporter::finished);
        r.send(QStringLiteral("t"), QStringLiteral("b"), {});
        QCOMPARE(spy.count(), 1);
        QVERIFY(!spy.at(0).at(0).toBool());
    }
    void emptyTitleRefused()
    {
        FakeServer srv;
        QVERIFY(srv.listen(QHostAddress::LocalHost));
        IssueReporter r(srv.url(), QStringLiteral("tok"));
        QSignalSpy spy(&r, &IssueReporter::finished);
        r.send(QStringLiteral("   "), QStringLiteral("b"), {});
        QCOMPARE(spy.count(), 1);
        QVERIFY(!spy.at(0).at(0).toBool());
        QVERIFY(srv.request.isEmpty());
    }
    void successSendsHeadersAndBody()
    {
        FakeServer srv;
        QVERIFY(srv.listen(QHostAddress::LocalHost));
        IssueReporter r(srv.url(), QStringLiteral("secret-token"));
        QSignalSpy spy(&r, &IssueReporter::finished);
        r.send(QStringLiteral("SUP - Bug"), QStringLiteral("Détail é"), {QStringLiteral("bug")});
        QVERIFY(r.busy());
        QVERIFY(spy.wait(5000));
        QVERIFY(!r.busy());
        QVERIFY(spy.at(0).at(0).toBool());
        QCOMPARE(spy.at(0).at(2).toString(), QStringLiteral("https://github.com/x/y/issues/7"));
        const QByteArray req = srv.request;
        QVERIFY(req.startsWith("POST /repos/o/r/issues"));
        QVERIFY(req.toLower().contains("authorization: bearer secret-token"));
        QVERIFY(req.toLower().contains("accept: application/vnd.github+json"));
        QVERIFY(req.toLower().contains("x-github-api-version:"));
        const QJsonObject o = QJsonDocument::fromJson(req.mid(req.indexOf("\r\n\r\n") + 4)).object();
        QCOMPARE(o.value("title").toString(), QStringLiteral("SUP - Bug"));
        QCOMPARE(o.value("body").toString(), QStringLiteral("Détail é"));
        QCOMPARE(o.value("labels").toArray().at(0).toString(), QStringLiteral("bug"));
    }
    void errorStatusesGiveMessagesWithoutToken()
    {
        for (int code : {401, 422, 500}) {
            FakeServer srv;
            srv.status = code;
            srv.replyBody = R"({"message":"secret-token leaked"})";
            QVERIFY(srv.listen(QHostAddress::LocalHost));
            IssueReporter r(srv.url(), QStringLiteral("secret-token"));
            QSignalSpy spy(&r, &IssueReporter::finished);
            r.send(QStringLiteral("t"), QStringLiteral("b"), {});
            QVERIFY(spy.wait(5000));
            QVERIFY(!spy.at(0).at(0).toBool());
            const QString msg = spy.at(0).at(1).toString();
            QVERIFY(!msg.isEmpty());
            QVERIFY(!msg.contains(QStringLiteral("secret-token")));
        }
    }
    void unreachableServer()
    {
        quint16 port;
        { QTcpServer t; QVERIFY(t.listen(QHostAddress::LocalHost)); port = t.serverPort(); }
        IssueReporter r(QUrl(QStringLiteral("http://127.0.0.1:%1/i").arg(port)), QStringLiteral("tok"));
        QSignalSpy spy(&r, &IssueReporter::finished);
        r.send(QStringLiteral("t"), QStringLiteral("b"), {});
        QVERIFY(spy.wait(5000));
        QVERIFY(!spy.at(0).at(0).toBool());
        QVERIFY(!r.busy());
    }
    void secondSendWhileBusyRefused()
    {
        FakeServer srv;
        srv.status = 0;       // ne répond pas
        QVERIFY(srv.listen(QHostAddress::LocalHost));
        IssueReporter r(srv.url(), QStringLiteral("tok"));
        QSignalSpy spy(&r, &IssueReporter::finished);
        r.send(QStringLiteral("t"), QStringLiteral("b"), {});
        QVERIFY(r.busy());
        r.send(QStringLiteral("t2"), QStringLiteral("b"), {});
        QCOMPARE(spy.count(), 1);
        QVERIFY(!spy.at(0).at(0).toBool());
    }
};

QTEST_MAIN(TstIssueReporter)
#include "tst_issuereporter.moc"
