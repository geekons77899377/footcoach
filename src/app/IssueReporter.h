#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

// Envoi d'un signalement d'assistance : crée un ticket (issue) sur le dépôt GitHub de FootCoach par l'API REST, sans
// navigateur. Seul réseau de l'application ; il n'envoie que le texte relu et validé par l'utilisateur.
//
// Le jeton (fine-grained, permission « Issues : écriture » sur ce seul dépôt, rien d'autre) est injecté à la compilation
// (-DFOOTCOACH_BUG_TOKEN=...) et n'est jamais dans le dépôt. Il reste extractible de l'application installée : sa portée
// limitée (création de tickets) est la seule protection, et il se révoque depuis GitHub. Sans jeton, available est faux.
class IssueReporter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit IssueReporter(QObject *parent = nullptr);
    // Tests : serveur local et jeton choisis.
    IssueReporter(const QUrl &endpoint, const QString &token, QObject *parent = nullptr);

    bool available() const { return !m_token.isEmpty(); }
    bool busy() const { return m_reply != nullptr; }

    // Crée le ticket. Une seule requête à la fois ; le résultat arrive par finished(). labels : étiquettes du dépôt.
    Q_INVOKABLE void send(const QString &title, const QString &body, const QStringList &labels);

signals:
    void busyChanged();
    // ok : ticket créé (number, url du ticket). Sinon message à afficher (sans le jeton ni la réponse brute).
    void finished(bool ok, const QString &message, const QString &url);

private:
    void fail(const QString &message);
    void onReplyFinished();

    QNetworkAccessManager *m_net;
    QNetworkReply *m_reply = nullptr;
    QTimer *m_timeout;
    QUrl m_endpoint;
    QString m_token;
};
