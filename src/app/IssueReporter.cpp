#include "IssueReporter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#ifndef FOOTCOACH_BUG_TOKEN
#define FOOTCOACH_BUG_TOKEN ""
#endif
#ifndef FOOTCOACH_BUG_REPO
#define FOOTCOACH_BUG_REPO "geekons77899377/footcoach"
#endif

namespace {
constexpr int kTimeoutMs = 20000;
constexpr int kMaxTitle = 200;
constexpr int kMaxBody = 20000;
}

IssueReporter::IssueReporter(QObject *parent)
    : IssueReporter(QUrl(QStringLiteral("https://api.github.com/repos/%1/issues").arg(QStringLiteral(FOOTCOACH_BUG_REPO))),
                    QStringLiteral(FOOTCOACH_BUG_TOKEN), parent)
{
}

IssueReporter::IssueReporter(const QUrl &endpoint, const QString &token, QObject *parent)
    : QObject(parent), m_net(new QNetworkAccessManager(this)), m_timeout(new QTimer(this)), m_endpoint(endpoint),
      m_token(token.trimmed())
{
    m_timeout->setSingleShot(true);
    m_timeout->setInterval(kTimeoutMs);
    connect(m_timeout, &QTimer::timeout, this, [this]() {
        if (m_reply)
            m_reply->abort();                 // finished() sera émis avec une erreur d'annulation : message de délai
    });
}

void IssueReporter::fail(const QString &message)
{
    emit finished(false, message, QString());
}

void IssueReporter::send(const QString &title, const QString &body, const QStringList &labels)
{
    if (!available()) {
        fail(tr("L'envoi de signalements n'est pas disponible dans cette version."));
        return;
    }
    if (m_reply) {
        fail(tr("Un envoi est déjà en cours."));
        return;
    }
    const QString t = title.simplified().left(kMaxTitle);
    if (t.isEmpty()) {
        fail(tr("Indiquez un titre."));
        return;
    }
    QJsonObject json;
    json.insert(QStringLiteral("title"), t);
    json.insert(QStringLiteral("body"), body.left(kMaxBody));
    if (!labels.isEmpty())
        json.insert(QStringLiteral("labels"), QJsonArray::fromStringList(labels));

    QNetworkRequest req(m_endpoint);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    req.setRawHeader("User-Agent", "FootCoach");
    req.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    req.setTransferTimeout(kTimeoutMs);

    m_reply = m_net->post(req, QJsonDocument(json).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::finished, this, &IssueReporter::onReplyFinished);
    m_timeout->start();
    emit busyChanged();
}

void IssueReporter::onReplyFinished()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    m_timeout->stop();
    reply->deleteLater();
    emit busyChanged();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::NoError && status == 201) {
        const QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
        emit finished(true, tr("Signalement envoyé, merci !"), o.value(QStringLiteral("html_url")).toString());
        return;
    }
    // Messages sans détail technique ni contenu de la réponse (qui pourrait citer le jeton ou des en-têtes).
    QString message;
    if (status == 401 || status == 403 || status == 404)
        message = tr("Le service de signalement a refusé la demande (accès). Utilisez l'envoi par e-mail.");
    else if (status == 422)
        message = tr("Le signalement a été refusé (contenu invalide). Vérifiez le titre et la description.");
    else if (status == 429)
        message = tr("Trop d'envois : réessayez plus tard.");
    else if (reply->error() == QNetworkReply::OperationCanceledError || reply->error() == QNetworkReply::TimeoutError)
        message = tr("Le service n'a pas répondu à temps. Vérifiez votre connexion et réessayez.");
    else if (status == 0)
        message = tr("Impossible de joindre le service. Vérifiez votre connexion et réessayez.");
    else
        message = tr("Le service a répondu par une erreur (%1). Réessayez plus tard.").arg(status);
    fail(message);
}
