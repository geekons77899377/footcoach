#pragma once

#include "core/BoardRepository.h"

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Passerelle entre l'interface QML et la base de données (schémas tactiques pour l'instant).
//
// Cette classe ne dépend que de QtCore/QtSql : elle est exposée à QML par main.cpp (setContextProperty) et se
// teste avec Qt Test sans interface graphique. Le contenu d'un schéma circule en JSON (Tactics::toJson) : QML
// l'édite en mémoire, ce qui est enregistré est toujours revalidé ici avant d'aller dans la base.
class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit Backend(const QSqlDatabase &db, QObject *parent = nullptr);

    QString lastError() const { return m_lastError; }

    // Liste des schémas, triés par titre : {id, title, updatedAt, exerciseId, sessionId, readable}.
    Q_INVOKABLE QVariantList boards();
    // {id, title, json, readable, error}. readable == false : le JSON stocké est illisible (le schéma est alors
    // à ne pas modifier, pour ne pas écraser ce qu'il contient). Map vide si l'id n'existe pas.
    Q_INVOKABLE QVariantMap board(int id);

    Q_INVOKABLE int createBoard(const QString &title);     // id du nouveau schéma, -1 en cas d'échec
    Q_INVOKABLE int duplicateBoard(int id);                // id de la copie, -1 en cas d'échec
    Q_INVOKABLE bool renameBoard(int id, const QString &title);
    Q_INVOKABLE bool deleteBoard(int id);
    // Enregistre le contenu (JSON de Tactics::Board). Refusé, sans rien écrire, si le JSON n'est pas un schéma valide
    // ou si le schéma stocké est illisible.
    Q_INVOKABLE bool saveBoard(int id, const QString &json);

    // Formations de départ : noms, et JSON {"items": [...]} au format du schéma (joueurs numérotés,
    // adversaires en miroir si demandé). Chaîne vide si le nom est inconnu.
    Q_INVOKABLE QStringList formationNames() const;
    Q_INVOKABLE QString formationJson(const QString &name, bool withOpponents) const;
    // Couleurs proposées : [{name, color}].
    Q_INVOKABLE QVariantList palette() const;

signals:
    void lastErrorChanged();

private:
    bool find(int id, BoardRecord *out);
    void setError(const QString &message);

    QSqlDatabase m_db;
    BoardRepository m_repo;
    QString m_lastError;
};
